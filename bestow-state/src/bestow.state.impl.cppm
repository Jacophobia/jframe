// bestow-state/src/bestow.state.impl.cppm
// State system implementation — SQLite-backed key-value persistence
//
// Note: sqlite3.h is NOT included here due to C++23 module compatibility.
// All SQLite usage is confined to StateDatabase.cpp via PIMPL.

export module bestow.state.impl;

import std;
import bestow.state;
import bestow.types;
import bestow.services;

export namespace bestow {

//==========================================================================
// CacheEntry — stores a typed value in the in-memory cache
//==========================================================================

enum class ValueType : int {
    Null = 0,
    Number = 1,
    String = 2,
    Bool = 3,
    Json = 4
};

struct CacheEntry {
    ValueType type = ValueType::Null;
    double numValue = 0.0;
    std::string strValue;
};

//==========================================================================
// StateDatabase — SQLite wrapper with PIMPL
//==========================================================================

struct StateDatabaseImpl;

class StateDatabase {
public:
    StateDatabase();
    ~StateDatabase();

    StateDatabase(const StateDatabase&) = delete;
    StateDatabase& operator=(const StateDatabase&) = delete;
    StateDatabase(StateDatabase&&) noexcept;
    StateDatabase& operator=(StateDatabase&&) noexcept;

    bool open(const std::filesystem::path& dbPath);
    void close();
    bool isOpen() const;

    bool writeSlot(StateSlot slot,
                   const std::unordered_map<std::string, CacheEntry>& data,
                   const StateMetadata& metadata);

    std::unordered_map<std::string, CacheEntry> readSlot(StateSlot slot);
    std::optional<StateMetadata> readMetadata(StateSlot slot);
    std::vector<StateMetadata> readAllMetadata();
    bool deleteSlot(StateSlot slot);
    bool slotExists(StateSlot slot);

private:
    std::unique_ptr<StateDatabaseImpl> impl_;
};

//==========================================================================
// StateWorker — background thread for async commit/restore
//==========================================================================

class StateWorker {
public:
    StateWorker();
    ~StateWorker();

    StateWorker(const StateWorker&) = delete;
    StateWorker& operator=(const StateWorker&) = delete;

    void start(const std::filesystem::path& dbPath);
    void stop();

    struct CommitCommand {
        StateSlot slot;
        std::unordered_map<std::string, CacheEntry> data;
        StateMetadata metadata;
        StateCallback callback;
    };

    struct RestoreCommand {
        StateSlot slot;
        StateCallback callback;
    };

    struct DeleteCommand {
        StateSlot slot;
    };

    void enqueueCommit(CommitCommand cmd);
    void enqueueRestore(RestoreCommand cmd);

    struct CompletedOp {
        enum class Type { Commit, Restore };
        Type type;
        bool success;
        StateError error = StateError::Success;
        StateCallback callback;
        // For restore: the loaded data
        std::unordered_map<std::string, CacheEntry> restoredData;
        StateSlot slot = 0;
    };

    // Poll completed operations (call from main thread)
    std::vector<CompletedOp> pollCompleted();

private:
    void workerLoop();

    struct Command {
        enum class Type { Commit, Restore, Delete, Stop };
        Type type;
        CommitCommand commitCmd;
        RestoreCommand restoreCmd;
        DeleteCommand deleteCmd;
    };

    std::thread thread_;
    std::mutex queueMutex_;
    std::condition_variable queueCV_;
    std::vector<Command> commandQueue_;

    std::mutex completedMutex_;
    std::vector<CompletedOp> completedOps_;

    std::filesystem::path dbPath_;
    std::atomic<bool> running_{false};
};

//==========================================================================
// JsonExporter — JSON export/import for debugging
//==========================================================================

class JsonExporter {
public:
    static Result<void, StateError> exportToJson(
        StateDatabase& db, StateSlot slot, const std::string& path);
    static Result<void, StateError> importFromJson(
        StateDatabase& db, StateSlot slot, const std::string& path);
};

//==========================================================================
// StateSystem — main implementation
//==========================================================================

class StateSystem : public IStateSystem {
public:
    StateSystem() = default;
    ~StateSystem() override;

