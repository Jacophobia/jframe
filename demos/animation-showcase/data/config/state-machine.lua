-- config/state-machine.lua
-- Character locomotion state machine definition

-- State IDs (must match C++ AnimState enum for now)
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
    name = "CharacterLocomotion",

    -- Parameter definitions
    parameters = {
        Speed = { type = "float", default = 0.0 },
        IsGrounded = { type = "bool", default = true },
        Jump = { type = "trigger" }
    },

    -- State definitions
    states = {
        [States.Idle] = {
            name = "idle",
            clip = "idle",
            wrapMode = "loop",
            blendTime = 0.8
        },
        [States.Walk] = {
            name = "walk",
            clip = "walk",
            wrapMode = "loop",
            blendTime = 0.8
        },
        [States.Jog] = {
            name = "jog",
            clip = "jog",
            wrapMode = "loop",
            blendTime = 0.6
        },
        [States.Run] = {
            name = "run",
            clip = "run",
            wrapMode = "loop",
            blendTime = 0.4
        },
        [States.Jump] = {
            name = "jump",
            clip = "jump",
            wrapMode = "once",
            blendTime = 0.25
        },
        [States.Falling] = {
            name = "falling",
            clip = "falling",
            wrapMode = "loop",
            blendTime = 0.3
        },
        [States.Landing] = {
            name = "landing",
            clip = "landing",
            wrapMode = "once",
            blendTime = 0.25
        },
        [States.Recovery] = {
            name = "recovery",
            clip = "recovery",
            wrapMode = "once",
            blendTime = 0.4
        }
    },

    -- Default starting state
    defaultState = States.Idle,

    -- Transition definitions
    transitions = {
        -- Idle to locomotion
        { from = States.Idle, to = States.Walk, conditions = {
            { param = "Speed", op = ">", value = 0.1 },
            { param = "Speed", op = "<", value = 0.4 },
            { param = "IsGrounded", op = "==", value = true }
        }, blendTime = 0.3, priority = 1 },

        { from = States.Idle, to = States.Jog, conditions = {
            { param = "Speed", op = ">=", value = 0.4 },
            { param = "Speed", op = "<", value = 0.7 },
            { param = "IsGrounded", op = "==", value = true }
        }, blendTime = 0.3, priority = 2 },

        { from = States.Idle, to = States.Run, conditions = {
            { param = "Speed", op = ">=", value = 0.7 },
            { param = "IsGrounded", op = "==", value = true }
        }, blendTime = 0.3, priority = 3 },

        -- Walk transitions
        { from = States.Walk, to = States.Idle, conditions = {
            { param = "Speed", op = "<=", value = 0.1 },
            { param = "IsGrounded", op = "==", value = true }
        }, blendTime = 0.4, priority = 1 },

        { from = States.Walk, to = States.Jog, conditions = {
            { param = "Speed", op = ">=", value = 0.4 },
            { param = "IsGrounded", op = "==", value = true }
        }, blendTime = 0.3, priority = 2 },

        -- Jog transitions
        { from = States.Jog, to = States.Walk, conditions = {
            { param = "Speed", op = "<", value = 0.4 },
            { param = "Speed", op = ">", value = 0.1 },
            { param = "IsGrounded", op = "==", value = true }
        }, blendTime = 0.3, priority = 1 },

        { from = States.Jog, to = States.Run, conditions = {
            { param = "Speed", op = ">=", value = 0.7 },
            { param = "IsGrounded", op = "==", value = true }
        }, blendTime = 0.3, priority = 2 },

        { from = States.Jog, to = States.Idle, conditions = {
            { param = "Speed", op = "<=", value = 0.1 },
            { param = "IsGrounded", op = "==", value = true }
        }, blendTime = 0.5, priority = 0 },

        -- Run transitions
        { from = States.Run, to = States.Jog, conditions = {
            { param = "Speed", op = "<", value = 0.7 },
            { param = "Speed", op = ">=", value = 0.4 },
            { param = "IsGrounded", op = "==", value = true }
        }, blendTime = 0.3, priority = 1 },

        { from = States.Run, to = States.Idle, conditions = {
            { param = "Speed", op = "<=", value = 0.1 },
            { param = "IsGrounded", op = "==", value = true }
        }, blendTime = 0.5, priority = 0 },

        -- Jump (from any grounded state)
        { from = -1, to = States.Jump, conditions = {  -- -1 = any state
            { param = "Jump", op = "triggered" },
            { param = "IsGrounded", op = "==", value = true }
        }, blendTime = 0.1, priority = 10 },

        -- Jump to Falling
        { from = States.Jump, to = States.Falling, conditions = {
            { param = "IsGrounded", op = "==", value = false }
        }, blendTime = 0.2, priority = 1, onAnimationEnd = true },

        -- Falling to Landing
        { from = States.Falling, to = States.Landing, conditions = {
            { param = "IsGrounded", op = "==", value = true }
        }, blendTime = 0.15, priority = 1 },

        -- Landing to Recovery
        { from = States.Landing, to = States.Recovery, conditions = {},
          blendTime = 0.25, priority = 1, onAnimationEnd = true },

        -- Recovery to Idle
        { from = States.Recovery, to = States.Idle, conditions = {
            { param = "IsGrounded", op = "==", value = true }
        }, blendTime = 0.4, priority = 1, onAnimationEnd = true }
    }
}
