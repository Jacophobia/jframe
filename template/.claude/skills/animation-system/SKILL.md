---
name: animation-system
description: Control skeletal animations, playback, blending, and IK in Bestow. Use when playing character animations, crossfading between animations, or implementing procedural animation.
---

# Animation System

The animation system provides skeletal animation playback, blending, and inverse kinematics.

## Basic Animation Playback

### Setting Up an Animated Entity

```lua
local character = bestow.entity.create()

-- Add transform and mesh
bestow.entity.addComponent(character, "Transform3D", {
    position = Vec3.new(0, 0, 0),
    rotation = Quat.identity(),
    scale = Vec3.new(1, 1, 1)
})

bestow.entity.addComponent(character, "MeshRenderer", {
    mesh = "meshes/character.gltf",
    material = "materials/character"
})

-- Create animator
local animator = bestow.animation.createAnimator(character, "meshes/character.gltf")

-- Store animator handle
bestow.entity.addComponent(character, "Animator", {
    handle = animator
})
```

### Playing Animations

```lua
local animator = bestow.entity.getComponent(character, "Animator").handle

-- Play animation (replaces current)
bestow.animation.play(animator, "idle")

-- Play with options
bestow.animation.play(animator, {
    clip = "run",
    speed = 1.0,
    loop = true,
    startTime = 0.0
})

-- Play once (no loop)
bestow.animation.play(animator, {
    clip = "attack",
    loop = false
})
```

### Crossfading

Smooth transition between animations:

```lua
-- Crossfade to new animation over 0.2 seconds
bestow.animation.crossfade(animator, "walk", 0.2)

-- Crossfade with options
bestow.animation.crossfade(animator, {
    clip = "run",
    duration = 0.3,
    loop = true
})
```

### Stopping and Pausing

```lua
-- Stop animation
bestow.animation.stop(animator)

-- Pause/Resume
bestow.animation.setPaused(animator, true)
bestow.animation.setPaused(animator, false)

-- Check if paused
if bestow.animation.isPaused(animator) then
    -- Animation is paused
end
```

### Playback Control

```lua
-- Set playback speed (1.0 = normal, 2.0 = double, -1.0 = reverse)
bestow.animation.setSpeed(animator, 1.5)
local speed = bestow.animation.getSpeed(animator)

-- Get current time
local time = bestow.animation.getCurrentTime(animator)
local normalized = bestow.animation.getNormalizedTime(animator)  -- 0.0 to 1.0

-- Set time directly
bestow.animation.setCurrentTime(animator, 0.5)
bestow.animation.setNormalizedTime(animator, 0.5)  -- Jump to middle

-- Get clip duration
local duration = bestow.animation.getClipDuration(animator, "run")
```

## Animation Layers

Layer animations for partial body control:

```lua
-- Layer 0: Full body (default)
bestow.animation.play(animator, { clip = "run", layer = 0 })

-- Layer 1: Upper body only (for aiming while running)
bestow.animation.play(animator, {
    clip = "aim",
    layer = 1,
    blendMode = "Override"  -- or "Additive"
})

-- Set layer weight
bestow.animation.setLayerWeight(animator, 1, 0.8)

-- Stop specific layer
bestow.animation.stopLayer(animator, 1)
```

### Bone Masks

Limit which bones a layer affects:

```lua
-- Create mask for upper body
local upperBodyBones = {
    "Spine", "Spine1", "Spine2",
    "Neck", "Head",
    "LeftShoulder", "LeftArm", "LeftForeArm", "LeftHand",
    "RightShoulder", "RightArm", "RightForeArm", "RightHand"
}

bestow.animation.setLayerBoneMask(animator, 1, upperBodyBones)
```

## Animation Events

Respond to animation events (defined in animation files):

```lua
-- Subscribe to animation events
bestow.animation.subscribeToEvents(animator, function(event)
    if event.name == "footstep" then
        app.systems.audio.playSfx("footstep")
    elseif event.name == "attack_hit" then
        app.systems.combat.checkHit(character)
    end
end)

-- Subscribe to animation completion
bestow.animation.subscribeToComplete(animator, function(clipName)
    if clipName == "attack" then
        -- Attack finished, return to idle
        bestow.animation.crossfade(animator, "idle", 0.1)
    elseif clipName == "death" then
        bestow.entity.destroy(character)
    end
end)
```

## Animation State Machine Pattern

