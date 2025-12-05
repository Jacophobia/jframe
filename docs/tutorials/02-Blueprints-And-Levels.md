# Tutorial 2: Blueprints and Levels

In this tutorial, you'll learn how to use Bestow's data-driven architecture to create entities from Lua blueprints and build levels without recompiling C++ code.

## What Are Blueprints?

Blueprints are Lua files that define entity templates. They describe what components an entity has and their initial values. Instead of hardcoding entity creation in C++, you define them in Lua files that can be edited and hot-reloaded.

### Why Use Blueprints?

- **No Recompilation** - Edit entity properties without rebuilding C++
- **Hot Reload** - Changes appear instantly in debug builds
- **Designer Friendly** - Lua is easier to learn than C++
- **Reusable** - Define once, spawn many times
- **Inheritance** - Extend base blueprints to create variants

## Blueprint Structure

A blueprint is a Lua table with component data:

```lua
-- data/blueprints/player.lua
return {
    -- Physics component
    physics = {
        type = "dynamic",
        width = 32,
        height = 48,
        fixedRotation = true
    },

    -- Sprite component
    sprite = {
        texture = "player.png",
        width = 32,
        height = 48
    },

    -- Controller component
    controller = {
        moveSpeed = 200,
        jumpForce = 400,
        airControl = 0.3
    },

    -- Health component
    health = {
        current = 100,
        maximum = 100
    }
}
```

## Step 1: Create a Simple Blueprint

Create `data/blueprints/enemy.lua`:

```lua
-- data/blueprints/enemy.lua
-- A simple enemy that patrols back and forth

return {
    physics = {
        type = "dynamic",
        width = 24,
        height = 24,
        fixedRotation = true
    },

    enemy = {
        damage = 10,
        moveSpeed = 50,
        patrolRange = 100
    },

    health = {
        current = 50,
        maximum = 50
    }
}
```

## Step 2: Using Traits

Traits are reusable functions that return component data. They help avoid repetition.

Create `data/traits/physics_body.lua`:

```lua
-- data/traits/physics_body.lua
-- Returns a physics component configuration

return function(opts)
    return {
        type = opts.type or "dynamic",
        width = opts.width or 32,
        height = opts.height or 32,
        fixedRotation = opts.fixedRotation ~= false,
        density = opts.density or 1.0,
        friction = opts.friction or 0.3
    }
end
```

Create `data/traits/health.lua`:

```lua
-- data/traits/health.lua
-- Returns a health component

return function(maxHealth)
    return {
        current = maxHealth,
        maximum = maxHealth,
        invulnerable = false
    }
end
```

Now update `enemy.lua` to use traits:

```lua
-- data/blueprints/enemy.lua
local physics = require("traits/physics_body")
local health = require("traits/health")

return {
    physics = physics({
        width = 24,
        height = 24,
        type = "dynamic"
    }),

    health = health(50),

    enemy = {
        damage = 10,
        moveSpeed = 50,
        patrolRange = 100
    }
}
```

## Step 3: Blueprint Inheritance

Create a base enemy blueprint that all enemies extend.

Create `data/blueprints/_base.lua`:

```lua
-- data/blueprints/_base.lua
-- Prototype inheritance system

local P = {}

-- Deep copy table
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

Create `data/blueprints/enemies/base.lua`:

```lua
-- data/blueprints/enemies/base.lua
local physics = require("traits/physics_body")
local health = require("traits/health")

return {
    physics = physics({
        width = 24,
        height = 24,
        type = "dynamic"
    }),

    health = health(50),

    enemy = {
        damage = 10,
        moveSpeed = 50,
        aiState = "idle"
    }
}
```

Create `data/blueprints/enemies/slime.lua`:

```lua
-- data/blueprints/enemies/slime.lua
local P = require("blueprints/_base")

return P.extend("blueprints/enemies/base", {
    -- Override to be weaker
    health = {
        current = 30,
        maximum = 30
    },

    -- Override to be slower but deal more damage
    enemy = {
        damage = 15,
        moveSpeed = 30,
        aiState = "patrolling"
    }
})
```

Create `data/blueprints/enemies/fast.lua`:

```lua
-- data/blueprints/enemies/fast.lua
local P = require("blueprints/_base")

