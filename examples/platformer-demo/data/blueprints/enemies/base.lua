-- blueprints/enemies/base.lua
-- Base enemy blueprint - other enemies extend this

local physics = dofile("data/traits/physics_body.lua")
local patrol = dofile("data/traits/patrol_behavior.lua")
local damage = dofile("data/traits/damage.lua")

return {
    type = "enemy",
    tags = { "enemy" },

    physics = physics({
        type = "dynamic",
        width = 40,
        height = 40,
        fixedRotation = true,
        density = 1.0,
        friction = 0.3
    }),

    patrol = patrol(100, 50),

    damage = damage(20)
}
