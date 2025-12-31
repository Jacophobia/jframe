-- games/snake3d/modules/input.lua
-- Input handling per game phase
--
-- Matches C++ input from snake.game.cppm lines 1771-2393
-- Supports Dvorak (,AOE) and Arrow keys

local Constants = bestow.include("modules/constants")
local GamePhase = bestow.include("modules/phases")
local Direction = bestow.include("modules/direction")
local Audio = bestow.include("modules/audio")
local Log = bestow.include("modules/log")

local Input = {}

local log = Log.category("Input")

-- GLFW key codes (matching C++)
local KEY = {
    COMMA = 44,      -- Dvorak W (up)
    O = 79,          -- Dvorak S (down)
    A = 65,          -- Left
    E = 69,          -- Dvorak D (right)
    UP = 265,
    DOWN = 264,
    LEFT = 263,
    RIGHT = 262,
    ESCAPE = 256,
    SPACE = 32,
    ENTER = 257,
    R = 82,
}

-- Check if a key was just pressed
local function keyPressed(key)
    return bestow.input.wasKeyJustPressed(key)
end

-- Handle all input based on current game phase
function Input.handleInput(game)
    log.trace("Handling input for phase %s", GamePhase.getName(game.currentPhase))

    if game.currentPhase == GamePhase.MainMenu then
        Input.handleMainMenuInput(game)
        return
    end

    if game.currentPhase == GamePhase.Paused then
        Input.handlePauseMenuInput(game)
        return
    end

    if game.currentPhase == GamePhase.WorldMap then
        Input.handleWorldMapInput(game)
        return
    end

    if game.currentPhase == GamePhase.LevelComplete then
        Input.handleLevelCompleteInput(game)
        return
    end

    if game.gameOver then
        Input.handleGameOverInput(game)
        return
    end

    -- Default: Playing phase gameplay input
    if game.currentPhase == GamePhase.Playing then
        Input.handlePlayingInput(game)
    end
end

-- Handle main menu input
-- Matches C++ handleMainMenuInput() at lines 2336-2359
function Input.handleMainMenuInput(game)
    -- Navigate Up
    if keyPressed(KEY.COMMA) or keyPressed(KEY.UP) then
        game.mainMenuSelection = (game.mainMenuSelection - 1 + Constants.MAIN_MENU_COUNT) % Constants.MAIN_MENU_COUNT
        Audio.playSFX("menu_move")
        log.debug("Menu selection changed to %d", game.mainMenuSelection)
    end

    -- Navigate Down
    if keyPressed(KEY.O) or keyPressed(KEY.DOWN) then
        game.mainMenuSelection = (game.mainMenuSelection + 1) % Constants.MAIN_MENU_COUNT
        Audio.playSFX("menu_move")
        log.debug("Menu selection changed to %d", game.mainMenuSelection)
    end

    -- Select
    if keyPressed(KEY.ENTER) or keyPressed(KEY.SPACE) then
        Audio.playSFX("menu_select")

        if game.mainMenuSelection == Constants.MAIN_MENU_PLAY then
            log.info("Starting game from main menu")
            game:startGame()
        elseif game.mainMenuSelection == Constants.MAIN_MENU_QUIT then
            log.info("Quitting from main menu")
            game.running = false
        end
    end
end

-- Handle pause menu input
-- Matches C++ handlePauseMenuInput() at lines 2361-2393
function Input.handlePauseMenuInput(game)
    -- Navigate Up
    if keyPressed(KEY.COMMA) or keyPressed(KEY.UP) then
        game.pauseMenuSelection = (game.pauseMenuSelection - 1 + Constants.PAUSE_MENU_COUNT) % Constants.PAUSE_MENU_COUNT
        Audio.playSFX("menu_move")
        log.debug("Pause menu selection changed to %d", game.pauseMenuSelection)
    end

    -- Navigate Down
    if keyPressed(KEY.O) or keyPressed(KEY.DOWN) then
        game.pauseMenuSelection = (game.pauseMenuSelection + 1) % Constants.PAUSE_MENU_COUNT
        Audio.playSFX("menu_move")
        log.debug("Pause menu selection changed to %d", game.pauseMenuSelection)
    end

    -- Select
    if keyPressed(KEY.ENTER) or keyPressed(KEY.SPACE) then
        Audio.playSFX("menu_select")

        if game.pauseMenuSelection == Constants.PAUSE_MENU_RESUME then
            log.info("Resuming game")
            game:transitionTo(GamePhase.Playing)
        elseif game.pauseMenuSelection == Constants.PAUSE_MENU_RESTART then
            log.info("Restarting game")
            game:restart()
        elseif game.pauseMenuSelection == Constants.PAUSE_MENU_QUIT then
            log.info("Quitting to main menu")
            game:transitionTo(GamePhase.MainMenu)
        end
    end

    -- ESC for quick resume
    if keyPressed(KEY.ESCAPE) then
        Audio.playSFX("menu_select")
        log.info("Quick resume via ESC")
        game:transitionTo(GamePhase.Playing)
    end
