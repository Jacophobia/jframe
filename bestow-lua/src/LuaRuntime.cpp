// bestow-lua/src/LuaRuntime.cpp
// Core LuaRuntime implementation - lifecycle, sandboxing, file loading

module;

#include <sol/sol.hpp>
#include <spdlog/spdlog.h>

module bestow.lua.impl;

import std;
import bestow.services;

namespace bestow {

//==============================================================================
// Destructor
//==============================================================================

LuaRuntime::~LuaRuntime() {
    if (initialized_) {
        shutdown();
    }
}

//==============================================================================
// Lifecycle
//==============================================================================

bool LuaRuntime::initialize() {
    if (initialized_) {
        spdlog::warn("[LuaRuntime] Already initialized");
        return true;
    }

    spdlog::info("[LuaRuntime] Initializing...");

    // Open standard libraries (safe subset)
    lua_.open_libraries(
        sol::lib::base,
        sol::lib::math,
        sol::lib::string,
        sol::lib::table,
        sol::lib::utf8,
        sol::lib::coroutine
    );

    // Setup sandbox (remove dangerous functions)
    setupSandbox();

    // Setup engine bindings
    setupBindings();

    // Register built-in component creators
    registerBuiltinComponents();

    // Determine which engine systems are available
    if (pIAssetSystem_) availableSystems_.insert("assets");
    if (pIEventSystem_) availableSystems_.insert("events");
    if (pIEntitySystem_) availableSystems_.insert("entities");

    initialized_ = true;
    spdlog::info("[LuaRuntime] Initialized successfully");
    return true;
}

void LuaRuntime::update(float dt) {
    if (!initialized_) return;

    // Update Lua systems (sorted by priority)
    for (const auto& handle : systemUpdateOrder_) {
        auto it = loadedSystems_.find(handle.id);
        if (it == loadedSystems_.end() || !it->second.enabled) continue;

        auto& sys = it->second;
        if (sys.definition.valid()) {
            sol::optional<sol::function> updateFn = sys.definition["update"];
            if (updateFn) {
                auto result = (*updateFn)(dt);
                if (!result.valid()) {
                    sol::error err = result;
                    spdlog::error("[LuaRuntime] System '{}' update error: {}",
                                  sys.name, err.what());
                }
            }
        }
    }

    // Update behavior instances
    for (auto& instance : behaviorInstances_) {
        if (!instance.enabled) continue;

        // Check if entity still exists
        if (!pIEntitySystem_->isValid(instance.entity)) {
            continue;  // Will be cleaned up later
        }

        callBehaviorCallback(instance, "update", dt);
    }

    // Clean up instances for destroyed entities
    std::erase_if(behaviorInstances_, [this](const BehaviorInstance& inst) {
        return !pIEntitySystem_->isValid(inst.entity);
    });
}

void LuaRuntime::fixedUpdate(float fixedDt) {
    if (!initialized_) return;

    // Fixed update Lua systems
    for (const auto& handle : systemUpdateOrder_) {
        auto it = loadedSystems_.find(handle.id);
        if (it == loadedSystems_.end() || !it->second.enabled) continue;

        auto& sys = it->second;
        if (sys.definition.valid()) {
            sol::optional<sol::function> fixedUpdateFn = sys.definition["fixedUpdate"];
            if (fixedUpdateFn) {
                auto result = (*fixedUpdateFn)(fixedDt);
                if (!result.valid()) {
                    sol::error err = result;
                    spdlog::error("[LuaRuntime] System '{}' fixedUpdate error: {}",
                                  sys.name, err.what());
                }
            }
        }
    }

    // Fixed update behavior instances
    for (auto& instance : behaviorInstances_) {
        if (!instance.enabled) continue;
        if (!pIEntitySystem_->isValid(instance.entity)) continue;

        callBehaviorCallback(instance, "fixedUpdate", fixedDt);
    }
}

void LuaRuntime::shutdown() {
    if (!initialized_) return;

    spdlog::info("[LuaRuntime] Shutting down...");

    // Call shutdown on all Lua systems
    for (const auto& handle : systemUpdateOrder_) {
        auto it = loadedSystems_.find(handle.id);
        if (it == loadedSystems_.end()) continue;

        auto& sys = it->second;
        if (sys.definition.valid()) {
            sol::optional<sol::function> shutdownFn = sys.definition["shutdown"];
            if (shutdownFn) {
                auto result = (*shutdownFn)();
                if (!result.valid()) {
                    sol::error err = result;
                    spdlog::warn("[LuaRuntime] System '{}' shutdown error: {}",
                                 sys.name, err.what());
                }
            }
        }
    }

    // Call destroy on all behavior instances
    for (auto& instance : behaviorInstances_) {
        callBehaviorCallback(instance, "destroy", 0.0f);
    }

    // Unsubscribe from asset changes
    if (pIAssetSystem_) {
        for (auto subId : assetSubscriptions_) {
            pIAssetSystem_->unsubscribe(subId);
        }
    }
    assetSubscriptions_.clear();

    // Clear all state
    behaviorInstances_.clear();
    loadedBehaviors_.clear();
    behaviorsByName_.clear();
    loadedSystems_.clear();
    systemUpdateOrder_.clear();
    blueprints_.clear();
    blueprintAssets_.clear();
    configValues_.clear();
    configAssets_.clear();
    componentCreators_.clear();
    appConfig_ = sol::table();

    initialized_ = false;
    spdlog::info("[LuaRuntime] Shutdown complete");
}

//==============================================================================
// Sandbox Setup
//==============================================================================

void LuaRuntime::setupSandbox() {
    // Remove dangerous functions that could access the file system or execute code
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
    lua_["rawequal"] = sol::lua_nil;
    lua_["rawlen"] = sol::lua_nil;
    lua_["collectgarbage"] = sol::lua_nil;

    // Provide safe print function
    lua_["print"] = [](sol::variadic_args va) {
        std::string output;
        for (auto arg : va) {
            if (!output.empty()) output += "\t";
            if (arg.is<std::string>()) {
                output += arg.as<std::string>();
            } else if (arg.is<double>()) {
                output += std::format("{}", arg.as<double>());
            } else if (arg.is<bool>()) {
                output += arg.as<bool>() ? "true" : "false";
            } else if (arg.is<sol::nil_t>()) {
                output += "nil";
            } else {
                output += "[object]";
            }
        }
        spdlog::info("[Lua] {}", output);
    };

    // Add safe assert function
    lua_["assert"] = [](bool condition, sol::optional<std::string> message) {
        if (!condition) {
            std::string msg = message.value_or("assertion failed!");
            spdlog::error("[Lua] Assert failed: {}", msg);
            throw std::runtime_error(msg);
        }
    };

    // Add error function
    lua_["error"] = [](std::string message) {
        spdlog::error("[Lua] Error: {}", message);
        throw std::runtime_error(message);
    };

    spdlog::debug("[LuaRuntime] Sandbox configured");
}

//==============================================================================
// App Entry Point
//==============================================================================

LuaResult<void> LuaRuntime::loadApp() {
    // Search standard locations
    for (const auto& searchPath : LuaPath::AppSearchPaths) {
        std::filesystem::path path = PathResolver::resolve(std::string(searchPath));
        if (std::filesystem::exists(path)) {
            return loadApp(searchPath);
        }
    }

    return std::unexpected(LuaError{
        .code = LuaErrorCode::FileNotFound,
        .message = "No app entry point found. Searched: app.lua, main.lua, data/app.lua, game/app.lua",
        .source = "app"
    });
}

LuaResult<void> LuaRuntime::loadApp(std::string_view path) {
    spdlog::info("[LuaRuntime] Loading app from: {}", path);

    std::filesystem::path resolvedPath = PathResolver::resolve(std::string(path));

    // Read the file
    std::ifstream file(resolvedPath);
    if (!file.is_open()) {
        return std::unexpected(LuaError{
            .code = LuaErrorCode::FileNotFound,
            .message = std::format("Could not open app file: {}", resolvedPath.string()),
            .source = std::string(path)
        });
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string luaCode = buffer.str();

    // Execute the app file
    auto result = lua_.safe_script(luaCode, sol::script_pass_on_error);
    if (!result.valid()) {
        sol::error err = result;
        return std::unexpected(LuaError{
            .code = LuaErrorCode::ParseError,
            .message = err.what(),
            .source = std::string(path)
        });
    }

    // The app file should return a table with configuration
    if (result.get_type() == sol::type::table) {
        appConfig_ = result.get<sol::table>();
        appPath_ = std::string(path);

        // Process app config
        sol::optional<std::string> name = appConfig_["name"];
        if (name) {
            spdlog::info("[LuaRuntime] App loaded: {}", *name);
        }

        // Auto-load blueprints directory if specified
        sol::optional<std::string> blueprintsDir = appConfig_["blueprints"];
        if (blueprintsDir) {
            auto loadResult = loadBlueprints(*blueprintsDir);
            if (!loadResult) {
                spdlog::warn("[LuaRuntime] Failed to load blueprints: {}",
                             loadResult.error().message);
            }
        }

        // Auto-load behaviors directory if specified
        sol::optional<std::string> behaviorsDir = appConfig_["behaviors"];
        if (behaviorsDir) {
            // Will be loaded from the directory
        }

        // Auto-load systems directory if specified
        sol::optional<std::string> systemsDir = appConfig_["systems"];
        if (systemsDir) {
            // Will be loaded from the directory
        }

        // Call init function if provided
        sol::optional<sol::function> initFn = appConfig_["init"];
        if (initFn) {
            auto initResult = (*initFn)();
            if (!initResult.valid()) {
                sol::error err = initResult;
                spdlog::error("[LuaRuntime] App init error: {}", err.what());
            }
        }
    } else {
        spdlog::warn("[LuaRuntime] App file did not return a table");
    }

    return {};
}

sol::table LuaRuntime::getAppConfig() {
    return appConfig_;
}

//==============================================================================
// Direct Lua Access
//==============================================================================

LuaResult<sol::object> LuaRuntime::execute(std::string_view code, std::string_view description) {
    auto result = lua_.safe_script(std::string(code), sol::script_pass_on_error);
    if (!result.valid()) {
        sol::error err = result;
        return std::unexpected(LuaError{
            .code = LuaErrorCode::ParseError,
            .message = err.what(),
            .source = std::string(description)
        });
    }
    return result.get<sol::object>();
}

LuaResult<void> LuaRuntime::run(std::string_view code, std::string_view description) {
    auto result = execute(code, description);
    if (!result) {
        return std::unexpected(result.error());
    }
    return {};
}

LuaResult<sol::object> LuaRuntime::callImpl(std::string_view funcName, sol::variadic_args args) {
    sol::object func = lua_[std::string(funcName)];
    if (!func.valid() || func.get_type() != sol::type::function) {
        return std::unexpected(LuaError{
            .code = LuaErrorCode::RuntimeError,
            .message = std::format("Function '{}' not found or not a function", funcName),
            .source = "global"
        });
    }

    sol::function fn = func.as<sol::function>();
    sol::protected_function_result result = fn(args);

    if (!result.valid()) {
        sol::error err = result;
        return std::unexpected(LuaError{
            .code = LuaErrorCode::RuntimeError,
            .message = err.what(),
            .source = std::string(funcName)
        });
    }

    return result.get<sol::object>();
}

void LuaRuntime::setGlobal(std::string_view name, sol::object value) {
    lua_[std::string(name)] = value;
}

sol::object LuaRuntime::getGlobal(std::string_view name) {
    return lua_[std::string(name)];
}

sol::state& LuaRuntime::getLuaState() {
    return lua_;
}

//==============================================================================
// Configuration Values
//==============================================================================

std::optional<float> LuaRuntime::getFloat(std::string_view key) const {
    auto it = configValues_.find(std::string(key));
    if (it == configValues_.end()) return std::nullopt;
    if (auto* val = std::get_if<float>(&it->second.value)) {
        return *val;
    }
    if (auto* val = std::get_if<int>(&it->second.value)) {
        return static_cast<float>(*val);
    }
    return std::nullopt;
}

std::optional<int> LuaRuntime::getInt(std::string_view key) const {
    auto it = configValues_.find(std::string(key));
    if (it == configValues_.end()) return std::nullopt;
    if (auto* val = std::get_if<int>(&it->second.value)) {
        return *val;
    }
    if (auto* val = std::get_if<float>(&it->second.value)) {
        return static_cast<int>(*val);
    }
    return std::nullopt;
}

std::optional<bool> LuaRuntime::getBool(std::string_view key) const {
    auto it = configValues_.find(std::string(key));
    if (it == configValues_.end()) return std::nullopt;
    if (auto* val = std::get_if<bool>(&it->second.value)) {
        return *val;
    }
    return std::nullopt;
}

std::optional<std::string> LuaRuntime::getString(std::string_view key) const {
    auto it = configValues_.find(std::string(key));
    if (it == configValues_.end()) return std::nullopt;
    if (auto* val = std::get_if<std::string>(&it->second.value)) {
        return *val;
    }
    return std::nullopt;
}

float LuaRuntime::getFloatOr(std::string_view key, float defaultVal) const {
    return getFloat(key).value_or(defaultVal);
}

int LuaRuntime::getIntOr(std::string_view key, int defaultVal) const {
    return getInt(key).value_or(defaultVal);
}

bool LuaRuntime::getBoolOr(std::string_view key, bool defaultVal) const {
    return getBool(key).value_or(defaultVal);
}

std::string LuaRuntime::getStringOr(std::string_view key, std::string_view defaultVal) const {
    return getString(key).value_or(std::string(defaultVal));
}

std::vector<PropertyValue> LuaRuntime::getTable(std::string_view key) const {
    std::vector<PropertyValue> result;

    // Navigate the app config table using dot-separated key path
    if (!appConfig_.valid()) {
        return result;
    }

    // Parse key path (e.g., "graphics.clearColor")
    std::vector<std::string> parts;
    std::string keyStr(key);
    std::size_t pos = 0;
    while ((pos = keyStr.find('.')) != std::string::npos) {
        parts.push_back(keyStr.substr(0, pos));
        keyStr.erase(0, pos + 1);
    }
    parts.push_back(keyStr);

    // Navigate to the target table
    sol::table current = appConfig_;
    for (std::size_t i = 0; i < parts.size(); ++i) {
        sol::optional<sol::table> next = current[parts[i]];
        if (!next) {
            return result;  // Path not found
        }
        current = *next;
    }

    // Extract values from the array-like table
    for (auto& [k, v] : current) {
        if (k.get_type() == sol::type::number) {
            // Array element
            if (v.get_type() == sol::type::number) {
                // Check if it's an integer or float
                double d = v.as<double>();
                if (d == std::floor(d) && d >= std::numeric_limits<std::int64_t>::min() &&
                    d <= std::numeric_limits<std::int64_t>::max()) {
                    result.push_back(static_cast<std::int64_t>(d));
                } else {
                    result.push_back(d);
                }
            } else if (v.get_type() == sol::type::string) {
                result.push_back(v.as<std::string>());
            } else if (v.get_type() == sol::type::boolean) {
                result.push_back(v.as<bool>());
            }
        }
    }

    return result;
}

void LuaRuntime::setFloat(std::string_view key, float value) {
    configValues_[std::string(key)] = ConfigValue{value};
}

void LuaRuntime::setInt(std::string_view key, int value) {
    configValues_[std::string(key)] = ConfigValue{value};
}

void LuaRuntime::setBool(std::string_view key, bool value) {
    configValues_[std::string(key)] = ConfigValue{value};
}

void LuaRuntime::setString(std::string_view key, std::string_view value) {
    configValues_[std::string(key)] = ConfigValue{std::string(value)};
}

bool LuaRuntime::hasKey(std::string_view key) const {
    return configValues_.contains(std::string(key));
}

LuaResult<void> LuaRuntime::loadConfig(std::string_view path) {
    std::filesystem::path resolvedPath = PathResolver::resolve(std::string(path));

    std::ifstream file(resolvedPath);
    if (!file.is_open()) {
        return std::unexpected(LuaError{
            .code = LuaErrorCode::FileNotFound,
            .message = std::format("Could not open config file: {}", resolvedPath.string()),
            .source = std::string(path)
        });
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string luaCode = buffer.str();

    auto result = lua_.safe_script(luaCode, sol::script_pass_on_error);
    if (!result.valid()) {
        sol::error err = result;
        return std::unexpected(LuaError{
            .code = LuaErrorCode::ParseError,
            .message = err.what(),
            .source = std::string(path)
        });
    }

    // Config file should return a table
    if (result.get_type() == sol::type::table) {
        sol::table config = result.get<sol::table>();

        // Extract all key-value pairs recursively
        std::function<void(const sol::table&, const std::string&)> extractValues;
        extractValues = [this, &extractValues](const sol::table& table, const std::string& prefix) {
            for (const auto& [key, value] : table) {
                std::string keyStr;
                if (key.is<std::string>()) {
                    keyStr = prefix.empty() ? key.as<std::string>()
                                            : prefix + "." + key.as<std::string>();
                } else if (key.is<int>()) {
                    keyStr = prefix + "[" + std::to_string(key.as<int>()) + "]";
                } else {
                    continue;
                }

                if (value.is<double>()) {
                    double d = value.as<double>();
                    if (d == static_cast<int>(d)) {
                        configValues_[keyStr] = ConfigValue{static_cast<int>(d)};
                    } else {
                        configValues_[keyStr] = ConfigValue{static_cast<float>(d)};
                    }
                } else if (value.is<bool>()) {
                    configValues_[keyStr] = ConfigValue{value.as<bool>()};
                } else if (value.is<std::string>()) {
                    configValues_[keyStr] = ConfigValue{value.as<std::string>()};
                } else if (value.is<sol::table>()) {
                    extractValues(value.as<sol::table>(), keyStr);
                }
            }
        };

        extractValues(config, "");
        spdlog::info("[LuaRuntime] Loaded config from: {}", path);
    }

    return {};
}

//==============================================================================
// Hot Reload
//==============================================================================

void LuaRuntime::enableHotReload(bool enable) {
    if (hotReloadEnabled_ == enable) return;

    hotReloadEnabled_ = enable;

    if (enable && pIAssetSystem_) {
        // Subscribe to all Lua/Data asset changes
        auto subId = pIAssetSystem_->subscribeToType(AssetType::Data,
            [this](AssetHandle handle, AssetType type) {
                onAssetReloaded(handle, type);
            });
        assetSubscriptions_.push_back(subId);

        spdlog::info("[LuaRuntime] Hot reload enabled");
    } else if (!enable && pIAssetSystem_) {
        for (auto subId : assetSubscriptions_) {
            pIAssetSystem_->unsubscribe(subId);
        }
        assetSubscriptions_.clear();

        spdlog::info("[LuaRuntime] Hot reload disabled");
    }
}

bool LuaRuntime::isHotReloadEnabled() const {
    return hotReloadEnabled_;
}

LuaResult<void> LuaRuntime::reloadFile(std::string_view path) {
    std::string pathStr(path);

    // Check if it's a behavior
    for (auto& [id, behavior] : loadedBehaviors_) {
        if (behavior.path == pathStr) {
            reloadBehavior(behavior);
            return {};
        }
    }

    // Check if it's a system
    for (auto& [id, system] : loadedSystems_) {
        if (system.path == pathStr) {
            // Reload the system file
            std::filesystem::path resolvedPath = PathResolver::resolve(pathStr);
            std::ifstream file(resolvedPath);
            if (!file.is_open()) {
                return std::unexpected(LuaError{
                    .code = LuaErrorCode::FileNotFound,
                    .message = std::format("Could not open system file: {}", pathStr),
                    .source = pathStr
                });
            }

            std::stringstream buffer;
            buffer << file.rdbuf();

            auto result = lua_.safe_script(buffer.str(), sol::script_pass_on_error);
            if (!result.valid()) {
                sol::error err = result;
                return std::unexpected(LuaError{
                    .code = LuaErrorCode::ParseError,
                    .message = err.what(),
                    .source = pathStr
                });
            }

            if (result.get_type() == sol::type::table) {
                system.definition = result.get<sol::table>();

                // Call onReload if defined
                sol::optional<sol::function> reloadFn = system.definition["onReload"];
                if (reloadFn) {
                    (*reloadFn)();
                }
            }
            return {};
        }
    }

    // Not a tracked file, try to reload as generic Lua
    return loadLuaFile(path, path);
}

LuaResult<void> LuaRuntime::reloadAll() {
    spdlog::info("[LuaRuntime] Reloading all Lua files...");

    // Reload all behaviors (preserving state)
    for (auto& [id, behavior] : loadedBehaviors_) {
        reloadBehavior(behavior);
    }

    // Reload all systems
    for (auto& [id, system] : loadedSystems_) {
        auto result = reloadFile(system.path);
        if (!result) {
            spdlog::error("[LuaRuntime] Failed to reload system '{}': {}",
                          system.name, result.error().message);
        }
    }

    spdlog::info("[LuaRuntime] Reload complete");
    return {};
}

void LuaRuntime::onAssetReloaded(AssetHandle handle, AssetType type) {
    if (type != AssetType::Data) return;

    // Find which file this asset corresponds to
    // This is called when any Data asset is reloaded

    // Check behaviors
    for (auto& [id, behavior] : loadedBehaviors_) {
        if (behavior.asset.uuid == handle.uuid) {
            spdlog::info("[LuaRuntime] Hot reloading behavior: {}", behavior.name);
            reloadBehavior(behavior);
            return;
        }
    }

    // Check systems
    for (auto& [id, system] : loadedSystems_) {
        if (system.asset.uuid == handle.uuid) {
            spdlog::info("[LuaRuntime] Hot reloading system: {}", system.name);
            reloadFile(system.path);
            return;
        }
    }

    // Check blueprints
    for (const auto& bpAsset : blueprintAssets_) {
        if (bpAsset.uuid == handle.uuid) {
            spdlog::info("[LuaRuntime] Hot reloading blueprints");
            // Re-parse the blueprints file
            // The asset system will have already reloaded the file data
            break;
        }
    }

    // Check watched configs
    for (const auto& [path, cfgHandle] : watchedConfigs_) {
        if (cfgHandle.uuid == handle.uuid) {
            spdlog::info("[LuaRuntime] Hot reloading config: {}", path);

            // Re-execute the config file and update the global
            std::filesystem::path resolvedPath = PathResolver::resolve(path);
            std::ifstream file(resolvedPath);
            if (file) {
                std::stringstream buffer;
                buffer << file.rdbuf();
                auto result = lua_.safe_script(buffer.str(), sol::script_pass_on_error);
                if (result.valid()) {
                    // Store result in global (extract name from path)
                    std::string globalName = std::filesystem::path(path).stem().string();
                    // Convert kebab-case to camelCase for Lua global
                    std::string luaName;
                    bool capitalizeNext = false;
                    for (char c : globalName) {
                        if (c == '-') {
                            capitalizeNext = true;
                        } else if (capitalizeNext) {
                            luaName += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                            capitalizeNext = false;
                        } else {
                            luaName += c;
                        }
                    }
                    lua_[luaName] = result.get<sol::table>();

                    // Notify all subscribers
                    notifyConfigReloaded(path);
                } else {
                    sol::error err = result;
                    spdlog::error("[LuaRuntime] Failed to reload config '{}': {}",
                                  path, err.what());
                }
            }
            return;
        }
    }
}

void LuaRuntime::notifyConfigReloaded(std::string_view path) {
    for (const auto& sub : configReloadCallbacks_) {
        if (sub.pathPattern.empty()) {
            // Subscribe to all - always call
            sub.callback(path);
        } else {
            // Check if path matches pattern (simple prefix/suffix matching for now)
            std::string pathStr(path);
            if (pathStr.find(sub.pathPattern) != std::string::npos ||
                sub.pathPattern == "*") {
                sub.callback(path);
            }
        }
    }
}

SubscriptionId LuaRuntime::onConfigReloaded(ConfigReloadCallback callback) {
    auto id = nextConfigSubId_++;
    configReloadCallbacks_.push_back({
        .id = id,
        .pathPattern = "",
        .callback = std::move(callback)
    });
    return id;
}

SubscriptionId LuaRuntime::onConfigReloaded(std::string_view pathPattern,
                                             ConfigReloadCallback callback) {
    auto id = nextConfigSubId_++;
    configReloadCallbacks_.push_back({
        .id = id,
        .pathPattern = std::string(pathPattern),
        .callback = std::move(callback)
    });
    return id;
}

void LuaRuntime::unsubscribeConfigReload(SubscriptionId id) {
    std::erase_if(configReloadCallbacks_, [id](const auto& sub) {
        return sub.id == id;
    });
}

AssetHandle LuaRuntime::watchConfig(std::string_view path) {
    std::string pathStr(path);

    // Check if already watching
    auto it = watchedConfigs_.find(pathStr);
    if (it != watchedConfigs_.end()) {
        return it->second;
    }

    if (!pIAssetSystem_) {
        spdlog::warn("[LuaRuntime] Cannot watch config '{}': AssetSystem not available", path);
        return {};
    }

    // Resolve the path
    std::filesystem::path resolvedPath = PathResolver::resolve(pathStr);

    // Register with AssetSystem as a Data asset
    auto handle = pIAssetSystem_->registerAsset(AssetType::Data, resolvedPath.string());
    watchedConfigs_[pathStr] = handle;

    // Load the asset so it's tracked
    pIAssetSystem_->loadAsset(handle);

    spdlog::info("[LuaRuntime] Watching config for hot reload: {}", path);
    return handle;
}

//==============================================================================
// Engine System Availability
//==============================================================================

bool LuaRuntime::isSystemAvailable(std::string_view systemName) const {
    return availableSystems_.contains(std::string(systemName));
}

std::vector<std::string> LuaRuntime::getAvailableSystems() const {
    return std::vector<std::string>(availableSystems_.begin(), availableSystems_.end());
}

//==============================================================================
// Internal Helpers
//==============================================================================

LuaResult<void> LuaRuntime::loadLuaFile(std::string_view path, std::string_view description) {
    std::filesystem::path resolvedPath = PathResolver::resolve(std::string(path));

    std::ifstream file(resolvedPath);
    if (!file.is_open()) {
        return std::unexpected(LuaError{
            .code = LuaErrorCode::FileNotFound,
            .message = std::format("Could not open file: {}", resolvedPath.string()),
            .source = std::string(path)
        });
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    auto result = lua_.safe_script(buffer.str(), sol::script_pass_on_error);
    if (!result.valid()) {
        sol::error err = result;
        return std::unexpected(LuaError{
            .code = LuaErrorCode::ParseError,
            .message = err.what(),
            .source = std::string(description)
        });
    }

    return {};
}

}  // namespace bestow
