-- Scene Manager Example
-- Demonstrates a complete game flow with menu, gameplay, pause, and game over scenes.

-- main.lua
return {
    title = "Scene Manager Demo",
    width = 1280,
    height = 720,

    init = function()
        app.main.state = { score = 0 }

        -- Register all scenes upfront
        bestow.scene.register("main_menu", "scenes/main_menu.lua")
        bestow.scene.register("gameplay", "scenes/gameplay.lua")
        bestow.scene.register("pause", "scenes/pause.lua")
        bestow.scene.register("game_over", "scenes/game_over.lua")

        -- Set up input actions
        local k = KeyCode
        bestow.action.builder():duringPhase("mainmenu"):whenPressed(k.Space):emitAction("StartGame"):discretely()
        bestow.action.builder():duringPhase("gameplay"):whenPressed(k.Escape):emitAction("Pause"):discretely()
        bestow.action.builder():duringPhase("pause"):whenPressed(k.Escape):emitAction("Resume"):discretely()

        -- Dvorak + QWERTY movement
        bestow.action.builder():duringPhase("gameplay"):whenActive(k.Comma):emitAction("MoveForward"):continuously()
        bestow.action.builder():duringPhase("gameplay"):whenActive(k.W):emitAction("MoveForward"):continuously()
        bestow.action.builder():duringPhase("gameplay"):whenActive(k.O):emitAction("MoveBack"):continuously()
        bestow.action.builder():duringPhase("gameplay"):whenActive(k.S):emitAction("MoveBack"):continuously()

        -- Subscribe to input events
        bestow.events.subscribe("action:StartGame", function()
            bestow.scene.replace("gameplay")
        end)
        bestow.events.subscribe("action:Pause", function()
            bestow.scene.push("pause")
        end)
        bestow.events.subscribe("action:Resume", function()
            bestow.scene.pop()
        end)

        -- Start with main menu
        bestow.scene.push("main_menu")
    end,

    update = function(dt)
        bestow.timer.update(dt)
        -- Keep running as long as there's a scene on the stack
        return bestow.scene.active() ~= nil
    end,

    render = function()
        -- Rendering is handled by the active scene's render() callback
    end,

    run = function()
        app.main.init()
        while app.main.update(bestow.core.deltaTime()) do
            app.main.render()
        end
    end
}

-- scenes/main_menu.lua (separate file)
--[[
return {
    phase = "mainmenu",
    ui = { "ui/main_menu.rml" },

    enter = function()
        bestow.debug("Main menu entered")
    end,

    update = function(dt)
        return true
    end,

    render = function()
        bestow.graphics3d.beginFrame()
        bestow.graphics3d.endFrame()
    end
}
]]

-- scenes/gameplay.lua (separate file)
--[[
return {
    phase = "gameplay",
    ui = { "ui/gameplay_hud.rml" },

    enter = function(params)
        local state = app.main.state
        if params and params.restart then
            state.score = 0
        end
        bestow.debug("Gameplay entered, score:", state.score)
    end,

    update = function(dt)
        local state = app.main.state
        app.systems.player.update(dt)

        -- Game over condition
        if state.health and state.health <= 0 then
            bestow.scene.replace("game_over", { finalScore = state.score })
            return true
        end
        return true
    end,

    render = function()
        bestow.graphics3d.beginFrame()
        -- Draw game world
        bestow.graphics3d.endFrame()
    end,

    exit = function()
        bestow.debug("Gameplay exited")
    end
}
]]

-- scenes/pause.lua (separate file)
--[[
return {
    phase = "pause",
    ui = { "ui/pause_menu.rml" },

    enter = function()
        bestow.debug("Game paused")
    end,

    update = function(dt)
        -- Pause menu doesn't need to update game logic
        return true
    end,

    render = function()
        -- Could render a dim overlay
        bestow.graphics3d.beginFrame()
        bestow.graphics3d.endFrame()
    end
}
]]

-- scenes/game_over.lua (separate file)
--[[
return {
    phase = "mainmenu",
    ui = { "ui/game_over.rml" },

    enter = function(params)
        if params then
            bestow.debug("Game over! Final score:", params.finalScore)
        end
    end,

    update = function(dt)
        return true
    end,

    render = function()
        bestow.graphics3d.beginFrame()
        bestow.graphics3d.endFrame()
    end
}
]]
