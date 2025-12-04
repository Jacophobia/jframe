# EngineBuilder API

The `EngineBuilder` provides a fluent interface for configuring and building a Bestow engine instance.

## Overview

```cpp
import bestow.core;

auto engineResult = EngineBuilder()
    .withEvents()
    .withEntities()
    .withPhysics()
    .withGraphics({.width = 1280, .height = 720, .title = "Game"})
    .withAudio()
    .withInput()
    .withAssets("assets")
    .withLevel()
    .withBlueprints()
    .build();

if (!engineResult) {
    std::cerr << "Error: " << engineResult.error() << std::endl;
    return 1;
}

auto engine = std::move(*engineResult);
```

## Constructor

```cpp
EngineBuilder();
```

Creates a new builder with no systems enabled.

## System Configuration Methods

All methods return `EngineBuilder&` for method chaining.

### withEvents()

```cpp
EngineBuilder& withEvents();
```

Enable the event system for publish/subscribe messaging.

**No dependencies.**

---

### withEntities()

```cpp
EngineBuilder& withEntities();
```

Enable the entity-component system (EnTT-based).

**No dependencies.**

---

### withPhysics()

```cpp
EngineBuilder& withPhysics();
```

Enable the physics system (Box2D-based).

**No dependencies**, but typically used with entities.

---

### withGraphics(GraphicsConfig config)

```cpp
struct GraphicsConfig {
    int width = 1280;
    int height = 720;
    std::string title = "Bestow Application";
    bool vsync = true;
    Color clearColor = Color{26, 26, 26, 255};  // Dark gray
};

EngineBuilder& withGraphics(GraphicsConfig config);
```

Enable the graphics system with the specified configuration.

**Required for:** Input system (needs window)

**Example:**

```cpp
.withGraphics({
    .width = 1920,
    .height = 1080,
    .title = "My Platformer",
    .vsync = true,
    .clearColor = Color{0, 128, 255, 255}  // Sky blue
})
```

---

### withAudio()

```cpp
EngineBuilder& withAudio();
```

Enable the audio system (FMOD-based).

**Recommends:** Assets system (for loading sound files)

---

### withInput()

```cpp
EngineBuilder& withInput();
```

Enable the input system for keyboard, mouse, and controller input.

**Requires:** Graphics system (for window handle)

---

### withAssets(std::string_view basePath)

```cpp
EngineBuilder& withAssets(std::string_view basePath = "assets");
```

Enable the asset system with the specified base path.

**No dependencies.**

**Example:**

```cpp
.withAssets("data")  // Assets will be loaded from "./data/"
```

---

### withSave(std::string_view savePath)

```cpp
EngineBuilder& withSave(std::string_view savePath = "saves");
```

Enable the save system for binary save files.

**No dependencies.**

---

### withLevel()

```cpp
EngineBuilder& withLevel();
```

Enable the level system for loading Lua-based levels.

**Requires:** Assets system

---

### withAI()

```cpp
EngineBuilder& withAI();
```

Enable the AI system for behavior trees, pathfinding, and line-of-sight.

**Recommends:** Physics system (for line-of-sight) and Assets system (for navmesh)

---

### withCamera(Size viewportSize)

```cpp
EngineBuilder& withCamera(Size viewportSize);
```

Enable the camera system with the specified viewport size.

**Example:**

```cpp
.withCamera({.width = 1280, .height = 720})
```

---

### withGAS()

```cpp
EngineBuilder& withGAS();
```

Enable the Gameplay Ability System for tags, attributes, effects, and abilities.

**No dependencies.**

---

### withBlueprints()

```cpp
EngineBuilder& withBlueprints();
```

Enable the blueprint factory for data-driven entity creation from Lua.

**Requires:** Entities system

**Recommends:** Physics system (for automatic physics body creation)

---

## Building the Engine

### build()

```cpp
std::expected<Engine, std::string> build();
```

Validates configuration, initializes all enabled systems in dependency order, and returns an `Engine` instance.

**Returns:**
- `Engine` on success
- `std::unexpected<std::string>` with error message on failure

**Errors:**
- Missing required dependencies
- System initialization failure
- Invalid configuration

**Example:**

```cpp
auto engineResult = builder.build();

if (!engineResult) {
    logError("Failed to build engine: " + engineResult.error());
    return 1;
}

Engine engine = std::move(*engineResult);
```

---

## Engine Class

Once built, the `Engine` manages all systems and the game loop.

### systems()

```cpp
BestowEngine& systems();
const BestowEngine& systems() const;
```

Access the engine aggregate containing all initialized systems.

**Example:**

```cpp
auto& sys = engine.systems();
Entity player = sys.entities->createEntity();
sys.graphics->setClearColor(Color::blue());
```

---

