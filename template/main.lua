-- Game entry point
-- The engine reads config/*.cfg.lua files during boot to initialise systems.
-- main.lua registers scenes, sets up game state, and starts.

return {
    init = function()
        ----------------------------------------------------------------
        -- 1. Initialise menu navigation system
        ----------------------------------------------------------------
        app.systems.menu_nav.init()

        ----------------------------------------------------------------
        -- 2. Persistent settings state (survives scene transitions)
        ----------------------------------------------------------------
        app.main.state = {
            running = true,
            settings = {
                fullscreen  = false,
                vsync       = true,
                renderScale = 1.0,
                masterVolume = 0.8,
                musicVolume  = 0.7,
                sfxVolume    = 1.0,
                uiVolume     = 0.8,
                shadows      = true,
                ssao         = true,
                bloom        = true,
            },
        }

        ----------------------------------------------------------------
        -- 3. Register scenes and start
        ----------------------------------------------------------------
        bestow.scene.register("main_menu", "scenes/main_menu.lua")
        bestow.scene.register("play",      "scenes/play.lua")
        bestow.scene.register("pause",     "scenes/pause.lua")
        bestow.scene.register("controls",  "scenes/controls.lua")
        bestow.scene.register("settings",  "scenes/settings.lua")

        bestow.scene.push("main_menu")
    end,

    update = function(dt)
        return app.main.state.running and bestow.scene.active() ~= nil
    end,

    render = function()
        -- Engine handles scene render and UI render automatically.
        -- Add any custom overlay rendering here.
    end,
}
