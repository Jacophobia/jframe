# Game Development with Bestow Engine

> **This is your primary guide for building games with Bestow.** It covers architecture, best practices, and practical patterns for creating polished games.

> **IMPORTANT: Always consult the [Decision Runbook](engine-docs/DECISION-RUNBOOK.md) before implementing any feature.** The runbook contains decision trees for common scenarios - where code belongs, how to store data, how systems communicate, etc.

---

## Quick Start

```cpp
// src/main.cpp - Minimal game setup
import std;
import bestow;
import bestow.core;

#include "Game.h"

int main() {
    // Build the engine with the systems you need
    auto engineResult = bestow::core::EngineBuilder()
        .withEvents()
        .withEntities()
        .withPhysics()
        .withGraphics({
            .width = 1280,
            .height = 720,
            .title = "My Game",
            .vsync = true
        })
        .withInput()
        .withAssets("data")
        .withAudio()
        .build();

    if (!engineResult) {
        std::cerr << "Engine build failed: " << engineResult.error() << std::endl;
        return 1;
    }

    // Run your game
    MyGame game;
    engineResult->run(game);

    return 0;
}
```

```cpp
// src/Game.h - Implement the Application interface
#pragma once

import bestow;
import bestow.core;

class MyGame : public bestow::core::Application {
public:
    bool initialize(bestow::core::Engine& engine) override;
    void updateFixed(bestow::DeltaTime dt) override;
    void render(float alpha) override;
    void shutdown() override;

private:
    bestow::core::Engine* engine_ = nullptr;
    bestow::Entity player_;
};
```

---

## Core Philosophy

### 1. Lua-First Design

**Spend your time in Lua, not C++.** The engine is designed so game developers write:

| In Lua | In C++ |
|--------|--------|
| Entity blueprints | Custom systems |
| Level definitions | Performance-critical code |
| Game configuration | Engine extensions |
| Materials and shaders | One-time setup |
| Ability definitions | |

```lua
-- data/blueprints/player.lua
return {
    extends = "character",  -- Inheritance!

    transform = { x = 100, y = 200 },

    sprite = {
        texture = "textures/player.png",
        width = 32, height = 48
    },

    physics = {
        type = "dynamic",
        fixedRotation = true,
        shape = { type = "box", width = 28, height = 44 }
    },

    -- Custom properties
    health = 100,
    speed = 200,
    jumpForce = 400
}
```

### 2. Dvorak-Friendly Controls

**Default movement keys are `,AOE` (Dvorak WASD equivalent).**

```lua
-- data/config/input.lua
return {
    movement = {
        up = ",",      -- Dvorak W
        left = "A",    -- Dvorak A
        down = "O",    -- Dvorak S
        right = "E"    -- Dvorak D
    },

    actions = {
        jump = "Space",
        attack = "Return"
    }
}
```

### 3. Vulkan-First Rendering

**Vulkan is the primary renderer.** OpenGL exists for rapid prototyping and fallback. Always test with Vulkan before shipping.

### 4. Contract-Based Architecture

**All systems are accessed through interfaces (contracts).** Your game never depends on specific implementations - only on the interface contracts defined in `bestow-contract`.

```cpp
// ✅ Correct - Depend on interfaces
IGraphicsSystem* graphics_;
IEntitySystem* entities_;
IPhysicsSystem* physics_;

// ❌ Wrong - Never depend on implementations
VulkanGraphicsSystem* graphics_;
EntityManager* entities_;
```

---

## Project Structure

```
games/game1/
├── CLAUDE.md              # This file - your primary guide
├── CMakeLists.txt         # Build configuration
├── src/
│   ├── main.cpp           # Entry point with EngineBuilder
│   ├── Game.h             # Application interface implementation
│   ├── Game.cpp           # Game logic
│   └── systems/           # Custom game-specific systems
├── data/
│   ├── blueprints/        # Entity templates (Lua)
│   │   ├── player.lua
│   │   ├── enemies/
│   │   └── items/
│   ├── levels/            # Level definitions (Lua)
│   │   ├── level1.lua
│   │   └── level2.lua
│   ├── config/            # Game configuration (Lua)
│   │   ├── game.lua
│   │   ├── input.lua
│   │   └── audio.lua
│   ├── textures/          # Images (.png)
│   ├── audio/
│   │   ├── sfx/           # Sound effects
│   │   └── music/         # Background music
│   ├── shaders/           # Custom shaders (.vert, .frag)
│   └── materials/         # Material definitions (Lua)
└── engine-docs/           # System-specific guides
```

