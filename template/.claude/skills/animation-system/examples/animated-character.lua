-- Example: Animated Character Setup
-- Shows loading, playing, blending animations, sockets, and state machine pattern
-- All animation functions use the handle-based bestow.animation.* API

-- systems/player_animation.lua
local BLEND_TIME = 0.2

return {
    currentState = "idle",
    eventSubId = nil,
    completeSubId = nil,

    init = function()
        local state = app.main.state

        -- loadCharacter takes a SINGLE STRING path argument
        -- Returns a plain table: { skeleton, animator, clips, modelHandle, mesh, material }
        local char = bestow.animation.loadCharacter(":library:/models/character.glb")
        if not char then
            print("Failed to load character model")
            return
        end

        -- Store all handles in state (survives hot reload)
        state.skeleton = char.skeleton
        state.animator = char.animator
        state.clips = char.clips         -- table: { idle = handle, walk = handle, ... }
        state.charMesh = char.mesh
        state.charMaterial = char.material

        -- Play idle animation using the handle-based API
        bestow.animation.play(state.animator, "idle")

        -- Subscribe to animation events (footsteps, etc.)
        -- subscribeToEvents takes an animator handle and a callback function
        local self = app.systems.player_animation
        self.eventSubId = bestow.animation.subscribeToEvents(state.animator, function(event)
            -- event is an AnimationEvent with fields:
            --   .animator, .clip, .layer, .name
            --   .clipTime, .normalizedTime
            --   .stringParam, .floatParam, .intParam
            if event.name == "footstep_left" or event.name == "footstep_right" then
                local pos = bestow.entity.getField(app.main.state.player, "Transform3D", "position")
                bestow.audio.playPositional({
                    asset = app.main.state.sounds.footstep,
                    position = pos,
                    volume = 0.5,
                    minDistance = 1.0,
                    maxDistance = 15.0
                })
            end
        end)

        -- Subscribe to animation completion (for one-shot animations)
        -- Callback receives: (animatorHandle, clipHandle, layer)
        self.completeSubId = bestow.animation.subscribeToComplete(state.animator, function(animatorHandle, clipHandle, layer)
            -- Check if the completed clip was "attack" by looking up the clip info
            local clipInfo = bestow.animation.getAnimationClipInfo(clipHandle)
            if clipInfo and clipInfo.name == "attack" then
                bestow.animation.play(animatorHandle, "idle", 0.2)
            end
        end)

        -- Define a weapon socket on the right hand bone
        local socketDef = SocketDef.new()
        socketDef.name = "right_hand_weapon"
        socketDef.boneName = "RightHand"
        socketDef.localPosition = Vec3.new(0, 0, 0)
        socketDef.localRotation = Quat.identity()
        socketDef.localScale = Vec3.new(1, 1, 1)
        socketDef.attachMode = SocketAttachMode.FollowBone
        bestow.animation.defineSocket(state.skeleton, socketDef)
    end,

    update = function(dt)
        local self = app.systems.player_animation
        local state = app.main.state
        if not state.animator then return end

        -- Simple state machine: pick animation based on movement
        local groundInfo = bestow.physics3d.getCharacterGroundInfo(state.player)
        local grounded = groundInfo and groundInfo.state == CharacterGroundState.OnGround

        local moveX, moveZ = 0, 0
        if bestow.input.isActionActive("MoveLeft") then moveX = moveX - 1 end
        if bestow.input.isActionActive("MoveRight") then moveX = moveX + 1 end
        if bestow.input.isActionActive("MoveForward") then moveZ = moveZ - 1 end
        if bestow.input.isActionActive("MoveBack") then moveZ = moveZ + 1 end
        local moving = (moveX ~= 0 or moveZ ~= 0)
        local sprinting = bestow.input.isActionActive("Sprint")

        -- Determine target animation state
        local target = "idle"
        if not grounded then
            target = "jump"
        elseif moving and sprinting then
            target = "run"
        elseif moving then
            target = "walk"
        end

        -- Only transition if state changed (avoid restarting same animation)
        if target ~= self.currentState then
            -- Use AnimationPlayConfig for full control
            local config = AnimationPlayConfig.new()
            config.clipName = target
            config.blendInTime = BLEND_TIME

            -- One-shot for jump, loop for everything else
            if target == "jump" then
                config.wrapMode = AnimationWrapMode.Once
            else
                config.wrapMode = AnimationWrapMode.Loop
            end

            bestow.animation.play(state.animator, config)
            self.currentState = target
        end

        -- Update weapon socket position (if weapon entity exists)
        if state.weapon then
            local entityTransform = bestow.entity.getField(state.player, "Transform3D", "matrix")
            local socketTransform = bestow.animation.getSocketTransform(
                state.animator, "right_hand_weapon", entityTransform
            )
            if socketTransform then
                bestow.entity.setField(state.weapon, "Transform3D", "position", socketTransform.position)
                bestow.entity.setField(state.weapon, "Transform3D", "rotation", socketTransform.rotation)
            end
        end
    end,

    render = function()
        local state = app.main.state
        if not state.animator or not state.charMesh then return end

        -- Render the animated character using drawCharacterTransform
        local transform = bestow.entity.getComponent(state.player, "Transform3D")
        if transform then
            bestow.animation.drawCharacterTransform(
                state.animator, state.charMesh, state.charMaterial, transform
            )
        end
    end,

    shutdown = function()
        local self = app.systems.player_animation

        -- Clean up subscriptions
        if self.eventSubId then
            bestow.animation.unsubscribe(self.eventSubId)
            self.eventSubId = nil
        end
        if self.completeSubId then
            bestow.animation.unsubscribe(self.completeSubId)
            self.completeSubId = nil
        end

        -- Clean up animation resources
        local state = app.main.state
        if state.animator then
            bestow.animation.destroyAnimator(state.animator)
            state.animator = nil
        end
        if state.skeleton then
            bestow.animation.destroySkeleton(state.skeleton)
            state.skeleton = nil
        end
    end
}
