---
name: collectibles-and-items
description: Create collectible items, pickups, and inventory systems in Bestow. Use when implementing coins, power-ups, health pickups, or inventory management.
---

# Collectibles and Items

Implement pickups, collectibles, and inventory systems.

## Basic Collectible

```lua
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

        -- Trigger volume for pickup
        bestow.physics3d.createBody(entity, {
            type = "Static",
            shapeType = "Sphere",
            radius = 0.5,
            isSensor = true,
            layer = Layers.Trigger
        })

        -- Bobbing animation state
        bestow.entity.addComponent(entity, "BobAnimation", {
            baseY = position.y,
            offset = 0,
            speed = 2.0,
            amplitude = 0.2
        })

        return entity
    end
}
```

## Collectible System

```lua
-- systems/collectibles.lua
return {
    init = function()
        local self = app.systems.collectibles

        -- Subscribe using table+method pattern (hot-reload safe)
        self.triggerSubId = bestow.events.subscribe("trigger_enter_3d", {},
            app.systems.collectibles, "onTrigger")
    end,

    onTrigger = function(event)
        local self = app.systems.collectibles
        self.onTriggerEnter(event.entityA, event.entityB)
    end,

    update = function(dt)
        local self = app.systems.collectibles

        -- Animate collectibles
        bestow.entity.each(function(entity)
            if not bestow.entity.hasComponent(entity, "BobAnimation") then return end

            local bob = bestow.entity.getComponent(entity, "BobAnimation")
            local transform = bestow.entity.getComponent(entity, "Transform3D")

            -- Bobbing motion
            bob.offset = bob.offset + bob.speed * dt
            local newY = bob.baseY + math.sin(bob.offset) * bob.amplitude
            transform.position.y = newY

            -- Rotation
            transform.rotation = transform.rotation * Quat.fromAxisAngle(
                Vec3.new(0, 1, 0),
                dt * 2  -- Rotate speed
            )

            bestow.entity.setComponent(entity, "BobAnimation", bob)
            bestow.entity.setComponent(entity, "Transform3D", transform)
        end)
    end,

    onTriggerEnter = function(entityA, entityB)
        local self = app.systems.collectibles
        local state = app.main.state

        -- Check if player touched a collectible
        local player, collectible = nil, nil

        if entityA == state.player and bestow.entity.hasComponent(entityB, "Collectible") then
            player = entityA
            collectible = entityB
        elseif entityB == state.player and bestow.entity.hasComponent(entityA, "Collectible") then
            player = entityB
            collectible = entityA
        end

        if player and collectible then
            self.collect(collectible)
        end
    end,

    collect = function(entity)
        local self = app.systems.collectibles
        local state = app.main.state

        local collectible = bestow.entity.getComponent(entity, "Collectible")
        if collectible.collected then return end  -- Already collected

        collectible.collected = true

        -- Handle by type
        if collectible.type == "coin" then
            state.coins = (state.coins or 0) + collectible.value
            app.systems.audio.playSfx("coin")

        elseif collectible.type == "health" then
            local health = bestow.entity.getComponent(state.player, "Health")
            health.current = math.min(health.max, health.current + collectible.value)
            bestow.entity.setComponent(state.player, "Health", health)
            app.systems.audio.playSfx("health")

        elseif collectible.type == "key" then
            state.keys = (state.keys or 0) + 1
            app.systems.audio.playSfx("key")

        elseif collectible.type == "powerup" then
            app.systems.powerups.activate(collectible.powerupType, collectible.duration)
            app.systems.audio.playSfx("powerup")
        end

        -- Spawn collection effect
        local pos = bestow.entity.getField(entity, "Transform3D", "position")
        app.systems.effects.spawn("collect", pos)

        -- Destroy collectible
        bestow.entity.destroy(entity)
    end
}
```

## Power-Up System

