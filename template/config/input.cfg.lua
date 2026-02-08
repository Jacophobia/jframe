-- Input action definitions (pure data — no API calls)
-- The engine reads this file during boot and registers ActionBuilder calls in C++.
--
-- Each action entry:
--   action   = string        Action name emitted to the event system
--   phases   = {string,...}  Input phases where this action is active
--   keys     = {string,...}  Keyboard keys (KeyCode names: "Comma", "W", "Space", etc.)
--   buttons  = {string,...}  Gamepad buttons (GamepadButton names: "A", "B", "Start", etc.)
--   axes     = {string,...}  Gamepad axes (GamepadAxis names: "LeftX", "LeftY", etc.)
--   mode     = string        "discrete" (fire once per press) or "continuous" (fire while held)
--   deadzone = number        Analog deadzone for axes (0.0–1.0, default 0.15)

return {
    holdThreshold = 0.3,    -- Seconds before a hold is recognised

    actions = {
        -----------------------------------------------------------------------
        -- Menu / UI navigation (active in menu, pause, controls, settings)
        -----------------------------------------------------------------------
        { action = "MenuUp",      phases = {"menu", "pause", "controls", "settings"},
          keys = {"Comma", "W", "Up"},       buttons = {"DPadUp"},   mode = "discrete" },
        { action = "MenuDown",    phases = {"menu", "pause", "controls", "settings"},
          keys = {"O", "S", "Down"},         buttons = {"DPadDown"}, mode = "discrete" },
        { action = "MenuLeft",    phases = {"menu", "pause", "controls", "settings"},
          keys = {"A", "Left"},              buttons = {"DPadLeft"}, mode = "discrete" },
        { action = "MenuRight",   phases = {"menu", "pause", "controls", "settings"},
          keys = {"E", "D", "Right"},        buttons = {"DPadRight"},mode = "discrete" },
        { action = "MenuConfirm", phases = {"menu", "pause", "controls", "settings"},
          keys = {"Enter", "Space"},         buttons = {"A"},        mode = "discrete" },
        { action = "MenuBack",    phases = {"menu", "pause", "controls", "settings"},
          keys = {"Escape"},                 buttons = {"B"},        mode = "discrete" },

        -----------------------------------------------------------------------
        -- Gameplay movement
        -----------------------------------------------------------------------
        { action = "MoveForward", phases = {"gameplay"},
          keys = {"Comma", "W"},  mode = "continuous" },
        { action = "MoveBack",   phases = {"gameplay"},
          keys = {"O", "S"},      mode = "continuous" },
        { action = "MoveLeft",   phases = {"gameplay"},
          keys = {"A"},           mode = "continuous" },
        { action = "MoveRight",  phases = {"gameplay"},
          keys = {"E", "D"},      mode = "continuous" },

        -- Analog stick movement
        { action = "MoveAxisX",  phases = {"gameplay"},
          axes = {"LeftX"},       mode = "continuous", deadzone = 0.15 },
        { action = "MoveAxisY",  phases = {"gameplay"},
          axes = {"LeftY"},       mode = "continuous", deadzone = 0.15 },

        -----------------------------------------------------------------------
        -- Gameplay actions
        -----------------------------------------------------------------------
        { action = "Jump",   phases = {"gameplay"},
          keys = {"Space"},   buttons = {"A"},        mode = "discrete" },
        { action = "Attack", phases = {"gameplay"},
          keys = {},          buttons = {"X"},         mode = "discrete" },

        -----------------------------------------------------------------------
        -- Pause / resume
        -----------------------------------------------------------------------
        { action = "Pause",  phases = {"gameplay"},
          keys = {"Escape"},  buttons = {"Start"},     mode = "discrete" },
        { action = "Resume", phases = {"pause"},
          keys = {"Escape"},  buttons = {"Start", "B"},mode = "discrete" },
    },
}
