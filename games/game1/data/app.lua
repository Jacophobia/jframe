-- app.lua
-- 3D Isometric Snake Game entry point
-- Demonstrates the Lua-first approach for game configuration

return {
    name = "Snake Game",
    version = "1.0.0",

    -- Window configuration
    window = {
        title = "Snake Game - Bestow Demo",
        width = 1280,
        height = 720,
        vsync = true,
        fullscreen = false
    },

    -- Graphics configuration
    graphics = {
        clearColor = {30, 35, 45, 255},  -- Dark blue-gray
        ambientLight = {
            color = {0.8, 0.85, 0.9},
            intensity = 0.6
        },
        directionalLight = {
            direction = {-0.5, -0.8, 0.3},
            color = {1.0, 0.98, 0.9},
            intensity = 1.2
        }
    },

    -- Game settings
    game = {
        initialGridSize = 10,
        initialMoveInterval = 0.15,
        minMoveInterval = 0.05,
        expandRate = 2,
        cameraDistance = 8.0,
        cameraHeight = 10.0
    },

    -- Called after initialization
    init = function()
        print("Snake Game initialized")
    end
}