    void update(DeltaTime dt) override;

    // Key-value data
    void setNumber(const std::string& key, double value) override;
    double getNumber(const std::string& key, double defaultValue) const override;
    void setString(const std::string& key, const std::string& value) override;
    std::string getString(const std::string& key, const std::string& defaultValue) const override;
    void setBool(const std::string& key, bool value) override;
    bool getBool(const std::string& key, bool defaultValue) const override;
    void setJsonData(const std::string& key, const std::string& json) override;
    std::string getJsonData(const std::string& key) const override;
    bool hasData(const std::string& key) const override;
    void removeData(const std::string& key) override;
    void clearData() override;

    // Commit/Restore
    void commit(StateSlot slot, const std::string& name, StateCallback cb) override;
    void restore(StateSlot slot, StateCallback cb) override;
    bool deleteSlot(StateSlot slot) override;

    // Quick commit/restore
    void quickCommit(StateCallback cb) override;
    void quickRestore(StateCallback cb) override;

    // Auto-commit
    void enableAutoCommit(std::chrono::seconds interval) override;
    void disableAutoCommit() override;

    // Metadata queries
    std::vector<StateMetadata> getAllSlotMetadata() const override;
    std::optional<StateMetadata> getSlotMetadata(StateSlot slot) const override;
    bool slotExists(StateSlot slot) const override;

    // Profile management
    void setActiveProfile(const std::string& profileId) override;
    std::string getActiveProfile() const override;
    std::vector<std::string> getProfiles() const override;

    // Game version & playtime
    void setGameVersion(const std::string& version) override;
    std::string getGameVersion() const override;
    std::uint64_t getSessionPlaytime() const override;
    std::uint64_t getTotalPlaytime() const override;
    void resetSessionPlaytime() override;
    void setCompletionPercentage(float percentage) override;
    void setCurrentLevel(const std::string& levelName) override;

    // Migration
    void setFormatVersion(int version) override;
    void registerMigration(int fromVersion, int toVersion,
                            std::function<void()> fn) override;

    // JSON export/import
    Result<void, StateError> exportToJson(StateSlot slot,
                                           const std::string& path) const override;
    Result<void, StateError> importFromJson(StateSlot slot,
                                             const std::string& path) override;

    // IStateful registration
    void registerStateful(IStateful* stateful) override;
    void unregisterStateful(IStateful* stateful) override;

private:
    void ensureDbOpen() const;
    std::filesystem::path getDbPath() const;
    StateMetadata buildMetadata(StateSlot slot, const std::string& name) const;

    // In-memory cache
    std::unordered_map<std::string, CacheEntry> cache_;

    // Background worker
    StateWorker worker_;
    bool workerStarted_ = false;

    // Main-thread database for sync metadata queries
    mutable StateDatabase mainDb_;
    mutable bool mainDbOpen_ = false;

    // Registered IStateful objects
    std::vector<IStateful*> statefuls_;

    // Migration registry
    struct MigrationEntry {
        int fromVersion;
        int toVersion;
        std::function<void()> fn;
    };
    std::vector<MigrationEntry> migrations_;
    int currentFormatVersion_ = 1;

    // Profile and config
    std::string activeProfile_ = "default";
    std::filesystem::path savesDirectory_ = "saves";

    // Game tracking
    std::string gameVersion_ = "1.0.0";
    float sessionPlaytimeSeconds_ = 0.0f;
    std::uint64_t loadedPlaytimeSeconds_ = 0;
    float completionPercentage_ = 0.0f;
    std::string currentLevel_;

    // Auto-commit
    std::chrono::seconds autoCommitInterval_{0};
    float autoCommitTimer_ = 0.0f;
    bool autoCommitEnabled_ = false;
    StateSlot autoCommitSlot_ = StateSlots::AutoCommit;
};

}  // namespace bestow