---

## The Application Interface

Your game implements the `Application` interface to hook into the engine's game loop.

```cpp
class Application {
public:
    virtual ~Application() = default;
    virtual bool initialize(Engine& engine) = 0;
    virtual void updateFixed(DeltaTime dt) = 0;
    virtual void render(float alpha) = 0;
    virtual void shutdown() = 0;
};
```

### initialize(Engine& engine)

Called once before the game loop starts.

**Returns:** `true` on success, `false` to abort startup.

```cpp
bool MyGame::initialize(bestow::core::Engine& engine) {
    engine_ = &engine;
    auto& sys = engine.systems();

    // Load initial assets
    playerTexture_ = sys.assets->registerAsset(
        AssetType::Texture, "textures/player.png"
    );
    sys.assets->loadAsset(playerTexture_);

    // Create player entity
    player_ = sys.entities->createEntity();
    sys.physics->createBody(player_, {
        .type = BodyType::Dynamic,
        .transform = {.x = 100, .y = 200},
        .size = {32, 48}
    });

    // Load first level
    sys.levels->loadLevel("levels/level1.lua");

    return true;
}
```

### updateFixed(DeltaTime dt)

Called at a fixed timestep (typically 60Hz) for deterministic simulation.

**Use for:** Physics, AI, game logic, animations

```cpp
void MyGame::updateFixed(DeltaTime dt) {
    auto& sys = engine_->systems();

    // Handle input
    handleInput(sys);

    // Update systems
    sys.physics->update(dt);
    sys.ai->update(dt);
    sys.audio->update(dt);

    // Process events
    sys.events->processQueue();
}
```

### render(float alpha)

Called as fast as possible for smooth rendering.

**Parameters:**
- `alpha`: Interpolation factor between previous and current physics state (0.0-1.0)

```cpp
void MyGame::render(float alpha) {
    auto& sys = engine_->systems();

    sys.graphics->beginFrame();

    // Render game entities
    sys.graphics->renderEntities(*sys.entities);

    // Render UI on top
    ui_->render();

    sys.graphics->endFrame();
}
```

### shutdown()

Called once after the game loop exits.

```cpp
void MyGame::shutdown() {
    // Save game state
    saveProgress();

    // Cleanup resources
    ui_.reset();
}
```

---

## Available Systems

Access all systems through `engine.systems()`:

```cpp
auto& sys = engine.systems();

// All 19 system interfaces:
sys.events      // IEventSystem - Pub/sub messaging
sys.assets      // IAssetSystem - File loading and hot reload
sys.entities    // IEntitySystem - ECS with EnTT
sys.graphics    // IGraphicsSystem - 2D rendering
sys.graphics3d  // IGraphics3DSystem - 3D rendering
sys.audio       // IAudioSystem - FMOD audio
sys.input       // IInputSystem - Action mapping
sys.physics     // IPhysicsSystem - 2D Box2D
sys.physics3d   // IPhysics3DSystem - 3D Jolt
sys.levels      // ILevelSystem - Lua level loading
sys.save        // ISaveSystem - Binary saves
sys.config      // IConfigSystem - Lua config parsing
sys.camera      // ICameraSystem - Camera control
sys.shader      // IShaderSystem - Shader management
sys.ai          // IAISystem - Behavior trees, pathfinding
sys.ui          // IUISystem - RmlUi interface
sys.gas         // IGASSystem - Gameplay Ability System
sys.gamestate   // IGameStateSystem - State machine
sys.blueprints  // IBlueprintFactory - Entity templates
```

**Note:** Only systems you enabled with `EngineBuilder` will be non-null.

