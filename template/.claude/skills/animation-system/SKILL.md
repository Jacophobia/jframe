---
name: animation-system
description: Control skeletal animations, playback, blending, IK, sockets, ragdoll, and state machines in Bestow. Use when playing character animations, crossfading between animations, attaching weapons to bones, or implementing procedural animation.
---

# Animation System

The animation system uses a **handle-based API**. All functions live in the `bestow.animation.*` namespace and operate on handles (skeleton, animator, clip, socket). There are no OOP methods on returned objects.

## Loading Animated Models

### Quick Method: loadCharacter

The simplest way to get an animated character running. Takes a **single string argument** (the file path) and returns a plain table of handles:

```lua
local char = bestow.animation.loadCharacter(":library:/models/character.glb")

if char then
    -- char is a plain table with these keys:
    --   char.skeleton   (SkeletonHandle)
    --   char.animator   (AnimatorHandle)
    --   char.clips      (table: clip name -> AnimationClipHandle)
    --   char.modelHandle (AssetHandle)
    --   char.mesh       (MeshHandle, 0 if no graphics)
    --   char.material   (MaterialHandle, 0 if no graphics)

    -- Play an animation using the handle-based API
    bestow.animation.play(char.animator, "idle")
end
```

### Manual Method: Step by Step

For full control over the animation pipeline:

```lua
-- 1. Load model via asset system
local modelHandle = bestow.assets.loadModel(":library:/models/character.glb")

-- 2. Create skeleton from model data
local skeleton = bestow.animation.createSkeletonFromModel(modelHandle)

-- 3. Load animation clips from the model into the skeleton
--    Returns a table: { idle = clipHandle, walk = clipHandle, ... }
local clips = bestow.animation.loadClipsFromModel(skeleton, modelHandle)

-- 4. Create animator for playback
local animator = bestow.animation.createAnimator(skeleton)

-- 5. Store handles in state
app.main.state.playerAnimator = animator
app.main.state.playerSkeleton = skeleton
app.main.state.playerClips = clips
```

### Loading Clips from Separate Files

```lua
-- Load a single clip from a file (uses first animation if clipName is nil)
local runClip = bestow.animation.loadClip(skeleton, ":library:/animations/run.fbx")

-- Load a specific named clip from a file
local idleClip = bestow.animation.loadClip(skeleton, ":library:/animations/pack.fbx", "idle")
```

## Skeleton Info

```lua
local info = bestow.animation.getSkeletonInfo(skeleton)
-- info.boneCount, info.bones, info.rootBoneIndex, info.bounds
-- info:findBone("Spine") -> boneIndex or nil
-- info:getChildren(boneIndex) -> {boneIndex, ...}

local boneIndex = bestow.animation.findBoneIndex(skeleton, "RightHand")
local boneNames = bestow.animation.getBoneNames(skeleton)
local count = bestow.animation.getBoneCount(skeleton)

bestow.animation.isValidSkeleton(skeleton) -- -> bool
```

## Animation Playback

### Basic Play

```lua
-- Play by clip name (with default 0.25s crossfade)
bestow.animation.play(animator, "idle")

-- Play with explicit transition time
bestow.animation.play(animator, "run", 0.3)

-- Play by clip handle
bestow.animation.play(animator, clips.run, 0.2)

-- Play with full AnimationPlayConfig
local config = AnimationPlayConfig.new()
config.clipName = "attack"        -- or: config.clip = clipHandle
config.speed = 1.0
config.weight = 1.0
config.layer = 0
config.wrapMode = AnimationWrapMode.Once  -- Once, Loop, PingPong, ClampForever
config.blendMode = AnimationBlendMode.Override  -- Override, Additive
config.blendInTime = 0.2
config.blendOutTime = 0.2
config.startTime = 0.0
config.restartIfSame = true
bestow.animation.play(animator, config)
```

### Stop and Pause

```lua
bestow.animation.stop(animator)              -- Stop all layers (instant)
bestow.animation.stop(animator, 0.3)         -- Stop all layers with 0.3s fade out
bestow.animation.stopLayer(animator, 1)      -- Stop specific layer (instant)
bestow.animation.stopLayer(animator, 1, 0.2) -- Stop specific layer with fade out

bestow.animation.setPaused(animator, true)
bestow.animation.setPaused(animator, false)
bestow.animation.isPaused(animator)           -- -> bool
bestow.animation.isPlaying(animator)          -- -> bool
bestow.animation.isLayerPlaying(animator, 0)  -- -> bool
```

