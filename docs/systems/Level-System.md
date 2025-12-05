# Bestow Level System Documentation

## Overview

The Level System (`ILevelSystem`) manages level loading, entity spawning, spawn points, and level transitions in Bestow. Levels are defined in Lua files for flexibility and ease of content authoring. The system parses these Lua files to extract level metadata, entity definitions, and spawn point locations.

### Key Features

- Lua-based level definitions with variables, functions, and procedural generation
- Multiple simultaneously loaded levels
- Level transitions with optional previous level unloading
- Named spawn points for entity placement
- Entity definition parsing with custom properties
- Level state management

### Dependencies

- **Asset System**: Required to load level Lua files
- **Entity System**: Used by game code to spawn entities from level definitions
- **Events System**: Can publish level lifecycle events

## Architecture

### Level States

```cpp
enum class LevelState : std::uint8_t {
    Unloaded,    // Level not in memory
    Loading,     // Asynchronous load in progress (future feature)
    Loaded,      // Level data loaded and ready
    Active       // Level is currently being played
};
```

### Core Types

```cpp
struct LevelMetadata {
    LevelId id;                       // Unique identifier
    AssetHandle assetHandle;          // Reference to source asset
    std::string levelName;            // Human-readable name
    LevelState state;                 // Current state
    float width;                      // Level bounds
    float height;
};

struct LevelTransition {
    LevelId fromLevel;                // Source level ID
    LevelId toLevel;                  // Target level ID
    std::optional<std::string> spawnPoint;  // Where to spawn player
    bool unloadPrevious;              // Unload source level after transition
};

struct EntityDef {
    std::string type;                 // Entity type identifier
    Transform2D transform;            // Initial position/rotation/scale
    std::unordered_map<std::string, std::any> properties;  // Custom data
};
```

## Defining Levels in Lua

### Basic Level Structure

Every level Lua file must return a table with metadata and entity definitions:

```lua
-- levels/tutorial.lua
return {
    name = "Tutorial Level",
    width = 1920,
    height = 1080,

    spawnPoints = {
        default = { x = 100, y = 500, rotation = 0 }
    },

    entities = {
        -- Entity definitions here
    }
}
```

### Level Metadata

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `name` | string | No | Human-readable level name |
| `width` | number | No | Level width in pixels |
| `height` | number | No | Level height in pixels |

Additional custom fields can be added and accessed via the parsed metadata.

### Spawn Points

Spawn points define named locations where entities (typically the player) can be placed:

```lua
spawnPoints = {
    default = { x = 100, y = 500, rotation = 0 },
    checkpoint1 = { x = 500, y = 400, rotation = 0 },
    checkpoint2 = { x = 1000, y = 300, rotation = 0 },
    boss_room = { x = 1800, y = 600, rotation = 180 },
    secret_area = { x = 200, y = 100, rotation = 90 }
}
```

Each spawn point is a table with:
- `x` (number): X position
- `y` (number): Y position
- `rotation` (number, optional): Rotation in degrees (default: 0)

### Entity Definitions

Entities are defined in the `entities` array. Each entity must have a `type` field:

```lua
entities = {
    {
        type = "platform",
        x = 0,
        y = 550,
        width = 800,
        height = 50
    },
    {
        type = "enemy",
        x = 400,
        y = 500,
        patrolRange = 100,
        speed = 50,
        hostile = true
    },
    {
        type = "collectible",
        x = 200,
        y = 450,
        value = 10,
        collectType = "coin"
    }
}
```

### Transform Properties

These properties are extracted as `Transform2D`:

| Property | Type | Description |
|----------|------|-------------|
| `x` | number | X position (default: 0) |
| `y` | number | Y position (default: 0) |
| `rotation` | number | Rotation in degrees (default: 0) |
| `scaleX` | number | Horizontal scale (default: 1.0) |
| `scaleY` | number | Vertical scale (default: 1.0) |

Example with full transform:

```lua
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
```

### Custom Properties

Any field other than `type` and transform properties is stored as a custom property:

