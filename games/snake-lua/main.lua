-- Snake Game - Lua Implementation
-- A complete port of the C++ snake game to Lua using Bestow contracts
--
-- Structure:
--   main.lua      - Entry point and game loop (becomes app.main)
--   config.lua    - Constants and configuration (app.config)
--   types.lua     - Data structures (app.types)
--   state.lua     - Game state management (app.state)
--   input.lua     - Input handling per phase (app.input)
--   rendering.lua - All drawing functions (app.rendering)
--   game_logic.lua - Snake movement, collision, food (app.game_logic)
--   enemies.lua   - Enemy AI and management (app.enemies)
--   levels.lua    - Level/world loading (app.levels)
--   audio.lua     - Sound and music (app.audio)
--   ui.lua        - Menus and HUD (app.ui)
--   camera.lua    - Camera management (app.camera)
--   pixeltext.lua - Pixel font rendering (app.pixeltext)
--   worldmap.lua  - World map rendering (app.worldmap)
--
-- All modules are accessed via app.* namespace inside functions for hot-reload support.
-- Dependencies are NOT required at file scope.

-- Game phases (matching C++ GamePhase enum)
-- These are GLOBALS so they can be used across all modules
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
-- These are GLOBALS so they can be used across all modules
Direction = {
    Up = "Up",       -- -Z
    Down = "Down",   -- +Z
    Left = "Left",   -- -X
    Right = "Right"  -- +X
}

-- Main game table
local main = {
    title = "Snake 3D - Bestow Lua Demo",
    width = 1280,
    height = 720
}

-- Initialize the game
function main.init()
    local state = app.state
    local audio = app.audio
    local levels = app.levels
    local camera = app.camera

    -- Initialize graphics
    local gfxConfig = {
        windowWidth = main.width,
        windowHeight = main.height,
        windowTitle = main.title,
        vsync = true,
        fullscreen = false
    }

    if not bestow.graphics3d.initialize(gfxConfig) then
        bestow.error("Failed to initialize graphics")
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

    -- Initialize state with proper types (must happen before any state access)
    state.initialize()

    -- Create meshes
    state.cubeMesh = bestow.graphics3d.createCubeMesh(1.0)
    state.groundMesh = bestow.graphics3d.createPlaneMesh(1.0, 1.0, 1, 1)

    -- Load sounds
    audio.loadSoundConfig()

    -- Load the first world
    if not levels.loadWorld("data/worlds/forest/world.lua") then
        bestow.warn("Could not load world, using defaults")
    end

    -- Initialize random number generator
    math.randomseed(bestow.util.time())

    -- Set initial game state
    state.running = true
    state.gameTime = 0

    -- Setup initial camera
    camera.setup()

    -- Start at main menu (this triggers menu music via transitionTo)
    main.transitionTo(GamePhase.MainMenu)

    -- Setup lighting
    bestow.graphics3d.setDirectionalLight({
        direction = Vec3.new(0.5, -1.0, 0.3),
        color = Vec3.new(1.0, 1.0, 1.0),
        intensity = 1.5
    })

    bestow.info("Snake game initialized")
    return true
end

-- Main game loop
function main.loop()
    local state = app.state
    local input = app.input
    local levels = app.levels
    local game_logic = app.game_logic
    local enemies = app.enemies
    local camera = app.camera
    local rendering = app.rendering
    local ui = app.ui
    local worldmap = app.worldmap

    local fixedDt = 1.0 / 60.0
    local accumulator = 0.0

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
            bestow.audio.update(dt)
        end

        -- Tracy profiling: mark frame end and send performance data
        if bestow.profiler and bestow.profiler.isEnabled() then
            bestow.profiler.plot("Frame Time (ms)", dt * 1000)
            bestow.profiler.plot("FPS", 1.0 / dt)
            bestow.profiler.frameMark()
        end
    end
end

-- Cleanup
function main.cleanup()
    bestow.input.shutdown()
    bestow.graphics3d.shutdown()
    bestow.info("Snake game cleanup complete")
end

-- Transition to a new game phase
function main.transitionTo(newPhase)
    local state = app.state
    local audio = app.audio
    local worldmap = app.worldmap
    local levels = app.levels

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
function main.run()
    if not main.init() then
        bestow.error("Failed to initialize game")
        return
    end

    main.loop()
    main.cleanup()
end

-- Export for bestow runtime (becomes app.main)
return main
