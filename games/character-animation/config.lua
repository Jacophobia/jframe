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
        acceleration = 1.5,     -- How quickly speed builds up (slower = cleaner animation transitions)
        deceleration = 1.5,     -- How quickly character stops (matches acceleration for symmetry)
        turnSpeed = 5.0,        -- Rotation speed (radians per second) - smoother turning
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
        -- Note: Blend times should be shorter than the time between state changes
        -- to prevent overlapping crossfades
        blendTimes = {
            locomotion = 0.3,       -- idle <-> walk <-> jog <-> run (longer for smoother overlap)
            jumpStart = 0.1,        -- Grounded -> jumping
            jumpToFall = 0.1,       -- Jumping -> falling
            land = 0.1,             -- Falling -> landing
            landToRecovery = 0.1,   -- Landing -> recovery
            recoveryToIdle = 0.2,   -- Recovery -> idle/walk
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

    -- Camera settings (Elden Ring style)
    camera = {
        distance = 2.67,        -- Distance from character (1/3 of original 8.0)
        height = 1.5,           -- Height above character (proportionally reduced)
        lookAhead = 0.5,        -- Look ahead in movement direction (closer camera needs less)
        smoothing = 5.0,        -- Position smoothing factor
        orbitSmoothing = 0.4,   -- Orbit rotation smoothing (lower = slower/smoother)
        followDelay = 2.0,      -- Seconds before camera starts rotating to follow
        moveThreshold = 0.3,    -- Speed threshold to consider player "moving"
        -- Manual camera control
        manualControlCooldown = 2.0,  -- Seconds after manual input before auto-follow resumes
        mouseSensitivity = 0.003,     -- Mouse look sensitivity
        stickSensitivity = 2.5,       -- Right stick look sensitivity
    },

    -- Character physics settings
    character = {
        radius = 0.3,           -- Capsule radius
        height = 1.8,           -- Capsule height
        stepHeight = 0.35,      -- Max step height to climb
        maxSlopeAngle = 45.0,   -- Max walkable slope angle
        mass = 80.0,            -- Character mass in kg
    },
}