```lua
{
    type = "trigger",
    x = 800,
    y = 400,
    radius = 50.5,           -- double
    active = true,           -- bool
    message = "Secret found!", -- string
    triggerCount = 1         -- double (Lua stores all numbers as doubles)
}
```

Supported property types:
- `boolean` (stored as `bool`)
- `string` (stored as `std::string`)
- `number` (stored as `double`)

### Procedural Generation

The `entities` field can be a Lua function for procedural generation:

```lua
local GROUND_Y = 550
local SECTION_WIDTH = 800

local function generateStartingEntities()
    local entities = {}

    -- Generate platforms
    for i = 1, 5 do
        table.insert(entities, {
            type = "platform",
            x = i * SECTION_WIDTH,
            y = GROUND_Y,
            width = 200,
            height = 30
        })
    end

    -- Generate coins
    for i = 1, 10 do
        table.insert(entities, {
            type = "coin",
            x = 100 + (i * 80),
            y = GROUND_Y - 60,
            value = 10
        })
    end

    return entities
end

return {
    name = "Endless Runner",
    width = 10000,
    height = 720,

    spawnPoints = {
        player = { x = 200, y = 450 }
    },

    -- Function is called once at load time
    entities = generateStartingEntities
}
```

### Advanced Lua Features

Leverage Lua's power for dynamic level design:

```lua
-- Variables for easy tuning
local PLATFORM_Y = 500
local ENEMY_COUNT = 5
local DEBUG = true

-- Helper functions
local function createPlatform(x, width)
    return {
        type = "platform",
        x = x,
        y = PLATFORM_Y,
        width = width,
        height = 50
    }
end

local function createPatrolEnemy(x)
    return {
        type = "enemy",
        x = x,
        y = PLATFORM_Y - 60,
        patrolRange = 100,
        speed = 50
    }
end

-- Build entity list
local entities = {}

-- Create platforms
table.insert(entities, createPlatform(0, 800))
table.insert(entities, createPlatform(1000, 600))

-- Create enemies based on difficulty
for i = 1, ENEMY_COUNT do
    table.insert(entities, createPatrolEnemy(200 + i * 150))
end

-- Debug entities (only in debug builds)
if DEBUG then
    table.insert(entities, {
        type = "debug_marker",
        x = 100,
        y = 100
    })
end

return {
    name = "Dynamic Level",
    width = 2000,
    height = 1000,
    entities = entities
}
```

## Loading Levels

### Registration and Loading

Levels are assets and must be registered with the Asset System first:

```cpp
import bestow;

// Register the level asset
AssetHandle levelAsset = assetSystem->registerAsset(
    AssetType::Level,
    "levels/tutorial.lua"
);

// Load the asset data (Lua file contents)
assetSystem->loadAsset(levelAsset);

// Wait for async load if needed
while (!assetSystem->isLoaded(levelAsset)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
}

// Load the level through the Level System
Result<LevelId, std::error_code> result = levelSystem->loadLevel(levelAsset);

if (result.has_value()) {
    LevelId levelId = result.value();

    // Verify level is loaded
    LevelState state = levelSystem->getLevelState(levelId);
    assert(state == LevelState::Loaded);

    // Get metadata
    LevelMetadata metadata = levelSystem->getLevelMetadata(levelId);
    std::cout << "Loaded: " << metadata.levelName << "\n";
} else {
    std::cerr << "Failed to load level: " << result.error().message() << "\n";
}
```

### Setting Active Level

Only one level can be active at a time:

```cpp
// Set the level as active
levelSystem->setActiveLevel(levelId);

// Check which level is active
std::optional<LevelId> activeLevel = levelSystem->getActiveLevel();
if (activeLevel.has_value()) {
    std::cout << "Active level: " << *activeLevel << "\n";
}
```

### Querying Loaded Levels

```cpp
// Get all loaded levels
std::vector<LevelMetadata> loadedLevels = levelSystem->getLoadedLevels();

for (const auto& meta : loadedLevels) {
    std::cout << meta.levelName
              << " (" << meta.width << "x" << meta.height << ")"
              << " - State: " << static_cast<int>(meta.state) << "\n";
}
```

