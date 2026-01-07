--[[
    Combat System

    Handles attack logic for entities with Combat and Transform components.
    Demonstrates event pattern matching - subscriptions only fire when
    the event data matches the specified pattern.
]]

return {
    pattern = { "Transform", "Combat" },
    order = 0,

    init = function(self, scope)
        self.transient.attackRequests = {}

        -- Subscribe to Attack action - empty pattern matches all
        self.transient.attackSub = bestow.events.subscribe("Attack", {}, function(event)
            -- Queue attack for player (system will process)
            self.transient.attackRequests["player"] = true
        end)
    end,

    destroy = function(self, scope)
        bestow.events.unsubscribe(self.transient.attackSub)
    end,

    update = function(self, dt, scope, entities)
        for _, entity in ipairs(entities) do
            local combat = entity.Combat
            local transform = entity.Transform

            -- Process cooldowns
            if combat.cooldownRemaining > 0 then
                combat.cooldownRemaining = combat.cooldownRemaining - dt
            end

            -- Process attack states
            if combat.isAttacking then
                combat.attackTimer = combat.attackTimer + dt
                self:updateAttackPhase(entity, dt, scope)
            end

            -- Check for attack request (player)
            if entity.tag == "player" and self.transient.attackRequests["player"] then
                self:startAttack(entity, scope)
                self.transient.attackRequests["player"] = nil
            end
        end
    end,

    startAttack = function(self, entity, scope)
        local combat = entity.Combat

        if combat.isAttacking or combat.cooldownRemaining > 0 then
            return false
        end

        combat.isAttacking = true
        combat.attackPhase = "windup"
        combat.attackTimer = 0
        combat.hitEntitiesThisSwing = {}

        bestow.events.emit("AttackStarted", { entity = entity.id })
        return true
    end,

    updateAttackPhase = function(self, entity, dt, scope)
        local combat = entity.Combat

        if combat.attackPhase == "windup" then
            if combat.attackTimer >= combat.windupDuration then
                combat.attackPhase = "active"
                combat.attackTimer = combat.attackTimer - combat.windupDuration
            end

        elseif combat.attackPhase == "active" then
            self:performHitDetection(entity, scope)
            if combat.attackTimer >= combat.activeDuration then
                combat.attackPhase = "recovery"
                combat.attackTimer = combat.attackTimer - combat.activeDuration
            end

        elseif combat.attackPhase == "recovery" then
            if combat.attackTimer >= combat.recoveryDuration then
                self:endAttack(entity, scope)
            end
        end
    end,

    performHitDetection = function(self, entity, scope)
        local combat = entity.Combat
        local transform = entity.Transform

        local hits = bestow.physics.sphereCast({
            origin = transform.position,
            radius = combat.attackRange,
            excludeEntity = entity.id,
        })

        for _, hit in ipairs(hits) do
            if not combat.hitEntitiesThisSwing[hit.entityId] then
                combat.hitEntitiesThisSwing[hit.entityId] = true

                local damage = combat.baseDamage
                if scope.level and scope.level.playerDamageMultiplier then
                    damage = damage * scope.level.playerDamageMultiplier
                end

                local knockback = (hit.position - transform.position)
                knockback.y = 0
                knockback = knockback:normalized() * 5

                bestow.events.emit("Hit", {
                    target = hit.entityId,
                    source = entity.id,
                    damage = damage,
                    hitPoint = hit.position,
                    knockback = knockback,
                })

                bestow.events.emit("CombatHit", {
                    attacker = entity.id,
                    target = hit.entityId,
                    damage = damage,
                })
            end
        end
    end,

    endAttack = function(self, entity, scope)
        local combat = entity.Combat

        combat.isAttacking = false
        combat.attackPhase = "none"
        combat.attackTimer = 0
        combat.hitEntitiesThisSwing = {}
        combat.cooldownRemaining = 1.0 / combat.attackSpeed

        bestow.events.emit("AttackEnded", { entity = entity.id })
    end,
}
