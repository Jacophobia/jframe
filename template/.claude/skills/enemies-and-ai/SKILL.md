---
name: enemies-and-ai
description: Create enemies with AI behavior, pathfinding, and combat in Bestow. Use when implementing enemy characters, patrol behavior, chase mechanics, or attack patterns.
---

# Enemies and AI

Create enemies with behavior patterns, pathfinding, and combat mechanics.

## Basic Enemy Entity

```lua
-- entities/enemies/basic.lua
return {
    create = function(position, config)
        local self = app.entities.enemies.basic
        local entity = bestow.entity.create()

        config = config or {}

        bestow.entity.addComponent(entity, "Transform3D", {
            position = position or Vec3.new(0, 0, 0),
            rotation = Quat.identity(),
            scale = Vec3.new(1, 1, 1)
        })

        bestow.entity.addComponent(entity, "MeshRenderer", {
            mesh = config.mesh or "meshes/enemy.obj",
            material = config.material or "materials/enemy"
        })

        bestow.entity.addComponent(entity, "Health", {
            current = config.health or 50,
            max = config.health or 50
        })

        bestow.entity.addComponent(entity, "Enemy", {
            type = config.type or "basic",
            state = "idle",
            target = nil,
            damage = config.damage or 10,
            attackRange = config.attackRange or 2.0,
            detectRange = config.detectRange or 10.0,
            moveSpeed = config.moveSpeed or 3.0
        })

        -- Physics body
        bestow.physics3d.createBody(entity, {
            type = "Dynamic",
            shapeType = "Capsule",
            shapeRadius = 0.4,
            shapeHalfHeight = 0.8,
            density = 1000.0
        })

        return entity
    end
}
```

## Simple State Machine AI

```lua
-- systems/enemy_ai.lua
return {
    update = function(dt)
        local self = app.systems.enemy_ai
        local state = app.main.state

        bestow.entity.each(function(entity)
            if not bestow.entity.hasComponent(entity, "Enemy") then return end

            local enemy = bestow.entity.getComponent(entity, "Enemy")
            local pos = bestow.entity.getField(entity, "Transform3D", "position")

            -- Update based on state
            if enemy.state == "idle" then
                self.updateIdle(entity, enemy, pos, dt)
            elseif enemy.state == "patrol" then
                self.updatePatrol(entity, enemy, pos, dt)
            elseif enemy.state == "chase" then
                self.updateChase(entity, enemy, pos, dt)
            elseif enemy.state == "attack" then
                self.updateAttack(entity, enemy, pos, dt)
            elseif enemy.state == "hurt" then
                self.updateHurt(entity, enemy, pos, dt)
            end

            bestow.entity.setComponent(entity, "Enemy", enemy)
        end)
    end,

    updateIdle = function(entity, enemy, pos, dt)
        local self = app.systems.enemy_ai
        local state = app.main.state

        -- Check for player in range
        if state.player then
            local playerPos = bestow.entity.getField(state.player, "Transform3D", "position")
            local distance = (playerPos - pos):length()

            if distance < enemy.detectRange then
                enemy.state = "chase"
                enemy.target = state.player
            end
        end
    end,

    updateChase = function(entity, enemy, pos, dt)
        local self = app.systems.enemy_ai

        if not enemy.target or not bestow.entity.isValid(enemy.target) then
            enemy.state = "idle"
            enemy.target = nil
            return
        end

        local targetPos = bestow.entity.getField(enemy.target, "Transform3D", "position")
        local toTarget = targetPos - pos
        local distance = toTarget:length()

        -- Lost target
        if distance > enemy.detectRange * 1.5 then
            enemy.state = "patrol"
            enemy.target = nil
            return
        end

        -- In attack range
        if distance < enemy.attackRange then
            enemy.state = "attack"
            return
        end

        -- Move towards target
        local direction = toTarget:normalize()
        local velocity = direction * enemy.moveSpeed

        -- Simple movement (or use character controller)
        local newPos = pos + velocity * dt
        bestow.entity.setField(entity, "Transform3D", "position", newPos)

        -- Face target
        local angle = math.atan2(direction.x, -direction.z)
        local rotation = Quat.fromAxisAngle(Vec3.new(0, 1, 0), angle)
        bestow.entity.setField(entity, "Transform3D", "rotation", rotation)
    end,

    updateAttack = function(entity, enemy, pos, dt)
        local self = app.systems.enemy_ai

        -- Attack cooldown
        if not enemy.attackCooldown then
            enemy.attackCooldown = 0
        end

        enemy.attackCooldown = enemy.attackCooldown - dt

        if enemy.attackCooldown <= 0 then
            -- Perform attack
            if enemy.target and bestow.entity.isValid(enemy.target) then
                local targetPos = bestow.entity.getField(enemy.target, "Transform3D", "position")
                local distance = (targetPos - pos):length()

                if distance < enemy.attackRange then
                    app.systems.combat.damage(enemy.target, enemy.damage, entity)
                    app.systems.audio.playSfx("enemy_attack")
                end
            end

            enemy.attackCooldown = 1.0  -- 1 second between attacks
        end

        -- Check if target moved out of range
        if enemy.target and bestow.entity.isValid(enemy.target) then
            local targetPos = bestow.entity.getField(enemy.target, "Transform3D", "position")
            local distance = (targetPos - pos):length()

            if distance > enemy.attackRange * 1.5 then
                enemy.state = "chase"
            end
        else
            enemy.state = "idle"
        end
    end,

    updateHurt = function(entity, enemy, pos, dt)
        if not enemy.hurtTimer then
            enemy.hurtTimer = 0.5
        end

        enemy.hurtTimer = enemy.hurtTimer - dt

        if enemy.hurtTimer <= 0 then
            enemy.state = "chase"
        end
    end
}
```

