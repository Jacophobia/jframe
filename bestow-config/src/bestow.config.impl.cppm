// bestow-config/src/bestow.config.impl.cppm
// Config system implementation - Lua-based configuration loading

module;

#include <kangaru/kangaru.hpp>
#include <bestow/sol2_compat.hpp>
#include <spdlog/spdlog.h>

export module bestow.config.impl;

import std;
import bestow.types;
import bestow.config;
import bestow.assets;
import bestow.services;

export namespace bestow {

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
    // Asset System Integration
    //==========================================================================

    void setAssetSystem(IAssetSystem* assetSystem) { assetSystem_ = assetSystem; }

    //==========================================================================
    // EventSystem Integration
    //==========================================================================

    void setEventSystem(IEventSystem* events) override { eventSystem_ = events; }

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

    //==========================================================================
    // Unified Lua Parsing (For other systems to use)
    //==========================================================================

    /// Parse Lua code string and return the result object
    std::optional<sol::object> parseLuaString(
        const std::string& luaCode,
        const std::string& description = "lua") override;

    /// Parse Lua asset and return the result object
    std::optional<sol::object> parseLuaAsset(
        AssetHandle luaAsset,
        const std::string& description = "lua") override;

    /// Execute Lua code string (no return value expected)
    bool executeLuaString(
        const std::string& luaCode,
        const std::string& description = "lua") override;

    /// Get direct access to Lua state (use with caution)
    sol::state* getLuaState() override;

private:
    // Lua parsing helpers
    void parseLuaTable(sol::table& table, const std::string& prefix,
                       const std::string& sourcePath);
    void parseLuaValue(const std::string& key, sol::object& value,
                       const std::string& sourcePath);

    // Notification helpers
    void notifyChange(const ConfigKey& key);

    // Configuration storage
    std::unordered_map<ConfigKey, ConfigEntry> config_;

    // Asset system integration
    IAssetSystem* assetSystem_ = nullptr;

    // Event system integration
    IEventSystem* eventSystem_ = nullptr;

    // Loaded file tracking
    struct LoadedFile {
        std::string path;
        AssetHandle assetHandle;  // Track asset handle instead of file time
        Timestamp loadTime = 0.0f;
    };
    std::vector<LoadedFile> loadedFiles_;

    // Asset subscription tracking
    std::unordered_map<AssetHandle, SubscriptionId, AssetHandleHash> assetSubscriptions_;

    // Lua state (sandboxed)
    sol::state lua_;
    bool luaInitialized_ = false;

    // Hot reload
    bool hotReloadEnabled_ = false;

    // Subscriptions
    std::vector<ConfigSubscription> subscriptions_;
    SubscriptionId nextSubscriptionId_ = 1;

    // Current time tracking
    Timestamp currentTime_ = 0.0f;
};

// Kangaru service definitions
struct ConfigSystemService : kgr::single_service<ConfigSystem>, kgr::overrides<IConfigSystemService> {
    // ConfigSystem depends on AssetSystem for file I/O
    template<typename... T>
    static auto construct(T&&... args) -> decltype(kgr::inject(std::forward<T>(args)...)) {
        return kgr::inject(std::forward<T>(args)...);
    }
};

}  // namespace bestow
