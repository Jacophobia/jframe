-- games/snake3d/main.lua
-- Snake 3D - Lua Edition
-- A complete port of the C++ snake game demonstrating the Lua-first API
--
-- Run with: bestow --library path/to/asset-library games/snake3d/main.lua

--============================================================================
-- Load Modules (hot-reloadable via bestow.include)
--============================================================================

-- Core modules
local Log = bestow.include("modules/log")
local Constants = bestow.include("modules/constants")
local GamePhase = bestow.include("modules/phases")
local Direction = bestow.include("modules/direction")
local Grid = bestow.include("modules/grid")

-- System modules
local Camera = bestow.include("modules/camera")
local Audio = bestow.include("modules/audio")
local Input = bestow.include("modules/input")
local Effects = bestow.include("modules/effects")

-- Game logic modules
local Snake = bestow.include("modules/snake")
local Enemies = bestow.include("modules/enemies")
local Rendering = bestow.include("modules/rendering")

-- Create main logger
local log = Log.category("Game")

--============================================================================
-- Game Object
--============================================================================

game = {
    title = "Snake 3D - Lua Edition",
    width = 1280,
    height = 720,

    -- State
    running = true,
    gameOver = false,
    currentPhase = GamePhase.MainMenu,

    -- Grid
    gridSize = Constants.INITIAL_GRID_SIZE,
    pendingGridSize = nil,
    isExpanding = false,
    expansionTimer = 0,

    -- Snake (initialized by Snake.init)
    snake = {},
    direction = Direction.Right,
    nextDirection = Direction.Right,
    inputDirection = Direction.Right,
    hasBufferedInput = false,

    -- Movement
    moveTimer = 0,
    moveInterval = Constants.INITIAL_MOVE_INTERVAL,

    -- Food
    foodPos = {x = 5, z = 5},
    foodColor = {1.0, 0.85, 0.2, 1.0},
    foodCollected = 0,
    foodRequired = 5,
    foodPickups = {},

    -- Score
    score = 0,

    -- Camera (initialized by Camera.init)
    cameraTarget = {x = 0, y = 0, z = 0},
    cameraAngle = 45,
    currentCameraDistance = Constants.BASE_CAMERA_DISTANCE,
    currentCameraHeight = Constants.BASE_CAMERA_HEIGHT,
    screenShakeOffset = {x = 0, y = 0, z = 0},
    screenShakeTime = 0,
    screenShakeIntensity = 0,

    -- Level
    currentLevel = {
        width = 30,
        height = 30,
        walls = {},
        foodSpawnPoints = {},
        enemyZones = {}
    },

    -- Enemies and obstacles
    enemies = {},
    obstacles = {},

    -- Detached segments (death animation)
    detachedSegments = {},

    -- Effects (initialized by Effects.init)
    foodPopEffect = {active = false, pos = {x = 0, y = 0, z = 0}, time = 0, maxTime = 0.3},

    -- Menu state
    mainMenuSelection = 0,
    pauseMenuSelection = 0,

    -- Time
    gameTime = 0,

    -- Clear color (must be 0-255 integers for engine, not 0-1 floats)
    clearColor = {30, 120, 50}
}

--============================================================================
-- Helper Methods
--============================================================================

function game:gridToWorld(pos)
    return Rendering.gridToWorld(self, pos)
end

function game:transitionTo(phase)
    local oldPhase = self.currentPhase
    log.info("Phase transition: %s -> %s", GamePhase.getName(oldPhase), GamePhase.getName(phase))

    self.currentPhase = phase

    -- Handle audio transitions (matches C++ snake.game.cppm)
    if phase == GamePhase.MainMenu then
        Audio.playMusic("menu")
    elseif phase == GamePhase.Playing then
        if oldPhase == GamePhase.MainMenu or oldPhase == GamePhase.WorldMap then
            Audio.playMusic("game")
        end
        -- Note: No sound when resuming from pause (matches C++)
        self.gameOver = false
    elseif phase == GamePhase.Paused then
        Audio.playSFX("pause")
    elseif phase == GamePhase.LevelComplete then
        Audio.playSFX("level_complete")
    elseif phase == GamePhase.GameOver then
        self.gameOver = true
        Audio.playSFX("game_over")
        Audio.playMusic("menu")
    end
end