---

## EngineBuilder Configuration

The `EngineBuilder` provides a fluent interface to configure your engine:

```cpp
auto engineResult = EngineBuilder()
    // Core systems (no dependencies)
    .withEvents()
    .withEntities()
    .withAssets("data")  // Base path for assets

    // Graphics and input (input requires graphics)
    .withGraphics({
        .width = 1280,
        .height = 720,
        .title = "My Game",
        .vsync = true,
        .clearColor = Color{26, 26, 26, 255}
    })
    .withInput()
    .withCamera({.width = 1280, .height = 720})

    // Physics
    .withPhysics()      // 2D Box2D
    // .withPhysics3D() // 3D Jolt (alternative)

    // Audio
    .withAudio()

    // High-level systems (require other systems)
    .withLevel()        // Requires assets
    .withBlueprints()   // Requires entities
    .withAI()           // Requires physics (for line-of-sight)
    .withGAS()          // Gameplay Ability System
    .withSave("saves")  // Binary save files

    // Build and validate
    .build();

if (!engineResult) {
    std::cerr << "Build failed: " << engineResult.error() << std::endl;
    return 1;
}

auto engine = std::move(*engineResult);
```

### System Dependencies

The builder enforces dependency order:

| System | Requires | Recommends |
|--------|----------|------------|
| Events | None | |
| Assets | None | |
| Entities | None | |
| Graphics | None | |
| Input | Graphics (window) | |
| Physics | None | |
| Audio | None | Assets |
| Level | Assets | |
| Blueprints | Entities | Physics |
| AI | None | Physics, Assets |
| Camera | None | |
| GAS | None | |

---

## Entity Component System (ECS)

Bestow uses EnTT for its ECS. **Think in components, not objects.**

### Components Are Data

```cpp
// Pure data structures
struct Transform2D {
    float x = 0, y = 0;
    float rotation = 0;
    float scaleX = 1, scaleY = 1;
};

struct Velocity {
    float x = 0, y = 0;
};

struct Health {
    int current;
    int maximum;
};

struct PlayerTag {};  // Empty tag component
```

### Creating Entities

```cpp
// From code
Entity player = sys.entities->createEntity();
sys.entities->emplace<Transform2D>(player, 100.0f, 200.0f);
sys.entities->emplace<Velocity>(player);
sys.entities->emplace<Health>(player, 100, 100);

// From blueprint (preferred - Lua-driven)
Entity player = sys.blueprints->createEntity("player");
```

### Iterating Entities

```cpp
void MovementSystem::update(float dt) {
    auto& sys = engine_->systems();

    // Get all entities with Transform2D AND Velocity
    auto view = sys.entities->view<Transform2D, Velocity>();

    for (auto entity : view) {
        auto& transform = view.get<Transform2D>(entity);
        auto& velocity = view.get<Velocity>(entity);

        transform.x += velocity.x * dt;
        transform.y += velocity.y * dt;
    }
}
```

### Best Practices

1. **Small, focused components** - One responsibility per component
2. **Prefer composition** - Combine simple components over complex ones
3. **Use tags** - Empty structs for categorization (PlayerTag, EnemyTag)
4. **Avoid deep hierarchies** - ECS is flat by design

---

## Input Handling

### Polling Input State

```cpp
void handleInput() {
    auto& sys = engine_->systems();

    // Keyboard - Use ,AOE for Dvorak movement
    if (sys.input->isKeyHeld(Key::Comma)) {  // Up (Dvorak W)
        moveUp();
    }
    if (sys.input->isKeyHeld(Key::A)) {      // Left
        moveLeft();
    }
    if (sys.input->isKeyHeld(Key::O)) {      // Down (Dvorak S)
        moveDown();
    }
    if (sys.input->isKeyHeld(Key::E)) {      // Right (Dvorak D)
        moveRight();
    }

    // Single press detection
    if (sys.input->isKeyJustPressed(Key::Space)) {
        jump();
    }

    // Mouse
    auto [mx, my] = sys.input->getMousePosition();
    if (sys.input->isMouseButtonJustPressed(MouseButton::Left)) {
        shoot(mx, my);
    }
}
```

