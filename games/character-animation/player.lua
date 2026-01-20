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

        -- Physics entity (for character controller)
        entity = nil,
    }

    -- Store in app's transient state
    appSelf.transient.player = p

    -- Create physics character controller
    player.createPhysicsController(appSelf, config)

    -- Load character model and animations
    player.loadAssets(appSelf, scope)

    return p
end

-- Create physics character controller for collision
function player.createPhysicsController(appSelf, config)
    local p = appSelf.transient.player
    local charConfig = config.character

    -- Create entity for physics
    p.entity = bestow.entity.create()

    -- Create character controller definition
    local charDef = CharacterControllerDef.new()
    charDef.radius = charConfig.radius
    charDef.height = charConfig.height
    charDef.stepHeight = charConfig.stepHeight
    charDef.maxSlopeAngle = charConfig.maxSlopeAngle
    charDef.mass = charConfig.mass
    charDef.layer = bestow.physics3d.Layer.Character

    -- Create the character controller
    bestow.physics3d.createCharacter(p.entity, charDef)

    bestow.info("Physics character controller created")
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

    -- Scale character from centimeters to meters (Mixamo exports in cm)
    char:setScale(0.01)

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
    if bestow.input.isActionActive("MoveForward") then
        inputZ = inputZ + 1
    end
    if bestow.input.isActionActive("MoveBackward") then
        inputZ = inputZ - 1
    end
    if bestow.input.isActionActive("MoveLeft") then
        inputX = inputX - 1
    end
    if bestow.input.isActionActive("MoveRight") then
        inputX = inputX + 1
    end

    p.runModifier = bestow.input.isActionActive("Run")

    -- Normalize input direction
    local inputMag = math.sqrt(inputX * inputX + inputZ * inputZ)
    if inputMag > 0.001 then
        inputX = inputX / inputMag
        inputZ = inputZ / inputMag
    else
        inputX = 0
        inputZ = 0
    end

    -- Transform input to world space using camera angle (camera-relative movement)
    -- Camera orbit angle: 0 = behind player (looking +Z), π/2 = left of player (looking +X)
    local cam = appSelf.transient.camera
    local camAngle = cam and cam.currentOrbitAngle or 0

    local worldInputX = 0
    local worldInputZ = 0
    if inputMag > 0.001 then
        local cosA = math.cos(camAngle)
        local sinA = math.sin(camAngle)
        -- Forward (inputZ) maps to camera's look direction, Right (inputX) perpendicular
        worldInputX = inputZ * sinA + inputX * cosA
        worldInputZ = inputZ * cosA - inputX * sinA
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

    -- Update horizontal velocity using world-space input
    if inputMag > 0.001 then
        p.velocity.x = worldInputX * actualSpeed
        p.velocity.z = worldInputZ * actualSpeed
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

    -- Apply gravity
    if not p.isGrounded then
        p.verticalVelocity = p.verticalVelocity + physics.gravity * dt
    end

    -- Move character using physics controller
    local velocity3D = Vec3.new(p.velocity.x, p.verticalVelocity, p.velocity.z)
    bestow.physics3d.moveCharacter(p.entity, velocity3D, dt)

    -- Get position from physics
    local newPos = bestow.physics3d.getCharacterPosition(p.entity)
    if newPos then
        p.position = newPos
    end

    -- Get ground state from physics
    local groundInfo = bestow.physics3d.getCharacterGroundInfo(p.entity)
    if groundInfo then
        local wasGrounded = p.isGrounded
        p.isGrounded = (groundInfo.state == CharacterGroundState.OnGround)
        -- Reset vertical velocity when landing
        if p.isGrounded and not wasGrounded then
            p.verticalVelocity = 0
        end
    end

    -- Rotate character to face movement direction (using world-space input)
    if inputMag > 0.001 then
        local targetRotation = math.atan2(worldInputX, worldInputZ)
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

    -- Destroy physics entity
    if p.entity then
        bestow.entity.destroy(p.entity)
    end

    appSelf.transient.player = nil
end

return player
