# Tutorial 8: Lua Data-Driven Design

In this tutorial, you'll learn how to use Lua to create fully data-driven games in Bestow. **Game developers should spend most of their time in Lua files, not C++.** This philosophy enables rapid iteration, hot reload, and designer-friendly workflows.

## Why Lua Instead of JSON?

Lua offers powerful features that JSON cannot provide:

| Feature | Lua | JSON |
|---------|-----|------|
| Comments | `-- comment` | Not allowed |
| Trailing commas | Allowed | Causes errors |
| Variables | `local SPEED = 100` | Not supported |
| Loops | `for i = 1, 10 do ... end` | Not supported |
| Math | `math.sin(i) * 100` | Not supported |
| Functions | `function makeEnemy(x) ... end` | Not supported |
| Conditionals | `DEBUG and {...} or {}` | Not supported |

### Real Example: Compare JSON vs Lua

**JSON approach (rigid, repetitive):**
```json
{
  "platforms": [
    {"x": 100, "y": 200, "width": 100, "height": 20},
    {"x": 200, "y": 200, "width": 100, "height": 20},
    {"x": 300, "y": 200, "width": 100, "height": 20},
    {"x": 400, "y": 200, "width": 100, "height": 20},
    {"x": 500, "y": 200, "width": 100, "height": 20}
  ]
}
```

**Lua approach (flexible, procedural):**
```lua
local platforms = {}
for i = 0, 4 do
    table.insert(platforms, {
        x = 100 + i * 100,
        y = 200,
        width = 100,
        height = 20
    })
end
return {platforms = platforms}
```

## Core Principle: Lua-First Design

**Always put configuration and content in Lua unless there's a compelling technical reason not to.**

- **C++ layer** = Engine (systems, rendering, physics)
- **Lua layer** = Content (levels, entities, behaviors, tuning)

## Data Files in Bestow

### Blueprints (`data/blueprints/*.lua`)

Entity templates with component configurations:

```lua
-- data/blueprints/player.lua
return {
    type = "player",
    tags = {"player", "controllable"},

    -- Physics component
    physics = {
        type = "dynamic",
        width = 30,
        height = 50,
        fixedRotation = true
    },

    -- Sprite component
    sprite = {
        texture = "textures/player.png",
        frameWidth = 32,
        frameHeight = 32
    },

    -- Health component
    health = {
        current = 100,
        maximum = 100
    }
}
```

### Levels (`data/levels/*.lua`)

Complete level definitions with entities and spawn points:

```lua
-- data/levels/level1.lua
return {
    name = "The Beginning",
    width = 1600,
    height = 600,
    backgroundColor = {0.4, 0.6, 0.9, 1.0},  -- Sky blue

    -- Named spawn points
    spawnPoints = {
        player = {x = 100, y = 400},
        boss = {x = 1400, y = 400}
    },

    -- Static environment
    platforms = {
        {x = 800, y = 50, width = 1600, height = 100},  -- Ground
        {x = 300, y = 200, width = 200, height = 20},   -- Platform 1
        {x = 700, y = 350, width = 200, height = 20}    -- Platform 2
    },

    -- Entities to spawn
    entities = {
        {blueprint = "player", spawnPoint = "player"},
        {blueprint = "enemies/slime", x = 600, y = 400},
        {blueprint = "items/health_potion", x = 800, y = 300}
    }
}
```

### Configuration (`data/config/*.lua`)

Gameplay parameters, tuning values, and settings:

```lua
-- data/config/player.lua
return {
    movement = {
        walkSpeed = 400.0,
        runSpeed = 600.0,
        jumpForce = 800.0,
        airControl = 0.3  -- 30% control while airborne
    },

    health = {
        initial = 100,
        maximum = 100,
        regenRate = 5.0  -- HP per second
    },

    stamina = {
        initial = 100,
        maximum = 100,
        regenRate = 20.0,
        sprintDrain = 10.0
    }
}
```

### Materials (`data/materials/*.lua`)

Shader configurations and rendering properties:

