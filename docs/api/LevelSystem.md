# LevelSystem API

The `LevelSystem` provides Lua-based level loading, transitions, and management.

## Overview

```cpp
auto& levels = sys.levels;
auto& assets = sys.assets;

// Register level asset
AssetHandle levelAsset = assets->registerAsset(
    AssetType::Level, "levels/level1.lua"
);
assets->loadAsset(levelAsset);

// Load level
auto levelId = levels->loadLevel(levelAsset);
if (levelId) {
    levels->setActiveLevel(*levelId);
}

// Get spawn point
auto spawn = levels->getSpawnPoint(*levelId, "player_start");
if (spawn) {
    physics->setPosition(player, spawn->position());
}
```

## Level Management

### loadLevel(AssetHandle levelAsset)

```cpp
Result<LevelId, std::error_code> loadLevel(AssetHandle levelAsset);
```

Loads a level from a Lua file asset.

**Returns:** `LevelId` on success, error code on failure

**Example:**

```cpp
AssetHandle level1 = assets->registerAsset(
    AssetType::Level, "levels/level1.lua"
);
assets->loadAsset(level1);

auto result = levels->loadLevel(level1);
if (!result) {
    logError("Failed to load level");
    return;
}

LevelId levelId = *result;
```

---

### unloadLevel(LevelId levelId)

```cpp
void unloadLevel(LevelId levelId);
```

Unloads a level and destroys all its entities.

---

### setActiveLevel(LevelId levelId)

```cpp
void setActiveLevel(LevelId levelId);
```

Makes a loaded level active. Only one level can be active at a time.

---

## Level Transitions

### transition(const LevelTransition& transition)

```cpp
void transition(const LevelTransition& transition);
```

Performs a level transition.

**LevelTransition Structure:**

```cpp
struct LevelTransition {
    LevelId fromLevel;
    LevelId toLevel;
    std::optional<std::string> spawnPoint;
    bool unloadPrevious = true;
};
```

**Example:**

```cpp
// Transition to next level at "entrance" spawn
levels->transition({
    .fromLevel = currentLevel,
    .toLevel = nextLevel,
    .spawnPoint = "entrance",
    .unloadPrevious = true
});
```

---

## State Queries

### getActiveLevel()

```cpp
std::optional<LevelId> getActiveLevel() const;
```

Returns the currently active level, or `std::nullopt` if none.

---

### getLevelState(LevelId levelId)

```cpp
LevelState getLevelState(LevelId levelId) const;
```

Returns the state of a level.

**LevelState Enum:**

```cpp
enum class LevelState : uint8_t {
    Unloaded,
    Loading,
    Loaded,
    Active,
    Unloading
};
```

---

### getLevelMetadata(LevelId levelId)

```cpp
LevelMetadata getLevelMetadata(LevelId levelId) const;
```

Returns metadata about a level.

**LevelMetadata Structure:**

```cpp
struct LevelMetadata {
    LevelId id;
    AssetHandle assetHandle;
    std::string levelName;
    LevelState state = LevelState::Unloaded;
    float width = 0.0f;
    float height = 0.0f;
};
```

---

### getLoadedLevels()

```cpp
std::vector<LevelMetadata> getLoadedLevels() const;
```

Returns metadata for all loaded levels.

---

## Spawn Points

### getSpawnPoint(LevelId levelId, const std::string& name)

```cpp
std::optional<Transform2D> getSpawnPoint(LevelId levelId,
                                         const std::string& name) const;
```

Returns a spawn point by name.

**Example:**

```cpp
if (auto spawn = levels->getSpawnPoint(levelId, "player_start")) {
    physics->setPosition(player, spawn->position());
}

// Respawn at checkpoint
if (auto checkpoint = levels->getSpawnPoint(levelId, "checkpoint_1")) {
    physics->setPosition(player, checkpoint->position());
}
```

---

### getSpawnPointNames(LevelId levelId)

```cpp
std::vector<std::string> getSpawnPointNames(LevelId levelId) const;
```