### Unloading Levels

```cpp
// Unload a specific level
levelSystem->unloadLevel(levelId);

// Verify unloaded
LevelState state = levelSystem->getLevelState(levelId);
assert(state == LevelState::Unloaded);
```

If the unloaded level is active, `getActiveLevel()` will return `std::nullopt`.

## Level Transitions

### Basic Transition

Transitions happen asynchronously on the next `update()` call:

```cpp
LevelTransition transition{
    .fromLevel = currentLevelId,
    .toLevel = nextLevelId,
    .spawnPoint = std::nullopt,
    .unloadPrevious = false
};

levelSystem->transition(transition);

// Transition occurs on next update
levelSystem->update(deltaTime);

// Now active level has changed
assert(levelSystem->getActiveLevel() == nextLevelId);
```

### Transition with Unload

To free memory, unload the previous level:

```cpp
LevelTransition transition{
    .fromLevel = currentLevelId,
    .toLevel = nextLevelId,
    .spawnPoint = std::nullopt,
    .unloadPrevious = true  // Previous level will be unloaded
};

levelSystem->transition(transition);
levelSystem->update(deltaTime);

// Previous level is now unloaded
assert(levelSystem->getLevelState(currentLevelId) == LevelState::Unloaded);
```

### Transition with Spawn Point

Use named spawn points to position the player:

```cpp
LevelTransition transition{
    .fromLevel = currentLevelId,
    .toLevel = nextLevelId,
    .spawnPoint = "checkpoint1",  // Named spawn point in next level
    .unloadPrevious = true
};

levelSystem->transition(transition);
levelSystem->update(deltaTime);

// Get the spawn point position
std::optional<Transform2D> spawnTransform =
    levelSystem->getSpawnPoint(nextLevelId, "checkpoint1");

if (spawnTransform.has_value()) {
    // Position player at spawn point
    playerTransform.x = spawnTransform->x;
    playerTransform.y = spawnTransform->y;
    playerTransform.rotation = spawnTransform->rotation;
}
```

### Multi-Level Hub System

Keep a hub level loaded while transitioning to sub-levels:

```cpp
// Load hub level
Result<LevelId, std::error_code> hubResult =
    levelSystem->loadLevel(hubLevelAsset);
LevelId hubId = *hubResult;
levelSystem->setActiveLevel(hubId);

// Load sub-level without unloading hub
Result<LevelId, std::error_code> missionResult =
    levelSystem->loadLevel(missionLevelAsset);
LevelId missionId = *missionResult;

// Transition to mission (keep hub loaded)
LevelTransition toMission{
    .fromLevel = hubId,
    .toLevel = missionId,
    .spawnPoint = "start",
    .unloadPrevious = false  // Keep hub in memory
};
levelSystem->transition(toMission);
levelSystem->update(deltaTime);

// Later: Return to hub
LevelTransition toHub{
    .fromLevel = missionId,
    .toLevel = hubId,
    .spawnPoint = "mission_return",
    .unloadPrevious = true  // Unload mission
};
levelSystem->transition(toHub);
levelSystem->update(deltaTime);
```

## Entity Spawning from Level Data

### Basic Entity Spawning

The Level System stores entity definitions but does not spawn entities directly. Game code is responsible for spawning:

```cpp
// Get entity definitions from level
std::vector<EntityDef> entityDefs = levelSystem->getEntityDefs(levelId);

for (const auto& def : entityDefs) {
    // Create entity
    Entity entity = entitySystem->createEntity();

    // Apply transform
    entitySystem->emplace<Transform>(entity, def.transform);

    // Spawn based on type
    if (def.type == "platform") {
        spawnPlatform(entity, def);
    } else if (def.type == "enemy") {
        spawnEnemy(entity, def);
    } else if (def.type == "collectible") {
        spawnCollectible(entity, def);
    }

    // Track spawned entity
    levelEntities.push_back(entity);
}
```

