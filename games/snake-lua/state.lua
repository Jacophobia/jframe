-- state.lua - Global game state matching C++ member variables

local config = require("config")
local types = require("types")

local state = {}

-- Graphics assets
state.cubeMesh = nil
state.groundMesh = nil
state.groundMaterial = nil
state.gameFont = nil
state.titleFont = nil

-- Game phase
state.currentPhase = GamePhase.MainMenu
state.previousPhase = nil

-- Level state
state.currentLevel = types.LevelMap()
state.currentLevelIndex = 0
state.currentWorldIndex = 0
state.foodCollected = 0
state.foodRequired = 5
state.levelMaxSize = 15
state.levelOffsetX = 0
state.levelOffsetZ = 0

-- World/progress state
state.currentWorld = types.WorldConfig()
state.saveData = types.GameSaveData()
state.segmentsEarnedThisLevel = 0
state.totalSegmentsInWorld = 0
state.selectedNodeIndex = 0
state.worldMapCursorBob = 0.0
state.worldMapCameraTarget = Vec3.new(0, 0, 0)
state.worldMapCameraDistance = config.WORLD_MAP_CAMERA_DISTANCE

-- Snake state
state.snake = {}  -- Array of SnakeSegment
state.obstacles = {}  -- Array of GridPos (visible walls)
state.allWalls = {}  -- All walls from level (before visibility culling)
state.foodPos = types.GridPos(5, 5)
state.foodColor = {1.0, 0.4, 0.4, 1.0}
state.direction = Direction.Right
state.nextDirection = Direction.Right
state.inputDirection = Direction.Right
state.hasBufferedInput = false
state.moveTimer = 0.0
state.moveInterval = config.INITIAL_MOVE_INTERVAL
state.score = 0
state.gridSize = config.INITIAL_GRID_SIZE
state.gameOver = false
state.running = true

-- Enemies
state.enemies = {}  -- Array of Enemy
state.detachedSegments = {}  -- Array of DetachedSegment
state.foodPickups = {}  -- Array of FoodPickup

-- Animation/interpolation
state.visualGridSize = config.INITIAL_GRID_SIZE
state.currentCameraDistance = config.BASE_CAMERA_DISTANCE
state.currentCameraHeight = config.BASE_CAMERA_DISTANCE * 0.8
state.gameTime = 0.0
state.isExpanding = false
state.expansionTimer = 0.0
state.targetGridSize = config.INITIAL_GRID_SIZE

-- Camera
state.cameraTarget = Vec3.new(0, 0, 0)
state.cameraAngle = config.CAMERA_ANGLE

-- Screen effects
state.screenShakeIntensity = 0.0
state.screenShakeTimer = 0.0
state.screenShakeDuration = 0.0
state.screenShakeOffset = Vec3.new(0, 0, 0)
state.foodPopScale = 0.0
state.foodPopTimer = 0.0
state.lastFoodPos = Vec3.new(0, 0, 0)

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

-- Reset game state for restart
function state.reset()
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