```lua
-- data/materials/toon.lua
return {
    shader = "shaders/toon",

    properties = {
        -- Material colors
        baseColor = {1.0, 0.8, 0.6, 1.0},
        rimColor = {1.0, 1.0, 1.0, 1.0},

        -- Toon shading
        shadingLevels = 3,
        specularIntensity = 0.8,
        rimPower = 2.0
    },

    textures = {
        albedo = "textures/character_diffuse.png",
        normal = "textures/character_normal.png"
    }
}
```

## Advanced Lua Patterns

### Pattern 1: Variables for Constants

Use local variables to avoid magic numbers:

```lua
-- data/levels/platformer.lua
local GROUND_Y = 50
local GROUND_HEIGHT = 100
local PLATFORM_WIDTH = 200
local PLATFORM_HEIGHT = 20

return {
    platforms = {
        {x = 800, y = GROUND_Y, width = 1600, height = GROUND_HEIGHT},
        {x = 300, y = 200, width = PLATFORM_WIDTH, height = PLATFORM_HEIGHT},
        {x = 700, y = 350, width = PLATFORM_WIDTH, height = PLATFORM_HEIGHT}
    }
}
```

### Pattern 2: Loops for Procedural Generation

Generate repeating patterns programmatically:

```lua
-- data/levels/coin_collector.lua

-- Generate a 10x3 grid of coins
local coins = {}
for row = 0, 2 do
    for col = 0, 9 do
        table.insert(coins, {
            blueprint = "items/coin",
            x = 200 + col * 50,
            y = 200 + row * 50
        })
    end
end

return {
    name = "Coin Collector",
    entities = coins
}
```

### Pattern 3: Math for Complex Patterns

Use Lua's math library for sophisticated layouts:

```lua
-- data/levels/wave_enemies.lua

-- Generate a sine wave of enemies
local enemies = {}
for i = 0, 20 do
    local angle = i * 0.3
    table.insert(enemies, {
        blueprint = "enemies/slime",
        x = 100 + i * 50,
        y = 300 + math.sin(angle) * 100  -- Sine wave pattern
    })
end

-- Generate a circle of enemies
local centerX, centerY = 800, 300
local radius = 200
for i = 0, 7 do
    local angle = (i / 8) * 2 * math.pi
    table.insert(enemies, {
        blueprint = "enemies/flying",
        x = centerX + math.cos(angle) * radius,
        y = centerY + math.sin(angle) * radius
    })
end

return {entities = enemies}
```

### Pattern 4: Conditionals for Difficulty

Adjust content based on configuration:

```lua
-- data/levels/adaptive.lua
local difficulty = "hard"  -- Can be loaded from config

local enemies = {}

if difficulty == "easy" then
    -- Fewer enemies, lower stats
    table.insert(enemies, {
        blueprint = "enemies/weak_slime",
        x = 500,
        y = 300
    })
elseif difficulty == "normal" then
    -- Standard challenge
    table.insert(enemies, {blueprint = "enemies/slime", x = 500, y = 300})
    table.insert(enemies, {blueprint = "enemies/slime", x = 700, y = 300})
elseif difficulty == "hard" then
    -- More enemies, stronger variants
    table.insert(enemies, {blueprint = "enemies/fast_slime", x = 500, y = 300})
    table.insert(enemies, {blueprint = "enemies/fast_slime", x = 600, y = 300})
    table.insert(enemies, {blueprint = "enemies/boss_slime", x = 800, y = 300})
end

return {entities = enemies}
```

### Pattern 5: Functions for Reusability

Define helper functions in your Lua files:

```lua
-- data/levels/helpers/patterns.lua
local P = {}

-- Create a horizontal line of entities
function P.line(blueprint, startX, startY, count, spacing)
    local entities = {}
    for i = 0, count - 1 do
        table.insert(entities, {
            blueprint = blueprint,
            x = startX + i * spacing,
            y = startY
        })
    end
    return entities
end

-- Create a grid of entities
function P.grid(blueprint, startX, startY, cols, rows, spacingX, spacingY)
    local entities = {}
    for row = 0, rows - 1 do
        for col = 0, cols - 1 do
            table.insert(entities, {
                blueprint = blueprint,
                x = startX + col * spacingX,
                y = startY + row * spacingY
            })
        end
    end
    return entities
end

-- Create a circle of entities
function P.circle(blueprint, centerX, centerY, radius, count)
    local entities = {}
    for i = 0, count - 1 do
        local angle = (i / count) * 2 * math.pi
        table.insert(entities, {
            blueprint = blueprint,
            x = centerX + math.cos(angle) * radius,
            y = centerY + math.sin(angle) * radius
        })
    end
    return entities
end

return P
```

