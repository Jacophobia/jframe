# Data-Driven Design Guide

JFrame uses a **data-driven architecture** where game content is defined in Lua, not C++. This guide explains what belongs in each layer.

## Core Principle: Lua First

**Always put configuration and content in Lua unless there's a compelling technical reason not to.**

The C++ layer provides the *engine* (systems, rendering, physics). The Lua layer provides the *content* (levels, entities, behaviors, tuning).

## What Goes in Lua

### Levels (`data/levels/*.lua`)

ALL level content:

```lua
return {
    name = "Demo Level 1",

    -- Player spawn point
    spawnPoints = {
        player = { x = 100, y = 400 },
    },

    -- Every entity in the level
    entities = {
        { type = "platform", x = 400, y = 550, width = 800, height = 50 },
        { type = "jump_zone", x = 150, y = 450, width = 150, height = 200 },
        { type = "enemy", blueprint = "enemies/slime", x = 400, y = 500 },
    }
}
```

### Blueprints (`data/blueprints/*.lua`)

Entity templates with component configurations:

```lua
return {
    type = "player",
    tags = { "player", "controllable" },

    physics = {
        type = "dynamic",
        width = 30,
        height = 50,
        fixedRotation = true
    },

    sprite = {
        texture = "data/textures/player.png",
        frameWidth = 32,
        frameHeight = 32
    }
}
```

### Configuration (`data/config/*.lua`)

All gameplay parameters, tuning values, and settings:

```lua
-- config/player.lua
return {
    moveSpeed = 400.0,
    jumpForce = 800.0,
    health = { initial = 100, maximum = 100 },
    stamina = { initial = 100, maximum = 100, regenRate = 20.0 }
}

-- config/abilities.lua
return {
    dash = {
        cooldown = 2.0,
        staminaCost = 25.0,
        speedMultiplier = 3.0,
        duration = 0.3
    }
}
```

### Input Mappings (`data/config/input.lua`)

```lua
return {
    actions = {
        move_left = { keyboard = "A", keyboard_alt = "Left" },
        move_right = { keyboard = "D", keyboard_alt = "Right" },
        jump = { keyboard = "Space" },
        dash = { keyboard = "LeftShift" }
    }
}
```

### Audio/Visual Assets (`data/config/assets.lua`)

```lua
return {
    textures = {
        player = "textures/player.png",
        platform = "textures/block.png"
    },
    sounds = {
        jump = "audio/jump.wav",
        coin = "audio/coin.wav"
    }
}
```

## What Goes in C++

### Engine Systems

- Entity System (creating/destroying entities)
- Physics System (Box2D integration)
- Graphics System (rendering)
- Input System (polling hardware)
- Audio System (FMOD integration)

### Entity Factory Logic

C++ reads Lua and creates entities:

```cpp
void Game::loadLevel(const std::string& path) {
    auto levelData = lua.loadLevel(path);  // Parse Lua

    for (const auto& entityDef : levelData.entities) {
        createEntityFromDef(entityDef);  // Generic factory
    }
}

void Game::createEntityFromDef(const EntityDef& def) {
    if (def.type == "platform") {
        createPlatform(def.x, def.y, def.width, def.height);
    } else if (def.type == "jump_zone") {
        createJumpZone(def.x, def.y, def.width, def.height);
    }
    // etc.
}
```

### Core Game Loop

```cpp
void Game::updateFixed(DeltaTime dt) {
    gas_->update(dt);
    handleInput(dt);  // Uses config values loaded from Lua
    updateCamera(dt);
}
```

### System Interactions

Complex interactions between systems that can't be expressed declaratively:

```cpp
void Game::handleJump() {
    if (gas_->canActivateAbility(player_, jumpAbility_)) {
        gas_->tryActivateAbility(player_, jumpAbility_);

        // Apply physics impulse
        Vec2 vel = physics_->getVelocity(player_);
        vel.y = -config_.jumpForce;  // Value from Lua!
        physics_->setVelocity(player_, vel);
    }
}
```

## Decision Guide

| Content Type | Location | Reason |
|-------------|----------|--------|
| Entity positions | Lua | Level design changes frequently |
| Platform dimensions | Lua | Easily tweakable without rebuild |
| Player speed/jump force | Lua | Balance tuning |
| Ability cooldowns | Lua | Balance tuning |
| Input mappings | Lua | Player customization |
| Spawn points | Lua | Level design |
| Enemy patrol routes | Lua | Level design |
| Animation frame data | Lua | Art pipeline |
| Physics body definitions | Lua | Entity configuration |
| **System implementations** | **C++** | Core engine code |
| **Rendering pipeline** | **C++** | Performance critical |
| **Entity factory dispatch** | **C++** | Type safety |
| **Complex state machines** | **C++** | Performance/complexity |

## Example: Wrong vs Right

### Wrong: Hardcoded Level in C++

```cpp
// DON'T DO THIS
void Game::createTestPlatforms() {
    // Ground platform
    physics->createBody(ground, PhysicsBodyDef{
        .transform = {.x = 400.0f, .y = 550.0f},
        .size = {800.0f, 20.0f}
    });

    // Left platform
    physics->createBody(leftPlat, PhysicsBodyDef{
        .transform = {.x = 200.0f, .y = 400.0f},
        .size = {200.0f, 20.0f}
    });
}
```

### Right: Data-Driven Level

**data/levels/level1.lua:**
```lua
return {
    entities = {
        { type = "platform", x = 400, y = 550, width = 800, height = 20 },
        { type = "platform", x = 200, y = 400, width = 200, height = 20 },
    }
}
```

**Game.cpp:**
```cpp
void Game::loadLevel(const std::string& path) {
    auto level = lua_.loadLevel(path);

    for (const auto& e : level.entities) {
        if (e.type == "platform") {
            createPlatform(e.x, e.y, e.width, e.height);
        }
    }
}

void Game::createPlatform(float x, float y, float w, float h) {
    Entity platform = entities_->createEntity();
    entities_->emplace<PlatformTag>(platform);
    physics_->createBody(platform, PhysicsBodyDef{
        .type = BodyType::Static,
        .transform = {.x = x, .y = y},
        .size = {w, h}
    });
}
```

## Benefits

1. **Iteration Speed**: Change levels without recompiling
2. **Designer-Friendly**: Non-programmers can edit Lua
3. **Hot Reload**: Changes can be applied at runtime (debug builds)
4. **Version Control**: Level changes are easy to diff/review
5. **Mod Support**: Players can create custom content
6. **Testing**: Easy to create test scenarios

## File Organization

```
data/
├── blueprints/          # Entity templates
│   ├── player.lua
│   ├── platform.lua
│   └── enemies/
│       └── slime.lua
├── config/              # Game configuration
│   ├── player.lua       # Player settings
│   ├── input.lua        # Input mappings
│   ├── abilities.lua    # GAS definitions
│   └── physics.lua      # Physics settings
├── levels/              # Level definitions
│   ├── level1.lua
│   └── level2.lua
└── traits/              # Reusable component configs
    ├── physics_body.lua
    └── health.lua
```

## Summary

**When in doubt, put it in Lua.**

Only use C++ for:
- Engine system implementations
- Performance-critical code paths
- Complex logic that can't be expressed declaratively
- Type-safe factory dispatch (but values still come from Lua!)
