--[[
    Camera Module

    Third-person follow camera that smoothly tracks the player.
    The camera maintains a fixed distance and height above the player,
    with optional look-ahead in the movement direction.
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
    }

    appSelf.transient.camera = cam
    return cam
end

-- Update camera to follow player
function camera.update(appSelf, dt, scope)
    local cam = appSelf.transient.camera
    local p = appSelf.transient.player
    if not cam or not p then return end

    -- Calculate target position behind and above player
    local playerPos = p.position
    local playerRot = p.rotation

    -- Calculate look-ahead offset based on player velocity
    local lookAheadX = 0
    local lookAheadZ = 0
    local velMag = math.sqrt(p.velocity.x * p.velocity.x + p.velocity.z * p.velocity.z)
    if velMag > 0.1 then
        local lookFactor = math.min(velMag / 5.0, 1.0) * cam.lookAhead
        lookAheadX = (p.velocity.x / velMag) * lookFactor
        lookAheadZ = (p.velocity.z / velMag) * lookFactor
    end

    -- Camera offset from player (behind and above)
    local offsetX = -math.sin(playerRot) * cam.distance
    local offsetZ = -math.cos(playerRot) * cam.distance

    -- Calculate target camera position
    cam.targetPosition.x = playerPos.x + offsetX
    cam.targetPosition.y = playerPos.y + cam.height
    cam.targetPosition.z = playerPos.z + offsetZ

    -- Smoothly interpolate camera position
    local smoothFactor = 1.0 - math.exp(-cam.smoothing * dt)
    cam.position.x = cam.position.x + (cam.targetPosition.x - cam.position.x) * smoothFactor
    cam.position.y = cam.position.y + (cam.targetPosition.y - cam.position.y) * smoothFactor
    cam.position.z = cam.position.z + (cam.targetPosition.z - cam.position.z) * smoothFactor

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
