-- traits/health.lua
-- Reusable health component trait factory

return function(max, opts)
    opts = opts or {}
    return {
        current = opts.initial or max,
        maximum = max,
        invincibilityDuration = opts.invincibilityDuration or 1.0
    }
end
