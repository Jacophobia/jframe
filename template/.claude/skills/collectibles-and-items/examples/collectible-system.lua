-- Example: Collectible System
-- Shows creating pickups, trigger detection, and collection handling

-- entities/collectibles/coin.lua
return {
    create = function(position)
        local entity = bestow.entity.create()

        bestow.entity.addComponent(entity, "Transform3D", {
            position = position or Vec3.new(0, 1, 0),
            rotation = Quat.identity(),
            scale = Vec3.new(0.5, 0.5, 0.5)
        })
        bestow.entity.addComponent(entity, "MeshRenderer", {
            mesh = "meshes/coin.obj",
            material = "materials/gold"
        })
        bestow.entity.addComponent(entity, "Collectible", {
            type = "coin",
            value = 1,
            collected = false
        })
        bestow.entity.addComponent(entity, "BobAnimation", {
            baseY = position.y,
            phase = 0,
            speed = 2.0,
            amplitude = 0.2
        })

        -- Trigger volume for pickup detection
        bestow.physics3d.createBody(entity, {
            type = "Static",
            shapeType = "Sphere",
            radius = 0.75,
            isSensor = true
        })

        return entity
    end
}

-- systems/collectibles.lua
return {
    triggerSubId = nil,

    init = function()
        local self = app.systems.collectibles

        -- Subscribe to trigger events (table+method for hot-reload safety)
        self.triggerSubId = bestow.events.subscribe("trigger_enter_3d", {},
            app.systems.collectibles, "onTriggerEnter")
    end,

    update = function(dt)
        -- Animate collectibles (bobbing + rotation)
        bestow.entity.each(function(entity)
            if not bestow.entity.hasComponent(entity, "BobAnimation") then return end

            local bob = bestow.entity.getComponent(entity, "BobAnimation")
            bob.phase = bob.phase + bob.speed * dt
            local newY = bob.baseY + math.sin(bob.phase) * bob.amplitude

            bestow.entity.setField(entity, "Transform3D", "position",
                Vec3.new(
                    bestow.entity.getField(entity, "Transform3D", "position").x,
                    newY,
                    bestow.entity.getField(entity, "Transform3D", "position").z
                ))

            -- Spin
            local rot = bestow.entity.getField(entity, "Transform3D", "rotation")
            local spin = Quat.fromAxisAngle(Vec3.up(), dt * 2)
            bestow.entity.setField(entity, "Transform3D", "rotation", rot * spin)

            bestow.entity.setComponent(entity, "BobAnimation", bob)
        end)
    end,

    onTriggerEnter = function(event)
        local self = app.systems.collectibles
        local state = app.main.state

        -- Check if player touched a collectible
        local collectible = nil
        if event.entityA == state.player and bestow.entity.hasComponent(event.entityB, "Collectible") then
            collectible = event.entityB
        elseif event.entityB == state.player and bestow.entity.hasComponent(event.entityA, "Collectible") then
            collectible = event.entityA
        end

        if collectible then
            self.collect(collectible)
        end
    end,

    collect = function(entity)
        local state = app.main.state
        local data = bestow.entity.getComponent(entity, "Collectible")
        if data.collected then return end

        if data.type == "coin" then
            state.score = (state.score or 0) + data.value
        elseif data.type == "health" then
            local health = bestow.entity.getComponent(state.player, "Health")
            if health then
                health.current = math.min(health.max, health.current + data.value)
                bestow.entity.setComponent(state.player, "Health", health)
            end
        end

        bestow.events.emit("item_collected", {
            collector = state.player,
            item = entity,
            itemType = data.type,
            value = data.value
        })

        bestow.entity.destroy(entity)
    end,

    shutdown = function()
        local self = app.systems.collectibles
        if self.triggerSubId then bestow.events.unsubscribe(self.triggerSubId) end
    end
}