### Accessing Custom Properties

```cpp
void spawnPlatform(Entity entity, const EntityDef& def) {
    // Get width and height from properties
    double width = std::any_cast<double>(def.properties.at("width"));
    double height = std::any_cast<double>(def.properties.at("height"));

    // Create physics body
    PhysicsBodyDef bodyDef{
        .type = BodyType::Static,
        .transform = def.transform,
        .fixedRotation = true
    };
    physicsSystem->createBody(entity, bodyDef);

    // Add collider shape
    BoxShape box{
        .width = static_cast<float>(width),
        .height = static_cast<float>(height)
    };
    physicsSystem->addShape(entity, box);

    // Add sprite
    entitySystem->emplace<Sprite>(entity,
        platformTexture,
        def.transform,
        Color::white()
    );
}

void spawnEnemy(Entity entity, const EntityDef& def) {
    // Extract enemy properties
    double patrolRange = std::any_cast<double>(def.properties.at("patrolRange"));
    double speed = std::any_cast<double>(def.properties.at("speed"));
    bool hostile = std::any_cast<bool>(def.properties.at("hostile"));

    // Add AI component
    PatrolBehavior patrol{
        .centerX = def.transform.x,
        .range = static_cast<float>(patrolRange),
        .speed = static_cast<float>(speed)
    };
    entitySystem->emplace<PatrolBehavior>(entity, patrol);

    // Add health if hostile
    if (hostile) {
        entitySystem->emplace<Health>(entity, 50, 50);
    }
}

void spawnCollectible(Entity entity, const EntityDef& def) {
    // Extract collectible properties
    double value = std::any_cast<double>(def.properties.at("value"));
    std::string collectType = std::any_cast<std::string>(
        def.properties.at("collectType")
    );

    // Add collectible component
    Collectible collectible{
        .value = static_cast<int>(value),
        .type = collectType
    };
    entitySystem->emplace<Collectible>(entity, collectible);

    // Add trigger for collection
    CircleShape trigger{ .radius = 20.0f };
    physicsSystem->addTrigger(entity, trigger);
}
```

### Safe Property Access

Always check if properties exist before casting:

```cpp
void spawnEntity(Entity entity, const EntityDef& def) {
    // Check if property exists
    if (def.properties.contains("health")) {
        try {
            double health = std::any_cast<double>(def.properties.at("health"));
            entitySystem->emplace<Health>(entity,
                static_cast<int>(health),
                static_cast<int>(health)
            );
        } catch (const std::bad_any_cast&) {
            // Property exists but wrong type
            std::cerr << "Invalid type for 'health' property\n";
        }
    }

    // Provide defaults for optional properties
    float speed = 100.0f;  // Default
    if (def.properties.contains("speed")) {
        try {
            speed = static_cast<float>(std::any_cast<double>(def.properties.at("speed")));
        } catch (const std::bad_any_cast&) {}
    }
}
```

## Spawn Points

### Querying Spawn Points

```cpp
// Get all spawn point names
std::vector<std::string> spawnNames =
    levelSystem->getSpawnPointNames(levelId);

for (const auto& name : spawnNames) {
    std::cout << "Spawn point: " << name << "\n";
}

// Get specific spawn point
std::optional<Transform2D> spawn =
    levelSystem->getSpawnPoint(levelId, "checkpoint1");

if (spawn.has_value()) {
    std::cout << "Checkpoint1 at ("
              << spawn->x << ", "
              << spawn->y << ")\n";
}
```

### Spawning Player at Spawn Point

```cpp
void spawnPlayerAtSpawn(LevelId levelId, const std::string& spawnName) {
    auto spawnTransform = levelSystem->getSpawnPoint(levelId, spawnName);

    if (!spawnTransform.has_value()) {
        // Fall back to default spawn
        spawnTransform = levelSystem->getSpawnPoint(levelId, "default");

        if (!spawnTransform.has_value()) {
            // Use hardcoded fallback
            spawnTransform = Transform2D{ .x = 0, .y = 0 };
        }
    }

    // Create or reposition player
    Entity player = entitySystem->createEntity();
    entitySystem->emplace<Transform>(player, *spawnTransform);
    entitySystem->emplace<Player>(player);

    // Add physics body
    PhysicsBodyDef bodyDef{
        .type = BodyType::Dynamic,
        .transform = *spawnTransform,
        .fixedRotation = true
    };
    physicsSystem->createBody(player, bodyDef);
}
```