### Speed and Time Control

```lua
bestow.animation.setSpeed(animator, 1.5)              -- 1.0 = normal, 2.0 = double, -1 = reverse
bestow.animation.getSpeed(animator)                    -- -> float

bestow.animation.getCurrentTime(animator)              -- -> float (layer 0 by default)
bestow.animation.getCurrentTime(animator, 1)           -- -> float (specific layer)
bestow.animation.getNormalizedTime(animator)            -- -> float 0.0 to 1.0
bestow.animation.setCurrentTime(animator, 0.5)         -- Set time on layer 0
bestow.animation.setCurrentTime(animator, 0.5, 1)      -- Set time on specific layer
bestow.animation.setNormalizedTime(animator, 0.5)       -- Jump to middle
bestow.animation.getClipDuration(animator)              -- -> float (layer 0)
```

## Animation Layers

Layers allow playing multiple animations simultaneously (e.g., run + aim):

```lua
-- Layer 0: Full body locomotion
local locoConfig = AnimationPlayConfig.new()
locoConfig.clipName = "run"
locoConfig.layer = 0
locoConfig.wrapMode = AnimationWrapMode.Loop
bestow.animation.play(animator, locoConfig)

-- Layer 1: Upper body aiming (overrides upper body bones)
local aimConfig = AnimationPlayConfig.new()
aimConfig.clipName = "aim"
aimConfig.layer = 1
aimConfig.blendMode = AnimationBlendMode.Override
bestow.animation.play(animator, aimConfig)

-- Set layer weight (0.0 = no effect, 1.0 = full)
bestow.animation.setLayerWeight(animator, 1, 0.8)
bestow.animation.getLayerWeight(animator, 1) -- -> float

-- Set blend mode
bestow.animation.setLayerBlendMode(animator, 1, AnimationBlendMode.Additive)

-- Get layer count
bestow.animation.getLayerCount(animator) -- -> int

-- Get layer state
local state = bestow.animation.getLayerState(animator, 0)
-- state.clip           (AnimationClipHandle)
-- state.clipName       (string)
-- state.time           (float)
-- state.normalizedTime (float)
-- state.speed          (float)
-- state.weight         (float)
-- state.fadeWeight      (float)
-- state.wrapMode       (AnimationWrapMode)
-- state.blendMode      (AnimationBlendMode)
-- state.playing        (bool)
-- state.paused         (bool)
-- state.finished       (bool)
-- state.effectiveWeight (float)
-- state.isActive       (bool)
```

## Rendering Animated Characters

Use `drawCharacter` or `drawCharacterTransform` to render a skinned mesh with bone transforms:

```lua
-- Using a Mat4 world matrix
bestow.animation.drawCharacter(char.animator, char.mesh, char.material, worldMatrix)

-- Using a Transform3D
bestow.animation.drawCharacterTransform(char.animator, char.mesh, char.material, transform)
```

## Animation Events

Events fire at specific times during animation playback:

