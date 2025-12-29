-- behaviors/enemy_ai.lua
-- Simple enemy AI behavior with patrol and chase logic

return {
    name = "enemy_ai",

    init = function(entity, state)
        state.speed = 100
        state.patrolDistance = 200
        state.detectionRange = 300
        state.startX = entity:getPosition().x
        state.movingRight = true
        state.chasing = false

        print("Enemy AI initialized")
    end,

    update = function(entity, state, dt)
        local pos = entity:getPosition()
        local player = engine.findByName("MainPlayer")

        -- Check for player detection
        if player and player:isValid() then
            local playerPos = player:getPosition()
            local distance = mathx.distance(pos.x, pos.y, playerPos.x, playerPos.y)

            if distance < state.detectionRange then
                state.chasing = true
            else
                state.chasing = false
            end
        else
            state.chasing = false
        end

        local vel = entity:getVelocity()

        if state.chasing and player then
            -- Chase the player
            local playerPos = player:getPosition()
            local direction = mathx.sign(playerPos.x - pos.x)
            vel.x = direction * state.speed * 1.5  -- Move faster when chasing
        else
            -- Patrol behavior
            if state.movingRight then
                if pos.x > state.startX + state.patrolDistance then
                    state.movingRight = false
                end
                vel.x = state.speed
            else
                if pos.x < state.startX - state.patrolDistance then
                    state.movingRight = true
                end
                vel.x = -state.speed
            end
        end

        entity:setVelocity(vel.x, vel.y)
    end,

    onReload = function(entity, state)
        print("Enemy AI reloaded")
    end
}