### Dynamic Spawn Point Selection

```cpp
// Find nearest spawn point to a position
std::string findNearestSpawnPoint(LevelId levelId, Vec2 position) {
    std::vector<std::string> spawnNames =
        levelSystem->getSpawnPointNames(levelId);

    std::string nearest;
    float minDistSq = std::numeric_limits<float>::max();

    for (const auto& name : spawnNames) {
        auto spawn = levelSystem->getSpawnPoint(levelId, name);
        if (!spawn.has_value()) continue;

        float dx = spawn->x - position.x;
        float dy = spawn->y - position.y;
        float distSq = dx * dx + dy * dy;

        if (distSq < minDistSq) {
            minDistSq = distSq;
            nearest = name;
        }
    }

    return nearest;
}
```

## Complete Example: Level Loading and Transition

```cpp
import bestow;

class Game {
public:
    void initialize() {
        // Load level 1
        AssetHandle level1Asset = assetSystem->registerAsset(
            AssetType::Level, "levels/level1.lua"
        );
        assetSystem->loadAsset(level1Asset);

        auto result1 = levelSystem->loadLevel(level1Asset);
        if (!result1.has_value()) {
            std::cerr << "Failed to load level 1\n";
            return;
        }

        currentLevelId = *result1;
        levelSystem->setActiveLevel(currentLevelId);

        // Spawn entities from level
        spawnLevelEntities(currentLevelId);

        // Spawn player at default spawn point
        spawnPlayerAtSpawn(currentLevelId, "default");
    }

    void spawnLevelEntities(LevelId levelId) {
        std::vector<EntityDef> entityDefs = levelSystem->getEntityDefs(levelId);

        for (const auto& def : entityDefs) {
            Entity entity = entitySystem->createEntity();
            entitySystem->emplace<Transform>(entity, def.transform);

            if (def.type == "platform") {
                double width = std::any_cast<double>(def.properties.at("width"));
                double height = std::any_cast<double>(def.properties.at("height"));

                PhysicsBodyDef bodyDef{
                    .type = BodyType::Static,
                    .transform = def.transform
                };
                physicsSystem->createBody(entity, bodyDef);

                BoxShape box{
                    .width = static_cast<float>(width),
                    .height = static_cast<float>(height)
                };
                physicsSystem->addShape(entity, box);
            }

            levelEntities.push_back(entity);
        }
    }

    void spawnPlayerAtSpawn(LevelId levelId, const std::string& spawnName) {
        auto spawnTransform = levelSystem->getSpawnPoint(levelId, spawnName);
        if (!spawnTransform.has_value()) {
            spawnTransform = Transform2D{ .x = 100, .y = 100 };
        }

        playerEntity = entitySystem->createEntity();
        entitySystem->emplace<Transform>(playerEntity, *spawnTransform);

        PhysicsBodyDef bodyDef{
            .type = BodyType::Dynamic,
            .transform = *spawnTransform,
            .fixedRotation = true
        };
        physicsSystem->createBody(playerEntity, bodyDef);
    }

    void transitionToLevel(AssetHandle nextLevelAsset,
                          const std::string& spawnPoint = "default") {
        // Load next level
        auto result = levelSystem->loadLevel(nextLevelAsset);
        if (!result.has_value()) {
            std::cerr << "Failed to load next level\n";
            return;
        }

        LevelId nextLevelId = *result;

        // Clean up current level entities
        for (Entity entity : levelEntities) {
            entitySystem->destroyEntity(entity);
        }
        levelEntities.clear();

        // Transition
        LevelTransition transition{
            .fromLevel = currentLevelId,
            .toLevel = nextLevelId,
            .spawnPoint = spawnPoint,
            .unloadPrevious = true
        };

        levelSystem->transition(transition);
        levelSystem->update(0.016f);  // Process transition

        currentLevelId = nextLevelId;

        // Spawn entities in new level
        spawnLevelEntities(nextLevelId);

        // Reposition player
        auto spawn = levelSystem->getSpawnPoint(nextLevelId, spawnPoint);
        if (spawn.has_value()) {
            auto* transform = entitySystem->get<Transform>(playerEntity);
            if (transform) {
                transform->x = spawn->x;
                transform->y = spawn->y;
                transform->rotation = spawn->rotation;
            }
        }
    }

    void update(DeltaTime dt) {
        // Always update level system to process transitions
        levelSystem->update(dt);

        // Check for level transition triggers
        if (playerReachedExit) {
            AssetHandle nextLevel = assetSystem->registerAsset(
                AssetType::Level, "levels/level2.lua"
            );
            assetSystem->loadAsset(nextLevel);
            transitionToLevel(nextLevel, "entrance");
        }
    }

private:
    ILevelSystem* levelSystem;
    IAssetSystem* assetSystem;
    IEntitySystem* entitySystem;
    IPhysicsSystem* physicsSystem;

    LevelId currentLevelId;
    Entity playerEntity;
    std::vector<Entity> levelEntities;
};
```

