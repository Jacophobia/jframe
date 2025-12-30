-- config/input.lua
-- Input bindings for the animation showcase

return {
    -- Camera controls (Dvorak-friendly)
    camera = {
        rotateLeft = { "a", "," },   -- Dvorak: , is in A position
        rotateRight = { "e", "o" },  -- Dvorak: O is in E position
        zoomIn = { "w", "." },       -- Dvorak: . is in W position (sort of)
        zoomOut = { "s" }
    },

    -- Animation controls
    animation = {
        toggleAutoPlay = { "p" },
        toggleRootMotion = { "r" },
        resetPosition = { "home" }
    },

    -- Manual state controls (number keys)
    states = {
        ["1"] = { state = "idle", speed = 0.0 },
        ["2"] = { state = "walk", speed = 0.3 },
        ["3"] = { state = "jog", speed = 0.6 },
        ["4"] = { state = "run", speed = 0.9 },
        ["space"] = { action = "jump" }
    },

    -- Camera rotation speed (degrees per second)
    cameraRotationSpeed = 60.0,

    -- Camera zoom speed
    cameraZoomSpeed = 50.0
}
