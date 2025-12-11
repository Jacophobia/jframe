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
│   │   │   └─► Access via entities_->get<T>(entity)
│   │   │
│   │   └─► NO: Add component at runtime when needed
│   │       └─► entities_->emplace<T>(entity, ...)
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
│   │   │   └─► config_->getFloat("player.speed")
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
| Entity properties | Component | `entities_->get<T>()` |
| Entity tags | Tag component | `entities_->has<T>()` |
| Global game state | Game class member | Direct access |
| Persistent progress | SaveSystem | `save_->save()/load()` |
| Tunable values | ConfigSystem | `config_->getFloat()` |
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
│   │       └─► Never use std::ifstream, lua.script_file(), etc.
│   │
│   └─► NO: Good, continue...
│
├─► Is it needed immediately at startup?
│   │
│   ├─► YES: Use synchronous loading
│   │   └─► assets_->loadAsset(handle)
│   │
│   └─► NO: Can the game continue without it?
│       │
│       ├─► YES: Use async loading
│       │   └─► assets_->loadAssetAsync(handle, callback)
│       │
│       └─► NO: Load sync or show loading screen
│
├─► Will I need hot reload during development?
│   │
│   ├─► YES: Subscribe to changes
│   │   └─► assets_->subscribe(handle, callback)
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
    └─► Material → AssetType::Material → MaterialData
```

### Asset Loading Patterns

```cpp
// Pattern 1: Sync load at startup
AssetHandle tex = assets_->registerAsset(AssetType::Texture, "textures/player.png");
assets_->loadAsset(tex);  // Blocks until loaded

// Pattern 2: Async load with callback
assets_->loadAssetAsync(tex, [this](AssetHandle h, AssetState state) {
    if (state == AssetState::Loaded) {
        onTextureReady(h);
    }
});

// Pattern 3: Hot reload subscription
assets_->subscribe(tex, [this](AssetHandle h, AssetType t) {
    refreshSprite(h);  // Called when file changes
});
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
│   │   │   └─► events_->emit(EventType, data)
│   │   │
│   │   └─► NO: Is it performance-critical?
│   │       │
│   │       ├─► YES: Use direct interface call
│   │       │   └─► systemB_->doSomething()
│   │       │
│   │       └─► NO: Use EventSystem (better decoupling)
│   │
│   └─► NO: Is it a request/response pattern?
│       │
│       ├─► YES: Use direct interface call with return value
│       │   └─► auto result = systemB_->query()
│       │
│       └─► NO: Is it deferred (process later)?
│           └─► Use events_->emitDeferred() + processDeferred()
│
├─► Does the receiver need to unsubscribe later?
│   │
│   ├─► YES: Store SubscriptionId, call unsubscribe() in cleanup
│   │
│   └─► NO: Anonymous subscription is fine
│
└─► Is it a collision/physics event?
    └─► Use built-in physics events (CollisionBegin, etc.)
```

### Communication Patterns

| Scenario | Pattern | Code |
|----------|---------|------|
| Player died (many listeners) | Event | `events_->emit(Events::PlayerDied, {})` |
| Get player position | Direct call | `physics_->getPosition(player)` |
| Asset loaded notification | Event | Subscribe to `Events::AssetLoaded` |
| UI button clicked | Event | Custom event type |
| Physics collision | Built-in event | Subscribe to `Events::CollisionBegin` |

---

## 5. How Should I Handle This Input?

```
START: I need to respond to player input
│
├─► Is it movement (continuous)?
│   │
│   ├─► YES: Use isKeyHeld() or isActionHeld()
│   │   └─► Check every frame in update()
│   │
│   └─► NO: Is it a one-shot action (jump, attack)?
│       │
│       ├─► YES: Use isKeyJustPressed() or isActionJustPressed()
│       │   └─► Only triggers once per press
│       │
│       └─► NO: Is it a release trigger?
│           └─► Use isKeyJustReleased()
│
├─► Should it support multiple input devices?
│   │
│   ├─► YES: Use action mapping
│   │   │
│   │   │   // Setup
│   │   │   input_->bindAction("jump", Key::Space);
│   │   │   input_->bindAction("jump", GamepadButton::A);
│   │   │
│   │   │   // Usage
│   │   │   if (input_->isActionJustPressed("jump")) { ... }
│   │   │
│   │   └─► Player can rebind via UI
│   │
│   └─► NO: Use direct key checks (prototyping only)
│
├─► Is it for UI/menus?
│   │
│   ├─► YES: Let UI system handle it
│   │   └─► RmlUi captures input automatically
│   │
│   └─► NO: Is it mouse-based (clicking in world)?
│       └─► Convert screen to world coordinates
│           └─► camera_->screenToWorld(mouseX, mouseY)
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
// Dvorak-friendly movement (,AOE)
if (input_->isKeyHeld(Key::Comma)) moveUp();    // Dvorak W
if (input_->isKeyHeld(Key::A)) moveLeft();
if (input_->isKeyHeld(Key::O)) moveDown();      // Dvorak S
if (input_->isKeyHeld(Key::E)) moveRight();     // Dvorak D

