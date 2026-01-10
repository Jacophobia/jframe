--[[
    Player Module

    Handles player state, animation state machine, and movement.
    Animation states transition based on movement speed and ground status.

    State Machine:
      Locomotion: idle <-> walk <-> jog <-> run (based on normalized speed)
      Jumping: grounded -> jump -> falling -> landing -> recovery -> grounded

    Uses the simplified Character API for animation management.
]]

-- Animation states
local AnimState = {
    Idle = "idle",
    Walk = "walk",
    Jog = "jog",
    Run = "run",
    Jump = "jump",
    Falling = "falling",
    Landing = "landing",
    LandingRecovery = "landingRecovery",
}

-- Player module
local player = {}

-- Create and initialize player state
function player.create(appSelf, scope)
    local config = app.config

    -- Create player state table
    local p = {
        -- Position and orientation
        position = Vec3.new(0, 0, 0),
        rotation = 0,  -- Radians, Y-axis rotation

        -- Velocity
        velocity = Vec3.new(0, 0, 0),

        -- Input state
        inputDir = { x = 0, z = 0 },
        runModifier = false,

        -- Physics state
        isGrounded = true,
        verticalVelocity = 0,

        -- Animation state
        currentState = AnimState.Idle,
        previousState = AnimState.Idle,
        stateTime = 0,
        currentSpeed = 0,  -- Normalized 0-1

        -- Character (filled by loadAssets)
        character = nil,
    }

    -- Store in app's transient state
    appSelf.transient.player = p

    -- Load character model and animations
    player.loadAssets(appSelf, scope)

    return p
end

-- Load character model and animation clips using simplified Character API
function player.loadAssets(appSelf, scope)
    local config = app.config
    local p = appSelf.transient.player

    bestow.info("Loading character with simplified API...")

    -- Load character with single call - animations loaded by name
    local char = bestow.animation.loadCharacter({
        model = config.assets.character,
        animations = {
            idle = config.assets.animations.idle,
            walk = config.assets.animations.walk,
            jog = config.assets.animations.jog,
            run = config.assets.animations.run,
            jump = config.assets.animations.jump,
            falling = config.assets.animations.falling,
            landing = config.assets.animations.landing,
            landingRecovery = config.assets.animations.landingRecovery,
        }
    })

    if not char then
        bestow.error("Failed to load character")
        return
    end

    p.character = char

    -- Set default blend time from config
    char:setDefaultBlendTime(config.animation.blendTimes.locomotion)

    -- Add animation events for footsteps (demonstration)
    char:addEvent("walk", 0.0, "footstep", { foot = "left" })
    char:addEvent("walk", 0.5, "footstep", { foot = "right" })
    char:addEvent("run", 0.0, "footstep", { foot = "left" })
    char:addEvent("run", 0.5, "footstep", { foot = "right" })

    -- Subscribe to footstep events (hot-reload safe)
    char:on("footstep", appSelf, "onFootstep")

    -- Log available animations
    local anims = char:getAvailableAnimations()
    bestow.info("Character loaded with animations:", table.concat(anims, ", "))

    -- Start with idle animation
    char:play(AnimState.Idle, 0)
end

-- Trigger jump
function player.jump(appSelf, scope)
    local p = appSelf.transient.player
    local config = app.config

    if not p or not p.isGrounded then return end

    p.isGrounded = false
    p.verticalVelocity = config.physics.jumpForce

    -- Transition to jump animation
    player.changeState(appSelf, AnimState.Jump)
end

