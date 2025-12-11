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

### Configuration Loading

```cpp
// Load a Lua config file from path (via AssetSystem)
bool loadConfig(const std::string& filePath);
// Example: config->loadConfig("data/config/player.lua")

// Load config from pre-registered asset handle
bool loadConfigAsset(AssetHandle configAsset);

// Reload all loaded config files (for manual hot reload)
bool reloadAll();

// Reload a specific config file
bool reloadConfig(const std::string& filePath);
```

**Config File Format:**
```lua
-- File must return a table
return {
    -- Your config values here
}
```

### Type-Safe Value Access

```cpp
// Get value as optional (returns nullopt if missing/wrong type)
std::optional<float> getFloat(const ConfigKey& key) const;
std::optional<int> getInt(const ConfigKey& key) const;
std::optional<bool> getBool(const ConfigKey& key) const;
std::optional<std::string> getString(const ConfigKey& key) const;

// Get value with default fallback
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

// Use default fallback for safe access
float gravity = config->getFloatOr("physics.gravity", -980.0f);
int maxHealth = config->getIntOr("player.max_health", 100);
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
// Set values at runtime (does not modify files)
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
// Returns: ["player.speed", "player.max_health", "player.jump_force", ...]

// List loaded configs
for (const auto& path : config->getLoadedConfigs()) {
    spdlog::info("Loaded config: {}", path);
}
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

**Usage:**
```cpp
// Subscribe to player config changes
SubscriptionId subId = config->onKeyChanged("player.",
    [this](const ConfigKey& key) {
        spdlog::info("Player config changed: {}", key);
        reloadPlayerStats();
    });

// Clean up when done
config->unsubscribe(subId);
```

### Unified Lua Parsing

The Config System provides sandboxed Lua parsing for other systems to use:

```cpp
// Parse Lua string and get result object
std::optional<sol::object> parseLuaString(
    const std::string& luaCode,
    const std::string& description = "lua");

// Parse Lua asset (integrates with AssetSystem hot reload)
std::optional<sol::object> parseLuaAsset(
    AssetHandle luaAsset,
    const std::string& description = "lua");

// Execute Lua code for side effects (no return value)
bool executeLuaString(
    const std::string& luaCode,
    const std::string& description = "lua");

// Get direct access to Lua state (advanced use only)
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
```

## Lua Config Format

### Basic Config Structure

All config files must return a table:

```lua
-- data/config/player.lua
return {
    speed = 100.0,
    max_health = 100,
    jump_force = 500.0,
    can_double_jump = true,
    name = "Hero"
}
```

Access in C++:
```cpp
config->loadConfig("data/config/player.lua");

float speed = config->getFloatOr("speed", 100.0f);
int maxHealth = config->getIntOr("max_health", 100);
bool canDoubleJump = config->getBoolOr("can_double_jump", false);
```

### Nested Tables

Use nested tables for organization:

```lua
-- data/config/game.lua
return {
    player = {
        speed = 100.0,
        max_health = 100,
        jump_force = 500.0
    },

    physics = {
        gravity = -980.0,
        terminal_velocity = -1000.0,
        friction = 0.8
    },

    audio = {
        master_volume = 1.0,
        music_volume = 0.7,
        sfx_volume = 0.9
    }
}
```

Access with dot notation:
```cpp
config->loadConfig("data/config/game.lua");

float speed = config->getFloatOr("player.speed", 100.0f);
float gravity = config->getFloatOr("physics.gravity", -980.0f);
float musicVol = config->getFloatOr("audio.music_volume", 0.7f);
```

### Arrays

Lua arrays use 1-based indexing and sequential keys:

```lua
-- data/config/animations.lua
return {
    walk = {
        frames = {1, 2, 3, 4, 5, 6},
        frame_duration = 0.1
    },

    idle = {
        frames = {7, 8},
        frame_duration = 0.5
    },

    spawn_points = {
        {x = 100, y = 200},
        {x = 300, y = 200},
        {x = 500, y = 200}
    },

    tags = {"player", "controllable", "damageable"}
}
```

Access arrays:
```cpp
config->loadConfig("data/config/animations.lua");

auto walkFrames = config->getIntArray("walk.frames");
float frameDuration = config->getFloatOr("walk.frame_duration", 0.1f);

auto tags = config->getStringArray("tags");
```

### Using Lua Features

#### Variables and Constants

```lua
-- data/config/level.lua
local TILE_SIZE = 32
local SCREEN_WIDTH = 1280
local SCREEN_HEIGHT = 720

