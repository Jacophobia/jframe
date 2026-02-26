# Config System

> **Visibility:** Public (Lua + C++ API)
> **Tier:** 1
> **Dependencies:** Types, Assets
> **Lua Paths:** `bestow.config` (high-level), `bestow.config.core` (low-level)

## Purpose

The Config System loads and manages Lua-based configuration files, providing hierarchical key-value access with dot-notation paths. Lua configs support comments, variables, computed values, and conditional logic, making them far more expressive than JSON or INI files. The high-level API provides simple typed accessors with optional default values, key existence checks, and change notification callbacks. The low-level API adds lifecycle management, asset-based loading, hot reload control, runtime modification, array and table access, key enumeration, metadata queries, and raw Lua parsing and execution.

## High-Level API: `IConfigSystem`

The simplified API for loading configs and reading values. Supports float, int, bool, and string types with optional defaults. No lifecycle methods -- the engine manages those internally.

### Type-Safe Access

| Method | Returns | Description |
|--------|---------|-------------|
| `getFloat(std::string_view key)` | `std::optional<float>` | Get a float config value by dot-path key |
| `getInt(std::string_view key)` | `std::optional<int>` | Get an integer config value |
| `getBool(std::string_view key)` | `std::optional<bool>` | Get a boolean config value |
| `getString(std::string_view key)` | `std::optional<std::string>` | Get a string config value |

### With Defaults

| Method | Returns | Description |
|--------|---------|-------------|
| `getFloatOr(std::string_view key, float defaultValue)` | `float` | Get float with fallback default |
| `getIntOr(std::string_view key, int defaultValue)` | `int` | Get int with fallback default |
| `getBoolOr(std::string_view key, bool defaultValue)` | `bool` | Get bool with fallback default |
| `getStringOr(std::string_view key, std::string_view defaultValue)` | `std::string` | Get string with fallback default |

### Queries

| Method | Returns | Description |
|--------|---------|-------------|
| `hasKey(std::string_view key)` | `bool` | Check if a config key exists |

### Notifications

| Method | Returns | Description |
|--------|---------|-------------|
| `onConfigChanged(std::function<void(std::string_view key)> callback)` | `SubscriptionId` | Subscribe to config change events |
| `unsubscribe(SubscriptionId id)` | `void` | Unsubscribe from change notifications |

## Low-Level API: `IConfigCore`

Full control API. Exposes lifecycle, asset-based loading, reload, array and table access, runtime modification, key enumeration, metadata queries, hot reload control, scoped change notifications, and raw Lua parsing and execution.

### Lifecycle

| Method | Returns | Description |
|--------|---------|-------------|
| `initialize()` | `bool` | Initialize the config system; returns true on success |
| `update(DeltaTime dt)` | `void` | Process hot reload file checks and pending notifications |
| `shutdown()` | `void` | Shut down the config system and release all loaded configs |

### Loading

| Method | Returns | Description |
|--------|---------|-------------|
| `loadConfig(std::string_view filePath)` | `bool` | Load a Lua configuration file from a file path; returns true on success |
| `loadConfigAsset(AssetHandle asset)` | `bool` | Load a configuration from a previously registered asset handle; returns true on success |
| `reloadAll()` | `bool` | Reload all currently loaded configuration files from disk |
| `reloadConfig(std::string_view filePath)` | `bool` | Reload a specific configuration file from disk |

### Type-Safe Access

| Method | Returns | Description |
|--------|---------|-------------|
| `getFloat(std::string_view key)` | `std::optional<float>` | Retrieve a float value by dot-path key |
| `getInt(std::string_view key)` | `std::optional<int>` | Retrieve an integer value by dot-path key |
| `getBool(std::string_view key)` | `std::optional<bool>` | Retrieve a boolean value by dot-path key |
| `getString(std::string_view key)` | `std::optional<std::string>` | Retrieve a string value by dot-path key |
| `getFloatOr(std::string_view key, float defaultValue)` | `float` | Retrieve a float or return the default |
| `getIntOr(std::string_view key, int defaultValue)` | `int` | Retrieve an integer or return the default |
| `getBoolOr(std::string_view key, bool defaultValue)` | `bool` | Retrieve a boolean or return the default |
| `getStringOr(std::string_view key, std::string_view defaultValue)` | `std::string` | Retrieve a string or return the default |

### Arrays