### Action Mapping (Recommended)

```cpp
// Define actions once in initialize()
sys.input->registerMapping({
    .binding = {
        .deviceType = InputDeviceType::Keyboard,
        .keyCode = 44,  // Comma key (Dvorak W)
        .scale = 1.0f
    },
    .action = "move_up"
});

sys.input->registerMapping({
    .binding = {
        .deviceType = InputDeviceType::Gamepad,
        .buttonCode = 0,  // A button
    },
    .action = "jump"
});

// Use actions everywhere
if (sys.input->wasActionJustPressed("jump")) {
    jump();
}

float moveX = sys.input->getActionValue("move_right") -
              sys.input->getActionValue("move_left");
```

### Gamepad Support

```cpp
// Check for connected controllers
if (sys.input->hasGamepad(0)) {
    // Analog stick with deadzone
    float horizontal = sys.input->getAxisValue(0, GamepadAxis::LeftX);
    float vertical = sys.input->getAxisValue(0, GamepadAxis::LeftY);

    if (std::abs(horizontal) > 0.2f) {  // Deadzone
        movePlayer(horizontal);
    }
}
```

---

## Physics

### Creating Physics Bodies

```cpp
// From code
PhysicsBodyDef def{
    .type = BodyType::Dynamic,
    .transform = { .x = 100, .y = 200 },
    .size = { 32, 48 },
    .fixedRotation = true  // Prevent rotation (platformers)
};
sys.physics->createBody(entity, def);
```

### From Blueprints (Preferred)

```lua
-- data/blueprints/player.lua
return {
    physics = {
        type = "dynamic",
        fixedRotation = true,
        linearDamping = 0.5,

        shape = {
            type = "box",
            width = 28,
            height = 44,
            offsetY = -2  -- Offset from center
        }
    }
}
```

### Collision Handling

```cpp
// Subscribe to collision events
sys.events->subscribe(Events::CollisionBegin, [this](const EventData& data) {
    auto& collision = std::get<CollisionEvent>(data);

    Entity a = collision.entityA;
    Entity b = collision.entityB;

    // Check what collided
    if (sys.entities->has<PlayerTag>(a) && sys.entities->has<EnemyTag>(b)) {
        onPlayerHitEnemy(a, b);
    }

    if (sys.entities->has<PlayerTag>(a) && sys.entities->has<CoinTag>(b)) {
        collectCoin(b);
    }
});
```

### Raycasting

```cpp
// Check ground beneath player
auto result = sys.physics->raycast(
    playerPos,                          // Start
    {playerPos.x, playerPos.y + 50},    // End (downward)
    CollisionMask::Ground               // What to hit
);

if (result.hit && result.distance < 5.0f) {
    isGrounded_ = true;
}
```

---

## Asset Management

### The AssetSystem Rule

**ALL file system interactions MUST go through AssetSystem.** Never directly read files from other systems.

```cpp
// ❌ WRONG - Direct file I/O
std::ifstream file("data/config.lua");

// ✅ CORRECT - Through AssetSystem
AssetHandle config = sys.assets->registerAsset(AssetType::Data, "config.lua");
sys.assets->loadAsset(config);
```

### Loading Assets

```cpp
void loadAssets() {
    auto& sys = engine_->systems();

    // Register assets (doesn't load yet)
    playerTexture_ = sys.assets->registerAsset(
        AssetType::Texture, "textures/player.png"
    );
    jumpSound_ = sys.assets->registerAsset(
        AssetType::Sound, "audio/sfx/jump.ogg"
    );

    // Load synchronously (blocking)
    sys.assets->loadAsset(playerTexture_);

    // Load asynchronously (non-blocking, preferred)
    sys.assets->loadAssetAsync(jumpSound_, [](AssetHandle h, AssetState state) {
        if (state == AssetState::Loaded) {
            logInfo("Jump sound loaded!");
        }
    });
}
```

### Hot Reload