```lua
-- entities/collectibles/powerup.lua
return {
    types = {
        speed = {
            mesh = "meshes/powerup_speed.obj",
            material = "materials/powerup_blue",
            duration = 10.0
        },
        invincibility = {
            mesh = "meshes/powerup_shield.obj",
            material = "materials/powerup_gold",
            duration = 5.0
        },
        doubleJump = {
            mesh = "meshes/powerup_wings.obj",
            material = "materials/powerup_white",
            duration = 15.0
        }
    },

    create = function(position, powerupType)
        local self = app.entities.collectibles.powerup
        local config = self.types[powerupType]

        local entity = bestow.entity.create()

        bestow.entity.addComponent(entity, "Transform3D", {
            position = position,
            rotation = Quat.identity(),
            scale = Vec3.new(1, 1, 1)
        })

        bestow.entity.addComponent(entity, "MeshRenderer", {
            mesh = config.mesh,
            material = config.material
        })

        bestow.entity.addComponent(entity, "Collectible", {
            type = "powerup",
            powerupType = powerupType,
            duration = config.duration
        })

        bestow.entity.addComponent(entity, "BobAnimation", {
            baseY = position.y,
            offset = 0,
            speed = 2.0,
            amplitude = 0.3
        })

        bestow.physics3d.createBody(entity, {
            type = "Static",
            shapeType = "Sphere",
            radius = 0.75,
            isSensor = true
        })

        return entity
    end
}

-- systems/powerups.lua
return {
    active = {},  -- { [type] = { timer = X, ... } }

    activate = function(powerupType, duration)
        local self = app.systems.powerups
        local state = app.main.state

        self.active[powerupType] = {
            timer = duration
        }

        -- Apply immediate effects
        if powerupType == "speed" then
            local controller = bestow.entity.getComponent(state.player, "PlayerController")
            if controller then
                controller.originalSpeed = controller.speed
                controller.speed = controller.speed * 1.5
                bestow.entity.setComponent(state.player, "PlayerController", controller)
            end

        elseif powerupType == "invincibility" then
            local health = bestow.entity.getComponent(state.player, "Health")
            health.invulnerable = true
            bestow.entity.setComponent(state.player, "Health", health)

        elseif powerupType == "doubleJump" then
            local playerState = bestow.entity.getComponent(state.player, "PlayerState")
            playerState.maxJumps = 3
            bestow.entity.setComponent(state.player, "PlayerState", playerState)
        end

        -- Visual indicator
        app.systems.ui.showPowerupIndicator(powerupType, duration)
    end,

    deactivate = function(powerupType)
        local self = app.systems.powerups
        local state = app.main.state

        -- Remove effects
        if powerupType == "speed" then
            local controller = bestow.entity.getComponent(state.player, "PlayerController")
            if controller and controller.originalSpeed then
                controller.speed = controller.originalSpeed
                bestow.entity.setComponent(state.player, "PlayerController", controller)
            end

        elseif powerupType == "invincibility" then
            local health = bestow.entity.getComponent(state.player, "Health")
            health.invulnerable = false
            bestow.entity.setComponent(state.player, "Health", health)

        elseif powerupType == "doubleJump" then
            local playerState = bestow.entity.getComponent(state.player, "PlayerState")
            playerState.maxJumps = 2
            bestow.entity.setComponent(state.player, "PlayerState", playerState)
        end

        self.active[powerupType] = nil
    end,

    update = function(dt)
        local self = app.systems.powerups

        for powerupType, data in pairs(self.active) do
            data.timer = data.timer - dt

            if data.timer <= 0 then
                self.deactivate(powerupType)
            elseif data.timer < 3 then
                -- Warning flash when about to expire
                app.systems.ui.flashPowerupIndicator(powerupType)
            end
        end
    end,

    isActive = function(powerupType)
        local self = app.systems.powerups
        return self.active[powerupType] ~= nil
    end
}
```

## Inventory System