-- Change animation state
function player.changeState(appSelf, newState)
    local p = appSelf.transient.player
    local config = app.config
    if not p or not p.character then return end

    if p.currentState == newState then return end

    p.previousState = p.currentState
    p.currentState = newState
    p.stateTime = 0

    -- Get appropriate blend time
    local blendTime = config.animation.blendTimes.locomotion
    local from = p.previousState
    local to = newState

    if to == AnimState.Jump then
        blendTime = config.animation.blendTimes.jumpStart
    elseif from == AnimState.Jump and to == AnimState.Falling then
        blendTime = config.animation.blendTimes.jumpToFall
    elseif from == AnimState.Falling and to == AnimState.Landing then
        blendTime = config.animation.blendTimes.land
    elseif from == AnimState.Landing and to == AnimState.LandingRecovery then
        blendTime = config.animation.blendTimes.landToRecovery
    elseif from == AnimState.LandingRecovery then
        blendTime = config.animation.blendTimes.recoveryToIdle
    end

    bestow.info("State: " .. from .. " -> " .. to .. " (blend: " .. blendTime .. "s)")
    p.character:play(newState, blendTime)
end

-- Get target state based on speed (for grounded locomotion)
function player.getLocomotionState(normalizedSpeed)
    local config = app.config
    local thresholds = config.animation

    if normalizedSpeed < thresholds.idleThreshold then
        return AnimState.Idle
    elseif normalizedSpeed < thresholds.walkThreshold then
        return AnimState.Walk
    elseif normalizedSpeed < thresholds.jogThreshold then
        return AnimState.Jog
    else
        return AnimState.Run
    end
end

-- Check if current state is a locomotion state
function player.isLocomotionState(state)
    return state == AnimState.Idle or
           state == AnimState.Walk or
           state == AnimState.Jog or
           state == AnimState.Run
end

-- Update player state and animation
function player.update(appSelf, dt, scope)
    local p = appSelf.transient.player
    local config = app.config
    if not p then return end

    local movement = config.movement
    local physics = config.physics

    -- Accumulate input from continuous actions
    local inputX = 0
    local inputZ = 0

    -- Check for active movement actions
    if bestow.action.isActive("MoveForward") then
        inputZ = inputZ + 1
    end
    if bestow.action.isActive("MoveBackward") then
        inputZ = inputZ - 1
    end
    if bestow.action.isActive("MoveLeft") then
        inputX = inputX - 1
    end
    if bestow.action.isActive("MoveRight") then
        inputX = inputX + 1
    end

    p.runModifier = bestow.action.isActive("Run")

    -- Normalize input direction
    local inputMag = math.sqrt(inputX * inputX + inputZ * inputZ)
    if inputMag > 0.001 then
        inputX = inputX / inputMag
        inputZ = inputZ / inputMag
    else
        inputX = 0
        inputZ = 0
    end

    -- Determine target speed
    local targetSpeed = 0
    if inputMag > 0.001 then
        if p.runModifier then
            targetSpeed = movement.runSpeed
        else
            targetSpeed = movement.walkSpeed
        end
    end

    -- Accelerate/decelerate horizontal movement
    if inputMag > 0.001 then
        p.currentSpeed = p.currentSpeed + movement.acceleration * dt
        if p.currentSpeed > 1.0 then p.currentSpeed = 1.0 end
    else
        p.currentSpeed = p.currentSpeed - movement.deceleration * dt
        if p.currentSpeed < 0 then p.currentSpeed = 0 end
    end

    -- Calculate actual movement speed
    local maxSpeed = p.runModifier and movement.runSpeed or movement.walkSpeed
    local actualSpeed = p.currentSpeed * maxSpeed

    -- Update horizontal velocity
    if inputMag > 0.001 then
        p.velocity.x = inputX * actualSpeed
        p.velocity.z = inputZ * actualSpeed
    else
        -- Decelerate to stop
        local vel = math.sqrt(p.velocity.x * p.velocity.x + p.velocity.z * p.velocity.z)
        if vel > 0.001 then
            local decel = movement.deceleration * dt
            local newVel = vel - decel
            if newVel < 0 then newVel = 0 end
            local scale = newVel / vel
            p.velocity.x = p.velocity.x * scale
            p.velocity.z = p.velocity.z * scale
        else
            p.velocity.x = 0
            p.velocity.z = 0
        end
    end

    -- Apply gravity and update vertical position
    if not p.isGrounded then
        p.verticalVelocity = p.verticalVelocity + physics.gravity * dt
        p.position.y = p.position.y + p.verticalVelocity * dt

        -- Check ground collision
        if p.position.y <= physics.groundY then
            p.position.y = physics.groundY
            p.verticalVelocity = 0
            p.isGrounded = true
        end
    end

    -- Update horizontal position
    p.position.x = p.position.x + p.velocity.x * dt
    p.position.z = p.position.z + p.velocity.z * dt

    -- Rotate character to face movement direction
    if inputMag > 0.001 then
        local targetRotation = math.atan2(inputX, inputZ)
        local rotDiff = targetRotation - p.rotation

        -- Normalize to -pi to pi
        while rotDiff > math.pi do rotDiff = rotDiff - 2 * math.pi end
        while rotDiff < -math.pi do rotDiff = rotDiff + 2 * math.pi end

        -- Smooth rotation
        local rotStep = movement.turnSpeed * dt
        if math.abs(rotDiff) < rotStep then
            p.rotation = targetRotation
        else
            if rotDiff > 0 then
                p.rotation = p.rotation + rotStep
            else
                p.rotation = p.rotation - rotStep
            end
        end

        -- Normalize rotation
        while p.rotation > math.pi do p.rotation = p.rotation - 2 * math.pi end
        while p.rotation < -math.pi do p.rotation = p.rotation + 2 * math.pi end
    end

    -- Update state time
    p.stateTime = p.stateTime + dt

    -- Animation state machine logic
    player.updateStateMachine(appSelf, dt)

    -- Update character (processes animation events)
    if p.character then
        p.character:update(dt)
    end
