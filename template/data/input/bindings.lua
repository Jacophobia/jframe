-- template/data/input/bindings.lua
-- Input Action Bindings
-- Maps logical actions to physical keys/buttons
-- (This file is for reference - input is currently configured in Game.cpp)

return {
    actions = {
        -- Movement actions
        move_left = {
            keys = {"A", "Left"},
            buttons = {"DPadLeft"},
            axis = {axis = "LeftStickX", threshold = -0.5}
        },

        move_right = {
            keys = {"D", "Right"},
            buttons = {"DPadRight"},
            axis = {axis = "LeftStickX", threshold = 0.5}
        },

        -- Jump action
        jump = {
            keys = {"Space", "W", "Up"},
            buttons = {"A"}  -- Xbox A / PlayStation X
        },

        -- Future actions (example)
        attack = {
            keys = {"F"},
            buttons = {"X"}  -- Xbox X / PlayStation Square
        },

        pause = {
            keys = {"Escape"},
            buttons = {"Start"}
        }
    }
}