local TILES_X = SCREEN_WIDTH / TILE_SIZE
local TILES_Y = SCREEN_HEIGHT / TILE_SIZE

return {
    tile_size = TILE_SIZE,
    grid_width = TILES_X,
    grid_height = TILES_Y,
    total_tiles = TILES_X * TILES_Y
}
```

#### Math Expressions

```lua
-- data/config/enemies.lua
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
        -- Boss is 5x tougher than goblin
        toughness_multiplier = math.floor(1000 / 100)
    }
}
```

#### Loops for Generated Data

```lua
-- data/config/waves.lua
local waves = {}

-- Generate 10 waves of increasing difficulty
for i = 1, 10 do
    waves[i] = {
        enemy_count = i * 5,
        enemy_health_multiplier = 1.0 + (i * 0.1),
        enemy_speed_multiplier = 1.0 + (i * 0.05),
        spawn_interval = math.max(1.0, 2.0 - (i * 0.1))
    }
end

return {
    waves = waves
}
```

#### Conditional Values

```lua
-- data/config/debug.lua
local DEBUG = true  -- Toggle debug mode

return {
    debug_enabled = DEBUG,

    player = {
        invincible = DEBUG,
        speed = DEBUG and 500.0 or 100.0,
        starting_health = DEBUG and 9999 or 100
    },

    enemy = {
        spawn_enabled = not DEBUG,
        damage_multiplier = DEBUG and 0.0 or 1.0
    },

    rendering = {
        show_colliders = DEBUG,
        show_fps = DEBUG,
        vsync = not DEBUG
    }
}
```

#### Comments

```lua
-- data/config/player.lua
return {
    -- Movement
    speed = 100.0,          -- Pixels per second
    jump_force = 500.0,     -- Initial upward velocity
    max_fall_speed = -1000.0, -- Terminal velocity

    -- Combat
    max_health = 100,       -- Hit points
    attack_damage = 25,     -- Damage per hit
    attack_cooldown = 0.5,  -- Seconds between attacks

    -- Abilities
    can_double_jump = true, -- Enable double jump mechanic
    dash_distance = 150.0,  -- Dash distance in pixels
    dash_cooldown = 2.0     -- Seconds before dash can be used again
}
```

#### Including Other Config Files

Use the `include()` function to load other config files:

```lua
-- data/config/main.lua
local player = include("data/config/player.lua")
local enemies = include("data/config/enemies.lua")

return {
    player = player,
    enemies = enemies,

    game = {
        difficulty = "normal",
        starting_level = 1
    }
}
```

Note: `include()` paths cannot contain `..` for security.

## Hot Reload

### Enabling Hot Reload

Hot reload is powered by the AssetSystem's file watching via efsw:

```cpp
void Game::init() {
    // Enable hot reload in debug builds
    #if defined(BESTOW_DEV_TOOLS)
        config_->enableHotReload(true);
        spdlog::info("Config hot reload enabled");
    #endif

    // Load configs
    config_->loadConfig("data/config/player.lua");
    config_->loadConfig("data/config/game.lua");
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
                handleConfigChange(key);
            });

        // Load initial values
        loadPlayerConfig();
    }

    void shutdown() {
        config_->unsubscribe(configSubId_);
    }

private:
    void handleConfigChange(const ConfigKey& key) {
        spdlog::info("Player config changed: {}", key);

        if (key == "player.speed") {
            playerSpeed_ = config_->getFloatOr("player.speed", 100.0f);
        } else if (key == "player.max_health") {
            maxHealth_ = config_->getIntOr("player.max_health", 100);
        } else {
            // Other keys changed, reload all
            loadPlayerConfig();
        }
    }

    void loadPlayerConfig() {
        playerSpeed_ = config_->getFloatOr("player.speed", 100.0f);
        maxHealth_ = config_->getIntOr("player.max_health", 100);
        jumpForce_ = config_->getFloatOr("player.jump_force", 500.0f);
        // ... load other values
    }

    IConfigSystem* config_ = nullptr;
    SubscriptionId configSubId_ = 0;

    float playerSpeed_ = 100.0f;
    int maxHealth_ = 100;
    float jumpForce_ = 500.0f;
};
```

### Hot Reload Workflow

1. **Start game in debug mode** - Hot reload is enabled
2. **Edit config file** - Change values in your editor
3. **Save file** - AssetSystem detects the change via efsw
4. **Automatic reload** - Config file is re-executed
5. **Notifications sent** - All subscribers receive callbacks
6. **Game updates** - Systems apply new values

Example workflow:
```lua
-- Edit data/config/player.lua while game is running
return {
    speed = 150.0,  -- Changed from 100.0
    jump_force = 600.0  -- Changed from 500.0
}

