---
name: animation-system
description: Control skeletal animations, playback, blending, IK, sockets, ragdoll, and state machines in Bestow. Use when playing character animations, crossfading between animations, attaching weapons to bones, or implementing procedural animation.
---

# Animation System

## Loading Animated Models

### Quick Method: loadCharacter

The simplest way to get an animated character running:

```lua
local character = bestow.animation.loadCharacter({
    model = ":library:/models/character.glb",  -- Path to model with skeleton and animations
    position = Vec3.new(0, 0, 0),
    rotation = Quat.identity(),
    scale = Vec3.new(1, 1, 1)
})

-- Check if loaded
if character:isLoaded() then
    character:playAnimation("idle")
end

-- Control
character:setPosition(Vec3.new(5, 0, 0))
character:setRotation(Quat.fromAxisAngle(Vec3.up(), math.rad(90)))
character:crossfadeAnimation("walk", 0.2)
character:stopAnimation()
```

### Manual Method: Step by Step

For full control over the animation pipeline:

```lua
-- 1. Load model asset
local modelHandle = bestow.assets.loadModel(":library:/models/character.glb")
bestow.assets.loadAsset(modelHandle)

-- 2. Create skeleton from model data
local skeleton = bestow.animation.createSkeleton(modelHandle)

-- 3. Get available animations
local clipNames = bestow.assets.getAnimationNames(modelHandle)
-- e.g., {"idle", "walk", "run", "jump"}

-- 4. Create animator for playback
local animator = bestow.animation.createAnimator(skeleton)

-- 5. Store in state
app.main.state.playerAnimator = animator
app.main.state.playerSkeleton = skeleton
```

## Skeleton Info

```lua
local info = bestow.animation.getSkeletonInfo(skeleton)
-- info.boneCount, info.bones, info.rootBoneIndex, info.bounds
-- info:findBone("Spine") -> boneIndex or nil
-- info:getChildren(boneIndex) -> [boneIndex]

local boneIndex = bestow.animation.findBoneIndex(skeleton, "RightHand")
local boneNames = bestow.animation.getBoneNames(skeleton)
local count = bestow.animation.getBoneCount(skeleton)
```

## Animation Playback

### Basic Play

```lua
-- Play by clip name (with default 0.25s transition)
bestow.animation.play(animator, "idle")

-- Play with explicit transition time
bestow.animation.play(animator, "run", 0.3)

-- Play with full config
bestow.animation.play(animator, {
    clipName = "attack",      -- or clip = clipHandle
    speed = 1.0,
    weight = 1.0,
    layer = 0,
    wrapMode = "Once",        -- "Once", "Loop", "PingPong", "ClampForever"
    blendInTime = 0.2,
    blendOutTime = 0.2,
    startTime = 0.0,
    restartIfSame = true
})
```

### Stop and Pause

```lua
bestow.animation.stop(animator, fadeOutTime?)           -- Stop all layers
bestow.animation.stopLayer(animator, layerIndex, fade?) -- Stop specific layer
bestow.animation.setPaused(animator, true)
bestow.animation.setPaused(animator, false)
bestow.animation.isPaused(animator) -> bool
bestow.animation.isPlaying(animator) -> bool
bestow.animation.isLayerPlaying(animator, layer) -> bool
```

### Speed and Time Control

```lua
bestow.animation.setSpeed(animator, 1.5)              -- 1.0 = normal, 2.0 = double, -1 = reverse
bestow.animation.getSpeed(animator) -> float

bestow.animation.getCurrentTime(animator, layer?) -> float
bestow.animation.getNormalizedTime(animator, layer?) -> float  -- 0.0 to 1.0
bestow.animation.setCurrentTime(animator, time, layer?)
bestow.animation.setNormalizedTime(animator, 0.5, layer?)      -- Jump to middle
bestow.animation.getClipDuration(animator, layer?) -> float
```

## Animation Layers

Layers allow playing multiple animations simultaneously (e.g., run + aim):

