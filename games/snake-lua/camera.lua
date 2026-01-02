-- camera.lua - Camera management
-- Dependencies: app.config, app.state, app.rendering (accessed inside functions)

local camera = {}

-- Set up the initial camera
function camera.setup()
    local cam = Camera3D.new()
    cam.fovY = 45.0
    cam.nearPlane = 0.1
    cam.farPlane = 100.0

    camera.updateTransform(cam)
    bestow.graphics3d.setCamera(cam)
end

-- Update camera position and orientation
function camera.update(dt)
    local state = app.state
    local rendering = app.rendering

    if #state.snake > 0 then
        local headWorldPos = rendering.gridToWorld(state.snake[1].pos)
        state.cameraTarget = Vec3.new(
            state.cameraTarget.x + (headWorldPos.x - state.cameraTarget.x) * (5.0 * dt),
            state.cameraTarget.y + (headWorldPos.y - state.cameraTarget.y) * (5.0 * dt),
            state.cameraTarget.z + (headWorldPos.z - state.cameraTarget.z) * (5.0 * dt)
        )
    end

    local cam = bestow.graphics3d.getCamera()
    camera.updateTransform(cam)
    bestow.graphics3d.setCamera(cam)
end

-- Update camera transform (position and rotation)
function camera.updateTransform(cam)
    local state = app.state

    -- Use smoothly animated camera distance/height
    local angleRad = math.rad(state.cameraAngle)
    local cameraPos = Vec3.new(
        state.cameraTarget.x + state.currentCameraDistance * math.sin(angleRad),
        state.cameraTarget.y + state.currentCameraHeight,
        state.cameraTarget.z + state.currentCameraDistance * math.cos(angleRad)
    )

    -- Apply screen shake offset
    cameraPos = Vec3.new(
        cameraPos.x + state.screenShakeOffset.x,
        cameraPos.y + state.screenShakeOffset.y,
        cameraPos.z + state.screenShakeOffset.z
    )

    cam.transform.position = cameraPos

    -- Calculate look direction
    local lookDir = Vec3.new(
        state.cameraTarget.x - cameraPos.x,
        state.cameraTarget.y - cameraPos.y,
        state.cameraTarget.z - cameraPos.z
    ):normalize()

    -- Calculate rotation quaternion to look at target
    local up = Vec3.new(0.0, 1.0, 0.0)
    cam.transform.rotation = Quat.lookAt(lookDir, up)
end

-- Update camera zoom based on grid size
function camera.updateZoom(dt)
    local config = app.config
    local state = app.state

    -- Use visual grid size for smooth transition during expansion
    local cameraTargetSize = state.isExpanding and state.visualGridSize or state.gridSize

    local targetDistance = config.BASE_CAMERA_DISTANCE +
        (cameraTargetSize - config.INITIAL_GRID_SIZE) * config.CAMERA_SCALE
    local targetHeight = config.BASE_CAMERA_HEIGHT +
        (cameraTargetSize - config.INITIAL_GRID_SIZE) * config.CAMERA_SCALE * 0.8

    state.currentCameraDistance = state.currentCameraDistance +
        (targetDistance - state.currentCameraDistance) * config.ANIMATION_SMOOTH * dt
    state.currentCameraHeight = state.currentCameraHeight +
        (targetHeight - state.currentCameraHeight) * config.ANIMATION_SMOOTH * dt
end

-- Update screen shake effect
function camera.updateScreenShake(dt)
    local state = app.state

    if state.screenShakeTimer > 0 then
        state.screenShakeTimer = state.screenShakeTimer - dt
        local progress = state.screenShakeTimer / state.screenShakeDuration
        local currentIntensity = state.screenShakeIntensity * progress

        -- Random shake offset
        state.screenShakeOffset = Vec3.new(
            (math.random() * 2.0 - 1.0) * currentIntensity,
            (math.random() * 2.0 - 1.0) * currentIntensity * 0.5,
            (math.random() * 2.0 - 1.0) * currentIntensity
        )
    else
        state.screenShakeOffset = Vec3.new(0, 0, 0)
    end
end

-- Trigger screen shake effect
function camera.triggerScreenShake(intensity, duration)
    local state = app.state
    state.screenShakeIntensity = intensity
    state.screenShakeDuration = duration
    state.screenShakeTimer = duration
end

-- Update food pop effect
function camera.updateFoodPop(dt)
    local config = app.config
    local state = app.state

    if state.foodPopTimer > 0 then
        state.foodPopTimer = state.foodPopTimer - dt
        state.foodPopScale = state.foodPopTimer / config.FOOD_POP_DURATION
    end
end

-- Trigger food pop effect
function camera.triggerFoodPop(pos)
    local config = app.config
    local state = app.state

    state.lastFoodPos = pos
    state.foodPopTimer = config.FOOD_POP_DURATION
    state.foodPopScale = 1.0
end

-- Full camera update (includes zoom, shake, and pop effects)
function camera.fullUpdate(dt)
    camera.updateZoom(dt)
    camera.updateScreenShake(dt)
    camera.updateFoodPop(dt)
    camera.update(dt)
end

return camera
