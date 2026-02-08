-- Pause overlay scene
-- Pushed on top of gameplay. Dark backdrop, three buttons: Resume, Main Menu, Quit.

local doc = nil

return {
    phase = "pause",

    enter = function(params)
        local nav = app.systems.menu_nav
        doc = bestow.ui.loadDocument("assets/ui/pause.rml")
        if not doc then return end
        bestow.ui.showDocument(doc)

        -- Wire button click handlers
        local btnResume = bestow.ui.getElementById(doc, "btn-resume")
        local btnMenu   = bestow.ui.getElementById(doc, "btn-main-menu")
        local btnQuit   = bestow.ui.getElementById(doc, "btn-quit")

        if btnResume then bestow.ui.onElementEvent(btnResume, "click", function()
            bestow.scene.pop()
        end) end
        if btnMenu then bestow.ui.onElementEvent(btnMenu, "click", function()
            bestow.scene.clear()
            bestow.scene.push("main_menu")
        end) end
        if btnQuit then bestow.ui.onElementEvent(btnQuit, "click", function()
            app.main.state.running = false
        end) end

        -- Keyboard/gamepad navigation
        nav.setItems(doc, {"btn-resume", "btn-main-menu", "btn-quit"})
        nav.focus("btn-resume")
        nav.onBack = function() bestow.scene.pop() end

        -- Subscribe to Resume action (Escape / Start while paused)
        bestow.scene.subscribe("action:Resume", function()
            bestow.scene.pop()
        end)

        -- Confirm via keyboard/gamepad
        bestow.scene.subscribe("menu:confirm", function(data)
            if data.id == "btn-resume"    then bestow.scene.pop()
            elseif data.id == "btn-main-menu" then
                bestow.scene.clear()
                bestow.scene.push("main_menu")
            elseif data.id == "btn-quit"  then app.main.state.running = false
            end
        end)

        -- Pause audio
        bestow.audio.pauseAll()
    end,

    update = function(dt)
        return true
    end,

    exit = function()
        app.systems.menu_nav.clear()
        if doc then bestow.ui.hideDocument(doc) end
        doc = nil
        bestow.audio.resumeAll()
    end,
}