```lua
-- Layer 0: Full body locomotion
bestow.animation.play(animator, { clipName = "run", layer = 0, wrapMode = "Loop" })

-- Layer 1: Upper body aiming (overrides upper body bones)
bestow.animation.play(animator, { clipName = "aim", layer = 1, blendMode = "Override" })

-- Set layer weight (0.0 = no effect, 1.0 = full)
bestow.animation.setLayerWeight(animator, 1, 0.8)
bestow.animation.getLayerWeight(animator, 1) -> float

-- Set blend mode: "Override" (replaces) or "Additive" (adds on top)
bestow.animation.setLayerBlendMode(animator, 1, "Additive")

-- Bone masks: restrict which bones a layer affects
bestow.animation.setLayerBoneMask(animator, 1, {
    "Spine", "Spine1", "Spine2", "Neck", "Head",
    "LeftShoulder", "LeftArm", "LeftForeArm", "LeftHand",
    "RightShoulder", "RightArm", "RightForeArm", "RightHand"
})

-- Get layer state
local state = bestow.animation.getLayerState(animator, 0)
-- state.clipName, state.time, state.normalizedTime, state.speed,
-- state.weight, state.fadeWeight, state.wrapMode, state.playing, state.finished
```

## Animation Events

Events fire at specific times during animation playback (defined in model or added at runtime):

```lua
-- Subscribe to animation events
local subId = bestow.animation.subscribeToEvents(animator, function(event)
    -- event.name, event.clipTime, event.normalizedTime
    -- event.stringParam, event.floatParam, event.intParam
    if event.name == "footstep" then
        local sound = event.stringParam == "left" and "step_l" or "step_r"
        bestow.audio.playOnChannel(3, { asset = stepHandle, volume = 0.5 })
    elseif event.name == "attack_hit" then
        checkDamage()
    end
end)

-- Subscribe to animation completion (one-shot animations)
local completeId = bestow.animation.subscribeToComplete(animator, function(animHandle, clipHandle, layer)
    -- Animation finished on this layer
    bestow.animation.play(animator, "idle", 0.15)
end)

-- Clean up
bestow.animation.unsubscribe(subId)
bestow.animation.unsubscribe(completeId)
```

## Sockets (Attachment Points)

Attach objects (weapons, effects) to animated bones:

```lua
-- Define a socket on a bone
bestow.animation.defineSocket(skeleton, {
    name = "right_hand_weapon",
    boneName = "RightHand",
    localPosition = Vec3.new(0, 0, 0),
    localRotation = Quat.identity(),
    localScale = Vec3.one(),
    attachMode = "FollowBone"  -- "FollowBone", "FollowPosition", "FollowRotation", "WorldSpace"
})

-- Check if socket exists
bestow.animation.hasSocket(skeleton, "right_hand_weapon") -> bool

-- Get socket world transform (needs entity world matrix)
local entityMatrix = Mat4.fromTransform(entityTransform)
local socket = bestow.animation.getSocketTransform(animator, "right_hand_weapon", entityMatrix)
if socket then
    -- socket.position, socket.rotation, socket.scale
    -- socket.forward, socket.up, socket.right
    -- socket.worldMatrix
    bestow.entity.setField(weapon, "Transform3D", "position", socket.position)
    bestow.entity.setField(weapon, "Transform3D", "rotation", socket.rotation)
end
```

### Socket Raycasting

Cast rays from sockets for melee weapon hit detection:

```lua
local result = bestow.animation.raycastFromSocket(animator, {
    socketName = "sword_tip",
    direction = Vec3.new(0, 0, 1),  -- Local forward
    maxDistance = 1.5,
    collisionMask = 0xFFFF
}, entityMatrix, bestow.physics3d)

if result.hit then
    -- result.hitPoint, result.hitNormal, result.distance, result.hitEntity
    applyDamage(result.hitEntity)
end
```

## Inverse Kinematics (IK)

### Two-Bone IK (Arms/Legs)

```lua
-- Define IK chain
bestow.animation.defineIKChain(skeleton, {
    name = "right_arm",
    rootBoneName = "RightArm",
    midBoneName = "RightForeArm",
    tipBoneName = "RightHand"
})

-- Set target in update
bestow.animation.setIKTarget(animator, {
    chainName = "right_arm",
    targetPosition = worldTargetPos,
    poleVector = elbowHintPos,   -- Optional: controls elbow direction
    weight = 1.0,
    enabled = true
})

-- Adjust weight for blending
bestow.animation.setIKWeight(animator, "right_arm", 0.5)

-- Clear target
bestow.animation.clearIKTarget(animator, "right_arm")
bestow.animation.clearAllIKTargets(animator)
```

### Aim IK (Look At)

```lua
-- Define aim constraint
bestow.animation.defineIKAim(skeleton, {
    name = "head_look",
    boneName = "Head",
    aimAxis = Vec3.new(0, 0, 1),    -- Which axis points forward
    upAxis = Vec3.new(0, 1, 0),
    horizontalLimit = math.rad(70),
    verticalLimit = math.rad(45)
})

-- Set look target
bestow.animation.setIKTarget(animator, {
    configName = "head_look",
    targetPosition = enemyHeadPos,
    worldUp = Vec3.up(),
    weight = 0.8,
    enabled = true
})
```