| Method | Returns | Description |
|--------|---------|-------------|
| `getIntArray(std::string_view key)` | `std::vector<int>` | Retrieve a Lua array as a vector of integers |
| `getFloatArray(std::string_view key)` | `std::vector<float>` | Retrieve a Lua array as a vector of floats |
| `getStringArray(std::string_view key)` | `std::vector<std::string>` | Retrieve a Lua array as a vector of strings |

### Runtime Modification

| Method | Returns | Description |
|--------|---------|-------------|
| `setFloat(std::string_view key, float value)` | `void` | Set or overwrite a float value at runtime; triggers change notifications |
| `setInt(std::string_view key, int value)` | `void` | Set or overwrite an integer value at runtime; triggers change notifications |
| `setBool(std::string_view key, bool value)` | `void` | Set or overwrite a boolean value at runtime; triggers change notifications |
| `setString(std::string_view key, std::string_view value)` | `void` | Set or overwrite a string value at runtime; triggers change notifications |

### State Queries

| Method | Returns | Description |
|--------|---------|-------------|
| `hasKey(std::string_view key)` | `bool` | Check whether a configuration key exists |
| `getKeysWithPrefix(std::string_view prefix)` | `std::vector<std::string>` | Return all keys that start with the given prefix |
| `getLoadedConfigs()` | `std::vector<std::string>` | Return the file paths of all currently loaded configuration files |
| `getMetadata(std::string_view filePath)` | `ConfigMetadata` | Return metadata for a loaded configuration file |

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
| `parseLuaString(std::string_view code, std::string_view description)` | `Result<sol::object>` | Parse a Lua string and return the resulting object without side effects |
| `parseLuaAsset(AssetHandle asset, std::string_view description)` | `Result<sol::object>` | Parse a Lua asset file and return the resulting object |
| `executeLuaString(std::string_view code, std::string_view description)` | `bool` | Execute raw Lua code in the config sandbox; returns true on success |
| `getLuaState()` | `sol::state*` | Return a pointer to the internal Lua state for advanced use |

## Types

### ConfigMetadata

Metadata describing a loaded configuration file.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `sourcePath` | `std::string` | `""` | The file path the configuration was loaded from |
| `loadTime` | `std::chrono::steady_clock::time_point` | -- | The time at which the configuration was last loaded or reloaded |
| `isDirty` | `bool` | `false` | Whether the config has been modified at runtime since it was last loaded from disk |

## Lua Mapping

The Config System maps to two Lua namespaces:

- **`bestow.config`** maps to `IConfigSystem` (high-level). Methods are called directly: `bestow.config.getFloat("key")`.
- **`bestow.config.core`** maps to `IConfigCore` (low-level). Methods are called via the core sub-table: `bestow.config.core.loadConfig("path")`.

`std::optional` return values map to Lua values that are either the value or `nil`. `Result<T>` returns follow the Lua multi-return convention: `value, err` where `err` is `nil` on success or a table with a `message` field on failure. `SubscriptionId` is an opaque integer handle. `ConfigMetadata` maps to a Lua table with `sourcePath`, `loadTime`, and `isDirty` fields.

## Examples

### Lua

