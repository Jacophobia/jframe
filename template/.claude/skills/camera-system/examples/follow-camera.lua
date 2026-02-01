-- Example: Smooth Follow Camera
-- Shows third-person follow camera with smoothing, look-ahead, and shake

-- systems/camera.lua
return {
    offset = Vec3.new(0, 8, -12),
    lookAheadFactor = 2.0,
    smoothSpeed = 5.0,

    initState = function()
        app.main.state.camera = {
            position = Vec3.new(0, 8, -12),
            shakeIntensity = 0,
            shakeDuration = 0,
            shakeTimer = 0,
        }
    end,

    update = function(dt)
        local self = app.systems.camera
        local state = app.main.state
        local camState = state.camera
        if not state.player then return end

        -- Get target position
        local targetPos = bestow.entity.getField(state.player, "Transform3D", "position")

        -- Desired position with offset
        local desiredPos = targetPos + self.offset

        -- Smooth follow (lerp)
        local diff = desiredPos - camState.position
        camState.position = camState.position + diff * math.min(1.0, self.smoothSpeed * dt)

        -- Look at target (with look-ahead)
        local lookTarget = targetPos
        local forward = (lookTarget - camState.position):normalize()
        local rotation = Quat.lookAt(forward, Vec3.up())

        -- Apply screen shake
        local shakeOffset = Vec3.zero()
        if camState.shakeTimer > 0 then
            camState.shakeTimer = camState.shakeTimer - dt
            local t = camState.shakeTimer / camState.shakeDuration
            local intensity = camState.shakeIntensity * t
            shakeOffset = Vec3.new(
                (math.random() * 2 - 1) * intensity,
                (math.random() * 2 - 1) * intensity,
                (math.random() * 2 - 1) * intensity
            )
        end

        -- Set camera
        bestow.graphics3d.setCamera({
            position = camState.position + shakeOffset,
            rotation = rotation,
            fov = 45.0,
            near = 0.1,
            far = 1000.0
        })
    end,

    -- Trigger screen shake
    shake = function(intensity, duration)
        local camState = app.main.state.camera
        camState.shakeIntensity = intensity
        camState.shakeDuration = duration
        camState.shakeTimer = duration
    end
}

-- Usage:
-- app.systems.camera.initState()   -- In main.lua init()
-- app.systems.camera.update(dt)    -- In main.lua update()
-- app.systems.camera.shake(0.5, 0.3) -- On impact