```cpp
void initialize() {
    auto& sys = engine_->systems();

    // Enable hot reload in debug builds
    sys.assets->enableHotReload(true);

    // Subscribe to texture changes
    sys.assets->subscribeToType(AssetType::Texture,
        [this](AssetHandle h, AssetType) {
            logInfo("Texture reloaded, refreshing sprites...");
            refreshSprites();
        }
    );
}

void updateFixed(DeltaTime dt) {
    // Process hot reload notifications
    sys.assets->update();
}
```

### Path Schemes

| Scheme | Resolves To | Use For |
|--------|-------------|---------|
| `:assets:/` | `data/` | Game-specific assets |
| `:library:/` | Engine library | Shared/engine assets |
| Relative | Current directory | Quick testing |

```cpp
// These are equivalent:
sys.assets->registerAsset(AssetType::Texture, ":assets:/textures/player.png");
sys.assets->registerAsset(AssetType::Texture, "data/textures/player.png");
```

---

## Levels

### Lua Level Format

```lua
-- data/levels/level1.lua
local TILE = 32
local GROUND_Y = 500

return {
    name = "Green Hills",
    width = 3200,
    height = 600,

    -- Named spawn points
    spawnPoints = {
        player = { x = 100, y = GROUND_Y - 50 },
        checkpoint1 = { x = 1600, y = GROUND_Y - 50 },
        boss = { x = 3000, y = GROUND_Y - 100 }
    },

    -- Entity definitions
    entities = {
        -- Ground platforms
        { type = "platform", x = 0, y = GROUND_Y, width = 3200, height = 100 },

        -- Enemies using loops
        generateEnemies(),

        -- Coins
        { type = "coin", x = 200, y = 400 },
        { type = "coin", x = 250, y = 400 },
    }
}

-- Helper function for procedural content
function generateEnemies()
    local enemies = {}
    for i = 1, 5 do
        table.insert(enemies, {
            type = "slime",
            x = 400 + i * 200,
            y = GROUND_Y - 30,
            patrolDistance = 100
        })
    end
    return table.unpack(enemies)
end
```

### Loading Levels

```cpp
void loadLevel(const std::string& path) {
    auto& sys = engine_->systems();

    // Unload current level
    if (currentLevel_) {
        sys.levels->unloadLevel(currentLevel_);
    }

    // Load level asset
    AssetHandle levelAsset = sys.assets->registerAsset(
        AssetType::Level, path
    );
    sys.assets->loadAsset(levelAsset);

    // Parse and activate level
    auto result = sys.levels->loadLevel(levelAsset);
    if (!result) {
        logError("Failed to load level: " + path);
        return;
    }

    currentLevel_ = *result;

    // Spawn player at spawn point
    auto spawnPos = sys.levels->getSpawnPoint(currentLevel_, "player");
    if (spawnPos) {
        spawnPlayer(spawnPos->x, spawnPos->y);
    }
}
```

---

## Blueprints

### Inheritance

```lua
-- data/blueprints/_base/character.lua
return {
    transform = { x = 0, y = 0 },
    sprite = { layer = 10 },
    physics = { type = "dynamic" },
    health = 100
}

-- data/blueprints/player.lua
return {
    extends = "_base/character",  -- Inherit from character

    sprite = {
        texture = "textures/player.png",
        width = 32, height = 48
        -- layer inherited as 10
    },

    health = 150,  -- Override
    speed = 200,   -- New property
}
```

### Creating Entities from Blueprints

```cpp
// Simple creation
Entity player = sys.blueprints->createEntity("player");

// With position
Entity enemy = sys.blueprints->createEntityAt("enemies/slime", 500, 300);

// With property overrides
Entity boss = sys.blueprints->createEntity("enemies/slime", {
    {"health", 300},
    {"scale", 2.0f},
    {"isBoss", true}
});
```

---

## Audio

### Playing Sounds

```cpp
// One-shot sound effect
sys.audio->playSound("audio/sfx/jump.ogg");

// With volume
sys.audio->playSound("audio/sfx/coin.ogg", 0.8f);

// Background music (loops by default)
sys.audio->playMusic("audio/music/level1.ogg");

// Control music
sys.audio->setMusicVolume(0.5f);
sys.audio->pauseMusic();
sys.audio->resumeMusic();
```