Use these helpers in your levels:

```lua
-- data/levels/pattern_showcase.lua
local patterns = require("levels/helpers/patterns")

local entities = {}

-- Add player
table.insert(entities, {blueprint = "player", x = 100, y = 300})

-- Add a line of coins
for _, coin in ipairs(patterns.line("items/coin", 200, 200, 10, 50)) do
    table.insert(entities, coin)
end

-- Add a grid of enemies
for _, enemy in ipairs(patterns.grid("enemies/slime", 500, 300, 4, 3, 100, 100)) do
    table.insert(entities, enemy)
end

-- Add a circle of power-ups
for _, powerup in ipairs(patterns.circle("items/powerup", 1000, 400, 150, 8)) do
    table.insert(entities, powerup)
end

return {
    name = "Pattern Showcase",
    entities = entities
}
```

## Blueprint Inheritance

Create base blueprints and extend them for variants:

### Base Blueprint

```lua
-- data/blueprints/_base.lua
-- Prototype inheritance system

local P = {}

-- Deep copy a table
local function deepCopy(original)
    if type(original) ~= 'table' then
        return original
    end
    local copy = {}
    for k, v in pairs(original) do
        copy[k] = deepCopy(v)
    end
    return copy
end

-- Merge child into parent (child wins conflicts)
local function merge(parent, child)
    local result = deepCopy(parent)
    for k, v in pairs(child) do
        if type(v) == 'table' and type(result[k]) == 'table' then
            result[k] = merge(result[k], v)
        else
            result[k] = v
        end
    end
    return result
end

-- Extend a parent blueprint
function P.extend(parentPath, overrides)
    local parent = require(parentPath)
    return merge(parent, overrides)
end

return P
```

### Parent Blueprint

```lua
-- data/blueprints/enemies/base.lua
return {
    tags = {"enemy", "damageable"},

    physics = {
        type = "dynamic",
        width = 24,
        height = 24,
        fixedRotation = true
    },

    health = {
        current = 50,
        maximum = 50
    },

    enemy = {
        damage = 10,
        moveSpeed = 50,
        detectionRange = 200,
        aiState = "idle"
    }
}
```

### Child Blueprints

```lua
-- data/blueprints/enemies/slime.lua
local P = require("blueprints/_base")

-- Slow, tanky variant
return P.extend("blueprints/enemies/base", {
    health = {
        current = 100,
        maximum = 100
    },

    enemy = {
        damage = 15,
        moveSpeed = 30
    }
})
```

```lua
-- data/blueprints/enemies/fast.lua
local P = require("blueprints/_base")

-- Fast, fragile variant
return P.extend("blueprints/enemies/base", {
    health = {
        current = 25,
        maximum = 25
    },

    enemy = {
        damage = 5,
        moveSpeed = 120
    }
})
```

```lua
-- data/blueprints/enemies/boss.lua
local P = require("blueprints/_base")

-- Powerful boss variant
return P.extend("blueprints/enemies/base", {
    physics = {
        width = 64,
        height = 64
    },

    health = {
        current = 500,
        maximum = 500
    },

    enemy = {
        damage = 50,
        moveSpeed = 40,
        detectionRange = 400,
        aiState = "aggressive"
    }
})
```

## Traits: Reusable Component Configs

Traits are functions that return component data:

```lua
-- data/traits/physics_body.lua
return function(opts)
    opts = opts or {}
    return {
        type = opts.type or "dynamic",
        width = opts.width or 32,
        height = opts.height or 32,
        fixedRotation = opts.fixedRotation ~= false,
        density = opts.density or 1.0,
        friction = opts.friction or 0.3,
        restitution = opts.restitution or 0.0
    }
end
```

```lua
-- data/traits/health.lua
return function(maxHealth, startingPercent)
    startingPercent = startingPercent or 1.0
    return {
        current = maxHealth * startingPercent,
        maximum = maxHealth,
        invulnerable = false,
        regenRate = 0.0
    }
end
```