## Root Motion

Extract movement from animation data and apply to game entity:

```lua
-- Enable root motion
bestow.animation.setRootMotionEnabled(animator, true)

-- Configure what to extract
bestow.animation.setRootMotionConfig(animator, {
    enabled = true,
    extractTranslationX = true,
    extractTranslationY = false,   -- Usually don't extract vertical
    extractTranslationZ = true,
    extractRotationY = true,
    extractRotationXZ = false,
    rootBoneName = "Root"
})

-- In update: consume and apply root motion
local rootMotion = bestow.animation.getRootMotion(animator)
if rootMotion.hasTranslation then
    local pos = bestow.entity.getField(entity, "Transform3D", "position")
    pos = pos + rootMotion.deltaPosition
    bestow.entity.setField(entity, "Transform3D", "position", pos)
end
if rootMotion.hasRotation then
    local rot = bestow.entity.getField(entity, "Transform3D", "rotation")
    rot = rootMotion.deltaRotation * rot
    bestow.entity.setField(entity, "Transform3D", "rotation", rot)
end
bestow.animation.consumeRootMotion(animator)  -- Reset delta
```

## Ragdoll

Switch between animation and physics-driven ragdoll:

```lua
-- Create ragdoll from skeleton (usually once in init)
bestow.animation.createRagdoll(entity, ragdollDef, bestow.physics3d)

-- Activate ragdoll (e.g., on death)
bestow.animation.activateRagdoll(entity, bestow.physics3d)               -- Instant
bestow.animation.activateRagdoll(entity, bestow.physics3d, false, 0.3)   -- Blend over 0.3s

-- Deactivate (return to animation)
bestow.animation.deactivateRagdoll(entity, bestow.physics3d, false, 0.3)

-- Blend weight (0 = animation, 1 = ragdoll)
bestow.animation.setRagdollBlendWeight(entity, 0.5)
bestow.animation.isRagdollActive(entity) -> bool

-- Apply hit impulse
bestow.animation.applyBoneImpulse(entity, "Spine", Vec3.new(0, 0, 100), bestow.physics3d)

-- Clean up
bestow.animation.destroyRagdoll(entity, bestow.physics3d)
```

## Bone Transforms

Access bone positions for effects, targeting, etc.:

```lua
local boneTransform = bestow.animation.getBoneTransform(animator, "Head")
local worldTransform = bestow.animation.getBoneWorldTransform(animator, boneIndex, entityWorldMatrix)
-- Returns Mat4
```

## Animation State Machine Pattern

A typical character controller with animation states:

```lua
-- systems/player_animation.lua
local BLEND_TIME = 0.2

return {
    currentState = "idle",

    update = function(dt)
        local self = app.systems.player_animation
        local state = app.main.state
        if not state.player or not state.playerAnimator then return end

        local animator = state.playerAnimator
        local velocity = bestow.entity.getComponent(state.player, "Velocity")
        local ground = bestow.physics3d.getCharacterGroundInfo(state.player)
        local grounded = ground and ground.grounded

        -- Determine target state
        local target = "idle"
        if not grounded then
            target = (velocity and velocity.linear.y > 0) and "jump" or "fall"
        else
            local speed = velocity and Vec3.new(velocity.linear.x, 0, velocity.linear.z):length() or 0
            if speed > 5 then target = "run"
            elseif speed > 0.5 then target = "walk"
            else target = "idle" end
        end

        -- Transition if state changed
        if target ~= self.currentState then
            local wrapMode = (target == "jump" or target == "fall") and "Once" or "Loop"
            bestow.animation.play(animator, {
                clipName = target,
                wrapMode = wrapMode,
                blendInTime = BLEND_TIME
            })
            self.currentState = target
        end
    end
}
```

## Best Practices

1. **Use `loadCharacter` for simple cases** - handles skeleton, animator, clips automatically
2. **Use short crossfade times** - 0.1-0.3 seconds for responsive transitions
3. **Use animation events for timing** - Don't guess when attacks connect
4. **Subscribe to completion** for one-shot animations (attacks, jumps)
5. **Use layers for partial body** - Run on layer 0, aim on layer 1 with upper body mask
6. **Store animator handles in `app.main.state`** - Survives hot reload
7. **Consume root motion every frame** - Or it accumulates
8. **Clean up subscriptions** - Unsubscribe when entities are destroyed
