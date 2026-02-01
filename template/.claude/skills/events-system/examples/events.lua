-- Example: Events System Usage
-- Shows subscribing, emitting, filtering, and cleanup

-- systems/combat.lua
return {
    collisionSubId = nil,
    damageSubId = nil,

    init = function()
        local self = app.systems.combat

        -- Subscribe to collision events (table+method pattern for hot-reload safety)
        self.collisionSubId = bestow.events.subscribe("collision_3d", {},
            app.systems.combat, "onCollision")

        -- Subscribe to damage events with filter (only events targeting the player)
        self.damageSubId = bestow.events.subscribe("entity_damaged",
            { target = app.main.state.player },
            app.systems.combat, "onPlayerDamaged")
    end,

    onCollision = function(event)
        local self = app.systems.combat
        -- event has: entityA, entityB, contactPoint, contactNormal, impulse

        if bestow.entity.hasComponent(event.entityA, "Projectile") then
            self.applyDamage(event.entityA, event.entityB, 25)
        elseif bestow.entity.hasComponent(event.entityB, "Projectile") then
            self.applyDamage(event.entityB, event.entityA, 25)
        end
    end,

    onPlayerDamaged = function(event)
        -- event has: target, source, amount
        print("Player took " .. event.amount .. " damage!")
    end,

    applyDamage = function(source, target, amount)
        local self = app.systems.combat

        if not bestow.entity.hasComponent(target, "Health") then return end

        local health = bestow.entity.getComponent(target, "Health")
        health.current = math.max(0, health.current - amount)
        bestow.entity.setComponent(target, "Health", health)

        -- Emit damage event (other systems can react)
        bestow.events.emit("entity_damaged", {
            target = target,
            source = source,
            amount = amount
        })

        -- Emit death event if dead
        if health.current <= 0 then
            bestow.events.emit("entity_died", {
                entity = target,
                killer = source
            })
        end
    end,

    shutdown = function()
        local self = app.systems.combat
        if self.collisionSubId then bestow.events.unsubscribe(self.collisionSubId) end
        if self.damageSubId then bestow.events.unsubscribe(self.damageSubId) end
    end
}
