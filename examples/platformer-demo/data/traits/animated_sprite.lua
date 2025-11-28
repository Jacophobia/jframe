-- traits/animated_sprite.lua
-- Reusable animated sprite trait factory

return function(texturePath, animations, opts)
    opts = opts or {}
    return {
        texture = texturePath,
        frameWidth = opts.frameWidth or 32,
        frameHeight = opts.frameHeight or 32,
        columns = opts.columns or 1,
        rows = opts.rows or 1,
        padding = opts.padding or 0,
        animations = animations or {},
        defaultAnimation = opts.defaultAnimation or "idle"
    }
end
