-- template/data/config/game.lua
-- Game Configuration
-- All game tuning values should be defined here, not in C++ code

return {
    -- Window settings (used by main.cpp)
    window = {
        width = 800,
        height = 600,
        title = "JFrame Template Game",
        vsync = true
    },

    -- Player configuration
    player = {
        -- Movement parameters
        moveSpeed = 200.0,      -- Horizontal movement speed (pixels/second)
        jumpForce = 400.0,      -- Vertical jump impulse (negative Y)

        -- Physics body dimensions
        physics = {
            width = 30,         -- Player width (pixels)
            height = 50,        -- Player height (pixels)
            fixedRotation = true,   -- Prevent rotation
            linearDamping = 0.0,    -- Air resistance
            friction = 0.0          -- Surface friction (0 = no wall sticking)
        },

        -- Visual settings
        color = {
            r = 50,
            g = 200,
            b = 50,
            a = 255
        }
    },

    -- Camera settings
    camera = {
        smoothing = 0.1,        -- 0.0 = instant snap, 0.9 = very smooth
        offsetX = 0.0,          -- Camera offset from player (X)
        offsetY = -50.0,        -- Camera offset from player (Y, negative = look ahead)
    },

    -- Physics world settings
    physics = {
        gravity = 980.0,        -- Gravity acceleration (pixels/second^2)
        velocityIterations = 8,
        positionIterations = 3
    },

    -- Game rules
    gameplay = {
        maxHealth = 100,
        maxStamina = 100,
        staminaRegenRate = 20.0,    -- Per second
    }
}
