// bestow-contract/src/bestow.state.cppm
// State system interface — SQLite-backed, async, Lua-first data persistence

export module bestow.state;

import std;
import bestow.types;

export namespace bestow {

struct StateMetadata {
    StateSlot slot;
    std::string name;
    std::chrono::system_clock::time_point timestamp;
    std::string gameVersion;
    std::uint64_t playtimeSeconds = 0;
    float completionPercentage = 0.0f;
    std::optional<std::string> levelName;
    int formatVersion = 1;
};

using StateCallback = std::function<void(bool success, StateError error)>;

class IStateSystem {
public:
    virtual ~IStateSystem() = default;

    //======================================================================
    // Lifecycle
    //======================================================================

    virtual void update(DeltaTime dt) = 0;

    //======================================================================
    // Key-Value Data (in-memory cache, write-back)
    //======================================================================

    virtual void setNumber(const std::string& key, double value) = 0;
    virtual double getNumber(const std::string& key, double defaultValue = 0.0) const = 0;
    virtual void setString(const std::string& key, const std::string& value) = 0;
    virtual std::string getString(const std::string& key, const std::string& defaultValue = "") const = 0;
    virtual void setBool(const std::string& key, bool value) = 0;
    virtual bool getBool(const std::string& key, bool defaultValue = false) const = 0;
    virtual void setJsonData(const std::string& key, const std::string& json) = 0;
    virtual std::string getJsonData(const std::string& key) const = 0;
    virtual bool hasData(const std::string& key) const = 0;
    virtual void removeData(const std::string& key) = 0;
    virtual void clearData() = 0;

    //======================================================================
    // Commit/Restore (async — dispatched to worker thread)
    //======================================================================

    virtual void commit(StateSlot slot, const std::string& name, StateCallback cb = {}) = 0;
    virtual void restore(StateSlot slot, StateCallback cb = {}) = 0;
    virtual bool deleteSlot(StateSlot slot) = 0;

    //======================================================================
    // Quick Commit/Restore (uses reserved slot)
    //======================================================================

    virtual void quickCommit(StateCallback cb = {}) = 0;
    virtual void quickRestore(StateCallback cb = {}) = 0;

    //======================================================================
    // Auto-Commit
    //======================================================================

    virtual void enableAutoCommit(std::chrono::seconds interval) = 0;
    virtual void disableAutoCommit() = 0;

    //======================================================================
    // Metadata Queries (sync reads from SQLite — fast for small metadata)
    //======================================================================

    virtual std::vector<StateMetadata> getAllSlotMetadata() const = 0;
    virtual std::optional<StateMetadata> getSlotMetadata(StateSlot slot) const = 0;
    virtual bool slotExists(StateSlot slot) const = 0;

    //======================================================================
    // Profile Management
    //======================================================================

    virtual void setActiveProfile(const std::string& profileId) = 0;
    virtual std::string getActiveProfile() const = 0;
    virtual std::vector<std::string> getProfiles() const = 0;

    //======================================================================
    // Game Version & Playtime Tracking
    //======================================================================

    virtual void setGameVersion(const std::string& version) = 0;
    virtual std::string getGameVersion() const = 0;
    virtual std::uint64_t getSessionPlaytime() const = 0;
    virtual std::uint64_t getTotalPlaytime() const = 0;
    virtual void resetSessionPlaytime() = 0;
    virtual void setCompletionPercentage(float percentage) = 0;
    virtual void setCurrentLevel(const std::string& levelName) = 0;

    //======================================================================
    // Schema Migration
    //======================================================================

    virtual void setFormatVersion(int version) = 0;
    virtual void registerMigration(int fromVersion, int toVersion,
                                    std::function<void()> fn) = 0;

    //======================================================================
    // JSON Export/Import (dev tools)
    //======================================================================

    virtual Result<void, StateError> exportToJson(StateSlot slot,
                                                   const std::string& path) const = 0;
    virtual Result<void, StateError> importFromJson(StateSlot slot,
                                                     const std::string& path) = 0;

    //======================================================================
    // IStateful Registration (C++ engine systems that persist state)
    //======================================================================

    virtual void registerStateful(class IStateful* stateful) = 0;
    virtual void unregisterStateful(class IStateful* stateful) = 0;
};

/// IStateful — for C++ engine systems that need to persist internal state.
/// Each stateful's keys are automatically prefixed with `_sys.<stateKey>.`
/// to avoid collisions with Lua game data.
class IStateful {
public:
    virtual ~IStateful() = default;
    virtual std::string getStateKey() const = 0;
    virtual void onCommit(IStateSystem& state) const = 0;
    virtual void onRestore(IStateSystem& state) = 0;
};

}  // namespace bestow
