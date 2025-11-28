-- Endless Runner Configuration
-- All gameplay values can be tuned here

return {
    -- Game settings
    game = {
        runSpeed = 350.0,       -- Player auto-run speed (pixels/sec)
        sectionWidth = 800.0,   -- Width of each procedural section
        groundY = 550.0,        -- Y position of ground level
    },

    -- Player settings
    player = {
        jumpForce = 750.0,      -- Jump impulse
    },

    -- Physics settings
    physics = {
        gravity = 980.0,
        player = {
            width = 50.0,
            height = 80.0,
        },
    },

    -- Camera settings
    camera = {
        zoom = 1.0,
        followSmoothing = 0.0,  -- 0 = instant follow (no lag)
    },
}
