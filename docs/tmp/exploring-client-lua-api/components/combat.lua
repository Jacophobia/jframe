--[[
    Combat Component

    Data component for entities that can attack.
    Attack behavior is handled by the Combat system.

    State Pattern:
    - self.foo = saved to disk
    - self.transient.foo = not saved
]]

return {
    init = function(self, scope)
        -- Saved state (persistent stats)
        if self.baseDamage == nil then
            self.baseDamage = 10
            self.attackSpeed = 1.0
            self.attackRange = 1.5
            self.totalDamageDealt = 0
            self.totalKills = 0
        end

        -- Transient state (runtime, handled by Combat system)
        self.transient.cooldownRemaining = 0
        self.transient.isAttacking = false
        self.transient.attackPhase = "none"  -- none, windup, active, recovery
        self.transient.attackTimer = 0
        self.transient.hitEntitiesThisSwing = {}

        -- Attack phase timing (could be saved if upgradeable)
        self.transient.windupDuration = 0.2
        self.transient.activeDuration = 0.3
        self.transient.recoveryDuration = 0.2

        -- Track kills for stats - pattern matches when THIS entity is the killer
        self.transient.killSub = bestow.events.subscribe("EntityDied", {
            killer = scope.entity.id
        }, function(event)
            self.totalKills = self.totalKills + 1
            scope.global.totalKills = (scope.global.totalKills or 0) + 1
        end)
    end,

    destroy = function(self, scope)
        bestow.events.unsubscribe(self.transient.killSub)
    end,

    -- No update needed - Combat system handles attack state machine
    -- Components CAN have update if needed for entity-specific logic
}
