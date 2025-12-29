// bestow-lua/src/LuaBehaviors.cpp
// Behavior system implementation with hot reload and state preservation

module;

#include <sol/sol.hpp>
#include <spdlog/spdlog.h>

module bestow.lua.impl;

import std;
import bestow.services;

namespace bestow {

//==============================================================================
// Behavior Loading
//==============================================================================

LuaResult<BehaviorHandle> LuaRuntime::loadBehavior(std::string_view path) {
    std::filesystem::path resolvedPath = PathResolver::resolve(std::string(path));

    std::ifstream file(resolvedPath);
    if (!file.is_open()) {
        return std::unexpected(LuaError{
            .code = LuaErrorCode::FileNotFound,
            .message = std::format("Could not open behavior file: {}", resolvedPath.string()),
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

    // Behavior file should return a table with callbacks
    if (result.get_type() != sol::type::table) {
        return std::unexpected(LuaError{
            .code = LuaErrorCode::InvalidBehavior,
            .message = "Behavior file must return a table",
            .source = std::string(path)
        });
    }

    sol::table behaviorTable = result.get<sol::table>();

    // Extract behavior name
    std::string behaviorName;
    sol::optional<std::string> name = behaviorTable["name"];
    if (name) {
        behaviorName = *name;
    } else {
        // Use filename as behavior name
        behaviorName = resolvedPath.stem().string();
    }

    // Check if behavior already exists
    auto existingIt = behaviorsByName_.find(behaviorName);
    if (existingIt != behaviorsByName_.end()) {
        // Update existing behavior (hot reload case)
        auto& existing = loadedBehaviors_[existingIt->second.id];
        existing.definition = behaviorTable;
        existing.path = std::string(path);

        spdlog::info("[LuaRuntime] Reloaded behavior: {}", behaviorName);
        return existingIt->second;
    }

    // Create new behavior
    BehaviorHandle handle{nextBehaviorId_++};

    LoadedBehavior loadedBehavior{
        .handle = handle,
        .name = behaviorName,
        .path = std::string(path),
        .asset = {},
        .definition = behaviorTable
    };

    // Register with asset system for hot reload
    if (pIAssetSystem_ && hotReloadEnabled_) {
        loadedBehavior.asset = pIAssetSystem_->registerAsset(AssetType::Data, resolvedPath);
    }

    loadedBehaviors_[handle.id] = std::move(loadedBehavior);
    behaviorsByName_[behaviorName] = handle;

    spdlog::info("[LuaRuntime] Loaded behavior: {} (id={})", behaviorName, handle.id);
    return handle;
}

LuaResult<void> LuaRuntime::loadAllBehaviors() {
    std::filesystem::path behaviorsDir = PathResolver::resolve(std::string(LuaPath::Behaviors));

    if (!std::filesystem::exists(behaviorsDir)) {
        spdlog::debug("[LuaRuntime] Behaviors directory does not exist: {}", behaviorsDir.string());
        return {};
    }

    for (const auto& entry : std::filesystem::directory_iterator(behaviorsDir)) {
        if (entry.path().extension() == ".lua") {
            auto result = loadBehavior(entry.path().string());
            if (!result) {
                spdlog::warn("[LuaRuntime] Failed to load behavior '{}': {}",
                             entry.path().string(), result.error().message);
            }
        }
    }

    return {};
}

//==============================================================================
// Behavior Attachment
//==============================================================================

LuaResult<void> LuaRuntime::attachBehavior(Entity entity, BehaviorHandle behavior) {
    if (!pIEntitySystem_->isValid(entity)) {
        return std::unexpected(LuaError{
            .code = LuaErrorCode::EntityNotFound,
            .message = "Entity is not valid",
            .source = "attachBehavior"
        });
    }

    auto it = loadedBehaviors_.find(behavior.id);
    if (it == loadedBehaviors_.end()) {
        return std::unexpected(LuaError{
            .code = LuaErrorCode::InvalidBehavior,
            .message = std::format("Behavior with id {} not found", behavior.id),
            .source = "attachBehavior"
        });
    }

    const auto& loadedBehavior = it->second;

    // Create a new state table for this instance
    sol::table state = lua_.create_table();

    // Create callbacks table from the behavior definition
    sol::table callbacks = lua_.create_table();
    for (const auto& [key, value] : loadedBehavior.definition) {
        if (key.is<std::string>() && value.is<sol::function>()) {
            callbacks[key] = value;
        }
    }

    // Create the behavior instance
    BehaviorInstance instance{
        .handle = behavior,
        .entity = entity,
        .state = state,
        .callbacks = callbacks,
        .enabled = true,
        .behaviorName = loadedBehavior.name
    };

    // Call init if provided
    sol::optional<sol::function> initFn = loadedBehavior.definition["init"];
    if (initFn) {
        // Create entity wrapper for Lua
        sol::table entityWrapper = createEntityWrapper(entity);

        auto result = (*initFn)(entityWrapper, state);
        if (!result.valid()) {
            sol::error err = result;
            spdlog::error("[LuaRuntime] Behavior '{}' init error: {}",
                          loadedBehavior.name, err.what());
        }
    }

    behaviorInstances_.push_back(std::move(instance));

    spdlog::debug("[LuaRuntime] Attached behavior '{}' to entity {}",
                  loadedBehavior.name, static_cast<std::uint32_t>(entity));
    return {};
}

LuaResult<void> LuaRuntime::attachBehavior(Entity entity, std::string_view behaviorName) {
    auto it = behaviorsByName_.find(std::string(behaviorName));
    if (it == behaviorsByName_.end()) {
        // Try to load it first
        std::string path = std::format("{}{}.lua", LuaPath::Behaviors, behaviorName);
        auto loadResult = loadBehavior(path);
        if (!loadResult) {
            return std::unexpected(LuaError{
                .code = LuaErrorCode::InvalidBehavior,
                .message = std::format("Behavior '{}' not found", behaviorName),
                .source = "attachBehavior"
            });
        }
        return attachBehavior(entity, *loadResult);
    }

    return attachBehavior(entity, it->second);
}

void LuaRuntime::detachBehavior(Entity entity, BehaviorHandle behavior) {
    auto it = std::find_if(behaviorInstances_.begin(), behaviorInstances_.end(),
        [entity, behavior](const BehaviorInstance& inst) {
            return inst.entity == entity && inst.handle.id == behavior.id;
        });

    if (it != behaviorInstances_.end()) {
        // Call destroy if provided
        callBehaviorCallback(*it, "destroy", 0.0f);

        behaviorInstances_.erase(it);
        spdlog::debug("[LuaRuntime] Detached behavior {} from entity {}",
                      behavior.id, static_cast<std::uint32_t>(entity));
    }
}

void LuaRuntime::detachAllBehaviors(Entity entity) {
    auto it = behaviorInstances_.begin();
    while (it != behaviorInstances_.end()) {
        if (it->entity == entity) {
            callBehaviorCallback(*it, "destroy", 0.0f);
            it = behaviorInstances_.erase(it);
        } else {
            ++it;
        }
    }
}

std::vector<BehaviorHandle> LuaRuntime::getBehaviors(Entity entity) const {
    std::vector<BehaviorHandle> handles;
    for (const auto& inst : behaviorInstances_) {
        if (inst.entity == entity) {
            handles.push_back(inst.handle);
        }
    }
    return handles;
}

void LuaRuntime::setBehaviorEnabled(Entity entity, BehaviorHandle behavior, bool enabled) {
    for (auto& inst : behaviorInstances_) {
        if (inst.entity == entity && inst.handle.id == behavior.id) {
            if (inst.enabled != enabled) {
                inst.enabled = enabled;
                if (enabled) {
                    callBehaviorCallback(inst, "onEnable", 0.0f);
                } else {
                    callBehaviorCallback(inst, "onDisable", 0.0f);
                }
            }
            break;
        }
    }
}

sol::table LuaRuntime::getBehaviorState(Entity entity, BehaviorHandle behavior) {
    for (const auto& inst : behaviorInstances_) {
        if (inst.entity == entity && inst.handle.id == behavior.id) {
            return inst.state;
        }
    }
    return sol::table();
}

//==============================================================================
// Behavior Callbacks
//==============================================================================

void LuaRuntime::callBehaviorCallback(BehaviorInstance& instance,
                                       std::string_view callback,
                                       float dt) {
    sol::optional<sol::function> fn = instance.callbacks[std::string(callback)];
    if (!fn) return;

    // Create entity wrapper
    sol::table entityWrapper = createEntityWrapper(instance.entity);

    sol::protected_function_result result;
    if (callback == "update" || callback == "fixedUpdate") {
        result = (*fn)(entityWrapper, instance.state, dt);
    } else {
        result = (*fn)(entityWrapper, instance.state);
    }

    if (!result.valid()) {
        sol::error err = result;
        spdlog::error("[LuaRuntime] Behavior '{}' {} error: {}",
                      instance.behaviorName, callback, err.what());
    }
}

//==============================================================================
// Behavior Hot Reload
//==============================================================================

void LuaRuntime::reloadBehavior(LoadedBehavior& behavior) {
    std::filesystem::path resolvedPath = PathResolver::resolve(behavior.path);

    std::ifstream file(resolvedPath);
    if (!file.is_open()) {
        spdlog::error("[LuaRuntime] Could not reload behavior '{}': file not found",
                      behavior.name);
        return;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string luaCode = buffer.str();

    auto result = lua_.safe_script(luaCode, sol::script_pass_on_error);
    if (!result.valid()) {
        sol::error err = result;
        spdlog::error("[LuaRuntime] Behavior '{}' reload error: {}",
                      behavior.name, err.what());
        return;
    }

    if (result.get_type() != sol::type::table) {
        spdlog::error("[LuaRuntime] Behavior '{}' reload: file must return a table",
                      behavior.name);
        return;
    }

    // Update the behavior definition
    behavior.definition = result.get<sol::table>();

    // Update all instances of this behavior (preserving state)
    for (auto& instance : behaviorInstances_) {
        if (instance.handle.id == behavior.handle.id) {
            // Rebuild callbacks from new definition
            sol::table callbacks = lua_.create_table();
            for (const auto& [key, value] : behavior.definition) {
                if (key.is<std::string>() && value.is<sol::function>()) {
                    callbacks[key] = value;
                }
            }
            instance.callbacks = callbacks;

            // Call onReload if provided
            sol::optional<sol::function> reloadFn = behavior.definition["onReload"];
            if (reloadFn) {
                sol::table entityWrapper = createEntityWrapper(instance.entity);
                auto reloadResult = (*reloadFn)(entityWrapper, instance.state);
                if (!reloadResult.valid()) {
                    sol::error err = reloadResult;
                    spdlog::warn("[LuaRuntime] Behavior '{}' onReload error: {}",
                                 behavior.name, err.what());
                }
            }
        }
    }

    spdlog::info("[LuaRuntime] Hot reloaded behavior: {}", behavior.name);
}

//==============================================================================
// Entity Wrapper Creation
//==============================================================================

sol::table LuaRuntime::createEntityWrapper(Entity entity) {
    sol::table wrapper = lua_.create_table();

    // Store the entity ID
    wrapper["_id"] = static_cast<std::uint32_t>(entity);

    // Add isValid method
    wrapper["isValid"] = [this, entity]() {
        return pIEntitySystem_->isValid(entity);
    };

    // Add getPosition method
    wrapper["getPosition"] = [this, entity]() -> sol::table {
        auto* transform = pIEntitySystem_->tryGet<Transform2D>(entity);
        sol::table pos = lua_.create_table();
        if (transform) {
            pos["x"] = transform->x;
            pos["y"] = transform->y;
        } else {
            pos["x"] = 0.0f;
            pos["y"] = 0.0f;
        }
        return pos;
    };

    // Add setPosition method
    wrapper["setPosition"] = [this, entity](float x, float y) {
        auto* transform = pIEntitySystem_->tryGet<Transform2D>(entity);
        if (transform) {
            transform->x = x;
            transform->y = y;
        }
    };

    // Add getVelocity method
    wrapper["getVelocity"] = [this, entity]() -> sol::table {
        auto* velocity = pIEntitySystem_->tryGet<Velocity2D>(entity);
        sol::table vel = lua_.create_table();
        if (velocity) {
            vel["x"] = velocity->x;
            vel["y"] = velocity->y;
        } else {
            vel["x"] = 0.0f;
            vel["y"] = 0.0f;
        }
        return vel;
    };

    // Add setVelocity method
    wrapper["setVelocity"] = [this, entity](float x, float y) {
        auto* velocity = pIEntitySystem_->tryGet<Velocity2D>(entity);
        if (velocity) {
            velocity->x = x;
            velocity->y = y;
        }
    };

    // Add getRotation method
    wrapper["getRotation"] = [this, entity]() -> float {
        auto* transform = pIEntitySystem_->tryGet<Transform2D>(entity);
        return transform ? transform->rotation : 0.0f;
    };

    // Add setRotation method
    wrapper["setRotation"] = [this, entity](float rotation) {
        auto* transform = pIEntitySystem_->tryGet<Transform2D>(entity);
        if (transform) {
            transform->rotation = rotation;
        }
    };

    // Add getScale method
    wrapper["getScale"] = [this, entity]() -> sol::table {
        auto* transform = pIEntitySystem_->tryGet<Transform2D>(entity);
        sol::table scale = lua_.create_table();
        if (transform) {
            scale["x"] = transform->scaleX;
            scale["y"] = transform->scaleY;
        } else {
            scale["x"] = 1.0f;
            scale["y"] = 1.0f;
        }
        return scale;
    };

    // Add setScale method
    wrapper["setScale"] = [this, entity](float x, float y) {
        auto* transform = pIEntitySystem_->tryGet<Transform2D>(entity);
        if (transform) {
            transform->scaleX = x;
            transform->scaleY = y;
        }
    };

    // Add destroy method
    wrapper["destroy"] = [this, entity]() {
        detachAllBehaviors(entity);
        pIEntitySystem_->destroyEntity(entity);
    };

    // Add getTag method
    wrapper["getTag"] = [this, entity]() -> std::string {
        auto* tag = pIEntitySystem_->tryGet<Tag>(entity);
        return tag ? tag->value : "";
    };

    // Add setTag method
    wrapper["setTag"] = [this, entity](std::string_view tag) {
        if (auto* existing = pIEntitySystem_->tryGet<Tag>(entity)) {
            existing->value = std::string(tag);
        } else {
            pIEntitySystem_->emplace<Tag>(entity, Tag{.value = std::string(tag)});
        }
    };

    // Add getName method
    wrapper["getName"] = [this, entity]() -> std::string {
        auto* name = pIEntitySystem_->tryGet<Name>(entity);
        return name ? name->value : "";
    };

    // Add setName method
    wrapper["setName"] = [this, entity](std::string_view name) {
        if (auto* existing = pIEntitySystem_->tryGet<Name>(entity)) {
            existing->value = std::string(name);
        } else {
            pIEntitySystem_->emplace<Name>(entity, Name{.value = std::string(name)});
        }
    };

    return wrapper;
}

//==============================================================================
// Lua System Management
//==============================================================================

LuaResult<LuaSystemHandle> LuaRuntime::loadSystem(std::string_view path) {
    std::filesystem::path resolvedPath = PathResolver::resolve(std::string(path));

    std::ifstream file(resolvedPath);
    if (!file.is_open()) {
        return std::unexpected(LuaError{
            .code = LuaErrorCode::FileNotFound,
            .message = std::format("Could not open system file: {}", resolvedPath.string()),
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

    if (result.get_type() != sol::type::table) {
        return std::unexpected(LuaError{
            .code = LuaErrorCode::InvalidSystem,
            .message = "System file must return a table",
            .source = std::string(path)
        });
    }

    sol::table systemTable = result.get<sol::table>();

    // Extract system name
    std::string systemName;
    sol::optional<std::string> name = systemTable["name"];
    if (name) {
        systemName = *name;
    } else {
        systemName = resolvedPath.stem().string();
    }

    // Extract priority
    int priority = systemTable["priority"].get_or(0);

    // Create system handle
    LuaSystemHandle handle{nextSystemId_++};

    LoadedLuaSystem loadedSystem{
        .handle = handle,
        .name = systemName,
        .path = std::string(path),
        .asset = {},
        .definition = systemTable,
        .priority = priority,
        .enabled = true
    };

    // Register with asset system for hot reload
    if (pIAssetSystem_ && hotReloadEnabled_) {
        loadedSystem.asset = pIAssetSystem_->registerAsset(AssetType::Data, resolvedPath);
    }

    loadedSystems_[handle.id] = std::move(loadedSystem);

    // Re-sort update order by priority
    systemUpdateOrder_.push_back(handle);
    std::sort(systemUpdateOrder_.begin(), systemUpdateOrder_.end(),
        [this](const LuaSystemHandle& a, const LuaSystemHandle& b) {
            return loadedSystems_[a.id].priority < loadedSystems_[b.id].priority;
        });

    // Call init if provided
    sol::optional<sol::function> initFn = systemTable["init"];
    if (initFn) {
        auto initResult = (*initFn)();
        if (!initResult.valid()) {
            sol::error err = initResult;
            spdlog::error("[LuaRuntime] System '{}' init error: {}",
                          systemName, err.what());
        }
    }

    spdlog::info("[LuaRuntime] Loaded system: {} (priority={})", systemName, priority);
    return handle;
}

LuaResult<void> LuaRuntime::loadAllSystems() {
    std::filesystem::path systemsDir = PathResolver::resolve(std::string(LuaPath::Systems));

    if (!std::filesystem::exists(systemsDir)) {
        spdlog::debug("[LuaRuntime] Systems directory does not exist: {}", systemsDir.string());
        return {};
    }

    for (const auto& entry : std::filesystem::directory_iterator(systemsDir)) {
        if (entry.path().extension() == ".lua") {
            auto result = loadSystem(entry.path().string());
            if (!result) {
                spdlog::warn("[LuaRuntime] Failed to load system '{}': {}",
                             entry.path().string(), result.error().message);
            }
        }
    }

    return {};
}

void LuaRuntime::setSystemEnabled(LuaSystemHandle system, bool enabled) {
    auto it = loadedSystems_.find(system.id);
    if (it != loadedSystems_.end()) {
        it->second.enabled = enabled;
    }
}

std::vector<LuaSystemHandle> LuaRuntime::getLuaSystems() const {
    std::vector<LuaSystemHandle> handles;
    handles.reserve(loadedSystems_.size());
    for (const auto& [id, sys] : loadedSystems_) {
        handles.push_back(sys.handle);
    }
    return handles;
}

std::optional<LuaSystemDef> LuaRuntime::getSystemInfo(LuaSystemHandle system) const {
    auto it = loadedSystems_.find(system.id);
    if (it == loadedSystems_.end()) {
        return std::nullopt;
    }

    const auto& sys = it->second;
    return LuaSystemDef{
        .name = sys.name,
        .priority = sys.priority
    };
}

}  // namespace bestow
