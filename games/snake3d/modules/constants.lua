-- games/snake3d/modules/constants.lua
-- All game constants matching the C++ snake game exactly
--
-- These values are extracted from games/game1/src/snake.game.cppm
-- DO NOT CHANGE without verifying against C++ source

local Constants = {}

-- Grid and Cell
Constants.INITIAL_GRID_SIZE = 10        -- Starting grid size (10x10)
Constants.CELL_SIZE = 1.0               -- World units per grid cell

-- Movement Timing
Constants.INITIAL_MOVE_INTERVAL = 0.12  -- Seconds between snake moves
Constants.MIN_MOVE_INTERVAL = 0.04      -- Maximum speed cap
Constants.SPEED_INCREASE = 0.001        -- Speed increase per food eaten

-- Camera
Constants.BASE_CAMERA_DISTANCE = 12.0   -- Base distance from target
Constants.BASE_CAMERA_HEIGHT = 10.0     -- Base height above target
Constants.CAMERA_SCALE = 0.3            -- Distance increase per grid expansion
Constants.CAMERA_FOV = 45.0             -- Field of view in degrees
Constants.CAMERA_ANGLE = 45.0           -- Isometric angle in degrees
Constants.CAMERA_LERP_SPEED = 5.0       -- Camera follow smoothness

-- Animation
Constants.ANIMATION_SMOOTH = 3.0        -- Animation interpolation factor
Constants.EXPANSION_WAIT_TIME = 1.0     -- Grid expansion animation duration
Constants.FOOD_SPIN_SPEED = 2.0         -- Food rotation speed (rad/s)
Constants.FOOD_BOUNCE_SPEED = 4.0       -- Food bounce frequency (Hz)
Constants.FOOD_PULSE_SPEED = 6.0        -- Food pulse frequency (Hz)
Constants.FOOD_POP_DURATION = 0.3       -- Food pop effect duration

-- Enemies
Constants.ENEMY_MOVE_INTERVAL = 0.5     -- Base enemy movement interval
Constants.ENEMY_LERP_SPEED = 8.0        -- Enemy visual interpolation speed
Constants.HEALTH_BAR_DURATION = 3.0     -- How long health bar shows after damage
Constants.DAMAGE_FLASH_DURATION = 0.3   -- Enemy damage flash duration
Constants.DAMAGE_FLASH_FREQUENCY = 30.0 -- Flash rate (Hz)

-- Detached Segments / Particles
Constants.DETACH_ANIMATION_TIME = 0.5   -- Chain break animation duration
Constants.SHATTER_CHANCE = 1/6          -- Chance for particle to shatter
Constants.PARTICLE_GRAVITY = -15.0      -- Gravity for explosion particles
Constants.PARTICLE_LIFE_MIN = 2.0       -- Minimum particle lifetime
Constants.PARTICLE_LIFE_MAX = 4.0       -- Maximum particle lifetime

-- Food Spawning
Constants.MIN_FOOD_DISTANCE = 5         -- Minimum distance between food spawns

-- Screen Shake
Constants.SHAKE_INTENSITY_DEATH = 0.5   -- Death screen shake intensity
Constants.SHAKE_DURATION_DEATH = 0.4    -- Death screen shake duration
Constants.SHAKE_INTENSITY_EAT = 0.1     -- Eating screen shake intensity
Constants.SHAKE_DURATION_EAT = 0.1      -- Eating screen shake duration

-- Colors (RGBA 0-1)
Constants.COLORS = {
    -- Snake
    SNAKE_HEAD = {0.2, 0.9, 0.3, 1.0},          -- Bright green
    SNAKE_BODY = {0.25, 0.85, 0.35, 1.0},       -- Slightly darker green
    SNAKE_TAIL = {0.3, 0.8, 0.4, 1.0},          -- Even darker green

    -- Ground
    GROUND = {0.15, 0.4, 0.2, 1.0},             -- Dark forest green

    -- Enemies
    ENEMY = {0.8, 0.2, 0.3, 1.0},               -- Red-purple
    ENEMY_FLASH = {1.0, 1.0, 1.0, 1.0},         -- White (damage flash)

    -- Obstacles
    OBSTACLE = {0.3, 0.25, 0.2, 1.0},           -- Brown

    -- Grid Border
    BORDER = {0.31, 0.24, 0.16, 1.0},           -- Brown (80, 60, 40)
    BORDER_EXPANSION = {1.0, 1.0, 0.6, 1.0},    -- Yellow-white (glow)

    -- UI
    FOOD_BAR_BG = {0.2, 0.2, 0.2, 1.0},         -- Dark gray
    FOOD_BAR_FG = {1.0, 0.8, 0.2, 1.0},         -- Gold
    SEGMENT_BAR = {0.2, 0.8, 0.3, 1.0},         -- Green
    SEGMENT_BAR_HEAD = {1.0, 0.8, 0.2, 1.0},    -- Gold (head marker)

    -- Effects
    FOOD_POP = {1.0, 0.78, 0.2, 1.0},           -- Gold
    EMBER_GLOW = {0.8, 0.2, 0.1, 1.0},          -- Orange-red
}

-- Clear color (background)
Constants.CLEAR_COLOR = {30/255, 120/255, 50/255}  -- Forest green

-- Ambient lighting
Constants.AMBIENT_COLOR = {0.3, 0.35, 0.3}
Constants.AMBIENT_INTENSITY = 0.5

-- Directional light
Constants.LIGHT_DIRECTION = {0.5, -1.0, 0.3}
Constants.LIGHT_COLOR = {1.0, 0.98, 0.95}
Constants.LIGHT_INTENSITY = 1.2

-- PBR Material defaults
Constants.PBR = {
    GROUND = {roughness = 0.9, metallic = 0.0},
    SNAKE = {roughness = 0.4, metallic = 0.1},
    ENEMY = {roughness = 0.5, metallic = 0.2},
    OBSTACLE = {roughness = 0.8, metallic = 0.1},
    FOOD = {roughness = 0.2, metallic = 0.3, emissive = 0.3},
    FOOD_PICKUP = {roughness = 0.2, metallic = 0.4, emissive = 0.4},
}

-- Menu
Constants.MAIN_MENU_COUNT = 2
Constants.MAIN_MENU_PLAY = 0
Constants.MAIN_MENU_QUIT = 1

Constants.PAUSE_MENU_COUNT = 3
Constants.PAUSE_MENU_RESUME = 0
Constants.PAUSE_MENU_RESTART = 1
Constants.PAUSE_MENU_QUIT = 2

return Constants