Returns all spawn point names in a level.

---

## Level Queries

### getLevelEntities(LevelId levelId)

```cpp
std::vector<Entity> getLevelEntities(LevelId levelId) const;
```

Returns all entities that belong to a level.

**Use for:** Cleaning up level-specific entities

---

### getEntityDefs(LevelId levelId)

```cpp
std::vector<EntityDef> getEntityDefs(LevelId levelId) const;
```

Returns the entity definitions from the level file.

**EntityDef Structure:**

```cpp
struct EntityDef {
    std::string type;  // "platform", "enemy", "collectible", etc.
    Transform2D transform;
    std::unordered_map<std::string, std::any> properties;
};
```

---

## Update Loop

### update(DeltaTime dt)

```cpp
void update(DeltaTime dt);
```

Updates the level system. Call once per frame.

---

## Lua Level Format

Level files are written in Lua and return a table:

```lua
-- levels/level1.lua

return {
    name = "The Beginning",
    width = 3200,
    height = 720,

    spawnPoints = {
        player_start = { x = 100, y = 500 },
        checkpoint_1 = { x = 800, y = 500 },
        checkpoint_2 = { x = 1600, y = 400 }
    },

    entities = {
        -- Platform
        {
            type = "platform",
            x = 400,
            y = 600,
            width = 200,
            height = 32,
            properties = {
                texture = "textures/stone.png"
            }
        },

        -- Enemy
        {
            type = "enemy_slime",
            x = 600,
            y = 550,
            properties = {
                patrolDistance = 100,
                speed = 50
            }
        },

        -- Collectible coin
        {
            type = "coin",
            x = 500,
            y = 400
        }
    },

    -- Level-specific configuration
    config = {
        gravity = 980,
        backgroundColor = {r = 135, g = 206, b = 235},
        musicTrack = "music/level1.ogg"
    }
}
```

---

## Level Events

Level events are published to the event system:

```cpp
namespace Events {
    inline constexpr const char* LevelLoaded = "level_loaded";
    inline constexpr const char* LevelUnloaded = "level_unloaded";
}
```

**Subscribe to events:**

```cpp
events->subscribe(Events::LevelLoaded, [](const EventData& data) {
    auto& levelEvent = std::get<LevelEventData>(data);
    // Handle level load
});
```

**LevelEventData:**

```cpp
struct LevelEventData {
    LevelId levelId;
    LevelEvent event;
};

enum class LevelEvent : uint8_t {
    LoadStarted,
    LoadCompleted,
    UnloadStarted,
    UnloadCompleted,
    Activated,
    Deactivated
};
```

---

## Common Patterns

### Loading First Level

```cpp
bool initialize(Engine& engine) override {
    auto& sys = engine.systems();

    // Register and load level
    AssetHandle level1 = sys.assets->registerAsset(
        AssetType::Level, "levels/level1.lua"
    );
    sys.assets->loadAsset(level1);

    auto levelId = sys.levels->loadLevel(level1);
    if (!levelId) {
        logError("Failed to load level");
        return false;
    }

    sys.levels->setActiveLevel(*levelId);

    // Spawn player at start
    if (auto spawn = sys.levels->getSpawnPoint(*levelId, "player_start")) {
        player_ = createPlayer(spawn->x, spawn->y);
    }

    // Start level music
    auto metadata = sys.levels->getLevelMetadata(*levelId);
    // Load and play music from level config...

    return true;
}
```

---

### Level Transition with Door Trigger

