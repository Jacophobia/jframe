// bestow-contract/src/bestow.cppm
// Primary module that re-exports all Bestow interfaces

export module bestow;

export import bestow.types;
export import bestow.entity;
export import bestow.graphics;
export import bestow.graphics3d;
export import bestow.audio;
export import bestow.input;
export import bestow.assets;
export import bestow.save;
export import bestow.level;
export import bestow.events;
export import bestow.physics;
export import bestow.physics3d;
export import bestow.ai;
export import bestow.config;
export import bestow.camera;
export import bestow.gas;
export import bestow.blueprints;
export import bestow.ui;
export import bestow.gamestate;
export import bestow.shader;

export namespace bestow {

// Engine aggregate - provides access to all systems
struct BestowEngine {
    IEventSystem* events = nullptr;
    IAssetSystem* assets = nullptr;
    IEntitySystem* entities = nullptr;
    IGraphicsSystem* graphics = nullptr;
    IGraphics3DSystem* graphics3d = nullptr;
    IAudioSystem* audio = nullptr;
    IInputSystem* input = nullptr;
    IPhysicsSystem* physics = nullptr;
    IPhysics3DSystem* physics3d = nullptr;
    ILevelSystem* levels = nullptr;
    ISaveSystem* save = nullptr;
    IAISystem* ai = nullptr;
    IConfigSystem* config = nullptr;
    ICameraSystem* camera = nullptr;
    IGASSystem* gas = nullptr;
    IBlueprintFactory* blueprints = nullptr;
    IUISystem* ui = nullptr;
    IGameStateSystem* gameStates = nullptr;
    IShaderSystem* shaders = nullptr;

    bool isValid() const {
        return events && assets && entities && graphics &&
               audio && input && physics && levels && save && ai && config;
    }

    bool has3DSupport() const {
        return graphics3d != nullptr && physics3d != nullptr;
    }

    bool hasShaderSupport() const {
        return shaders != nullptr;
    }
};

}  // namespace bestow
