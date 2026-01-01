-- input.lua - Input handling for each game phase
-- Dependencies: app.config, app.state, app.types, app.main, app.levels, app.game_logic, app.audio
-- (accessed inside functions)

local input = {}

-- Handle input based on current game phase
function input.handleInput(dt)
    local state = app.state

    if state.currentPhase == GamePhase.MainMenu then
        input.handleMainMenuInput()
    elseif state.currentPhase == GamePhase.WorldMap then
        input.handleWorldMapInput()
    elseif state.currentPhase == GamePhase.Playing then
        input.handlePlayingInput(dt)
    elseif state.currentPhase == GamePhase.Paused then
        input.handlePauseMenuInput()
    elseif state.currentPhase == GamePhase.LevelComplete then
        input.handleLevelCompleteInput()
    elseif state.currentPhase == GamePhase.GameOver then
        input.handleGameOverInput()
    end
end

-- Main menu input
function input.handleMainMenuInput()
    local config = app.config
    local state = app.state
    local audio = app.audio

    -- Navigation: Comma (Dvorak W) or Up
    if bestow.input.wasKeyJustPressed(Keys.Comma) or
       bestow.input.wasKeyJustPressed(Keys.Up) then
        state.mainMenuSelection = (state.mainMenuSelection - 1) % config.MAIN_MENU_COUNT
        if state.mainMenuSelection < 0 then
            state.mainMenuSelection = config.MAIN_MENU_COUNT - 1
        end
        audio.playMenuMove()
    end

    -- Navigation: O (Dvorak S) or Down
    if bestow.input.wasKeyJustPressed(Keys.O) or
       bestow.input.wasKeyJustPressed(Keys.Down) then
        state.mainMenuSelection = (state.mainMenuSelection + 1) % config.MAIN_MENU_COUNT
        audio.playMenuMove()
    end

    -- Select: Enter or Space
    if bestow.input.wasKeyJustPressed(Keys.Enter) or
       bestow.input.wasKeyJustPressed(Keys.Space) then
        audio.playMenuSelect()
        if state.mainMenuSelection == config.MAIN_MENU_PLAY then
            -- Transition to world map
            local main = app.main
            main.transitionTo(GamePhase.WorldMap)
        elseif state.mainMenuSelection == config.MAIN_MENU_QUIT then
            state.running = false
        end
    end
end