// Action-based (recommended)
if (input_->isActionJustPressed("jump")) jump();
if (input_->isActionHeld("fire")) continuousFire();

// Mouse in world
auto [wx, wy] = camera_->screenToWorld(input_->getMouseX(), input_->getMouseY());
```

---

## 6. How Should I Implement This Entity?

```
START: I need to create a new entity type
│
├─► Is it defined by data (position, size, sprite)?
│   │
│   ├─► YES: Create a blueprint in Lua
│   │   │
│   │   │   -- data/blueprints/my_entity.lua
│   │   │   return {
│   │   │       transform = { x = 0, y = 0 },
│   │   │       sprite = { texture = "..." },
│   │   │       physics = { type = "dynamic" }
│   │   │   }
│   │   │
│   │   └─► Spawn with: blueprints_->createEntity("my_entity")
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
│   │   │   └─► ai_->attachBehaviorTree(entity, "patrol")
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
    ├─► YES: Implement ISaveable or save entity state
    │
    └─► NO: Recreate from level/blueprint on load
```

### Entity Creation Patterns

```cpp
// From blueprint (preferred)
Entity player = blueprints_->createEntity("player");

// From blueprint with position
Entity enemy = blueprints_->createEntityAt("enemy", 500, 300);

// From blueprint with overrides
Entity boss = blueprints_->createEntity("enemy", {
    {"health", 500},
    {"scale", 2.0f}
});

// Manual creation (rare)
Entity e = entities_->createEntity();
entities_->emplace<Transform2D>(e, 100, 200);
entities_->emplace<EnemyTag>(e);
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
│           │   physics = {
│           │       shape = { ... },
│           │       isSensor = true  -- No physical response
│           │   }
│           │
│           └─► Subscribe to trigger events
│
├─► What should happen on collision?
│   │
│   ├─► Damage: Apply damage via GAS or direct health modification
│   │   └─► gas_->applyEffect(target, "damage_effect")
│   │
│   ├─► Collect: Destroy collectible, add to inventory
│   │   └─► entities_->destroyEntity(collectible)
│   │
│   ├─► Trigger: Activate something (door, checkpoint)
│   │   └─► events_->emit(GameEvents::CheckpointReached, {})
│   │
│   └─► Push: Apply force/impulse
│       └─► physics_->applyImpulse(entity, force)
│
├─► Do I need to filter collisions?
│   │
│   ├─► YES: Use collision layers/masks
│   │   │
│   │   │   physics = {
│   │   │       collisionLayer = Layers.Player,
│   │   │       collisionMask = Layers.Enemy | Layers.Ground
│   │   │   }
│   │   │
│   │   └─► Only collides with specified layers
│   │
│   └─► NO: Default collision (everything collides)
│
└─► How do I detect the collision?
    │
    ├─► Subscribe to collision events
    │   │
    │   │   events_->subscribe(Events::CollisionBegin, [](auto& data) {
    │   │       auto& c = std::get<CollisionEvent>(data);
    │   │       handleCollision(c.entityA, c.entityB);
    │   │   });
    │   │
    │   └─► Check entity types in handler
    │
    └─► Use component queries to identify entities
        └─► entities_->has<PlayerTag>(entity)