-- Save file
-- Console output:
-- [Config] Hot reload: data/config/player.lua modified, reloading
-- [Config] Loaded 2 config values from data/config/player.lua
-- Player config changed: player.speed
-- Player config changed: player.jump_force

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
└── levels/
    ├── level1.lua
    ├── level2.lua
    └── level3.lua
```

### 2. Use Namespaces in Flat Configs

If you load multiple configs, use unique prefixes:

```lua
-- data/config/player.lua
return {
    player_speed = 100.0,
    player_health = 100,
    player_jump = 500.0
}

-- data/config/enemy.lua
return {
    enemy_speed = 60.0,
    enemy_health = 50,
    enemy_damage = 10
}
```

Or better yet, use nested tables in a single file:

```lua
-- data/config/game.lua
return {
    player = {
        speed = 100.0,
        health = 100,
        jump_force = 500.0
    },

    enemy = {
        speed = 60.0,
        health = 50,
        damage = 10
    }
}
```

### 3. Always Provide Default Values

Use `getXxxOr()` methods to provide safe defaults:

```cpp
// BAD - Can return nullopt
auto speed = config->getFloat("player.speed");
if (speed) {
    player.setSpeed(*speed);
}

// GOOD - Always has a value
float speed = config->getFloatOr("player.speed", 100.0f);
player.setSpeed(speed);
```

### 4. Validate Critical Values

Check that critical config values are reasonable:

```cpp
void loadPlayerConfig() {
    float speed = config->getFloatOr("player.speed", 100.0f);

    // Validate
    if (speed <= 0.0f || speed > 1000.0f) {
        spdlog::warn("Invalid player speed {}, clamping to [1, 1000]", speed);
        speed = std::clamp(speed, 1.0f, 1000.0f);
    }

    playerSpeed_ = speed;
}
```

### 5. Cache Frequently-Accessed Values

Don't call `getXxx()` every frame:

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

### 6. Separate Config from Game Data

**Config** = Simple values, constants, settings
**Game Data** = Complex structures, blueprints, levels

```lua
-- GOOD - Config file
return {
    enemy_health = 50,
    enemy_speed = 60.0,
    enemy_damage = 10
}

-- BAD - This should be a Blueprint, not Config
return {
    enemies = {
        {
            type = "slime",
            position = {x = 100, y = 200},
            patrol_path = {{0, 0}, {100, 0}, {100, 100}},
            ai_behavior = function() ... end
        }
    }
}
```

### 7. Use Comments Extensively

Lua allows comments - use them!

```lua
return {
    -- Player Movement
    -- All speeds are in pixels per second
    speed = 100.0,        -- Normal walking speed
    sprint_speed = 200.0, -- Sprinting speed (2x normal)

    -- Jump Physics
    jump_force = 500.0,   -- Initial upward velocity
    gravity = -980.0,     -- Acceleration due to gravity
    max_fall_speed = -1000.0, -- Terminal velocity

    -- EXPERIMENTAL: Double jump mechanic
    -- TODO: Needs playtesting
    can_double_jump = false,
    double_jump_force = 400.0
}
```

### 8. Version Your Config Files

For games with updates/DLC, version your configs:

```lua
-- data/config/game.lua
return {
    _config_version = "1.2.0",

    -- ... config values
}
```

Then validate:

```cpp
void loadGameConfig() {
    config->loadConfig("data/config/game.lua");

    auto version = config->getString("_config_version");
    if (!version || *version != "1.2.0") {
        spdlog::error("Config version mismatch! Expected 1.2.0, got {}",
                     version.value_or("unknown"));
    }
}
```

## Code Examples

### Example 1: Game Settings Config

```lua
-- data/config/game.lua
return {
    window = {
        title = "My Platformer",
        width = 1280,
        height = 720,
        fullscreen = false,
        vsync = true
    },

    audio = {
        master_volume = 1.0,
        music_volume = 0.7,
        sfx_volume = 0.9,
        mute_when_unfocused = true
    },

    gameplay = {
        difficulty = "normal", -- "easy", "normal", "hard"
        starting_lives = 3,
        time_limit_seconds = 300,
        enable_powerups = true
    }
}
```

Loading in C++:

```cpp
void Game::init() {
    config_->loadConfig("data/config/game.lua");

    // Window settings
    windowTitle_ = config_->getStringOr("window.title", "Game");
    windowWidth_ = config_->getIntOr("window.width", 1280);
    windowHeight_ = config_->getIntOr("window.height", 720);

    // Audio settings
    float masterVol = config_->getFloatOr("audio.master_volume", 1.0f);
    float musicVol = config_->getFloatOr("audio.music_volume", 0.7f);
    audio_->setMasterVolume(masterVol);
    audio_->setMusicVolume(musicVol);

    // Gameplay settings
    difficulty_ = config_->getStringOr("gameplay.difficulty", "normal");
    startingLives_ = config_->getIntOr("gameplay.starting_lives", 3);
}
```

### Example 2: Player Stats Config

```lua
-- data/config/player.lua
local DEBUG = false