```lua
-- Add an event to a clip at runtime
local eventDef = AnimationEventDef.new()
eventDef.name = "footstep"
eventDef.time = 0.4              -- Time in seconds within the clip
eventDef.stringParam = "left"    -- Optional string parameter
eventDef.floatParam = 0.5        -- Optional float parameter
eventDef.intParam = 0            -- Optional int parameter
bestow.animation.addClipEvent(clipHandle, eventDef)

-- Remove a specific event by name
bestow.animation.removeClipEvent(clipHandle, "footstep")

-- Clear all events from a clip
bestow.animation.clearClipEvents(clipHandle)

-- Get all events on a clip
local events = bestow.animation.getClipEvents(clipHandle)

-- Subscribe to animation events (receives AnimationEvent)
local subId = bestow.animation.subscribeToEvents(animator, function(event)
    -- event.animator      (AnimatorHandle)
    -- event.clip          (AnimationClipHandle)
    -- event.layer         (uint32)
    -- event.name          (string)
    -- event.clipTime      (float)
    -- event.normalizedTime (float)
    -- event.stringParam   (string)
    -- event.floatParam    (float)
    -- event.intParam      (int)
    if event.name == "footstep" then
        bestow.audio.playOnChannel(3, {
            asset = stepHandle,
            volume = 0.5
        })
    end
end)

-- Subscribe to animation completion (one-shot animations)
local completeId = bestow.animation.subscribeToComplete(animator, function(animatorHandle, clipHandle, layer)
    -- Called when a non-looping animation finishes on a layer
    bestow.animation.play(animatorHandle, "idle", 0.15)
end)

-- Subscribe to layer changes
local changeId = bestow.animation.subscribeToLayerChanges(animator, function(animatorHandle, layer, prevClip, newClip)
    -- Called when a layer transitions from one clip to another
end)

-- Clean up subscriptions
bestow.animation.unsubscribe(subId)
bestow.animation.unsubscribe(completeId)
bestow.animation.unsubscribe(changeId)
```

## Clip Info

```lua
local clipInfo = bestow.animation.getAnimationClipInfo(clipHandle)
-- clipInfo.name           (string)
-- clipInfo.skeleton       (SkeletonHandle)
-- clipInfo.duration       (float, in seconds)
-- clipInfo.ticksPerSecond (float)
-- clipInfo.defaultWrapMode (AnimationWrapMode)
-- clipInfo.channelCount   (int)
-- clipInfo.keyframeCount  (int)
-- clipInfo.hasRootMotion  (bool)
-- clipInfo.looping        (bool)

bestow.animation.isValidClip(clipHandle) -- -> bool
bestow.animation.findClip(skeleton, "idle") -- -> AnimationClipHandle or nil
bestow.animation.getClipsForSkeleton(skeleton) -- -> {clipHandle, ...}
bestow.animation.getClipNames(skeleton) -- -> {"idle", "walk", ...}
```

## Sockets (Attachment Points)

Attach objects (weapons, effects) to animated bones:

```lua
-- Define a socket on a bone
local socketDef = SocketDef.new()
socketDef.name = "right_hand_weapon"
socketDef.boneName = "RightHand"
socketDef.localPosition = Vec3.new(0, 0, 0)
socketDef.localRotation = Quat.identity()
socketDef.localScale = Vec3.new(1, 1, 1)
socketDef.attachMode = SocketAttachMode.FollowBone
-- SocketAttachMode: FollowBone, FollowPosition, FollowRotation, WorldSpace

local socketHandle = bestow.animation.defineSocket(skeleton, socketDef)

-- Check if socket exists
bestow.animation.hasSocket(skeleton, "right_hand_weapon") -- -> bool

-- Find socket handle by name
local socket = bestow.animation.findSocket(skeleton, "right_hand_weapon")

-- Get all sockets on a skeleton
local sockets = bestow.animation.getSockets(skeleton)

-- Get socket definition
local def = bestow.animation.getSocketDef(socketHandle) -- -> SocketDef or nil

-- Get socket world transform (needs entity world matrix)
local socketTransform = bestow.animation.getSocketTransform(animator, "right_hand_weapon", entityWorldMatrix)
-- Or by handle:
local socketTransform = bestow.animation.getSocketTransform(animator, socketHandle, entityWorldMatrix)

if socketTransform then
    -- socketTransform.worldMatrix (Mat4)
    -- socketTransform.position    (Vec3)
    -- socketTransform.rotation    (Quat)
    -- socketTransform.scale       (Vec3)
    -- socketTransform.forward     (Vec3)
    -- socketTransform.up          (Vec3)
    -- socketTransform.right       (Vec3)
    bestow.entity.setField(weapon, "Transform3D", "position", socketTransform.position)
    bestow.entity.setField(weapon, "Transform3D", "rotation", socketTransform.rotation)
end

-- Modify socket transform at runtime
bestow.animation.setSocketLocalTransform(socketHandle, position, rotation)        -- scale defaults to (1,1,1)
bestow.animation.setSocketLocalTransform(socketHandle, position, rotation, scale)

-- Enable/disable a socket
bestow.animation.setSocketEnabled(socketHandle, false)
bestow.animation.isSocketEnabled(socketHandle) -- -> bool

-- Remove a socket
bestow.animation.removeSocket(socketHandle)
bestow.animation.removeSocket(skeleton, "right_hand_weapon")
```

