// bestow-state/src/StateSystem.cpp
// State system implementation — in-memory cache with async SQLite persistence

module;

#include <spdlog/spdlog.h>

module bestow.state.impl;

namespace bestow {

StateSystem::~StateSystem() {
    worker_.stop();
}

void StateSystem::update(DeltaTime dt) {
    // Track playtime
    sessionPlaytimeSeconds_ += dt;

    // Poll async completions from worker thread
    auto completed = worker_.pollCompleted();
    for (auto& op : completed) {
        if (op.type == StateWorker::CompletedOp::Type::Restore && op.success) {
            // Replace cache with restored data
            cache_ = std::move(op.restoredData);

            // Read metadata for playtime restoration
            ensureDbOpen();
            auto meta = mainDb_.readMetadata(op.slot);
            if (meta) {
                loadedPlaytimeSeconds_ = meta->playtimeSeconds;
                sessionPlaytimeSeconds_ = 0.0f;
                completionPercentage_ = meta->completionPercentage;
                if (meta->levelName) {
                    currentLevel_ = *meta->levelName;
                }

                // Run migrations if needed
                if (meta->formatVersion < currentFormatVersion_) {
                    for (const auto& mig : migrations_) {
                        if (mig.fromVersion == meta->formatVersion &&
                            mig.toVersion <= currentFormatVersion_) {
                            spdlog::info("[State] Running migration v{} -> v{}",
                                         mig.fromVersion, mig.toVersion);
                            mig.fn();
                        }
                    }
                }
            }

            // Notify registered IStatefuls
            for (auto* stateful : statefuls_) {
                stateful->onRestore(*this);
            }
        }

        // Fire callback on main thread
        if (op.callback) {
            op.callback(op.success, op.error);
        }
    }

    // Auto-commit timer
    if (autoCommitEnabled_) {
        autoCommitTimer_ += dt;
        if (autoCommitTimer_ >= static_cast<float>(autoCommitInterval_.count())) {
            commit(autoCommitSlot_, "Auto Commit", {});
            autoCommitTimer_ = 0.0f;
        }
    }
}

//==========================================================================
// Key-Value Data (in-memory cache only — no I/O)
//==========================================================================

void StateSystem::setNumber(const std::string& key, double value) {
    cache_[key] = CacheEntry{ValueType::Number, value, {}};
}

double StateSystem::getNumber(const std::string& key, double defaultValue) const {
    auto it = cache_.find(key);
    if (it == cache_.end() || it->second.type != ValueType::Number) return defaultValue;
    return it->second.numValue;
}

void StateSystem::setString(const std::string& key, const std::string& value) {
    cache_[key] = CacheEntry{ValueType::String, 0.0, value};
}

std::string StateSystem::getString(const std::string& key, const std::string& defaultValue) const {
    auto it = cache_.find(key);
    if (it == cache_.end() || it->second.type != ValueType::String) return defaultValue;
    return it->second.strValue;
}

void StateSystem::setBool(const std::string& key, bool value) {
    cache_[key] = CacheEntry{ValueType::Bool, value ? 1.0 : 0.0, {}};
}

bool StateSystem::getBool(const std::string& key, bool defaultValue) const {
    auto it = cache_.find(key);
    if (it == cache_.end() || it->second.type != ValueType::Bool) return defaultValue;
    return it->second.numValue != 0.0;
}

void StateSystem::setJsonData(const std::string& key, const std::string& json) {
    cache_[key] = CacheEntry{ValueType::Json, 0.0, json};
}

std::string StateSystem::getJsonData(const std::string& key) const {
    auto it = cache_.find(key);
    if (it == cache_.end() || it->second.type != ValueType::Json) return "{}";
    return it->second.strValue;
}

bool StateSystem::hasData(const std::string& key) const {
    return cache_.contains(key);
}

void StateSystem::removeData(const std::string& key) {
    cache_.erase(key);
}

void StateSystem::clearData() {
    cache_.clear();
}

//==========================================================================
// Commit/Restore
//==========================================================================

void StateSystem::commit(StateSlot slot, const std::string& name, StateCallback cb) {
    // Notify IStatefuls to write their state (prefixed keys)
    for (auto* stateful : statefuls_) {
        stateful->onCommit(*this);
    }

    // Snapshot the cache
    auto snapshot = cache_;

    // Build metadata
    auto meta = buildMetadata(slot, name);

    // Start worker if needed
    if (!workerStarted_) {
        worker_.start(getDbPath());
        workerStarted_ = true;
    }

    // Enqueue to worker thread
    StateWorker::CommitCommand cmd;
    cmd.slot = slot;
    cmd.data = std::move(snapshot);
    cmd.metadata = meta;
    cmd.callback = std::move(cb);
    worker_.enqueueCommit(std::move(cmd));
}

void StateSystem::restore(StateSlot slot, StateCallback cb) {
    // Start worker if needed
    if (!workerStarted_) {
        worker_.start(getDbPath());
        workerStarted_ = true;
    }

    StateWorker::RestoreCommand cmd;
    cmd.slot = slot;
    cmd.callback = std::move(cb);
    worker_.enqueueRestore(std::move(cmd));
}

bool StateSystem::deleteSlot(StateSlot slot) {
    ensureDbOpen();
    return mainDb_.deleteSlot(slot);
}

void StateSystem::quickCommit(StateCallback cb) {
    commit(StateSlots::QuickCommit, "Quick Save", std::move(cb));
}

void StateSystem::quickRestore(StateCallback cb) {
    restore(StateSlots::QuickCommit, std::move(cb));
}

//==========================================================================
// Auto-Commit
//==========================================================================

void StateSystem::enableAutoCommit(std::chrono::seconds interval) {
    autoCommitInterval_ = interval;
    autoCommitEnabled_ = true;
    autoCommitTimer_ = 0.0f;
}

void StateSystem::disableAutoCommit() {
    autoCommitEnabled_ = false;
}

//==========================================================================
// Metadata Queries (sync — reads from main-thread SQLite connection)
//==========================================================================

std::vector<StateMetadata> StateSystem::getAllSlotMetadata() const {
    ensureDbOpen();
    return mainDb_.readAllMetadata();
}

std::optional<StateMetadata> StateSystem::getSlotMetadata(StateSlot slot) const {
    ensureDbOpen();
    return mainDb_.readMetadata(slot);
}

bool StateSystem::slotExists(StateSlot slot) const {
    ensureDbOpen();
    return mainDb_.slotExists(slot);
}

//==========================================================================
// Profile Management
//==========================================================================

void StateSystem::setActiveProfile(const std::string& profileId) {
    if (activeProfile_ == profileId) return;

    // Stop worker (closes its DB connection)
    worker_.stop();
    workerStarted_ = false;

    // Close main-thread DB
    mainDb_.close();
    mainDbOpen_ = false;

    activeProfile_ = profileId;

    // Create profile directory
    auto profilePath = savesDirectory_ / activeProfile_;
    std::filesystem::create_directories(profilePath);
}

std::string StateSystem::getActiveProfile() const {
    return activeProfile_;
}

std::vector<std::string> StateSystem::getProfiles() const {
    std::vector<std::string> profiles;
    if (!std::filesystem::exists(savesDirectory_)) return profiles;

    for (auto it = std::filesystem::directory_iterator(savesDirectory_);
         it != std::default_sentinel; ++it) {
        if (it->is_directory()) {
            profiles.push_back(it->path().filename().string());
        }
    }
    return profiles;
}

//==========================================================================
// Game Version & Playtime
//==========================================================================

void StateSystem::setGameVersion(const std::string& version) {
    gameVersion_ = version;
}

std::string StateSystem::getGameVersion() const {
    return gameVersion_;
}

std::uint64_t StateSystem::getSessionPlaytime() const {
    return static_cast<std::uint64_t>(sessionPlaytimeSeconds_);
}

std::uint64_t StateSystem::getTotalPlaytime() const {
    return loadedPlaytimeSeconds_ + static_cast<std::uint64_t>(sessionPlaytimeSeconds_);
}

void StateSystem::resetSessionPlaytime() {
    sessionPlaytimeSeconds_ = 0.0f;
    loadedPlaytimeSeconds_ = 0;
}

void StateSystem::setCompletionPercentage(float percentage) {
    completionPercentage_ = std::clamp(percentage, 0.0f, 100.0f);
}

void StateSystem::setCurrentLevel(const std::string& levelName) {
    currentLevel_ = levelName;
}

//==========================================================================
// Migration
//==========================================================================

void StateSystem::setFormatVersion(int version) {
    currentFormatVersion_ = version;
}

void StateSystem::registerMigration(int fromVersion, int toVersion,
                                     std::function<void()> fn) {
    migrations_.push_back(MigrationEntry{fromVersion, toVersion, std::move(fn)});
}

//==========================================================================
// JSON Export/Import
//==========================================================================

Result<void, StateError> StateSystem::exportToJson(StateSlot slot,
                                                    const std::string& path) const {
    ensureDbOpen();
    return JsonExporter::exportToJson(mainDb_, slot, path);
}

Result<void, StateError> StateSystem::importFromJson(StateSlot slot,
                                                      const std::string& path) {
    ensureDbOpen();
    return JsonExporter::importFromJson(mainDb_, slot, path);
}

//==========================================================================
// IStateful Registration
//==========================================================================

void StateSystem::registerStateful(IStateful* stateful) {
    statefuls_.push_back(stateful);
}

void StateSystem::unregisterStateful(IStateful* stateful) {
    std::erase(statefuls_, stateful);
}

//==========================================================================
// Private Helpers
//==========================================================================

void StateSystem::ensureDbOpen() const {
    if (mainDbOpen_) return;

    auto path = getDbPath();
    std::filesystem::create_directories(path.parent_path());
    mainDbOpen_ = mainDb_.open(path);
}

std::filesystem::path StateSystem::getDbPath() const {
    return savesDirectory_ / activeProfile_ / "state.db";
}

StateMetadata StateSystem::buildMetadata(StateSlot slot, const std::string& name) const {
    StateMetadata meta;
    meta.slot = slot;
    meta.name = name;
    meta.timestamp = std::chrono::system_clock::now();
    meta.gameVersion = gameVersion_;
    meta.playtimeSeconds = getTotalPlaytime();
    meta.completionPercentage = completionPercentage_;
    if (!currentLevel_.empty()) {
        meta.levelName = currentLevel_;
    }
    meta.formatVersion = currentFormatVersion_;
    return meta;
}

}  // namespace bestow