## Best Practices

### 1. Always Load Assets First

```cpp
// WRONG: Level System expects asset to be loaded
AssetHandle handle = assetSystem->registerAsset(AssetType::Level, "level.lua");
auto result = levelSystem->loadLevel(handle);  // Asset not loaded yet!

// CORRECT: Load asset, then load level
AssetHandle handle = assetSystem->registerAsset(AssetType::Level, "level.lua");
assetSystem->loadAsset(handle);
while (!assetSystem->isLoaded(handle)) {
    // Wait for async load
}
auto result = levelSystem->loadLevel(handle);
```

### 2. Process Transitions with Update

```cpp
// Transitions are deferred until update() is called
levelSystem->transition(transition);
// Active level has NOT changed yet!

levelSystem->update(deltaTime);
// NOW the transition has occurred
```

### 3. Clean Up Level Entities

```cpp
// Track entities spawned from level
std::vector<Entity> currentLevelEntities;

// When transitioning, destroy level entities
for (Entity entity : currentLevelEntities) {
    entitySystem->destroyEntity(entity);
}
currentLevelEntities.clear();

// Then transition
levelSystem->transition(transition);
levelSystem->update(dt);
```

### 4. Use Spawn Point Fallbacks

```cpp
auto spawn = levelSystem->getSpawnPoint(levelId, requestedSpawn);
if (!spawn.has_value()) {
    // Try default
    spawn = levelSystem->getSpawnPoint(levelId, "default");
}
if (!spawn.has_value()) {
    // Hardcoded fallback
    spawn = Transform2D{ .x = 0, .y = 0 };
}
```

### 5. Type-Safe Property Access

```cpp
// Use templates for property extraction
template<typename T>
std::optional<T> getProperty(const EntityDef& def, const std::string& key) {
    if (!def.properties.contains(key)) {
        return std::nullopt;
    }

    try {
        return std::any_cast<T>(def.properties.at(key));
    } catch (const std::bad_any_cast&) {
        return std::nullopt;
    }
}

// Usage
auto width = getProperty<double>(def, "width").value_or(100.0);
auto hostile = getProperty<bool>(def, "hostile").value_or(false);
auto message = getProperty<std::string>(def, "message").value_or("");
```

### 6. Validate Level Data

```cpp
bool validateEntityDef(const EntityDef& def) {
    if (def.type.empty()) {
        std::cerr << "Entity missing type\n";
        return false;
    }

    // Validate required properties per type
    if (def.type == "platform") {
        if (!def.properties.contains("width") ||
            !def.properties.contains("height")) {
            std::cerr << "Platform missing width/height\n";
            return false;
        }
    }

    return true;
}
```

