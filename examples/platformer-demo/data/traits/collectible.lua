-- traits/collectible.lua
-- Reusable collectible trait factory

return function(scoreValue, opts)
    opts = opts or {}
    return {
        value = scoreValue or 10,
        sound = opts.sound or "data/audio/sfx/pepSound1.ogg",
        destroyOnCollect = opts.destroyOnCollect ~= false  -- default true
    }
end
