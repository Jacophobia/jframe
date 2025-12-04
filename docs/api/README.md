# Bestow API Reference

Complete API documentation for Bestow's core systems.

## Core Systems

### Engine and Application

- [EngineBuilder](EngineBuilder.md) - Fluent API for configuring and building the engine
- Application - Base class for creating Bestow applications (see EngineBuilder.md)

### Entity-Component System

- [EntitySystem](EntitySystem.md) - Entity creation and component management with EnTT
- [BlueprintFactory](BlueprintFactory.md) - Data-driven entity creation from Lua blueprints

### Simulation

- [PhysicsSystem](PhysicsSystem.md) - Box2D-based 2D physics simulation
- [GASSystem](GASSystem.md) - Gameplay Ability System for tags, attributes, effects, and abilities

### Rendering

- [GraphicsSystem](GraphicsSystem.md) - OpenGL-based 2D rendering with sprites, primitives, and text
- [CameraSystem](CameraSystem.md) - Camera control with following, smoothing, and effects

### Input and Audio

- [InputSystem](InputSystem.md) - Action-based input mapping for keyboard, mouse, and controllers
- [AudioSystem](AudioSystem.md) - FMOD-based audio with channels and 3D positional sound

### Data Management

- [AssetSystem](AssetSystem.md) - Asset loading and hot-reload for textures, fonts, sounds, and more
- [LevelSystem](LevelSystem.md) - Lua-based level loading and management
- [SaveSystem](SaveSystem.md) - Binary save/load with versioning and compression
- [EventSystem](EventSystem.md) - Event publishing and subscription

## Quick Start

```cpp
import bestow;
import bestow.core;

class MyGame : public bestow::core::Application {
public:
    bool initialize(bestow::core::Engine& engine) override {
        auto& sys = engine.systems();

        // Create entities
        bestow::Entity player = sys.entities->createEntity();
        sys.entities->emplace<bestow::Transform2D>(player, 100.0f, 200.0f);

        return true;
    }

    void updateFixed(bestow::DeltaTime dt) override {
        // Game logic
    }

    void render(float alpha) override {
        // Rendering (interpolation)
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
        .withGraphics({.width = 1280, .height = 720, .title = "My Game"})
        .withAudio()
        .withInput()
        .withAssets("assets")
        .withLevel()
        .withBlueprints()
        .build();

    if (!engineResult) {
        return 1;
    }

    auto engine = std::move(*engineResult);
    MyGame game;
    engine.run(game);

    return 0;
}
```

## Core Types

All systems use common types from `bestow.types`:

```cpp
using DeltaTime = float;
using Entity = entt::entity;
using Vec2 = glm::vec2;
using Vec3 = glm::vec3;

template<typename T, typename E = std::error_code>
using Result = std::expected<T, E>;

struct Transform2D {
    float x, y;
    float rotation;
    float scaleX, scaleY;
};

struct Color {
    uint8_t r, g, b, a;
    static constexpr Color white();
    static constexpr Color black();
    static constexpr Color red();
    static constexpr Color green();
    static constexpr Color blue();
};

struct AssetHandle {
    UUID uuid;
    AssetType type;
    bool isValid() const;
};
```

## Common Patterns

### Creating Entities with Components

```cpp
auto& entities = sys.entities;

Entity enemy = entities->createEntity();
entities->emplace<Transform2D>(enemy, 300.0f, 400.0f);
entities->emplace<DebugRect>(enemy, Vec2{32.0f, 32.0f}, Color::red());
```

### Querying Entities

```cpp
// View pattern (fastest, for iteration)
for (auto [entity, transform, health] : entities->view<Transform2D, Health>().each()) {
    // Update health based on transform
}

// Collection pattern (for modification during iteration)
auto enemies = entities->collect<Enemy, Transform2D>();
for (Entity e : enemies) {
    entities->destroyEntity(e);  // Safe to destroy during iteration
}
```

### Loading Assets

```cpp
auto& assets = sys.assets;

AssetHandle texture = assets->registerAsset(AssetType::Texture, "textures/player.png");
assets->loadAssetAsync(texture, [](AssetHandle h, AssetState state) {
    if (state == AssetState::Loaded) {
        // Texture ready
    }
});
```

### Physics Bodies

```cpp
PhysicsBodyDef def{
    .type = BodyType::Dynamic,
    .transform = {.x = 100.0f, .y = 200.0f},
    .size = {32.0f, 32.0f},
    .fixedRotation = true
};
sys.physics->createBody(playerEntity, def);
```

### Event Handling

```cpp
auto id = sys.events->subscribe(Events::Collision, [](const EventData& data) {
    auto& collision = std::get<CollisionEvent>(data);
    // Handle collision
});

// Later: unsubscribe
sys.events->unsubscribe(id);
```

## Error Handling

Bestow uses `std::expected` for error handling:

```cpp
Result<LevelId, std::error_code> loadLevel(AssetHandle levelAsset);

auto result = sys.levels->loadLevel(levelAsset);
if (result) {
    LevelId id = *result;
    // Success
} else {
    std::error_code err = result.error();
    // Handle error
}
```

## Module Organization

```cpp
import bestow;              // Import all systems
import bestow.core;         // Core utilities (Timer, FrameTimer, etc.)
import bestow.types;        // Common types
import bestow.entity;       // Entity system interface
import bestow.graphics;     // Graphics system interface
// ... etc
```

## Build Requirements

- **C++23** (modules, std::expected, ranges)
- **LLVM Clang 20+** on macOS (Apple Clang does not support `import std;`)
- **CMake 3.28+**
- See `CLAUDE.md` for full compiler requirements

## See Also

- [Getting Started Guide](../Getting-Started.md)
- [Data-Driven Design](../Data-Driven-Design.md)
- [Technical Design Document](../bestow-technical-design.md)
- [CLAUDE.md](../../CLAUDE.md) - Development guidelines