## Patrol Behavior

```lua
-- Add patrol data to enemy
bestow.entity.addComponent(entity, "Patrol", {
    points = {
        Vec3.new(0, 0, 0),
        Vec3.new(10, 0, 0),
        Vec3.new(10, 0, 10),
        Vec3.new(0, 0, 10)
    },
    currentPoint = 1,
    waitTime = 2.0,
    waitTimer = 0
})

-- In enemy_ai.lua
updatePatrol = function(entity, enemy, pos, dt)
    local self = app.systems.enemy_ai
    local state = app.main.state

    local patrol = bestow.entity.getComponent(entity, "Patrol")
    if not patrol then
        enemy.state = "idle"
        return
    end

    -- Check for player
    if state.player then
        local playerPos = bestow.entity.getField(state.player, "Transform3D", "position")
        local distance = (playerPos - pos):length()

        if distance < enemy.detectRange then
            enemy.state = "chase"
            enemy.target = state.player
            return
        end
    end

    -- Waiting at point
    if patrol.waitTimer > 0 then
        patrol.waitTimer = patrol.waitTimer - dt
        bestow.entity.setComponent(entity, "Patrol", patrol)
        return
    end

    -- Move to current point
    local targetPoint = patrol.points[patrol.currentPoint]
    local toPoint = targetPoint - pos
    local distance = toPoint:length()

    if distance < 0.5 then
        -- Reached point, wait then move to next
        patrol.waitTimer = patrol.waitTime
        patrol.currentPoint = patrol.currentPoint + 1
        if patrol.currentPoint > #patrol.points then
            patrol.currentPoint = 1
        end
        bestow.entity.setComponent(entity, "Patrol", patrol)
        return
    end

    -- Move towards point
    local direction = toPoint:normalize()
    local velocity = direction * enemy.moveSpeed * 0.5  -- Slower when patrolling

    local newPos = pos + velocity * dt
    bestow.entity.setField(entity, "Transform3D", "position", newPos)

    -- Face movement direction
    local angle = math.atan2(direction.x, -direction.z)
    local rotation = Quat.fromAxisAngle(Vec3.new(0, 1, 0), angle)
    bestow.entity.setField(entity, "Transform3D", "rotation", rotation)
end
```

## Enemy Spawner

```lua
-- systems/enemy_spawner.lua
return {
    spawners = {},

    registerSpawner = function(entity, config)
        local self = app.systems.enemy_spawner

        table.insert(self.spawners, {
            entity = entity,
            enemyType = config.enemyType or "basic",
            maxEnemies = config.maxEnemies or 3,
            spawnInterval = config.spawnInterval or 5.0,
            spawnRadius = config.spawnRadius or 2.0,
            spawnTimer = config.initialDelay or 0,
            spawnedEnemies = {}
        })
    end,

    update = function(dt)
        local self = app.systems.enemy_spawner
        local state = app.main.state

        for _, spawner in ipairs(self.spawners) do
            -- Clean up dead enemies
            local alive = {}
            for _, enemy in ipairs(spawner.spawnedEnemies) do
                if bestow.entity.isValid(enemy) then
                    table.insert(alive, enemy)
                end
            end
            spawner.spawnedEnemies = alive

            -- Check if player nearby (optional: only spawn when player close)
            local spawnerPos = bestow.entity.getField(spawner.entity, "Transform3D", "position")
            local playerClose = false

            if state.player then
                local playerPos = bestow.entity.getField(state.player, "Transform3D", "position")
                playerClose = (playerPos - spawnerPos):length() < 30
            end

            -- Spawn if needed
            if playerClose and #spawner.spawnedEnemies < spawner.maxEnemies then
                spawner.spawnTimer = spawner.spawnTimer - dt

                if spawner.spawnTimer <= 0 then
                    local spawnPos = spawnerPos + Vec3.new(
                        (math.random() - 0.5) * spawner.spawnRadius * 2,
                        0,
                        (math.random() - 0.5) * spawner.spawnRadius * 2
                    )

                    local enemy = app.entities.enemies[spawner.enemyType].create(spawnPos)
                    table.insert(spawner.spawnedEnemies, enemy)

                    spawner.spawnTimer = spawner.spawnInterval
                end
            end
        end
    end
}
```

## Boss Enemy

