-- config/demo-sequence.lua
-- Auto-play sequence for demonstrating animation states

-- State IDs (must match state-machine.lua)
local States = {
    Idle = 0,
    Walk = 1,
    Jog = 2,
    Run = 3,
    Jump = 4,
    Falling = 5,
    Landing = 6,
    Recovery = 7
}

return {
    -- Whether auto-play starts enabled
    enabled = true,

    -- Demo sequence steps
    -- Each step: state to transition to, duration, speed value, grounded state
    steps = {
        { state = States.Idle,     duration = 2.0, speed = 0.0, grounded = true },
        { state = States.Walk,     duration = 2.0, speed = 0.3, grounded = true },
        { state = States.Jog,      duration = 2.0, speed = 0.6, grounded = true },
        { state = States.Run,      duration = 2.0, speed = 0.9, grounded = true },
        { state = States.Jump,     duration = 0.5, speed = 0.0, grounded = true, jump = true },
        { state = States.Falling,  duration = 0.8, speed = 0.0, grounded = false },
        { state = States.Landing,  duration = 0.5, speed = 0.0, grounded = true },
        { state = States.Recovery, duration = 0.8, speed = 0.0, grounded = true }
    },

    -- Sequence behavior
    loop = true,
    transitionDelay = 0.1  -- Small delay between steps for smoother transitions
}
