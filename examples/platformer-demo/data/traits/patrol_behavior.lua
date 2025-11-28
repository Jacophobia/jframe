-- traits/patrol_behavior.lua
-- Reusable patrol behavior trait factory

return function(range, speed, opts)
    opts = opts or {}
    return {
        range = range or 100,
        speed = speed or 50,
        startDirection = opts.startDirection or "right"
    }
end
