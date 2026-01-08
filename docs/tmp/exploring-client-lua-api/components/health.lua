--[[
    Health Component

    Components hold entity-specific state and behavior.
    Systems handle cross-entity concerns.

    State Pattern:
    - self.foo = saved to disk
    - self.transient.foo = not saved (engine pre-creates self.transient = {})
]]

return {
    init = function(self, scope)
        -- Saved state
        if self.current == nil then
            self.current = 100
            self.max = 100
            self.regenRate = 0
            self.regenDelay = 3.0
        end

        -- Transient state (engine pre-creates self.transient = {})
        self.transient.invulnerable = false
        self.transient.timeSinceDamage = 0
        self.transient.lastDamageSource = nil
        self.transient.entityId = scope.entity.id  -- Store for event handler

        -- Event subscription WITH PATTERN MATCHING (hot-reload-safe)
        -- Uses table + method name instead of closure
        -- Only invoked when event.target matches this entity's id
        self.transient.hitSub = bestow.events.subscribe("Hit", {
            target = scope.entity.id
        }, self, "onHit")
    end,

    -- Hot-reload-safe event handler (looked up by name at dispatch time)
    onHit = function(self, event, scope)
        self:takeDamage(event.damage, event.source, scope)
    end,

    destroy = function(self, scope)
        -- Clean up all subscriptions and timers owned by this component
        -- This is simpler and safer than tracking individual IDs
        bestow.events.unsubscribeAll(self)
        bestow.timer.cancelFor(self)
    end,

    update = function(self, dt, scope)
        if self.regenRate > 0 and self.current < self.max then
            self.transient.timeSinceDamage = self.transient.timeSinceDamage + dt

            if self.transient.timeSinceDamage >= self.regenDelay then
                self:heal(self.regenRate * dt, scope)
            end
        end
    end,

    takeDamage = function(self, amount, source, scope)
        if self.transient.invulnerable then
            bestow.events.emit("DamageBlocked", {
                entity = scope.entity.id,
                amount = amount,
            })
            return 0
        end

        local actual = math.min(self.current, amount)
        self.current = self.current - actual
        self.transient.timeSinceDamage = 0
        self.transient.lastDamageSource = source

        bestow.events.emit("HealthChanged", {
            entity = scope.entity.id,
            oldHealth = self.current + actual,
            newHealth = self.current,
            change = -actual,
        })

        if self.current <= 0 then
            self:die(scope)
        end

        return actual
    end,

    heal = function(self, amount, scope)
        local actual = math.min(self.max - self.current, amount)
        if actual > 0 then
            self.current = self.current + actual
            bestow.events.emit("HealthChanged", {
                entity = scope.entity.id,
                oldHealth = self.current - actual,
                newHealth = self.current,
                change = actual,
            })
        end
        return actual
    end,

    setInvulnerable = function(self, value, duration)
        self.transient.invulnerable = value
        if value and duration then
            -- Hot-reload-safe timer callback (uses table + method name)
            bestow.timer.after(duration, self, "clearInvulnerability")
        end
    end,

    -- Hot-reload-safe timer callback
    clearInvulnerability = function(self)
        self.transient.invulnerable = false
    end,

    getPercent = function(self)
        return self.current / self.max
    end,

    isDead = function(self)
        return self.current <= 0
    end,

    die = function(self, scope)
        bestow.events.emit("EntityDied", {
            entity = scope.entity.id,
            killer = self.transient.lastDamageSource,
            tag = scope.entity.tag,
        })
    end,
}
