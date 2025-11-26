// jframe-contract/src/jframe.cppm
// Primary module that re-exports all JFrame interfaces

export module jframe;

export import jframe.types;
export import jframe.entity;
export import jframe.graphics;
export import jframe.audio;
export import jframe.input;
export import jframe.assets;
export import jframe.save;
export import jframe.level;
export import jframe.events;
export import jframe.physics;
export import jframe.ai;

export namespace jframe {

// Engine aggregate - provides access to all systems
struct JFrameEngine {
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

    bool isValid() const {
        return events && assets && entities && graphics &&
               audio && input && physics && levels && save && ai;
    }
};

}  // namespace jframe
