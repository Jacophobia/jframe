-- games/lua-demo/systems/camera.lua
-- Camera system for the demo game
--
-- Provides a simple follow camera that tracks the player entity.

return {
    -- Camera state
    position = Vec3.new(0, 10, 10),
    target = Vec3.new(0, 0, 0),
    up = Vec3.new(0, 1, 0),
    fov = 60.0,
    nearPlane = 0.1,
    farPlane = 1000.0,
    followDistance = 15.0,
    followHeight = 10.0,
    smoothing = 5.0,

    -- Cached matrices
    viewMatrix = nil,
    projectionMatrix = nil,

    -- Initialize the camera
    init = function()
        local self = app.systems.camera

        -- Calculate initial view matrix
        self.viewMatrix = Mat4.lookAt(self.position, self.target, self.up)

        -- Calculate projection matrix
        local aspectRatio = 1280.0 / 720.0  -- TODO: Get from graphics system
        self.projectionMatrix = Mat4.perspective(
            math.rad(self.fov),
            aspectRatio,
            self.nearPlane,
            self.farPlane
        )

        print("[Camera] Initialized")
    end,

    -- Update the camera each frame
    update = function(dt)
        local self = app.systems.camera
        local state = app.main.state

        if not state or not state.player then
            return
        end

        -- Get player position
        local transform = bestow.entity.getComponent(state.player, "Transform3D")
        if not transform then
            return
        end

        -- Calculate desired camera position (behind and above player)
        local desiredPosition = Vec3.new(
            transform.position.x,
            transform.position.y + self.followHeight,
            transform.position.z + self.followDistance
        )

        -- Smooth camera movement
        local t = math.min(1.0, self.smoothing * dt)
        self.position = self.position:lerp(desiredPosition, t)
        self.target = self.target:lerp(transform.position, t)

        -- Update view matrix
        self.viewMatrix = Mat4.lookAt(self.position, self.target, self.up)
    end,

    -- Get the current view matrix
    getViewMatrix = function()
        local self = app.systems.camera
        return self.viewMatrix
    end,

    -- Get the current projection matrix
    getProjectionMatrix = function()
        local self = app.systems.camera
        return self.projectionMatrix
    end,

    -- Set field of view
    setFov = function(fov)
        local self = app.systems.camera
        self.fov = fov

        -- Recalculate projection matrix
        local aspectRatio = 1280.0 / 720.0
        self.projectionMatrix = Mat4.perspective(
            math.rad(self.fov),
            aspectRatio,
            self.nearPlane,
            self.farPlane
        )
    end,

    -- Get Camera3D struct for bestow.graphics3d.setCamera()
    getCamera = function()
        local self = app.systems.camera

        -- Create a Camera3D struct
        local camera = Camera3D.new()

        -- Set transform (position + look direction)
        camera.transform = Transform3D.new()
        camera.transform.position = self.position

        -- Calculate rotation from lookAt direction
        local forward = (self.target - self.position):normalize()
        camera.transform.rotation = Quat.lookRotation(forward, self.up)

        -- Set projection parameters
        camera.projection = ProjectionType.Perspective
        camera.fovY = math.rad(self.fov)
        camera.aspectRatio = 1280.0 / 720.0
        camera.nearPlane = self.nearPlane
        camera.farPlane = self.farPlane

        return camera
    end
}
