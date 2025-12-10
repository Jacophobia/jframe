# Bestow Level System Guide

The Level System provides Lua-based level definitions with support for entity spawning, spawn points, and level transitions. Levels are data-driven, allowing designers to create and modify game levels without touching C++ code.

## Table of Contents

1. [Overview](#overview)
2. [Core Concepts](#core-concepts)
3. [Lua Level Format](#lua-level-format)
4. [API Reference](#api-reference)
5. [Entity Spawning](#entity-spawning)
6. [Best Practices](#best-practices)
7. [Code Examples](#code-examples)

---

## Overview

### Philosophy: Lua-First Design

The Level System follows Bestow's Lua-first design philosophy:

- **Levels are defined in Lua** - All level data lives in `.lua` files
- **No hardcoded levels** - Game designers work in Lua, not C++
- **Procedural generation supported** - Lua functions can generate entity patterns
- **Hot reload friendly** - Level files can be reloaded during development

### Key Features

- **Level metadata** - Name, dimensions, and properties
- **Spawn points** - Named locations for player/entity spawning
- **Entity definitions** - Pre-defined entities to spawn in the level
- **Level transitions** - Seamless transitions between levels with optional unloading
- **AssetSystem integration** - Levels are assets with hot reload support

---

## Core Concepts

### LevelId

A unique identifier for a loaded level instance. Type alias for `UUID` (64-bit unsigned integer).

```cpp
using LevelId = UUID;
```

Multiple instances of the same level asset can be loaded simultaneously, each with a unique `LevelId`.

### LevelState

An enum representing the lifecycle state of a level:

```cpp
enum class LevelState : std::uint8_t {
    Unloaded,   // Level is not in memory
    Loading,    // Level is being loaded (async)
    Loaded,     // Level is loaded and ready
    Active,     // Level is currently active (future use)
    Unloading   // Level is being unloaded (async)
};
```

**Current behavior**: Levels are immediately `Loaded` after `loadLevel()`. The `Loading`, `Active`, and `Unloading` states are reserved for future async loading support.

### LevelMetadata

Metadata about a loaded level:

```cpp
struct LevelMetadata {
    LevelId id;                   // Unique level instance ID
    AssetHandle assetHandle;      // Asset handle for the level file
    std::string levelName;        // Display name from Lua
    LevelState state;             // Current lifecycle state
    float width;                  // Level width in world units
    float height;                 // Level height in world units
};
```

### Transform2D

Position, rotation, and scale in 2D space:

```cpp
struct Transform2D {
    float x = 0.0f;
    float y = 0.0f;
    float rotation = 0.0f;        // Degrees
    float scaleX = 1.0f;
    float scaleY = 1.0f;

    Vec2 position() const;        // Returns {x, y}
    Vec2 scale() const;           // Returns {scaleX, scaleY}
};
```

### EntityDef

A definition for an entity to be spawned in the level:

```cpp
struct EntityDef {
    std::string type;                                    // Entity type ("player", "enemy", "platform", etc.)
    Transform2D transform;                               // Transform properties
    std::unordered_map<std::string, std::any> properties; // Custom properties
};
```

**Property types supported**:
- `bool` - Boolean values
- `std::string` - String values
- `double` - All Lua numbers (integers stored as doubles)

**Important**: Lua stores all numbers as doubles. When reading numeric properties:

```cpp
// Correct - all Lua numbers are doubles
double value = std::any_cast<double>(entityDef.properties.at("value"));

// Wrong - will throw std::bad_any_cast
int value = std::any_cast<int>(entityDef.properties.at("value"));
```

### LevelTransition

Describes a transition from one level to another:

```cpp
struct LevelTransition {
    LevelId fromLevel;                         // Source level ID
    LevelId toLevel;                           // Destination level ID
    std::optional<std::string> spawnPoint;     // Optional spawn point name in destination
    bool unloadPrevious = true;                // Unload source level after transition
};
```

**Transition behavior**:
- Transitions are **queued** on `transition()` and **executed** on next `update()`
- If `unloadPrevious = true`, the source level is unloaded after switching
- If `spawnPoint` is provided, game code can use it to position the player

---

## Lua Level Format

### Basic Structure

All level files return a Lua table with the following structure:

```lua
return {
  -- Level metadata (required)
  name = "My Level",
  width = 1920,
  height = 1080,

  -- Spawn points (optional)
  spawnPoints = {
    default = { x = 100, y = 500, rotation = 0 },
    checkpoint1 = { x = 500, y = 400 }
  },

  -- Entity definitions (optional)
  entities = {
    { type = "player", x = 100, y = 500 },
    { type = "enemy", x = 400, y = 500, hostile = true }
  }
}
```

### Level Metadata

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `name` | string | Yes | Display name of the level |
| `width` | number | Yes | Level width in world units |
| `height` | number | Yes | Level height in world units |

### Spawn Points

Spawn points are named locations in the level. Each spawn point is a table with:

| Field | Type | Required | Default | Description |
|-------|------|----------|---------|-------------|
| `x` | number | Yes | - | X position in world space |
| `y` | number | Yes | - | Y position in world space |
| `rotation` | number | No | 0 | Rotation in degrees |

**Example**:

```lua
spawnPoints = {
  default = { x = 100, y = 500, rotation = 0 },
  checkpoint1 = { x = 500, y = 400 },
  boss_room = { x = 1800, y = 600, rotation = 180 },
  secret_area = { x = 200, y = 100, rotation = 90 }
}
```

**Naming conventions**:
- Use descriptive names: `default`, `checkpoint1`, `boss_entrance`, `secret_area`
- Names are case-sensitive
- Always provide a `default` spawn point

### Entity Definitions

Entity definitions are tables in the `entities` array. Each entity has:

**Required fields**:
- `type` - String identifying the entity type

**Transform fields** (optional):
- `x` - X position (default: 0)
- `y` - Y position (default: 0)
- `rotation` - Rotation in degrees (default: 0)
- `scaleX` - Horizontal scale (default: 1.0)
- `scaleY` - Vertical scale (default: 1.0)

**Custom properties** (optional):
- Any other fields become custom properties
- Supports: `number`, `string`, `boolean`
- Complex types (tables, functions) are not supported

**Example**:

```lua
entities = {
  -- Minimal entity
  { type = "player", x = 100, y = 500 },

  -- Entity with custom properties
  {
    type = "enemy",
    x = 400,
    y = 500,
    patrolRange = 100,
    speed = 50,
    hostile = true
  },

  -- Entity with transform properties
  {
    type = "rotating_platform",
    x = 600,
    y = 300,
    rotation = 45,
    scaleX = 2.0,
    scaleY = 1.5,
    width = 100,
    height = 20
  }
}
```

### Using Lua Features

#### Variables

```lua
local GROUND_Y = 1000
local PLATFORM_HEIGHT = 50

return {
  name = "Platformer Level",
  width = 1920,
  height = 1080,

  entities = {
    { type = "platform", x = 0, y = GROUND_Y, width = 1920, height = PLATFORM_HEIGHT }
  }
}
```

#### Loops

```lua
return {
  name = "Enemy Wave",
  width = 2000,
  height = 1080,

  entities = function()
    local entities = {}

    -- Generate 10 enemies in a line
    for i = 1, 10 do
      table.insert(entities, {
        type = "enemy",
        x = i * 200,
        y = 500,
        speed = 50
      })
    end

    return entities
  end
}
```

**Note**: The Level System supports both:
- Static entity arrays: `entities = { ... }`
- Generator functions: `entities = function() return {...} end`

#### Math

```lua
return {
  name = "Wave Pattern",
  width = 2000,
  height = 1080,

  entities = function()
    local entities = {}

    -- Sine wave pattern of platforms
    for i = 0, 20 do
      local x = i * 100
      local y = 500 + math.sin(i * 0.5) * 200

      table.insert(entities, {
        type = "platform",
        x = x,
        y = y,
        width = 80,
        height = 20
      })
    end

    return entities
  end
}
```

#### Conditionals

```lua
local DEBUG = true

return {
  name = "Debug Level",
  width = 1920,
  height = 1080,

  entities = {
    { type = "player", x = 100, y = 500 },

    -- Debug entities only
    DEBUG and { type = "debug_grid", x = 0, y = 0 } or nil
  }
}
```

#### Comments

```lua
return {
  name = "My Level",
  width = 1920,
  height = 1080,

  -- This is a comment

  entities = {
    -- Player spawn
    { type = "player", x = 100, y = 500 },

    -- Enemy patrol area
    { type = "enemy", x = 400, y = 500, patrolRange = 100 }
  }
}
```

---

## API Reference

### Lifecycle

#### `void update(DeltaTime dt)`

Updates the level system. Must be called every frame.

**Responsibilities**:
- Processes pending level transitions
- Updates async operations (future)

**Parameters**:
- `dt` - Delta time since last frame

**Usage**:

```cpp
void Game::update(float dt) {
    levels->update(dt);
    // ... other updates
}
```

### Level Management

#### `Result<LevelId, std::error_code> loadLevel(AssetHandle levelAsset)`

Loads a level from a registered asset.

**Parameters**:
- `levelAsset` - Asset handle for a level file (from AssetSystem)

**Returns**:
- `Result` containing `LevelId` on success, or `std::error_code` on failure

**Behavior**:
- Parses the Lua file
- Extracts metadata, spawn points, and entity definitions
- Assigns a unique `LevelId`
- State is set to `LevelState::Loaded`

**Usage**:

```cpp
// Register the level asset
AssetHandle levelAsset = assets->registerAsset(AssetType::Level, ":assets:/levels/level1.lua");

// Load the level
auto result = levels->loadLevel(levelAsset);
if (result.has_value()) {
    LevelId levelId = result.value();
    // Level loaded successfully
} else {
    std::error_code error = result.error();
    // Handle error
}
```

#### `void unloadLevel(LevelId levelId)`

Unloads a level from memory.

**Parameters**:
- `levelId` - ID of the level to unload

**Behavior**:
- Removes level from memory
- If level is active, clears active level
- Safe to call on already unloaded levels (no-op)
- Safe to call on invalid level IDs (no-op)

**Usage**:

```cpp
levels->unloadLevel(levelId);
```

#### `void setActiveLevel(LevelId levelId)`

Sets the currently active level.

**Parameters**:
- `levelId` - ID of the level to activate

**Behavior**:
- Sets the active level immediately
- If `levelId` is invalid or unloaded, does nothing
- Does not unload previous active level

**Usage**:

```cpp
levels->setActiveLevel(levelId);
```

### Level Transitions

#### `void transition(const LevelTransition& transition)`

Queues a level transition.

**Parameters**:
- `transition` - Transition description

**Behavior**:
- Transition is **queued**, not executed immediately
- Transition executes on next `update()` call
- If multiple transitions are queued, only the last applies

**Usage**:

```cpp
LevelTransition transition{
    .fromLevel = currentLevel,
    .toLevel = nextLevel,
    .spawnPoint = "checkpoint1",
    .unloadPrevious = true
};

levels->transition(transition);
// Transition happens on next update()
```

**Typical transition workflow**:

```cpp
// 1. Load destination level
auto result = levels->loadLevel(nextLevelAsset);
if (!result.has_value()) {
    return; // Handle error
}
LevelId nextLevel = result.value();

// 2. Queue transition
LevelTransition transition{
    .fromLevel = currentLevel,
    .toLevel = nextLevel,
    .spawnPoint = "default",
    .unloadPrevious = true  // Unload current level
};
levels->transition(transition);

// 3. On next update(), transition executes:
//    - Active level switches to nextLevel
//    - If unloadPrevious=true, currentLevel is unloaded
//    - Game code can read spawnPoint and position player
```

### State Queries

#### `std::optional<LevelId> getActiveLevel() const`

Returns the currently active level.

**Returns**:
- `std::optional<LevelId>` containing active level ID, or `std::nullopt` if no active level

**Usage**:

```cpp
if (auto levelId = levels->getActiveLevel()) {
    // levelId.value() is the active level
} else {
    // No active level
}
```

#### `LevelState getLevelState(LevelId levelId) const`

Returns the state of a level.

**Parameters**:
- `levelId` - Level to query

**Returns**:
- `LevelState` (returns `LevelState::Unloaded` for invalid IDs)

**Usage**:

```cpp
LevelState state = levels->getLevelState(levelId);
if (state == LevelState::Loaded) {
    // Level is ready
}
```

#### `LevelMetadata getLevelMetadata(LevelId levelId) const`

Returns metadata for a level.

**Parameters**:
- `levelId` - Level to query

**Returns**:
- `LevelMetadata` (returns default-constructed metadata for invalid IDs)

**Usage**:

```cpp
LevelMetadata meta = levels->getLevelMetadata(levelId);
std::cout << "Level: " << meta.levelName << "\n";
std::cout << "Size: " << meta.width << "x" << meta.height << "\n";
```

#### `std::vector<LevelMetadata> getLoadedLevels() const`

Returns metadata for all loaded levels.

**Returns**:
- `std::vector<LevelMetadata>` containing all loaded levels

**Usage**:

```cpp
auto loadedLevels = levels->getLoadedLevels();
std::cout << "Loaded levels: " << loadedLevels.size() << "\n";
for (const auto& meta : loadedLevels) {
    std::cout << "  - " << meta.levelName << " (ID: " << meta.id << ")\n";
}
```

### Spawn Points

#### `std::optional<Transform2D> getSpawnPoint(LevelId levelId, const std::string& name) const`

Returns a spawn point by name.

**Parameters**:
- `levelId` - Level containing the spawn point
- `name` - Name of the spawn point

**Returns**:
- `std::optional<Transform2D>` containing spawn position/rotation, or `std::nullopt` if not found

**Usage**:

```cpp
if (auto spawn = levels->getSpawnPoint(levelId, "checkpoint1")) {
    // Position player at spawn point
    player.x = spawn->x;
    player.y = spawn->y;
    player.rotation = spawn->rotation;
}
```

#### `std::vector<std::string> getSpawnPointNames(LevelId levelId) const`

Returns all spawn point names in a level.

**Parameters**:
- `levelId` - Level to query

**Returns**:
- `std::vector<std::string>` containing spawn point names

**Usage**:

```cpp
auto spawnNames = levels->getSpawnPointNames(levelId);
for (const auto& name : spawnNames) {
    std::cout << "Spawn point: " << name << "\n";
}
```

### Level Queries

#### `std::vector<Entity> getLevelEntities(LevelId levelId) const`

Returns all spawned entities for a level.

**Parameters**:
- `levelId` - Level to query

**Returns**:
- `std::vector<Entity>` containing entity IDs

**Note**: This returns entities that have been spawned by game code, not `EntityDef` definitions. The Level System does not spawn entities automatically.

**Usage**:

```cpp
auto entities = levels->getLevelEntities(levelId);
for (Entity entity : entities) {
    // Do something with entity
}
```

### Entity Definitions

#### `std::vector<EntityDef> getEntityDefs(LevelId levelId) const`

Returns entity definitions from the level file.

**Parameters**:
- `levelId` - Level to query

**Returns**:
- `std::vector<EntityDef>` containing entity definitions

**Usage**:

```cpp
auto entityDefs = levels->getEntityDefs(levelId);
for (const auto& def : entityDefs) {
    // Spawn entity based on definition
    Entity entity = spawnEntity(def);
}
```

---

## Entity Spawning

### Understanding EntityDef

The Level System **parses** entity definitions from Lua, but **does not spawn entities**. Game code is responsible for spawning entities based on `EntityDef` data.

**Why?** This gives game code full control over entity creation, blueprint application, and component assignment.

### Spawning Workflow

```cpp
// 1. Load level
auto result = levels->loadLevel(levelAsset);
if (!result.has_value()) return;
LevelId levelId = result.value();

// 2. Get entity definitions
auto entityDefs = levels->getEntityDefs(levelId);

// 3. Spawn entities
for (const auto& def : entityDefs) {
    Entity entity = entities->createEntity();

    // Apply transform
    entities->emplace<Transform>(entity, def.transform);

    // Apply blueprint (if using BlueprintSystem)
    if (blueprints->hasBlueprint(def.type)) {
        blueprints->applyBlueprint(entity, def.type);
    }

    // Apply custom properties
    for (const auto& [key, value] : def.properties) {
        applyProperty(entity, key, value);
    }
}
```

### Reading Custom Properties

Custom properties are stored as `std::any`. Use `std::any_cast<T>()` to extract values:

```cpp
void applyProperty(Entity entity, const std::string& key, const std::any& value) {
    try {
        // Try as double (all Lua numbers)
        if (key == "speed") {
            double speed = std::any_cast<double>(value);
            entities->emplace<Speed>(entity, static_cast<float>(speed));
        }
        // Try as bool
        else if (key == "hostile") {
            bool hostile = std::any_cast<bool>(value);
            entities->emplace<Hostile>(entity, hostile);
        }
        // Try as string
        else if (key == "message") {
            std::string message = std::any_cast<std::string>(value);
            entities->emplace<Message>(entity, message);
        }
    } catch (const std::bad_any_cast& e) {
        // Handle type mismatch
    }
}
```

### Blueprint Integration

If using the Blueprint System, entity types can correspond to blueprints:

```cpp
// Lua level file
entities = {
  { type = "player", x = 100, y = 500 },
  { type = "enemy_slime", x = 400, y = 500 }
}
```

```cpp
// C++ game code
for (const auto& def : entityDefs) {
    Entity entity = entities->createEntity();

    // Apply blueprint if it exists
    if (blueprints->hasBlueprint(def.type)) {
        blueprints->applyBlueprint(entity, def.type);
    }

    // Override transform from level
    entities->emplace<Transform>(entity, def.transform);
}
```

---

## Best Practices

### Level Organization

**File structure**:

```
assets/levels/
  world1/
    level1.lua
    level2.lua
    level3.lua
  world2/
    level1.lua
    level2.lua
  shared/
    helpers.lua    -- Shared functions
```

**Shared helpers**:

```lua
-- assets/levels/shared/helpers.lua
local M = {}

function M.createPlatform(x, y, width, height)
  return {
    type = "platform",
    x = x,
    y = y,
    width = width,
    height = height
  }
end

return M
```

```lua
-- assets/levels/world1/level1.lua
local helpers = require("shared.helpers")

return {
  name = "World 1 - Level 1",
  width = 1920,
  height = 1080,

  entities = {
    helpers.createPlatform(0, 1000, 1920, 80)
  }
}
```

### Procedural Level Generation

**Pattern generation**:

```lua
local function createEnemyWave(startX, startY, count, spacing)
  local entities = {}
  for i = 1, count do
    table.insert(entities, {
      type = "enemy",
      x = startX + (i - 1) * spacing,
      y = startY,
      speed = 50
    })
  end
  return entities
end

return {
  name = "Enemy Waves",
  width = 3000,
  height = 1080,

  entities = function()
    local entities = {}

    -- Wave 1
    for _, enemy in ipairs(createEnemyWave(500, 500, 5, 100)) do
      table.insert(entities, enemy)
    end

    -- Wave 2
    for _, enemy in ipairs(createEnemyWave(1500, 400, 8, 80)) do
      table.insert(entities, enemy)
    end

    return entities
  end
}
```

**Randomized placement**:

```lua
math.randomseed(12345)  -- Fixed seed for reproducibility

return {
  name = "Random Collectibles",
  width = 2000,
  height = 1080,

  entities = function()
    local entities = {}

    -- 20 random collectibles
    for i = 1, 20 do
      table.insert(entities, {
        type = "collectible",
        x = math.random(100, 1900),
        y = math.random(100, 1000),
        value = math.random(1, 10)
      })
    end

    return entities
  end
}
```

### Level Streaming Considerations

**Pre-loading next level**:

```cpp
void Game::update(float dt) {
    // Check if player is near level exit
    if (player.x > levelWidth - 200) {
        if (!nextLevelLoaded) {
            // Pre-load next level
            auto result = levels->loadLevel(nextLevelAsset);
            if (result.has_value()) {
                nextLevelId = result.value();
                nextLevelLoaded = true;
            }
        }
    }

    // Trigger transition when player reaches exit
    if (player.x > levelWidth) {
        LevelTransition transition{
            .fromLevel = currentLevelId,
            .toLevel = nextLevelId,
            .spawnPoint = "default",
            .unloadPrevious = true
        };
        levels->transition(transition);
    }
}
```

**Multi-level loading** (e.g., for persistent game world):

```cpp
// Load multiple levels but only one is active
auto level1 = levels->loadLevel(level1Asset);
auto level2 = levels->loadLevel(level2Asset);
auto level3 = levels->loadLevel(level3Asset);

// Set one as active
levels->setActiveLevel(level1.value());

// Transition without unloading
LevelTransition transition{
    .fromLevel = level1.value(),
    .toLevel = level2.value(),
    .spawnPoint = "default",
    .unloadPrevious = false  // Keep level1 loaded
};
levels->transition(transition);
```

### Hot Reload Support

**Development workflow**:

1. Enable hot reload in AssetSystem
2. Edit level Lua file
3. Level automatically reloads
4. Re-spawn entities

```cpp
// Enable hot reload for development
assets->enableHotReload(true);

// Subscribe to level asset changes
SubscriptionId subId = assets->subscribeToType(AssetType::Level, [this](AssetHandle handle, AssetType type) {
    // Find level by asset handle
    for (const auto& meta : levels->getLoadedLevels()) {
        if (meta.assetHandle == handle) {
            // Level changed - reload it
            reloadLevel(meta.id);
            break;
        }
    }
});

void Game::reloadLevel(LevelId levelId) {
    // Destroy existing entities
    auto entities = levels->getLevelEntities(levelId);
    for (Entity entity : entities) {
        entitySystem->destroyEntity(entity);
    }

    // Re-spawn entities
    spawnLevelEntities(levelId);
}
```

### Error Handling

**Check load results**:

```cpp
auto result = levels->loadLevel(levelAsset);
if (!result.has_value()) {
    std::error_code error = result.error();
    std::cerr << "Failed to load level: " << error.message() << "\n";
    // Fallback to default level or show error screen
    return;
}
```

**Validate spawn points**:

```cpp
auto spawn = levels->getSpawnPoint(levelId, spawnPointName);
if (!spawn.has_value()) {
    // Fallback to default spawn
    spawn = levels->getSpawnPoint(levelId, "default");
    if (!spawn.has_value()) {
        // Use hardcoded fallback
        spawn = Transform2D{100.0f, 500.0f, 0.0f};
    }
}

player.x = spawn->x;
player.y = spawn->y;
```

---

## Code Examples

### Example 1: Complete Level File

```lua
-- assets/levels/world1/level1.lua

local GROUND_Y = 1000
local PLAYER_SPAWN_Y = 500

local function createPlatform(x, y, w, h)
  return {
    type = "platform",
    x = x,
    y = y,
    width = w,
    height = h
  }
end

return {
  -- Level metadata
  name = "Forest Entrance",
  width = 2560,
  height = 1080,

  -- Named spawn points
  spawnPoints = {
    default = { x = 100, y = PLAYER_SPAWN_Y },
    checkpoint1 = { x = 800, y = 400 },
    checkpoint2 = { x = 1600, y = 300 },
    boss_entrance = { x = 2400, y = PLAYER_SPAWN_Y, rotation = 180 }
  },

  -- Entity definitions
  entities = {
    -- Ground platform
    createPlatform(0, GROUND_Y, 2560, 80),

    -- Floating platforms
    createPlatform(300, 800, 200, 40),
    createPlatform(700, 650, 200, 40),
    createPlatform(1100, 500, 200, 40),

    -- Enemies
    { type = "enemy_slime", x = 400, y = 950, patrolRange = 150 },
    { type = "enemy_slime", x = 900, y = 950, patrolRange = 200 },
    { type = "enemy_bat", x = 1200, y = 400, patrolRange = 300 },

    -- Collectibles
    { type = "coin", x = 350, y = 750, value = 1 },
    { type = "coin", x = 750, y = 600, value = 1 },
    { type = "gem", x = 1150, y = 450, value = 10 },

    -- Level exit trigger
    { type = "level_exit", x = 2500, y = 900, targetLevel = "world1_level2" }
  }
}
```

### Example 2: Loading and Activating a Level

```cpp
#include <iostream>

void Game::loadAndStartLevel(const std::string& levelPath) {
    // Register level asset
    AssetHandle levelAsset = assets->registerAsset(AssetType::Level, levelPath);

    // Load level
    auto result = levels->loadLevel(levelAsset);
    if (!result.has_value()) {
        std::cerr << "Failed to load level: " << result.error().message() << "\n";
        return;
    }

    LevelId levelId = result.value();

    // Print level metadata
    LevelMetadata meta = levels->getLevelMetadata(levelId);
    std::cout << "Loaded: " << meta.levelName << "\n";
    std::cout << "Size: " << meta.width << "x" << meta.height << "\n";

    // Set as active
    levels->setActiveLevel(levelId);

    // Spawn entities
    spawnLevelEntities(levelId);

    // Position player at default spawn
    positionPlayerAtSpawn(levelId, "default");
}
```

### Example 3: Spawning Entities

```cpp
void Game::spawnLevelEntities(LevelId levelId) {
    auto entityDefs = levels->getEntityDefs(levelId);

    std::cout << "Spawning " << entityDefs.size() << " entities...\n";

    for (const auto& def : entityDefs) {
        // Create entity
        Entity entity = entities->createEntity();

        // Apply transform
        entities->emplace<Transform>(entity, def.transform);

        // Apply blueprint if available
        if (blueprints->hasBlueprint(def.type)) {
            blueprints->applyBlueprint(entity, def.type);
            std::cout << "  - Spawned " << def.type << " at (" << def.transform.x << ", " << def.transform.y << ")\n";
        } else {
            std::cout << "  - Warning: No blueprint for type '" << def.type << "'\n";
        }

        // Apply custom properties
        applyCustomProperties(entity, def.properties);
    }
}

void Game::applyCustomProperties(Entity entity, const std::unordered_map<std::string, std::any>& props) {
    for (const auto& [key, value] : props) {
        try {
            // Handle numeric properties
            if (value.type() == typeid(double)) {
                double numValue = std::any_cast<double>(value);

                if (key == "patrolRange") {
                    entities->emplace<PatrolRange>(entity, static_cast<float>(numValue));
                } else if (key == "speed") {
                    entities->emplace<Speed>(entity, static_cast<float>(numValue));
                } else if (key == "value") {
                    entities->emplace<Value>(entity, static_cast<int>(numValue));
                }
            }
            // Handle boolean properties
            else if (value.type() == typeid(bool)) {
                bool boolValue = std::any_cast<bool>(value);

                if (key == "hostile") {
                    entities->emplace<Hostile>(entity, boolValue);
                }
            }
            // Handle string properties
            else if (value.type() == typeid(std::string)) {
                std::string strValue = std::any_cast<std::string>(value);

                if (key == "targetLevel") {
                    entities->emplace<LevelExit>(entity, strValue);
                }
            }
        } catch (const std::bad_any_cast& e) {
            std::cerr << "Failed to cast property '" << key << "'\n";
        }
    }
}
```

### Example 4: Using Spawn Points

```cpp
void Game::positionPlayerAtSpawn(LevelId levelId, const std::string& spawnName) {
    auto spawn = levels->getSpawnPoint(levelId, spawnName);

    if (!spawn.has_value()) {
        std::cerr << "Spawn point '" << spawnName << "' not found, using 'default'\n";
        spawn = levels->getSpawnPoint(levelId, "default");
    }

    if (spawn.has_value()) {
        // Position player
        auto& transform = entities->get<Transform>(playerEntity);
        transform.x = spawn->x;
        transform.y = spawn->y;
        transform.rotation = spawn->rotation;

        std::cout << "Player spawned at " << spawnName << " (" << spawn->x << ", " << spawn->y << ")\n";
    } else {
        std::cerr << "No valid spawn point found!\n";
    }
}
```

### Example 5: Level Transitions

```cpp
void Game::transitionToNextLevel(const std::string& nextLevelPath, const std::string& spawnPoint) {
    // Get current level
    auto currentLevel = levels->getActiveLevel();
    if (!currentLevel.has_value()) {
        std::cerr << "No active level to transition from\n";
        return;
    }

    // Register and load next level
    AssetHandle nextAsset = assets->registerAsset(AssetType::Level, nextLevelPath);
    auto result = levels->loadLevel(nextAsset);

    if (!result.has_value()) {
        std::cerr << "Failed to load next level\n";
        return;
    }

    LevelId nextLevel = result.value();

    // Queue transition
    LevelTransition transition{
        .fromLevel = currentLevel.value(),
        .toLevel = nextLevel,
        .spawnPoint = spawnPoint,
        .unloadPrevious = true  // Unload old level
    };

    levels->transition(transition);

    // Transition executes on next update()
    // After transition, spawn entities and position player
}

void Game::update(float dt) {
    // Update level system (processes transitions)
    levels->update(dt);

    // Check if level changed
    auto currentLevel = levels->getActiveLevel();
    if (currentLevel != lastActiveLevel) {
        std::cout << "Level changed!\n";

        // Spawn entities for new level
        spawnLevelEntities(currentLevel.value());

        // Position player at spawn point (if transition specified one)
        // Game code needs to track the spawn point from the transition

        lastActiveLevel = currentLevel;
    }

    // ... rest of game update
}
```

### Example 6: Level Streaming (Pre-loading)

```cpp
void Game::updateLevelStreaming(float dt) {
    auto currentLevel = levels->getActiveLevel();
    if (!currentLevel.has_value()) return;

    // Get player position
    auto& playerTransform = entities->get<Transform>(playerEntity);

    // Get level metadata
    LevelMetadata meta = levels->getLevelMetadata(currentLevel.value());

    // Check if player is approaching level exit
    float distanceToExit = meta.width - playerTransform.x;

    if (distanceToExit < 500.0f && !nextLevelPreloaded) {
        std::cout << "Pre-loading next level...\n";

        // Load next level in background
        AssetHandle nextAsset = assets->registerAsset(AssetType::Level, nextLevelPath);
        auto result = levels->loadLevel(nextAsset);

        if (result.has_value()) {
            nextLevelId = result.value();
            nextLevelPreloaded = true;
            std::cout << "Next level pre-loaded\n";
        }
    }

    // Trigger transition when player reaches exit
    if (distanceToExit < 50.0f && nextLevelPreloaded) {
        std::cout << "Transitioning to next level...\n";

        LevelTransition transition{
            .fromLevel = currentLevel.value(),
            .toLevel = nextLevelId,
            .spawnPoint = "default",
            .unloadPrevious = true
        };

        levels->transition(transition);
        nextLevelPreloaded = false;  // Reset flag
    }
}
```

### Example 7: Debugging Level Data

```cpp
void Game::debugPrintLevel(LevelId levelId) {
    // Print metadata
    LevelMetadata meta = levels->getLevelMetadata(levelId);
    std::cout << "=== Level Debug Info ===\n";
    std::cout << "Name: " << meta.levelName << "\n";
    std::cout << "Size: " << meta.width << "x" << meta.height << "\n";
    std::cout << "State: " << static_cast<int>(meta.state) << "\n";

    // Print spawn points
    auto spawnNames = levels->getSpawnPointNames(levelId);
    std::cout << "\nSpawn Points (" << spawnNames.size() << "):\n";
    for (const auto& name : spawnNames) {
        auto spawn = levels->getSpawnPoint(levelId, name);
        if (spawn.has_value()) {
            std::cout << "  - " << name << ": (" << spawn->x << ", " << spawn->y << ") rot=" << spawn->rotation << "\n";
        }
    }

    // Print entity definitions
    auto entityDefs = levels->getEntityDefs(levelId);
    std::cout << "\nEntity Definitions (" << entityDefs.size() << "):\n";
    for (size_t i = 0; i < entityDefs.size(); ++i) {
        const auto& def = entityDefs[i];
        std::cout << "  [" << i << "] " << def.type << " at (" << def.transform.x << ", " << def.transform.y << ")\n";

        // Print custom properties
        if (!def.properties.empty()) {
            std::cout << "      Properties:\n";
            for (const auto& [key, value] : def.properties) {
                std::cout << "        " << key << " = ";

                if (value.type() == typeid(double)) {
                    std::cout << std::any_cast<double>(value);
                } else if (value.type() == typeid(bool)) {
                    std::cout << (std::any_cast<bool>(value) ? "true" : "false");
                } else if (value.type() == typeid(std::string)) {
                    std::cout << "\"" << std::any_cast<std::string>(value) << "\"";
                }
                std::cout << "\n";
            }
        }
    }

    std::cout << "========================\n";
}
```

---

## Summary

The Level System provides a Lua-first approach to level design:

- **Lua-based levels** - Designers work in Lua, not C++
- **Flexible entity definitions** - Custom properties for any entity type
- **Named spawn points** - Easy player/entity positioning
- **Level transitions** - Seamless level switching with optional unloading
- **AssetSystem integration** - Automatic hot reload support
- **Procedural generation** - Use Lua functions, loops, and math

**Key takeaways**:

1. Levels are parsed from Lua, returning `EntityDef` definitions
2. Game code is responsible for spawning entities from definitions
3. Transitions are queued and execute on `update()`
4. Always provide a `"default"` spawn point
5. All Lua numbers are `double` - cast appropriately
6. Use Lua features (loops, functions, math) for complex level generation

For more information, see:
- `bestow-contract/src/bestow.level.cppm` - Interface definition
- `tests/unit/LevelSystemTests.cpp` - Usage examples
- `tests/testdata/*.lua` - Example level files