```lua
-- data/traits/sprite.lua
return function(texturePath, width, height)
    return {
        texture = texturePath,
        frameWidth = width or 32,
        frameHeight = height or 32,
        layer = 10,
        visible = true,
        flipX = false,
        flipY = false
    }
end
```

Use traits in blueprints:

```lua
-- data/blueprints/player.lua
local physics = require("traits/physics_body")
local health = require("traits/health")
local sprite = require("traits/sprite")

return {
    type = "player",

    physics = physics({
        width = 30,
        height = 50,
        fixedRotation = true
    }),

    health = health(100),  -- 100 max HP, start at 100%

    sprite = sprite("textures/player.png", 32, 32)
}
```

## Loading Lua in C++

### Loading Configuration

```cpp
// In your game's initialize()
void Game::loadConfig() {
    auto& assets = engine_->systems().assets;

    // Register and load config file
    auto configHandle = assets->registerAsset(
        bestow::AssetType::Data,
        "data/config/player.lua"
    );
    assets->loadAsset(configHandle);

    // Get the loaded data
    auto* data = assets->getAsset<bestow::DataAsset>(configHandle);
    if (!data) return;

    // Extract values
    playerMoveSpeed_ = getJsonValue<float>(*data, "movement.walkSpeed", 400.0f);
    playerJumpForce_ = getJsonValue<float>(*data, "movement.jumpForce", 800.0f);
    playerMaxHealth_ = getJsonValue<int>(*data, "health.maximum", 100);
}
```

### Loading Levels

```cpp
void Game::loadLevel(const std::string& levelPath) {
    auto& sys = engine_->systems();

    // Load the level file
    auto levelHandle = sys.assets->registerAsset(
        bestow::AssetType::Data,
        levelPath
    );
    sys.assets->loadAsset(levelHandle);

    // Parse level data
    auto* levelData = sys.assets->getAsset<bestow::DataAsset>(levelHandle);
    if (!levelData) return;

    // Get level metadata
    std::string levelName = getJsonValue<std::string>(*levelData, "name", "Untitled");
    bestow::core::logInfo("Loading level: " + levelName);

    // Spawn entities defined in level
    // (Implementation depends on your entity spawning system)
}
```

## Hot Reload Workflow

One of the biggest advantages of Lua is **instant hot reload** in debug builds:

### Setup Hot Reload

```cpp
#if defined(BESTOW_DEV_TOOLS)
void Game::setupHotReload() {
    auto& assets = engine_->systems().assets;

    // Enable hot reload
    assets->enableHotReload(true);

    // Subscribe to config changes
    configSubId_ = assets->subscribeToType(
        bestow::AssetType::Data,
        [this](bestow::AssetHandle handle, bestow::AssetType type) {
            auto metadata = assets->getAssetMetadata(handle);

            // Reload player config
            if (metadata.path.string().find("config/player.lua") != std::string::npos) {
                loadPlayerConfig();
                bestow::core::logInfo("Player config reloaded!");
            }

            // Reload current level
            if (metadata.path.string().find("levels/") != std::string::npos) {
                reloadCurrentLevel();
                bestow::core::logInfo("Level reloaded!");
            }
        }
    );
}
#endif
```

### Development Loop

1. Run your game in debug mode
2. Edit `data/config/player.lua`
3. Change `walkSpeed = 400.0` to `walkSpeed = 600.0`
4. Save the file
5. **Changes apply instantly** - no recompilation needed!

This rapid iteration loop is the core benefit of data-driven design.

## Best Practices

### 1. Comment Your Lua Files

```lua
-- data/config/abilities.lua
-- Defines player ability parameters
-- Balance tuned for normal difficulty

return {
    dash = {
        cooldown = 2.0,        -- Seconds between uses
        staminaCost = 25.0,    -- Stamina consumed per use
        speedMultiplier = 3.0, -- How much faster than normal
        duration = 0.3         -- How long dash lasts (seconds)
    }
}
```

### 2. Use Descriptive Variable Names