return P.extend("blueprints/enemies/base", {
    -- Override to be faster and weaker
    health = {
        current = 25,
        maximum = 25
    },

    enemy = {
        damage = 5,
        moveSpeed = 100,
        aiState = "chasing"
    }
})
```

## Step 4: Creating Levels

Levels are Lua files that reference blueprints and specify spawn positions.

Create `data/levels/level1.lua`:

```lua
-- data/levels/level1.lua
return {
    name = "The Beginning",
    width = 1600,
    height = 600,
    backgroundColor = {0.4, 0.6, 0.9, 1.0},  -- Sky blue

    -- Spawn points for named entities
    spawnPoints = {
        player = {x = 100, y = 400},
        enemy1 = {x = 600, y = 400},
        enemy2 = {x = 1000, y = 400}
    },

    -- Static platforms
    platforms = {
        -- Ground
        {x = 800, y = 50, width = 1600, height = 100},

        -- Floating platforms
        {x = 300, y = 200, width = 200, height = 20},
        {x = 700, y = 350, width = 200, height = 20},
        {x = 1100, y = 500, width = 200, height = 20}
    },

    -- Entities to spawn (references blueprints)
    entities = {
        -- Player
        {blueprint = "player", spawnPoint = "player"},

        -- Enemies
        {blueprint = "enemies/slime", spawnPoint = "enemy1"},
        {blueprint = "enemies/fast", spawnPoint = "enemy2"}
    }
}
```

## Step 5: Loading Levels in C++

Here's how to load and use a level in your game:

```cpp
// In Game.cpp - initialize()

// Register and load the level asset
levelAsset_ = sys.assets->registerAsset(
    bestow::AssetType::Data,
    "data/levels/level1.lua"
);
sys.assets->loadAsset(levelAsset_);

// Load the level through the level system
auto levelResult = sys.levels->loadLevel(levelAsset_);
if (levelResult) {
    currentLevel_ = *levelResult;
    sys.levels->setActiveLevel(currentLevel_);
    bestow::core::logInfo("Level loaded successfully");
} else {
    bestow::core::logError("Failed to load level");
    return false;
}

// Get spawn points from the level
auto playerSpawn = sys.levels->getSpawnPoint(currentLevel_, "player");
if (playerSpawn) {
    bestow::core::logInfo(std::format("Player spawn: ({}, {})",
        playerSpawn->x, playerSpawn->y));
}

// Get level metadata
auto metadata = sys.levels->getLevelMetadata(currentLevel_);
bestow::core::logInfo("Level name: " + metadata.name);
```

## Step 6: Advanced Level Patterns

Use Lua's programming features to create complex patterns:

### Pattern: Grid of Coins

```lua
-- Generate a 5x3 grid of coins
local coins = {}
for row = 0, 2 do
    for col = 0, 4 do
        table.insert(coins, {
            blueprint = "items/coin",
            x = 200 + col * 50,
            y = 200 + row * 50
        })
    end
end
```

### Pattern: Wave Formation

```lua
-- Generate a sine wave of enemies
local enemies = {}
for i = 0, 10 do
    table.insert(enemies, {
        blueprint = "enemies/slime",
        x = 100 + i * 100,
        y = 300 + math.sin(i * 0.5) * 100
    })
end
```

### Pattern: Conditional Spawning

```lua
-- Spawn different entities based on difficulty
local difficulty = "hard"

local enemies = {}
if difficulty == "easy" then
    table.insert(enemies, {blueprint = "enemies/slime", x = 500, y = 300})
elseif difficulty == "hard" then
    table.insert(enemies, {blueprint = "enemies/fast", x = 500, y = 300})
    table.insert(enemies, {blueprint = "enemies/fast", x = 600, y = 300})
end
```

### Full Example with Helpers

Create `data/levels/helpers/patterns.lua`:

```lua
-- data/levels/helpers/patterns.lua
local P = {}

-- Spawn entities in a grid
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

-- Spawn entities in a circle
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

Use in a level:

```lua
-- data/levels/level2.lua
local patterns = require("levels/helpers/patterns")

local entities = {}

-- Add player
table.insert(entities, {blueprint = "player", x = 100, y = 300})

-- Add grid of coins
for _, coin in ipairs(patterns.grid("items/coin", 300, 200, 10, 3, 50, 50)) do
    table.insert(entities, coin)
end

-- Add circle of enemies
for _, enemy in ipairs(patterns.circle("enemies/slime", 800, 300, 200, 8)) do
    table.insert(entities, enemy)
end

return {
    name = "Pattern Showcase",
    entities = entities
}
```

## Step 7: Hot Reload

In debug builds, Bestow watches for file changes and reloads automatically:

