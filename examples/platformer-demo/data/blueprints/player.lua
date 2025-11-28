-- blueprints/player.lua
-- Player entity blueprint using traits

local physics = dofile("data/traits/physics_body.lua")
local health = dofile("data/traits/health.lua")
local sprite = dofile("data/traits/animated_sprite.lua")

return {
    type = "player",
    tags = { "player" },

    physics = physics({
        type = "dynamic",
        width = 50,
        height = 80,
        fixedRotation = true,
        density = 1.0,
        friction = 0.0,
        restitution = 0.0
    }),

    health = health(100),

    sprite = sprite("data/textures/player_spritesheet.png", {
        idle = { frames = {0, 1, 2, 3}, duration = 0.1, looping = true },
        run = { frames = {9, 10, 11, 12, 13, 14}, duration = 0.08, looping = true },
        jump = { frames = {18}, duration = 0.1, looping = false },
        fall = { frames = {27}, duration = 0.1, looping = false }
    }, {
        frameWidth = 66,
        frameHeight = 92,
        columns = 9,
        rows = 7
    }),

    movement = {
        speed = 400,
        maxJumps = 2,
        jumpForce = 800
    },

    score = {
        initial = 0
    }
}
