--[[
    Configuration for Character Animation Demo

    Defines movement speeds, animation thresholds, and blend times.
    These values are tuned to match the animation clips from the asset library.
]]

return {
    -- Window settings
    window = {
        width = 1280,
        height = 720,
    },

    -- Movement settings
    movement = {
        walkSpeed = 2.5,        -- Units per second when walking
        jogSpeed = 4.5,         -- Units per second when jogging
        runSpeed = 7.0,         -- Units per second when running
        acceleration = 12.0,    -- How quickly speed builds up
        deceleration = 10.0,    -- How quickly character stops
        turnSpeed = 8.0,        -- Rotation speed (radians per second)
    },

    -- Jump/physics settings
    physics = {
        jumpForce = 8.0,        -- Initial upward velocity
        gravity = -20.0,        -- Gravity acceleration
        groundY = 0.0,          -- Ground plane Y position
    },

    -- Animation state machine thresholds
    -- Speed is normalized 0-1 where 1 = running speed
    animation = {
        -- Speed thresholds for state transitions
        idleThreshold = 0.05,       -- Below this = idle
        walkThreshold = 0.35,       -- Below this = walk, above = jog
        jogThreshold = 0.75,        -- Below this = jog, above = run

        -- Blend times for smooth transitions (seconds)
        blendTimes = {
            locomotion = 0.3,       -- idle <-> walk <-> jog <-> run
            jumpStart = 0.15,       -- Grounded -> jumping
            jumpToFall = 0.1,       -- Jumping -> falling
            land = 0.1,             -- Falling -> landing
            landToRecovery = 0.1,   -- Landing -> recovery
            recoveryToIdle = 0.3,   -- Recovery -> idle/walk
        },
    },

    -- Asset paths (using :library:/ for shared assets)
    assets = {
        character = ":library:/characters/test-character.fbx",
        animations = {
            idle = ":library:/animations/idle.fbx",
            walk = ":library:/animations/walk.fbx",
            jog = ":library:/animations/jog.fbx",
            run = ":library:/animations/run.fbx",
            jump = ":library:/animations/jumping-up.fbx",
            falling = ":library:/animations/falling.fbx",
            landing = ":library:/animations/landing.fbx",
            landingRecovery = ":library:/animations/landing-recovery.fbx",
        },
    },

    -- Camera settings
    camera = {
        distance = 8.0,         -- Distance from character
        height = 4.0,           -- Height above character
        lookAhead = 2.0,        -- Look ahead in movement direction
        smoothing = 5.0,        -- Camera follow smoothing factor
    },
}
