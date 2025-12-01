# Level System Demo

A comprehensive demonstration of the JFrame Level System API.

## Overview

This demo exercises **every method** in the `ILevelSystem` interface, showcasing:

- Level loading and unloading
- Level state management
- Level transitions with spawn points
- Spawn point queries
- Entity definition queries
- Level metadata inspection
- Lua-based level file format
- Event system integration

## Building

```bash
# Configure
cmake --preset macos-debug

# Build
cmake --build --preset macos-debug --target level-demo

# Run
./build/bin/level-demo
```

## Files

### Source Files

- **src/main.cpp** - Comprehensive API demonstration with mock systems
  - Shows how to use every `ILevelSystem` method
  - Demonstrates common usage patterns
  - Includes detailed comments and expected behavior

### Level Data Files

- **data/level1.lua** - "Tutorial Village"
  - Simple platformer level
  - 4 spawn points (default, fromLeft, fromRight, checkpoint1)
  - 12 entities (platforms, enemies, collectibles, decorations, exit)
  - Demonstrates basic level structure

- **data/level2.lua** - "Dungeon Depths"
  - Complex dungeon level with boss
  - 4 spawn points (default, fromLeft, checkpoint1, bossArena)
  - 25+ entities (platforms, enemies, hazards, collectibles, checkpoint, exit)
  - Demonstrates advanced level features

## Level System API Coverage

### Lifecycle
- ✓ `update(DeltaTime dt)` - Per-frame update

### Level Management
- ✓ `loadLevel(AssetHandle)` - Load level from asset
- ✓ `unloadLevel(LevelId)` - Unload level from memory
- ✓ `setActiveLevel(LevelId)` - Set which level is active

### Level Transitions
- ✓ `transition(LevelTransition)` - Transition between levels with spawn point

### State Queries
- ✓ `getActiveLevel()` - Get currently active level ID
- ✓ `getLevelState(LevelId)` - Get level state (Unloaded/Loading/Loaded/Active/Unloading)
- ✓ `getLevelMetadata(LevelId)` - Get level metadata (name, dimensions, etc.)
- ✓ `getLoadedLevels()` - Get all loaded level metadata

### Spawn Points
- ✓ `getSpawnPoint(LevelId, name)` - Get specific spawn point transform
- ✓ `getSpawnPointNames(LevelId)` - Get all spawn point names

### Level Queries
- ✓ `getLevelEntities(LevelId)` - Get spawned entity handles
- ✓ `getEntityDefs(LevelId)` - Get entity definitions from level

## Types Demonstrated

### Level Types
- `LevelId` - UUID identifying a level
- `LevelState` - Enum (Unloaded, Loading, Loaded, Active, Unloading)
- `LevelMetadata` - Level information (id, name, dimensions, state, asset)
- `LevelTransition` - Transition parameters (from, to, spawn point, unload flag)
- `LevelEvent` - Enum (LoadStarted, LoadCompleted, Activated, etc.)
- `LevelEventData` - Event payload (levelId, event type)

### Entity Types
- `EntityDef` - Entity template (type, transform, properties map)
- `Transform2D` - Position, rotation, scale

## Lua Level Format

JFrame uses Lua for level files instead of JSON because:

- **Comments allowed** (`-- comment`)
- **Trailing commas OK** (no syntax errors)
- **Variables** (`local GROUND_Y = 900`)
- **Functions** (`makeEnemyWave(x, count)`)
- **Loops** (`for i = 1, 10 do ... end`)
- **Math** (`math.sin(i * 0.1) * 100`)
- **Conditionals** (`DEBUG and {...} or {}`)

### Minimal Level Structure

```lua
return {
  metadata = {
    name = "Level Name",
    width = 1920.0,
    height = 1080.0
  },

  spawnPoints = {
    default = { x = 960.0, y = 540.0, rotation = 0.0 }
  },

  entities = {
    {
      type = "platform",
      transform = { x = 960.0, y = 900.0, rotation = 0.0 },
      properties = {
        width = 1920.0,
        height = 100.0,
        texture = "ground"
      }
    }
  }
}
```

### Advanced Features

```lua
-- Variables for reusability
local GROUND_Y = 900
local PLATFORM_WIDTH = 400

-- Procedural generation with loops
local entities = {}

for i = 1, 10 do
  table.insert(entities, {
    type = "coin",
    transform = {
      x = i * 100,
      y = 500 + math.sin(i * 0.5) * 50,
      rotation = 0.0
    },
    properties = { value = 10 }
  })
end

return {
  metadata = { name = "Generated Level", width = 1920.0, height = 1080.0 },
  spawnPoints = { default = { x = 100, y = GROUND_Y - 50, rotation = 0.0 } },
  entities = entities
}
```

