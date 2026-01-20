--[[
    Camera Module - Elden Ring Style

    Third-person follow camera that:
    - Maintains position behind the player
    - Only rotates to follow movement direction when player is actively moving
    - Has a delay before starting to rotate (doesn't snap immediately)
    - Stays in place when player stops moving
]]

local camera = {}

-- Create and initialize camera state
function camera.create(appSelf, scope)
    local config = app.config
    local camConfig = config.camera

    local cam = {
        -- Current camera position
        position = Vec3.new(0, camConfig.height, -camConfig.distance),

        -- Target position (for smoothing)
        targetPosition = Vec3.new(0, camConfig.height, -camConfig.distance),

        -- Look-at target
        lookAt = Vec3.new(0, 0, 0),

        -- Configuration
        distance = camConfig.distance,
        height = camConfig.height,
        lookAhead = camConfig.lookAhead,
        smoothing = camConfig.smoothing,

        -- Orbit angle (camera's own rotation, independent of player)
        currentOrbitAngle = 0,
        targetOrbitAngle = 0,
        orbitSmoothing = camConfig.orbitSmoothing or 1.5,

        -- Elden Ring style: delay before camera starts following
        followDelay = camConfig.followDelay or 0.3,  -- seconds before camera starts rotating
        movingTime = 0,  -- how long player has been moving

        -- Speed threshold to consider "moving"
        moveThreshold = camConfig.moveThreshold or 0.5,
    }

    appSelf.transient.camera = cam
    return cam
end

-- Update camera to follow player
function camera.update(appSelf, dt, scope)
    local cam = appSelf.transient.camera
    local p = appSelf.transient.player
    if not cam or not p then return end

    local playerPos = p.position

    -- Calculate player's movement speed
    local velMag = math.sqrt(p.velocity.x * p.velocity.x + p.velocity.z * p.velocity.z)
    local isMoving = velMag > cam.moveThreshold

    -- Track how long player has been moving
    if isMoving then
        cam.movingTime = cam.movingTime + dt
    else
        cam.movingTime = 0
    end

    -- Only update target orbit angle when player has been moving for a while
    -- This creates the "delay before following" effect
    if isMoving and cam.movingTime > cam.followDelay then
        -- Calculate movement direction angle (where player is going, not where they're facing)
        local moveAngle = math.atan2(p.velocity.x, p.velocity.z)
        cam.targetOrbitAngle = moveAngle
    end
    -- When not moving (or during delay), targetOrbitAngle stays unchanged

    -- Smoothly interpolate current orbit angle toward target
    local angleDiff = cam.targetOrbitAngle - cam.currentOrbitAngle
    -- Normalize to -pi to pi for shortest path rotation
    while angleDiff > math.pi do angleDiff = angleDiff - 2 * math.pi end
    while angleDiff < -math.pi do angleDiff = angleDiff + 2 * math.pi end

    -- Smooth interpolation using exponential decay
    local orbitSmoothFactor = 1.0 - math.exp(-cam.orbitSmoothing * dt)
    cam.currentOrbitAngle = cam.currentOrbitAngle + angleDiff * orbitSmoothFactor

    -- Normalize resulting angle
    while cam.currentOrbitAngle > math.pi do cam.currentOrbitAngle = cam.currentOrbitAngle - 2 * math.pi end
    while cam.currentOrbitAngle < -math.pi do cam.currentOrbitAngle = cam.currentOrbitAngle + 2 * math.pi end

    -- Camera offset from player (behind and above) using current orbit angle
    local offsetX = -math.sin(cam.currentOrbitAngle) * cam.distance
    local offsetZ = -math.cos(cam.currentOrbitAngle) * cam.distance

    -- Calculate target camera position
    cam.targetPosition.x = playerPos.x + offsetX
    cam.targetPosition.y = playerPos.y + cam.height
    cam.targetPosition.z = playerPos.z + offsetZ

    -- Smoothly interpolate camera position
    local smoothFactor = 1.0 - math.exp(-cam.smoothing * dt)
    cam.position.x = cam.position.x + (cam.targetPosition.x - cam.position.x) * smoothFactor
    cam.position.y = cam.position.y + (cam.targetPosition.y - cam.position.y) * smoothFactor
    cam.position.z = cam.position.z + (cam.targetPosition.z - cam.position.z) * smoothFactor

    -- Calculate look-ahead offset based on player velocity
    local lookAheadX = 0
    local lookAheadZ = 0
    if velMag > 0.1 then
        local lookFactor = math.min(velMag / 5.0, 1.0) * cam.lookAhead
        lookAheadX = (p.velocity.x / velMag) * lookFactor
        lookAheadZ = (p.velocity.z / velMag) * lookFactor
    end

    -- Calculate look-at point (player position + look ahead)
    cam.lookAt.x = playerPos.x + lookAheadX
    cam.lookAt.y = playerPos.y + 1.5  -- Look at character's upper body
    cam.lookAt.z = playerPos.z + lookAheadZ

    -- Apply camera transform
    bestow.graphics3d.setCamera({
        position = cam.position,
        target = cam.lookAt,
        up = Vec3.new(0, 1, 0),
        fov = 60.0,
        near = 0.1,
        far = 1000.0
    })
end

return camera
