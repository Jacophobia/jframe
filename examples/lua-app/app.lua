-- app.lua
-- Example Lua-first application entry point
-- This is the main entry point for a Lua-driven Bestow game

return {
    -- Application metadata
    name = "Example Lua Game",
    version = "1.0.0",

    -- Asset directories (auto-loaded)
    blueprints = "blueprints/",
    behaviors = "behaviors/",
    systems = "systems/",

    -- Window configuration
    window = {
        title = "Lua Game Example",
        width = 1280,
        height = 720,
        vsync = true,
        fullscreen = false
    },

    -- Game configuration
    game = {
        gravity = -980.0,
        playerSpeed = 200.0,
        jumpForce = 400.0
    },

    -- Called after all systems are initialized
    init = function()
        print("App initialized!")

        -- Spawn the player at the center of the screen
        local player = engine.spawn("Player", 640, 360)
        if player then
            print("Player spawned at position: " .. player:getPosition().x .. ", " .. player:getPosition().y)
            player:setName("MainPlayer")
        end

        -- Spawn some enemies
        for i = 1, 3 do
            local enemy = engine.spawn("Enemy", 200 + i * 200, 360)
            if enemy then
                enemy:setTag("enemy")
            end
        end

        -- Set up keyboard bindings using the event system
        events.on("keyPressed", function(key)
            if key == "escape" then
                print("Escape pressed - quitting")
                -- engine.quit()
            end
        end)
    end,

    -- Optional: called each frame (alternative to Lua systems)
    update = function(dt)
        -- Game-level update logic can go here
    end
}
