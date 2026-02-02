-- Example: Enemy AI with State Machine
-- Shows idle, patrol, chase, and attack states

-- entities/enemies/goblin.lua
return {
    create = function(position)
        local entity = bestow.entity.create()

        bestow.entity.addComponent(entity, "Transform3D", {
            position = position or Vec3.new(0, 0, 0),
            rotation = Quat.identity(),
            scale = Vec3.one()
        })
        bestow.entity.addComponent(entity, "MeshRenderer", {
            mesh = "meshes/goblin.obj",
            material = "materials/goblin"
        })
        bestow.entity.addComponent(entity, "Health", { current = 50, max = 50 })
        bestow.entity.addComponent(entity, "Enemy", {
            state = "idle",
            target = nil,
            damage = 10,
            attackRange = 2.0,
            detectRange = 12.0,
            moveSpeed = 3.0,
            attackCooldown = 0
        })

        bestow.physics3d.createBody(entity, {
            type = "Dynamic",
            shapeType = "Capsule",
            radius = 0.4,
            height = 1.4,
            mass = 50.0
        })

        return entity
    end
}

-- systems/enemy_ai.lua
return {
    update = function(dt)
        local self = app.systems.enemy_ai
        local state = app.main.state

        bestow.entity.each(function(entity)
            if not bestow.entity.hasComponent(entity, "Enemy") then return end

            local enemy = bestow.entity.getComponent(entity, "Enemy")
            local pos = bestow.entity.getField(entity, "Transform3D", "position")

            if enemy.state == "idle" then
                -- Look for player
                if state.player and bestow.entity.isValid(state.player) then
                    local playerPos = bestow.entity.getField(state.player, "Transform3D", "position")
                    if (playerPos - pos):length() < enemy.detectRange then
                        enemy.state = "chase"
                        enemy.target = state.player
                    end
                end

            elseif enemy.state == "chase" then
                if not enemy.target or not bestow.entity.isValid(enemy.target) then
                    enemy.state = "idle"
                    enemy.target = nil
                else
                    local targetPos = bestow.entity.getField(enemy.target, "Transform3D", "position")
                    local toTarget = targetPos - pos
                    local distance = toTarget:length()

                    if distance > enemy.detectRange * 1.5 then
                        enemy.state = "idle"
                        enemy.target = nil
                    elseif distance < enemy.attackRange then
                        enemy.state = "attack"
                    else
                        -- Move toward target
                        local dir = toTarget:normalize()
                        local newPos = pos + dir * enemy.moveSpeed * dt
                        bestow.entity.setField(entity, "Transform3D", "position", newPos)

                        -- Face target
                        local angle = math.atan2(-dir.x, -dir.z)
                        bestow.entity.setField(entity, "Transform3D", "rotation",
                            Quat.fromAxisAngle(Vec3.up(), angle))
                    end
                end

            elseif enemy.state == "attack" then
                enemy.attackCooldown = enemy.attackCooldown - dt
                if enemy.attackCooldown <= 0 then
                    if enemy.target and bestow.entity.isValid(enemy.target) then
                        local targetPos = bestow.entity.getField(enemy.target, "Transform3D", "position")
                        if (targetPos - pos):length() < enemy.attackRange then
                            -- Deal damage
                            local health = bestow.entity.getComponent(enemy.target, "Health")
                            if health then
                                health.current = math.max(0, health.current - enemy.damage)
                                bestow.entity.setComponent(enemy.target, "Health", health)
                                bestow.events.emit("entity_damaged", {
                                    target = enemy.target,
                                    source = entity,
                                    amount = enemy.damage
                                })
                            end
                        else
                            enemy.state = "chase"
                        end
                    else
                        enemy.state = "idle"
                    end
                    enemy.attackCooldown = 1.0
                end
            end

            bestow.entity.setComponent(entity, "Enemy", enemy)
        end)
    end
}