### Spatial Audio

```cpp
// Set listener position (usually camera or player)
sys.audio->setListenerPosition(playerPos.x, playerPos.y);

// Play sound at position
sys.audio->playSoundAt("audio/sfx/explosion.ogg", enemyPos.x, enemyPos.y);
```

---

## Camera

### Basic Camera Control

```cpp
void updateCamera(float dt) {
    auto& sys = engine_->systems();

    // Follow player with smoothing
    if (player_) {
        auto& playerPos = sys.entities->get<Transform2D>(player_);
        sys.camera->follow(playerPos.x, playerPos.y, 5.0f * dt);
    }

    // Clamp to level bounds
    auto bounds = sys.levels->getLevelBounds(currentLevel_);
    sys.camera->clampToBounds(bounds);
}
```

### Screen Shake

```cpp
void onPlayerDamaged() {
    sys.camera->shake(0.3f, 10.0f);  // Duration, intensity
}
```

### Coordinate Conversion

```cpp
// Screen to world (for mouse interaction)
auto [worldX, worldY] = sys.camera->screenToWorld(mouseX, mouseY);

// World to screen (for UI positioning)
auto [screenX, screenY] = sys.camera->worldToScreen(entityX, entityY);
```

---

## Events

### Subscribing to Events

```cpp
void initialize() {
    auto& sys = engine_->systems();

    // Built-in events
    sys.events->subscribe(Events::CollisionBegin, [this](const EventData& d) {
        handleCollision(std::get<CollisionEvent>(d));
    });

    sys.events->subscribe(Events::AssetLoaded, [this](const EventData& d) {
        auto& e = std::get<AssetLoadedEvent>(d);
        logInfo("Asset loaded: " + e.path);
    });
}
```

### Custom Events

```cpp
// Define custom event types
namespace GameEvents {
    constexpr EventType PlayerDied = 1000;
    constexpr EventType ScoreChanged = 1001;
}

struct ScoreChangedEvent {
    int oldScore;
    int newScore;
};

// Emit custom events
void addScore(int points) {
    int oldScore = score_;
    score_ += points;

    sys.events->emit(GameEvents::ScoreChanged, ScoreChangedEvent{
        .oldScore = oldScore,
        .newScore = score_
    });
}

// Subscribe to custom events
sys.events->subscribe(GameEvents::ScoreChanged, [this](const EventData& d) {
    auto& e = std::get<ScoreChangedEvent>(d);
    ui_->updateScoreDisplay(e.newScore);
});
```

---

## Save System

### Saving Game State

```cpp
struct SaveData {
    int level;
    int score;
    int health;
    float playerX, playerY;
    std::vector<std::string> collectedItems;

    // Cereal serialization
    template<class Archive>
    void serialize(Archive& ar) {
        ar(level, score, health, playerX, playerY, collectedItems);
    }
};

void saveGame(int slot) {
    SaveData data{
        .level = currentLevelIndex_,
        .score = score_,
        .health = sys.entities->get<Health>(player_).current,
        .playerX = sys.entities->get<Transform2D>(player_).x,
        .playerY = sys.entities->get<Transform2D>(player_).y,
        .collectedItems = inventory_
    };

    sys.save->save(slot, data);
}

void loadGame(int slot) {
    auto result = sys.save->load<SaveData>(slot);
    if (result) {
        loadLevel(result->level);
        score_ = result->score;
        // ... restore state
    }
}
```

---

## Best Practices

### Do's

1. **Use EngineBuilder** - Declarative system configuration
2. **Use blueprints for entities** - Define in Lua, spawn from C++
3. **Use action mapping for input** - Don't hardcode keys
4. **Use events for decoupling** - Systems shouldn't know about each other
5. **Use fixed timestep for physics** - Deterministic simulation
6. **Use async loading** - Don't block the main thread
7. **Enable hot reload** - Faster iteration
8. **Use ,AOE for movement** - Dvorak-friendly defaults
9. **Access systems through engine.systems()** - Never store raw system pointers

