-- blueprints/platform.lua
-- Platform entity blueprint

local physics = dofile("data/traits/physics_body.lua")
local sprite = dofile("data/traits/animated_sprite.lua")

return {
    type = "platform",
    tags = { "platform" },

    physics = physics({
        type = "static",
        width = 100,  -- default, overridden per instance
        height = 30,  -- default, overridden per instance
        density = 0,
        friction = 0.5
    }),

    sprite = sprite("data/textures/block.png", {}, {
        frameWidth = 226,
        frameHeight = 148,
        columns = 1,
        rows = 1
    })
}
