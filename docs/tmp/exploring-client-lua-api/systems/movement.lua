--[[
    Movement System

    Auto-discovered from systems/. Systems operate on entities with specific
    component patterns. They run BEFORE component updates.

    Pattern declares required components - only entities with ALL listed
    components are passed to update().

    State Pattern:
    - self.foo = saved (rarely needed for systems)
    - self.transient.foo = not saved
]]

return {
    -- Pattern declares what entities this system operates on
    pattern = { "Transform", "Movement" },

    -- Optional: define update order relative to other systems
    -- Lower numbers run first. Default is 0.
    order = -10,

    init = function(self, scope)
        -- Systems rarely need saved state, but can have transient
        self.transient.moveSpeed = 5.0
        self.transient.inputDirection = { x = 0, y = 0, z = 0 }

        -- Subscribe to movement actions with pattern matching
        -- Empty pattern {} means "match all events of this type"
        self.transient.forwardSub = bestow.events.subscribe("MoveForward", {}, function(event)
            self.transient.inputDirection.z = 1
        end)

        self.transient.backwardSub = bestow.events.subscribe("MoveBackward", {}, function(event)
            self.transient.inputDirection.z = -1
        end)

        self.transient.leftSub = bestow.events.subscribe("MoveLeft", {}, function(event)
            self.transient.inputDirection.x = -1
        end)

        self.transient.rightSub = bestow.events.subscribe("MoveRight", {}, function(event)
            self.transient.inputDirection.x = 1
        end)
    end,

    destroy = function(self, scope)
        bestow.events.unsubscribe(self.transient.forwardSub)
        bestow.events.unsubscribe(self.transient.backwardSub)
        bestow.events.unsubscribe(self.transient.leftSub)
        bestow.events.unsubscribe(self.transient.rightSub)
    end,

    -- Called once per frame with ALL matching entities
    -- 'entities' is a list of entities that have Transform AND Movement
    update = function(self, dt, scope, entities)
        local dir = self.transient.inputDirection
        local speed = self.transient.moveSpeed

        for _, entity in ipairs(entities) do
            local transform = entity.Transform
            local movement = entity.Movement

            -- Apply entity-specific speed modifier
            local effectiveSpeed = speed * (movement.speedMultiplier or 1.0)

            -- Only player responds to input
            if entity.tag == "player" then
                transform.position.x = transform.position.x + dir.x * effectiveSpeed * dt
                transform.position.z = transform.position.z + dir.z * effectiveSpeed * dt
            end

            -- AI entities use their own movement logic (could be in AI component)
            if entity.tag == "enemy" and movement.targetPosition then
                local dx = movement.targetPosition.x - transform.position.x
                local dz = movement.targetPosition.z - transform.position.z
                local dist = math.sqrt(dx * dx + dz * dz)

                if dist > 0.1 then
                    transform.position.x = transform.position.x + (dx / dist) * effectiveSpeed * dt
                    transform.position.z = transform.position.z + (dz / dist) * effectiveSpeed * dt
                end
            end
        end

        -- Reset input direction (continuous actions re-set it each frame)
        dir.x = 0
        dir.z = 0
    end,
}