```

### Collision Patterns

```cpp
// Collision handler
events_->subscribe(Events::CollisionBegin, [this](const EventData& data) {
    auto& c = std::get<CollisionEvent>(data);

    // Player hit enemy
    if (entities_->has<PlayerTag>(c.entityA) &&
        entities_->has<EnemyTag>(c.entityB)) {
        damagePlayer(c.entityA);
    }

    // Player collected coin
    if (entities_->has<PlayerTag>(c.entityA) &&
        entities_->has<CoinTag>(c.entityB)) {
        collectCoin(c.entityB);
        entities_->destroyEntity(c.entityB);
    }
});
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
│       ├─► YES: Put in Lua config
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
│   │   └─► save_->save(SETTINGS_SLOT, userSettings)
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
// Access in code
float speed = config_->getFloat("player.speed");
float gravity = config_->getFloat("physics.gravity");
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
│               └─► Use graphics_->drawDebug*() methods
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
│   │   └─► save_->save(slot, saveData)
│   │
│   └─► NO: Is it user settings (volume, controls)?
│       │
│       ├─► YES: Save to settings slot
│       │   └─► save_->save(SETTINGS_SLOT, settings)
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
    ├─► Check load result for errors
    │   │
    │   │   auto result = save_->load<SaveData>(slot);
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

    auto result = save_->save(slot, data);
    if (!result) {
        showError("Failed to save game");
    }
}

