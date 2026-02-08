// bestow-scene/src/SceneSystem.cpp
// Scene system implementation

module;

#include <any>
#include <cstdint>
#include <optional>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

#include <bestow/sol2_compat.hpp>
#include <spdlog/spdlog.h>

module bestow.scene.impl;

namespace bestow {

//==========================================================================
// Lifecycle
//==========================================================================

SceneSystem::~SceneSystem() {
    // Unsubscribe from asset hot reload
    if (assets_ && assetSub_ != 0) {
        assets_->unsubscribe(assetSub_);
        assetSub_ = 0;
    }

    // Clear the stack (calls exit/shutdown on active scenes)
    clearStack();
}

void SceneSystem::initialize(sol::state* lua) {
    lua_ = lua;

    // Subscribe to scene asset changes for hot reload
    if (assets_) {
        assetSub_ = assets_->subscribeToType(
            AssetType::Scene,
            [this](AssetHandle handle, AssetType type) {
                onAssetChanged(handle, type);
            });
    }

    spdlog::debug("[SceneSystem] Initialized");
}

void SceneSystem::update(DeltaTime dt) {
    if (stack_.empty()) {
        return;
    }

    // Only update the top (active) scene
    auto& top = stack_.back();
    auto regIt = registry_.find(top.name);
    if (regIt == registry_.end()) {
        return;
    }

    auto& def = regIt->second.def;
    if (def.table.valid() && def.table["update"].valid()) {
        sol::protected_function updateFn = def.table["update"];
        sol::protected_function_result result = updateFn(dt);
        if (!result.valid()) {
            sol::error err = result;
            spdlog::error("[SceneSystem] Scene '{}' update() error: {}",
                          top.name, err.what());
        } else if (result.return_count() > 0) {
            // If update returns false, request pop
            sol::optional<bool> shouldContinue = result;
            if (shouldContinue.has_value() && !shouldContinue.value()) {
                popScene();
            }
        }
    }
}

void SceneSystem::render() {
    if (stack_.empty()) {
        return;
    }

    // Only render the top (active) scene
    auto& top = stack_.back();
    auto regIt = registry_.find(top.name);
    if (regIt == registry_.end()) {
        return;
    }

    auto& def = regIt->second.def;
    if (def.table.valid() && def.table["render"].valid()) {
        sol::protected_function renderFn = def.table["render"];
        sol::protected_function_result result = renderFn();
        if (!result.valid()) {
            sol::error err = result;
            spdlog::error("[SceneSystem] Scene '{}' render() error: {}",
                          top.name, err.what());
        }
    }
}

//==========================================================================
// Registration
//==========================================================================

Result<SceneId, std::error_code> SceneSystem::registerScene(
    const std::string& name, AssetHandle sceneAsset) {

    if (registry_.contains(name)) {
        spdlog::warn("[SceneSystem] Scene '{}' already registered", name);
        return std::unexpected(std::make_error_code(std::errc::file_exists));
    }

    SceneId id = generateId();

    // Try to load and parse the scene Lua file if asset system is available
    SceneDef def{};
    if (assets_) {
        const void* rawAsset = assets_->getRawAsset(sceneAsset);
        if (rawAsset) {
            const std::any* assetAny = static_cast<const std::any*>(rawAsset);
            // Pointer-based any_cast returns nullptr on type mismatch (no throw)
            const DataAsset* dataAsset = std::any_cast<DataAsset>(assetAny);
            if (dataAsset && !dataAsset->rawText.empty()) {
                auto parseResult = parseSceneLua(dataAsset->rawText);
                if (parseResult.has_value()) {
                    def = std::move(parseResult.value());
                } else {
                    spdlog::warn("[SceneSystem] Failed to parse scene '{}', "
                                 "registering with empty definition", name);
                }
            }
        }
    }

    RegisteredScene reg{
        .metadata = SceneMetadata{
            .id = id,
            .name = name,
            .assetHandle = sceneAsset,
            .phase = def.phase,
            .state = SceneState::Ready,
        },
        .def = std::move(def),
    };

    // Call init() if defined
    if (reg.def.table.valid()) {
        callLuaCallback(reg.def.table, "init");
    }

    registry_[name] = std::move(reg);
    spdlog::debug("[SceneSystem] Registered scene '{}' (id={})", name, id);

    return id;
}

Result<void, std::error_code> SceneSystem::unregisterScene(
    const std::string& name) {

    auto it = registry_.find(name);
    if (it == registry_.end()) {
        return std::unexpected(
            std::make_error_code(std::errc::no_such_file_or_directory));
    }

    // Check if scene is on the stack
    for (const auto& entry : stack_) {
        if (entry.name == name) {
            spdlog::error("[SceneSystem] Cannot unregister scene '{}' while "
                          "it is on the stack", name);
            return std::unexpected(
                std::make_error_code(std::errc::device_or_resource_busy));
        }
    }

    // Call shutdown() if defined
    if (it->second.def.table.valid()) {
        callLuaCallback(it->second.def.table, "shutdown");
    }

    registry_.erase(it);
    spdlog::debug("[SceneSystem] Unregistered scene '{}'", name);

    return {};
}

//==========================================================================
// Stack Operations
//==========================================================================

Result<void, std::error_code> SceneSystem::pushScene(
    const std::string& name,
    const std::unordered_map<std::string, std::any>& params) {

    auto regIt = registry_.find(name);
    if (regIt == registry_.end()) {
        spdlog::error("[SceneSystem] Cannot push unregistered scene '{}'", name);
        return std::unexpected(
            std::make_error_code(std::errc::no_such_file_or_directory));
    }

    // Pause the current top scene
    if (!stack_.empty()) {
        auto& current = stack_.back();
        auto currentRegIt = registry_.find(current.name);
        if (currentRegIt != registry_.end()) {
            // Call exit() on current scene
            callLuaCallback(currentRegIt->second.def.table, "exit");
            cleanupSceneLuaSubs(current.name);
            currentRegIt->second.metadata.state = SceneState::Paused;

            // Hide current scene's UI
            hideSceneUI(current);
        }
    }

    // Create stack entry
    StackEntry entry{.name = name, .uiDocs = {}};

    // Push input phase
    if (input_ && !regIt->second.def.phase.empty()) {
        input_->pushPhase(regIt->second.def.phase);
    }

    // Load and show UI documents
    loadSceneUI(entry, regIt->second.def);

    // Update state
    regIt->second.metadata.state = SceneState::Active;
    stack_.push_back(std::move(entry));

    // Call enter(params) on new scene
    sol::table luaParams = paramsToLuaTable(params);
    callLuaCallback(regIt->second.def.table, "enter", luaParams);

    // Publish event
    publishEvent(Events::ScenePushed, name,
                 regIt->second.metadata.id, SceneState::Active);

    spdlog::debug("[SceneSystem] Pushed scene '{}' (stack depth: {})",
                  name, stack_.size());

    return {};
}

Result<void, std::error_code> SceneSystem::popScene() {
    if (stack_.empty()) {
        spdlog::warn("[SceneSystem] Cannot pop: stack is empty");
        return std::unexpected(
            std::make_error_code(std::errc::operation_not_permitted));
    }

    auto& top = stack_.back();
    std::string poppedName = top.name;

    auto regIt = registry_.find(poppedName);
    if (regIt != registry_.end()) {
        // Call exit() on current scene
        callLuaCallback(regIt->second.def.table, "exit");
        cleanupSceneLuaSubs(poppedName);

        // Hide and unload UI
        unloadSceneUI(top);

        // Pop input phase
        if (input_ && !regIt->second.def.phase.empty()) {
            input_->popPhase();
        }

        regIt->second.metadata.state = SceneState::Ready;
    }

    SceneId poppedId = regIt != registry_.end() ? regIt->second.metadata.id : 0;
    stack_.pop_back();

    // Resume the previous scene
    if (!stack_.empty()) {
        auto& newTop = stack_.back();
        auto newTopRegIt = registry_.find(newTop.name);
        if (newTopRegIt != registry_.end()) {
            newTopRegIt->second.metadata.state = SceneState::Active;

            // Show previous scene's UI
            showSceneUI(newTop);

            // Call enter() on resumed scene (no params on resume)
            callLuaCallback(newTopRegIt->second.def.table, "enter");
        }
    }

    // Publish event
    publishEvent(Events::ScenePopped, poppedName,
                 poppedId, SceneState::Ready);

    spdlog::debug("[SceneSystem] Popped scene '{}' (stack depth: {})",
                  poppedName, stack_.size());

    return {};
}

Result<void, std::error_code> SceneSystem::replaceScene(
    const std::string& name,
    const std::unordered_map<std::string, std::any>& params) {

    auto newRegIt = registry_.find(name);
    if (newRegIt == registry_.end()) {
        spdlog::error("[SceneSystem] Cannot replace with unregistered scene '{}'",
                      name);
        return std::unexpected(
            std::make_error_code(std::errc::no_such_file_or_directory));
    }

    // Tear down the current top scene
    if (!stack_.empty()) {
        auto& top = stack_.back();
        auto oldRegIt = registry_.find(top.name);
        if (oldRegIt != registry_.end()) {
            // Call exit() + shutdown() on current scene
            callLuaCallback(oldRegIt->second.def.table, "exit");
            cleanupSceneLuaSubs(top.name);
            callLuaCallback(oldRegIt->second.def.table, "shutdown");

            // Hide and unload UI
            unloadSceneUI(top);

            // Pop input phase for old scene
            if (input_ && !oldRegIt->second.def.phase.empty()) {
                input_->popPhase();
            }

            oldRegIt->second.metadata.state = SceneState::Ready;
        }

        stack_.pop_back();
    }

    // Create new stack entry
    StackEntry entry{.name = name, .uiDocs = {}};

    // Push input phase for new scene
    if (input_ && !newRegIt->second.def.phase.empty()) {
        input_->pushPhase(newRegIt->second.def.phase);
    }

    // Load and show UI documents
    loadSceneUI(entry, newRegIt->second.def);

    // Update state
    newRegIt->second.metadata.state = SceneState::Active;
    stack_.push_back(std::move(entry));

    // Call enter(params) on new scene
    sol::table luaParams = paramsToLuaTable(params);
    callLuaCallback(newRegIt->second.def.table, "enter", luaParams);

    // Publish event
    publishEvent(Events::SceneReplaced, name,
                 newRegIt->second.metadata.id, SceneState::Active);

    spdlog::debug("[SceneSystem] Replaced top with scene '{}' (stack depth: {})",
                  name, stack_.size());

    return {};
}

void SceneSystem::clearStack() {
    // Pop all scenes from top to bottom
    while (!stack_.empty()) {
        auto& top = stack_.back();
        auto regIt = registry_.find(top.name);
        if (regIt != registry_.end()) {
            callLuaCallback(regIt->second.def.table, "exit");
            cleanupSceneLuaSubs(top.name);
            callLuaCallback(regIt->second.def.table, "shutdown");
            unloadSceneUI(top);

            if (input_ && !regIt->second.def.phase.empty()) {
                input_->popPhase();
            }

            regIt->second.metadata.state = SceneState::Ready;
        }
        stack_.pop_back();
    }

    spdlog::debug("[SceneSystem] Stack cleared");
}

//==========================================================================
// Queries
//==========================================================================

std::optional<std::string> SceneSystem::getActiveSceneName() const {
    if (stack_.empty()) {
        return std::nullopt;
    }
    return stack_.back().name;
}

std::vector<std::string> SceneSystem::getSceneStack() const {
    std::vector<std::string> result;
    result.reserve(stack_.size());
    for (const auto& entry : stack_) {
        result.push_back(entry.name);
    }
    return result;
}

SceneState SceneSystem::getSceneState(const std::string& name) const {
    auto it = registry_.find(name);
    if (it == registry_.end()) {
        return SceneState::Unloaded;
    }
    return it->second.metadata.state;
}

std::optional<SceneMetadata> SceneSystem::getSceneMetadata(
    const std::string& name) const {
    auto it = registry_.find(name);
    if (it == registry_.end()) {
        return std::nullopt;
    }
    return it->second.metadata;
}

std::vector<std::string> SceneSystem::getRegisteredScenes() const {
    std::vector<std::string> result;
    result.reserve(registry_.size());
    for (const auto& [name, _] : registry_) {
        result.push_back(name);
    }
    return result;
}

//==========================================================================
// Internal Helpers
//==========================================================================

Result<SceneSystem::SceneDef, std::error_code> SceneSystem::parseSceneLua(
    const std::string& luaCode) {

    if (!lua_) {
        return std::unexpected(
            std::make_error_code(std::errc::no_such_device));
    }

    sol::protected_function_result result =
        lua_->safe_script(luaCode, sol::script_pass_on_error);

    if (!result.valid()) {
        sol::error err = result;
        spdlog::error("[SceneSystem] Lua parse error: {}", err.what());
        return std::unexpected(
            std::make_error_code(std::errc::invalid_argument));
    }

    if (result.get_type() != sol::type::table) {
        spdlog::error("[SceneSystem] Scene Lua must return a table");
        return std::unexpected(
            std::make_error_code(std::errc::invalid_argument));
    }

    sol::table table = result;
    SceneDef def;
    def.table = table;

    // Extract phase
    sol::optional<std::string> phase = table["phase"];
    if (phase.has_value()) {
        def.phase = phase.value();
    }

    // Extract UI document paths
    if (table["ui"].valid() && table["ui"].get_type() == sol::type::table) {
        sol::table uiTable = table["ui"];
        for (std::size_t i = 1; i <= uiTable.size(); ++i) {
            sol::optional<std::string> path = uiTable[i];
            if (path.has_value()) {
                def.uiPaths.push_back(path.value());
            }
        }
    }

    return def;
}

sol::table SceneSystem::paramsToLuaTable(
    const std::unordered_map<std::string, std::any>& params) {

    if (!lua_ || params.empty()) {
        return sol::lua_nil;
    }

    sol::table table = lua_->create_table();
    for (const auto& [key, value] : params) {
        if (auto* s = std::any_cast<std::string>(&value)) {
            table[key] = *s;
        } else if (auto* d = std::any_cast<double>(&value)) {
            table[key] = *d;
        } else if (auto* f = std::any_cast<float>(&value)) {
            table[key] = *f;
        } else if (auto* i = std::any_cast<int>(&value)) {
            table[key] = *i;
        } else if (auto* b = std::any_cast<bool>(&value)) {
            table[key] = *b;
        }
        // Other types are silently skipped
    }
    return table;
}

void SceneSystem::callLuaCallback(const sol::table& sceneDef,
                                   const char* name, sol::table params) {
    if (!sceneDef.valid()) {
        return;
    }

    sol::object fn = sceneDef[name];
    if (!fn.valid() || fn.get_type() != sol::type::function) {
        return;
    }

    sol::protected_function callback = fn;
    sol::protected_function_result result;

    if (params.valid()) {
        result = callback(params);
    } else {
        result = callback();
    }

    if (!result.valid()) {
        sol::error err = result;
        spdlog::error("[SceneSystem] Lua callback '{}' error: {}",
                      name, err.what());
    }
}

template<typename T>
std::optional<T> SceneSystem::callLuaCallbackWithReturn(
    const sol::table& sceneDef, const char* name) {

    if (!sceneDef.valid()) {
        return std::nullopt;
    }

    sol::object fn = sceneDef[name];
    if (!fn.valid() || fn.get_type() != sol::type::function) {
        return std::nullopt;
    }

    sol::protected_function callback = fn;
    sol::protected_function_result result = callback();

    if (!result.valid()) {
        sol::error err = result;
        spdlog::error("[SceneSystem] Lua callback '{}' error: {}",
                      name, err.what());
        return std::nullopt;
    }

    sol::optional<T> value = result;
    if (value.has_value()) {
        return value.value();
    }
    return std::nullopt;
}

void SceneSystem::loadSceneUI(StackEntry& entry, const SceneDef& def) {
    if (!ui_) {
        return;
    }

    for (const auto& path : def.uiPaths) {
        auto result = ui_->loadDocument(path);
        if (result.has_value()) {
            UIDocumentHandle doc = result.value();
            ui_->showDocument(doc);
            entry.uiDocs.push_back(doc);
        } else {
            spdlog::warn("[SceneSystem] Failed to load UI document: {}", path);
        }
    }
}

void SceneSystem::unloadSceneUI(StackEntry& entry) {
    if (!ui_) {
        return;
    }

    for (auto doc : entry.uiDocs) {
        ui_->hideDocument(doc);
    }
    entry.uiDocs.clear();
}

void SceneSystem::showSceneUI(const StackEntry& entry) {
    if (!ui_) {
        return;
    }

    for (auto doc : entry.uiDocs) {
        ui_->showDocument(doc);
    }
}

void SceneSystem::hideSceneUI(const StackEntry& entry) {
    if (!ui_) {
        return;
    }

    for (auto doc : entry.uiDocs) {
        ui_->hideDocument(doc);
    }
}

void SceneSystem::publishEvent(const char* eventType,
                                const std::string& sceneName,
                                SceneId id, SceneState state) {
    if (!events_) {
        return;
    }

    SceneEventData data{
        .sceneId = id,
        .sceneName = sceneName,
        .newState = state,
    };
    events_->publish(eventType, data);
}

void SceneSystem::onAssetChanged(AssetHandle handle, AssetType type) {
    if (type != AssetType::Scene) {
        return;
    }

    // Find which scene uses this asset
    for (auto& [name, reg] : registry_) {
        if (reg.metadata.assetHandle.uuid != handle.uuid) {
            continue;
        }

        spdlog::info("[SceneSystem] Hot reloading scene '{}'", name);

        // Re-parse the Lua file
        if (!assets_) {
            break;
        }

        assets_->reloadAsset(handle);
        const void* rawAsset = assets_->getRawAsset(handle);
        if (!rawAsset) {
            break;
        }

        const std::any* assetAny = static_cast<const std::any*>(rawAsset);
        // Pointer-based any_cast returns nullptr on type mismatch (no throw)
        const DataAsset* dataAsset = std::any_cast<DataAsset>(assetAny);
        if (!dataAsset || dataAsset->rawText.empty()) {
            break;
        }

        auto parseResult = parseSceneLua(dataAsset->rawText);
        if (!parseResult.has_value()) {
            spdlog::error("[SceneSystem] Failed to re-parse scene '{}'", name);
            break;
        }

        // Check if this scene is currently active (top of stack)
        bool isActive = !stack_.empty() && stack_.back().name == name;

        if (isActive) {
            // Call exit() on old definition
            callLuaCallback(reg.def.table, "exit");
            cleanupSceneLuaSubs(name);
        }

        // Replace the definition
        reg.def = std::move(parseResult.value());
        reg.metadata.phase = reg.def.phase;

        if (isActive) {
            // Call enter() on new definition
            callLuaCallback(reg.def.table, "enter");
        }

        // Publish reload event
        publishEvent(Events::SceneReloaded, name,
                     reg.metadata.id, reg.metadata.state);

        break;
    }
}

void SceneSystem::cleanupSceneLuaSubs(const std::string& sceneName) {
    if (!lua_) {
        return;
    }

    sol::object bestowObj = (*lua_)["bestow"];
    if (!bestowObj.valid() || bestowObj.get_type() != sol::type::table) {
        return;
    }

    sol::table bestow = bestowObj;
    sol::object sceneObj = bestow["scene"];
    if (!sceneObj.valid() || sceneObj.get_type() != sol::type::table) {
        return;
    }

    sol::table scene = sceneObj;
    sol::object cleanupObj = scene["_cleanupSubs"];
    if (!cleanupObj.valid() || cleanupObj.get_type() != sol::type::function) {
        return;
    }

    sol::protected_function cleanup = cleanupObj;
    sol::protected_function_result result = cleanup(sceneName);
    if (!result.valid()) {
        sol::error err = result;
        spdlog::error("[SceneSystem] _cleanupSubs error: {}", err.what());
    }
}

SceneId SceneSystem::generateId() {
    return nextId_++;
}

}  // namespace bestow