## Inverse Kinematics (IK)

### Two-Bone IK (Arms/Legs)

```lua
-- Define IK chain
local chain = IKTwoBoneChain.new()
chain.name = "right_arm"
chain.rootBoneName = "RightArm"
chain.midBoneName = "RightForeArm"
chain.tipBoneName = "RightHand"
bestow.animation.defineIKChain(skeleton, chain)

-- Set target in update
local target = IKTwoBoneTarget.new()
target.chainName = "right_arm"
target.targetPosition = worldTargetPos
target.poleVector = elbowHintPos   -- Controls elbow/knee direction
target.weight = 1.0
target.enabled = true
bestow.animation.setIKTarget(animator, target)

-- Adjust weight for blending
bestow.animation.setIKWeight(animator, "right_arm", 0.5)
bestow.animation.getIKWeight(animator, "right_arm") -- -> float

-- Get current target
local currentTarget = bestow.animation.getIKTwoBoneTarget(animator, "right_arm") -- -> IKTwoBoneTarget or nil

-- Clear target
bestow.animation.clearIKTarget(animator, "right_arm")
bestow.animation.clearAllIKTargets(animator)

-- Remove chain definition
bestow.animation.removeIKChain(skeleton, "right_arm")

-- List all IK chain names
bestow.animation.getIKChainNames(skeleton) -- -> {"right_arm", ...}
```

### Aim IK (Look At)

```lua
-- Define aim constraint
local aimConfig = IKAimConfig.new()
aimConfig.name = "head_look"
aimConfig.boneName = "Head"
aimConfig.aimAxis = Vec3.new(0, 0, 1)     -- Which local axis points forward
aimConfig.upAxis = Vec3.new(0, 1, 0)
aimConfig.horizontalLimit = math.rad(70)
aimConfig.verticalLimit = math.rad(45)
bestow.animation.defineIKAim(skeleton, aimConfig)

-- Set look target
local aimTarget = IKAimTarget.new()
aimTarget.configName = "head_look"
aimTarget.targetPosition = enemyHeadPos
aimTarget.worldUp = Vec3.new(0, 1, 0)
aimTarget.weight = 0.8
aimTarget.enabled = true
bestow.animation.setIKTarget(animator, aimTarget)

-- Get current aim target
local current = bestow.animation.getIKAimTarget(animator, "head_look") -- -> IKAimTarget or nil

-- Remove aim definition
bestow.animation.removeIKAim(skeleton, "head_look")

-- List all aim config names
bestow.animation.getIKAimNames(skeleton) -- -> {"head_look", ...}
```

## Root Motion

Extract movement from animation data and apply to game entity:

```lua
-- Enable root motion
bestow.animation.setRootMotionEnabled(animator, true)

-- Configure what to extract
local rmConfig = RootMotionConfig.new()
rmConfig.enabled = true
rmConfig.extractTranslationX = true
rmConfig.extractTranslationY = false   -- Usually don't extract vertical
rmConfig.extractTranslationZ = true
rmConfig.extractRotationY = true
rmConfig.extractRotationXZ = false
rmConfig.rootBoneName = "Root"
bestow.animation.setRootMotionConfig(animator, rmConfig)

-- Query root motion config
bestow.animation.isRootMotionEnabled(animator) -- -> bool
bestow.animation.getRootMotionConfig(animator) -- -> RootMotionConfig

-- In update: get and apply root motion
local rootMotion = bestow.animation.getRootMotion(animator)
-- rootMotion.deltaPosition    (Vec3 - movement this frame)
-- rootMotion.deltaRotation    (Quat - rotation this frame)
-- rootMotion.totalPosition    (Vec3 - total accumulated position)
-- rootMotion.totalRotation    (Quat - total accumulated rotation)
-- rootMotion.hasTranslation   (bool)
-- rootMotion.hasRotation      (bool)

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
bestow.animation.consumeRootMotion(animator)  -- Reset deltas for next frame

-- Extract root motion from a clip directly (for preview/tools)
local motion = bestow.animation.extractRootMotion(clipHandle, fromTime, toTime)
```

