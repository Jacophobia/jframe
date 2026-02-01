---
name: game-state
description: Manage game states like menus, gameplay, and pause screens in Bestow. Use when implementing main menus, pause screens, game over states, or state transitions.
---

# Game State Management

Manage different game states (menus, gameplay, pause) with clean transitions.

## Simple State Pattern

For simpler games, use a state variable:

```lua
-- main.lua
return {
    init = function()
        app.main.state = {
            phase = "main_menu",  -- "main_menu", "playing", "paused", "game_over"
            player = nil,
            score = 0
        }
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
        local state = app.main.state

        bestow.graphics3d.beginFrame()

        if state.phase == "main_menu" then
            app.main.renderMainMenu()
        elseif state.phase == "playing" then
            app.main.renderPlaying()
        elseif state.phase == "paused" then
            app.main.renderPlaying()  -- Draw game in background
            app.main.renderPaused()   -- Draw pause overlay
        elseif state.phase == "game_over" then
            app.main.renderPlaying()  -- Frozen game in background
            app.main.renderGameOver()
        end

        bestow.graphics3d.endFrame()
    end,

    -- State-specific update functions
    updateMainMenu = function(dt)
        if bestow.input.wasKeyJustPressed(Keys.Space) or
           bestow.input.wasKeyJustPressed(Keys.Enter) then
            app.main.startGame()
        end

        if bestow.input.wasKeyJustPressed(Keys.Escape) then
            return false  -- Quit
        end

        return true
    end,

    updatePlaying = function(dt)
        local state = app.main.state

        -- Escape to pause
        if bestow.input.wasKeyJustPressed(Keys.Escape) then
            state.phase = "paused"
            return true
        end

        -- Update game systems
        app.systems.movement.update(dt)
        app.systems.enemies.update(dt)
        app.systems.camera.update(dt)

        -- Check game over
        local health = bestow.entity.getComponent(state.player, "Health")
        if health and health.current <= 0 then
            state.phase = "game_over"
        end

        return true
    end,

    updatePaused = function(dt)
        local state = app.main.state

        -- Escape or P to unpause
        if bestow.input.wasKeyJustPressed(Keys.Escape) or
           bestow.input.wasKeyJustPressed(Keys.P) then
            state.phase = "playing"
        end

        -- Q to quit to menu
        if bestow.input.wasKeyJustPressed(Keys.Q) then
            app.main.returnToMenu()
        end

        return true
    end,

    updateGameOver = function(dt)
        -- Space to restart
        if bestow.input.wasKeyJustPressed(Keys.Space) then
            app.main.startGame()
        end

        -- Escape to menu
        if bestow.input.wasKeyJustPressed(Keys.Escape) then
            app.main.returnToMenu()
        end

        return true
    end,

    -- State transitions
    startGame = function()
        local state = app.main.state
        state.phase = "playing"
        state.score = 0

        app.levels.level1.load()
    end,

    returnToMenu = function()
        local state = app.main.state

        -- Unload current level
        if state.currentLevel then
            app.levels[state.currentLevel].unload()
        end

        state.phase = "main_menu"
        state.player = nil
    end
}
```

## State Stack Pattern

For complex games with overlays:

```lua
-- systems/state_manager.lua
return {
    stack = {},  -- Stack of active states

    push = function(stateName)
        local self = app.systems.state_manager
        local state = app.states[stateName]

        if state and state.onEnter then
            state.onEnter()
        end

        table.insert(self.stack, stateName)
    end,

    pop = function()
        local self = app.systems.state_manager

        if #self.stack == 0 then return end

        local stateName = table.remove(self.stack)
        local state = app.states[stateName]

        if state and state.onExit then
            state.onExit()
        end

        -- Resume state below
        local below = self.current()
        if below then
            local belowState = app.states[below]
            if belowState and belowState.onResume then
                belowState.onResume()
            end
        end
    end,

    replace = function(stateName)
        local self = app.systems.state_manager
        self.pop()
        self.push(stateName)
    end,

    current = function()
        local self = app.systems.state_manager
        return self.stack[#self.stack]
    end,

    update = function(dt)
        local self = app.systems.state_manager
        local currentName = self.current()

        if not currentName then return true end

        local state = app.states[currentName]
        if state and state.update then
            return state.update(dt)
        end

        return true
    end,

    render = function()
        local self = app.systems.state_manager

        -- Render from bottom to top (allows transparent overlays)
        for i, stateName in ipairs(self.stack) do
            local state = app.states[stateName]
            if state and state.render then
                state.render()
            end
        end
    end
}
```

### State Definitions

