-- state.lua - Global game state matching C++ member variables
-- Dependencies: app.config, app.types (accessed inside functions)

local state = {}

-- Graphics assets
state.cubeMesh = nil
state.groundMesh = nil
state.groundMaterial = nil
state.gameFont = nil
state.titleFont = nil

-- Game phase
state.currentPhase = nil  -- Will be set to GamePhase.MainMenu in init
state.previousPhase = nil

-- Level state (initialized with defaults, proper init in reset/init functions)
state.currentLevel = nil  -- Will be initialized with types.LevelMap()
state.currentLevelIndex = 0
state.currentWorldIndex = 0
state.foodCollected = 0
state.foodRequired = 5
state.levelMaxSize = 15
state.levelOffsetX = 0
state.levelOffsetZ = 0

-- World/progress state
state.currentWorld = nil  -- Will be initialized with types.WorldConfig()
state.saveData = nil  -- Will be initialized with types.GameSaveData()
state.segmentsEarnedThisLevel = 0
state.totalSegmentsInWorld = 0
state.selectedNodeIndex = 0
state.worldMapCursorBob = 0.0
state.worldMapCameraTarget = nil  -- Will be Vec3
state.worldMapCameraDistance = 15.0  -- Default from config

-- Snake state
state.snake = {}  -- Array of SnakeSegment
state.obstacles = {}  -- Array of GridPos (visible walls)
state.allWalls = {}  -- All walls from level (before visibility culling)
state.foodPos = nil  -- Will be initialized
state.foodColor = {1.0, 0.4, 0.4, 1.0}
state.direction = nil  -- Will be Direction.Right
state.nextDirection = nil
state.inputDirection = nil
state.hasBufferedInput = false
state.moveTimer = 0.0
state.moveInterval = 0.15  -- Default from config
state.score = 0
state.gridSize = 10  -- Default from config
state.gameOver = false
state.running = true

-- Enemies
state.enemies = {}  -- Array of Enemy
state.detachedSegments = {}  -- Array of DetachedSegment
state.foodPickups = {}  -- Array of FoodPickup

-- Animation/interpolation
state.visualGridSize = 10  -- Default from config
state.currentCameraDistance = 12.0  -- Default from config
state.currentCameraHeight = 9.6  -- DEFAULT_CAMERA_DISTANCE * 0.8
state.gameTime = 0.0
state.isExpanding = false
state.expansionTimer = 0.0
state.targetGridSize = 10

-- Camera
state.cameraTarget = nil  -- Will be Vec3
state.cameraAngle = 45.0

-- Screen effects
state.screenShakeIntensity = 0.0
state.screenShakeTimer = 0.0
state.screenShakeDuration = 0.0
state.screenShakeOffset = nil  -- Will be Vec3
state.foodPopScale = 0.0
state.foodPopTimer = 0.0
state.lastFoodPos = nil  -- Will be Vec3

-- Menu state
state.mainMenuSelection = 0
state.pauseMenuSelection = 0
state.gameOverMenuSelection = 0
state.menuAnimTime = 0.0

-- Audio handles
state.soundEat = nil
state.soundDeath = nil
state.soundLevelComplete = nil
state.soundMenuSelect = nil
state.soundMenuMove = nil
state.soundEnemyHit = nil
state.soundChainBreak = nil
state.soundPause = nil
state.soundGameOver = nil
state.musicGame = nil
state.musicMenu = nil
state.soundsLoaded = false

-- Initialize state with proper types (called once on game start)
function state.initialize()
    local config = app.config
    local types = app.types

    -- Initialize types that need constructors
    state.currentLevel = types.LevelMap()
    state.currentWorld = types.WorldConfig()
    state.saveData = types.GameSaveData()
    state.foodPos = types.GridPos(5, 5)
    state.worldMapCameraTarget = Vec3.new(0, 0, 0)
    state.cameraTarget = Vec3.new(0, 0, 0)
    state.screenShakeOffset = Vec3.new(0, 0, 0)
    state.lastFoodPos = Vec3.new(0, 0, 0)

    -- Initialize from config
    state.gridSize = config.INITIAL_GRID_SIZE
    state.visualGridSize = config.INITIAL_GRID_SIZE
    state.targetGridSize = config.INITIAL_GRID_SIZE
    state.moveInterval = config.INITIAL_MOVE_INTERVAL
    state.currentCameraDistance = config.BASE_CAMERA_DISTANCE
    state.currentCameraHeight = config.BASE_CAMERA_DISTANCE * 0.8
    state.cameraAngle = config.CAMERA_ANGLE
    state.worldMapCameraDistance = config.WORLD_MAP_CAMERA_DISTANCE

    -- Initialize directions
    state.direction = Direction.Right
    state.nextDirection = Direction.Right
    state.inputDirection = Direction.Right

    -- Initialize phase
    state.currentPhase = GamePhase.MainMenu
end

-- Reset game state for restart
function state.reset()
    local config = app.config

    state.snake = {}
    state.detachedSegments = {}
    state.foodPickups = {}
    state.direction = Direction.Right
    state.nextDirection = Direction.Right
    state.inputDirection = Direction.Right
    state.hasBufferedInput = false
    state.moveTimer = 0.0
    state.moveInterval = config.INITIAL_MOVE_INTERVAL
    state.score = 0
    state.gridSize = config.INITIAL_GRID_SIZE
    state.visualGridSize = config.INITIAL_GRID_SIZE
    state.gameOver = false
    state.isExpanding = false
    state.expansionTimer = 0.0
    state.foodCollected = 0
    state.segmentsEarnedThisLevel = 0

    state.screenShakeIntensity = 0.0
    state.screenShakeTimer = 0.0
    state.foodPopScale = 0.0
    state.foodPopTimer = 0.0

    -- Reset camera
    state.currentCameraDistance = config.BASE_CAMERA_DISTANCE
    state.currentCameraHeight = config.BASE_CAMERA_DISTANCE * 0.8
    state.cameraTarget = Vec3.new(0, 0, 0)

    -- Reset enemies
    state.enemies = {}
end

-- Initialize snake at starting position
function state.initSnake(startPos)
    local config = app.config
    local types = app.types

    state.snake = {}
    -- Head is at front (index 1 in Lua)
    local headColor = {
        config.SNAKE_HEAD_COLOR[1] / 255,
        config.SNAKE_HEAD_COLOR[2] / 255,
        config.SNAKE_HEAD_COLOR[3] / 255,
        1.0
    }
    table.insert(state.snake, types.SnakeSegment(startPos, headColor))

    -- Add a couple initial segments behind
    for i = 1, 2 do
        local segPos = types.GridPos(startPos.x - i, startPos.z)
        local segColor = {
            config.SNAKE_BODY_COLOR[1] / 255,
            config.SNAKE_BODY_COLOR[2] / 255,
            config.SNAKE_BODY_COLOR[3] / 255,
            1.0
        }
        table.insert(state.snake, types.SnakeSegment(segPos, segColor))
    end
end

return state
