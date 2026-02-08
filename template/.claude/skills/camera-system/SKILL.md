---
name: camera-system
description: Control cameras, implement following behavior, screen shake, and zoom in Bestow. Use when setting up cameras, making them follow players, adding screen effects, or converting between screen and world coordinates.
---

# Camera System

Bestow provides `bestow.graphics3d.setCamera()` for setting camera position and orientation. All high-level camera features (follow, shake, zoom, bounds) are implemented as Lua systems using this API.

## Basic Camera Setup

### Manual Camera Control

```lua
-- Set camera directly (table shorthand - accepts position, rotation, fov, near, far)
bestow.graphics3d.setCamera({
    position = Vec3.new(0, 10, -20),
    rotation = Quat.identity(),
    fov = 45.0,
    near = 0.1,
    far = 1000.0
})

-- Or use look-at style (target instead of rotation)
bestow.graphics3d.setCamera({
    position = Vec3.new(0, 10, -20),
    target = Vec3.new(0, 0, 0),      -- Look at origin
    up = Vec3.new(0, 1, 0),          -- Optional, defaults to Y-up
    fov = 45.0,
    near = 0.1,
    far = 1000.0
})

-- Read current camera (returns Camera3D struct)
local cam = bestow.graphics3d.getCamera()
-- cam.transform.position  (Vec3)
-- cam.transform.rotation  (Quat)
-- cam.fovY                (float)
-- cam.nearPlane           (float)
-- cam.farPlane            (float)
-- cam.aspectRatio         (float)
-- cam.projection          (ProjectionType.Perspective or .Orthographic)
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
        if not bestow.entity.isValid(state.player) then return end

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

Limit camera movement to level boundaries (implemented in Lua):

```lua
-- In camera system
local function clampToBounds(position, bounds)
    return Vec3.new(
        math.max(bounds.minX, math.min(bounds.maxX, position.x)),
        math.max(bounds.minY, math.min(bounds.maxY, position.y)),
        math.max(bounds.minZ, math.min(bounds.maxZ, position.z))
    )
end

update = function(dt)
    local self = app.systems.camera
    local state = app.main.state

    -- Calculate desired position...
    local desiredPos = targetPos + self.offset

    -- Clamp to bounds
    local level = app.levels[state.currentLevel]
    if level and level.cameraBounds then
        desiredPos = clampToBounds(desiredPos, level.cameraBounds)
    end

    -- Continue with smoothing...
end
```

## Screen Shake

Add impact feedback (implemented as Lua camera state):

```lua
-- systems/camera.lua - add shake support
return {
    offset = Vec3.new(0, 8, -12),
    smoothing = 0.1,

    initState = function()
        app.main.state.camera = {
            position = Vec3.new(0, 8, -12),
            shakeIntensity = 0,
            shakeDuration = 0,
            shakeTimer = 0
        }
    end,

    shake = function(intensity, duration)
        local camState = app.main.state.camera
        camState.shakeIntensity = intensity
        camState.shakeDuration = duration
        camState.shakeTimer = 0
    end,

    stopShake = function()
        local camState = app.main.state.camera
        camState.shakeIntensity = 0
        camState.shakeTimer = 0
    end,

    update = function(dt)
        local self = app.systems.camera
        local camState = app.main.state.camera

        -- ... follow logic (calculate finalPos and rotation) ...

        -- Apply shake offset
        if camState.shakeIntensity > 0 then
            camState.shakeTimer = camState.shakeTimer + dt
            if camState.shakeTimer < camState.shakeDuration then
                local t = 1.0 - (camState.shakeTimer / camState.shakeDuration)
                local strength = camState.shakeIntensity * t
                local shakeOffset = Vec3.new(
                    (math.random() - 0.5) * 2 * strength,
                    (math.random() - 0.5) * 2 * strength,
                    (math.random() - 0.5) * 2 * strength
                )
                finalPos = finalPos + shakeOffset
            else
                camState.shakeIntensity = 0
            end
        end

        bestow.graphics3d.setCamera({
            position = finalPos,
            rotation = rotation,
            fov = 45.0,
            near = 0.1,
            far = 1000.0
        })
    end
}
```

### Contextual Shake

```lua
-- Light hit
app.systems.camera.shake(0.1, 0.1)

-- Heavy hit
app.systems.camera.shake(0.3, 0.2)

-- Explosion
app.systems.camera.shake(0.8, 0.5)

-- Earthquake
app.systems.camera.shake(0.5, 2.0)
```

### Shake with Audio

```lua
function explosion(position)
    -- Visual effect
    app.systems.effects.spawnExplosion(position)

    -- Screen shake based on distance
    local cam = bestow.graphics3d.getCamera()
    local distance = (position - cam.transform.position):length()
    local intensity = math.max(0, 1.0 - distance / 50.0)

    if intensity > 0 then
        app.systems.camera.shake(intensity * 0.8, 0.4)
    end

    -- Audio (use asset handle, not path)
    local explosionSound = app.main.state.sounds.explosion  -- preloaded asset handle
    bestow.audio.playPositional({
        asset = explosionSound,
        position = position,
        volume = 1.0,
        minDistance = 10.0,
        maxDistance = 100.0
    })
end
```

## Zoom

Zoom is implemented by adjusting the camera's FOV:

```lua
-- In camera state
app.main.state.camera = {
    position = Vec3.new(0, 8, -12),
    currentZoom = 1.0,
    targetZoom = 1.0,
    baseFov = 45.0
}

-- Update with smoothing
update = function(dt)
    local camState = app.main.state.camera

    -- Smooth zoom transition
    local zoomDiff = camState.targetZoom - camState.currentZoom
    camState.currentZoom = camState.currentZoom + zoomDiff * 0.1

    -- Apply zoom as FOV adjustment (higher zoom = lower FOV)
    local fov = camState.baseFov / camState.currentZoom

    bestow.graphics3d.setCamera({
        position = camState.position,
        rotation = rotation,
        fov = fov,
        near = 0.1,
        far = 1000.0
    })
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

### Screen to World (Raycasting)

```lua
-- For 3D, use a raycast from camera through screen point
local screenPos = bestow.input.getMousePosition()

-- Raycast from camera into the scene
local ray = bestow.graphics3d.screenToWorldRay(screenPos)
local hit = bestow.physics3d.raycast(ray.origin, ray.direction, 1000)
if hit then
    local clickedWorldPos = hit.point
end
```

### World to Screen

```lua
-- Get screen position of world object
local enemyPos = bestow.entity.getField(enemy, "Transform3D", "position")
local screenPos = bestow.graphics3d.worldToScreen(enemyPos)

-- Check if on screen (getWindowSize returns Size with .width and .height)
local windowSize = bestow.graphics3d.getWindowSize()
if screenPos and screenPos.x >= 0 and screenPos.x <= windowSize.width and
   screenPos.y >= 0 and screenPos.y <= windowSize.height then
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
        local cam = bestow.graphics3d.getCamera()
        self.transitioning = true
        self.transitionStart = {
            position = cam.transform.position,
            rotation = cam.transform.rotation
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
7. **Validate entity handles** - Always check `bestow.entity.isValid()` before accessing
