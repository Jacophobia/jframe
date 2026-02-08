// bestow-contract/src/bestow.scene.cppm
// Scene system interface — stack-based scene manager

module;

#include <any>
#include <optional>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

export module bestow.scene;

import bestow.types;

export namespace bestow {

class ISceneSystem {
public:
    virtual ~ISceneSystem() = default;

    //======================================================================
    // Lifecycle (called by engine main loop)
    //======================================================================

    virtual void update(DeltaTime dt) = 0;
    virtual void render() = 0;

    //======================================================================
    // Registration — associate a name with a scene asset
    //======================================================================

    /// Register a scene definition (Lua file loaded via AssetSystem)
    virtual Result<SceneId, std::error_code> registerScene(
        const std::string& name, AssetHandle sceneAsset) = 0;

    /// Unregister a scene (must not be on stack)
    virtual Result<void, std::error_code> unregisterScene(
        const std::string& name) = 0;

    //======================================================================
    // Stack Operations — controls what's on screen
    //======================================================================

    /// Push a scene onto the stack (becomes active, previous paused)
    /// params: passed to the scene's enter() callback
    virtual Result<void, std::error_code> pushScene(
        const std::string& name,
        const std::unordered_map<std::string, std::any>& params = {}) = 0;

    /// Pop the active scene (previous scene resumes)
    virtual Result<void, std::error_code> popScene() = 0;

    /// Replace the active scene with a different one
    virtual Result<void, std::error_code> replaceScene(
        const std::string& name,
        const std::unordered_map<std::string, std::any>& params = {}) = 0;

    /// Clear the entire stack
    virtual void clearStack() = 0;

    //======================================================================
    // Queries
    //======================================================================

    virtual std::optional<std::string> getActiveSceneName() const = 0;
    virtual std::vector<std::string> getSceneStack() const = 0;
    virtual SceneState getSceneState(const std::string& name) const = 0;
    virtual std::optional<SceneMetadata> getSceneMetadata(
        const std::string& name) const = 0;
    virtual std::vector<std::string> getRegisteredScenes() const = 0;
};

}  // namespace bestow
