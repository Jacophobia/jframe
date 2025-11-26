-- data/blueprints/helpers.lua
-- Shared blueprint utilities

local C = {}

-- Reusable physics body configurations
function C.dynamicBody(opts)
    return {
        bodyType = "dynamic",
        width = opts.width or 32,
        height = opts.height or 32,
        density = opts.density or 1.0,
        friction = opts.friction or 0.3,
        restitution = opts.restitution or 0.0,
        fixedRotation = opts.fixedRotation ~= false,
        categoryBits = opts.categoryBits or 0x0001,
        maskBits = opts.maskBits or 0xFFFF
    }
end

function C.staticBody(opts)
    local body = C.dynamicBody(opts)
    body.bodyType = "static"
    return body
end

function C.sensor(opts)
    local body = C.dynamicBody(opts)
    body.isSensor = true
    return body
end

-- Collision layer constants
C.Layers = {
    PLAYER     = 0x0001,
    ENEMY      = 0x0002,
    PROJECTILE = 0x0004,
    TERRAIN    = 0x0008,
    TRIGGER    = 0x0010,
    ITEM       = 0x0020
}

return C