```cpp
struct Door {
    std::string targetLevel;
    std::string targetSpawn;
};

// Create door entity
Entity createDoor(float x, float y, const Door& door) {
    Entity entity = entities->createEntity();
    entities->emplace<Transform2D>(entity, x, y);
    entities->emplace<Door>(entity, door);

    // Trigger volume
    PhysicsBodyDef doorDef{
        .type = BodyType::Static,
        .transform = {.x = x, .y = y},
        .size = {64.0f, 128.0f},
        .isSensor = true
    };
    physics->createBody(entity, doorDef);
    physics->setCollisionLayer(entity, CollisionLayers::Trigger);

    return entity;
}

// Handle trigger
events->subscribe(Events::TriggerEnter, [this](const EventData& data) {
    auto& trigger = std::get<TriggerEvent>(data);

    if (!entities->allOf<Player>(trigger.entityA)) return;
    if (!entities->allOf<Door>(trigger.entityB)) return;

    auto& door = entities->get<Door>(trigger.entityB);

    // Load target level
    AssetHandle nextLevelAsset = assets->registerAsset(
        AssetType::Level, "levels/" + door.targetLevel + ".lua"
    );
    assets->loadAsset(nextLevelAsset);

    auto nextLevelId = levels->loadLevel(nextLevelAsset);
    if (nextLevelId) {
        levels->transition({
            .fromLevel = *levels->getActiveLevel(),
            .toLevel = *nextLevelId,
            .spawnPoint = door.targetSpawn,
            .unloadPrevious = true
        });
    }
});
```

---

### Procedural Level Generation

```lua
-- levels/procedural.lua

local function generatePlatforms(startX, endX, groundY)
    local platforms = {}
    local x = startX

    while x < endX do
        table.insert(platforms, {
            type = "platform",
            x = x,
            y = groundY,
            width = math.random(100, 300),
            height = 32
        })
        x = x + math.random(150, 400)
    end

    return platforms
end

local function generateEnemies(count, minX, maxX, y)
    local enemies = {}
    for i = 1, count do
        table.insert(enemies, {
            type = "enemy_slime",
            x = math.random(minX, maxX),
            y = y,
            properties = {
                patrolDistance = math.random(50, 150)
            }
        })
    end
    return enemies
end

return {
    name = "Procedural Level",
    width = 5000,
    height = 720,

    spawnPoints = {
        player_start = { x = 100, y = 600 }
    },

    entities = function()
        local ents = {}

        -- Generate platforms
        local platforms = generatePlatforms(0, 5000, 650)
        for _, p in ipairs(platforms) do
            table.insert(ents, p)
        end

        -- Generate enemies
        local enemies = generateEnemies(10, 200, 4800, 600)
        for _, e in ipairs(enemies) do
            table.insert(ents, e)
        end

        return ents
    end()
}
```

---

### Multi-Level Loading (Streaming)

```cpp
// Preload adjacent levels for seamless transitions
void preloadAdjacentLevels(LevelId currentLevel) {
    auto adjacentLevels = getAdjacentLevels(currentLevel);

    for (const auto& levelPath : adjacentLevels) {
        AssetHandle levelAsset = assets->registerAsset(
            AssetType::Level, levelPath
        );
        assets->loadAssetAsync(levelAsset, [this, levelAsset](auto h, auto state) {
            if (state == AssetState::Loaded) {
                levels->loadLevel(levelAsset);
            }
        });
    }
}

// Keep multiple levels loaded
void transitionWithStreaming(LevelId nextLevel) {
    levels->setActiveLevel(nextLevel);
    // Don't unload previous level yet

    // Unload distant levels
    for (auto& metadata : levels->getLoadedLevels()) {
        if (isDistant(metadata.id, nextLevel)) {
            levels->unloadLevel(metadata.id);
        }
    }

    // Preload next adjacent levels
    preloadAdjacentLevels(nextLevel);
}
```

---

## Performance Tips

1. **Preload adjacent levels** - Stream in background for seamless transitions
2. **Unload unused levels** - Free memory when far away
3. **Use blueprint factory** - Let blueprints handle entity creation
4. **Cache spawn points** - Store frequently used spawn locations
5. **Lua functions for generation** - Use Lua logic for procedural content

## See Also

- [BlueprintFactory](BlueprintFactory.md) - Creating entities from level data
- [AssetSystem](AssetSystem.md) - Loading level files
- [EventSystem](EventSystem.md) - Level load events
- [Data-Driven Design](../Data-Driven-Design.md) - Level file format guide
