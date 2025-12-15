# Bestow Engine Decision Runbook

> **Purpose:** This runbook provides decision trees for AI agents (and humans) to follow when implementing features in a Bestow game. Follow these flowcharts to make consistent, correct architectural decisions.

---

## Table of Contents

1. [Where Does This Code Belong?](#1-where-does-this-code-belong)
2. [How Should I Store This Data?](#2-how-should-i-store-this-data)
3. [How Should I Load This Asset?](#3-how-should-i-load-this-asset)
4. [How Should Systems Communicate?](#4-how-should-systems-communicate)
5. [How Should I Handle This Input?](#5-how-should-i-handle-this-input)
6. [How Should I Implement This Entity?](#6-how-should-i-implement-this-entity)
7. [How Should I Handle This Collision?](#7-how-should-this-collision-be-handled)
8. [Where Should This Configuration Live?](#8-where-should-this-configuration-live)
9. [How Should I Render This?](#9-how-should-i-render-this)
10. [How Should I Save This?](#10-how-should-i-save-this)
11. [How Should I Structure This UI?](#11-how-should-i-structure-this-ui)
12. [How Should I Implement This AI?](#12-how-should-i-implement-this-ai)
13. [How Should I Handle This Error?](#13-how-should-i-handle-this-error)
14. [Performance Decision Tree](#14-performance-decision-tree)

---

## 1. Where Does This Code Belong?

```
START: I need to add new functionality
│
├─► Is it game-specific logic?
│   │
│   ├─► YES: Can it be defined in Lua?
│   │   │
│   │   ├─► YES: Is it entity data/templates?
│   │   │   └─► Put in data/blueprints/*.lua
│   │   │
│   │   ├─► YES: Is it level layout/spawning?
│   │   │   └─► Put in data/levels/*.lua
│   │   │
│   │   ├─► YES: Is it configuration/tuning values?
│   │   │   └─► Put in data/config/*.lua
│   │   │
│   │   ├─► YES: Is it material/shader parameters?
│   │   │   └─► Put in data/materials/*.lua
│   │   │
│   │   └─► NO: Needs C++ for performance/complexity
│   │       └─► Put in src/systems/*.cpp as a game system
│   │
│   └─► NO: Is it reusable across games?
│       │
│       ├─► YES: Should it be an engine feature?
│       │   └─► Propose addition to bestow-* system
│       │
│       └─► NO: Put in src/ as game-specific code
│
└─► Is it a component (data)?
    │
    ├─► YES: Is it just for tagging/categorization?
    │   └─► Use an empty struct tag component
    │
    └─► YES: Does it hold data?
        └─► Create a struct with public members, no logic
```

### Quick Reference

| Type of Code | Location | Format |
|--------------|----------|--------|
| Entity templates | `data/blueprints/` | Lua |
| Level layouts | `data/levels/` | Lua |
| Game settings | `data/config/` | Lua |
| Materials | `data/materials/` | Lua |
| Game systems | `src/systems/` | C++ |
| Components | `src/components/` | C++ structs |
| Main game class | `src/Game.cpp` | C++ |

---

## 2. How Should I Store This Data?

```
START: I need to store some data
│
├─► Is it per-entity data?
│   │
│   ├─► YES: Does every entity of this type need it?
│   │   │
│   │   ├─► YES: Add to blueprint as property
│   │   │   └─► Access via entities->get<T>(entity)
│   │   │
│   │   └─► NO: Add component at runtime when needed
│   │       └─► entities->emplace<T>(entity, ...)
│   │
│   └─► NO: Is it global game state?
│       │
│       ├─► YES: Does it persist across levels?
│       │   │
│       │   ├─► YES: Does it need to be saved?
│       │   │   └─► Store in SaveData struct, use SaveSystem
│       │   │
│       │   └─► NO: Store as Game class member variable
│       │
│       └─► NO: Is it per-level state?
│           └─► Store in level-specific manager, reset on level change
│
├─► Is it configuration (tunable values)?
│   │
│   ├─► YES: Can it change at runtime?
│   │   │
│   │   ├─► YES: Use ConfigSystem with hot reload
│   │   │   └─► config->get<float>("player.speed")
│   │   │
│   │   └─► NO: Use constexpr in code or Lua config
│   │
│   └─► NO: Is it user preferences?
│       └─► Use SaveSystem with settings profile
│
└─► Is it temporary/frame data?
    └─► Use local variables or frame-scoped storage
```

### Data Storage Quick Reference

| Data Type | Storage | Access Pattern |
|-----------|---------|----------------|
| Entity properties | Component | `entities->get<T>()` |
| Entity tags | Tag component | `entities->allOf<T>()` |
| Global game state | Game class member | Direct access |
| Persistent progress | SaveSystem | `save->save()/load()` |
| Tunable values | ConfigSystem | `config->get<float>()` |
| Per-level state | Level manager | Reset on level change |

---

## 3. How Should I Load This Asset?

```
START: I need to load a file
│
├─► STOP: Am I using direct file I/O?
│   │
│   ├─► YES: Is this the SaveSystem?
│   │   │
│   │   ├─► YES: OK - SaveSystem is the only exception
│   │   │
│   │   └─► NO: WRONG! Use AssetSystem instead
│   │       └─► Never use std::ifstream, lua.safe_script_file(), etc.
│   │
│   └─► NO: Good, continue...
│
├─► Is it needed immediately at startup?
│   │
│   ├─► YES: Use synchronous loading
│   │   └─► assets->loadAsset(handle)
│   │
│   └─► NO: Can the game continue without it?
│       │
│       ├─► YES: Use async loading
│       │   └─► assets->loadAssetAsync(handle, callback)
│       │
│       └─► NO: Load sync or show loading screen
│
├─► Will I need hot reload during development?
│   │
│   ├─► YES: Subscribe to changes
│   │   │
│   │   ├─► For specific asset:
│   │   │   └─► assets->subscribe(handle, callback)
│   │   │
│   │   └─► For asset type (all shaders, all textures):
│   │       └─► assets->subscribeToType(AssetType::Texture, callback)
│   │
│   └─► NO: Just load and use
│
└─► What type of asset is it?
    │
    ├─► Texture → AssetType::Texture → TextureData
    ├─► Sound → AssetType::Sound → SoundData
    ├─► Shader → AssetType::Shader → ShaderData
    ├─► Font → AssetType::Font → FontData
    ├─► Lua/JSON data → AssetType::Data → DataAsset
    ├─► 3D Mesh → AssetType::Mesh → MeshData
    ├─► 3D Model → AssetType::Model → ModelData
    ├─► Material → AssetType::Material → MaterialData
    ├─► Cubemap → AssetType::Cubemap → CubemapData
    └─► NavMesh → AssetType::NavMesh → NavMeshData
```

### Asset Loading Patterns

```cpp
// Pattern 1: Sync load at startup
AssetHandle tex = assets->registerAsset(AssetType::Texture, "textures/player.png");
assets->loadAsset(tex);  // Blocks until loaded

// Pattern 2: Async load with callback
assets->loadAssetAsync(tex, [this](AssetHandle h, AssetState state) {
    if (state == AssetState::Loaded) {
        onTextureReady(h);
    }
});

// Pattern 3: Hot reload subscription (specific asset)
SubscriptionId subId = assets->subscribe(tex, [this](AssetHandle h, AssetType t) {
    refreshSprite(h);  // Called when file changes
});

// Pattern 4: Hot reload subscription (all of type)
SubscriptionId typeSubId = assets->subscribeToType(AssetType::Shader,
    [this](AssetHandle h, AssetType t) {
        recompileShader(h);  // Called when ANY shader changes
    }
);

// Important: Call update() in your game loop
void update() {
    assets->update();  // Processes async loads and hot reload notifications
}

// Clean up subscriptions
assets->unsubscribe(subId);
```

---

## 4. How Should Systems Communicate?

```
START: System A needs to notify/call System B
│
├─► Is it a one-time notification (fire and forget)?
│   │
│   ├─► YES: Do multiple systems need to know?
│   │   │
│   │   ├─► YES: Use EventSystem
│   │   │   │
│   │   │   ├─► Immediate dispatch:
│   │   │   │   └─► events->publish(Events::PlayerDied, data)
│   │   │   │
│   │   │   └─► Deferred dispatch (processed later):
│   │   │       └─► events->queue(Events::Checkpoint, data)
│   │   │           └─► events->processQueue() in update loop
│   │   │
│   │   └─► NO: Is it performance-critical?
│   │       │
│   │       ├─► YES: Use direct interface call
│   │       │   └─► systemB->doSomething()
│   │       │
│   │       └─► NO: Use EventSystem (better decoupling)
│   │
│   └─► NO: Is it a request/response pattern?
│       │
│       ├─► YES: Use direct interface call with return value
│       │   └─► auto result = systemB->query()
│       │
│       └─► NO: Use EventSystem for async notifications
│
├─► Does the receiver need to unsubscribe later?
│   │
│   ├─► YES: Store SubscriptionId, call unsubscribe() in cleanup
│   │   │
│   │   │   SubscriptionId id = events->subscribe(Events::Collision, callback);
│   │   │   // Later...
│   │   │   events->unsubscribe(id);
│   │
│   └─► NO: Anonymous subscription is fine
│
└─► Is it a built-in physics/asset/state event?
    └─► Use predefined event constants in Events namespace
```

### Communication Patterns

| Scenario | Pattern | Code |
|----------|---------|------|
| Player died (many listeners) | Event | `events->publish(Events::PlayerDeath, {})` |
| Get player position | Direct call | `physics->getPosition(player)` |
| Asset loaded notification | Event | Subscribe to `Events::AssetLoaded` |
| UI button clicked | Event | Custom event type |
| Physics collision | Built-in event | Subscribe to `Events::Collision` |
| Config changed | Built-in event | Subscribe to `Events::ConfigChanged` |

---

## 5. How Should I Handle This Input?

```
START: I need to respond to player input
│
├─► Is it movement (continuous)?
│   │
│   ├─► YES: Use action mapping (recommended)
│   │   │
│   │   │   // Setup once
│   │   │   input->registerMapping({
│   │   │       .binding = {.deviceType = Keyboard, .keyCode = 44},  // Comma
│   │   │       .action = "move_up"
│   │   │   });
│   │   │
│   │   │   // Check every frame
│   │   │   if (input->isActionActive("move_up")) { moveUp(); }
│   │   │
│   │   └─► Supports rebinding, multiple devices, analog values
│   │
│   └─► NO: Is it a one-shot action (jump, attack)?
│       │
│       ├─► YES: Use wasActionJustPressed()
│       │   └─► Only triggers once per press
│       │
│       └─► NO: Is it a release trigger?
│           └─► Use wasActionJustReleased()
│
├─► Should it support multiple input devices?
│   │
│   ├─► YES: Use action mapping (required)
│   │   │
│   │   │   input->registerMapping({
│   │   │       .binding = {.deviceType = Keyboard, .keyCode = 32},
│   │   │       .action = "jump"
│   │   │   });
│   │   │   input->registerMapping({
│   │   │       .binding = {.deviceType = Gamepad, .buttonCode = 0},
│   │   │       .action = "jump"
│   │   │   });
│   │   │
│   │   └─► Player can rebind via UI
│   │
│   └─► NO: Raw input is OK for prototyping (not recommended)
│
├─► Is it for UI/menus?
│   │
│   ├─► YES: Let UI system handle it
│   │   └─► RmlUi captures input automatically
│   │
│   └─► NO: Is it mouse-based (clicking in world)?
│       └─► Convert screen to world coordinates
│           └─► camera->screenToWorld(mousePos)
│
└─► Does it need input buffering?
    │
    ├─► YES: Store input in buffer, check within time window
    │   └─► Useful for fighting games, responsive platformers
    │
    └─► NO: Process immediately
```

### Input Patterns

```cpp
// Dvorak-friendly movement (,AOE) - Direct input (prototyping only)
// NOTE: Prefer action mapping for production
Vec2 getMovementInput() {
    Vec2 movement{0, 0};
    // Using raw mouse button state since no key enum exists yet
    if (input->isMouseButtonDown(0)) movement.y -= 1;  // Up
    // Would use Key enum when available
    return movement;
}

// Action-based (RECOMMENDED for production)
void setupInputActions() {
    // Dvorak movement: ,AOE
    input->registerMapping({
        .binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 44},
        .action = "move_up"
    });
    input->registerMapping({
        .binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 30},
        .action = "move_left"
    });
    input->registerMapping({
        .binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 24},
        .action = "move_down"
    });
    input->registerMapping({
        .binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = 18},
        .action = "move_right"
    });

    // Gamepad support
    input->registerMapping({
        .binding = {.deviceType = InputDeviceType::Gamepad, .buttonCode = 0},
        .action = "jump"
    });
}

void handleInput() {
    // Movement (continuous)
    if (input->isActionActive("move_up")) moveUp();

    // Jump (one-shot)
    if (input->wasActionJustPressed("jump")) jump();

    // Analog stick
    float moveX = input->getActionValue("move_right") -
                  input->getActionValue("move_left");
}

// Mouse in world
Vec2 mousePos = input->getMousePosition();
Vec2 worldPos = camera->screenToWorld(mousePos);
if (input->isMouseButtonDown(0)) {  // Left click
    shootAt(worldPos);
}
```

---

## 6. How Should I Implement This Entity?

```
START: I need to create a new entity type
│
├─► Is it defined by data (position, size, sprite)?
│   │
│   ├─► YES: Create a blueprint in Lua (PREFERRED)
│   │   │
│   │   │   -- data/blueprints/my_entity.lua
│   │   │   return {
│   │   │       transform = { x = 0, y = 0 },
│   │   │       sprite = { texture = "..." },
│   │   │       physics = { type = "dynamic" }
│   │   │   }
│   │   │
│   │   └─► Spawn with: blueprints->createEntity("my_entity")
│   │
│   └─► NO: Does it share properties with existing entities?
│       │
│       ├─► YES: Use blueprint inheritance
│       │   │
│       │   │   return {
│       │   │       extends = "base_enemy",
│       │   │       health = 50  -- Override
│       │   │   }
│       │   │
│       │   └─► Reuse base, override differences
│       │
│       └─► NO: Create new blueprint from scratch
│
├─► Does it need custom behavior (not just data)?
│   │
│   ├─► YES: Is the behavior simple (patrol, follow)?
│   │   │
│   │   ├─► YES: Use AI system with behavior tree
│   │   │   └─► ai->attachBehaviorTree(entity, "patrol")
│   │   │
│   │   └─► NO: Create a game system to handle it
│   │       └─► Iterate entities with specific components
│   │
│   └─► NO: Blueprint + physics is enough
│
├─► Does it need physics?
│   │
│   ├─► YES: Add physics section to blueprint
│   │   │
│   │   │   physics = {
│   │   │       type = "dynamic",  -- or "static", "kinematic"
│   │   │       shape = { type = "box", width = 32, height = 32 }
│   │   │   }
│   │   │
│   │   └─► Physics body created automatically
│   │
│   └─► NO: Omit physics section
│
└─► Does it need to be saved/loaded?
    │
    ├─► YES: Save entity state in SaveData
    │
    └─► NO: Recreate from level/blueprint on load
```

### Entity Creation Patterns

```cpp
// From blueprint (preferred)
Entity player = blueprints->createEntity("player");

// Manual creation (rare - when blueprints not available)
Entity e = entities->createEntity();
entities->emplace<Transform2D>(e, 100.0f, 200.0f);
entities->emplace<Velocity>(e, 0.0f, 0.0f);

// Iterate entities with specific components
auto view = entities->view<Transform2D, Velocity>();
for (auto entity : view) {
    auto& transform = entities->get<Transform2D>(entity);
    auto& velocity = entities->get<Velocity>(entity);
    // Update logic...
}
```

---

## 7. How Should This Collision Be Handled?

```
START: Two entities collided
│
├─► Is it a physical collision (should they bounce/block)?
│   │
│   ├─► YES: Let physics handle it naturally
│   │   └─► Bodies will collide and respond
│   │
│   └─► NO: Is it a trigger/sensor (detection only)?
│       └─► Use sensor in physics definition
│           │
│           │   physics->setSensor(entity, true)
│           │
│           └─► Subscribe to trigger events
│
├─► What should happen on collision?
│   │
│   ├─► Damage: Apply damage via GAS or direct health modification
│   │   └─► gas->applyEffect(target, "damage_effect")
│   │
│   ├─► Collect: Destroy collectible, add to inventory
│   │   └─► entities->destroyEntity(collectible)
│   │
│   ├─► Trigger: Activate something (door, checkpoint)
│   │   └─► events->publish(Events::Checkpoint, {})
│   │
│   └─► Push: Apply force/impulse
│       └─► physics->applyImpulse(entity, force)
│
├─► Do I need to filter collisions?
│   │
│   ├─► YES: Use collision layers/masks
│   │   │
│   │   │   physics->setCollisionLayer(entity, Layers::Player);
│   │   │   physics->setCollisionMask(entity, Layers::Enemy | Layers::Ground);
│   │   │
│   │   └─► Only collides with specified layers
│   │
│   └─► NO: Default collision (everything collides)
│
└─► How do I detect the collision?
    │
    ├─► Subscribe to collision events
    │   │
    │   │   events->subscribe(Events::Collision, [](const EventData& data) {
    │   │       // Handle collision
    │   │   });
    │   │
    │   └─► Check entity types in handler using components
    │
    └─► Use component queries to identify entities
        └─► entities->allOf<PlayerTag>(entity)
```

### Collision Patterns

```cpp
// Collision handler
void setupCollisionHandling() {
    events->subscribe(Events::Collision, [this](const EventData& data) {
        // Extract collision info from EventData variant
        // Implementation depends on event data structure

        // Example pattern:
        Entity entityA = /* extract from data */;
        Entity entityB = /* extract from data */;

        // Player hit enemy
        if (entities->allOf<PlayerTag>(entityA) &&
            entities->allOf<EnemyTag>(entityB)) {
            damagePlayer(entityA);
        }

        // Player collected coin
        if (entities->allOf<PlayerTag>(entityA) &&
            entities->allOf<CoinTag>(entityB)) {
            collectCoin(entityB);
            entities->destroyEntity(entityB);
        }
    });
}

// Raycast for ground detection
auto hit = physics->raycast(
    playerPos,                          // Origin
    {0, 1},                             // Direction (down)
    50.0f,                              // Max distance
    CollisionMask::Ground               // What to hit
);

if (hit && hit->distance < 5.0f) {
    isGrounded = true;
}
```

---

## 8. Where Should This Configuration Live?

```
START: I have a value that might need tuning
│
├─► Is it a constant that never changes?
│   │
│   ├─► YES: Use constexpr in C++
│   │   └─► constexpr float GRAVITY = -980.0f;
│   │
│   └─► NO: Will it change during development?
│       │
│       ├─► YES: Put in Lua config (RECOMMENDED)
│       │   │
│       │   │   -- data/config/physics.lua
│       │   │   return {
│       │   │       gravity = -980,
│       │   │       player_speed = 200
│       │   │   }
│       │   │
│       │   └─► Enables hot reload for tuning
│       │
│       └─► NO: Put in Lua config anyway (future-proofing)
│
├─► Is it per-entity configuration?
│   │
│   ├─► YES: Put in blueprint
│   │   │
│   │   │   return {
│   │   │       speed = 200,
│   │   │       health = 100
│   │   │   }
│   │   │
│   │   └─► Access via blueprint properties
│   │
│   └─► NO: Is it global game configuration?
│       └─► Put in data/config/game.lua
│
├─► Is it user-changeable (settings)?
│   │
│   ├─► YES: Use SaveSystem for persistence
│   │   └─► save->save(SETTINGS_SLOT, userSettings)
│   │
│   └─► NO: Use ConfigSystem (read-only from Lua)
│
└─► Should designers be able to change it?
    │
    ├─► YES: Must be in Lua (they can edit without code)
    │
    └─► NO: Can be in C++ or Lua
```

### Configuration Patterns

```lua
-- data/config/game.lua (global settings)
return {
    physics = {
        gravity = -980,
        fixed_timestep = 1/60
    },
    player = {
        speed = 200,
        jump_force = 400,
        coyote_time = 0.1
    },
    camera = {
        follow_smoothing = 5.0,
        shake_decay = 3.0
    }
}
```

```cpp
// Access in code (when ConfigSystem available)
float speed = config->get<float>("player.speed");
float gravity = config->get<float>("physics.gravity");
```

---

## 9. How Should I Render This?

```
START: I need to display something
│
├─► Is it a game entity (player, enemy, item)?
│   │
│   ├─► YES: Add sprite component to blueprint
│   │   │
│   │   │   sprite = {
│   │   │       texture = "textures/player.png",
│   │   │       width = 32, height = 48,
│   │   │       layer = RenderLayers.Entities
│   │   │   }
│   │   │
│   │   └─► Graphics system renders automatically
│   │
│   └─► NO: Is it UI (health bar, score, menu)?
│       │
│       ├─► YES: Use UI System (RmlUi)
│       │   └─► Create .rml document and .rcss stylesheet
│       │
│       └─► NO: Is it a background/tilemap?
│           │
│           ├─► YES: Use lower render layer
│           │   └─► layer = RenderLayers.Background
│           │
│           └─► NO: Is it debug visualization?
│               └─► Use graphics->drawDebug*() methods
│
├─► What render layer should it use?
│   │
│   │   Layer Order (back to front):
│   │   0: Background
│   │   10: Tiles
│   │   20: Entities
│   │   30: Player
│   │   40: Foreground
│   │   50: Particles
│   │   100: UI
│   │
│   └─► Choose based on visual order
│
├─► Does it need custom shaders?
│   │
│   ├─► YES: Create material in Lua
│   │   │
│   │   │   -- data/materials/glow.lua
│   │   │   return {
│   │   │       shader = "shaders/glow",
│   │   │       uniforms = {
│   │   │           glow_color = {1, 0.5, 0},
│   │   │           glow_intensity = 2.0
│   │   │       }
│   │   │   }
│   │   │
│   │   └─► Assign to sprite: material = "glow"
│   │
│   └─► NO: Default rendering is fine
│
└─► Is it animated?
    │
    ├─► YES: Use sprite sheet + animation component
    │   │
    │   │   animation = {
    │   │       frames = 4,
    │   │       frameTime = 0.1,
    │   │       loop = true
    │   │   }
    │   │
    │   └─► Or use animated sprite component
    │
    └─► NO: Static sprite is fine
```

### Rendering Patterns

```lua
-- Blueprint with layered rendering
return {
    sprite = {
        texture = "textures/player.png",
        width = 32,
        height = 48,
        layer = 30,  -- Above entities, below UI
        material = "default"  -- Or custom material
    }
}
```

---

## 10. How Should I Save This?

```
START: I need to persist data
│
├─► Is it player progress (score, level, items)?
│   │
│   ├─► YES: Create SaveData struct
│   │   │
│   │   │   struct SaveData {
│   │   │       int level;
│   │   │       int score;
│   │   │       std::vector<std::string> inventory;
│   │   │
│   │   │       template<class Archive>
│   │   │       void serialize(Archive& ar) {
│   │   │           ar(level, score, inventory);
│   │   │       }
│   │   │   };
│   │   │
│   │   └─► save->save(slot, saveData)
│   │
│   └─► NO: Is it user settings (volume, controls)?
│       │
│       ├─► YES: Save to settings slot
│       │   └─► save->save(SETTINGS_SLOT, settings)
│       │
│       └─► NO: Is it auto-save (checkpoint)?
│           └─► Save to AUTO_SAVE slot automatically
│
├─► What should I save vs derive?
│   │
│   │   SAVE:
│   │   - Player position (if mid-level)
│   │   - Collected items
│   │   - Unlocked levels
│   │   - Quest progress
│   │   - Settings
│   │
│   │   DERIVE (don't save):
│   │   - Current health (derive from max - damage taken)
│   │   - Entity positions (respawn from level)
│   │   - Cached calculations
│   │   - UI state
│   │
│   └─► Only save what's needed to recreate state
│
├─► When should I save?
│   │
│   ├─► Checkpoint reached → Auto-save
│   ├─► Level completed → Manual save
│   ├─► Player requests → Quick save or slot save
│   └─► Periodic interval → Auto-save (optional)
│
└─► How do I handle save corruption?
    │
    ├─► Use std::expected return type
    │   │
    │   │   auto result = save->load<SaveData>(slot);
    │   │   if (!result) {
    │   │       handleLoadError(result.error());
    │   │   }
    │   │
    │   └─► Fallback to default or previous save
    │
    └─► Keep backup of last good save
```

### Save Patterns

```cpp
// Save game
void Game::saveGame(uint32_t slot) {
    SaveData data{
        .level = currentLevel_,
        .score = score_,
        .inventory = inventory_
    };

    auto result = save->save(slot, data);
    if (!result) {
        showError("Failed to save game");
    }
}

// Load game
void Game::loadGame(uint32_t slot) {
    auto result = save->load<SaveData>(slot);
    if (result) {
        currentLevel_ = result->level;
        score_ = result->score;
        inventory_ = result->inventory;
        loadLevel(currentLevel_);
    } else {
        // Start new game or show error
    }
}
```

---

## 11. How Should I Structure This UI?

```
START: I need to create a UI element
│
├─► Is it a full screen (menu, inventory)?
│   │
│   ├─► YES: Create a document (.rml file)
│   │   │
│   │   │   <!-- data/ui/main_menu.rml -->
│   │   │   <rml>
│   │   │   <head>
│   │   │       <link type="text/rcss" href="styles/menu.rcss"/>
│   │   │   </head>
│   │   │   <body>
│   │   │       <!-- Content -->
│   │   │   </body>
│   │   │   </rml>
│   │   │
│   │   └─► Load with ui->loadDocument("ui/main_menu.rml")
│   │
│   └─► NO: Is it an overlay (HUD, notifications)?
│       │
│       ├─► YES: Create document that doesn't block input
│       │   └─► Show alongside game, update dynamically
│       │
│       └─► NO: Is it a popup (dialog, tooltip)?
│           └─► Create document, show/hide as needed
│
├─► How should I style it?
│   │
│   ├─► Create .rcss stylesheet
│   │   │
│   │   │   /* data/ui/styles/menu.rcss */
│   │   │   body {
│   │   │       background-color: #000000aa;
│   │   │   }
│   │   │   button {
│   │   │       background-color: #444;
│   │   │       padding: 10px 20px;
│   │   │   }
│   │   │   button:hover {
│   │   │       background-color: #666;
│   │   │   }
│   │   │
│   │   └─► Link in document head
│   │
│   └─► Use classes for reusable styles
│
├─► How should I handle events?
│   │
│   ├─► Bind in C++ after loading (when UI system available)
│   │   │
│   │   │   auto btn = ui->getElementById(doc, "start-btn");
│   │   │   ui->bindEvent(btn, "click", [this]() {
│   │   │       startGame();
│   │   │   });
│   │   │
│   │   └─► Use lambda for inline handling
│   │
│   └─► Or use data binding for dynamic content
│
└─► How should I update dynamic content?
    │
    ├─► Simple text: ui->setInnerText(element, value)
    ├─► Attributes: ui->setAttribute(element, "width", value)
    ├─► Classes: ui->addClass/removeClass(element, "active")
    └─► Complex: Use data binding model
```

### UI Patterns

```cpp
// Load and setup menu (when UI system available)
void Game::showMainMenu() {
    menuDoc_ = ui->loadDocument("ui/main_menu.rml");

    ui->bindEvent(ui->getElementById(menuDoc_, "play-btn"), "click", [this]() {
        ui->hideDocument(menuDoc_);
        startGame();
    });

    ui->bindEvent(ui->getElementById(menuDoc_, "quit-btn"), "click", [this]() {
        shouldQuit_ = true;
    });

    ui->showDocument(menuDoc_);
}

// Update HUD
void Game::updateHUD() {
    ui->setInnerText(hudDoc_, "#score", std::to_string(score_));

    float healthPercent = (float)health_ / maxHealth_ * 100;
    ui->setAttribute(hudDoc_, "#health-bar", "style",
        "width: " + std::to_string(healthPercent) + "%");
}
```

---

## 12. How Should I Implement This AI?

```
START: I need an entity to have AI behavior
│
├─► How complex is the behavior?
│   │
│   ├─► Simple (patrol, follow, flee)
│   │   │
│   │   ├─► Use built-in AI steering behaviors
│   │   │   │
│   │   │   │   ai->setNavigationTarget(entity, targetPos);
│   │   │   │   ai->setPatrolPath(entity, waypoints);
│   │   │   │
│   │   │   └─► AI system handles movement
│   │   │
│   │   └─► Or simple state machine in game code
│   │
│   ├─► Medium (state-based: idle → chase → attack)
│   │   │
│   │   ├─► Use behavior tree
│   │   │   └─► ai->attachBehaviorTree(entity, "enemy_ai")
│   │   │
│   │   └─► Or explicit state machine
│   │
│   └─► Complex (planning, learning)
│       └─► Custom AI system with behavior trees
│
├─► Does it need pathfinding?
│   │
│   ├─► YES: Use navigation system
│   │   │
│   │   │   // Load navmesh for level
│   │   │   ai->loadNavMesh("levels/level1_nav.bin");
│   │   │
│   │   │   // Find path
│   │   │   auto path = ai->findPath(entity, targetPos);
│   │   │
│   │   └─► AI follows path automatically
│   │
│   └─► NO: Direct movement is fine
│
├─► Does it need to share data (blackboard)?
│   │
│   ├─► YES: Use blackboard for behavior tree
│   │   │
│   │   │   ai->setBlackboardValue(entity, "target", playerEntity);
│   │   │   ai->setBlackboardValue(entity, "alert_level", 0.5f);
│   │   │
│   │   └─► Behavior tree nodes read/write blackboard
│   │
│   └─► NO: Local state is sufficient
│
└─► Does it need spatial awareness?
    │
    ├─► YES: Use physics queries or AI queries
    │   │
    │   │   auto nearby = physics->queryCircle(pos, radius);
    │   │   // Check line of sight with raycast
    │   │   auto hit = physics->raycast(aiPos, direction, range);
    │   │
    │   └─► Make decisions based on awareness
    │
    └─► NO: Simple stimulus-response is enough
```

### AI Patterns

```cpp
// Simple patrol (when AI system available)
ai->setPatrolPath(enemy, {
    {100, 200}, {300, 200}, {300, 400}, {100, 400}
});

// State machine
void EnemyAI::update(float dt) {
    switch (state_) {
        case State::Patrol:
            // Check line of sight with raycast
            auto hit = physics->raycast(position, toPlayer, viewRange);
            if (hit && hit->entity == player) {
                state_ = State::Chase;
            }
            break;

        case State::Chase:
            moveTowards(playerPos);
            if (distance < attackRange) {
                state_ = State::Attack;
            }
            break;

        case State::Attack:
            attack();
            state_ = State::Chase;
            break;
    }
}
```

---

## 13. How Should I Handle This Error?

```
START: An operation might fail
│
├─► Is it a system operation (load, save, network)?
│   │
│   ├─► YES: Use std::expected return value
│   │   │
│   │   │   auto result = save->load<Data>(slot);
│   │   │   if (!result) {
│   │   │       // Handle error
│   │   │       auto error = result.error();
│   │   │   }
│   │   │
│   │   └─► Never ignore return values
│   │
│   └─► NO: Is it a validation error (bad input)?
│       │
│       ├─► YES: Validate early, fail fast
│       │   │
│       │   │   if (slot >= MAX_SLOTS) {
│       │   │       // Log error
│       │   │       return;
│       │   │   }
│       │   │
│       │   └─► Log and return early
│       │
│       └─► NO: Is it recoverable?
│           │
│           ├─► YES: Use fallback behavior
│           │   │
│           │   │   auto texture = assets->getAsset<TextureData>(handle);
│           │   │   if (!texture) {
│           │   │       texture = getDefaultTexture();  // Fallback
│           │   │   }
│           │   │
│           │   └─► Continue with default
│           │
│           └─► NO: Log error and stop operation
│
├─► Should the user see this error?
│   │
│   ├─► YES: Show UI notification or dialog
│   │   └─► "Failed to load save file"
│   │
│   └─► NO: Log for debugging only
│
└─► Should this crash in debug builds?
    │
    ├─► YES: Use assert for programming errors
    │   │
    │   │   assert(entity != Entity::Invalid && "Invalid entity");
    │   │
    │   └─► Helps catch bugs during development
    │
    └─► NO: Handle gracefully in all builds
```

### Error Handling Patterns

```cpp
// System operation with expected
auto result = save->load<SaveData>(slot);
if (result) {
    loadFromSave(*result);
} else {
    // Handle error based on error code
    handleSaveError(result.error());
}

// Validation
void setHealth(int value) {
    if (value < 0 || value > maxHealth_) {
        value = std::clamp(value, 0, maxHealth_);
    }
    health_ = value;
}

// Fallback
const TextureData* getTexture(AssetHandle handle) {
    if (auto tex = assets->getAsset<TextureData>(handle)) {
        return tex;
    }
    return &defaultTexture_;  // Never return null
}
```

---

## 14. Performance Decision Tree

```
START: Something is slow
│
├─► Is it asset loading?
│   │
│   ├─► YES: Are assets loaded synchronously?
│   │   │
│   │   ├─► YES: Switch to async loading
│   │   │   └─► assets->loadAssetAsync(handle, callback)
│   │   │
│   │   └─► NO: Are too many assets loading at once?
│   │       └─► Stagger loads, use loading screen
│   │
│   └─► NO: Is it rendering?
│       │
│       ├─► YES: Too many draw calls?
│       │   │
│       │   ├─► YES: Use batching
│       │   │   │
│       │   │   │   graphics->beginBatch();
│       │   │   │   for (auto& sprite : sprites) {
│       │   │   │       graphics->drawSpriteBatched(sprite);
│       │   │   │   }
│       │   │   │   graphics->endBatch();
│       │   │   │
│       │   │   └─► Single draw call for many sprites
│       │   │
│       │   └─► NO: Too many entities on screen?
│       │       └─► Implement frustum culling
│       │           └─► Only render visible entities
│       │
│       └─► NO: Is it physics?
│           │
│           ├─► YES: Too many physics bodies?
│           │   │
│           │   ├─► YES: Use simpler collision shapes
│           │   ├─► YES: Sleep inactive bodies
│           │   └─► YES: Reduce physics iterations
│           │
│           └─► NO: Is it game logic?
│               │
│               ├─► YES: Are you iterating all entities?
│               │   │
│               │   ├─► YES: Use view<>() with specific components
│               │   │   │
│               │   │   │   auto view = entities->view<Transform2D, Velocity>();
│               │   │   │   // Only iterates relevant entities
│               │   │   │
│               │   │   └─► Much faster than checking all entities
│               │   │
│               │   └─► NO: Cache expensive calculations
│               │
│               └─► NO: Profile to find bottleneck
│                   └─► Use Tracy profiler
│
├─► General optimizations:
│   │
│   ├─► Use object pooling for frequent create/destroy
│   ├─► Cache query results (entity views, spatial queries)
│   ├─► Use spatial partitioning for collision checks
│   ├─► Reduce string operations in hot paths
│   └─► Use fixed timestep for physics (already done)
│
└─► Measurement:
    │
    ├─► Profile before optimizing
    ├─► Measure impact of changes
    └─► Don't optimize what isn't slow
```

### Performance Patterns

```cpp
// Batched rendering (when batching API available)
void renderSprites() {
    graphics->beginBatch();
    auto view = entities->view<Sprite, Transform2D>();
    for (auto entity : view) {
        auto& sprite = entities->get<Sprite>(entity);
        auto& transform = entities->get<Transform2D>(entity);
        graphics->drawSpriteBatched(sprite, transform);
    }
    graphics->endBatch();  // Single draw call
}

// Efficient entity iteration
void updateEnemies(float dt) {
    // Only iterates entities with BOTH EnemyTag AND Transform2D
    auto view = entities->view<EnemyTag, Transform2D>();
    for (auto entity : view) {
        // Process enemy...
    }
}

// Object pooling
class BulletPool {
    std::vector<Entity> pool_;

    Entity acquire() {
        if (!pool_.empty()) {
            Entity e = pool_.back();
            pool_.pop_back();
            return e;
        }
        return createNewBullet();
    }

    void release(Entity e) {
        deactivate(e);
        pool_.push_back(e);
    }
};
```

---

## Quick Decision Matrix

| Question | Answer | Action |
|----------|--------|--------|
| Where does data go? | Entity-specific → Component, Global → Game class, Persistent → SaveSystem |
| Where does config go? | Lua file in `data/config/` |
| How to load files? | Always through AssetSystem (except SaveSystem) |
| How to communicate? | Events for broadcast (publish/queue), direct calls for queries |
| How to create entities? | Blueprints in Lua, spawn from C++ |
| How to handle collisions? | Subscribe to collision events, check component tags |
| How to render? | Add sprite to blueprint, graphics system auto-renders |
| How to save? | Create SaveData struct with serialize(), use SaveSystem |
| How to do UI? | RML documents + RCSS styles (when UI system available) |
| How to do AI? | Behavior trees or state machines |
| How to handle errors? | Use std::expected, fallbacks, logging |
| How to optimize? | Profile first, batch draws, cache queries, pool objects |

---

## Checklist: Before Implementing Any Feature

- [ ] Is this in the right place? (Lua vs C++, which file?)
- [ ] Am I using the correct system? (Assets, Config, Events, etc.)
- [ ] Am I following Bestow conventions? (Interfaces, DI, ECS)
- [ ] Does this need hot reload support?
- [ ] How will errors be handled? (std::expected)
- [ ] Is this testable?
- [ ] Is this performant? (No premature optimization, but don't be wasteful)
- [ ] Will a designer be able to tune this? (Put values in Lua if yes)
- [ ] Am I depending only on interfaces, not implementations?
- [ ] Am I using AssetSystem for all file I/O (except SaveSystem)?

---

## Key Architectural Rules

1. **Contract-Based**: Depend only on `ISystemName` interfaces, never implementations
2. **AssetSystem Gateway**: ALL file reading goes through AssetSystem (except SaveSystem)
3. **Lua-First**: Game data, config, blueprints, levels in Lua; C++ for performance/engine
4. **EventSystem Decoupling**: Use events for cross-system notifications
5. **ECS Composition**: Components are data, systems are logic, entities are IDs
6. **std::expected**: No exceptions, return expected<T, E> for failable operations
7. **Dependency Injection**: Systems receive dependencies via constructor (Kangaru DI)
8. **Hot Reload**: Subscribe to asset changes for live editing
9. **Dvorak-Friendly**: Default to ,AOE for movement
10. **Vulkan-First**: Vulkan is primary, OpenGL is fallback
