-- app.lua
-- Animation Showcase entry point
-- Demonstrates the Lua-first approach for configuring the demo

return {
    name = "Animation Showcase",
    version = "1.0.0",

    -- Window and graphics configuration
    window = {
        title = "Animation Showcase - Bestow Demo",
        width = 1280,
        height = 720,
        vsync = true,
        fullscreen = false
    },

    graphics = {
        clearColor = {30, 35, 45, 255},  -- Dark blue-gray
        ambientLight = {
            color = {0.8, 0.85, 0.9},
            intensity = 0.6
        },
        directionalLight = {
            direction = {-0.3, -0.7, -0.4},
            color = {1.0, 0.98, 0.95},
            intensity = 1.2
        }
    },

    -- Camera configuration
    camera = {
        distance = 300.0,
        height = 100.0,
        targetY = 100.0,
        angle = 0.0,
        fovY = 45.0,
        nearPlane = 1.0,
        farPlane = 1000.0
    },

    -- Character model
    character = {
        model = ":library:/characters/test-character.fbx",
        position = {0.0, 0.0, 0.0},
        scale = 1.0
    },

    -- Ground plane
    ground = {
        size = 500.0,
        material = {
            albedo = {0.2, 0.25, 0.2},
            roughness = 0.8,
            metallic = 0.0
        }
    },

    -- Debug visualization
    debug = {
        showBones = true,
        boneColor = {1.0, 1.0, 0.0, 1.0},
        boneScale = 3.0
    },

    -- Called after initialization
    init = function()
        print("Animation Showcase initialized")
    end
}
