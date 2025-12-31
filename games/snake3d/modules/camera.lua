-- games/snake3d/modules/camera.lua
-- Camera positioning and smooth following
--
-- Matches C++ camera logic from snake.game.cppm lines 2641-2687

local Constants = bestow.include("modules/constants")
local Grid = bestow.include("modules/grid")
local Log = bestow.include("modules/log")

local Camera = {}

local log = Log.category("Camera")

-- Initialize camera state
function Camera.init(game)
    log.debug("Initializing camera system")

    game.cameraTarget = {x = 0, y = 0, z = 0}
    game.cameraAngle = Constants.CAMERA_ANGLE
    game.currentCameraDistance = Constants.BASE_CAMERA_DISTANCE
    game.currentCameraHeight = Constants.BASE_CAMERA_HEIGHT
    game.screenShakeOffset = {x = 0, y = 0, z = 0}
    game.screenShakeTime = 0
    game.screenShakeIntensity = 0

    -- Set FOV (only if the function exists)
    if bestow.graphics.setCameraFOV then
        bestow.graphics.setCameraFOV(Constants.CAMERA_FOV)
        log.trace("Camera FOV set to %.1f degrees", Constants.CAMERA_FOV)
    else
        log.debug("setCameraFOV not available, using default FOV")
    end
end

-- Setup initial camera position (used for menus)
function Camera.setupInitial(game)
    log.debug("Setting up initial camera position")

    bestow.graphics.setCameraPosition(0, 12, 15)
    bestow.graphics.setCameraTarget(0, 1, 0)

    log.trace("Initial camera: pos=(0, 12, 15), target=(0, 1, 0)")
end

-- Update camera to follow snake head
-- Matches C++ updateCamera() at line 2651
function Camera.update(game, dt)
    -- Smooth follow snake head
    if #game.snake > 0 then
        local headWorldPos = Grid.toWorld(game.gridSize, game.snake[1].pos)
        local lerpFactor = Constants.CAMERA_LERP_SPEED * dt

        game.cameraTarget.x = game.cameraTarget.x + (headWorldPos.x - game.cameraTarget.x) * lerpFactor
        game.cameraTarget.y = game.cameraTarget.y + (headWorldPos.y - game.cameraTarget.y) * lerpFactor
        game.cameraTarget.z = game.cameraTarget.z + (headWorldPos.z - game.cameraTarget.z) * lerpFactor

        log.trace("Camera target lerping to (%.2f, %.2f, %.2f)",
            game.cameraTarget.x, game.cameraTarget.y, game.cameraTarget.z)
    end

    -- Update screen shake
    if game.screenShakeTime > 0 then
        game.screenShakeTime = game.screenShakeTime - dt
        local progress = game.screenShakeTime / Constants.SHAKE_DURATION_DEATH
        local currentIntensity = game.screenShakeIntensity * progress

        game.screenShakeOffset.x = (math.random() - 0.5) * 2 * currentIntensity
        game.screenShakeOffset.y = (math.random() - 0.5) * currentIntensity  -- Less vertical
        game.screenShakeOffset.z = (math.random() - 0.5) * 2 * currentIntensity

        log.trace("Screen shake: intensity=%.3f, offset=(%.3f, %.3f, %.3f)",
            currentIntensity, game.screenShakeOffset.x, game.screenShakeOffset.y, game.screenShakeOffset.z)
    else
        game.screenShakeOffset = {x = 0, y = 0, z = 0}
    end

    -- Update camera distance based on grid size
    local targetDistance = Grid.getCameraDistance(game.gridSize)
    local targetHeight = Grid.getCameraHeight(game.gridSize)

    game.currentCameraDistance = game.currentCameraDistance +
        (targetDistance - game.currentCameraDistance) * Constants.ANIMATION_SMOOTH * dt
    game.currentCameraHeight = game.currentCameraHeight +
        (targetHeight - game.currentCameraHeight) * Constants.ANIMATION_SMOOTH * dt

    -- Calculate camera position
    local angleRad = math.rad(game.cameraAngle)
    local camX = game.cameraTarget.x + game.currentCameraDistance * math.sin(angleRad) + game.screenShakeOffset.x
    local camY = game.cameraTarget.y + game.currentCameraHeight + game.screenShakeOffset.y
    local camZ = game.cameraTarget.z + game.currentCameraDistance * math.cos(angleRad) + game.screenShakeOffset.z

    -- Apply to graphics system
    bestow.graphics.setCameraPosition(camX, camY, camZ)
    bestow.graphics.setCameraTarget(game.cameraTarget.x, game.cameraTarget.y, game.cameraTarget.z)
end

-- Update camera for main menu (fixed top-down view matching C++)
function Camera.updateMainMenu(game, dt)
    -- C++ uses fixed position (0, 15, 10) with look direction (0, -0.8, -0.4)
    -- We approximate by setting camera at that position looking toward the scene
    bestow.graphics.setCameraPosition(0, 15, 10)
    -- Target calculated from direction: pos + normalized(0, -0.8, -0.4) * 10
    -- Results in looking at approximately (0, 6, 6)
    bestow.graphics.setCameraTarget(0, 0, 2)

    log.trace("Menu camera: fixed at (0, 15, 10)")
end

-- Update camera for world map (fixed overhead view)
function Camera.updateWorldMap(game, dt)
    bestow.graphics.setCameraPosition(0, 15, 10)
    bestow.graphics.setCameraTarget(0, 0, 0)
end

-- Trigger screen shake effect
function Camera.triggerShake(game, intensity, duration)
    log.debug("Triggering screen shake: intensity=%.2f, duration=%.2f", intensity, duration)

    game.screenShakeIntensity = intensity
    game.screenShakeTime = duration
end

return Camera
