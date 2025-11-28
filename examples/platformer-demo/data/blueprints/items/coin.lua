-- blueprints/items/coin.lua
-- Coin collectible blueprint

local physics = dofile("data/traits/physics_body.lua")
local collectible = dofile("data/traits/collectible.lua")
local sprite = dofile("data/traits/animated_sprite.lua")

return {
    type = "collectible",
    tags = { "collectible", "coin" },

    physics = physics({
        type = "static",
        width = 30,
        height = 30,
        isSensor = true
    }),

    collectible = collectible(10, {
        sound = "data/audio/sfx/pepSound1.ogg"
    }),

    sprite = sprite("data/textures/coinGold.png", {
        idle = { frames = {0}, duration = 0.1, looping = true }
    }, {
        frameWidth = 128,
        frameHeight = 128,
        columns = 1,
        rows = 1
    })
}