function game:triggerFoodPop(pos)
    Effects.triggerFoodPop(self, pos)
end

function game:triggerScreenShake(intensity, duration)
    Camera.triggerShake(self, intensity, duration)
end

function game:startExpansion()
    if self.gridSize < self.currentLevel.width then
        self.isExpanding = true
        self.expansionTimer = 0
        self.pendingGridSize = math.min(self.gridSize + 2, self.currentLevel.width)
        log.debug("Starting expansion: %d -> %d", self.gridSize, self.pendingGridSize)
    end
end

function game:finalizeExpansion()
    if self.pendingGridSize then
        local oldSize = self.gridSize
        self.gridSize = self.pendingGridSize
        self.pendingGridSize = nil

        -- Update camera zoom
        self.currentCameraDistance = Grid.getCameraDistance(self.gridSize)
        self.currentCameraHeight = Grid.getCameraHeight(self.gridSize)

        -- Rebuild ground mesh for new size
        Rendering.rebuildGroundMesh(self.gridSize)

        log.info("Expansion complete: %d -> %d", oldSize, self.gridSize)
    end
end

function game:restart()
    log.info("Restarting game...")

    self.gameOver = false
    self.score = 0
    self.foodCollected = 0
    self.gridSize = Constants.INITIAL_GRID_SIZE
    self.moveInterval = Constants.INITIAL_MOVE_INTERVAL
    self.direction = Direction.Right
    self.nextDirection = Direction.Right
    self.detachedSegments = {}
    self.isExpanding = false
    self.pendingGridSize = nil

    self:applyLevelToGame()
    Audio.playMusic("game")
    self:transitionTo(GamePhase.Playing)
end

function game:startGame()
    log.info("Starting game...")
    Audio.playSFX("menu_select")
    self:applyLevelToGame()
    self:transitionTo(GamePhase.Playing)
end

function game:completeLevel()
    log.info("Level complete! Food collected: %d/%d", self.foodCollected, self.foodRequired)
    Audio.playSFX("level_complete")
    self:transitionTo(GamePhase.LevelComplete)
end

function game:proceedToNextLevel()
    log.info("Proceeding to next level...")
    self.foodRequired = self.foodRequired + 3
    self.foodCollected = 0
    self:transitionTo(GamePhase.Playing)
end

function game:returnToWorldMap()
    log.info("Returning to world map")
    self:transitionTo(GamePhase.WorldMap)
end

--============================================================================
-- Level Setup
--============================================================================

