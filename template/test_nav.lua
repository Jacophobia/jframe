-- Visual test: auto-navigate through all 5 scenes
-- Run with: bestow run test_nav.lua

local elapsed = 0
local step = 0
local HOLD = 5.0  -- seconds to hold each scene (peekaboo needs time)

-- Each step: push a scene, hold it for HOLD seconds
-- Steps: 0=main_menu, 1=settings, 2=controls, 3=play, 4=pause, 5=quit
return {
    init = function()
        app.systems.menu_nav.init()
        app.main.state = {
            running = true,
            settings = {
                windowMode = "borderless", vsync = true, renderScale = 1.0,
                masterVolume = 0.8, musicVolume = 0.7, sfxVolume = 1.0, uiVolume = 0.8,
                shadows = true, ssao = true, bloom = true,
            },
        }
        bestow.scene.register("main_menu", "scenes/main_menu.lua")
        bestow.scene.register("play",      "scenes/play.lua")
        bestow.scene.register("pause",     "scenes/pause.lua")
        bestow.scene.register("controls",  "scenes/controls.lua")
        bestow.scene.register("settings",  "scenes/settings.lua")
        bestow.scene.push("main_menu")
    end,

    update = function(dt)
        elapsed = elapsed + dt
        local next = math.floor(elapsed / HOLD)
        if next > step then
            step = next
            if step == 1 then
                -- Switch to settings
                bestow.scene.clear()
                bestow.scene.push("settings")
            elseif step == 2 then
                -- Switch to controls
                bestow.scene.clear()
                bestow.scene.push("controls")
            elseif step == 3 then
                -- Switch to play
                bestow.scene.clear()
                bestow.scene.push("play")
            elseif step == 4 then
                -- Push pause on top of play
                bestow.scene.push("pause")
            elseif step == 5 then
                return false  -- quit
            end
        end
        return true
    end,

    render = function() end,
}