```cpp
#if defined(BESTOW_DEV_TOOLS)
    hotReload_.watchDirectory("data/");

    hotReload_.onBlueprintChanged = [this](const auto& path) {
        bestow::core::logInfo("Blueprint changed: " + path.string());
        // Reload blueprint and update entities
    };

    hotReload_.onLevelChanged = [this](const auto& path) {
        bestow::core::logInfo("Level changed: " + path.string());
        // Reload level
        auto& sys = engine_->systems();
        sys.assets->reloadAsset(levelAsset_);
        loadLevel();
    };
#endif
```

Now you can:
1. Run your game in debug mode
2. Edit `data/blueprints/enemy.lua`
3. Save the file
4. See changes instantly without recompiling!

## Best Practices

### 1. Use Traits for Reusability

Bad:
```lua
-- enemy1.lua
return {
    physics = {type = "dynamic", width = 24, height = 24}
}

-- enemy2.lua (duplicate code!)
return {
    physics = {type = "dynamic", width = 24, height = 24}
}
```

Good:
```lua
-- Both files use the same trait
local physics = require("traits/physics_body")
return {
    physics = physics({width = 24, height = 24})
}
```

### 2. Use Inheritance for Variants

Bad:
```lua
-- Copy-paste the entire blueprint for each variant
```

Good:
```lua
-- Extend base and override only what's different
return P.extend("blueprints/enemies/base", {
    health = {current = 30, maximum = 30}
})
```

### 3. Use Variables for Constants

Bad:
```lua
return {
    platforms = {
        {x = 100, y = 50, width = 800, height = 100},
        {x = 100, y = 200, width = 800, height = 20}
    }
}
```

Good:
```lua
local GROUND_HEIGHT = 100
local PLATFORM_WIDTH = 800

return {
    platforms = {
        {x = 100, y = 50, width = PLATFORM_WIDTH, height = GROUND_HEIGHT},
        {x = 100, y = 200, width = PLATFORM_WIDTH, height = 20}
    }
}
```

### 4. Comment Your Blueprints

```lua
-- Player character blueprint
-- Used in all levels
-- Health and speed tuned for normal difficulty

return {
    -- Fast movement for responsive controls
    controller = {
        moveSpeed = 250,  -- Pixels per second
        jumpForce = 450   -- Initial jump velocity
    },

    -- Takes 5 hits to die (10 damage per hit)
    health = health(50)
}
```

## Common Patterns

### Collectible Item

```lua
-- data/blueprints/items/coin.lua
local physics = require("traits/physics_body")

return {
    physics = physics({
        type = "static",
        width = 16,
        height = 16,
        isSensor = true  -- No collision, just triggers
    }),

    collectible = {
        type = "coin",
        value = 10,
        sound = "coin_pickup.wav"
    }
}
```

### Projectile

```lua
-- data/blueprints/projectiles/arrow.lua
local physics = require("traits/physics_body")

return {
    physics = physics({
        type = "dynamic",
        width = 4,
        height = 16,
        fixedRotation = false,  -- Rotates to face direction
        isBullet = true  -- Continuous collision detection
    }),

    projectile = {
        damage = 25,
        speed = 500,
        lifetime = 3.0,  -- Seconds before despawn
        pierce = false
    }
}
```

### Moving Platform

```lua
-- data/blueprints/platforms/moving.lua
local physics = require("traits/physics_body")

return {
    physics = physics({
        type = "kinematic",  -- Moves but ignores forces
        width = 100,
        height = 20
    }),

    movingPlatform = {
        startX = 200,
        endX = 600,
        speed = 100,
        moveTime = 4.0
    }
}
```

## Next Steps

Now you know how to create data-driven entities and levels! Next tutorials:

- **Tutorial 3: Physics and Collision** - Collision layers, callbacks, ground detection
- **Tutorial 4: Input and Controls** - Advanced input handling
- **Tutorial 5: Audio** - Adding sounds and music

## Troubleshooting

**Lua syntax error**
- Check for missing commas or brackets
- Lua allows trailing commas, so `{1, 2, 3,}` is valid
- Use `--` for comments

**Blueprint not found**
- Paths are relative to the data directory
- Use forward slashes: `"blueprints/enemies/slime.lua"`
- Don't include `.lua` extension in `require()`: `require("traits/health")`

**Hot reload not working**
- Only works in debug builds with `BESTOW_DEV_TOOLS`
- Make sure you called `hotReload_.watchDirectory("data/")`
- Check console logs for file watcher errors

**P.extend() undefined**
- Make sure `blueprints/_base.lua` exists
- The file must `return P` at the end
- Require it: `local P = require("blueprints/_base")`
