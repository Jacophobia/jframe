# Config System

> **Visibility:** Public (Lua + C++ API)
> **Tier:** 1
> **Dependencies:** Types
> **Lua Paths:** `bestow.config` (high-level), `bestow.config.core` (low-level)

## Purpose

The Config System loads and manages Lua-based configuration files, providing typed access to configuration values by dot-separated keys. Lua configs support comments, variables, computed values, and conditional logic, making them far more expressive than JSON or INI files. The high-level API provides simple load-and-read operations with optional default values, while the low-level API adds lifecycle management, asset-based loading, vector/color access, array and table access, runtime modification, hot reload with change notifications, key enumeration, and raw Lua execution.

## High-Level API: `IConfigSystem`

The simplified API for loading configs and reading values. Supports float, int, bool, and string types with optional defaults. No lifecycle methods -- the engine manages those internally.

### Loading

| Method | Returns | Description |
|--------|---------|-------------|
| `loadConfig(std::string_view path)` | `Result<void>` | Load a Lua configuration file from the given path; keys become accessible via dot notation |

### Type-Safe Access

| Method | Returns | Description |
|--------|---------|-------------|
| `getFloat(std::string_view key)` | `std::optional<float>` | Retrieve a float value by key, or nullopt if the key does not exist |
| `getInt(std::string_view key)` | `std::optional<int>` | Retrieve an integer value by key, or nullopt if the key does not exist |
| `getBool(std::string_view key)` | `std::optional<bool>` | Retrieve a boolean value by key, or nullopt if the key does not exist |
| `getString(std::string_view key)` | `std::optional<std::string>` | Retrieve a string value by key, or nullopt if the key does not exist |

### With Defaults

| Method | Returns | Description |
|--------|---------|-------------|
| `getFloatOr(std::string_view key, float def)` | `float` | Retrieve a float value by key, returning the default if not found |
| `getIntOr(std::string_view key, int def)` | `int` | Retrieve an integer value by key, returning the default if not found |
| `getBoolOr(std::string_view key, bool def)` | `bool` | Retrieve a boolean value by key, returning the default if not found |

### Queries

| Method | Returns | Description |
|--------|---------|-------------|
| `hasKey(std::string_view key)` | `bool` | Check whether a configuration key exists |

## Low-Level API: `IConfigCore`

Full control API. Exposes lifecycle, asset-based loading, reload, vector/color accessors, array and table access, runtime modification, key enumeration, hot reload control, change notifications, and raw Lua execution.

### Lifecycle

| Method | Returns | Description |
|--------|---------|-------------|
| `initialize()` | `Result<void>` | Initialize the config system |
| `shutdown()` | `void` | Shut down the config system and release all loaded configs |
| `update(DeltaTime dt)` | `void` | Process hot reload file checks and pending notifications |

### Loading

| Method | Returns | Description |
|--------|---------|-------------|
| `loadConfig(std::string_view filePath)` | `Result<void>` | Load a Lua configuration file from a file path |
| `loadConfigAsset(AssetHandle asset)` | `Result<void>` | Load a configuration from a previously registered asset handle |
| `reloadAll()` | `Result<void>` | Reload all currently loaded configuration files from disk |
| `reloadConfig(std::string_view filePath)` | `Result<void>` | Reload a specific configuration file from disk |

### Type-Safe Access

| Method | Returns | Description |
|--------|---------|-------------|
| `getFloat(std::string_view key)` | `std::optional<float>` | Retrieve a float value by key |
| `getInt(std::string_view key)` | `std::optional<int>` | Retrieve an integer value by key |
| `getBool(std::string_view key)` | `std::optional<bool>` | Retrieve a boolean value by key |
| `getString(std::string_view key)` | `std::optional<std::string>` | Retrieve a string value by key |
| `getVec2(std::string_view key)` | `std::optional<Vec2>` | Retrieve a 2D vector value by key (expects a table with x, y fields) |
| `getVec3(std::string_view key)` | `std::optional<Vec3>` | Retrieve a 3D vector value by key (expects a table with x, y, z fields) |
| `getColor(std::string_view key)` | `std::optional<Color>` | Retrieve a color value by key (expects a table with r, g, b, a fields) |

### With Defaults

| Method | Returns | Description |
|--------|---------|-------------|
| `getFloatOr(std::string_view key, float def)` | `float` | Retrieve a float or return the default |
| `getIntOr(std::string_view key, int def)` | `int` | Retrieve an integer or return the default |
| `getBoolOr(std::string_view key, bool def)` | `bool` | Retrieve a boolean or return the default |
| `getStringOr(std::string_view key, std::string_view def)` | `std::string` | Retrieve a string or return the default |

### Arrays

| Method | Returns | Description |
|--------|---------|-------------|
| `getIntArray(std::string_view key)` | `std::vector<int>` | Retrieve a Lua array as a vector of integers |
| `getFloatArray(std::string_view key)` | `std::vector<float>` | Retrieve a Lua array as a vector of floats |
| `getStringArray(std::string_view key)` | `std::vector<std::string>` | Retrieve a Lua array as a vector of strings |

### Tables

| Method | Returns | Description |
|--------|---------|-------------|
| `getTableKeys(std::string_view prefix)` | `std::vector<std::string>` | Return all child keys under the given prefix (e.g., "enemies" returns "enemies.goblin", "enemies.dragon") |
| `isTable(std::string_view key)` | `bool` | Check whether the value at the given key is a Lua table |

### Runtime Modification

| Method | Returns | Description |
|--------|---------|-------------|
| `setFloat(std::string_view key, float value)` | `void` | Set or overwrite a float value at runtime |
| `setInt(std::string_view key, int value)` | `void` | Set or overwrite an integer value at runtime |
| `setBool(std::string_view key, bool value)` | `void` | Set or overwrite a boolean value at runtime |
| `setString(std::string_view key, std::string_view value)` | `void` | Set or overwrite a string value at runtime |