```lua
-- High-level: Read configuration values
local gravity = bestow.config.getFloat("physics.gravity")
local maxHP = bestow.config.getInt("player.maxHealth")
local debug = bestow.config.getBool("debug.enabled")
local title = bestow.config.getString("window.title")

-- Read with defaults (never nil)
local speed = bestow.config.getFloatOr("player.speed", 5.0)
local lives = bestow.config.getIntOr("player.lives", 3)
local vsync = bestow.config.getBoolOr("window.vsync", true)
local name = bestow.config.getStringOr("window.title", "Untitled")

-- Check existence
if bestow.config.hasKey("audio.masterVolume") then
    local vol = bestow.config.getFloat("audio.masterVolume")
end

-- Subscribe to any config change
local subId = bestow.config.onConfigChanged(function(key)
    print("Config changed: " .. key)
end)

-- Unsubscribe when done
bestow.config.unsubscribe(subId)

-- Low-level: Loading
bestow.config.core.loadConfig("config/game.lua")
bestow.config.core.loadConfigAsset(configAssetHandle)

-- Arrays
local waves = bestow.config.core.getIntArray("enemies.waveCounts")
local speeds = bestow.config.core.getFloatArray("enemies.speeds")
local names = bestow.config.core.getStringArray("characters.available")

-- Runtime modification
bestow.config.core.setFloat("difficulty.multiplier", 2.0)
bestow.config.core.setBool("debug.showHitboxes", true)
bestow.config.core.setInt("player.lives", 5)
bestow.config.core.setString("player.name", "Hero")

-- Key enumeration
local audioKeys = bestow.config.core.getKeysWithPrefix("audio")
for _, key in ipairs(audioKeys) do
    print(key .. " = " .. tostring(bestow.config.core.getFloat(key)))
end

-- Loaded config list
local loaded = bestow.config.core.getLoadedConfigs()
for _, path in ipairs(loaded) do
    print("Loaded: " .. path)
end

-- Metadata
local meta = bestow.config.core.getMetadata("config/game.lua")
print("Dirty: " .. tostring(meta.isDirty))

-- Hot reload
bestow.config.core.enableHotReload(true)
local subId = bestow.config.core.onKeyChanged("audio", function(key)
    print("Audio config changed: " .. key)
end)

-- Reload
bestow.config.core.reloadConfig("config/game.lua")
bestow.config.core.reloadAll()

-- Lua parsing and execution
local obj, err = bestow.config.core.parseLuaString("return { x = 1, y = 2 }", "inline")
bestow.config.core.executeLuaString([[
    difficulty = {
        easy   = { multiplier = 0.5, enemyHP = 50 },
        normal = { multiplier = 1.0, enemyHP = 100 },
        hard   = { multiplier = 2.0, enemyHP = 200 }
    }
]], "difficulty_config")
```

### C++

```cpp
// High-level: Read configuration values
auto gravity = config->getFloat("physics.gravity"); // std::optional<float>
auto maxHP = config->getInt("player.maxHealth");
auto debug = config->getBool("debug.enabled");
auto title = config->getString("window.title");

// Read with defaults
float speed = config->getFloatOr("player.speed", 5.0f);
int lives = config->getIntOr("player.lives", 3);
bool vsync = config->getBoolOr("window.vsync", true);
std::string name = config->getStringOr("window.title", "Untitled");

// Check existence
if (config->hasKey("audio.masterVolume")) {
    float vol = *config->getFloat("audio.masterVolume");
}

// Subscribe to config changes
auto subId = config->onConfigChanged(
    [](std::string_view key) {
        spdlog::info("Config changed: {}", key);
    });
config->unsubscribe(subId);

// Low-level: Loading
configCore->loadConfig("config/game.lua");
configCore->loadConfigAsset(configAssetHandle);

// Arrays
auto waves = configCore->getIntArray("enemies.waveCounts");
auto speeds = configCore->getFloatArray("enemies.speeds");
auto names = configCore->getStringArray("characters.available");

// Runtime modification
configCore->setFloat("difficulty.multiplier", 2.0f);
configCore->setBool("debug.showHitboxes", true);
configCore->setInt("player.lives", 5);
configCore->setString("player.name", "Hero");

// Key enumeration
auto audioKeys = configCore->getKeysWithPrefix("audio");
for (const auto& key : audioKeys) {
    spdlog::info("{} = {}", key, configCore->getFloat(key).value_or(0.0f));
}

// Loaded config list and metadata
auto loaded = configCore->getLoadedConfigs();
for (const auto& path : loaded) {
    auto meta = configCore->getMetadata(path);
    spdlog::info("Config: {} dirty={}", meta.sourcePath, meta.isDirty);
}

// Hot reload with scoped notifications
configCore->enableHotReload(true);
auto subId = configCore->onKeyChanged("audio",
    [](std::string_view key) {
        spdlog::info("Audio config changed: {}", key);
    });

// Reload
configCore->reloadConfig("config/game.lua");
configCore->reloadAll();

// Lua parsing and execution
auto result = configCore->parseLuaString("return { x = 1, y = 2 }", "inline");
if (result) {
    sol::object obj = *result;
    // Use the parsed Lua object
}

configCore->executeLuaString(R"(
    difficulty = {
        easy   = { multiplier = 0.5, enemyHP = 50 },
        normal = { multiplier = 1.0, enemyHP = 100 },
        hard   = { multiplier = 2.0, enemyHP = 200 }
    }
)", "difficulty_config");

// Access the raw Lua state for advanced use
sol::state* lua = configCore->getLuaState();

// Clean up
configCore->unsubscribe(subId);
```
