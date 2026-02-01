---
name: camera-system
description: Control cameras, implement following behavior, screen shake, and zoom in Bestow. Use when setting up cameras, making them follow players, adding screen effects, or converting between screen and world coordinates.
---

# Camera System

The camera system provides follow behavior, effects, and coordinate conversion.

## Basic Camera Setup

### Manual Camera Control

```lua
-- Set camera directly
bestow.graphics3d.setCamera({
    position = Vec3.new(0, 10, -20),
    rotation = Quat.identity(),
    fov = 45.0,
    near = 0.1,
    far = 1000.0
})
```

### Using the Camera System

The camera system provides high-level features:

```lua
-- In init()
local camera = bestow.camera3d

-- Set target entity to follow
camera.setTarget(app.main.state.player)

-- Configure follow behavior
camera.setFollowSmoothing(0.1)  -- 0 = instant, 1 = very slow
camera.setOffset(Vec3.new(0, 10, -15))  -- Camera offset from target
camera.setDeadzone(Vec3.new(2, 1, 0))   -- Target can move this much without camera moving
```

## Follow Camera Pattern

For a complete follow camera implementation:

```lua
-- systems/camera.lua
return {
    -- Configuration
    offset = Vec3.new(0, 8, -12),
    lookAhead = 2.0,
    smoothing = 0.1,

    -- State (stored in app.main.state for hot reload safety)
    initState = function()
        app.main.state.camera = {
            position = Vec3.new(0, 8, -12),
            velocity = Vec3.new(0, 0, 0)
        }
    end,

    update = function(dt)
        local self = app.systems.camera
        local state = app.main.state
        local camState = state.camera

        if not state.player then return end

        -- Get target position
        local targetPos = bestow.entity.getField(state.player, "Transform3D", "position")

        -- Calculate desired camera position
        local desiredPos = targetPos + self.offset

        -- Smooth follow
        local diff = desiredPos - camState.position
        camState.position = camState.position + diff * self.smoothing

        -- Calculate look target (slightly ahead of player)
        local playerVel = bestow.entity.getComponent(state.player, "Velocity")
        local lookTarget = targetPos
        if playerVel then
            lookTarget = targetPos + playerVel.linear:normalize() * self.lookAhead
        end

        -- Calculate rotation to look at target
        local forward = (lookTarget - camState.position):normalize()
        local rotation = Quat.lookAt(forward, Vec3.new(0, 1, 0))

        -- Apply camera
        bestow.graphics3d.setCamera({
            position = camState.position,
            rotation = rotation,
            fov = 45.0,
            near = 0.1,
            far = 1000.0
        })
    end
}

-- In main.lua init():
app.systems.camera.initState()

-- In main.lua update():
app.systems.camera.update(dt)
```

## Camera Bounds

Limit camera movement to level boundaries:

```lua
-- Set bounds
bestow.camera3d.setBounds(
    -50, 50,   -- minX, maxX
    0, 100,    -- minY, maxY
    -50, 50    -- minZ, maxZ
)

-- Clear bounds
bestow.camera3d.clearBounds()
```

### Bounds with Follow Camera

```lua
update = function(dt)
    local self = app.systems.camera
    local state = app.main.state

    -- Calculate desired position...
    local desiredPos = targetPos + self.offset

    -- Clamp to bounds
    local level = app.levels[state.currentLevel]
    if level.cameraBounds then
        local b = level.cameraBounds
        desiredPos.x = math.max(b.minX, math.min(b.maxX, desiredPos.x))
        desiredPos.y = math.max(b.minY, math.min(b.maxY, desiredPos.y))
        desiredPos.z = math.max(b.minZ, math.min(b.maxZ, desiredPos.z))
    end

    -- Continue with smoothing...
end
```

## Screen Shake

Add impact feedback:

```lua
-- Trigger shake
bestow.camera3d.shake(
    0.5,    -- Intensity (world units of displacement)
    0.3     -- Duration (seconds)
)

-- Stop shake early
bestow.camera3d.stopShake()
```

### Contextual Shake

```lua
-- Light hit
bestow.camera3d.shake(0.1, 0.1)

-- Heavy hit
bestow.camera3d.shake(0.3, 0.2)

-- Explosion
bestow.camera3d.shake(0.8, 0.5)

-- Earthquake
bestow.camera3d.shake(0.5, 2.0)
```

### Shake with Audio