-- World map input
function input.handleWorldMapInput()
    local state = app.state
    local audio = app.audio
    local levels = app.levels
    local main = app.main

    if #state.currentWorld.nodes == 0 then return end

    local prevSelection = state.selectedNodeIndex

    -- Find connected nodes from current selection
    local function findNodeInDirection(dx, dz)
        local currentNode = state.currentWorld.nodes[state.selectedNodeIndex + 1]  -- Lua 1-indexed
        if not currentNode then return -1 end

        local bestIndex = -1
        local bestDist = math.huge

        for i, node in ipairs(state.currentWorld.nodes) do
            local nodeIdx = i - 1  -- Convert to 0-indexed for comparison
            if nodeIdx ~= state.selectedNodeIndex then
                local ddx = node.gridPos.x - currentNode.gridPos.x
                local ddz = node.gridPos.z - currentNode.gridPos.z

                -- Check if node is in the right direction
                local inDirection = false
                if dx ~= 0 and ((dx > 0 and ddx > 0) or (dx < 0 and ddx < 0)) then
                    inDirection = true
                end
                if dz ~= 0 and ((dz > 0 and ddz > 0) or (dz < 0 and ddz < 0)) then
                    inDirection = true
                end

                if inDirection then
                    local dist = math.abs(ddx) + math.abs(ddz)
                    if dist < bestDist then
                        bestDist = dist
                        bestIndex = nodeIdx
                    end
                end
            end
        end

        return bestIndex
    end

    -- Up: Comma (Dvorak W) or Up Arrow
    if bestow.input.wasKeyJustPressed(Keys.Comma) or
       bestow.input.wasKeyJustPressed(Keys.Up) then
        local node = findNodeInDirection(0, -1)
        if node >= 0 then state.selectedNodeIndex = node end
    end

    -- Down: O (Dvorak S) or Down Arrow
    if bestow.input.wasKeyJustPressed(Keys.O) or
       bestow.input.wasKeyJustPressed(Keys.Down) then
        local node = findNodeInDirection(0, 1)
        if node >= 0 then state.selectedNodeIndex = node end
    end

    -- Left: A or Left Arrow
    if bestow.input.wasKeyJustPressed(Keys.A) or
       bestow.input.wasKeyJustPressed(Keys.Left) then
        local node = findNodeInDirection(-1, 0)
        if node >= 0 then state.selectedNodeIndex = node end
    end

    -- Right: E (Dvorak D) or Right Arrow
    if bestow.input.wasKeyJustPressed(Keys.E) or
       bestow.input.wasKeyJustPressed(Keys.Right) then
        local node = findNodeInDirection(1, 0)
        if node >= 0 then state.selectedNodeIndex = node end
    end

    -- Play sound if selection changed
    if state.selectedNodeIndex ~= prevSelection then
        audio.playMenuMove()
    end

    -- Select level: Enter or Space
    if bestow.input.wasKeyJustPressed(Keys.Enter) or
       bestow.input.wasKeyJustPressed(Keys.Space) then
        local node = state.currentWorld.nodes[state.selectedNodeIndex + 1]
        if node and node.isUnlocked then
            audio.playMenuSelect()
            state.currentLevelIndex = node.levelIndex
            if levels.loadLevel(state.currentWorld.levelFiles[node.levelIndex + 1]) then
                main.transitionTo(GamePhase.Playing)
            end
        end
    end

    -- Back to main menu: Escape
    if bestow.input.wasKeyJustPressed(Keys.Escape) then
        main.transitionTo(GamePhase.MainMenu)
    end
end

-- Playing input - movement controls
function input.handlePlayingInput(dt)
    local state = app.state
    local types = app.types
    local levels = app.levels
    local main = app.main
    local audio = app.audio

    -- Level complete phase - wait for input to proceed
    if state.currentPhase == GamePhase.LevelComplete then
        if bestow.input.wasKeyJustPressed(Keys.Enter) or
           bestow.input.wasKeyJustPressed(Keys.Space) then
            levels.proceedToNextLevel()
        end
        if bestow.input.wasKeyJustPressed(Keys.Escape) then
            levels.returnToWorldMap()
        end
        return
    end

    -- Game over - handled by handleGameOverInput
    if state.gameOver then
        return
    end

    -- Direction input (Dvorak-friendly: ,AOE instead of WASD)
    -- Up: Comma (Dvorak W) or Up Arrow
    if bestow.input.wasKeyJustPressed(Keys.Comma) or
       bestow.input.wasKeyJustPressed(Keys.Up) then
        if not types.isOppositeDirection(Direction.Up, state.direction) then
            state.nextDirection = Direction.Up
            state.hasBufferedInput = true
        end
    end

    -- Down: O (Dvorak S) or Down Arrow
    if bestow.input.wasKeyJustPressed(Keys.O) or
       bestow.input.wasKeyJustPressed(Keys.Down) then
        if not types.isOppositeDirection(Direction.Down, state.direction) then
            state.nextDirection = Direction.Down
            state.hasBufferedInput = true
        end
    end

    -- Left: A or Left Arrow
    if bestow.input.wasKeyJustPressed(Keys.A) or
       bestow.input.wasKeyJustPressed(Keys.Left) then
        if not types.isOppositeDirection(Direction.Left, state.direction) then
            state.nextDirection = Direction.Left
            state.hasBufferedInput = true
        end
    end

    -- Right: E (Dvorak D) or Right Arrow
    if bestow.input.wasKeyJustPressed(Keys.E) or
       bestow.input.wasKeyJustPressed(Keys.Right) then
        if not types.isOppositeDirection(Direction.Right, state.direction) then
            state.nextDirection = Direction.Right
            state.hasBufferedInput = true
        end
    end

    -- Pause: Escape
    if bestow.input.wasKeyJustPressed(Keys.Escape) then
        state.pauseMenuSelection = 0
        audio.playPause()
        main.transitionTo(GamePhase.Paused)
    end
