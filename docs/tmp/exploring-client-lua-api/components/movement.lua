--[[
    Movement Component

    Data component for entities that can move.
    Movement behavior is handled by the Movement system.

    State Pattern:
    - self.foo = saved to disk
    - self.transient.foo = not saved
]]

return {
    init = function(self, scope)
        -- Saved state
        if self.speedMultiplier == nil then
            self.speedMultiplier = 1.0
        end

        -- Transient state (runtime)
        self.transient.targetPosition = nil  -- For AI pathfinding
        self.transient.velocity = { x = 0, y = 0, z = 0 }
        self.transient.isGrounded = true
    end,

    -- No update needed - Movement system handles movement logic
}
