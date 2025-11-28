-- traits/physics_body.lua
-- Reusable physics body trait factory

return function(opts)
    opts = opts or {}
    return {
        type = opts.type or "dynamic",
        size = {
            width = opts.width or 32,
            height = opts.height or 32
        },
        fixedRotation = opts.fixedRotation ~= false,  -- default true
        density = opts.density or 1.0,
        friction = opts.friction or 0.0,
        restitution = opts.restitution or 0.0,
        isSensor = opts.isSensor or false
    }
end
