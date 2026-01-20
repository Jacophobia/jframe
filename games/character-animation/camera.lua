--[[
    Camera Module - Elden Ring Style

    Third-person follow camera that:
    - Maintains position behind the player
    - Supports manual control via mouse and right stick
    - Only auto-rotates to follow movement when no manual input for 2 seconds
    - Has a delay before starting to auto-rotate
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

        -- Look-at target (current and target for smoothing)
        lookAt = Vec3.new(0, 0, 0),
        targetLookAt = Vec3.new(0, 0, 0),

        -- Configuration
        distance = camConfig.distance,
        height = camConfig.height,
        lookAhead = camConfig.lookAhead,
        smoothing = camConfig.smoothing,

        -- Orbit angles (camera's own rotation, independent of player)
        currentOrbitAngle = 0,      -- Horizontal (yaw)
        targetOrbitAngle = 0,
        currentPitchAngle = 0.3,    -- Vertical (pitch) - start slightly above
        targetPitchAngle = 0.3,
        minPitch = -0.5,            -- Don't look too far down
        maxPitch = 1.2,             -- Don't look too far up
        orbitSmoothing = camConfig.orbitSmoothing or 1.5,

        -- Manual camera control
        manualControlTime = 0,  -- Time since last manual camera input
        manualControlCooldown = camConfig.manualControlCooldown or 2.0,  -- Seconds before auto-follow resumes
        mouseSensitivity = camConfig.mouseSensitivity or 0.003,
        stickSensitivity = camConfig.stickSensitivity or 2.0,

        -- Elden Ring style: delay before camera starts following
        followDelay = camConfig.followDelay or 0.3,
        movingTime = 0,

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

    -- Handle manual camera input (mouse and right stick)
    local manualInputX = 0
    local manualInputY = 0

    -- Mouse input (using delta for smooth camera control)
    local mouseDelta = bestow.input.getMouseDelta()
    if mouseDelta then
        if math.abs(mouseDelta.x) > 0.5 then
            manualInputX = -mouseDelta.x * cam.mouseSensitivity  -- Inverted for natural feel
            cam.manualControlTime = 0
        end
        if math.abs(mouseDelta.y) > 0.5 then
            manualInputY = mouseDelta.y * cam.mouseSensitivity
            cam.manualControlTime = 0
        end
    end

    -- Right stick input (gamepad) - using new action-based API
    local cameraAxisX = bestow.input.getActionValue("CameraAxisX") or 0
    local cameraAxisY = bestow.input.getActionValue("CameraAxisY") or 0
    if math.abs(cameraAxisX) > 0.01 then
        manualInputX = manualInputX - cameraAxisX * cam.stickSensitivity * dt  -- Inverted
        cam.manualControlTime = 0
    end
    if math.abs(cameraAxisY) > 0.01 then
        manualInputY = manualInputY + cameraAxisY * cam.stickSensitivity * dt
        cam.manualControlTime = 0
    end

    -- Apply manual camera rotation (instant - no smoothing on angles)
    if math.abs(manualInputX) > 0.001 then
        cam.currentOrbitAngle = cam.currentOrbitAngle + manualInputX
        cam.targetOrbitAngle = cam.currentOrbitAngle
        -- Normalize
        while cam.currentOrbitAngle > math.pi do cam.currentOrbitAngle = cam.currentOrbitAngle - 2 * math.pi end
        while cam.currentOrbitAngle < -math.pi do cam.currentOrbitAngle = cam.currentOrbitAngle + 2 * math.pi end
        cam.targetOrbitAngle = cam.currentOrbitAngle
    end
    if math.abs(manualInputY) > 0.001 then
        cam.currentPitchAngle = cam.currentPitchAngle + manualInputY
        -- Clamp pitch
        cam.currentPitchAngle = math.max(cam.minPitch, math.min(cam.maxPitch, cam.currentPitchAngle))
        cam.targetPitchAngle = cam.currentPitchAngle
    end

    -- Increment manual control timer
    cam.manualControlTime = cam.manualControlTime + dt

    -- Calculate player's movement speed
    local velMag = math.sqrt(p.velocity.x * p.velocity.x + p.velocity.z * p.velocity.z)
    local isMoving = velMag > cam.moveThreshold

    -- Track how long player has been moving
    if isMoving then
        cam.movingTime = cam.movingTime + dt
    else
        cam.movingTime = 0
    end

    -- Only auto-follow if:
    -- 1. No manual camera input for manualControlCooldown seconds
    -- 2. Player has been moving for followDelay seconds
    local canAutoFollow = cam.manualControlTime > cam.manualControlCooldown

    if canAutoFollow and isMoving and cam.movingTime > cam.followDelay then
        -- Calculate movement direction angle
        local moveAngle = math.atan2(p.velocity.x, p.velocity.z)

        -- Check if this would be a large rotation (like moving backward)
        local angleDiffToMove = moveAngle - cam.currentOrbitAngle
        while angleDiffToMove > math.pi do angleDiffToMove = angleDiffToMove - 2 * math.pi end
        while angleDiffToMove < -math.pi do angleDiffToMove = angleDiffToMove + 2 * math.pi end

        -- Only auto-follow if the direction change isn't too extreme (less than 120 degrees)
        -- This prevents jitter when moving backward
        if math.abs(angleDiffToMove) < 2.1 then  -- ~120 degrees
            cam.targetOrbitAngle = moveAngle
        end
    end

    -- Instantly snap orbit angle to target (no rotation smoothing - user preference)
    -- Only smooth auto-follow, not manual control
    if cam.manualControlTime > cam.manualControlCooldown then
        -- Auto-follow: smoothly rotate toward target
        local angleDiff = cam.targetOrbitAngle - cam.currentOrbitAngle
        while angleDiff > math.pi do angleDiff = angleDiff - 2 * math.pi end
        while angleDiff < -math.pi do angleDiff = angleDiff + 2 * math.pi end
        local orbitSmoothFactor = 1.0 - math.exp(-cam.orbitSmoothing * dt)
        cam.currentOrbitAngle = cam.currentOrbitAngle + angleDiff * orbitSmoothFactor
    else
        -- Manual control: instant angle changes (already set above)
    end

    -- Normalize orbit angle
    while cam.currentOrbitAngle > math.pi do cam.currentOrbitAngle = cam.currentOrbitAngle - 2 * math.pi end
    while cam.currentOrbitAngle < -math.pi do cam.currentOrbitAngle = cam.currentOrbitAngle + 2 * math.pi end

    -- Pitch is also instant (no smoothing)
    -- (targetPitchAngle is already set to currentPitchAngle in manual input section)

    -- Camera offset from player using both yaw and pitch
    -- Pitch affects height and distance (spherical coordinates)
    local horizontalDist = cam.distance * math.cos(cam.currentPitchAngle)
    local verticalOffset = cam.distance * math.sin(cam.currentPitchAngle)

    local offsetX = -math.sin(cam.currentOrbitAngle) * horizontalDist
    local offsetZ = -math.cos(cam.currentOrbitAngle) * horizontalDist

    -- Calculate target camera position
    cam.targetPosition.x = playerPos.x + offsetX
    cam.targetPosition.y = playerPos.y + cam.height + verticalOffset
    cam.targetPosition.z = playerPos.z + offsetZ

    -- Smoothly interpolate camera position (reduced smoothing for less jerk)
    local smoothFactor = 1.0 - math.exp(-cam.smoothing * 0.5 * dt)  -- Halved for smoother movement
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

    -- Calculate target look-at point (player position + look ahead)
    cam.targetLookAt.x = playerPos.x + lookAheadX
    cam.targetLookAt.y = playerPos.y + 1.5  -- Look at character's upper body
    cam.targetLookAt.z = playerPos.z + lookAheadZ

    -- Smoothly interpolate look-at target (prevents jerky camera on direction change)
    local lookAtSmoothFactor = 1.0 - math.exp(-8.0 * dt)  -- Faster than position smoothing
    cam.lookAt.x = cam.lookAt.x + (cam.targetLookAt.x - cam.lookAt.x) * lookAtSmoothFactor
    cam.lookAt.y = cam.lookAt.y + (cam.targetLookAt.y - cam.lookAt.y) * lookAtSmoothFactor
    cam.lookAt.z = cam.lookAt.z + (cam.targetLookAt.z - cam.lookAt.z) * lookAtSmoothFactor

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
