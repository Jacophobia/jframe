// scene_binding.cpp - Scene system Lua bindings
// Exposes bestow.scene.* API

module;

#include <bestow/sol2_compat.hpp>
#include <spdlog/spdlog.h>

module bestow.luabind;

import std;

namespace bestow {

void bindSceneSystem(sol::state& lua, ISceneSystem& scene, IAssetSystem* assets) {
    sol::table bestow = lua["bestow"];
    sol::table sceneTable = lua.create_table();

    // bestow.scene.register(name, assetPath)
    // Registers a scene from an asset path. The asset is loaded via AssetSystem.
    sceneTable["register"] = [&scene, assets](
        const std::string& name, const std::string& assetPath) -> bool {

        if (!assets) {
            spdlog::error("[bestow.scene.register] AssetSystem not available");
            return false;
        }

        AssetHandle handle = assets->registerAsset(AssetType::Scene, assetPath);
        assets->loadAsset(handle);

        auto result = scene.registerScene(name, handle);
        if (!result.has_value()) {
            spdlog::error("[bestow.scene.register] Failed to register scene '{}'",
                          name);
            return false;
        }
        return true;
    };

    // bestow.scene.push(name [, params])
    // Pushes a scene onto the stack. Previous scene is paused.
    sceneTable["push"] = [&scene, &lua](
        const std::string& name, sol::optional<sol::table> paramsOpt) -> bool {

        std::unordered_map<std::string, std::any> params;
        if (paramsOpt.has_value()) {
            sol::table t = paramsOpt.value();
            for (const auto& pair : t) {
                if (!pair.first.is<std::string>()) {
                    spdlog::warn("[bestow.scene.push] Non-string key in params table, skipping");
                    continue;
                }
                std::string key = pair.first.as<std::string>();
                sol::object val = pair.second;
                if (val.is<std::string>()) {
                    params[key] = val.as<std::string>();
                } else if (val.is<double>()) {
                    params[key] = val.as<double>();
                } else if (val.is<bool>()) {
                    params[key] = val.as<bool>();
                }
            }
        }

        auto result = scene.pushScene(name, params);
        if (!result.has_value()) {
            spdlog::error("[bestow.scene.push] Failed to push scene '{}'", name);
            return false;
        }
        return true;
    };

    // bestow.scene.pop()
    // Pops the active scene. Previous scene resumes.
    sceneTable["pop"] = [&scene]() -> bool {
        auto result = scene.popScene();
        if (!result.has_value()) {
            spdlog::error("[bestow.scene.pop] Failed to pop scene");
            return false;
        }
        return true;
    };

    // bestow.scene.replace(name [, params])
    // Replaces the active scene with a different one.
    sceneTable["replace"] = [&scene, &lua](
        const std::string& name, sol::optional<sol::table> paramsOpt) -> bool {

        std::unordered_map<std::string, std::any> params;
        if (paramsOpt.has_value()) {
            sol::table t = paramsOpt.value();
            for (const auto& pair : t) {
                if (!pair.first.is<std::string>()) {
                    spdlog::warn("[bestow.scene.replace] Non-string key in params table, skipping");
                    continue;
                }
                std::string key = pair.first.as<std::string>();
                sol::object val = pair.second;
                if (val.is<std::string>()) {
                    params[key] = val.as<std::string>();
                } else if (val.is<double>()) {
                    params[key] = val.as<double>();
                } else if (val.is<bool>()) {
                    params[key] = val.as<bool>();
                }
            }
        }

        auto result = scene.replaceScene(name, params);
        if (!result.has_value()) {
            spdlog::error("[bestow.scene.replace] Failed to replace with scene '{}'",
                          name);
            return false;
        }
        return true;
    };

    // bestow.scene.clear()
    // Clears the entire scene stack.
    sceneTable["clear"] = [&scene]() {
        scene.clearStack();
    };

    // bestow.scene.active()
    // Returns the name of the active (top) scene, or nil.
    sceneTable["active"] = [&scene]() -> sol::optional<std::string> {
        auto name = scene.getActiveSceneName();
        if (name.has_value()) {
            return name.value();
        }
        return sol::nullopt;
    };

    // bestow.scene.stack()
    // Returns the full scene stack as a table (bottom to top).
    sceneTable["stack"] = [&scene, &lua]() -> sol::table {
        auto stack = scene.getSceneStack();
        sol::table result = lua.create_table();
        for (std::size_t i = 0; i < stack.size(); ++i) {
            result[i + 1] = stack[i];
        }
        return result;
    };

    // bestow.scene.state(name)
    // Returns the state of a scene as a string.
    sceneTable["state"] = [&scene](const std::string& name) -> std::string {
        SceneState state = scene.getSceneState(name);
        switch (state) {
            case SceneState::Unloaded:  return "unloaded";
            case SceneState::Loading:   return "loading";
            case SceneState::Ready:     return "ready";
            case SceneState::Active:    return "active";
            case SceneState::Paused:    return "paused";
            case SceneState::Unloading: return "unloading";
        }
        return "unknown";
    };

    // bestow.scene.registered()
    // Returns a table of all registered scene names.
    sceneTable["registered"] = [&scene, &lua]() -> sol::table {
        auto names = scene.getRegisteredScenes();
        sol::table result = lua.create_table();
        for (std::size_t i = 0; i < names.size(); ++i) {
            result[i + 1] = names[i];
        }
        return result;
    };

    bestow["scene"] = sceneTable;

    // ------------------------------------------------------------------
    // bestow.scene.subscribe(eventType, callback) — scene-scoped events
    //
    // Wraps a closure in a table+method pair so it works with the existing
    // hot-reload-safe event dispatcher, tracks the subscription ID under
    // the active scene name, and auto-unsubscribes when the scene exits.
    //
    // bestow.scene._cleanupSubs(sceneName) is called by SceneSystem C++
    // before/after every exit() callback.
    // ------------------------------------------------------------------
    lua.safe_script(R"(
        local _sceneSubs = {}

        function bestow.scene.subscribe(eventType, callback)
            -- Wrap the closure in a table so it passes the table+method check
            local wrapper = { _fn = callback }
            function wrapper:_handle(data, scope)
                self._fn(data)
            end

            local id = bestow.events.subscribe(eventType, {}, wrapper, "_handle")

            -- Track under the currently active scene
            local active = bestow.scene.active()
            if active then
                if not _sceneSubs[active] then
                    _sceneSubs[active] = {}
                end
                table.insert(_sceneSubs[active], id)
            end

            return id
        end

        function bestow.scene._cleanupSubs(sceneName)
            local subs = _sceneSubs[sceneName]
            if subs then
                for _, id in ipairs(subs) do
                    bestow.events.unsubscribe(id)
                end
                _sceneSubs[sceneName] = nil
            end
        end
    )", sol::script_pass_on_error);
}

} // namespace bestow
