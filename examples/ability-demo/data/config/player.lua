-- Player configuration for GAS Demo
-- All player tuning values should be defined here, NOT in C++

return {
    -- Movement parameters
    movement = {
        speed = 200.0,        -- Horizontal movement speed (pixels/sec)
        jumpForce = 400.0,    -- Vertical impulse when jumping (negative Y)
    },

    -- Physics body dimensions
    physics = {
        width = 30,
        height = 50,
        fixedRotation = true,
        linearDamping = 0.0,
        friction = 0.0,  -- Zero friction prevents wall sticking
    },

    -- Initial attribute values (used by GAS)
    attributes = {
        health = 100,
        stamina = 100,
        moveSpeed = 200,
    },

    -- Stamina regeneration
    staminaRegen = 20.0,  -- Per second

    -- Visual settings
    render = {
        defaultColor = { r = 0, g = 255, b = 0 },  -- Green
        dashingColor = { r = 0, g = 255, b = 255 },  -- Cyan
        stunnedColor = { r = 128, g = 0, b = 128 },  -- Purple
        regenColor = { r = 0, g = 255, b = 0 },      -- Bright green
    },
}
