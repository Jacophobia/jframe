-- config/animations.lua
-- Animation definitions

return {
    player = {
        idle = {
            name = "idle",
            -- Frame IDs 0-8 (row 0, columns 0-8)
            frameIds = { 0, 1, 2, 3, 4, 5, 6, 7, 8 },
            frameDuration = 0.1,
            looping = true
        },
        run = {
            name = "run",
            -- Frame IDs 9-17 (row 1, columns 0-8)
            frameIds = { 9, 10, 11, 12, 13, 14, 15, 16, 17 },
            frameDuration = 0.08,
            looping = true
        },
        jump = {
            name = "jump",
            -- Frame IDs 18-20 (row 2, columns 0-2)
            frameIds = { 18, 19, 20 },
            frameDuration = 0.1,
            looping = false
        },
        fall = {
            name = "fall",
            -- Frame IDs 27-29 (row 3, columns 0-2)
            frameIds = { 27, 28, 29 },
            frameDuration = 0.1,
            looping = true
        }
    }
}