## Ragdoll

Query ragdoll state on entities (ragdoll creation/activation uses physics3d):

```lua
bestow.animation.hasRagdoll(entity)                -- -> bool
bestow.animation.isRagdollActive(entity)            -- -> bool
bestow.animation.setRagdollBlendWeight(entity, 0.5) -- 0 = animation, 1 = ragdoll
bestow.animation.getRagdollBlendWeight(entity)      -- -> float

local ragdollState = bestow.animation.getRagdollState(entity)
-- ragdollState.created       (bool)
-- ragdollState.active        (bool)
-- ragdollState.blendWeight   (float)
-- ragdollState.blendTarget   (float)
-- ragdollState.blendDuration (float)
-- ragdollState.blendTime     (float)
```

## Bone Transforms

Access bone positions for effects, targeting, etc.:

```lua
-- Get bone transform by name or index (returns Mat4)
local boneTransform = bestow.animation.getBoneTransform(animator, "Head")
local boneTransform = bestow.animation.getBoneTransform(animator, 5)  -- by bone index

-- Get bone world transform (applies entity world matrix)
local worldTransform = bestow.animation.getBoneWorldTransform(animator, boneIndex, entityWorldMatrix)
```

## Statistics and Debugging

```lua
local stats = bestow.animation.getStats()
-- stats.skeletonCount, stats.clipCount, stats.animatorCount
-- stats.socketDefCount, stats.ikChainCount, stats.ragdollCount
-- stats.animatorsUpdated, stats.layersProcessed
-- stats.samplingJobs, stats.blendingJobs, stats.ikSolves
-- stats.socketQueries, stats.eventsDispatched, stats.ragdollSyncs
-- stats.updateTimeMs, stats.samplingTimeMs, stats.blendingTimeMs
-- stats.ikTimeMs, stats.ragdollSyncTimeMs
-- stats.skeletonMemoryBytes, stats.clipMemoryBytes, stats.animatorMemoryBytes
-- stats.totalMemoryBytes

bestow.animation.resetFrameStats()

-- Toggle debug visualization (draws skeletons, sockets, IK targets)
bestow.animation.setDebugVisualization(true)
bestow.animation.isDebugVisualizationEnabled() -- -> bool
```

## Handle Validation Constants

```lua
bestow.animation.Handle.InvalidSkeleton
bestow.animation.Handle.InvalidClip
bestow.animation.Handle.InvalidAnimator
bestow.animation.Handle.InvalidSocket
```

## Animator and Skeleton Cleanup

```lua
bestow.animation.destroyAnimator(animator)
bestow.animation.isValidAnimator(animator) -- -> bool
bestow.animation.getAnimatorSkeleton(animator) -- -> SkeletonHandle

bestow.animation.destroySkeleton(skeleton)
bestow.animation.destroyAnimationClip(clipHandle)
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
        if not state.player or not state.animator then return end

        local animator = state.animator
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
            local wrapMode = (target == "jump" or target == "fall")
                and AnimationWrapMode.Once or AnimationWrapMode.Loop
            local config = AnimationPlayConfig.new()
            config.clipName = target
            config.wrapMode = wrapMode
            config.blendInTime = BLEND_TIME
            bestow.animation.play(animator, config)
            self.currentState = target
        end
    end
}
```

## Best Practices

1. **Use `loadCharacter` for simple cases** -- handles skeleton, animator, clips, and mesh automatically
2. **All functions are in `bestow.animation.*`** -- there are no OOP methods on character/animator objects
3. **Use short crossfade times** -- 0.1-0.3 seconds for responsive transitions
4. **Use animation events for timing** -- don't guess when attacks connect
5. **Subscribe to completion** for one-shot animations (attacks, jumps)
6. **Use layers for partial body** -- run on layer 0, aim on layer 1
7. **Store handles in `app.main.state`** -- survives hot reload
8. **Consume root motion every frame** -- or deltas accumulate
9. **Clean up subscriptions** -- call `unsubscribe` when entities are destroyed
10. **Use `drawCharacter` or `drawCharacterTransform`** to render skinned meshes