return {
    -- Movement (pixels per second)
    walk_speed = 100.0,
    run_speed = 200.0,
    crouch_speed = 50.0,

    -- Jump physics
    jump_force = 500.0,
    double_jump_enabled = true,
    double_jump_force = 400.0,
    wall_jump_force = 450.0,

    -- Combat
    max_health = DEBUG and 9999 or 100,
    starting_health = 100,
    invincibility_frames = 60, -- 1 second at 60fps

    attack_damage = 25,
    attack_range = 32.0,
    attack_cooldown = 0.5,

    -- Special abilities
    dash_enabled = true,
    dash_distance = 150.0,
    dash_duration = 0.2,
    dash_cooldown = 2.0,

    -- Animation
    idle_frames = {1, 2, 3, 4},
    walk_frames = {5, 6, 7, 8, 9, 10},
    jump_frame = 11,
    fall_frame = 12,
    attack_frames = {13, 14, 15},

    frame_duration = 0.1
}
```

Using in a player system:

```cpp
class PlayerSystem {
public:
    void init(IConfigSystem* config) {
        config_ = config;
        loadConfig();

        // Subscribe to changes
        configSubId_ = config_->onKeyChanged("", [this](const auto& key) {
            loadConfig();
        });
    }

    void loadConfig() {
        // Movement
        walkSpeed_ = config_->getFloatOr("walk_speed", 100.0f);
        runSpeed_ = config_->getFloatOr("run_speed", 200.0f);

        // Jump
        jumpForce_ = config_->getFloatOr("jump_force", 500.0f);
        canDoubleJump_ = config_->getBoolOr("double_jump_enabled", true);
        doubleJumpForce_ = config_->getFloatOr("double_jump_force", 400.0f);

        // Combat
        maxHealth_ = config_->getIntOr("max_health", 100);
        attackDamage_ = config_->getIntOr("attack_damage", 25);
        attackCooldown_ = config_->getFloatOr("attack_cooldown", 0.5f);

        // Animation
        idleFrames_ = config_->getIntArray("idle_frames");
        walkFrames_ = config_->getIntArray("walk_frames");
        frameDuration_ = config_->getFloatOr("frame_duration", 0.1f);

        spdlog::info("Player config loaded: speed={}, health={}", walkSpeed_, maxHealth_);
    }

private:
    IConfigSystem* config_;
    SubscriptionId configSubId_;

    float walkSpeed_, runSpeed_;
    float jumpForce_, doubleJumpForce_;
    bool canDoubleJump_;
    int maxHealth_, attackDamage_;
    float attackCooldown_;
    std::vector<int> idleFrames_, walkFrames_;
    float frameDuration_;
};
```

### Example 3: Hot Reload Setup

```cpp
class Game {
public:
    void init() {
        // Enable hot reload in debug mode
        #if defined(BESTOW_DEV_TOOLS)
            config_->enableHotReload(true);
            spdlog::info("Hot reload enabled");
        #endif

        // Load all configs
        loadAllConfigs();

        // Subscribe to config changes
        configSubId_ = config_->onConfigChanged([this](const ConfigKey& key) {
            handleConfigChange(key);
        });
    }

    void shutdown() {
        config_->unsubscribe(configSubId_);
    }

private:
    void loadAllConfigs() {
        config_->loadConfig("data/config/game.lua");
        config_->loadConfig("data/config/player.lua");
        config_->loadConfig("data/config/enemies.lua");
        config_->loadConfig("data/config/physics.lua");

        applyAllConfigs();
    }

    void handleConfigChange(const ConfigKey& key) {
        spdlog::info("Config changed: {}", key);

        // Determine which system needs updating
        if (key.starts_with("player.")) {
            playerSystem_->loadConfig();
        } else if (key.starts_with("enemy.")) {
            enemySystem_->loadConfig();
        } else if (key.starts_with("physics.")) {
            physics_->loadConfig();
        } else {
            // Reload everything for safety
            applyAllConfigs();
        }
    }

