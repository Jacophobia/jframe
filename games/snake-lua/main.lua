-- Snake Game - Lua Implementation
-- A complete port of the C++ snake game to Lua using Bestow contracts
--
-- Structure:
--   main.lua      - Entry point and game loop
--   config.lua    - Constants and configuration
--   types.lua     - Data structures (GridPos, SnakeSegment, etc.)
--   state.lua     - Game state management
--   input.lua     - Input handling per phase
--   rendering.lua - All drawing functions
--   game_logic.lua - Snake movement, collision, food
--   enemies.lua   - Enemy AI and management
--   levels.lua    - Level/world loading
--   audio.lua     - Sound and music
--   ui.lua        - Menus and HUD
--   camera.lua    - Camera management
--   pixeltext.lua - Pixel font rendering
--   worldmap.lua  - World map rendering

-- Game phases (matching C++ GamePhase enum)
GamePhase = {
    MainMenu = "MainMenu",
    WorldMap = "WorldMap",
    Playing = "Playing",
    Paused = "Paused",
    BossFight = "BossFight",
    LevelComplete = "LevelComplete",
    GameOver = "GameOver"
}

-- Direction enum (matching C++ Direction)
Direction = {
    Up = "Up",       -- -Z
    Down = "Down",   -- +Z
    Left = "Left",   -- -X
    Right = "Right"  -- +X
}

-- Load all modules
local config = require("config")
local types = require("types")
local state = require("state")
local input = require("input")
local rendering = require("rendering")
local game_logic = require("game_logic")
local enemies = require("enemies")
local levels = require("levels")
local audio = require("audio")
local ui = require("ui")
local camera = require("camera")
local pixeltext = require("pixeltext")
local worldmap = require("worldmap")

-- Main game table
local game = {
    title = "Snake 3D - Bestow Lua Demo",
    width = 1280,
    height = 720
}

-- Initialize the game
function game.init()
    -- Initialize graphics
    local gfxConfig = {
        windowWidth = game.width,
        windowHeight = game.height,
        windowTitle = game.title,
        vsync = true,
        fullscreen = false
    }

    if not bestow.graphics3d.initialize(gfxConfig) then
        print("Failed to initialize graphics")
        return false
    end

    -- Set background color (green)
    bestow.graphics3d.setClearColor(Color.new(30, 120, 50, 255))

    -- Initialize input
    local windowHandle = bestow.graphics3d.getNativeWindowHandle()
    if windowHandle then
        bestow.input.initialize(windowHandle)
    end

    -- Initialize config system
    if bestow.config then
        bestow.config.initialize()
    end

    -- Initialize audio
    if bestow.audio then
        bestow.audio.initialize()
    end

    -- Create meshes
    state.cubeMesh = bestow.graphics3d.createCubeMesh(1.0)
    state.groundMesh = bestow.graphics3d.createPlaneMesh(1.0, 1.0, 1, 1)

    -- Load sounds
    audio.loadSoundConfig()

    -- Load the first world
    if not levels.loadWorld("data/worlds/forest/world.lua") then
        print("Warning: Could not load world, using defaults")
    end

    -- Initialize random number generator
    math.randomseed(os.time())

    -- Set initial game state
    state.currentPhase = GamePhase.MainMenu
    state.running = true
    state.gameTime = 0

    -- Setup initial camera
    camera.setup()

    -- Setup lighting
    bestow.graphics3d.setDirectionalLight({
        direction = Vec3.new(0.5, -1.0, 0.3),
        color = Vec3.new(1.0, 1.0, 1.0),
        intensity = 1.5
    })

    print("Snake game initialized")
    return true
end