```lua
-- systems/inventory.lua
return {
    maxSlots = 10,
    items = {},  -- { { id = "sword", count = 1 }, ... }

    init = function()
        local self = app.systems.inventory
        self.items = {}
    end,

    addItem = function(itemId, count)
        local self = app.systems.inventory
        count = count or 1

        -- Check for existing stack
        for _, slot in ipairs(self.items) do
            if slot.id == itemId then
                local itemDef = app.data.items[itemId]
                local maxStack = itemDef.maxStack or 99

                local canAdd = math.min(count, maxStack - slot.count)
                slot.count = slot.count + canAdd
                count = count - canAdd

                if count <= 0 then
                    return true
                end
            end
        end

        -- Add new slot(s)
        while count > 0 and #self.items < self.maxSlots do
            local itemDef = app.data.items[itemId]
            local maxStack = itemDef.maxStack or 99
            local toAdd = math.min(count, maxStack)

            table.insert(self.items, {
                id = itemId,
                count = toAdd
            })

            count = count - toAdd
        end

        return count <= 0  -- True if all items added
    end,

    removeItem = function(itemId, count)
        local self = app.systems.inventory
        count = count or 1

        for i = #self.items, 1, -1 do
            local slot = self.items[i]
            if slot.id == itemId then
                local toRemove = math.min(count, slot.count)
                slot.count = slot.count - toRemove
                count = count - toRemove

                if slot.count <= 0 then
                    table.remove(self.items, i)
                end

                if count <= 0 then
                    return true
                end
            end
        end

        return count <= 0
    end,

    hasItem = function(itemId, count)
        local self = app.systems.inventory
        count = count or 1

        local total = 0
        for _, slot in ipairs(self.items) do
            if slot.id == itemId then
                total = total + slot.count
            end
        end

        return total >= count
    end,

    getCount = function(itemId)
        local self = app.systems.inventory

        local total = 0
        for _, slot in ipairs(self.items) do
            if slot.id == itemId then
                total = total + slot.count
            end
        end

        return total
    end,

    useItem = function(slotIndex)
        local self = app.systems.inventory

        local slot = self.items[slotIndex]
        if not slot then return false end

        local itemDef = app.data.items[slot.id]
        if not itemDef.usable then return false end

        -- Apply item effect (pass state for hot-reload safe context)
        if itemDef.onUse then
            itemDef.onUse(app.main.state)
        end

        -- Consume item
        if itemDef.consumable then
            self.removeItem(slot.id, 1)
        end

        return true
    end
}

-- data/items.lua
return {
    health_potion = {
        name = "Health Potion",
        description = "Restores 50 health",
        icon = "textures/ui/health_potion.png",
        maxStack = 10,
        usable = true,
        consumable = true,
        onUse = function(state)
            if not state.player or not bestow.entity.isValid(state.player) then return end
            local health = bestow.entity.getComponent(state.player, "Health")
            health.current = math.min(health.max, health.current + 50)
            bestow.entity.setComponent(state.player, "Health", health)
            app.systems.audio.playSfx("drink")
        end
    },

    key = {
        name = "Key",
        description = "Opens locked doors",
        icon = "textures/ui/key.png",
        maxStack = 99,
        usable = false
    },

    sword = {
        name = "Iron Sword",
        description = "A basic sword",
        icon = "textures/ui/sword.png",
        maxStack = 1,
        usable = true,
        consumable = false,
        onUse = function(state)
            -- Equip sword
            app.systems.equipment.equip("weapon", "sword")
        end
    }
}
```

## Drop System

```lua
-- systems/drops.lua
return {
    dropTables = {
        enemy = {
            { item = "coin", chance = 0.5, min = 1, max = 3 },
            { item = "health_pickup", chance = 0.2 }
        },
        boss = {
            { item = "coin", chance = 1.0, min = 10, max = 20 },
            { item = "key", chance = 1.0 },
            { item = "powerup_speed", chance = 0.5 }
        },
        chest = {
            { item = "coin", chance = 1.0, min = 5, max = 15 },
            { item = "health_potion", chance = 0.3 },
            { item = "key", chance = 0.1 }
        }
    },

    spawn = function(entity)
        local self = app.systems.drops

        -- Determine drop table
        local dropTable = nil
        if bestow.entity.hasComponent(entity, "Boss") then
            dropTable = self.dropTables.boss
        elseif bestow.entity.hasComponent(entity, "Enemy") then
            dropTable = self.dropTables.enemy
        elseif bestow.entity.hasComponent(entity, "Chest") then
            dropTable = self.dropTables.chest
        end

        if not dropTable then return end

        local pos = bestow.entity.getField(entity, "Transform3D", "position")

        -- Roll for each possible drop
        for _, drop in ipairs(dropTable) do
            if math.random() < drop.chance then
                local count = 1
                if drop.min and drop.max then
                    count = math.random(drop.min, drop.max)
                end

                -- Spawn items
                for i = 1, count do
                    local offset = Vec3.new(
                        (math.random() - 0.5) * 2,
                        0.5,
                        (math.random() - 0.5) * 2
                    )

                    self.spawnDrop(drop.item, pos + offset)
                end
            end
        end
    end,

    spawnDrop = function(itemType, position)
        local factory = app.entities.collectibles[itemType]
        if factory then
            return factory.create(position)
        end
    end
}
```

## Best Practices

1. **Use trigger volumes** - Not distance checks every frame
2. **Subscribe to events** - For collection detection
3. **Add visual feedback** - Bobbing, rotation, particles
4. **Play sounds** - Different sounds per type
5. **Stack items** - Don't create new slots unnecessarily
6. **Timed power-ups** - Clear duration with warnings
7. **Drop tables** - Data-driven random drops