```lua
-- ❌ BAD - Magic numbers
for i = 0, 9 do
    table.insert(entities, {blueprint = "coin", x = 100 + i * 50, y = 200})
end

-- ✅ GOOD - Named constants
local COIN_START_X = 100
local COIN_Y = 200
local COIN_SPACING = 50
local COIN_COUNT = 10

for i = 0, COIN_COUNT - 1 do
    table.insert(entities, {
        blueprint = "coin",
        x = COIN_START_X + i * COIN_SPACING,
        y = COIN_Y
    })
end
```

### 3. Extract Helpers to Separate Files

```lua
-- ❌ BAD - Duplicate code in every level
-- level1.lua: for i = 0, 9 do ... end
-- level2.lua: for i = 0, 9 do ... end  (same code!)

-- ✅ GOOD - Shared helper
-- helpers/patterns.lua: function P.line(...) ... end
-- level1.lua: patterns.line(...)
-- level2.lua: patterns.line(...)
```

### 4. Use Inheritance for Variants

```lua
-- ❌ BAD - Copy entire blueprint for each variant
-- enemies/slime.lua: 50 lines
-- enemies/fast_slime.lua: 50 lines (95% duplicate!)

-- ✅ GOOD - Extend base blueprint
-- enemies/base.lua: 50 lines
-- enemies/slime.lua: P.extend(...) only overrides (5 lines)
-- enemies/fast_slime.lua: P.extend(...) only overrides (5 lines)
```

### 5. Keep C++ Generic

Your C++ should not know about specific entity types:

```cpp
// ❌ BAD - Hardcoded entity types in C++
void createPlayer() { ... }
void createEnemy() { ... }
void createCoin() { ... }

// ✅ GOOD - Generic factory
void createEntityFromBlueprint(const std::string& blueprintPath) {
    // Load any blueprint and create entity
}
```

## Common Patterns

### Debug Toggles

```lua
-- data/config/debug.lua
local DEBUG = true

return {
    -- Only spawn enemies in non-debug mode
    enableEnemies = not DEBUG,

    -- Give player debug powers
    playerInvulnerable = DEBUG,
    playerSpeed = DEBUG and 1000.0 or 400.0,

    -- Show debug visualizations
    showColliders = DEBUG,
    showSpawnPoints = DEBUG
}
```

### Randomization

```lua
-- data/levels/random_enemies.lua
math.randomseed(os.time())

local enemies = {}
for i = 1, 10 do
    local enemyType = math.random(3)
    local blueprint = "enemies/slime"

    if enemyType == 1 then blueprint = "enemies/slime"
    elseif enemyType == 2 then blueprint = "enemies/fast"
    elseif enemyType == 3 then blueprint = "enemies/tank"
    end

    table.insert(enemies, {
        blueprint = blueprint,
        x = 200 + i * 100,
        y = 300 + math.random(-50, 50)
    })
end

return {entities = enemies}
```

### Data Validation

```lua
-- data/blueprints/validated_player.lua
local function validate(value, min, max, default)
    if type(value) ~= "number" then return default end
    return math.max(min, math.min(max, value))
end

local moveSpeed = 400.0  -- Configured value

return {
    player = {
        -- Clamp to valid range
        moveSpeed = validate(moveSpeed, 100.0, 1000.0, 400.0),
        jumpForce = validate(800.0, 200.0, 2000.0, 800.0)
    }
}
```

## Summary

Lua-driven design in Bestow provides:

1. **Rapid Iteration** - Edit and reload without recompiling
2. **Designer-Friendly** - No C++ knowledge required for content
3. **Procedural Generation** - Loops, math, functions for complex patterns
4. **Reusability** - Traits, inheritance, helper functions
5. **Flexibility** - Conditionals, variables, computed values

**Remember:** Game developers should spend most of their time in Lua files, not C++. Put everything you can in Lua, and keep C++ for engine systems only.

## Next Steps

- **Tutorial 9: Advanced AI with Lua** - Behavior trees, state machines in Lua
- **Tutorial 10: Lua Scripting API** - Expose C++ systems to Lua for scripted gameplay
- **Tutorial 11: Modding Support** - Allow players to create custom content with Lua

Your games will be more maintainable, moddable, and enjoyable to develop when you embrace Lua-first design!
