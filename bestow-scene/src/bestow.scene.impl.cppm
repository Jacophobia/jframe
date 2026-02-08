// bestow-scene/src/bestow.scene.impl.cppm
// Scene system implementation — stack-based scene manager with Lua integration

module;

#include <bestow/sol2_compat.hpp>
#include <spdlog/spdlog.h>

export module bestow.scene.impl;

import std;
import bestow.services;

export namespace bestow {

class SceneSystem : public ISceneSystem {
public:
    SceneSystem() = default;
    ~SceneSystem() override;

    /// Initialize with dependencies (called after DI construction)
    void initialize(IAssetSystem* assets, IInputSystem* input,
                    IUISystem* ui, IEventSystem* events,
                    sol::state* lua);

    //==================================================================
    // Lifecycle
    //==================================================================

    void update(DeltaTime dt) override;
    void render() override;

    //==================================================================
    // Registration
    //==================================================================

    Result<SceneId, std::error_code> registerScene(
        const std::string& name, AssetHandle sceneAsset) override;
    Result<void, std::error_code> unregisterScene(
        const std::string& name) override;

    //==================================================================
    // Stack Operations
    //==================================================================

    Result<void, std::error_code> pushScene(
        const std::string& name,
        const std::unordered_map<std::string, std::any>& params = {}) override;
    Result<void, std::error_code> popScene() override;
    Result<void, std::error_code> replaceScene(
        const std::string& name,
        const std::unordered_map<std::string, std::any>& params = {}) override;
    void clearStack() override;

    //==================================================================
    // Queries
    //==================================================================

    std::optional<std::string> getActiveSceneName() const override;
    std::vector<std::string> getSceneStack() const override;
    SceneState getSceneState(const std::string& name) const override;
    std::optional<SceneMetadata> getSceneMetadata(
        const std::string& name) const override;
    std::vector<std::string> getRegisteredScenes() const override;

private:
    /// Parsed scene definition from Lua
    struct SceneDef {
        sol::table table;           // The Lua table returned by the scene file
        std::string phase;          // Input phase name
        std::vector<std::string> uiPaths;  // UI document paths
    };

    /// A registered scene with its metadata and parsed definition
    struct RegisteredScene {
        SceneMetadata metadata;
        SceneDef def;
    };

    /// An entry on the scene stack
    struct StackEntry {
        std::string name;
        std::vector<UIDocumentHandle> uiDocs;  // Loaded UI document handles
    };

    //==================================================================
    // Internal helpers
    //==================================================================

    /// Parse a Lua scene file and extract the definition
    Result<SceneDef, std::error_code> parseSceneLua(const std::string& luaCode);

    /// Convert params map to a sol::table for Lua callbacks
    sol::table paramsToLuaTable(
        const std::unordered_map<std::string, std::any>& params);

    /// Call a Lua callback safely (no-throw)
    void callLuaCallback(const sol::table& sceneDef, const char* name,
                         sol::table params = sol::lua_nil);

    /// Call a Lua callback that returns a value safely
    template<typename T>
    std::optional<T> callLuaCallbackWithReturn(const sol::table& sceneDef,
                                                const char* name);

    /// Load and show UI documents for a scene
    void loadSceneUI(StackEntry& entry, const SceneDef& def);

    /// Hide and unload UI documents for a scene
    void unloadSceneUI(StackEntry& entry);

    /// Show UI documents for a scene (when resuming)
    void showSceneUI(const StackEntry& entry);

    /// Hide UI documents for a scene (when pausing)
    void hideSceneUI(const StackEntry& entry);

    /// Publish a scene event
    void publishEvent(const char* eventType, const std::string& sceneName,
                      SceneId id, SceneState state);

    /// Handle hot reload for a scene asset
    void onAssetChanged(AssetHandle handle, AssetType type);

    /// Generate a unique scene ID
    SceneId generateId();

    //==================================================================
    // Dependencies
    //==================================================================

    IAssetSystem* assets_ = nullptr;
    IInputSystem* input_ = nullptr;
    IUISystem* ui_ = nullptr;
    IEventSystem* events_ = nullptr;
    sol::state* lua_ = nullptr;

    //==================================================================
    // State
    //==================================================================

    std::unordered_map<std::string, RegisteredScene> registry_;
    std::vector<StackEntry> stack_;
    SubscriptionId assetSub_ = 0;
    SceneId nextId_ = 1;
};

}  // namespace bestow