```lua
-- systems/player_animation.lua
return {
    states = {
        idle = "idle",
        walking = "walk",
        running = "run",
        jumping = "jump",
        falling = "fall",
        attacking = "attack"
    },

    currentState = "idle",

    update = function(dt)
        local self = app.systems.player_animation
        local state = app.main.state
        local player = state.player

        if not player then return end

        local animator = bestow.entity.getComponent(player, "Animator").handle
        local velocity = bestow.entity.getComponent(player, "Velocity")
        local controller = bestow.entity.getComponent(player, "CharacterController")
        local grounded = bestow.physics3d.getCharacterGroundInfo(controller.handle).grounded

        -- Determine desired state
        local newState = "idle"

        if not grounded then
            if velocity.linear.y > 0 then
                newState = "jumping"
            else
                newState = "falling"
            end
        else
            local speed = Vec3.new(velocity.linear.x, 0, velocity.linear.z):length()
            if speed > 5 then
                newState = "running"
            elseif speed > 0.5 then
                newState = "walking"
            else
                newState = "idle"
            end
        end

        -- Transition if changed
        if newState ~= self.currentState then
            local clip = self.states[newState]
            bestow.animation.crossfade(animator, clip, 0.2)
            self.currentState = newState
        end
    end
}
```

## Inverse Kinematics (IK)

### Two-Bone IK (Arms, Legs)

```lua
-- Define IK chain for right arm
bestow.animation.defineIKChain(animator, {
    name = "right_arm",
    rootBone = "RightArm",
    midBone = "RightForeArm",
    endBone = "RightHand"
})

-- Set IK target
bestow.animation.setIKTarget(animator, "right_arm", {
    position = targetPosition,
    rotation = targetRotation,
    weight = 1.0
})

-- Clear IK target
bestow.animation.clearIKTarget(animator, "right_arm")
```

### Aim IK (Look At)

```lua
-- Define aim for head
bestow.animation.defineIKAim(animator, {
    name = "head_aim",
    bone = "Head",
    aimAxis = Vec3.new(0, 0, 1),  -- Which axis points forward
    upAxis = Vec3.new(0, 1, 0)
})

-- Set aim target
bestow.animation.setIKTarget(animator, "head_aim", {
    position = lookAtPosition,
    weight = 0.8
})
```

## Root Motion

Use animation movement for character movement:

```lua
-- Enable root motion
bestow.animation.setRootMotionEnabled(animator, true)

-- In update loop
local rootMotion = bestow.animation.extractRootMotion(animator)
if rootMotion then
    -- Apply to character position
    local pos = bestow.entity.getField(character, "Transform3D", "position")
    pos = pos + rootMotion.deltaPosition
    bestow.entity.setField(character, "Transform3D", "position", pos)

    -- Apply rotation
    local rot = bestow.entity.getField(character, "Transform3D", "rotation")
    rot = rootMotion.deltaRotation * rot
    bestow.entity.setField(character, "Transform3D", "rotation", rot)
end
```

## Sockets (Attachment Points)

Attach objects to animated bones:

```lua
-- Define socket on hand
bestow.animation.defineSocket(animator, {
    name = "right_hand_weapon",
    bone = "RightHand",
    localOffset = Vec3.new(0, 0, 0),
    localRotation = Quat.identity()
})

-- Get socket transform for attaching weapon
local socket = bestow.animation.getSocketTransform(animator, "right_hand_weapon")
bestow.entity.setField(weapon, "Transform3D", "position", socket.position)
bestow.entity.setField(weapon, "Transform3D", "rotation", socket.rotation)
```

## Ragdoll

Switch between animation and physics:

```lua
-- Create ragdoll (usually done once in init)
bestow.animation.createRagdoll(animator, {
    -- Physics body definitions for each bone
    -- Usually auto-generated from skeleton
})

-- Activate ragdoll (e.g., on death)
bestow.animation.activateRagdoll(animator)

-- Deactivate and return to animation
bestow.animation.deactivateRagdoll(animator)

-- Blend between animation and ragdoll
bestow.animation.setRagdollBlendWeight(animator, 0.5)
```

## Animation for Non-Characters

### Simple Animation Playback

For environmental objects:

```lua
local door = bestow.entity.create()
-- ... add mesh

local doorAnimator = bestow.animation.createAnimator(door, "meshes/door.gltf")
bestow.entity.addComponent(door, "Animator", { handle = doorAnimator })

-- When player interacts
function openDoor(door)
    local animator = bestow.entity.getComponent(door, "Animator").handle
    bestow.animation.play(animator, {
        clip = "open",
        loop = false
    })
end
```

## Best Practices

1. **Use crossfade for transitions** - Avoids jarring pops
2. **Keep crossfade times short** - 0.1-0.3 seconds typical
3. **Use animation events for timing** - Don't guess when attacks hit
4. **Subscribe to completion** - For one-shot animations
5. **Use layers for partial body** - Aim while running, wave while walking
6. **Enable IK sparingly** - It's computationally expensive
7. **Store animator handles in components** - Access them efficiently