```lua
function explosion(position)
    -- Visual effect
    app.systems.effects.spawnExplosion(position)

    -- Screen shake based on distance
    local camPos = bestow.camera3d.getPosition()
    local distance = (position - camPos):length()
    local intensity = math.max(0, 1.0 - distance / 50.0)

    if intensity > 0 then
        bestow.camera3d.shake(intensity * 0.8, 0.4)
    end

    -- Audio
    bestow.audio.playPositional({
        path = "sounds/explosion.wav",
        position = position,
        volume = 1.0,
        minDistance = 10.0,
        maxDistance = 100.0
    })
end
```

## Zoom

```lua
-- Zoom in (values > 1)
bestow.camera3d.setZoom(1.5)

-- Zoom out (values < 1)
bestow.camera3d.setZoom(0.75)

-- Get current zoom
local zoom = bestow.camera3d.getZoom()

-- Reset to normal
bestow.camera3d.setZoom(1.0)
```

### Smooth Zoom

```lua
-- In camera state
app.main.state.camera = {
    currentZoom = 1.0,
    targetZoom = 1.0
}

-- Update with smoothing
update = function(dt)
    local camState = app.main.state.camera

    -- Smooth zoom transition
    local zoomDiff = camState.targetZoom - camState.currentZoom
    camState.currentZoom = camState.currentZoom + zoomDiff * 0.1

    bestow.camera3d.setZoom(camState.currentZoom)
end

-- Trigger zoom change
function zoomIn()
    app.main.state.camera.targetZoom = 1.5
end

function zoomOut()
    app.main.state.camera.targetZoom = 0.75
end
```

## Coordinate Conversion

### Screen to World

```lua
-- Get world position from screen position
local screenPos = bestow.input.getMousePosition()
local worldPos = bestow.camera3d.screenToWorld(screenPos)

-- For 3D, you often need a raycast
local ray = bestow.camera3d.screenToRay(screenPos)
local hit = bestow.physics3d.raycast(ray.origin, ray.direction, 1000)
if hit then
    local clickedWorldPos = hit.point
end
```

### World to Screen

```lua
-- Get screen position of world object
local enemyPos = bestow.entity.getField(enemy, "Transform3D", "position")
local screenPos = bestow.camera3d.worldToScreen(enemyPos)

-- Check if on screen
local windowSize = bestow.graphics3d.getWindowSize()
if screenPos.x >= 0 and screenPos.x <= windowSize.x and
   screenPos.y >= 0 and screenPos.y <= windowSize.y then
    -- Enemy is visible on screen
end
```

## Multiple Camera Views

For split-screen or picture-in-picture:

```lua
-- Main camera
bestow.graphics3d.setViewport(0, 0, 640, 720)  -- Left half
bestow.graphics3d.setCamera(mainCamera)
renderScene()

-- Secondary camera
bestow.graphics3d.setViewport(640, 0, 640, 720)  -- Right half
bestow.graphics3d.setCamera(secondCamera)
renderScene()

-- Reset to full screen
bestow.graphics3d.setViewport(0, 0, 1280, 720)
```

## Camera Transitions

```lua
-- systems/camera.lua
return {
    -- Transition state
    transitioning = false,
    transitionStart = nil,
    transitionEnd = nil,
    transitionDuration = 0,
    transitionTime = 0,

    transitionTo = function(newPosition, newRotation, duration)
        local self = app.systems.camera
        self.transitioning = true
        self.transitionStart = {
            position = bestow.camera3d.getPosition(),
            rotation = bestow.camera3d.getCamera().rotation
        }
        self.transitionEnd = {
            position = newPosition,
            rotation = newRotation
        }
        self.transitionDuration = duration
        self.transitionTime = 0
    end,

    update = function(dt)
        local self = app.systems.camera

        if self.transitioning then
            self.transitionTime = self.transitionTime + dt
            local t = self.transitionTime / self.transitionDuration

            if t >= 1.0 then
                t = 1.0
                self.transitioning = false
            end

            -- Smooth easing
            t = t * t * (3 - 2 * t)  -- Smoothstep

            -- Interpolate
            local pos = self.transitionStart.position +
                (self.transitionEnd.position - self.transitionStart.position) * t
            local rot = Quat.slerp(self.transitionStart.rotation, self.transitionEnd.rotation, t)

            bestow.graphics3d.setCamera({
                position = pos,
                rotation = rot,
                fov = 45.0,
                near = 0.1,
                far = 1000.0
            })
        else
            -- Normal follow behavior
            self.followTarget(dt)
        end
    end
}
```

## Best Practices

1. **Store camera state in app.main.state** - Survives hot reload
2. **Use smoothing for follow cameras** - Prevents jerky movement
3. **Add deadzone for subtle movement** - Reduces motion sickness
4. **Shake on impacts** - Great feedback, but don't overdo it
5. **Bound camera to level** - Prevent showing void
6. **Transition smoothly** - No hard cuts between camera positions
