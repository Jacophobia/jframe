# Game Development with Bestow Engine

> **IMPORTANT: Before implementing ANY feature, consult the [Decision Runbook](engine-docs/DECISION-RUNBOOK.md).** The runbook contains decision trees for every common scenario - where code belongs, how to store data, how systems communicate, etc. Following the runbook ensures consistent, correct architectural decisions.

> **This guide teaches you how to build games with the Bestow engine from the ground up.** It covers architecture, best practices, and practical patterns for creating polished games.

## Quick Start

```cpp
// main.cpp - Minimal game setup
import bestow.core;
import bestow.services;

class MyGame : public bestow::IApplication {
public:
    MyGame(
        bestow::IGraphicsSystem& graphics,
        bestow::IEntitySystem& entities,
        bestow::IInputSystem& input
    ) : graphics_(&graphics), entities_(&entities), input_(&input) {}

    void run() override {
        initialize();
        while (!shouldQuit_) {
            float dt = frameTimer_.tick();
            handleInput();
            update(dt);
            render();
        }
    }

private:
    bestow::IGraphicsSystem* graphics_;
    bestow::IEntitySystem* entities_;
    bestow::IInputSystem* input_;
    bestow::core::FrameTimer frameTimer_;
    bool shouldQuit_ = false;

    void initialize() { /* Setup game */ }
    void handleInput() { /* Process input */ }
    void update(float dt) { /* Update game logic */ }
    void render() { /* Draw everything */ }
};

int main() {
    bestow::core::Engine engine;
    engine.run<MyGame>();
    return 0;
}
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

### 2. Data-Driven Everything

**Define behavior in data, not code.** This enables:
- Hot reload during development
- Designer-friendly iteration
- Mod support
- Easier balancing

### 3. Dvorak-Friendly Controls

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

### 4. Vulkan-First Rendering

**Vulkan is the primary renderer.** OpenGL exists for:
- Rapid prototyping
- Fallback on older hardware
- Simpler debugging

Always test with Vulkan before shipping.

---

## Project Structure

```
games/game1/
├── CLAUDE.md              # This file
├── CMakeLists.txt         # Build configuration
├── src/
│   ├── main.cpp           # Entry point
│   ├── Game.cppm          # Main game class
│   └── systems/           # Custom game systems
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

## The Game Loop

### Basic Structure

```cpp
void Game::run() {
    // 1. Initialize all systems
    initialize();

    // 2. Load initial assets
    loadAssets();

    // 3. Load first level
    loadLevel("levels/level1.lua");

    // 4. Main loop
    while (!shouldQuit_) {
        float dt = frameTimer_.tick();

        // Fixed timestep for physics
        accumulator_ += dt;
        while (accumulator_ >= FIXED_DT) {
            fixedUpdate(FIXED_DT);
            accumulator_ -= FIXED_DT;
        }

        // Variable timestep for everything else
        handleInput();
        update(dt);
        render();

        // Process async operations
        assets_->update();
        events_->processDeferred();
    }

    // 5. Cleanup
    shutdown();
}
```

### Fixed vs Variable Timestep

| Fixed Timestep (fixedUpdate) | Variable Timestep (update) |
|------------------------------|---------------------------|
| Physics simulation | Input handling |
| Collision detection | Animation |
| Deterministic game logic | Camera movement |
| Networking | UI updates |

```cpp
static constexpr float FIXED_DT = 1.0f / 60.0f;  // 60 Hz physics

void Game::fixedUpdate(float dt) {
    physics_->step(dt);
    // Other deterministic systems...
}

void Game::update(float dt) {
    camera_->update(dt);
    animations_->update(dt);
    // Other variable systems...
}
```

---

## Entity Component System (ECS)

Bestow uses EnTT for its ECS. **Think in components, not objects.**

### Components Are Data

```cpp
// Good - Pure data
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

### Systems Are Logic

```cpp
void MovementSystem::update(float dt) {
    // Iterate all entities with Transform2D AND Velocity
    auto view = entities_->view<Transform2D, Velocity>();

    for (auto entity : view) {
        auto& transform = view.get<Transform2D>(entity);
        auto& velocity = view.get<Velocity>(entity);

        transform.x += velocity.x * dt;
        transform.y += velocity.y * dt;
    }
}
```

### Entity Creation

```cpp
// From code
Entity player = entities_->createEntity();
entities_->emplace<Transform2D>(player, 100.0f, 200.0f);
entities_->emplace<Velocity>(player);
entities_->emplace<Health>(player, 100, 100);
entities_->emplace<PlayerTag>(player);

