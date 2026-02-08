-- Example: Game State Machine
-- Shows main menu, gameplay, pause, and game over states with transitions

-- main.lua
return {
    title = "State Machine Example",
    width = 1280,
    height = 720,

    init = function()
        app.main.state = {
            phase = "main_menu",
            score = 0,
            player = nil
        }

        -- Set up pause action
        bestow.action.builder():duringPhase("gameplay"):whenPressed(KeyCode.Escape):emitAction("Pause"):discretely()
        bestow.action.builder():duringPhase("pause"):whenPressed(KeyCode.Escape):emitAction("Resume"):discretely()

        -- Initialize UI
        bestow.ui.initialize()
    end,

    update = function(dt)
        local state = app.main.state

        if state.phase == "main_menu" then
            return app.main.updateMainMenu(dt)
        elseif state.phase == "playing" then
            return app.main.updatePlaying(dt)
        elseif state.phase == "paused" then
            return app.main.updatePaused(dt)
        elseif state.phase == "game_over" then
            return app.main.updateGameOver(dt)
        end

        return true
    end,

    render = function()
        bestow.graphics3d.beginFrame()

        local state = app.main.state

        if state.phase == "playing" or state.phase == "paused" or state.phase == "game_over" then
            -- Always render the game world when it exists
        end

        bestow.ui.render()
        bestow.graphics3d.endFrame()
    end,

    -- State-specific update functions
    updateMainMenu = function(dt)
        if bestow.input.wasKeyJustPressed(KeyCode.Enter) or
           bestow.input.wasKeyJustPressed(KeyCode.Space) then
            app.main.startGame()
        end
        if bestow.input.wasKeyJustPressed(KeyCode.Escape) then
            return false  -- Quit
        end
        return true
    end,

    updatePlaying = function(dt)
        local state = app.main.state

        bestow.timer.update(dt)

        if bestow.input.wasActionJustPressed("Pause") then
            state.phase = "paused"
            bestow.phase.push("pause")
            bestow.audio.setMasterVolume(0.3)
            return true
        end

        -- Update game systems
        app.systems.player.update(dt)
        app.systems.camera.update(dt)

        -- Check game over
        if state.player and bestow.entity.isValid(state.player) then
            local health = bestow.entity.getComponent(state.player, "Health")
            if health and health.current <= 0 then
                state.phase = "game_over"
            end
        end

        return true
    end,

    updatePaused = function(dt)
        if bestow.input.wasActionJustPressed("Resume") then
            app.main.state.phase = "playing"
            bestow.phase.pop()
            bestow.audio.setMasterVolume(1.0)
        end
        return true
    end,

    updateGameOver = function(dt)
        if bestow.input.wasKeyJustPressed(KeyCode.Space) then
            app.main.startGame()
        end
        if bestow.input.wasKeyJustPressed(KeyCode.Escape) then
            app.main.state.phase = "main_menu"
        end
        return true
    end,

    -- Transitions
    startGame = function()
        local state = app.main.state
        state.phase = "playing"
        state.score = 0
        bestow.phase.push("gameplay")
        bestow.audio.setMasterVolume(1.0)
        -- Load level, create player, etc.
    end
}