### Don'ts

1. **Don't read files directly** - Always use AssetSystem
2. **Don't depend on implementations** - Only use interface contracts
3. **Don't create deep inheritance** - Use composition
4. **Don't poll every frame unnecessarily** - Use events
5. **Don't hardcode values** - Put them in Lua config
6. **Don't ignore the fixed timestep** - Physics needs it
7. **Don't forget cleanup** - Unsubscribe, unload, destroy

---

## System Documentation

Detailed guides for each engine system are in `engine-docs/`:

- [Entity System](engine-docs/ENTITY-SYSTEM.md) - ECS architecture
- [Graphics System](engine-docs/GRAPHICS-SYSTEM.md) - Rendering (Vulkan/OpenGL)
- [Physics System](engine-docs/PHYSICS-SYSTEM.md) - Box2D integration
- [Audio System](engine-docs/AUDIO-SYSTEM.md) - FMOD audio
- [Input System](engine-docs/INPUT-SYSTEM.md) - Keyboard/mouse/gamepad
- [Asset System](engine-docs/ASSET-SYSTEM.md) - Asset loading and hot reload
- [Level System](engine-docs/LEVEL-SYSTEM.md) - Lua level definitions
- [Config System](engine-docs/CONFIG-SYSTEM.md) - Lua configuration
- [Events System](engine-docs/EVENTS-SYSTEM.md) - Event-driven communication
- [Camera System](engine-docs/CAMERA-SYSTEM.md) - Camera control
- [AI System](engine-docs/AI-SYSTEM.md) - Behavior trees and pathfinding
- [Blueprints System](engine-docs/BLUEPRINTS-SYSTEM.md) - Entity templates
- [Save System](engine-docs/SAVE-SYSTEM.md) - Game saves
- [UI System](engine-docs/UI-SYSTEM.md) - RmlUi interface
- [GAS System](engine-docs/GAS-SYSTEM.md) - Gameplay abilities
- [Decision Runbook](engine-docs/DECISION-RUNBOOK.md) - When to use what

---

## Troubleshooting

### Common Issues

**"Asset not found"**
- Check path is correct and uses proper scheme (`:assets:/`)
- Ensure asset is registered before loading
- Check file exists in `data/` directory

**"Entity has no component"**
- Verify blueprint includes the component
- Check inheritance chain
- Use `sys.entities->has<T>()` before `get<T>()`

**"Physics bodies not colliding"**
- Check collision masks/categories
- Verify both bodies have shapes attached
- Ensure physics step is being called in updateFixed()

**"Hot reload not working"**
- Call `sys.assets->enableHotReload(true)`
- Call `sys.assets->update()` every frame
- Check file watcher is running (efsw)

**"Input not responding"**
- Check correct key codes (Dvorak vs QWERTY)
- Verify input system is enabled in EngineBuilder
- Check window has focus

**"System pointer is null"**
- Ensure you enabled the system with EngineBuilder (e.g., `.withPhysics()`)
- Check for null before accessing: `if (sys.physics) { ... }`

---

## Performance Tips

1. **Profile with Tracy** - Build with `BESTOW_ENABLE_TRACY=ON`
2. **Batch draw calls** - Use sprite batching
3. **Pool frequently created entities** - Avoid allocation churn
4. **Use async asset loading** - Don't block main thread
5. **Minimize component iteration** - Cache views when possible
6. **Use spatial partitioning** - For large entity counts
7. **Use fixed timestep properly** - Physics in updateFixed(), rendering in render()

---

## Before You Start Coding

> **REMINDER: Always consult the [Decision Runbook](engine-docs/DECISION-RUNBOOK.md) before implementing any feature.**

The runbook provides decision trees for:
- Where code belongs (Lua vs C++, which file)
- How to store data (components, config, save)
- How to load assets (sync vs async, hot reload)
- How systems should communicate (events vs direct calls)
- How to handle input, collisions, errors
- Performance optimization strategies

**Follow the runbook. Every time. No exceptions.**