```lua
-- states/main_menu.lua
return {
    onEnter = function()
        app.systems.audio.playMusic("menu")
    end,

    onExit = function()
        -- Cleanup
    end,

    update = function(dt)
        if bestow.input.wasKeyJustPressed(Keys.Enter) then
            app.systems.state_manager.replace("gameplay")
        end

        if bestow.input.wasKeyJustPressed(Keys.Escape) then
            return false  -- Quit game
        end

        return true
    end,

    render = function()
        -- Draw menu using bestow.ui (RmlUI)
        -- Load a menu document in onEnter, show/hide here
        bestow.ui.render()
    end
}

-- states/gameplay.lua
return {
    onEnter = function()
        app.main.state.score = 0
        app.levels.level1.load()
        app.systems.audio.playMusic("gameplay")
    end,

    onExit = function()
        local state = app.main.state
        if state.currentLevel then
            app.levels[state.currentLevel].unload()
        end
    end,

    onPause = function()
        -- Called when another state is pushed on top
    end,

    onResume = function()
        -- Called when overlaying state is popped
    end,

    update = function(dt)
        if bestow.input.wasKeyJustPressed(Keys.Escape) then
            app.systems.state_manager.push("pause")
            return true
        end

        app.systems.movement.update(dt)
        app.systems.enemies.update(dt)
        app.systems.camera.update(dt)

        -- Check player death
        local state = app.main.state
        local health = bestow.entity.getComponent(state.player, "Health")
        if health and health.current <= 0 then
            app.systems.state_manager.push("game_over")
        end

        return true
    end,

    render = function()
        -- Render game world
        app.systems.camera.update(0)  -- Ensure camera is set

        -- Update HUD via bestow.ui
        local state = app.main.state
        local scoreEl = bestow.ui.getElementById(app.systems.hud.doc, "score")
        if scoreEl then
            bestow.ui.setElementText(scoreEl, "Score: " .. state.score)
        end
        bestow.ui.render()
    end
}

-- states/pause.lua
return {
    onEnter = function()
        -- Could pause game systems, mute audio, etc.
        bestow.audio.setMasterVolume(0.3)
    end,

    onExit = function()
        bestow.audio.setMasterVolume(1.0)
    end,

    update = function(dt)
        if bestow.input.wasKeyJustPressed(Keys.Escape) or
           bestow.input.wasKeyJustPressed(Keys.P) then
            app.systems.state_manager.pop()
        end

        if bestow.input.wasKeyJustPressed(Keys.Q) then
            app.systems.state_manager.pop()  -- Exit pause
            app.systems.state_manager.replace("main_menu")  -- Go to menu
        end

        return true
    end,

    render = function()
        -- Use bestow.ui (RmlUI) for pause overlay
        -- Load a pause_overlay.rml document in onEnter, show/hide here
        bestow.ui.render()
    end
}

-- states/game_over.lua
return {
    deathTimer = 0,

    onEnter = function()
        local self = app.states.game_over
        self.deathTimer = 0
        -- Load and show game over UI document
        self.doc = bestow.ui.loadDocument("ui/game_over.rml")
        bestow.ui.showDocument(self.doc)
    end,

    onExit = function()
        local self = app.states.game_over
        if self.doc then
            bestow.ui.hideDocument(self.doc)
        end
    end,

    update = function(dt)
        local self = app.states.game_over
        self.deathTimer = self.deathTimer + dt

        -- Wait a moment before accepting input
        if self.deathTimer < 1.0 then
            return true
        end

        if bestow.input.wasKeyJustPressed(Keys.Space) then
            app.systems.state_manager.pop()  -- Exit game over
            app.systems.state_manager.replace("gameplay")  -- Restart
        end

        if bestow.input.wasKeyJustPressed(Keys.Escape) then
            app.systems.state_manager.pop()
            app.systems.state_manager.replace("main_menu")
        end

        return true
    end,

    render = function()
        -- Update dynamic UI elements
        local state = app.main.state
        local scoreEl = bestow.ui.getElementById(app.states.game_over.doc, "score")
        if scoreEl then
            bestow.ui.setElementText(scoreEl, "Final Score: " .. state.score)
        end
        bestow.ui.render()
    end
}
```

## Using the State Stack

```lua
-- In main.lua
init = function()
    app.main.state = {}
    app.systems.state_manager.push("main_menu")
end,

update = function(dt)
    return app.systems.state_manager.update(dt)
end,

render = function()
    bestow.graphics3d.beginFrame()
    app.systems.state_manager.render()
    bestow.graphics3d.endFrame()
end
```

## State Transitions with Effects

```lua
-- systems/state_manager.lua (enhanced)
return {
    stack = {},
    transitioning = false,
    transitionType = nil,
    transitionProgress = 0,
    pendingState = nil,

    transitionTo = function(stateName, transitionType)
        local self = app.systems.state_manager
        self.transitioning = true
        self.transitionType = transitionType or "fade"
        self.transitionProgress = 0
        self.pendingState = stateName
    end,

    update = function(dt)
        local self = app.systems.state_manager

        if self.transitioning then
            self.transitionProgress = self.transitionProgress + dt / 0.5

            if self.transitionProgress >= 1 then
                -- Complete transition
                self.pop()
                self.push(self.pendingState)
                self.transitioning = false
                self.pendingState = nil
            end

            return true
        end

        -- Normal update...
    end,

    renderTransition = function()
        local self = app.systems.state_manager
        if not self.transitioning then return end

        if self.transitionType == "fade" then
            local alpha = self.transitionProgress
            if alpha > 0.5 then alpha = 1 - alpha end
            alpha = alpha * 2

            -- Use bestow.ui to update a fade overlay element's opacity
            local fadeEl = bestow.ui.getElementById(self.fadeDoc, "fade-overlay")
            if fadeEl then
                bestow.ui.setStyle(fadeEl, "opacity", tostring(alpha))
            end
        end
    end
}
```

## Best Practices

1. **Keep states independent** - Each state manages its own lifecycle
2. **Use stack for overlays** - Pause menus, dialogs, etc.
3. **Use replace for navigation** - Menu → Gameplay → Game Over
4. **Implement onEnter/onExit** - Setup and teardown
5. **Handle input in current state only** - Don't leak to states below
6. **Render from bottom up** - For transparency effects
7. **Add transition effects** - Fades feel polished
