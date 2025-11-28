-- blueprints/enemies/slime.lua
-- Slime enemy - extends base enemy

local P = dofile("data/blueprints/_base.lua")
local sprite = dofile("data/traits/animated_sprite.lua")

-- Register base enemy first
local baseEnemy = dofile("data/blueprints/enemies/base.lua")
P.register("enemies/base", baseEnemy)

-- Extend base enemy with slime-specific traits
local slime = P.extend("enemies/base", {
    sprite = sprite("data/textures/enemyWalking_1.png", {
        idle = { frames = {0}, duration = 0.1, looping = true }
    }, {
        frameWidth = 32,
        frameHeight = 44,
        columns = 1,
        rows = 1
    }),

    -- Override patrol behavior for slower movement
    patrol = {
        range = 80,
        speed = 30
    }
})

return slime
