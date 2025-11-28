// jframe-contract/src/jframe.config.cppm
// Config system interface - loads and manages Lua configuration files

module;

#include <any>
#include <functional>
#include <optional>
#include <string>
#include <vector>

export module jframe.config;

import jframe.types;

export namespace jframe {

//==============================================================================
// Config Types
//==============================================================================

using ConfigKey = std::string;
using ConfigValue = std::any;

/// Callback for config change notifications
using ConfigChangeCallback = std::function<void(const ConfigKey& key)>;

/// Metadata about a loaded configuration
struct ConfigMetadata {
    std::string sourcePath;
    Timestamp loadTime = 0.0f;
    bool isDirty = false;
};

//==============================================================================
// Config System Interface
//==============================================================================

class IConfigSystem {
public:
    virtual ~IConfigSystem() = default;

    //==========================================================================
    // Lifecycle
    //==========================================================================

    /// Initialize the config system
    virtual bool initialize() = 0;

    /// Update - checks for hot reloads if enabled
    virtual void update(DeltaTime dt) = 0;

    /// Shutdown and release resources
    virtual void shutdown() = 0;

    //==========================================================================
    // Configuration Loading
    //==========================================================================

    /// Load a Lua config file from path
    virtual bool loadConfig(const std::string& filePath) = 0;

    /// Load config from asset handle (for asset system integration)
    virtual bool loadConfigAsset(AssetHandle configAsset) = 0;

    /// Reload all loaded config files
    virtual bool reloadAll() = 0;

    /// Reload a specific config file
    virtual bool reloadConfig(const std::string& filePath) = 0;

    //==========================================================================
    // Type-Safe Value Access
    //==========================================================================

    /// Get a float value
    virtual std::optional<float> getFloat(const ConfigKey& key) const = 0;

    /// Get an int value
    virtual std::optional<int> getInt(const ConfigKey& key) const = 0;

    /// Get a bool value
    virtual std::optional<bool> getBool(const ConfigKey& key) const = 0;

    /// Get a string value
    virtual std::optional<std::string> getString(const ConfigKey& key) const = 0;

    /// Get float with default fallback
    virtual float getFloatOr(const ConfigKey& key, float defaultValue) const = 0;

    /// Get int with default fallback
    virtual int getIntOr(const ConfigKey& key, int defaultValue) const = 0;

    /// Get bool with default fallback
    virtual bool getBoolOr(const ConfigKey& key, bool defaultValue) const = 0;

    /// Get string with default fallback
    virtual std::string getStringOr(const ConfigKey& key,
                                     const std::string& defaultValue) const = 0;

    //==========================================================================
    // Array/List Access
    //==========================================================================

    /// Get array of ints (e.g., for animation frame IDs)
    virtual std::vector<int> getIntArray(const ConfigKey& key) const = 0;

    /// Get array of floats
    virtual std::vector<float> getFloatArray(const ConfigKey& key) const = 0;

    /// Get array of strings
    virtual std::vector<std::string> getStringArray(const ConfigKey& key) const = 0;

    //==========================================================================
    // Runtime Value Modification
    //==========================================================================

    /// Set a float value at runtime
    virtual void setFloat(const ConfigKey& key, float value) = 0;

    /// Set an int value at runtime
    virtual void setInt(const ConfigKey& key, int value) = 0;

    /// Set a bool value at runtime
    virtual void setBool(const ConfigKey& key, bool value) = 0;

    /// Set a string value at runtime
    virtual void setString(const ConfigKey& key, const std::string& value) = 0;

    //==========================================================================
    // State Queries
    //==========================================================================

    /// Check if a key exists
    virtual bool hasKey(const ConfigKey& key) const = 0;

    /// Get all keys matching a prefix (e.g., "player." returns all player keys)
    virtual std::vector<ConfigKey> getKeysWithPrefix(
        const std::string& prefix) const = 0;

    /// Get all loaded config file paths
    virtual std::vector<std::string> getLoadedConfigs() const = 0;

    /// Get metadata for a config file
    virtual ConfigMetadata getMetadata(const std::string& filePath) const = 0;

    //==========================================================================
    // Hot Reload (Development)
    //==========================================================================

    /// Enable/disable hot reload file watching
    virtual void enableHotReload(bool enable) = 0;

    /// Check if hot reload is enabled
    virtual bool isHotReloadEnabled() const = 0;

    //==========================================================================
    // Change Notifications
    //==========================================================================

    /// Subscribe to config change notifications
    virtual SubscriptionId onConfigChanged(ConfigChangeCallback callback) = 0;

    /// Subscribe to changes for a specific key prefix
    virtual SubscriptionId onKeyChanged(const std::string& keyPrefix,
                                         ConfigChangeCallback callback) = 0;

    /// Unsubscribe from notifications
    virtual void unsubscribe(SubscriptionId id) = 0;
};

}  // namespace jframe