    void applyAllConfigs() {
        playerSystem_->loadConfig();
        enemySystem_->loadConfig();
        physics_->loadConfig();
        audio_->loadConfig();
    }

    IConfigSystem* config_;
    SubscriptionId configSubId_;

    std::unique_ptr<PlayerSystem> playerSystem_;
    std::unique_ptr<EnemySystem> enemySystem_;
    // ... other systems
};
```

### Example 4: Generated Config Data

```lua
-- data/config/waves.lua
local waves = {}

-- Generate 20 waves of increasing difficulty
for wave = 1, 20 do
    local difficulty = 1.0 + (wave * 0.15)

    waves[wave] = {
        -- More enemies each wave
        enemy_count = 5 + (wave * 2),

        -- Enemies get tougher
        health_multiplier = difficulty,
        speed_multiplier = 1.0 + (wave * 0.05),
        damage_multiplier = difficulty,

        -- Faster spawning
        spawn_interval = math.max(0.5, 2.0 - (wave * 0.1)),

        -- Boss every 5 waves
        has_boss = (wave % 5) == 0,
        boss_health = 500 * difficulty,

        -- Rewards scale
        gold_reward = 100 * wave,
        xp_reward = 50 * wave
    }
end

return {
    waves = waves,
    starting_wave = 1,
    max_waves = 20
}
```

Using in a wave manager:

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
    void loadConfig() {
        int waveCount = config_->getIntOr("max_waves", 10);

        for (int i = 1; i <= waveCount; i++) {
            std::string prefix = std::format("waves.{}", i);

            WaveConfig wave;
            wave.enemyCount = config_->getIntOr(prefix + ".enemy_count", 5);
            wave.healthMultiplier = config_->getFloatOr(prefix + ".health_multiplier", 1.0f);
            wave.spawnInterval = config_->getFloatOr(prefix + ".spawn_interval", 2.0f);
            wave.hasBoss = config_->getBoolOr(prefix + ".has_boss", false);
            wave.goldReward = config_->getIntOr(prefix + ".gold_reward", 100);

            waves_.push_back(wave);
        }

        spdlog::info("Loaded {} wave configurations", waves_.size());
    }

private:
    std::vector<WaveConfig> waves_;
};
```

## Advanced Topics

### Accessing Nested Tables

For deeply nested config:

```lua
return {
    characters = {
        player = {
            stats = {
                base = {
                    strength = 10,
                    agility = 15
                }
            }
        }
    }
}
```

Access with full path:
```cpp
int strength = config->getIntOr("characters.player.stats.base.strength", 10);
```

### Querying All Keys

Find all keys in a namespace:

```cpp
// Get all player-related keys
auto playerKeys = config->getKeysWithPrefix("player.");

// Results:
// "player.speed"
// "player.max_health"
// "player.jump_force"
// etc.

for (const auto& key : playerKeys) {
    spdlog::info("Player config: {}", key);
}
```

### Using Direct Lua State (Advanced)

For advanced use cases, get direct access to the sandboxed Lua state:

```cpp
sol::state* lua = config->getLuaState();

// Execute custom Lua code
lua->script(R"(
    function double(x)
        return x * 2
    end
)");

// Call Lua functions
sol::function doubleFunc = (*lua)["double"];
int result = doubleFunc(5); // result = 10
```

Warning: The Lua state is shared - be careful not to interfere with config loading.

### Config Events via EventSystem

Config changes are also published to the EventSystem:

```cpp
// Subscribe via EventSystem
eventSubId_ = events->subscribe(Events::ConfigChanged,
    [this](const EventData& data) {
        auto& event = std::get<ConfigEventData>(data);
        spdlog::info("Config changed: key={}, section={}",
                    event.key, event.section);
    });
```

This is useful for decoupled systems that don't want a direct reference to ConfigSystem.

## Summary

The Config System provides a powerful, Lua-based configuration solution with:

- Type-safe value access with optional defaults
- Hot reload for rapid iteration
- Nested tables and arrays
- Lua's full expressiveness (math, loops, conditionals)
- Sandboxed execution for security
- Change notifications for reactive systems

**Quick Checklist:**

- Load configs at startup: `config->loadConfig("data/config/game.lua")`
- Enable hot reload in debug: `config->enableHotReload(true)`
- Use `getXxxOr()` with defaults for safe access
- Subscribe to changes: `config->onKeyChanged("prefix.", callback)`
- Cache frequently-accessed values
- Use Lua features: comments, math, variables, loops
- Organize configs by system/feature

For more examples, see the unit tests in `/Users/jaaaacob/Documents/GameDev/jframe/tests/unit/ConfigSystemTests.cpp`.
