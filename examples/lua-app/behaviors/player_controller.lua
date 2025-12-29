-- behaviors/player_controller.lua
-- Player movement and input handling behavior

return {
    name = "player_controller",

    -- Called when the behavior is attached to an entity
    init = function(entity, state)
        -- Initialize state variables
        state.speed = config.getFloat("game.playerSpeed", 200)
        state.jumpForce = config.getFloat("game.jumpForce", 400)
        state.isGrounded = false
        state.facingRight = true

        print("Player controller initialized with speed: " .. state.speed)
    end,

    -- Called every frame
    update = function(entity, state, dt)
        local pos = entity:getPosition()
        local vel = entity:getVelocity()

        -- Horizontal movement (Dvorak: ,/O for left/right, A/E also supported)
        local moveX = 0
        if input.isKeyDown("a") or input.isKeyDown(",") then
            moveX = -1
            state.facingRight = false
        end
        if input.isKeyDown("e") or input.isKeyDown("o") then
            moveX = 1
            state.facingRight = true
        end

        -- Apply horizontal velocity
        vel.x = moveX * state.speed

        -- Jump (Dvorak: . for jump, Space also supported)
        if (input.isKeyPressed(".") or input.isKeyPressed("space")) and state.isGrounded then
            vel.y = state.jumpForce
            state.isGrounded = false
            audio.playSound("sounds/jump.wav")
        end

        -- Update entity velocity
        entity:setVelocity(vel.x, vel.y)

        -- Update facing direction (flip sprite)
        local scale = entity:getScale()
        if state.facingRight then
            entity:setScale(math.abs(scale.x), scale.y)
        else
            entity:setScale(-math.abs(scale.x), scale.y)
        end
    end,

    -- Called at fixed timestep for physics
    fixedUpdate = function(entity, state, dt)
        -- Ground check could be done here with physics queries
    end,

    -- Called when the behavior is hot-reloaded
    onReload = function(entity, state)
        -- Re-read config values
        state.speed = config.getFloat("game.playerSpeed", 200)
        state.jumpForce = config.getFloat("game.jumpForce", 400)
        print("Player controller reloaded - speed: " .. state.speed)
    end,

    -- Called when the behavior is detached
    destroy = function(entity, state)
        print("Player controller destroyed")
    end
}
