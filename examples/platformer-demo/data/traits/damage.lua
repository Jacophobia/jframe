-- traits/damage.lua
-- Reusable damage trait factory

return function(amount, opts)
    opts = opts or {}
    return {
        amount = amount or 10,
        knockback = opts.knockback or 0,
        damageType = opts.damageType or "contact"
    }
end