// Load game
void Game::loadGame(uint32_t slot) {
    auto result = save_->load<SaveData>(slot);
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
│   │   └─► Load with ui_->loadDocument("ui/main_menu.rml")
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
│   ├─► Bind in C++ after loading
│   │   │
│   │   │   auto btn = ui_->getElementById(doc, "start-btn");
│   │   │   ui_->bindEvent(btn, "click", [this]() {
│   │   │       startGame();
│   │   │   });
│   │   │
│   │   └─► Use lambda for inline handling
│   │
│   └─► Or use data binding for dynamic content
│
└─► How should I update dynamic content?
    │
    ├─► Simple text: ui_->setInnerText(element, value)
    ├─► Attributes: ui_->setAttribute(element, "width", value)
    ├─► Classes: ui_->addClass/removeClass(element, "active")
    └─► Complex: Use data binding model
```

### UI Patterns

```cpp
// Load and setup menu
void Game::showMainMenu() {
    menuDoc_ = ui_->loadDocument("ui/main_menu.rml");

    ui_->bindEvent(ui_->getElementById(menuDoc_, "play-btn"), "click", [this]() {
        ui_->hideDocument(menuDoc_);
        startGame();
    });

    ui_->bindEvent(ui_->getElementById(menuDoc_, "quit-btn"), "click", [this]() {
        shouldQuit_ = true;
    });

    ui_->showDocument(menuDoc_);
}

// Update HUD
void Game::updateHUD() {
    ui_->setInnerText(hudDoc_, "#score", std::to_string(score_));

    float healthPercent = (float)health_ / maxHealth_ * 100;
    ui_->setAttribute(hudDoc_, "#health-bar", "style",
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
│   │   │   │   ai_->setNavigationTarget(entity, targetPos);
│   │   │   │   ai_->setPatrolPath(entity, waypoints);
│   │   │   │
│   │   │   └─► AI system handles movement
│   │   │
│   │   └─► Or simple state machine in game code
│   │
│   ├─► Medium (state-based: idle → chase → attack)
│   │   │
│   │   ├─► Use behavior tree
│   │   │   └─► ai_->attachBehaviorTree(entity, "enemy_ai")
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
│   │   │   ai_->loadNavMesh("levels/level1_nav.bin");
│   │   │
│   │   │   // Find path
│   │   │   auto path = ai_->findPath(entity, targetPos);
│   │   │
│   │   └─► AI follows path automatically
│   │
│   └─► NO: Direct movement is fine
│
├─► Does it need to share data (blackboard)?
│   │
│   ├─► YES: Use blackboard for behavior tree
│   │   │
│   │   │   ai_->setBlackboardValue(entity, "target", playerEntity);
│   │   │   ai_->setBlackboardValue(entity, "alert_level", 0.5f);
│   │   │
│   │   └─► Behavior tree nodes read/write blackboard
│   │
│   └─► NO: Local state is sufficient
│
└─► Does it need spatial awareness?
    │
    ├─► YES: Use AI queries
    │   │
    │   │   auto nearby = ai_->findEntitiesInRadius(pos, radius);
    │   │   bool canSee = ai_->hasLineOfSight(entity, target);
    │   │
    │   └─► Make decisions based on awareness
    │
    └─► NO: Simple stimulus-response is enough
```

### AI Patterns

```cpp
// Simple patrol
ai_->setPatrolPath(enemy, {
    {100, 200}, {300, 200}, {300, 400}, {100, 400}
});

// State machine
void EnemyAI::update(float dt) {
    switch (state_) {
        case State::Patrol:
            if (ai_->hasLineOfSight(entity_, player_)) {
                state_ = State::Chase;
            }
            break;

        case State::Chase:
            ai_->setNavigationTarget(entity_, playerPos);
            if (distance < attackRange_) {
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
│   ├─► YES: Check std::expected return value
│   │   │
│   │   │   auto result = save_->load<Data>(slot);
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
│       │   │       spdlog::error("Invalid slot: {}", slot);
│       │   │       return;
│       │   │   }
│       │   │
│       │   └─► Log and return/throw early
│       │
│       └─► NO: Is it recoverable?
│           │
│           ├─► YES: Use fallback behavior
│           │   │
│           │   │   auto texture = assets_->getAsset<TextureData>(handle);
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
│       └─► spdlog::error("Internal error: {}", msg)
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
auto result = save_->load<SaveData>(slot);
if (result) {
    loadFromSave(*result);
} else {
    switch (result.error()) {
        case SaveError::FileNotFound:
            startNewGame();
            break;
        case SaveError::CorruptedFile:
            showError("Save file corrupted");
            break;
        default:
            spdlog::error("Unknown save error");
    }
}

// Validation
void setHealth(int value) {
    if (value < 0 || value > maxHealth_) {
        spdlog::warn("Health out of range: {}", value);
        value = std::clamp(value, 0, maxHealth_);
    }
    health_ = value;
}

// Fallback
const TextureData* getTexture(AssetHandle handle) {
    if (auto tex = assets_->getAsset<TextureData>(handle)) {
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
│   │   │   └─► assets_->loadAssetAsync(handle, callback)
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
│       │   │   │   graphics_->beginBatch();
│       │   │   │   for (auto& sprite : sprites) {
│       │   │   │       graphics_->drawSpriteBatched(sprite);
│       │   │   │   }
│       │   │   │   graphics_->endBatch();
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
│               │   │   └─► Only iterates relevant entities
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
// Batched rendering
void renderSprites() {
    graphics_->beginBatch();
    for (auto [entity, sprite, transform] : entities_->view<Sprite, Transform2D>()) {
        graphics_->drawSpriteBatched(sprite, transform);
    }
    graphics_->endBatch();  // Single draw call
}

// Efficient entity iteration
void updateEnemies(float dt) {
    // Only iterates entities with BOTH EnemyTag AND Transform2D
    for (auto entity : entities_->view<EnemyTag, Transform2D>()) {
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
| How to communicate? | Events for broadcast, direct calls for queries |
| How to create entities? | Blueprints in Lua, spawn from C++ |
| How to handle collisions? | Subscribe to collision events, check component tags |
| How to render? | Add sprite to blueprint, graphics system auto-renders |
| How to save? | Create SaveData struct, use SaveSystem |
| How to do UI? | RML documents + RCSS styles |
| How to do AI? | Behavior trees or state machines |
| How to handle errors? | Check std::expected, use fallbacks, log for debug |
| How to optimize? | Profile first, batch draws, cache queries, pool objects |

---

## Checklist: Before Implementing Any Feature

- [ ] Is this in the right place? (Lua vs C++, which file?)
- [ ] Am I using the correct system? (Assets, Config, Events, etc.)
- [ ] Am I following Bestow conventions? (Interfaces, DI, ECS)
- [ ] Does this need hot reload support?
- [ ] How will errors be handled?
- [ ] Is this testable?
- [ ] Is this performant? (No premature optimization, but don't be wasteful)
- [ ] Will a designer be able to tune this? (Put values in Lua if yes)