### run(Application& app)

```cpp
void run(Application& app);
```

Starts the game loop with the provided application.

**Game Loop:**
1. Poll input
2. Process events
3. Fixed timestep updates (physics, AI, etc.)
4. Render with interpolation
5. Present frame

**Exits when:** `shouldClose()` returns true or `quit()` is called.

---

### quit()

```cpp
void quit();
```

Stops the game loop gracefully.

---

### isRunning()

```cpp
bool isRunning() const;
```

Returns `true` if the game loop is currently running.

---

## Application Interface

Your game must implement the `Application` interface:

```cpp
class Application {
public:
    virtual bool initialize(Engine& engine) = 0;
    virtual void updateFixed(DeltaTime dt) = 0;
    virtual void render(float alpha) = 0;
    virtual void shutdown() = 0;
};
```

### initialize(Engine& engine)

Called once before the game loop starts.

**Returns:** `true` on success, `false` to abort startup.

**Use for:**
- Loading initial assets
- Creating initial entities
- Setting up game state

---

### updateFixed(DeltaTime dt)

Called at a fixed timestep (typically 60Hz) for deterministic simulation.

**Use for:**
- Physics updates
- AI updates
- Game logic
- Animation updates

**Example:**

```cpp
void updateFixed(DeltaTime dt) override {
    auto& sys = engine_.systems();

    // Update physics
    sys.physics->update(dt);

    // Update AI
    sys.ai->update(dt);

    // Process events
    sys.events->processQueue();
}
```

---

### render(float alpha)

Called as fast as possible for smooth rendering.

**Parameters:**
- `alpha`: Interpolation factor between previous and current physics state (0.0-1.0)

**Use for:**
- Rendering with interpolation
- UI rendering
- Visual effects

**Example:**

```cpp
void render(float alpha) override {
    auto& sys = engine_.systems();

    sys.graphics->beginFrame();
    sys.graphics->renderEntities(*sys.entities);
    sys.graphics->endFrame();
}
```

---

### shutdown()

Called once after the game loop exits.

**Use for:**
- Cleaning up game-specific resources
- Saving settings

---

## Complete Example

```cpp
import bestow;
import bestow.core;

class PlatformerGame : public bestow::core::Application {
    bestow::core::Engine* engine_ = nullptr;

public:
    bool initialize(bestow::core::Engine& engine) override {
        engine_ = &engine;
        auto& sys = engine.systems();

        // Load level
        AssetHandle levelAsset = sys.assets->registerAsset(
            AssetType::Level, "levels/level1.lua"
        );
        sys.assets->loadAsset(levelAsset);
        sys.levels->loadLevel(levelAsset);

        return true;
    }

    void updateFixed(DeltaTime dt) override {
        auto& sys = engine_->systems();

        sys.physics->update(dt);
        sys.ai->update(dt);
        sys.audio->update(dt);
        sys.events->processQueue();
    }

    void render(float alpha) override {
        auto& sys = engine_->systems();

        sys.graphics->beginFrame();
        sys.graphics->renderEntities(*sys.entities);
        sys.graphics->endFrame();
    }

    void shutdown() override {
        // Cleanup
    }
};

int main() {
    using namespace bestow::core;

    auto engineResult = EngineBuilder()
        .withEvents()
        .withEntities()
        .withPhysics()
        .withGraphics({.width = 1280, .height = 720, .title = "Platformer"})
        .withAudio()
        .withInput()
        .withAssets("assets")
        .withLevel()
        .withBlueprints()
        .build();

    if (!engineResult) {
        logError("Failed to build engine: " + engineResult.error());
        return 1;
    }

    auto engine = std::move(*engineResult);
    PlatformerGame game;
    engine.run(game);

    return 0;
}
```

## BestowEngine Aggregate

The `BestowEngine` struct provides raw pointers to all initialized systems:

```cpp
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

    bool isValid() const;
};
```

Only systems that were enabled via the builder will be non-null.

## Initialization Order

Systems are initialized in dependency order:

1. **Events** (no dependencies)
2. **Assets** (no dependencies)
3. **Graphics** (creates window)
4. **Input** (requires window)
5. **Entities** (no dependencies)
6. **Physics** (no dependencies)
7. **Audio** (no dependencies)
8. **Save** (no dependencies)
9. **Level** (requires assets)
10. **AI** (requires physics, assets)
11. **Camera** (requires viewport size)
12. **GAS** (no dependencies)
13. **Blueprints** (requires entities, physics)

## See Also

- [EntitySystem](EntitySystem.md) - Entity and component management
- [GraphicsSystem](GraphicsSystem.md) - Rendering API
- [PhysicsSystem](PhysicsSystem.md) - Physics simulation
- [Getting Started Guide](../Getting-Started.md)
