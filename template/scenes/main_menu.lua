-- Main Menu scene
-- Buttons: Play, Controls, Settings, Quit

return {
    phase = "menu",

    enter = function(params)
        local nav = app.systems.menu_nav
        local doc = bestow.ui.loadDocument("assets/ui/main_menu.rml")
        if not doc then return end
        bestow.ui.showDocument(doc)

        -- Wire button click handlers
        local btnPlay = bestow.ui.getElementById(doc, "btn-play")
        local btnCtrl = bestow.ui.getElementById(doc, "btn-controls")
        local btnOpts = bestow.ui.getElementById(doc, "btn-settings")
        local btnQuit = bestow.ui.getElementById(doc, "btn-quit")

        if btnPlay then bestow.ui.onElementEvent(btnPlay, "click", function()
            bestow.scene.push("play")
        end) end
        if btnCtrl then bestow.ui.onElementEvent(btnCtrl, "click", function()
            bestow.scene.push("controls")
        end) end
        if btnOpts then bestow.ui.onElementEvent(btnOpts, "click", function()
            bestow.scene.push("settings")
        end) end
        if btnQuit then bestow.ui.onElementEvent(btnQuit, "click", function()
            app.main.state.running = false
        end) end

        -- Set up keyboard/gamepad navigation
        nav.setItems(doc, {"btn-play", "btn-controls", "btn-settings", "btn-quit"})
        nav.focus("btn-play")
        nav.onBack = nil  -- No back action from main menu

        -- Subscribe to confirm events for keyboard/gamepad
        bestow.events.subscribe("menu:confirm", function(data)
            if data.id == "btn-play"         then bestow.scene.push("play")
            elseif data.id == "btn-controls" then bestow.scene.push("controls")
            elseif data.id == "btn-settings" then bestow.scene.push("settings")
            elseif data.id == "btn-quit"     then app.main.state.running = false
            end
        end)
    end,

    update = function(dt)
        return true
    end,

    exit = function()
        app.systems.menu_nav.clear()
    end,
}