local function generateDefaultLevel()
    log.debug("Generating default level...")

    local level = game.currentLevel

    -- Generate scattered walls in outer areas
    level.walls = {}
    for i = 1, 20 do
        local angle = (i / 20) * math.pi * 2
        local radius = 10 + math.random(5)
        local x = math.floor(level.width / 2 + math.cos(angle) * radius)
        local z = math.floor(level.height / 2 + math.sin(angle) * radius)
        if x >= 0 and x < level.width and z >= 0 and z < level.height then
            table.insert(level.walls, {x = x, z = z})
        end
    end

    -- Enemy zones
    level.enemyZones = {
        {
            bounds = {min = {x = 15, z = 5}, max = {x = 20, z = 10}},
            count = 1,
            moveInterval = Constants.ENEMY_MOVE_INTERVAL,
            health = 1
        },
        {
            bounds = {min = {x = 5, z = 15}, max = {x = 10, z = 20}},
            count = 1,
            moveInterval = Constants.ENEMY_MOVE_INTERVAL,
            health = 1
        }
    }

    log.debug("Generated %d walls and %d enemy zones", #level.walls, #level.enemyZones)
end

function game:applyLevelToGame()
    log.debug("Applying level to game...")

    -- Initialize snake
    Snake.init(self)

    -- Apply walls as obstacles within current grid
    self.obstacles = {}
    for _, wall in ipairs(self.currentLevel.walls) do
        if wall.x >= 0 and wall.x < self.gridSize and
           wall.z >= 0 and wall.z < self.gridSize then
            table.insert(self.obstacles, {x = wall.x, z = wall.z})
        end
    end

    -- Spawn enemies
    Enemies.init(self)
    Enemies.spawnFromZones(self)

    -- Spawn first food
    Snake.spawnFood(self)

    -- Rebuild ground mesh
    Rendering.rebuildGroundMesh(self.gridSize)

    log.info("Level applied: %d obstacles, %d enemies, snake at (%d, %d)",
        #self.obstacles, #self.enemies,
        self.snake[1] and self.snake[1].pos.x or 0,
        self.snake[1] and self.snake[1].pos.z or 0)
end

--============================================================================
-- Animation Updates
--============================================================================

function game:updateAnimations(dt)
    -- Expansion animation
    if self.isExpanding then
        self.expansionTimer = self.expansionTimer + dt
        if self.expansionTimer >= Constants.EXPANSION_WAIT_TIME then
            self.isExpanding = false
            self:finalizeExpansion()
        end
    end

    -- Food pop effect
    Effects.update(self, dt)
end

--============================================================================
-- Game Lifecycle
--============================================================================

function game:init()
    log.info("========================================")
    log.info("Snake 3D - Lua Edition")
    log.info("========================================")
    log.info("Controls: ,AOE (Dvorak) or Arrow Keys")
    log.info("Press ENTER to start, ESC to pause")

    -- Seed random
    local seed = 12345
    if bestow.time and bestow.time.getTime then
        seed = math.floor(bestow.time.getTime() * 1000)
    end
    math.randomseed(seed)
    log.trace("Random seed: %d", seed)

    -- Set clear color (only if function exists - working example doesn't call this)
    if bestow.graphics.setClearColor then
        bestow.graphics.setClearColor(self.clearColor[1], self.clearColor[2], self.clearColor[3])
        log.debug("Set clear color: (%d, %d, %d)", self.clearColor[1], self.clearColor[2], self.clearColor[3])
    else
        log.debug("setClearColor not available")
    end

    -- Initialize rendering (creates meshes, materials, sets up lighting)
    Rendering.init()

    -- Build ground mesh for MainMenu (before any drawing happens)
    Rendering.rebuildGroundMesh(self.gridSize)

    -- Initialize camera
    Camera.init(self)
    Camera.setupInitial(self)

    -- Initialize audio system
    Audio.init()

    -- Initialize effects
    Effects.init(self)

    -- Generate level data
    generateDefaultLevel()

    -- Start menu music
    Audio.playMusic("menu")

    log.info("Game initialized! Press ENTER to play.")
end

-- Handle input ONCE per frame (called by runtime BEFORE fixed timestep loop)
-- This matches C++ pattern and prevents missed inputs at high framerates
function game:handleInput()
    Input.handleInput(self)
end

function game:update(dt)
    -- Cap frame time to avoid spiral of death (matches C++ MAX_FRAME_TIME = 0.25)
    local cappedDt = math.min(dt, 0.25)

    self.gameTime = self.gameTime + cappedDt

    -- Update audio system (matches C++ audio->update(frameTime))
    if bestow.audio and bestow.audio.update then
        bestow.audio.update(cappedDt)
    end

    -- NOTE: Input handling moved to game:handleInput() which is called once per frame
    -- before the fixed timestep loop by the runtime

    -- Update based on phase
    if self.currentPhase == GamePhase.MainMenu then
        Camera.updateMainMenu(self, cappedDt)
        return
    end

    if self.currentPhase == GamePhase.WorldMap then
        Camera.updateWorldMap(self, cappedDt)
        return
    end

    -- Only update game logic when playing
    if self.currentPhase == GamePhase.Playing and not self.gameOver then
        -- Snake movement timer
        self.moveTimer = self.moveTimer + cappedDt
        if self.moveTimer >= self.moveInterval then
            self.moveTimer = 0
            Input.applyBufferedInput(self)
            Snake.move(self)
        end

        -- Update enemies
        Enemies.update(self, cappedDt)

        -- Update detached segments
        Snake.updateDetachedSegments(self, cappedDt)
    end

    -- Always update animations
    self:updateAnimations(cappedDt)

    -- Always update camera (for game phases)
    Camera.update(self, cappedDt)
end

function game:draw()
    Rendering.draw(self)
end

function game:shutdown()
    log.info("")
    log.info("========================================")
    log.info("Snake 3D - Lua Edition")
    log.info("Final Score: %d", self.score)
    log.info("Snake Length: %d", #self.snake)
    log.info("Thanks for playing!")
    log.info("========================================")
end

log.info("Snake 3D - Lua Edition loaded!")