end

-- Handle world map input
-- Matches C++ handleWorldMapInput() at lines 2246-2334
function Input.handleWorldMapInput(game)
    -- For now, simplified: just start playing on any confirm
    if keyPressed(KEY.ENTER) or keyPressed(KEY.SPACE) then
        Audio.playSFX("menu_select")
        log.info("Starting level from world map")
        game:startGame()
    end

    -- ESC to return to main menu
    if keyPressed(KEY.ESCAPE) then
        log.info("Returning to main menu from world map")
        game:transitionTo(GamePhase.MainMenu)
    end
end

-- Handle level complete input
-- Matches C++ lines 1792-1804
function Input.handleLevelCompleteInput(game)
    if keyPressed(KEY.ENTER) or keyPressed(KEY.SPACE) then
        log.info("Proceeding to next level")
        game:proceedToNextLevel()
    end

    if keyPressed(KEY.ESCAPE) then
        log.info("Returning to world map from level complete")
        game:returnToWorldMap()
    end
end

-- Handle game over input
-- Matches C++ lines 1806-1814
function Input.handleGameOverInput(game)
    if keyPressed(KEY.R) then
        log.info("Restarting game after game over")
        game:restart()
    end

    if keyPressed(KEY.ESCAPE) then
        log.info("Exiting game after game over")
        game.running = false
    end
end

-- Handle playing phase input (movement)
-- Matches C++ handleInput() lines 1816-1856
function Input.handlePlayingInput(game)
    -- Direction input with anti-reversing validation
    -- Up: Comma (Dvorak W) or Up Arrow
    if keyPressed(KEY.COMMA) or keyPressed(KEY.UP) then
        if game.direction ~= Direction.Down then
            game.inputDirection = Direction.Up
            game.hasBufferedInput = true
            log.trace("Buffered input: Up")
        end
    end

    -- Down: O (Dvorak S) or Down Arrow
    if keyPressed(KEY.O) or keyPressed(KEY.DOWN) then
        if game.direction ~= Direction.Up then
            game.inputDirection = Direction.Down
            game.hasBufferedInput = true
            log.trace("Buffered input: Down")
        end
    end

    -- Left: A or Left Arrow
    if keyPressed(KEY.A) or keyPressed(KEY.LEFT) then
        if game.direction ~= Direction.Right then
            game.inputDirection = Direction.Left
            game.hasBufferedInput = true
            log.trace("Buffered input: Left")
        end
    end

    -- Right: E (Dvorak D) or Right Arrow
    if keyPressed(KEY.E) or keyPressed(KEY.RIGHT) then
        if game.direction ~= Direction.Left then
            game.inputDirection = Direction.Right
            game.hasBufferedInput = true
            log.trace("Buffered input: Right")
        end
    end

    -- ESC to pause
    if keyPressed(KEY.ESCAPE) then
        game.pauseMenuSelection = 0
        Audio.playSFX("pause")
        log.info("Game paused")
        game:transitionTo(GamePhase.Paused)
    end
end

-- Apply buffered input direction
-- Matches C++ applyBufferedInput() at lines 1858-1863
function Input.applyBufferedInput(game)
    if game.hasBufferedInput then
        game.nextDirection = game.inputDirection
        game.hasBufferedInput = false
        log.trace("Applied buffered direction: %s", Direction.getName(game.nextDirection))
    end
end

return Input
