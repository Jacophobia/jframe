# Architecture Guide

This guide provides a comprehensive overview of Bestow's architecture, design patterns, and system interactions.

## Table of Contents

1. [Overview](#overview)
2. [Module-Based Architecture](#module-based-architecture)
3. [System Overview](#system-overview)
4. [Dependency Injection](#dependency-injection)
5. [Data-Driven Design](#data-driven-design)
6. [The Game Loop](#the-game-loop)
7. [Key Design Patterns](#key-design-patterns)
8. [Performance Architecture](#performance-architecture)

---

## Overview

Bestow is built on several core architectural principles:

1. **Program to Interfaces** - All systems implement abstract interfaces for testability and flexibility
2. **Composition Over Inheritance** - Entity Component System (ECS) architecture
3. **Data-Driven Design** - Game content defined in Lua, not C++
4. **Dependency Injection** - Systems are wired together via EngineBuilder
5. **C++23 Modules** - Modern module system for faster compilation and better encapsulation

### Philosophy

- **C++ provides the engine** - Systems, rendering, physics, audio
- **Lua provides the content** - Levels, entities, blueprints, configuration
- **Interfaces define contracts** - Implementation details are hidden
- **Hot reload first** - Change content without recompiling

---

## Module-Based Architecture

Bestow uses C++23 modules to organize code into logical units with clear dependencies.

### Module Hierarchy

```
bestow (primary module - re-exports all)
├── bestow.types          // Core types (Entity, Transform2D, Vec2, etc.)
├── bestow.core           // Core utilities (Timer, Logging, JobSystem)
├── bestow.events         // Event system interface
├── bestow.entity         // Entity/Component system interface
├── bestow.graphics       // Graphics system interface
├── bestow.physics        // Physics system interface
├── bestow.audio          // Audio system interface
├── bestow.input          // Input system interface
├── bestow.assets         // Asset system interface
├── bestow.save           // Save system interface
├── bestow.level          // Level system interface
├── bestow.ai             // AI system interface
├── bestow.camera         // Camera system interface
├── bestow.gas            // Gameplay Ability System interface
├── bestow.config         // Config system interface
└── bestow.dev            // Dev tools (debug builds only)
```

### Module Structure Pattern

All Bestow modules follow this structure:

```cpp
// bestow-{system}/src/bestow.{system}.cppm (Interface)
module;

// Global module fragment - third-party headers only
#include <third_party_header.hpp>

export module bestow.{system};

import std;
import bestow.types;

export namespace bestow {
    // Public interface declarations
    class ISystemName {
    public:
        virtual ~ISystemName() = default;
        virtual void method() = 0;
    };
}
```

```cpp
// bestow-{system}/src/bestow.{system}.impl.cppm (Implementation Interface)
module;

#include <third_party_header.hpp>

export module bestow.{system}.impl;

import std;
import bestow.{system};
import bestow.types;

export namespace bestow {
    // Concrete implementation
    class SystemImpl : public ISystemName {
        // Implementation details
    };

    // Factory function
    std::unique_ptr<ISystemName> createSystem();
}
```

```cpp
// bestow-{system}/src/SystemImpl.cpp (Implementation)
module bestow.{system}.impl;

import std;
import bestow.{system};

namespace bestow {
    // Method implementations
    void SystemImpl::method() {
        // ...
    }

    std::unique_ptr<ISystemName> createSystem() {
        return std::make_unique<SystemImpl>();
    }
}
```

### Usage in Game Code

```cpp
// Minimal import
import std;
import bestow;  // Everything

// Or selective imports
import bestow.entity;
import bestow.graphics;
import bestow.physics;

// Dev tools (debug builds only)
#ifdef BESTOW_DEV_TOOLS
import bestow.dev;
#endif
```

---

## System Overview

Bestow is composed of independent systems that communicate via interfaces and events.

### System Categories

#### Tier 0: Foundation (No Dependencies)

**Events System**
- Publish/subscribe event bus
- Immediate and deferred event dispatch
- Type-safe event data with `std::variant`
- Thread-safe event queue

#### Tier 1: Core Systems (Depend on Events)

**Entity System** (EnTT-based)
- Entity creation/destruction
- Component attachment/removal
- Efficient view iteration
- Tag-based queries

**Input System**
- Keyboard/mouse input (GLFW)
- Game controller support (SDL2)
- Action mapping with modifiers
- Hot-plugging support

**Config System**
- Lua-based configuration loading
- Runtime config changes
- Hot reload support

**Save System**
- Binary serialization (cereal)
- Profile management
- Auto-save scheduling
- Compression (zstd)

#### Tier 2: Rendering and Content (Depend on Tier 0-1)

**Graphics System** (OpenGL)
- Sprite rendering with batching
- Text rendering (MSDF fonts)
- Debug primitives (lines, circles, rectangles)
- Camera/viewport management
- Render layers

**Assets System**
- Asset loading (textures, sounds, fonts, data)
- Async loading with taskflow
- Reference counting and caching
- Hot reload support

**Audio System** (FMOD)
- Music and SFX playback
- 3D positional audio
- Channel management
- Volume groups

#### Tier 3: Simulation (Depend on Tier 0-2)

**Physics System** (Box2D)
- 2D rigid body simulation
- Collision detection/response
- Spatial queries (AABB, raycast, circle)
- Collision filtering by layer/mask
- Contact callbacks

**Level System**
- Lua level file execution
- Entity spawning from blueprints
- Spawn point/region extraction
- Level transitions
- Hot reload

**Camera System**
- Camera follow with deadzone
- Smooth camera movement
- Screen shake effects
- Zoom control
- Multiple camera support

#### Tier 4: Gameplay (Depend on Tier 0-3)

**AI System**
- Behavior trees (BehaviorTree.CPP)
- Blackboard data storage
- Navigation mesh queries (Recast/Detour)
- Pathfinding
- Steering behaviors

**GAS System** (Gameplay Ability System)
- Abilities with cooldowns and costs
- Gameplay effects (buffs, debuffs)
- Attribute system (health, mana, stamina)
- Effect stacking and duration
- Tag-based ability gating

#### Tier 5: Development Tools (Debug Only)

**Dev System**
- ImGui integration
- Entity inspector
- Component editor
- Hot reload manager
- Performance overlay

### System Dependency Graph

```
Events (Tier 0)
  ├── Entity (Tier 1)
  │     ├── Graphics (Tier 2)
  │     ├── Physics (Tier 3)
  │     │     ├── AI (Tier 4)
  │     │     └── Camera (Tier 3)
  │     ├── Audio (Tier 2)
  │     ├── Level (Tier 3)
  │     └── GAS (Tier 4)
  ├── Input (Tier 1)
  ├── Config (Tier 1)
  ├── Save (Tier 1)
  └── Assets (Tier 2)
        ├── Graphics (Tier 2)
        ├── Audio (Tier 2)
        └── Level (Tier 3)
```

**Rules:**
- Lower tier systems never depend on higher tier systems
- Systems in the same tier should not depend on each other
- Cross-tier communication happens via events or interfaces

---

## Dependency Injection

Bestow uses the **EngineBuilder** pattern to wire systems together.

### EngineBuilder Pattern

```cpp
auto engineResult = bestow::core::EngineBuilder()
    .withEvents()                    // Event system
    .withEntities()                  // Entity system (needs Events)
    .withPhysics()                   // Physics (needs Entities, Events)
    .withGraphics(graphicsConfig)    // Graphics (needs Entities)
    .withInput()                     // Input (needs Events)
    .withAssets("data")              // Assets (needs Events)
    .withAudio()                     // Audio (needs Assets)
    .withLevel()                     // Level (needs Entities, Assets, Events)
    .withAI()                        // AI (needs Physics, Entities)
    .withCamera()                    // Camera (needs Entities, Events)
    .withGAS()                       // GAS (needs Entities, Events)
    .build();

if (!engineResult) {
    logError("Engine build failed: " + engineResult.error());
    return 1;
}

Game game;
engineResult.value().run(game);
```

### How It Works

1. **EngineBuilder validates dependencies** - If you call `.withGraphics()` before `.withEntities()`, the build will fail
2. **Factory functions create system instances** - Each system is created via `createSystem()` factory
3. **Systems are injected into Engine** - All systems are owned by the `Engine` object
4. **Game accesses systems via Engine** - `engine.systems().graphics`, `engine.systems().physics`, etc.

### System Access in Game Code

```cpp
class Game {
public:
    bool initialize(bestow::core::Engine& engine) {
        engine_ = &engine;
        auto& sys = engine.systems();

        // Access systems via the systems() method
        player_ = sys.entities->createEntity();
        sys.physics->createBody(player_, bodyDef);
        sys.graphics->loadTexture("player.png");

        return true;
    }

private:
    bestow::core::Engine* engine_;
};
```

---

## Data-Driven Design

Bestow separates **engine code (C++)** from **game content (Lua)**.

### Lua Data Architecture

```
data/
├── traits/                 # Reusable component factories
│   ├── physics_body.lua    # Physics component factory
│   ├── health.lua          # Health component factory
│   └── patrol.lua          # AI patrol factory
│
├── blueprints/             # Entity templates
│   ├── _base.lua           # P.extend() inheritance
│   ├── player.lua          # Player blueprint
│   └── enemies/
│       ├── base.lua        # Common enemy traits
│       └── slime.lua       // Extends enemies/base
│
├── config/                 # Global settings
│   ├── game.lua            # Game-wide settings
│   ├── physics.lua         # Physics constants
│   ├── input.lua           # Key bindings
│   └── abilities.lua       # GAS definitions
│
└── levels/                 # Level definitions
    ├── level1.lua          # First level
    └── level2.lua          # Second level
```

### Traits - Component Factories

Traits are small Lua functions that return component data:

```lua
-- traits/health.lua
return function(maxHealth)
    return {
        current = maxHealth,
        max = maxHealth,
        invulnerable = false
    }
end
```

### Blueprints - Entity Templates

Blueprints compose traits to define complete entities:

```lua
-- blueprints/player.lua
local physics = require("traits/physics_body")
local health = require("traits/health")

return {
    physics = physics({ type = "dynamic", width = 32, height = 48 }),
    health = health(100),
    controller = { speed = 200, jumpForce = 400 }
}
```

### Prototype Inheritance

Use `P.extend()` for blueprint variants:

```lua
-- blueprints/enemies/slime.lua
return P.extend("blueprints/enemies/base", {
    health = { current = 30, max = 30 },  // Override
    patrol = patrol(100, 30)              // Add new component
})
```

### Levels - Entity Spawning

Levels reference blueprints by name:

```lua
-- levels/level1.lua
return {
    name = "The Beginning",

    entities = {
        { blueprint = "player", x = 100, y = 200 },
        { blueprint = "enemies/slime", x = 400, y = 200 },
        { blueprint = "items/coin", x = 300, y = 150 }
    }
}
```

### C++ Integration

C++ reads Lua and creates entities:

```cpp
void LevelSystem::loadLevel(const std::string& levelPath) {
    sol::state lua;
    sandboxLua(lua);

    auto result = lua.script_file(levelPath);
    sol::table level = result;

    sol::table entities = level["entities"];
    for (size_t i = 1; i <= entities.size(); ++i) {
        sol::table entityDef = entities[i];
        std::string blueprint = entityDef["blueprint"];
        float x = entityDef["x"];
        float y = entityDef["y"];

        // Spawn entity from blueprint
        spawnFromBlueprint(blueprint, x, y);
    }
}
```

For a complete guide, see [Data-Driven Design Guide](Data-Driven-Design.md).

---

## The Game Loop

Bestow uses a **fixed-timestep game loop** for physics and logic, with **variable-timestep rendering** for smooth visuals.

### Fixed Timestep Pattern

```
Time ────────────────────────────────────►
     ├─────┼─────┼─────┼─────┼─────┼─────  Fixed updates (60 Hz)
     │     │     │     │     │     │
     ▼     ▼     ▼     ▼     ▼     ▼
     U     U     U     U     U     U       updateFixed(dt)
     ├───R─┼──R──┼─────R──┼────R──┼       render(alpha)
```

### Game Interface

Your game implements this interface:

```cpp
class Game {
public:
    // Called once at startup
    bool initialize(bestow::core::Engine& engine);

    // Called at fixed intervals (e.g., 60 FPS)
    // Use for physics, game logic, AI
    void updateFixed(bestow::DeltaTime dt);

    // Called every frame with interpolation alpha [0, 1]
    // Use for rendering only
    void render(float alpha);

    // Called once at shutdown
    void shutdown();
};
```

### Engine Run Loop

```cpp
void Engine::run(Game& game) {
    if (!game.initialize(*this)) {
        return;
    }

    constexpr DeltaTime fixedDt = 1.0f / 60.0f;  // 16.67ms
    DeltaTime accumulator = 0.0f;

    while (shouldRun_) {
        DeltaTime frameDt = frameTimer_.tick();
        accumulator += frameDt;

        // Fixed timestep updates
        while (accumulator >= fixedDt) {
            game.updateFixed(fixedDt);
            systems_.physics->step(fixedDt);  // Physics always at 60 Hz
            accumulator -= fixedDt;
        }

        // Variable timestep rendering
        float alpha = accumulator / fixedDt;  // Interpolation factor
        game.render(alpha);
        systems_.graphics->present();

        // Process deferred events
        systems_.events->processQueue();
    }

    game.shutdown();
}
```

### Why Fixed Timestep?

- **Deterministic physics** - Same input produces same output
- **Stable simulation** - No jitter or instability
- **Replay support** - Record and playback inputs
- **Network sync** - Easier to synchronize multiplayer

### Interpolation

Use the `alpha` parameter in `render()` to smooth rendering:

```cpp
void Game::render(float alpha) {
    auto& transform = entities->get<Transform2D>(player);
    auto& velocity = physics->getVelocity(player);

    // Interpolate position for smooth rendering
    Vec2 renderPos = transform.position + velocity * alpha;

    graphics->drawSprite(playerSprite, renderPos);
}
```

---

## Key Design Patterns

### 1. Interface-Based Design

All systems implement abstract interfaces:

```cpp
// Interface in bestow-contract
export class IEntitySystem {
public:
    virtual ~IEntitySystem() = default;
    virtual Entity createEntity() = 0;
    virtual void destroyEntity(Entity e) = 0;
    // ...
};

// Implementation in bestow-entity
class EntitySystemImpl : public IEntitySystem {
    Entity createEntity() override {
        return registry_.create();
    }
    // ...
private:
    entt::registry registry_;
};
```

**Benefits:**
- Testability (mock implementations)
- Flexibility (swap implementations)
- Decoupling (depend on interfaces, not implementations)

### 2. Entity Component System (ECS)

Bestow uses EnTT for ECS:

```cpp
// Create entity
Entity player = entities->createEntity();

// Add components
entities->emplace<Transform2D>(player, Transform2D{100, 200});
entities->emplace<Health>(player, 100, 100);
entities->emplace<PlayerTag>(player);

// Query entities
auto view = entities->view<Transform2D, Health>();
for (auto [entity, transform, health] : view.each()) {
    // Process entities with Transform2D AND Health
}
```

**Benefits:**
- Cache-friendly data layout
- Flexible composition
- Fast iteration
- No inheritance hierarchies

### 3. Publish/Subscribe Events

Systems communicate via events:

```cpp
// Subscribe to collision events
auto id = events->subscribe(Events::Collision, [](const EventData& data) {
    auto& collision = std::get<CollisionEvent>(data);
    logInfo("Collision: {} hit {}", collision.entityA, collision.entityB);
});

// Publish event (immediate)
events->publish(Events::PlayerDeath, PlayerDeathEvent{player});

// Enqueue event (deferred, processed at end of frame)
events->enqueue(Events::LevelComplete, LevelCompleteEvent{levelId});
```

**Benefits:**
- Decoupling (systems don't need direct references)
- Flexibility (add/remove listeners at runtime)
- Type safety (compile-time event type checking)

### 4. Factory Pattern

Systems are created via factory functions:

```cpp
export std::unique_ptr<IEntitySystem> createEntitySystem();
export std::unique_ptr<IPhysicsSystem> createPhysicsSystem(IEntitySystem* entities);
export std::unique_ptr<IGraphicsSystem> createGraphicsSystem(const GraphicsConfig& config);
```

**Benefits:**
- Hide implementation details
- Control object creation
- Enable dependency injection

### 5. Result Type (std::expected)

Bestow uses `std::expected` for error handling without exceptions:

```cpp
template<typename T, typename E = std::error_code>
using Result = std::expected<T, E>;

Result<Entity, EntityError> createPlayerEntity() {
    if (tooManyEntities()) {
        return std::unexpected(EntityError::TooManyEntities);
    }
    return entities->createEntity();
}

// Usage
auto result = createPlayerEntity();
if (result) {
    Entity player = result.value();
} else {
    logError("Failed: {}", result.error());
}
```

**Benefits:**
- No exception overhead
- Explicit error handling
- Type-safe error codes
- Composable with monadic operations

---

## Performance Architecture

### Graphics Performance

**Sprite Batching**
- Sort sprites by texture and layer
- Batch identical materials
- Minimize state changes
- Instanced rendering for repeated sprites

```cpp
// Automatic batching in render loop
void GraphicsSystem::render() {
    std::vector<DrawCall> calls = collectDrawCalls();
    std::sort(calls.begin(), calls.end(), [](auto& a, auto& b) {
        if (a.layer != b.layer) return a.layer < b.layer;
        return a.texture < b.texture;  // Batch by texture
    });

    for (auto& call : calls) {
        submitBatch(call);
    }
}
```

**Render Layers**
- Background (layer 0)
- Terrain (layer 10)
- Entities (layer 20)
- Effects (layer 30)
- UI (layer 100)

### Physics Performance

**Fixed Timestep**
- Always run physics at 60 Hz
- Deterministic and stable

**Collision Filtering**
- Layer-based filtering (bitmask)
- Skip unnecessary collision checks

```cpp
PhysicsBodyDef def{
    .layer = PhysicsLayer::Player,
    .mask = PhysicsLayer::Terrain | PhysicsLayer::Enemy
    // Only collide with terrain and enemies
};
```

**Spatial Partitioning**
- Box2D's broadphase for spatial queries
- Avoid checking all entities

### Audio Performance

**Sound Caching**
- Load once, play many times
- Reference counting for unload

**Channel Pooling**
- Reuse FMOD channels
- Limit maximum concurrent sounds

**3D Audio Culling**
- Skip positional updates for distant sounds
- Reduce CPU overhead

### Asset Performance

**Async Loading**
- Load assets on background threads (taskflow)
- Avoid blocking the main thread

```cpp
assets->loadAssetAsync(texture, [](AssetHandle h, AssetState state) {
    if (state == AssetState::Loaded) {
        // Ready to use
    }
});
```

**Reference Counting**
- Track asset usage
- Unload unused assets automatically

**Hot Reload Debouncing**
- Delay reload if file changes rapidly
- Avoid reloading on every save

### ECS Performance

**Archetype Optimization**
- EnTT stores components in packed arrays
- Cache-friendly memory layout

**View Caching**
- Cache frequently used views
- Avoid recreating views every frame

```cpp
// BAD - creates view every frame
void update() {
    for (auto [e, t, h] : entities->view<Transform2D, Health>().each()) {
        // ...
    }
}

// GOOD - cache view
class System {
    entt::view<Transform2D, Health> view_;

    System(IEntitySystem* entities)
        : view_(entities->view<Transform2D, Health>()) {}

    void update() {
        for (auto [e, t, h] : view_.each()) {
            // ...
        }
    }
};
```

**Avoid Random Access**
- Iterate views instead of per-entity queries
- Use contiguous iteration for cache efficiency

```cpp
// BAD - random access
for (Entity e : allEntities) {
    auto* transform = entities->tryGet<Transform2D>(e);
    if (transform) {
        // ...
    }
}

// GOOD - view iteration
for (auto [e, transform] : entities->view<Transform2D>().each()) {
    // ...
}
```

---

## Summary

Bestow's architecture is built on:

1. **C++23 Modules** - Clean dependency graph, fast compilation
2. **Interface-Based Design** - Testable, flexible, decoupled
3. **Entity Component System** - Cache-friendly, composable, fast
4. **Data-Driven Design** - Lua for content, C++ for engine
5. **Dependency Injection** - EngineBuilder wires systems together
6. **Fixed Timestep Game Loop** - Deterministic physics, smooth rendering
7. **Publish/Subscribe Events** - Decoupled communication
8. **Performance First** - Batching, caching, spatial partitioning

### Next Steps

- Read [Getting Started Guide](Getting-Started.md) to build your first game
- Study [Data-Driven Design Guide](Data-Driven-Design.md) for Lua architecture
- Explore [Technical Design](bestow-technical-design.md) for deep dive
- Check out example projects in `examples/`

### Further Reading

- **Entity System**: `docs/systems/Entity-System.md`
- **Events System**: `docs/systems/Events-System.md`
- **Graphics System**: `docs/systems/Graphics-System.md`
- **Physics System**: `docs/systems/Physics-System.md`
- **Gameplay Ability System**: `docs/systems/GAS-System.md`
