// bestow-config/src/ConfigSystem.cpp
// Config system implementation

module;

#include <bestow/sol2_compat.hpp>
#include <spdlog/spdlog.h>

module bestow.config.impl;

import std;
import bestow.services;  // Re-exports all contracts

namespace bestow {

//==============================================================================
// Lifecycle
//==============================================================================

bool ConfigSystem::initialize() {
    spdlog::info("[Config] Initializing config system");

    // Initialize Lua with safe libraries only
    lua_.open_libraries(sol::lib::base, sol::lib::math,
                        sol::lib::table, sol::lib::string);

    // Sandbox - remove dangerous functions
    lua_["os"] = sol::lua_nil;
    lua_["io"] = sol::lua_nil;
    lua_["loadfile"] = sol::lua_nil;
    lua_["dofile"] = sol::lua_nil;
    lua_["load"] = sol::lua_nil;
    lua_["loadstring"] = sol::lua_nil;
    lua_["require"] = sol::lua_nil;
    lua_["package"] = sol::lua_nil;
    lua_["debug"] = sol::lua_nil;
    lua_["rawget"] = sol::lua_nil;
    lua_["rawset"] = sol::lua_nil;

    // Add safe 'include' function for loading config files via AssetSystem
    lua_["include"] = [this](const std::string& relativePath) -> sol::object {
        // Only allow loading from data/config/ directory
        if (relativePath.find("..") != std::string::npos) {
            spdlog::error("[Config] include() path cannot contain '..'");
            return sol::lua_nil;
        }

        if (!pIAssetSystem_) {
            spdlog::error("[Config] include() AssetSystem not available");
            return sol::lua_nil;
        }

        try {
            // Register and load the asset
            AssetHandle handle = pIAssetSystem_->registerAsset(AssetType::Data, relativePath);
            pIAssetSystem_->loadAsset(handle);

            // Check if load succeeded
            if (!pIAssetSystem_->isLoaded(handle)) {
                spdlog::error("[Config] include() failed to load: {}", relativePath);
                return sol::lua_nil;
            }

            // Get the loaded data
            const auto* dataAsset = pIAssetSystem_->getAsset<DataAsset>(handle);
            if (!dataAsset) {
                spdlog::error("[Config] include() invalid asset data: {}", relativePath);
                return sol::lua_nil;
            }

            // Execute the Lua content
            sol::protected_function_result result = lua_.safe_script(dataAsset->rawText);
            if (!result.valid()) {
                sol::error err = result;
                spdlog::error("[Config] include() error in {}: {}", relativePath, err.what());
                return sol::lua_nil;
            }
            return result;
        } catch (const std::exception& e) {
            spdlog::error("[Config] include() exception loading {}: {}", relativePath, e.what());
            return sol::lua_nil;
        }
    };

    luaInitialized_ = true;
    spdlog::info("[Config] Config system initialized (sandboxed Lua with include)");
    return true;
}

void ConfigSystem::update(DeltaTime dt) {
    currentTime_ += dt;
    // Hot reload is now event-driven via AssetSystem subscriptions
    // No polling needed here
}

void ConfigSystem::shutdown() {
    spdlog::info("[Config] Shutting down config system");

    // Unsubscribe from all asset change notifications
    if (pIAssetSystem_) {
        for (const auto& [handle, subId] : assetSubscriptions_) {
            pIAssetSystem_->unsubscribe(subId);
        }
    }

    config_.clear();
    loadedFiles_.clear();
    assetSubscriptions_.clear();
    subscriptions_.clear();
    luaInitialized_ = false;
}

//==============================================================================
// Configuration Loading
//==============================================================================

bool ConfigSystem::loadConfig(const std::string& filePath) {
    if (!luaInitialized_) {
        spdlog::error("[Config] Cannot load config - system not initialized");
        return false;
    }

    if (!pIAssetSystem_) {
        spdlog::error("[Config] Cannot load config - AssetSystem not available");
        return false;
    }

    spdlog::info("[Config] Loading config from: {}", filePath);

    try {
        // Register and load the asset
        AssetHandle handle = pIAssetSystem_->registerAsset(AssetType::Data, filePath);
        pIAssetSystem_->loadAsset(handle);

        // Check if load succeeded
        if (!pIAssetSystem_->isLoaded(handle)) {
            spdlog::error("[Config] Failed to load config file: {}", filePath);
            return false;
        }

        // Get the loaded data
        const auto* dataAsset = pIAssetSystem_->getAsset<DataAsset>(handle);
        if (!dataAsset) {
            spdlog::error("[Config] Invalid asset data for config: {}", filePath);
            return false;
        }

        // Execute Lua content
        sol::protected_function_result result = lua_.safe_script(dataAsset->rawText);

        if (!result.valid()) {
            sol::error err = result;
            spdlog::error("[Config] Lua error in {}: {}", filePath, err.what());
            return false;
        }

        // The file should return a table
        sol::object returnValue = result;
        if (returnValue.get_type() != sol::type::table) {
            spdlog::error("[Config] Config file {} must return a table", filePath);
            return false;
        }

        sol::table configTable = returnValue.as<sol::table>();

        // Parse the table recursively
        parseLuaTable(configTable, "", filePath);

        // Track loaded file for hot reload
        LoadedFile loadedFile;
        loadedFile.path = filePath;
        loadedFile.assetHandle = handle;
        loadedFile.loadTime = currentTime_;

        // Subscribe to asset changes for hot reload (if enabled)
        if (hotReloadEnabled_ && assetSubscriptions_.find(handle) == assetSubscriptions_.end()) {
            SubscriptionId subId = pIAssetSystem_->subscribe(handle,
                [this, filePath](AssetHandle h, AssetType type) {
                    spdlog::info("[Config] Hot reload: {} modified, reloading", filePath);
                    reloadConfig(filePath);
                });
            assetSubscriptions_[handle] = subId;
        }

        // Check if already loaded (replace)
        auto it = std::ranges::find_if(loadedFiles_,
            [&](const LoadedFile& f) { return f.path == filePath; });
        if (it != loadedFiles_.end()) {
            *it = loadedFile;
        } else {
            loadedFiles_.push_back(loadedFile);
        }

        spdlog::info("[Config] Loaded {} config values from {}", config_.size(), filePath);
        return true;

    } catch (const std::exception& e) {
        spdlog::error("[Config] Exception loading {}: {}", filePath, e.what());
        return false;
    }
}

bool ConfigSystem::loadConfigAsset(AssetHandle configAsset) {
    if (!luaInitialized_) {
        spdlog::error("[Config] Cannot load config - system not initialized");
        return false;
    }

    if (!pIAssetSystem_) {
        spdlog::error("[Config] Cannot load config - AssetSystem not available");
        return false;
    }

    spdlog::info("[Config] Loading config from asset handle");

    try {
        // Ensure the asset is loaded
        if (!pIAssetSystem_->isLoaded(configAsset)) {
            pIAssetSystem_->loadAsset(configAsset);
        }

        // Check if load succeeded
        if (!pIAssetSystem_->isLoaded(configAsset)) {
            spdlog::error("[Config] Failed to load config asset");
            return false;
        }

        // Get the loaded data
        const auto* dataAsset = pIAssetSystem_->getAsset<DataAsset>(configAsset);
        if (!dataAsset) {
            spdlog::error("[Config] Invalid asset data for config");
            return false;
        }

        // Get the source path for tracking
        auto metadata = pIAssetSystem_->getAssetMetadata(configAsset);
        std::string filePath = metadata.sourcePath.string();

        // Execute Lua content
        sol::protected_function_result result = lua_.safe_script(dataAsset->rawText);

        if (!result.valid()) {
            sol::error err = result;
            spdlog::error("[Config] Lua error in asset: {}", err.what());
            return false;
        }

        // The file should return a table
        sol::object returnValue = result;
        if (returnValue.get_type() != sol::type::table) {
            spdlog::error("[Config] Config asset must return a table");
            return false;
        }

        sol::table configTable = returnValue.as<sol::table>();

        // Parse the table recursively
        parseLuaTable(configTable, "", filePath);

        // Track loaded file for hot reload
        LoadedFile loadedFile;
        loadedFile.path = filePath;
        loadedFile.assetHandle = configAsset;
        loadedFile.loadTime = currentTime_;

        // Subscribe to asset changes for hot reload (if enabled)
        if (hotReloadEnabled_ && assetSubscriptions_.find(configAsset) == assetSubscriptions_.end()) {
            SubscriptionId subId = pIAssetSystem_->subscribe(configAsset,
                [this, filePath](AssetHandle h, AssetType type) {
                    spdlog::info("[Config] Hot reload: {} modified, reloading", filePath);
                    loadConfigAsset(h);
                });
            assetSubscriptions_[configAsset] = subId;
        }

        // Check if already loaded (replace)
        auto it = std::ranges::find_if(loadedFiles_,
            [&](const LoadedFile& f) { return f.path == filePath; });
        if (it != loadedFiles_.end()) {
            *it = loadedFile;
        } else {
            loadedFiles_.push_back(loadedFile);
        }

        spdlog::info("[Config] Loaded {} config values from asset", config_.size());
        return true;

    } catch (const std::exception& e) {
        spdlog::error("[Config] Exception loading config asset: {}", e.what());
        return false;
    }
}

bool ConfigSystem::reloadAll() {
    spdlog::info("[Config] Reloading all {} config files", loadedFiles_.size());

    bool allSuccess = true;
    for (const auto& file : loadedFiles_) {
        if (!reloadConfig(file.path)) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

bool ConfigSystem::reloadConfig(const std::string& filePath) {
    // Store old keys to detect removed values
    std::vector<ConfigKey> oldKeys;
    for (const auto& [key, entry] : config_) {
        if (entry.sourcePath == filePath) {
            oldKeys.push_back(key);
        }
    }

    // Reload the file
    if (!loadConfig(filePath)) {
        return false;
    }

    // Notify all changed keys
    for (const auto& key : oldKeys) {
        notifyChange(key);
    }

    return true;
}

//==============================================================================
// Lua Parsing Helpers
//==============================================================================

void ConfigSystem::parseLuaTable(sol::table& table, const std::string& prefix,
                                  const std::string& sourcePath) {
    for (auto& [key, value] : table) {
        std::string keyStr;

        // Convert key to string
        if (key.get_type() == sol::type::string) {
            keyStr = key.as<std::string>();
        } else if (key.get_type() == sol::type::number) {
            keyStr = std::to_string(key.as<int>());
        } else {
            continue;  // Skip non-string/number keys
        }

        // Build full key path
        std::string fullKey = prefix.empty() ? keyStr : prefix + "." + keyStr;

        // Parse the value
        parseLuaValue(fullKey, value, sourcePath);
    }
}

void ConfigSystem::parseLuaValue(const std::string& key, sol::object& value,
                                  const std::string& sourcePath) {
    ConfigEntry entry;
    entry.sourcePath = sourcePath;
    entry.loadTime = currentTime_;

    switch (value.get_type()) {
        case sol::type::number: {
            // Check if it's an integer or float
            double num = value.as<double>();
            if (num == static_cast<double>(static_cast<int>(num))) {
                entry.value = static_cast<int>(num);
                entry.type = "int";
            } else {
                entry.value = static_cast<float>(num);
                entry.type = "float";
            }
            config_[key] = entry;
            break;
        }

        case sol::type::boolean: {
            entry.value = value.as<bool>();
            entry.type = "bool";
            config_[key] = entry;
            break;
        }

        case sol::type::string: {
            entry.value = value.as<std::string>();
            entry.type = "string";
            config_[key] = entry;
            break;
        }

        case sol::type::table: {
            sol::table subTable = value.as<sol::table>();

            // Check if it's an array (sequential integer keys starting at 1)
            bool isArray = true;
            size_t expectedIndex = 1;
            for (auto& [k, v] : subTable) {
                if (k.get_type() != sol::type::number ||
                    k.as<size_t>() != expectedIndex) {
                    isArray = false;
                    break;
                }
                expectedIndex++;
            }

            if (isArray && subTable.size() > 0) {
                // Parse as array
                sol::object firstElement = subTable[1];
                if (firstElement.get_type() == sol::type::number) {
                    // Check if int or float array
                    std::vector<int> intArray;
                    std::vector<float> floatArray;
                    bool hasFloats = false;

                    for (size_t i = 1; i <= subTable.size(); i++) {
                        double num = subTable[i].get<double>();
                        if (num != static_cast<double>(static_cast<int>(num))) {
                            hasFloats = true;
                        }
                        intArray.push_back(static_cast<int>(num));
                        floatArray.push_back(static_cast<float>(num));
                    }

                    if (hasFloats) {
                        entry.value = floatArray;
                        entry.type = "float_array";
                    } else {
                        entry.value = intArray;
                        entry.type = "int_array";
                    }
                    config_[key] = entry;

                } else if (firstElement.get_type() == sol::type::string) {
                    std::vector<std::string> stringArray;
                    for (size_t i = 1; i <= subTable.size(); i++) {
                        stringArray.push_back(subTable[i].get<std::string>());
                    }
                    entry.value = stringArray;
                    entry.type = "string_array";
                    config_[key] = entry;
                }
            } else {
                // Parse as nested table (recurse)
                parseLuaTable(subTable, key, sourcePath);
            }
            break;
        }

        default:
            // Skip nil, function, userdata, etc.
            break;
    }
}

//==============================================================================
// Type-Safe Value Access
//==============================================================================

std::optional<float> ConfigSystem::getFloat(const ConfigKey& key) const {
    auto it = config_.find(key);
    if (it == config_.end()) {
        return std::nullopt;
    }

    const auto& entry = it->second;
    if (entry.type == "float") {
        return std::any_cast<float>(entry.value);
    } else if (entry.type == "int") {
        return static_cast<float>(std::any_cast<int>(entry.value));
    }
    return std::nullopt;
}

std::optional<int> ConfigSystem::getInt(const ConfigKey& key) const {
    auto it = config_.find(key);
    if (it == config_.end()) {
        return std::nullopt;
    }

    const auto& entry = it->second;
    if (entry.type == "int") {
        return std::any_cast<int>(entry.value);
    } else if (entry.type == "float") {
        return static_cast<int>(std::any_cast<float>(entry.value));
    }
    return std::nullopt;
}

std::optional<bool> ConfigSystem::getBool(const ConfigKey& key) const {
    auto it = config_.find(key);
    if (it == config_.end()) {
        return std::nullopt;
    }

    const auto& entry = it->second;
    if (entry.type == "bool") {
        return std::any_cast<bool>(entry.value);
    }
    return std::nullopt;
}

std::optional<std::string> ConfigSystem::getString(const ConfigKey& key) const {
    auto it = config_.find(key);
    if (it == config_.end()) {
        return std::nullopt;
    }

    const auto& entry = it->second;
    if (entry.type == "string") {
        return std::any_cast<std::string>(entry.value);
    }
    return std::nullopt;
}

float ConfigSystem::getFloatOr(const ConfigKey& key, float defaultValue) const {
    return getFloat(key).value_or(defaultValue);
}

int ConfigSystem::getIntOr(const ConfigKey& key, int defaultValue) const {
    return getInt(key).value_or(defaultValue);
}

bool ConfigSystem::getBoolOr(const ConfigKey& key, bool defaultValue) const {
    return getBool(key).value_or(defaultValue);
}

std::string ConfigSystem::getStringOr(const ConfigKey& key,
                                       const std::string& defaultValue) const {
    return getString(key).value_or(defaultValue);
}

//==============================================================================
// Array/List Access
//==============================================================================

std::vector<int> ConfigSystem::getIntArray(const ConfigKey& key) const {
    auto it = config_.find(key);
    if (it == config_.end()) {
        return {};
    }

    const auto& entry = it->second;
    if (entry.type == "int_array") {
        return std::any_cast<std::vector<int>>(entry.value);
    }
    return {};
}

std::vector<float> ConfigSystem::getFloatArray(const ConfigKey& key) const {
    auto it = config_.find(key);
    if (it == config_.end()) {
        return {};
    }

    const auto& entry = it->second;
    if (entry.type == "float_array") {
        return std::any_cast<std::vector<float>>(entry.value);
    } else if (entry.type == "int_array") {
        // Convert int array to float array
        auto intArray = std::any_cast<std::vector<int>>(entry.value);
        std::vector<float> floatArray;
        floatArray.reserve(intArray.size());
        for (int i : intArray) {
            floatArray.push_back(static_cast<float>(i));
        }
        return floatArray;
    }
    return {};
}

std::vector<std::string> ConfigSystem::getStringArray(const ConfigKey& key) const {
    auto it = config_.find(key);
    if (it == config_.end()) {
        return {};
    }

    const auto& entry = it->second;
    if (entry.type == "string_array") {
        return std::any_cast<std::vector<std::string>>(entry.value);
    }
    return {};
}

//==============================================================================
// Runtime Value Modification
//==============================================================================

void ConfigSystem::setFloat(const ConfigKey& key, float value) {
    ConfigEntry entry;
    entry.value = value;
    entry.type = "float";
    entry.sourcePath = "runtime";
    entry.loadTime = currentTime_;
    config_[key] = entry;
    notifyChange(key);
}

void ConfigSystem::setInt(const ConfigKey& key, int value) {
    ConfigEntry entry;
    entry.value = value;
    entry.type = "int";
    entry.sourcePath = "runtime";
    entry.loadTime = currentTime_;
    config_[key] = entry;
    notifyChange(key);
}

void ConfigSystem::setBool(const ConfigKey& key, bool value) {
    ConfigEntry entry;
    entry.value = value;
    entry.type = "bool";
    entry.sourcePath = "runtime";
    entry.loadTime = currentTime_;
    config_[key] = entry;
    notifyChange(key);
}

void ConfigSystem::setString(const ConfigKey& key, const std::string& value) {
    ConfigEntry entry;
    entry.value = value;
    entry.type = "string";
    entry.sourcePath = "runtime";
    entry.loadTime = currentTime_;
    config_[key] = entry;
    notifyChange(key);
}

//==============================================================================
// State Queries
//==============================================================================

bool ConfigSystem::hasKey(const ConfigKey& key) const {
    return config_.contains(key);
}

std::vector<ConfigKey> ConfigSystem::getKeysWithPrefix(
    const std::string& prefix) const {
    std::vector<ConfigKey> keys;
    for (const auto& [key, entry] : config_) {
        if (key.starts_with(prefix)) {
            keys.push_back(key);
        }
    }
    return keys;
}

std::vector<std::string> ConfigSystem::getLoadedConfigs() const {
    std::vector<std::string> paths;
    paths.reserve(loadedFiles_.size());
    for (const auto& file : loadedFiles_) {
        paths.push_back(file.path);
    }
    return paths;
}

ConfigMetadata ConfigSystem::getMetadata(const std::string& filePath) const {
    ConfigMetadata metadata;
    metadata.sourcePath = filePath;

    auto it = std::ranges::find_if(loadedFiles_,
        [&](const LoadedFile& f) { return f.path == filePath; });
    if (it != loadedFiles_.end()) {
        metadata.loadTime = it->loadTime;
    }

    return metadata;
}

//==============================================================================
// Hot Reload
//==============================================================================

void ConfigSystem::enableHotReload(bool enable) {
    if (hotReloadEnabled_ == enable) {
        return;  // No change
    }

    hotReloadEnabled_ = enable;

    if (!pIAssetSystem_) {
        spdlog::warn("[Config] Cannot enable hot reload - AssetSystem not available");
        return;
    }

    if (enable) {
        // Subscribe to all loaded config files
        for (const auto& file : loadedFiles_) {
            if (assetSubscriptions_.find(file.assetHandle) == assetSubscriptions_.end()) {
                std::string filePath = file.path;  // Capture by value
                SubscriptionId subId = pIAssetSystem_->subscribe(file.assetHandle,
                    [this, filePath](AssetHandle h, AssetType type) {
                        spdlog::info("[Config] Hot reload: {} modified, reloading", filePath);
                        reloadConfig(filePath);
                    });
                assetSubscriptions_[file.assetHandle] = subId;
            }
        }
        spdlog::info("[Config] Hot reload enabled");
    } else {
        // Unsubscribe from all asset change notifications
        for (const auto& [handle, subId] : assetSubscriptions_) {
            pIAssetSystem_->unsubscribe(subId);
        }
        assetSubscriptions_.clear();
        spdlog::info("[Config] Hot reload disabled");
    }
}

bool ConfigSystem::isHotReloadEnabled() const {
    return hotReloadEnabled_;
}

//==============================================================================
// Change Notifications
//==============================================================================

SubscriptionId ConfigSystem::onConfigChanged(ConfigChangeCallback callback) {
    ConfigSubscription sub;
    sub.id = nextSubscriptionId_++;
    sub.keyPrefix = "";  // All changes
    sub.callback = std::move(callback);
    subscriptions_.push_back(std::move(sub));
    return sub.id;
}

SubscriptionId ConfigSystem::onKeyChanged(const std::string& keyPrefix,
                                           ConfigChangeCallback callback) {
    ConfigSubscription sub;
    sub.id = nextSubscriptionId_++;
    sub.keyPrefix = keyPrefix;
    sub.callback = std::move(callback);
    subscriptions_.push_back(std::move(sub));
    return sub.id;
}

void ConfigSystem::unsubscribe(SubscriptionId id) {
    std::erase_if(subscriptions_,
        [id](const ConfigSubscription& sub) { return sub.id == id; });
}

void ConfigSystem::notifyChange(const ConfigKey& key) {
    // Notify local subscribers (existing callback mechanism)
    for (const auto& sub : subscriptions_) {
        if (sub.keyPrefix.empty() || key.starts_with(sub.keyPrefix)) {
            sub.callback(key);
        }
    }

    // Publish to EventSystem if available
    if (pIEventSystem_) {
        // Extract section from key (everything before the first '.')
        std::string section;
        auto dotPos = key.find('.');
        if (dotPos != std::string::npos) {
            section = key.substr(0, dotPos);
        }

        pIEventSystem_->publish(Events::ConfigChanged, ConfigEventData{
            .key = key,
            .section = section
        });
    }
}

//==============================================================================
// Unified Lua Parsing (For other systems to use)
//==============================================================================

std::optional<sol::object> ConfigSystem::parseLuaString(
    const std::string& luaCode,
    const std::string& description) {
    if (!luaInitialized_) {
        spdlog::error("[Config] parseLuaString: Lua not initialized");
        return std::nullopt;
    }

    try {
        sol::protected_function_result result = lua_.safe_script(
            luaCode,
            sol::script_pass_on_error
        );
        if (!result.valid()) {
            sol::error err = result;
            spdlog::error("[Config] Lua parse error in {}: {}", description, err.what());
            return std::nullopt;
        }
        return result.get<sol::object>();
    } catch (const std::exception& e) {
        spdlog::error("[Config] Exception parsing Lua in {}: {}", description, e.what());
        return std::nullopt;
    }
}

std::optional<sol::object> ConfigSystem::parseLuaAsset(
    AssetHandle luaAsset,
    const std::string& description) {
    if (!luaInitialized_) {
        spdlog::error("[Config] parseLuaAsset: Lua not initialized");
        return std::nullopt;
    }

    if (!pIAssetSystem_) {
        spdlog::error("[Config] parseLuaAsset: AssetSystem not available");
        return std::nullopt;
    }

    if (!pIAssetSystem_->isLoaded(luaAsset)) {
        pIAssetSystem_->loadAsset(luaAsset);
    }

    if (!pIAssetSystem_->isLoaded(luaAsset)) {
        spdlog::error("[Config] parseLuaAsset: Failed to load asset for {}", description);
        return std::nullopt;
    }

    // Get the loaded data
    const auto* dataAsset = pIAssetSystem_->getAsset<DataAsset>(luaAsset);
    if (!dataAsset) {
        spdlog::error("[Config] parseLuaAsset: Invalid asset data for {}", description);
        return std::nullopt;
    }

    // Parse the Lua text
    return parseLuaString(dataAsset->rawText, description);
}

bool ConfigSystem::executeLuaString(
    const std::string& luaCode,
    const std::string& description) {
    if (!luaInitialized_) {
        spdlog::error("[Config] executeLuaString: Lua not initialized");
        return false;
    }

    try {
        sol::protected_function_result result = lua_.safe_script(
            luaCode,
            sol::script_pass_on_error
        );
        if (!result.valid()) {
            sol::error err = result;
            spdlog::error("[Config] Lua execution error in {}: {}", description, err.what());
            return false;
        }
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[Config] Exception executing Lua in {}: {}", description, e.what());
        return false;
    }
}

sol::state* ConfigSystem::getLuaState() {
    return &lua_;
}

}  // namespace bestow