// From blueprint (preferred)
Entity player = blueprints_->createEntity("player");
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
void Game::handleInput() {
    // Keyboard - Use ,AOE for Dvorak movement
    if (input_->isKeyHeld(Key::Comma)) {  // Up (Dvorak W)
        moveUp();
    }
    if (input_->isKeyHeld(Key::A)) {      // Left
        moveLeft();
    }
    if (input_->isKeyHeld(Key::O)) {      // Down (Dvorak S)
        moveDown();
    }
    if (input_->isKeyHeld(Key::E)) {      // Right (Dvorak D)
        moveRight();
    }

    // Single press detection
    if (input_->isKeyJustPressed(Key::Space)) {
        jump();
    }

    // Mouse
    auto [mx, my] = input_->getMousePosition();
    if (input_->isMouseButtonJustPressed(MouseButton::Left)) {
        shoot(mx, my);
    }
}
```

### Action Mapping (Recommended)

```cpp
// Define actions once
input_->bindAction("move_up", Key::Comma);     // Dvorak
input_->bindAction("move_up", Key::W);         // QWERTY fallback
input_->bindAction("jump", Key::Space);
input_->bindAction("jump", GamepadButton::A);  // Controller support

// Use actions everywhere
if (input_->isActionHeld("move_up")) {
    moveUp();
}
if (input_->isActionJustPressed("jump")) {
    jump();
}
```

### Gamepad Support

```cpp
// Check for connected controllers
if (input_->hasGamepad(0)) {
    // Analog stick with deadzone
    float horizontal = input_->getAxis(0, GamepadAxis::LeftX);
    float vertical = input_->getAxis(0, GamepadAxis::LeftY);

    if (std::abs(horizontal) > 0.2f) {  // Deadzone
        move(horizontal, 0);
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
    .fixedRotation = true  // Prevent rotation (platformers)
};
physics_->createBody(entity, def);

// Add collision shape
physics_->addBoxShape(entity, 32, 48);  // width, height
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
events_->subscribe(Events::CollisionBegin, [this](const EventData& data) {
    auto& collision = std::get<CollisionEvent>(data);

    Entity a = collision.entityA;
    Entity b = collision.entityB;

    // Check what collided
    if (entities_->has<PlayerTag>(a) && entities_->has<EnemyTag>(b)) {
        onPlayerHitEnemy(a, b);
    }

    if (entities_->has<PlayerTag>(a) && entities_->has<CoinTag>(b)) {
        collectCoin(b);
    }
});
```

### Raycasting

```cpp
// Check ground beneath player
auto result = physics_->raycast(
    playerPos,                    // Start
    {playerPos.x, playerPos.y + 50},  // End (downward)
    CollisionMask::Ground         // What to hit
);

if (result.hit && result.distance < 5.0f) {
    isGrounded_ = true;
}
```

---

## Asset Management

### Loading Assets

```cpp
void Game::loadAssets() {
    // Register assets (doesn't load yet)
    playerTexture_ = assets_->registerAsset(AssetType::Texture, "textures/player.png");
    jumpSound_ = assets_->registerAsset(AssetType::Sound, "audio/sfx/jump.ogg");

    // Load synchronously (blocking)
    assets_->loadAsset(playerTexture_);

    // Load asynchronously (non-blocking)
    assets_->loadAssetAsync(jumpSound_, [this](AssetHandle h, AssetState state) {
        if (state == AssetState::Loaded) {
            spdlog::info("Jump sound loaded!");
        }
    });
}
```

### Hot Reload

```cpp
void Game::initialize() {
    // Enable hot reload in debug builds
    assets_->enableHotReload(true);

    // Subscribe to texture changes
    assets_->subscribeToType(AssetType::Texture, [this](AssetHandle h, AssetType) {
        spdlog::info("Texture reloaded, refreshing sprites...");
        refreshSprites();
    });
}

void Game::update(float dt) {
    // Process hot reload notifications
    assets_->update();
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
assets_->registerAsset(AssetType::Texture, ":assets:/textures/player.png");
assets_->registerAsset(AssetType::Texture, "data/textures/player.png");
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
        { type = "coin", x = 300, y = 400 },
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
void Game::loadLevel(const std::string& path) {
    // Unload current level
    if (currentLevel_) {
        level_->unloadLevel(*currentLevel_);
    }

    // Load level asset
    AssetHandle levelAsset = assets_->registerAsset(AssetType::Data, path);
    assets_->loadAsset(levelAsset);

    // Parse level
    auto result = level_->loadLevel(levelAsset);
    if (!result) {
        spdlog::error("Failed to load level: {}", path);
        return;
    }

    currentLevel_ = *result;
    level_->setActiveLevel(currentLevel_);

    // Spawn entities from definitions
    for (const auto& def : level_->getEntityDefs(currentLevel_)) {
        Entity e = blueprints_->createEntity(def.type);

        // Apply transform from level
        if (entities_->has<Transform2D>(e)) {
            auto& t = entities_->get<Transform2D>(e);
            t.x = def.transform.x;
            t.y = def.transform.y;
        }

        // Apply custom properties
        applyEntityProperties(e, def.properties);
    }

    // Spawn player at spawn point
    auto spawnPos = level_->getSpawnPoint(currentLevel_, "player");
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

-- data/blueprints/enemies/slime.lua
return {
    extends = "_base/character",

    sprite = {
        texture = "textures/enemies/slime.png",
        width = 24, height = 24
    },

    health = 30,   -- Override
    damage = 10,   -- New property

    ai = {
        behavior = "patrol",
        speed = 50
    }
}
```

### Creating Entities from Blueprints

```cpp
// Simple creation
Entity player = blueprints_->createEntity("player");

// With position
Entity enemy = blueprints_->createEntityAt("enemies/slime", 500, 300);

// With property overrides
Entity boss = blueprints_->createEntity("enemies/slime", {
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
audio_->playSound("audio/sfx/jump.ogg");

// With volume
audio_->playSound("audio/sfx/coin.ogg", 0.8f);

// Background music (loops by default)
audio_->playMusic("audio/music/level1.ogg");

// Control music
audio_->setMusicVolume(0.5f);
audio_->pauseMusic();
audio_->resumeMusic();
```

### Spatial Audio

```cpp
// Set listener position (usually camera or player)
audio_->setListenerPosition(playerPos.x, playerPos.y);

// Play sound at position
audio_->playSoundAt("audio/sfx/explosion.ogg", enemyPos.x, enemyPos.y);
```

### Audio Configuration

```lua
-- data/config/audio.lua
return {
    master_volume = 1.0,
    music_volume = 0.7,
    sfx_volume = 1.0,

    -- Preload frequently used sounds
    preload = {
        "audio/sfx/jump.ogg",
        "audio/sfx/coin.ogg",
        "audio/sfx/hit.ogg"
    }
}
```

---

## Camera

### Basic Camera Control

```cpp
void Game::updateCamera(float dt) {
    // Follow player with smoothing
    if (player_) {
        auto& playerPos = entities_->get<Transform2D>(player_);
        camera_->follow(playerPos.x, playerPos.y, 5.0f * dt);  // Lerp factor
    }

    // Clamp to level bounds
    auto bounds = level_->getLevelBounds(currentLevel_);
    camera_->clampToBounds(bounds);
}
```

### Screen Shake

```cpp
void Game::onPlayerDamaged() {
    camera_->shake(0.3f, 10.0f);  // Duration, intensity
}
```

### Coordinate Conversion

```cpp
// Screen to world (for mouse interaction)
auto [worldX, worldY] = camera_->screenToWorld(mouseX, mouseY);

// World to screen (for UI positioning)
auto [screenX, screenY] = camera_->worldToScreen(entityX, entityY);
```

---

## Events

### Subscribing to Events

```cpp
void Game::initialize() {
    // Built-in events
    events_->subscribe(Events::CollisionBegin, [this](const EventData& d) {
        handleCollision(std::get<CollisionEvent>(d));
    });

    events_->subscribe(Events::AssetLoaded, [this](const EventData& d) {
        auto& e = std::get<AssetLoadedEvent>(d);
        spdlog::info("Asset loaded: {}", e.path);
    });
}
```

### Custom Events

```cpp
// Define custom event types
namespace GameEvents {
    constexpr EventType PlayerDied = 1000;
    constexpr EventType ScoreChanged = 1001;
    constexpr EventType LevelCompleted = 1002;
}

struct ScoreChangedEvent {
    int oldScore;
    int newScore;
};

// Emit custom events
void Game::addScore(int points) {
    int oldScore = score_;
    score_ += points;

    events_->emit(GameEvents::ScoreChanged, ScoreChangedEvent{
        .oldScore = oldScore,
        .newScore = score_
    });
}

// Subscribe to custom events
events_->subscribe(GameEvents::ScoreChanged, [this](const EventData& d) {
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

void Game::saveGame(int slot) {
    SaveData data{
        .level = currentLevelIndex_,
        .score = score_,
        .health = entities_->get<Health>(player_).current,
        .playerX = entities_->get<Transform2D>(player_).x,
        .playerY = entities_->get<Transform2D>(player_).y,
        .collectedItems = inventory_
    };

    save_->save(slot, data);
}

void Game::loadGame(int slot) {
    auto result = save_->load<SaveData>(slot);
    if (result) {
        loadLevel(result->level);
        score_ = result->score;
        // ... restore state
    }
}
```

### Auto-Save

```cpp
void Game::onCheckpointReached() {
    saveGame(0);  // Slot 0 = auto-save
}
```

---

## UI System

### Loading UI Documents

```cpp
void Game::initializeUI() {
    // Load main menu
    mainMenu_ = ui_->loadDocument("ui/main_menu.rml");

    // Load HUD
    hud_ = ui_->loadDocument("ui/hud.rml");
    ui_->showDocument(hud_);
}
```

### RML Document Example

```html
<!-- data/ui/hud.rml -->
<rml>
<head>
    <link type="text/rcss" href="styles/hud.rcss"/>
</head>
<body>
    <div id="health-bar">
        <div id="health-fill" style="width: 100%;"/>
    </div>
    <div id="score">Score: <span id="score-value">0</span></div>
</body>
</rml>
```

### Updating UI

```cpp
void Game::updateHUD() {
    // Update health bar
    float healthPercent = (float)health_ / maxHealth_ * 100;
    ui_->setProperty(hud_, "#health-fill", "width",
                     std::to_string(healthPercent) + "%");

    // Update score
    ui_->setInnerText(hud_, "#score-value", std::to_string(score_));
}
```

---

## Best Practices Summary

### Do's

1. **Use blueprints for entities** - Define in Lua, spawn from C++
2. **Use action mapping for input** - Don't hardcode keys
3. **Use events for decoupling** - Systems shouldn't know about each other
4. **Use fixed timestep for physics** - Deterministic simulation
5. **Use async loading** - Don't block the main thread
6. **Enable hot reload** - Faster iteration
7. **Use ,AOE for movement** - Dvorak-friendly defaults

### Don'ts

1. **Don't read files directly** - Always use AssetSystem
2. **Don't create deep inheritance** - Use composition
3. **Don't poll every frame unnecessarily** - Use events
4. **Don't hardcode values** - Put them in Lua config
5. **Don't ignore the fixed timestep** - Physics needs it
6. **Don't forget cleanup** - Unsubscribe, unload, destroy

---

## System Documentation

Detailed guides for each engine system are in `engine-docs/`:

- [Entity System](engine-docs/ENTITY-SYSTEM.md) - ECS architecture
- [Graphics System](engine-docs/GRAPHICS-SYSTEM.md) - Rendering (Vulkan/OpenGL)
- [Physics System](engine-docs/PHYSICS-SYSTEM.md) - Box2D/Jolt integration
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
- Use `entities_->has<T>()` before `get<T>()`

**"Physics bodies not colliding"**
- Check collision masks/categories
- Verify both bodies have shapes attached
- Ensure physics step is being called

**"Hot reload not working"**
- Call `assets_->enableHotReload(true)`
- Call `assets_->update()` every frame
- Check file watcher is running (efsw)

**"Input not responding"**
- Check correct key codes (Dvorak vs QWERTY)
- Verify input system is being updated
- Check window has focus

---

## Performance Tips

1. **Profile with Tracy** - `BESTOW_ENABLE_TRACY=ON`
2. **Batch draw calls** - Use sprite batching
3. **Pool frequently created entities** - Avoid allocation
4. **Use async asset loading** - Don't block main thread
5. **Minimize component iteration** - Cache views when possible
6. **Use spatial partitioning** - For large entity counts

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
