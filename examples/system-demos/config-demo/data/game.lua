-- game.lua
-- Main game configuration file demonstrating JFrame Config System features

-- This file demonstrates:
-- - Simple primitive values (numbers, strings, bools)
-- - Nested tables accessed with dot notation
-- - Arrays/lists
-- - Computed values using Lua expressions
-- - Comments and trailing commas

return {
    -- ========================================================================
    -- GAME METADATA
    -- ========================================================================

    -- Simple string values
    title = "JFrame Config Demo",
    version = "1.0.0",
    author = "JFrame Team",

    -- ========================================================================
    -- WINDOW SETTINGS
    -- ========================================================================

    -- Nested table - accessed as "window.width", "window.height", etc.
    window = {
        width = 1280,
        height = 720,
        fullscreen = false,
        vsync = true,
        title = "Config System Demo",

        -- Computed value using Lua math
        aspectRatio = 1280 / 720,  -- Will be stored as float
    },

    -- ========================================================================
    -- GRAPHICS SETTINGS
    -- ========================================================================

    graphics = {
        targetFPS = 60,
        showFPS = true,
        antialiasing = true,

        -- Integer for quality level
        quality = 2,  -- 0=low, 1=medium, 2=high, 3=ultra

        -- Float values for rendering
        brightness = 1.0,
        contrast = 1.2,
        saturation = 0.95,

        -- Background color components (0-255)
        backgroundColor = {
            r = 32,
            g = 32,
            b = 48,
        },
    },

    -- ========================================================================
    -- AUDIO SETTINGS
    -- ========================================================================

    audio = {
        masterVolume = 0.8,
        musicVolume = 0.6,
        sfxVolume = 0.9,

        muted = false,

        -- Max simultaneous sounds
        maxChannels = 32,
    },

    -- ========================================================================
    -- DEBUG SETTINGS
    -- ========================================================================

    debug = {
        enabled = true,
        showColliders = false,
        showFPS = true,
        logLevel = "info",  -- "trace", "debug", "info", "warn", "error"

        -- Hot reload check interval in seconds
        hotReloadInterval = 0.5,
    },

    -- ========================================================================
    -- ARRAYS / LISTS
    -- ========================================================================

    -- Integer array - for things like level IDs, frame numbers, etc.
    levelIds = {1, 2, 3, 4, 5, 10, 15, 20},

    -- Float array - for things like spawn times, difficulty curve, etc.
    spawnTimes = {0.5, 1.0, 1.5, 2.5, 5.0, 10.0},

    -- String array - for level names, asset paths, etc.
    levelNames = {
        "Tutorial",
        "Forest",
        "Cave",
        "Mountain",
        "Boss Arena",
    },

    -- ========================================================================
    -- GAMEPLAY CONSTANTS
    -- ========================================================================

    gameplay = {
        -- Player lives
        startingLives = 3,
        maxLives = 5,

        -- Scoring
        scoreMultiplier = 1.5,
        comboTimeout = 2.0,

        -- Difficulty
        difficultyScale = 1.0,
        enemySpawnRate = 3.0,
    },

    -- ========================================================================
    -- COMPUTED VALUES USING LUA
    -- ========================================================================

    -- You can use Lua expressions for computed values
    computed = {
        -- Using math library
        pi = math.pi,
        twoPi = 2 * math.pi,
        halfPi = math.pi / 2,

        -- Using string operations
        fullTitle = "JFrame Config Demo v1.0.0",

        -- Conditional values (Lua ternary-like)
        startInDebug = true and 1 or 0,  -- 1 if true, 0 if false
    },
}
