// bestow-contract/src/bestow.cppm
// Primary module that re-exports all Bestow interfaces

export module bestow;

export import bestow.types;
export import bestow.entity;
export import bestow.graphics;
export import bestow.audio;
export import bestow.input;
export import bestow.assets;
export import bestow.save;
export import bestow.level;
export import bestow.events;
export import bestow.physics;
export import bestow.ai;
export import bestow.config;
export import bestow.camera;
export import bestow.gas;
export import bestow.blueprints;

export namespace bestow {

// Engine aggregate - provides access to all systems
struct BestowEngine {
    IEventSystem* events = nullptr;
    IAssetSystem* assets = nullptr;
    IEntitySystem* entities = nullptr;
    IGraphicsSystem* graphics = nullptr;
    IAudioSystem* audio = nullptr;
    IInputSystem* input = nullptr;
    IPhysicsSystem* physics = nullptr;
    ILevelSystem* levels = nullptr;
    ISaveSystem* save = nullptr;
    IAISystem* ai = nullptr;
    IConfigSystem* config = nullptr;
    ICameraSystem* camera = nullptr;
    IGASSystem* gas = nullptr;
    IBlueprintFactory* blueprints = nullptr;

    bool isValid() const {
        return events && assets && entities && graphics &&
               audio && input && physics && levels && save && ai && config;
    }
};

}  // namespace bestow
