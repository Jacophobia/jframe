-- config/player.lua
-- Player gameplay configuration

return {
    moveSpeed = 400.0,
    health = {
        initial = 100,
        maximum = 100
    },
    initialScore = 0,
    jump = {
        maxJumps = 2,  -- Double jump enabled
        jumpForce = 800.0
    },
    -- Animation state thresholds
    animation = {
        runningThreshold = 10.0,    -- Min X velocity to trigger run animation
        jumpingThreshold = -10.0    -- Min Y velocity (negative = up) to trigger jump
    },
    groundDetection = {
        velocityThreshold = 50.0    -- Max Y velocity to be considered grounded
    },
    spawnFallback = { x = 100.0, y = 400.0 }  -- Used if level has no spawn point
}
