# Config System Developer Guide

## Overview

The Config System is Bestow's Lua-based configuration management system. It provides a sandboxed, type-safe interface for loading and managing game configuration data with hot reload support during development.

**Key Features:**
- Lua-powered configuration files (vs JSON's limitations)
- Type-safe value access with optional default fallbacks
- Nested tables and arrays
- Automatic hot reload during development
- Change notification callbacks
- Runtime value modification
- Sandboxed Lua execution for security
- Unified Lua parsing for other systems

**Why Lua over JSON:**
- Comments for documentation (`-- this is a comment`)
- Trailing commas allowed (no syntax errors)
- Variables and constants (`local GROUND_Y = 100`)
- Math expressions (`math.sin(angle) * radius`)
- Loops for generated data (`for i = 1, 10 do ... end`)
- Conditional values (`DEBUG and {...} or {}`)
- Functions for reusable patterns

## Core Concepts

### Config vs Game Data

**Config files** are Lua files that define:
- Game settings and constants
- Player stats and abilities
- Enemy behavior parameters
- UI layout values
- Physics constants
- Audio volumes

Config data is loaded once at startup (or hot-reloaded during development) and accessed frequently throughout the game loop.

**Game data** (blueprints, levels) are more complex structures managed by other systems (Blueprint, Level systems).

### Lua Sandboxing

All Lua code runs in a sandboxed environment with dangerous functions removed:
- `os`, `io` - File system access blocked
- `loadfile`, `dofile`, `require` - Dynamic code loading blocked
- `debug` - Debug API blocked
- `rawget`, `rawset` - Raw table access blocked

Safe libraries available:
- `base` - Basic Lua functions (print, type, pairs, ipairs, etc.)
- `math` - Math functions (sin, cos, sqrt, random, etc.)
- `table` - Table manipulation (insert, remove, sort, etc.)
- `string` - String functions (format, match, gsub, etc.)

Safe custom functions:
- `include(path)` - Load another config file via AssetSystem (path cannot contain `..` for security)

### Type-Safe Retrieval

Config values are stored with type information and retrieved through type-safe getters:

```cpp
// Returns std::optional<T> - nullopt if key missing or wrong type
auto speed = config->getFloat("player.speed");

// Returns T with default fallback
float speed = config->getFloatOr("player.speed", 100.0f);
```

Type conversions are automatic where logical:
- `getFloat()` can read int values (converts to float)
- `getInt()` can read float values (converts to int)
- `getFloatArray()` can read int arrays (converts to float array)

## API Reference

### Lifecycle

```cpp
// Initialize the config system (called by engine)
bool initialize();

// Update - processes hot reload checks (called by engine)
void update(DeltaTime dt);

// Shutdown and release resources (called by engine)
void shutdown();
```

You typically don't call these directly - the engine handles the lifecycle.

### Configuration Loading

```cpp
// Load a Lua config file from path (via AssetSystem)
bool loadConfig(const std::string& filePath);
// Example: config->loadConfig("config/player.lua")

// Load config from pre-registered asset handle
bool loadConfigAsset(AssetHandle configAsset);

// Reload all loaded config files (for manual hot reload)
bool reloadAll();

// Reload a specific config file
bool reloadConfig(const std::string& filePath);
```

**Config File Format:**
All config files must return a table:
```lua
-- config/player.lua
return {
    -- Your config values here
    speed = 100.0,
    health = 100
}
```

### Type-Safe Value Access

```cpp
// Get value as optional (returns nullopt if missing/wrong type)
std::optional<float> getFloat(const ConfigKey& key) const;
std::optional<int> getInt(const ConfigKey& key) const;
std::optional<bool> getBool(const ConfigKey& key) const;
std::optional<std::string> getString(const ConfigKey& key) const;

// Get value with default fallback (recommended)
float getFloatOr(const ConfigKey& key, float defaultValue) const;
int getIntOr(const ConfigKey& key, int defaultValue) const;
bool getBoolOr(const ConfigKey& key, bool defaultValue) const;
std::string getStringOr(const ConfigKey& key, const std::string& defaultValue) const;
```

**Usage:**
```cpp
// Check if value exists before using
if (auto speed = config->getFloat("player.speed")) {
    player.setSpeed(*speed);
}

// Use default fallback for safe access (preferred)
float gravity = config->getFloatOr("physics.gravity", -980.0f);
int maxHealth = config->getIntOr("player.health.maximum", 100);
bool debugMode = config->getBoolOr("debug.enabled", false);
```

### Array Access

```cpp
// Get array of values (returns empty vector if missing/wrong type)
std::vector<int> getIntArray(const ConfigKey& key) const;
std::vector<float> getFloatArray(const ConfigKey& key) const;
std::vector<std::string> getStringArray(const ConfigKey& key) const;
```

**Usage:**
```cpp
// Animation frame IDs
auto frames = config->getIntArray("animations.walk.frames");
for (int frameId : frames) {
    // Use frame ID
}

// Spawn positions
auto positions = config->getFloatArray("level.spawn_x_positions");

// Item names
auto items = config->getStringArray("inventory.starting_items");
```

### Runtime Value Modification

```cpp
// Set values at runtime (does not modify files, in-memory only)
void setFloat(const ConfigKey& key, float value);
void setInt(const ConfigKey& key, int value);
void setBool(const ConfigKey& key, bool value);
void setString(const ConfigKey& key, const std::string& value);
```

**Usage:**
```cpp
// Runtime tweaking (e.g., from debug UI)
config->setFloat("player.speed", 150.0f);
config->setBool("debug.show_colliders", true);

// Note: Changes are in-memory only, not persisted to file
// When the file is hot reloaded, your changes will be overwritten
```

### State Queries

```cpp
// Check if a key exists
bool hasKey(const ConfigKey& key) const;

// Get all keys matching a prefix
std::vector<ConfigKey> getKeysWithPrefix(const std::string& prefix) const;

// Get all loaded config file paths
std::vector<std::string> getLoadedConfigs() const;

// Get metadata for a config file
ConfigMetadata getMetadata(const std::string& filePath) const;
```

**Usage:**
```cpp
// Check before accessing
if (config->hasKey("player.special_ability")) {
    // Handle special ability
}

// Get all player-related keys
auto playerKeys = config->getKeysWithPrefix("player.");
// Returns: ["player.speed", "player.health.initial", "player.health.maximum", ...]

// List loaded configs
for (const auto& path : config->getLoadedConfigs()) {
    logInfo("Loaded config: {}", path);
}

// Check when a config was last loaded
auto metadata = config->getMetadata("config/player.lua");
logInfo("Player config loaded at {}", metadata.loadTime);
```

### Hot Reload

```cpp
// Enable/disable hot reload file watching (via AssetSystem)
void enableHotReload(bool enable);

// Check if hot reload is enabled
bool isHotReloadEnabled() const;
```

**Usage:**
```cpp
// Enable hot reload during development
#if defined(BESTOW_DEV_TOOLS)
    config->enableHotReload(true);
#endif

// Hot reload is event-driven - no polling needed
// When you edit a config file, it automatically reloads
```

### Change Notifications

```cpp
// Subscribe to ALL config changes
SubscriptionId onConfigChanged(ConfigChangeCallback callback);

// Subscribe to changes for keys with a specific prefix
SubscriptionId onKeyChanged(const std::string& keyPrefix,
                           ConfigChangeCallback callback);

// Unsubscribe from notifications
void unsubscribe(SubscriptionId id);
```

**Callback signature:**
```cpp
using ConfigChangeCallback = std::function<void(const ConfigKey& key)>;
```

**Usage:**
```cpp
// Subscribe to player config changes
SubscriptionId subId = config->onKeyChanged("player.",
    [this](const ConfigKey& key) {
        logInfo("Player config changed: {}", key);
        reloadPlayerStats();
    });

// Clean up when done
config->unsubscribe(subId);
```

### Unified Lua Parsing

The Config System provides sandboxed Lua parsing for other systems to use instead of creating their own `sol::state`:

```cpp
// Parse Lua string and get result
std::optional<sol::object> parseLuaString(
    const std::string& luaCode,
    const std::string& description = "lua");

// Parse Lua asset (integrates with AssetSystem hot reload)
std::optional<sol::object> parseLuaAsset(
    AssetHandle luaAsset,
    const std::string& description = "lua");

// Execute Lua code for side effects (defining globals, functions, etc.)
bool executeLuaString(
    const std::string& luaCode,
    const std::string& description = "lua");

// Get direct access to the sandboxed Lua state for advanced use cases
// WARNING: The state is shared; be careful with modifications
sol::state* getLuaState();
```

**Usage:**
```cpp
// Other systems should use ConfigSystem for Lua parsing
auto result = config->parseLuaString(R"(
    return {
        name = "Player",
        speed = 100
    }
)", "inline config");

if (result) {
    sol::table table = result->as<sol::table>();
    std::string name = table["name"];
    float speed = table["speed"];
}

// Parse Lua from asset (e.g., blueprint, level)
AssetHandle levelAsset = assets->registerAsset(AssetType::Data, "levels/level1.lua");
assets->loadAsset(levelAsset);

auto levelData = config->parseLuaAsset(levelAsset, "level");
if (levelData) {
    sol::table level = levelData->as<sol::table>();
    std::string levelName = level["name"];
    int levelWidth = level["width"];
    // ... parse level data
}
```

## Lua Config Format

### Basic Config Structure

All config files must return a table:

```lua
-- config/player.lua
return {
    speed = 100.0,
    health = 100,
    jumpForce = 500.0,
    canDoubleJump = true,
    name = "Hero"
}
```

Access in C++:
```cpp
config->loadConfig("config/player.lua");

float speed = config->getFloatOr("speed", 100.0f);
int health = config->getIntOr("health", 100);
bool canDoubleJump = config->getBoolOr("canDoubleJump", false);
```

### Nested Tables

Use nested tables for organization:

```lua
-- config/game.lua
return {
    player = {
        speed = 100.0,
        health = {
            initial = 100,
            maximum = 100
        },
        jump = {
            maxJumps = 2,
            jumpForce = 800.0
        }
    },

    physics = {
        gravity = -980.0,
        terminalVelocity = -1000.0,
        friction = 0.8
    },

    audio = {
        masterVolume = 1.0,
        musicVolume = 0.7,
        sfxVolume = 0.9
    }
}
```

Access with dot notation:
```cpp
config->loadConfig("config/game.lua");

float speed = config->getFloatOr("player.speed", 100.0f);
int maxHealth = config->getIntOr("player.health.maximum", 100);
float gravity = config->getFloatOr("physics.gravity", -980.0f);
float musicVol = config->getFloatOr("audio.musicVolume", 0.7f);
```

### Arrays

Lua arrays use 1-based indexing and sequential keys:

```lua
-- config/animations.lua
return {
    walk = {
        frames = {1, 2, 3, 4, 5, 6},
        frameDuration = 0.1
    },

    idle = {
        frames = {7, 8},
        frameDuration = 0.5
    },

    spawnPoints = {
        {x = 100, y = 200},
        {x = 300, y = 200},
        {x = 500, y = 200}
    },

    tags = {"player", "controllable", "damageable"}
}
```

Access arrays:
```cpp
config->loadConfig("config/animations.lua");

auto walkFrames = config->getIntArray("walk.frames");
float frameDuration = config->getFloatOr("walk.frameDuration", 0.1f);

auto tags = config->getStringArray("tags");
```

### Using Lua Features

#### Variables and Constants

```lua
-- config/level.lua
local TILE_SIZE = 32
local SCREEN_WIDTH = 1280
local SCREEN_HEIGHT = 720

local TILES_X = SCREEN_WIDTH / TILE_SIZE
local TILES_Y = SCREEN_HEIGHT / TILE_SIZE

return {
    tileSize = TILE_SIZE,
    gridWidth = TILES_X,
    gridHeight = TILES_Y,
    totalTiles = TILES_X * TILES_Y
}
```

#### Math Expressions

```lua
-- config/enemies.lua
return {
    slime = {
        health = 50,
        speed = 60.0,
        damage = 10
    },

    goblin = {
        health = 100,
        speed = 80.0,
        damage = 15
    },

    boss = {
        health = 1000,
        speed = 50.0,
        damage = 25,
        -- Boss is 10x tougher than goblin
        toughnessMultiplier = math.floor(1000 / 100)
    }
}
```

#### Loops for Generated Data

```lua
-- config/waves.lua
local waves = {}

-- Generate 10 waves of increasing difficulty
for i = 1, 10 do
    waves[i] = {
        enemyCount = i * 5,
        healthMultiplier = 1.0 + (i * 0.1),
        speedMultiplier = 1.0 + (i * 0.05),
        spawnInterval = math.max(1.0, 2.0 - (i * 0.1))
    }
end

return {
    waves = waves
}
```

Access generated data:
```cpp
for (int i = 1; i <= 10; i++) {
    std::string prefix = std::format("waves.{}", i);
    int enemyCount = config->getIntOr(prefix + ".enemyCount", 5);
    float healthMult = config->getFloatOr(prefix + ".healthMultiplier", 1.0f);
    // ... use wave data
}
```

#### Conditional Values

```lua
-- config/debug.lua
local DEBUG = true  -- Toggle debug mode

return {
    debugEnabled = DEBUG,

    player = {
        invincible = DEBUG,
        speed = DEBUG and 500.0 or 100.0,
        startingHealth = DEBUG and 9999 or 100
    },

    enemy = {
        spawnEnabled = not DEBUG,
        damageMultiplier = DEBUG and 0.0 or 1.0
    },

    rendering = {
        showColliders = DEBUG,
        showFPS = DEBUG,
        vsync = not DEBUG
    }
}
```

#### Comments

```lua
-- config/player.lua
return {
    -- Movement
    speed = 100.0,          -- Pixels per second
    jumpForce = 500.0,      -- Initial upward velocity
    maxFallSpeed = -1000.0, -- Terminal velocity

    -- Combat
    health = 100,           -- Hit points
    attackDamage = 25,      -- Damage per hit
    attackCooldown = 0.5,   -- Seconds between attacks

    -- Abilities
    canDoubleJump = true,   -- Enable double jump mechanic
    dashDistance = 150.0,   -- Dash distance in pixels
    dashCooldown = 2.0      -- Seconds before dash can be used again
}
```

#### Including Other Config Files

Use the `include()` function to load other config files via AssetSystem:

```lua
-- config/main.lua
local player = include("config/player.lua")
local enemies = include("config/enemies.lua")

return {
    player = player,
    enemies = enemies,

    game = {
        difficulty = "normal",
        startingLevel = 1
    }
}
```

Note: `include()` paths cannot contain `..` for security.

## Hot Reload Workflow

### Enabling Hot Reload

Hot reload is powered by the AssetSystem's file watching:

```cpp
void Game::init() {
    auto& sys = engine.systems();

    // Enable hot reload in debug builds
    #if defined(BESTOW_DEV_TOOLS)
        sys.config->enableHotReload(true);
        logInfo("Config hot reload enabled");
    #endif

    // Load configs
    sys.config->loadConfig("config/player.lua");
    sys.config->loadConfig("config/game.lua");
}
```

### Subscribing to Config Changes

Handle config changes in your game systems:

```cpp
class PlayerSystem {
public:
    void init(IConfigSystem* config) {
        config_ = config;

        // Subscribe to player config changes
        configSubId_ = config_->onKeyChanged("player.",
            [this](const ConfigKey& key) {
                logInfo("Player config changed: {}", key);
                loadPlayerConfig();
            });

        // Load initial values
        loadPlayerConfig();
    }

    void shutdown() {
        config_->unsubscribe(configSubId_);
    }

private:
    void loadPlayerConfig() {
        playerSpeed_ = config_->getFloatOr("player.speed", 100.0f);
        maxHealth_ = config_->getIntOr("player.health.maximum", 100);
        jumpForce_ = config_->getFloatOr("player.jump.jumpForce", 500.0f);
        // ... load other values

        logInfo("Player config loaded: speed={}, health={}",
                playerSpeed_, maxHealth_);
    }

    IConfigSystem* config_ = nullptr;
    SubscriptionId configSubId_ = 0;

    float playerSpeed_ = 100.0f;
    int maxHealth_ = 100;
    float jumpForce_ = 500.0f;
};
```

### Hot Reload Flow

1. **Start game in debug mode** - Hot reload is enabled
2. **Edit config file** - Change values in your editor
3. **Save file** - AssetSystem detects the change
4. **Automatic reload** - Config file is re-executed
5. **Notifications sent** - All subscribers receive callbacks
6. **Game updates** - Systems apply new values

Example workflow:
```lua
-- Edit config/player.lua while game is running
return {
    speed = 150.0,       -- Changed from 100.0
    jumpForce = 600.0    -- Changed from 500.0
}

-- Save file
-- Console output:
-- [Config] Hot reload: config/player.lua modified, reloading
-- [Config] Loaded config from config/player.lua
-- Player config changed: player.speed
-- Player config changed: player.jump.jumpForce
-- Player config loaded: speed=150, health=100

-- Player immediately moves faster and jumps higher in-game!
```

## Best Practices

### 1. Config File Organization

Organize configs by system or feature:

```
data/config/
├── game.lua           # Global game settings
├── player.lua         # Player-specific config
├── enemies.lua        # Enemy definitions
├── physics.lua        # Physics constants
├── audio.lua          # Audio volumes
├── ui.lua             # UI layout values
└── input.lua          # Input mappings
```

### 2. Use Nested Tables for Structure

Group related values using nested tables:

```lua
-- GOOD - Organized with nested tables
return {
    player = {
        speed = 100.0,
        health = 100,
        jump = {
            maxJumps = 2,
            jumpForce = 800.0
        }
    }
}

-- BAD - Flat structure with prefixes
return {
    playerSpeed = 100.0,
    playerHealth = 100,
    playerMaxJumps = 2,
    playerJumpForce = 800.0
}
```

### 3. Always Provide Default Values

Use `getXxxOr()` methods to provide safe defaults:

```cpp
// BAD - Can return nullopt, requires checking
auto speed = config->getFloat("player.speed");
if (speed) {
    player.setSpeed(*speed);
}

// GOOD - Always has a value
float speed = config->getFloatOr("player.speed", 100.0f);
player.setSpeed(speed);
```

### 4. Cache Frequently-Accessed Values

Don't call `getXxx()` every frame - cache values and reload on change:

```cpp
// BAD - Config lookup every frame
void update(float dt) {
    float speed = config->getFloatOr("player.speed", 100.0f);
    player.move(speed * dt);
}

// GOOD - Cache the value
void init() {
    playerSpeed_ = config->getFloatOr("player.speed", 100.0f);
}

void update(float dt) {
    player.move(playerSpeed_ * dt);
}

void onConfigChanged(const ConfigKey& key) {
    if (key == "player.speed") {
        playerSpeed_ = config->getFloatOr("player.speed", 100.0f);
    }
}
```

### 5. Use Comments Extensively

Lua allows comments - use them to document your config:

```lua
return {
    -- Player Movement
    -- All speeds are in pixels per second
    speed = 100.0,        -- Normal walking speed
    sprintSpeed = 200.0,  -- Sprinting speed (2x normal)

    -- Jump Physics
    jumpForce = 500.0,    -- Initial upward velocity
    gravity = -980.0,     -- Acceleration due to gravity
    maxFallSpeed = -1000.0, -- Terminal velocity

    -- EXPERIMENTAL: Double jump mechanic
    -- TODO: Needs playtesting
    canDoubleJump = false,
    doubleJumpForce = 400.0
}
```

### 6. Separate Config from Game Data

**Config** = Simple values, constants, settings
**Game Data** = Complex structures, blueprints, levels

```lua
-- GOOD - Config file (simple values)
return {
    enemyHealth = 50,
    enemySpeed = 60.0,
    enemyDamage = 10
}

-- BAD - This should be a Blueprint, not Config
return {
    enemies = {
        {
            type = "slime",
            position = {x = 100, y = 200},
            patrolPath = {{0, 0}, {100, 0}, {100, 100}},
            aiBehavior = function() ... end  -- Too complex for config
        }
    }
}
```

### 7. Validate Critical Values

Check that critical config values are reasonable:

```cpp
void loadPlayerConfig() {
    float speed = config_->getFloatOr("player.speed", 100.0f);

    // Validate
    if (speed <= 0.0f || speed > 1000.0f) {
        logWarn("Invalid player speed {}, clamping to [1, 1000]", speed);
        speed = std::clamp(speed, 1.0f, 1000.0f);
    }

    playerSpeed_ = speed;
}
```

## Code Examples

### Example 1: Loading Player Config

```lua
-- config/player.lua
return {
    moveSpeed = 400.0,
    health = {
        initial = 100,
        maximum = 100
    },
    jump = {
        maxJumps = 2,  -- Double jump enabled
        jumpForce = 800.0
    },
    animation = {
        runningThreshold = 10.0,
        jumpingThreshold = -10.0
    },
    spawnFallback = { x = 100.0, y = 400.0 }
}
```

```cpp
class PlayerSystem {
public:
    void init(IConfigSystem* config) {
        config_ = config;
        config_->loadConfig("config/player.lua");
        loadPlayerConfig();

        // Subscribe to changes for hot reload
        configSubId_ = config_->onKeyChanged("player.", [this](auto&) {
            loadPlayerConfig();
        });
    }

    void loadPlayerConfig() {
        moveSpeed_ = config_->getFloatOr("moveSpeed", 400.0f);
        maxHealth_ = config_->getIntOr("health.maximum", 100);
        maxJumps_ = config_->getIntOr("jump.maxJumps", 2);
        jumpForce_ = config_->getFloatOr("jump.jumpForce", 800.0f);

        logInfo("Player config: speed={}, health={}, jumps={}",
                moveSpeed_, maxHealth_, maxJumps_);
    }

private:
    IConfigSystem* config_;
    SubscriptionId configSubId_;
    float moveSpeed_;
    int maxHealth_;
    int maxJumps_;
    float jumpForce_;
};
```

### Example 2: Game Settings Config

```lua
-- config/game.lua
return {
    window = {
        title = "My Platformer",
        width = 1280,
        height = 720,
        fullscreen = false,
        vsync = true
    },

    audio = {
        masterVolume = 1.0,
        musicVolume = 0.7,
        sfxVolume = 0.9
    },

    gameplay = {
        difficulty = "normal",
        startingLives = 3,
        timeLimitSeconds = 300
    }
}
```

```cpp
void Game::init() {
    auto& sys = engine.systems();
    sys.config->loadConfig("config/game.lua");

    // Window settings
    windowTitle_ = sys.config->getStringOr("window.title", "Game");
    windowWidth_ = sys.config->getIntOr("window.width", 1280);
    windowHeight_ = sys.config->getIntOr("window.height", 720);

    // Audio settings
    float masterVol = sys.config->getFloatOr("audio.masterVolume", 1.0f);
    float musicVol = sys.config->getFloatOr("audio.musicVolume", 0.7f);
    sys.audio->setMasterVolume(masterVol);
    sys.audio->setMusicVolume(musicVol);

    // Gameplay settings
    difficulty_ = sys.config->getStringOr("gameplay.difficulty", "normal");
    startingLives_ = sys.config->getIntOr("gameplay.startingLives", 3);
}
```

### Example 3: Generated Wave Data

```lua
-- config/waves.lua
local waves = {}

-- Generate 20 waves of increasing difficulty
for wave = 1, 20 do
    local difficulty = 1.0 + (wave * 0.15)

    waves[wave] = {
        enemyCount = 5 + (wave * 2),
        healthMultiplier = difficulty,
        speedMultiplier = 1.0 + (wave * 0.05),
        spawnInterval = math.max(0.5, 2.0 - (wave * 0.1)),
        hasBoss = (wave % 5) == 0,
        goldReward = 100 * wave
    }
end

return {
    waves = waves,
    startingWave = 1,
    maxWaves = 20
}
```

```cpp
struct WaveConfig {
    int enemyCount;
    float healthMultiplier;
    float spawnInterval;
    bool hasBoss;
    int goldReward;
};

class WaveManager {
public:
    void loadConfig(IConfigSystem* config) {
        int waveCount = config->getIntOr("maxWaves", 10);

        for (int i = 1; i <= waveCount; i++) {
            std::string prefix = std::format("waves.{}", i);

            WaveConfig wave;
            wave.enemyCount = config->getIntOr(prefix + ".enemyCount", 5);
            wave.healthMultiplier = config->getFloatOr(prefix + ".healthMultiplier", 1.0f);
            wave.spawnInterval = config->getFloatOr(prefix + ".spawnInterval", 2.0f);
            wave.hasBoss = config->getBoolOr(prefix + ".hasBoss", false);
            wave.goldReward = config->getIntOr(prefix + ".goldReward", 100);

            waves_.push_back(wave);
        }

        logInfo("Loaded {} wave configurations", waves_.size());
    }

private:
    std::vector<WaveConfig> waves_;
};
```

## Integration with Other Systems

### Using ConfigSystem for Lua Parsing

Other systems should use ConfigSystem for Lua parsing instead of creating their own `sol::state`:

```cpp
// Blueprint system parses Lua blueprints
class BlueprintSystem {
    void loadBlueprint(const std::string& path) {
        AssetHandle asset = assets_->registerAsset(AssetType::Data, path);
        assets_->loadAsset(asset);

        // Use ConfigSystem for sandboxed Lua parsing
        auto result = config_->parseLuaAsset(asset, "blueprint");
        if (!result) {
            logError("Failed to parse blueprint: {}", path);
            return;
        }

        sol::table blueprint = result->as<sol::table>();
        // ... process blueprint
    }

private:
    IConfigSystem* config_;
    IAssetSystem* assets_;
};
```

### Config and Hot Reload

ConfigSystem integrates with AssetSystem for hot reload:

1. Load config via `loadConfig()` - registers asset internally
2. Enable hot reload - `enableHotReload(true)`
3. Subscribe to changes - `onKeyChanged()` or `onConfigChanged()`
4. Edit file - AssetSystem detects change
5. Automatic reload - ConfigSystem reloads the Lua file
6. Notifications - Your callbacks are invoked
7. Apply changes - Your systems update their cached values

## Summary

The Config System provides a powerful, Lua-based configuration solution with:

- Type-safe value access with optional defaults
- Hot reload for rapid iteration
- Nested tables and arrays
- Lua's full expressiveness (math, loops, conditionals)
- Sandboxed execution for security
- Change notifications for reactive systems
- Unified Lua parsing for other systems

**Quick Checklist:**

- Load configs at startup: `config->loadConfig("config/game.lua")`
- Enable hot reload in debug: `config->enableHotReload(true)`
- Use `getXxxOr()` with defaults for safe access
- Subscribe to changes: `config->onKeyChanged("prefix.", callback)`
- Cache frequently-accessed values
- Use Lua features: comments, math, variables, loops
- Organize configs by system/feature
- Validate critical values
- Use nested tables for structure
- Let other systems use ConfigSystem for Lua parsing