end

-- Animation state machine update
function player.updateStateMachine(appSelf, dt)
    local p = appSelf.transient.player
    local config = app.config
    if not p or not p.character then return end

    local state = p.currentState

    if p.isGrounded then
        -- Grounded states
        if player.isLocomotionState(state) then
            -- Update locomotion based on speed
            local targetState = player.getLocomotionState(p.currentSpeed)
            if targetState ~= state then
                player.changeState(appSelf, targetState)
            end
        elseif state == AnimState.Landing then
            -- Wait for landing animation to finish
            local progress = p.character:getAnimationProgress()
            if progress >= 0.95 then
                player.changeState(appSelf, AnimState.LandingRecovery)
            end
        elseif state == AnimState.LandingRecovery then
            -- Wait for recovery animation to finish
            local progress = p.character:getAnimationProgress()
            if progress >= 0.95 then
                -- Transition to appropriate locomotion state
                local targetState = player.getLocomotionState(p.currentSpeed)
                player.changeState(appSelf, targetState)
            end
        end
    else
        -- Airborne states
        if state == AnimState.Jump then
            -- Wait for jump animation to finish, then transition to falling
            local progress = p.character:getAnimationProgress()
            if progress >= 0.9 then
                player.changeState(appSelf, AnimState.Falling)
            end
        elseif state == AnimState.Falling then
            -- Will transition to Landing when grounded (handled above)
        elseif player.isLocomotionState(state) then
            -- Just left ground (walked off edge) - go to falling
            player.changeState(appSelf, AnimState.Falling)
        end

        -- Check for landing
        if p.isGrounded and (state == AnimState.Jump or state == AnimState.Falling) then
            player.changeState(appSelf, AnimState.Landing)
        end
    end
end

-- Render the player
function player.render(appSelf, scope)
    local p = appSelf.transient.player
    if not p or not p.character then return end

    -- Update character transform and draw
    p.character:setPosition(p.position.x, p.position.y, p.position.z)
    p.character:setRotation(p.rotation)
    p.character:draw()
end

-- Cleanup player resources
function player.destroy(appSelf, scope)
    local p = appSelf.transient.player
    if not p then return end

    if p.character then
        p.character:destroy()
    end

    appSelf.transient.player = nil
end

return player