-- Main game loop
function game.loop()
    local fixedDt = 1.0 / 60.0
    local accumulator = 0.0
    local lastTime = 0.0

    while state.running and not bestow.graphics3d.shouldClose() do
        local dt = bestow.core.deltaTime()
        accumulator = accumulator + dt
        state.gameTime = state.gameTime + dt

        -- Fixed timestep updates
        while accumulator >= fixedDt do
            -- Update input
            bestow.input.update()

            -- Handle input based on current phase
            input.handleInput(fixedDt)

            -- Update game logic based on phase
            if state.currentPhase == GamePhase.MainMenu then
                state.menuAnimTime = state.menuAnimTime + fixedDt
            elseif state.currentPhase == GamePhase.WorldMap then
                levels.updateWorldMap(fixedDt)
            elseif state.currentPhase == GamePhase.Playing then
                if not state.gameOver then
                    game_logic.update(fixedDt)
                    enemies.update(fixedDt)
                    game_logic.updateDetachedSegments(fixedDt)
                    game_logic.updateAnimations(fixedDt)
                    camera.fullUpdate(fixedDt)
                end
            elseif state.currentPhase == GamePhase.Paused then
                -- Paused - no updates
            elseif state.currentPhase == GamePhase.LevelComplete then
                state.menuAnimTime = state.menuAnimTime + fixedDt
            elseif state.currentPhase == GamePhase.GameOver then
                state.menuAnimTime = state.menuAnimTime + fixedDt
            end

            accumulator = accumulator - fixedDt
        end

        -- Render based on current phase
        bestow.graphics3d.beginFrame()

        if state.currentPhase == GamePhase.MainMenu then
            ui.drawMainMenu()
        elseif state.currentPhase == GamePhase.WorldMap then
            worldmap.draw()
        elseif state.currentPhase == GamePhase.Playing or
               state.currentPhase == GamePhase.Paused or
               state.currentPhase == GamePhase.LevelComplete or
               state.currentPhase == GamePhase.GameOver then

            -- Draw game world
            rendering.drawGround()
            rendering.drawObstacles()
            rendering.drawEnemies()
            rendering.drawSnake()
            rendering.drawDetachedSegments()
            rendering.drawFoodPickups()
            rendering.drawFood()
            rendering.drawFoodPopEffect()
            rendering.drawGridBorder()
            rendering.drawHUD()
            rendering.drawLevelCompleteEffect()

            -- Draw UI overlays
            ui.drawHUDText()

            if state.currentPhase == GamePhase.Paused then
                ui.drawPauseMenu()
            elseif state.currentPhase == GamePhase.LevelComplete then
                ui.drawLevelCompleteUI()
            elseif state.currentPhase == GamePhase.GameOver then
                ui.drawGameOverMenu()
            end
        end

        bestow.graphics3d.endFrame()

        -- Update audio
        if bestow.audio then
            bestow.audio.update()
        end
    end
end

-- Cleanup
function game.cleanup()
    bestow.input.shutdown()
    bestow.graphics3d.shutdown()
    print("Snake game cleanup complete")
end

-- Transition to a new game phase
function game.transitionTo(newPhase)
    local oldPhase = state.currentPhase

    -- Exit current phase
    if oldPhase == GamePhase.Playing then
        -- Nothing special on exit
    elseif oldPhase == GamePhase.Paused then
        -- Nothing special on exit
    end

    -- Store previous phase
    state.previousPhase = oldPhase
    state.currentPhase = newPhase

    -- Enter new phase
    if newPhase == GamePhase.MainMenu then
        state.mainMenuSelection = 0
        audio.playMusicTrack(state.musicMenu)
    elseif newPhase == GamePhase.WorldMap then
        worldmap.initialize()
        levels.updateNodeStatuses()
        -- Continue menu music
    elseif newPhase == GamePhase.Playing then
        state.gameOver = false
        audio.playMusicTrack(state.musicGame)
    elseif newPhase == GamePhase.Paused then
        state.pauseMenuSelection = 0
        audio.playSound(state.soundPause)
    elseif newPhase == GamePhase.LevelComplete then
        audio.playSound(state.soundLevelComplete)
        levels.updateWorldProgress()
    elseif newPhase == GamePhase.GameOver then
        state.gameOver = true
        state.gameOverMenuSelection = 0
        audio.playSound(state.soundGameOver)
        audio.playMusicTrack(state.musicMenu)
    end
end

-- Run the game
function game.run()
    if not game.init() then
        print("Failed to initialize game")
        return
    end

    game.loop()
    game.cleanup()
end

-- Export for bestow runtime
return game
