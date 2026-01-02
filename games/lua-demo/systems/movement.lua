-- games/lua-demo/systems/movement.lua
-- Movement system for the demo game
--
-- Handles velocity-based movement for all entities with
-- Transform3D and Velocity components.

return {
    -- Gravity constant
    gravity = Vec3.new(0, -9.81, 0),

    -- Ground plane height
    groundLevel = 0.0,

    -- Update all moving entities
    update = function(dt)
        local self = app.systems.movement

        -- Iterate all entities with both Transform3D and Velocity
        bestow.entity.each(function(entity)
            if not bestow.entity.hasComponent(entity, "Transform3D") then
                return
            end
            if not bestow.entity.hasComponent(entity, "Velocity") then
                return
            end

            local transform = bestow.entity.getComponent(entity, "Transform3D")
            local velocity = bestow.entity.getComponent(entity, "Velocity")

            if not transform or not velocity then
                return
            end

            -- Apply gravity if above ground
            if transform.position.y > self.groundLevel then
                velocity.linear = velocity.linear + self.gravity * dt
            end

            -- Update position based on velocity
            local newPosition = transform.position + velocity.linear * dt

            -- Simple ground collision
            if newPosition.y < self.groundLevel then
                newPosition.y = self.groundLevel
                velocity.linear.y = 0
            end

            -- Apply rotation
            if velocity.angular:length() > 0.001 then
                local rotationDelta = Quat.fromEuler(velocity.angular * dt)
                local newRotation = transform.rotation * rotationDelta
                bestow.entity.setField(entity, "Transform3D", "rotation", newRotation)
            end

            -- Update transform position
            bestow.entity.setField(entity, "Transform3D", "position", newPosition)
            bestow.entity.setField(entity, "Velocity", "linear", velocity.linear)
        end)
    end,

    -- Apply an impulse to an entity
    applyImpulse = function(entity, impulse)
        if not bestow.entity.hasComponent(entity, "Velocity") then
            return false
        end

        local velocity = bestow.entity.getComponent(entity, "Velocity")
        velocity.linear = velocity.linear + impulse
        bestow.entity.setField(entity, "Velocity", "linear", velocity.linear)
        return true
    end,

    -- Set velocity directly
    setVelocity = function(entity, linear, angular)
        if not bestow.entity.hasComponent(entity, "Velocity") then
            return false
        end

        if linear then
            bestow.entity.setField(entity, "Velocity", "linear", linear)
        end
        if angular then
            bestow.entity.setField(entity, "Velocity", "angular", angular)
        end
        return true
    end
}