### Queries

| Method | Returns | Description |
|--------|---------|-------------|
| `hasKey(std::string_view key)` | `bool` | Check whether a configuration key exists |
| `getKeysWithPrefix(std::string_view prefix)` | `std::vector<std::string>` | Return all keys that start with the given prefix |
| `getLoadedConfigs()` | `std::vector<std::string>` | Return the file paths of all currently loaded configuration files |

### Hot Reload

| Method | Returns | Description |
|--------|---------|-------------|
| `enableHotReload(bool enable)` | `void` | Enable or disable automatic file watching and reloading |
| `isHotReloadEnabled()` | `bool` | Return whether hot reload is currently active |

### Notifications

| Method | Returns | Description |
|--------|---------|-------------|
| `onConfigChanged(std::function<void(std::string_view key)> callback)` | `SubscriptionId` | Subscribe to notifications when any config key value changes (via reload or runtime set) |
| `onKeyChanged(std::string_view prefix, std::function<void(std::string_view key)> callback)` | `SubscriptionId` | Subscribe to changes only for keys matching the given prefix |
| `unsubscribe(SubscriptionId id)` | `void` | Remove a previously registered change notification |

### Lua Parsing

| Method | Returns | Description |
|--------|---------|-------------|
| `executeLua(std::string_view code, std::string_view description = "lua")` | `Result<void>` | Execute raw Lua code in the config sandbox; the description is used for error messages |

## Types

The Config System uses only primitive types from the foundation (`float`, `int`, `bool`, `std::string`, `Vec2`, `Vec3`, `Color`). No custom struct types are defined by this system.

## Lua Examples

```lua
-- High-level: Load and read configuration
local _, err = bestow.config.loadConfig("config/game.lua")
if err then print("Config error: " .. err.message) return end

-- Read values with optional types
local gravity = bestow.config.getFloat("physics.gravity")
local maxHP = bestow.config.getInt("player.maxHealth")
local debug = bestow.config.getBool("debug.enabled")
local title = bestow.config.getString("window.title")

-- Read with defaults (never nil)
local speed = bestow.config.getFloatOr("player.speed", 5.0)
local lives = bestow.config.getIntOr("player.lives", 3)
local vsync = bestow.config.getBoolOr("window.vsync", true)

-- Check existence
if bestow.config.hasKey("audio.masterVolume") then
    local vol = bestow.config.getFloat("audio.masterVolume")
end

-- Low-level: Vectors and colors
local spawnPoint = bestow.config.core.getVec3("level.spawnPoint")
local bgColor = bestow.config.core.getColor("ui.backgroundColor")

-- Arrays
local waves = bestow.config.core.getIntArray("enemies.waveCounts")
local names = bestow.config.core.getStringArray("characters.available")

-- Tables
local enemyKeys = bestow.config.core.getTableKeys("enemies")
for _, key in ipairs(enemyKeys) do
    local hp = bestow.config.core.getFloat(key .. ".health")
end

-- Runtime modification
bestow.config.core.setFloat("difficulty.multiplier", 2.0)
bestow.config.core.setBool("debug.showHitboxes", true)

-- Hot reload
bestow.config.core.enableHotReload(true)

-- Subscribe to changes
local subId = bestow.config.core.onKeyChanged("audio", function(key)
    print("Audio config changed: " .. key)
end)

-- Execute raw Lua in config sandbox
bestow.config.core.executeLua([[
    difficulty = {
        easy   = { multiplier = 0.5, enemyHP = 50 },
        normal = { multiplier = 1.0, enemyHP = 100 },
        hard   = { multiplier = 2.0, enemyHP = 200 }
    }
]])
```

## C++ Examples

```cpp
// High-level: Load and read
auto result = config->loadConfig("config/game.lua");
if (!result) {
    spdlog::error("Config: {}", result.error().message);
    return;
}

auto gravity = config->getFloat("physics.gravity"); // std::optional<float>
auto maxHP = config->getInt("player.maxHealth");
float speed = config->getFloatOr("player.speed", 5.0f);
bool debug = config->getBoolOr("debug.enabled", false);

if (config->hasKey("audio.masterVolume")) {
    float vol = *config->getFloat("audio.masterVolume");
}

// Low-level: Vectors and colors
auto spawn = configCore->getVec3("level.spawnPoint");
auto bgColor = configCore->getColor("ui.backgroundColor");

// Arrays
auto waves = configCore->getIntArray("enemies.waveCounts");
auto names = configCore->getStringArray("characters.available");

// Table iteration
auto enemyKeys = configCore->getTableKeys("enemies");
for (const auto& key : enemyKeys) {
    auto hp = configCore->getFloat(key + ".health");
}

// Runtime modification
configCore->setFloat("difficulty.multiplier", 2.0f);
configCore->setBool("debug.showHitboxes", true);

// Hot reload with notifications
configCore->enableHotReload(true);
auto subId = configCore->onKeyChanged("audio",
    [](std::string_view key) {
        spdlog::info("Audio config changed: {}", key);
    });

// Reload a specific config
configCore->reloadConfig("config/game.lua");

// Execute raw Lua
configCore->executeLua(R"(
    difficulty = {
        easy   = { multiplier = 0.5, enemyHP = 50 },
        normal = { multiplier = 1.0, enemyHP = 100 },
        hard   = { multiplier = 2.0, enemyHP = 200 }
    }
)", "difficulty_config");

// Enumerate loaded configs
auto loaded = configCore->getLoadedConfigs();
for (const auto& path : loaded) {
    spdlog::info("Loaded config: {}", path);
}

// Clean up
configCore->unsubscribe(subId);
```