```lua
-- entities/enemies/boss.lua
return {
    create = function(position)
        local self = app.entities.enemies.boss
        local entity = app.entities.enemies.basic.create(position, {
            mesh = "meshes/boss.obj",
            material = "materials/boss",
            health = 500,
            damage = 30,
            attackRange = 3.0,
            detectRange = 20.0,
            moveSpeed = 2.0,
            type = "boss"
        })

        bestow.entity.addComponent(entity, "Boss", {
            phase = 1,
            attackPattern = 1,
            enraged = false
        })

        return entity
    end
}

-- systems/boss_ai.lua
return {
    update = function(dt)
        local self = app.systems.boss_ai

        bestow.entity.each(function(entity)
            if not bestow.entity.hasComponent(entity, "Boss") then return end

            local boss = bestow.entity.getComponent(entity, "Boss")
            local enemy = bestow.entity.getComponent(entity, "Enemy")
            local health = bestow.entity.getComponent(entity, "Health")

            -- Phase transitions
            local healthPercent = health.current / health.max
            if healthPercent < 0.3 and boss.phase < 3 then
                boss.phase = 3
                boss.enraged = true
                enemy.moveSpeed = enemy.moveSpeed * 1.5
                app.systems.audio.playSfx("boss_enrage")
            elseif healthPercent < 0.6 and boss.phase < 2 then
                boss.phase = 2
                app.systems.audio.playSfx("boss_phase2")
            end

            -- Attack patterns based on phase
            if enemy.state == "attack" then
                self.performAttackPattern(entity, boss, enemy, dt)
            end

            bestow.entity.setComponent(entity, "Boss", boss)
        end)
    end,

    performAttackPattern = function(entity, boss, enemy, dt)
        local self = app.systems.boss_ai

        if boss.phase == 1 then
            -- Simple attacks
            self.basicAttack(entity, enemy)
        elseif boss.phase == 2 then
            -- Add special attacks
            if math.random() < 0.3 then
                self.aoeAttack(entity, enemy)
            else
                self.basicAttack(entity, enemy)
            end
        elseif boss.phase == 3 then
            -- Enraged patterns
            if math.random() < 0.5 then
                self.chargeAttack(entity, enemy)
            else
                self.aoeAttack(entity, enemy)
            end
        end
    end,

    aoeAttack = function(entity, enemy)
        local pos = bestow.entity.getField(entity, "Transform3D", "position")

        -- Damage all players in radius
        local targets = bestow.physics3d.overlapSphere(pos, 5.0)
        for _, target in ipairs(targets) do
            if target ~= entity and bestow.entity.hasComponent(target, "Health") then
                app.systems.combat.damage(target, enemy.damage * 0.5, entity)
            end
        end

        app.systems.audio.playSfx("boss_aoe")
        -- Spawn visual effect
    end
}
```

## Combat System

```lua
-- systems/combat.lua
return {
    damage = function(target, amount, source)
        local self = app.systems.combat

        if not bestow.entity.hasComponent(target, "Health") then
            return false
        end

        local health = bestow.entity.getComponent(target, "Health")

        -- Apply damage
        health.current = health.current - amount
        bestow.entity.setComponent(target, "Health", health)

        -- Visual feedback
        self.flashRed(target)
        self.spawnDamageNumber(target, amount)

        -- Knockback
        if source then
            local sourcePos = bestow.entity.getField(source, "Transform3D", "position")
            local targetPos = bestow.entity.getField(target, "Transform3D", "position")
            local knockDir = (targetPos - sourcePos):normalize()

            self.applyKnockback(target, knockDir, amount * 0.1)
        end

        -- Death
        if health.current <= 0 then
            self.kill(target, source)
        else
            -- Trigger hurt state for enemies
            if bestow.entity.hasComponent(target, "Enemy") then
                local enemy = bestow.entity.getComponent(target, "Enemy")
                enemy.state = "hurt"
                enemy.hurtTimer = 0.3
                bestow.entity.setComponent(target, "Enemy", enemy)
            end
        end

        return true
    end,

    kill = function(entity, killer)
        local self = app.systems.combat
        local state = app.main.state

        if bestow.entity.hasComponent(entity, "Enemy") then
            -- Enemy death
            local enemy = bestow.entity.getComponent(entity, "Enemy")

            -- Award points
            local points = 100
            if enemy.type == "boss" then
                points = 1000
            end
            state.score = state.score + points

            -- Spawn drops
            app.systems.drops.spawn(entity)

            -- Effects
            app.systems.audio.playSfx("enemy_death")

            -- Destroy
            bestow.entity.destroy(entity)

        elseif entity == state.player then
            -- Player death
            app.systems.audio.playSfx("player_death")
            state.lives = state.lives - 1

            if state.lives > 0 then
                app.systems.checkpoints.respawnPlayer()
            else
                app.systems.state_manager.push("game_over")
            end
        end
    end
}
```

## Best Practices

1. **Use state machines** - Clear enemy behavior states
2. **Check entity validity** - Entities can be destroyed mid-frame
3. **Pool enemy spawning** - Don't spawn too many at once
4. **Use detection ranges** - Don't chase across the map
5. **Add patrol behavior** - Enemies feel alive
6. **Implement phases for bosses** - Escalating difficulty
7. **Visual feedback on hits** - Flash, knockback, numbers
