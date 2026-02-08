-- Automated scene navigation test
-- Verifies that all 5 scenes can be pushed/popped correctly.
-- Uses a frame counter to step through test actions at time thresholds.

local STEP_TIME = 2.0   -- seconds between steps (long enough for screenshots)
local elapsed = 0.0
local step = 0
local passed = 0
local total = 9

local function check(label, condition)
    if condition then
        passed = passed + 1
        print("[PASS] " .. label)
    else
        print("[FAIL] " .. label)
    end
end

return {
    init = function()
        app.systems.menu_nav.init()

        app.main.state = {
            running = true,
            settings = {
                windowMode   = "borderless",
                vsync        = true,
                renderScale  = 1.0,
                masterVolume = 0.8,
                musicVolume  = 0.7,
                sfxVolume    = 1.0,
                uiVolume     = 0.8,
                shadows      = true,
                ssao         = true,
                bloom        = true,
            },
        }

        bestow.scene.register("main_menu", "scenes/main_menu.lua")
        bestow.scene.register("play",      "scenes/play.lua")
        bestow.scene.register("pause",     "scenes/pause.lua")
        bestow.scene.register("controls",  "scenes/controls.lua")
        bestow.scene.register("settings",  "scenes/settings.lua")

        bestow.scene.push("main_menu")
        print("[TEST] Scene navigation test started")
    end,

    update = function(dt)
        elapsed = elapsed + dt

        local target = (step + 1) * STEP_TIME
        if elapsed < target then
            return true
        end

        step = step + 1

        if step == 1 then
            -- Step 1: Verify main_menu is active
            check("1. main_menu active", bestow.scene.active() == "main_menu")
            bestow.scene.push("settings")

        elseif step == 2 then
            -- Step 2: Verify settings is active
            check("2. settings active", bestow.scene.active() == "settings")
            bestow.scene.pop()

        elseif step == 3 then
            -- Step 3: Back to main_menu
            check("3. main_menu after settings pop", bestow.scene.active() == "main_menu")
            bestow.scene.push("controls")

        elseif step == 4 then
            -- Step 4: Verify controls is active
            check("4. controls active", bestow.scene.active() == "controls")
            bestow.scene.pop()

        elseif step == 5 then
            -- Step 5: Back to main_menu
            check("5. main_menu after controls pop", bestow.scene.active() == "main_menu")
            bestow.scene.push("play")

        elseif step == 6 then
            -- Step 6: Verify play is active
            check("6. play active", bestow.scene.active() == "play")
            bestow.scene.push("pause")

        elseif step == 7 then
            -- Step 7: Verify pause is active
            check("7. pause active", bestow.scene.active() == "pause")
            bestow.scene.pop()

        elseif step == 8 then
            -- Step 8: Back to play
            check("8. play after pause pop", bestow.scene.active() == "play")
            bestow.scene.clear()
            bestow.scene.push("main_menu")

        elseif step == 9 then
            -- Step 9: Back to main_menu after clear
            check("9. main_menu after clear+push", bestow.scene.active() == "main_menu")
            print(string.format("[RESULT] %d/%d passed", passed, total))
            if passed == total then
                print("[SUCCESS] All scene navigation tests passed!")
            else
                print("[FAILURE] Some tests failed.")
            end
            return false  -- quit
        end

        return true
    end,

    render = function()
    end,
}
