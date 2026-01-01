-- config.lua - Game constants and configuration
-- Matches the C++ constexpr values exactly
-- No dependencies - pure data module

local config = {}

-- Grid and cell sizes
config.CELL_SIZE = 1.0
config.INITIAL_GRID_SIZE = 10

-- Movement timing
config.INITIAL_MOVE_INTERVAL = 0.15  -- Time between snake moves
config.MIN_MOVE_INTERVAL = 0.05      -- Fastest snake speed
config.SPEED_INCREASE_PER_FOOD = 0.005

-- Food spawning
config.MIN_FOOD_DISTANCE = 5  -- Minimum distance from snake head to spawn food
config.FOOD_EDGE_MARGIN = 2   -- Don't spawn food within this many cells of edge

-- Camera settings
config.BASE_CAMERA_DISTANCE = 12.0
config.BASE_CAMERA_HEIGHT = 10.0
config.CAMERA_SCALE = 0.3  -- How much camera zooms out per grid size increase
config.CAMERA_ANGLE = 45.0  -- Isometric angle in degrees

-- Animation
config.EXPANSION_WAIT_TIME = 1.0  -- Time to wait before finalizing grid expansion
config.SCREEN_SHAKE_DECAY = 5.0   -- How fast screen shake decays
config.FOOD_POP_DURATION = 0.3    -- Duration of food eat popup
config.FOOD_SPIN_SPEED = 2.0      -- How fast food spins
config.ANIMATION_SMOOTH = 3.0     -- Smoothing factor for animations

-- Enemies
config.ENEMY_MOVE_INTERVAL = 0.5   -- Time between enemy moves
config.ENEMY_CONTINUE_CHANCE = 0.85  -- 85% chance to continue same direction
config.ENEMY_HEALTH_BAR_DURATION = 3.0  -- How long health bar shows after damage
config.ENEMY_DAMAGE_FLASH_DURATION = 0.2
config.ENEMY_LERP_SPEED = 8.0  -- How fast enemies glide to new position

-- Detached segments
config.DETACHED_SEGMENT_LIFETIME = 3.0
config.DETACH_ANIMATION_TIME = 0.5  -- Segment explosion duration
config.SEGMENT_SHATTER_CHANCE = 1.0 / 6.0  -- 1 in 6 segments shatter
config.PARTICLE_COUNT_PER_SHATTER = 8
config.PARTICLE_GRAVITY = -15.0
config.PARTICLE_BOUNCE_DAMPING = 0.6
config.PARTICLE_MAX_BOUNCES = 3
config.PARTICLE_ANGULAR_SPEED = 10.0

-- Food pickups
config.FOOD_PICKUP_FADE_TIME = 0.5

-- Ring attack
config.RING_MIN_SIZE = 2  -- Minimum segments between ring start and end
config.RING_DAMAGE = 1    -- Damage dealt to enemies in ring

-- HUD
config.HUD_FOOD_BAR_WIDTH = 3.0
config.HUD_FOOD_BAR_HEIGHT = 0.3

-- Menu constants
config.MAIN_MENU_PLAY = 0
config.MAIN_MENU_QUIT = 1
config.MAIN_MENU_COUNT = 2

config.PAUSE_MENU_RESUME = 0
config.PAUSE_MENU_RESTART = 1
config.PAUSE_MENU_QUIT = 2
config.PAUSE_MENU_COUNT = 3

config.GAME_OVER_RETRY = 0
config.GAME_OVER_QUIT = 1
config.GAME_OVER_COUNT = 2

-- Colors (as RGBA 0-255)
config.SNAKE_HEAD_COLOR = {50, 200, 80, 255}
config.SNAKE_BODY_COLOR = {40, 180, 60, 255}
config.FOOD_COLOR = {255, 100, 100, 255}
config.OBSTACLE_COLOR = {100, 80, 60, 255}
config.GROUND_COLOR = {60, 100, 60, 255}
config.BORDER_COLOR = {80, 60, 40, 255}

-- World map
config.WORLD_MAP_CAMERA_DISTANCE = 15.0
config.WORLD_MAP_NODE_SPACING = 2.0

return config
