// jframe-config/src/jframe.config.impl.cppm
// Config system implementation - Lua-based configuration loading

module;

#include <jframe/sol2_compat.hpp>
#include <spdlog/spdlog.h>

export module jframe.config.impl;

import std;
import jframe.types;
import jframe.config;

export namespace jframe {

/// Internal config value storage with type tracking
struct ConfigEntry {
    std::any value;
    std::string type;  // "float", "int", "bool", "string", "float_array", "int_array", "string_array"
    std::string sourcePath;
    Timestamp loadTime = 0.0f;
};

/// Subscription entry for change notifications
struct ConfigSubscription {
    SubscriptionId id;
    std::string keyPrefix;  // Empty = all changes
    ConfigChangeCallback callback;
};

class ConfigSystem : public IConfigSystem {
public:
    ConfigSystem() = default;
    ~ConfigSystem() override = default;

    //==========================================================================
    // Lifecycle
    //==========================================================================

    bool initialize() override;
    void update(DeltaTime dt) override;
    void shutdown() override;

    //==========================================================================
    // Configuration Loading
    //==========================================================================

    bool loadConfig(const std::string& filePath) override;
    bool loadConfigAsset(AssetHandle configAsset) override;
    bool reloadAll() override;
    bool reloadConfig(const std::string& filePath) override;

    //==========================================================================
    // Type-Safe Value Access
    //==========================================================================

    std::optional<float> getFloat(const ConfigKey& key) const override;
    std::optional<int> getInt(const ConfigKey& key) const override;
    std::optional<bool> getBool(const ConfigKey& key) const override;
    std::optional<std::string> getString(const ConfigKey& key) const override;

    float getFloatOr(const ConfigKey& key, float defaultValue) const override;
    int getIntOr(const ConfigKey& key, int defaultValue) const override;
    bool getBoolOr(const ConfigKey& key, bool defaultValue) const override;
    std::string getStringOr(const ConfigKey& key,
                            const std::string& defaultValue) const override;

    //==========================================================================
    // Array/List Access
    //==========================================================================

    std::vector<int> getIntArray(const ConfigKey& key) const override;
    std::vector<float> getFloatArray(const ConfigKey& key) const override;
    std::vector<std::string> getStringArray(const ConfigKey& key) const override;

    //==========================================================================
    // Runtime Value Modification
    //==========================================================================

    void setFloat(const ConfigKey& key, float value) override;
    void setInt(const ConfigKey& key, int value) override;
    void setBool(const ConfigKey& key, bool value) override;
    void setString(const ConfigKey& key, const std::string& value) override;

    //==========================================================================
    // State Queries
    //==========================================================================

    bool hasKey(const ConfigKey& key) const override;
    std::vector<ConfigKey> getKeysWithPrefix(const std::string& prefix) const override;
    std::vector<std::string> getLoadedConfigs() const override;
    ConfigMetadata getMetadata(const std::string& filePath) const override;

    //==========================================================================
    // Hot Reload
    //==========================================================================

    void enableHotReload(bool enable) override;
    bool isHotReloadEnabled() const override;

    //==========================================================================
    // Change Notifications
    //==========================================================================

    SubscriptionId onConfigChanged(ConfigChangeCallback callback) override;
    SubscriptionId onKeyChanged(const std::string& keyPrefix,
                                 ConfigChangeCallback callback) override;
    void unsubscribe(SubscriptionId id) override;

private:
    // Lua parsing helpers
    void parseLuaTable(sol::table& table, const std::string& prefix,
                       const std::string& sourcePath);
    void parseLuaValue(const std::string& key, sol::object& value,
                       const std::string& sourcePath);

    // Notification helpers
    void notifyChange(const ConfigKey& key);

    // Hot reload helpers
    bool checkFileModified(const std::string& filePath);

    // Configuration storage
    std::unordered_map<ConfigKey, ConfigEntry> config_;

    // Loaded file tracking
    struct LoadedFile {
        std::string path;
        std::filesystem::file_time_type lastModified;
        Timestamp loadTime = 0.0f;
    };
    std::vector<LoadedFile> loadedFiles_;

    // Lua state (sandboxed)
    sol::state lua_;
    bool luaInitialized_ = false;

    // Hot reload
    bool hotReloadEnabled_ = false;
    float hotReloadCheckInterval_ = 1.0f;  // Check every second
    float timeSinceLastCheck_ = 0.0f;

    // Subscriptions
    std::vector<ConfigSubscription> subscriptions_;
    SubscriptionId nextSubscriptionId_ = 1;

    // Current time tracking
    Timestamp currentTime_ = 0.0f;
};

// Factory function
inline std::unique_ptr<IConfigSystem> createConfigSystem() {
    return std::make_unique<ConfigSystem>();
}

}  // namespace jframe