## Common Usage Patterns

### 1. Loading Screen

```cpp
// Subscribe to level events
eventSystem->subscribe("Level", [](const EventData& data) {
  auto& evt = std::get<LevelEventData>(data);
  if (evt.event == LevelEvent::LoadStarted) {
    showLoadingUI();
  } else if (evt.event == LevelEvent::LoadCompleted) {
    hideLoadingUI();
  }
});

// Trigger level load
auto result = levelSystem->loadLevel(nextLevelAsset);
if (result.has_value()) {
  levelSystem->setActiveLevel(result.value());
}
```

### 2. Checkpoint System

```cpp
void saveCheckpoint(LevelId levelId, const std::string& spawnName) {
  checkpointData = { .levelId = levelId, .spawnPoint = spawnName };
}

void respawnAtCheckpoint() {
  LevelTransition transition{
    .fromLevel = currentLevelId,
    .toLevel = checkpointData.levelId,
    .spawnPoint = checkpointData.spawnPoint,
    .unloadPrevious = true
  };
  levelSystem->transition(transition);
}
```

### 3. Spawning Entities from Definitions

```cpp
void spawnLevelEntities(LevelId levelId) {
  auto entityDefs = levelSystem->getEntityDefs(levelId);

  for (const auto& def : entityDefs) {
    Entity entity = entitySystem->createEntity();
    entitySystem->emplace<Transform2D>(entity, def.transform);

    if (def.type == "platform") {
      auto width = std::any_cast<double>(def.properties.at("width"));
      auto height = std::any_cast<double>(def.properties.at("height"));
      // Add platform components...
    } else if (def.type == "enemy") {
      auto health = std::any_cast<int>(def.properties.at("health"));
      // Add enemy components...
    }
  }
}
```

### 4. Level Transition Trigger

```cpp
void onExitTrigger() {
  // Get exit properties from level definition
  auto entityDefs = levelSystem->getEntityDefs(currentLevelId);

  for (const auto& def : entityDefs) {
    if (def.type == "exit") {
      auto targetLevel = std::any_cast<std::string>(def.properties.at("targetLevel"));
      auto targetSpawn = std::any_cast<std::string>(def.properties.at("targetSpawn"));

      LevelId targetId = findLevelByName(targetLevel);

      LevelTransition transition{
        .fromLevel = currentLevelId,
        .toLevel = targetId,
        .spawnPoint = targetSpawn,
        .unloadPrevious = true
      };
      levelSystem->transition(transition);
      break;
    }
  }
}
```

## Event System Integration

Level events are emitted via the Event System:

```cpp
eventSystem->subscribe("Level", [](const EventData& data) {
  auto& levelEvent = std::get<LevelEventData>(data);

  switch (levelEvent.event) {
    case LevelEvent::LoadStarted:
      // Show loading screen
      break;
    case LevelEvent::LoadCompleted:
      // Hide loading screen
      break;
    case LevelEvent::Activated:
      // Start level (spawn player, start music, etc.)
      break;
    case LevelEvent::Deactivated:
      // Pause level, save state
      break;
    case LevelEvent::UnloadStarted:
      // Cleanup level-specific resources
      break;
    case LevelEvent::UnloadCompleted:
      // Level fully unloaded
      break;
  }
});
```

## Implementation Notes

This demo uses **mock implementations** of the dependency systems (Assets, Entity, Events) to demonstrate the Level System API in isolation. In a real game:

1. **Get systems from DI**: `auto levelSystem = injector.get<ILevelSystem*>()`
2. **Real implementations**: Link against `jframe-level`, `jframe-assets`, etc.
3. **Actual level files**: Load from filesystem
4. **Entity spawning**: Create real entities with components

## Expected Output

The demo prints detailed sections showing:

1. Setup and initialization
2. Level management (loading/unloading)
3. State queries (metadata, state, loaded levels)
4. Level activation
5. Spawn point queries
6. Entity definition queries
7. Level transitions
8. Lifecycle update
9. Cleanup and unloading
10. Complete API summary
11. Level data format examples
12. Event system integration
13. Common usage patterns

Each section demonstrates API usage, expected behavior, and data structures.

## Related Documentation

- [Level System Implementation Guide](../../../docs/SYSTEM-IMPLEMENTATION-GUIDE.md)
- [JFrame Technical Design](../../../docs/jframe-technical-design.md) - Level System section
- [Level System Interface](../../../jframe-contract/src/jframe.level.cppm)

## Dependencies

- `jframe-contract` - Interface definitions
- `jframe-level` - Level System implementation (when available)
- `jframe-entity` - Entity management
- `jframe-assets` - Asset loading
- `jframe-events` - Event system

## License

Part of the JFrame game engine.