## Performance Considerations

### Memory Management

- Keep only necessary levels loaded (use `unloadPrevious = true`)
- For hub-and-spoke designs, keep hub loaded but unload sub-levels
- Monitor memory usage with multiple loaded levels

### Entity Spawning

- Spawn level entities once at load time, not every frame
- Consider object pooling for frequently spawned entities
- Use spatial partitioning for large levels with many entities

### Lua Parsing

- Level Lua files are parsed synchronously at load time
- Keep entity counts reasonable (< 1000 entities per level)
- Consider breaking very large levels into multiple sub-levels

## Integration with Other Systems

### Asset System

```cpp
// Level files are Data assets
AssetHandle handle = assetSystem->registerAsset(AssetType::Level, path);
assetSystem->loadAsset(handle);

// Hot-reload support (dev builds)
if (assetSystem->isModified(handle)) {
    assetSystem->reloadAsset(handle);
    levelSystem->unloadLevel(levelId);
    levelSystem->loadLevel(handle);
}
```

### Physics System

```cpp
// Create physics bodies from entity definitions
for (const auto& def : levelSystem->getEntityDefs(levelId)) {
    if (def.type == "platform" || def.type == "wall") {
        Entity entity = entitySystem->createEntity();

        PhysicsBodyDef bodyDef{
            .type = BodyType::Static,
            .transform = def.transform
        };
        physicsSystem->createBody(entity, bodyDef);

        // Add shape based on properties
        auto width = getProperty<double>(def, "width").value_or(100.0);
        auto height = getProperty<double>(def, "height").value_or(100.0);

        BoxShape box{
            .width = static_cast<float>(width),
            .height = static_cast<float>(height)
        };
        physicsSystem->addShape(entity, box);
    }
}
```

### Save System

```cpp
// Save current level and spawn point
SaveData saveData;
saveData.currentLevelAsset = getCurrentLevelAssetPath();
saveData.spawnPoint = findNearestCheckpoint(playerPosition);

saveSystem->save(SaveSlots::AutoSave, saveData);

// Load saved level
SaveData loadedData;
if (saveSystem->load(saveSlot, loadedData)) {
    AssetHandle levelAsset = assetSystem->registerAsset(
        AssetType::Level,
        loadedData.currentLevelAsset
    );
    transitionToLevel(levelAsset, loadedData.spawnPoint);
}
```

## Debugging

### Logging Level State

```cpp
void logLevelState(ILevelSystem* levelSystem) {
    auto loadedLevels = levelSystem->getLoadedLevels();
    std::cout << "Loaded levels: " << loadedLevels.size() << "\n";

    for (const auto& meta : loadedLevels) {
        std::cout << "  - " << meta.levelName
                  << " (ID: " << meta.id << ")"
                  << " State: " << static_cast<int>(meta.state)
                  << "\n";

        // Log spawn points
        auto spawnNames = levelSystem->getSpawnPointNames(meta.id);
        for (const auto& name : spawnNames) {
            auto spawn = levelSystem->getSpawnPoint(meta.id, name);
            if (spawn.has_value()) {
                std::cout << "    Spawn '" << name
                          << "': (" << spawn->x << ", " << spawn->y << ")\n";
            }
        }
    }

    auto activeLevel = levelSystem->getActiveLevel();
    if (activeLevel.has_value()) {
        std::cout << "Active level: " << *activeLevel << "\n";
    }
}
```

### Visualizing Entity Definitions

```cpp
void debugDrawEntityDefs(LevelId levelId) {
    auto entityDefs = levelSystem->getEntityDefs(levelId);

    for (const auto& def : entityDefs) {
        // Draw marker at entity position
        debugRenderer->drawCircle(
            {def.transform.x, def.transform.y},
            10.0f,
            Color::yellow()
        );

        // Draw label
        debugRenderer->drawText(
            {def.transform.x, def.transform.y - 15},
            def.type,
            Color::white()
        );
    }
}
```

## Common Patterns