end

-- Pause menu input
function input.handlePauseMenuInput()
    local config = app.config
    local state = app.state
    local main = app.main
    local game_logic = app.game_logic
    local levels = app.levels
    local audio = app.audio

    -- Navigation: Comma or Up
    if bestow.input.wasKeyJustPressed(Keys.Comma) or
       bestow.input.wasKeyJustPressed(Keys.Up) then
        state.pauseMenuSelection = (state.pauseMenuSelection - 1) % config.PAUSE_MENU_COUNT
        if state.pauseMenuSelection < 0 then
            state.pauseMenuSelection = config.PAUSE_MENU_COUNT - 1
        end
        audio.playMenuMove()
    end

    -- Navigation: O or Down
    if bestow.input.wasKeyJustPressed(Keys.O) or
       bestow.input.wasKeyJustPressed(Keys.Down) then
        state.pauseMenuSelection = (state.pauseMenuSelection + 1) % config.PAUSE_MENU_COUNT
        audio.playMenuMove()
    end

    -- Select: Enter or Space
    if bestow.input.wasKeyJustPressed(Keys.Enter) or
       bestow.input.wasKeyJustPressed(Keys.Space) then
        audio.playMenuSelect()
        if state.pauseMenuSelection == config.PAUSE_MENU_RESUME then
            main.transitionTo(GamePhase.Playing)
        elseif state.pauseMenuSelection == config.PAUSE_MENU_RESTART then
            game_logic.restartGame()
            main.transitionTo(GamePhase.Playing)
        elseif state.pauseMenuSelection == config.PAUSE_MENU_QUIT then
            levels.returnToWorldMap()
        end
    end

    -- Resume on Escape
    if bestow.input.wasKeyJustPressed(Keys.Escape) then
        main.transitionTo(GamePhase.Playing)
    end
end

-- Level complete input
function input.handleLevelCompleteInput()
    local levels = app.levels

    if bestow.input.wasKeyJustPressed(Keys.Enter) or
       bestow.input.wasKeyJustPressed(Keys.Space) then
        levels.proceedToNextLevel()
    end
    if bestow.input.wasKeyJustPressed(Keys.Escape) then
        levels.returnToWorldMap()
    end
end

-- Game over input
function input.handleGameOverInput()
    local config = app.config
    local state = app.state
    local main = app.main
    local game_logic = app.game_logic
    local levels = app.levels
    local audio = app.audio

    -- Navigation: Comma or Up
    if bestow.input.wasKeyJustPressed(Keys.Comma) or
       bestow.input.wasKeyJustPressed(Keys.Up) then
        state.gameOverMenuSelection = (state.gameOverMenuSelection - 1) % config.GAME_OVER_COUNT
        if state.gameOverMenuSelection < 0 then
            state.gameOverMenuSelection = config.GAME_OVER_COUNT - 1
        end
        audio.playMenuMove()
    end

    -- Navigation: O or Down
    if bestow.input.wasKeyJustPressed(Keys.O) or
       bestow.input.wasKeyJustPressed(Keys.Down) then
        state.gameOverMenuSelection = (state.gameOverMenuSelection + 1) % config.GAME_OVER_COUNT
        audio.playMenuMove()
    end

    -- Select: Enter or Space
    if bestow.input.wasKeyJustPressed(Keys.Enter) or
       bestow.input.wasKeyJustPressed(Keys.Space) then
        audio.playMenuSelect()
        if state.gameOverMenuSelection == config.GAME_OVER_RETRY then
            game_logic.restartGame()
            main.transitionTo(GamePhase.Playing)
        elseif state.gameOverMenuSelection == config.GAME_OVER_QUIT then
            levels.returnToWorldMap()
        end
    end
end

return input