### Checkpoint System

```lua
-- level.lua
return {
    name = "Level with Checkpoints",

    spawnPoints = {
        start = { x = 100, y = 500 },
        checkpoint1 = { x = 500, y = 400 },
        checkpoint2 = { x = 1000, y = 300 },
        checkpoint3 = { x = 1500, y = 200 }
    },

    entities = {
        -- Checkpoint triggers
        { type = "checkpoint", x = 500, y = 400, id = 1 },
        { type = "checkpoint", x = 1000, y = 300, id = 2 },
        { type = "checkpoint", x = 1500, y = 200, id = 3 }
    }
}
```

```cpp
// Track last checkpoint
std::string lastCheckpoint = "start";

// When player touches checkpoint trigger
void onCheckpointReached(int checkpointId) {
    lastCheckpoint = "checkpoint" + std::to_string(checkpointId);

    // Save checkpoint
    SaveData data;
    data.checkpointSpawn = lastCheckpoint;
    saveSystem->quickSave(data);
}

// On player death
void respawnPlayer() {
    auto spawn = levelSystem->getSpawnPoint(currentLevelId, lastCheckpoint);
    if (spawn.has_value()) {
        repositionPlayer(*spawn);
    }
}
```

### Tiered Level Loading

```cpp
// Load adjacent levels in background for seamless transitions
void preloadAdjacentLevels(LevelId currentLevel) {
    // Get adjacent level IDs from metadata or design
    std::vector<std::string> adjacentPaths = {
        "levels/level" + std::to_string(currentLevelIndex + 1) + ".lua",
        "levels/level" + std::to_string(currentLevelIndex - 1) + ".lua"
    };

    for (const auto& path : adjacentPaths) {
        AssetHandle handle = assetSystem->registerAsset(AssetType::Level, path);
        assetSystem->loadAssetAsync(handle, [this, handle](AssetState state) {
            if (state == AssetState::Loaded) {
                levelSystem->loadLevel(handle);
            }
        });
    }
}
```

## Troubleshooting

### "Level fails to load"

1. Verify asset is loaded: `assetSystem->isLoaded(handle)`
2. Check Lua syntax errors in level file
3. Ensure level file returns a table
4. Verify file path is correct

### "Spawn point not found"

1. Check spelling of spawn point name
2. Verify `spawnPoints` table exists in Lua
3. Use `getSpawnPointNames()` to list available spawns
4. Always provide "default" spawn point

### "Entity properties not found"

1. Properties are optional - always check `contains()`
2. Remember Lua numbers are stored as `double`, not `int` or `float`
3. Use `try-catch` around `std::any_cast`
4. Validate level files have required properties

### "Transitions not working"

1. Call `levelSystem->update(dt)` after `transition()`
2. Verify both levels are loaded before transitioning
3. Check that target level ID is valid
4. Ensure you're not calling `transition()` every frame

## API Reference

See `bestow-contract/src/bestow.level.cppm` for the complete interface definition.

### Core Methods

| Method | Description |
|--------|-------------|
| `loadLevel(AssetHandle)` | Load level from asset, returns `LevelId` |
| `unloadLevel(LevelId)` | Unload level and free resources |
| `setActiveLevel(LevelId)` | Set the currently active level |
| `transition(LevelTransition)` | Queue level transition (processed on next update) |
| `update(DeltaTime)` | Process pending transitions |
| `getActiveLevel()` | Get currently active level ID |
| `getLevelState(LevelId)` | Get level's current state |
| `getLevelMetadata(LevelId)` | Get level metadata |
| `getLoadedLevels()` | Get all loaded level metadata |
| `getSpawnPoint(LevelId, name)` | Get named spawn point transform |
| `getSpawnPointNames(LevelId)` | Get all spawn point names |
| `getLevelEntities(LevelId)` | Get entities spawned in level |
| `getEntityDefs(LevelId)` | Get entity definitions from level |

---

**Next Steps**: See [Entity System](Entity-System.md) for entity management and [Physics System](Physics-System.md) for collision detection.
