-- Example: Basic 3D Scene Setup
-- Shows camera, lighting, fog, skybox, and mesh rendering

return {
    title = "3D Scene Example",
    width = 1280,
    height = 720,

    init = function()
        app.main.state = {}

        -- Camera
        bestow.graphics3d.setCamera({
            position = Vec3.new(0, 10, -20),
            rotation = Quat.lookAt(
                (Vec3.new(0, 0, 0) - Vec3.new(0, 10, -20)):normalize(),
                Vec3.up()
            ),
            fov = 45.0,
            near = 0.1,
            far = 1000.0
        })

        -- Lighting
        bestow.graphics3d.setAmbientLight(
            Vec3.new(0.2, 0.2, 0.3),
            0.3
        )
        bestow.graphics3d.setDirectionalLight({
            direction = Vec3.new(-0.5, -1, -0.5):normalize(),
            color = Vec3.new(1.0, 0.95, 0.8),
            intensity = 1.0,
            castShadows = true
        })

        -- Fog
        bestow.graphics3d.setFog({
            enabled = true,
            color = Color.new(0.5, 0.6, 0.7, 1.0),
            startDistance = 30.0,
            endDistance = 150.0
        })

        -- Skybox (requires a cubemap texture handle loaded via bestow.assets)
        -- local cubemapHandle = bestow.assets.registerAsset(AssetType.Cubemap, "textures/skybox")
        -- bestow.assets.loadAsset(cubemapHandle)
        -- local skybox = Skybox.new()
        -- skybox.cubemapTexture = cubemapHandle
        -- bestow.graphics3d.setSkybox(skybox)

        -- Ground plane
        local ground = bestow.entity.create()
        bestow.entity.addComponent(ground, "Transform3D", {
            position = Vec3.zero(),
            rotation = Quat.identity(),
            scale = Vec3.new(50, 1, 50)
        })
        bestow.entity.addComponent(ground, "MeshRenderer", {
            mesh = "meshes/plane.obj",
            material = "materials/grass"
        })
        bestow.physics3d.createBody(ground, {
            type = "Static",
            shapeType = "Box",
            shapeHalfExtents = Vec3.new(25, 0.5, 25)
        })

        -- Some objects
        for i = 1, 5 do
            local cube = bestow.entity.create()
            bestow.entity.addComponent(cube, "Transform3D", {
                position = Vec3.new(i * 3 - 9, 1, 0),
                rotation = Quat.identity(),
                scale = Vec3.one()
            })
            bestow.entity.addComponent(cube, "MeshRenderer", {
                mesh = "meshes/cube.obj",
                material = "materials/default"
            })
        end
    end,

    update = function(dt)
        return true
    end,

    render = function()
        bestow.graphics3d.beginFrame()
        -- Entities with MeshRenderer are drawn automatically

        -- Debug drawing (dev only)
        bestow.graphics3d.debugDrawLine(
            Vec3.zero(), Vec3.new(5, 0, 0), Color.red()
        )
        bestow.graphics3d.debugDrawLine(
            Vec3.zero(), Vec3.new(0, 5, 0), Color.green()
        )
        bestow.graphics3d.debugDrawLine(
            Vec3.zero(), Vec3.new(0, 0, 5), Color.blue()
        )

        bestow.graphics3d.endFrame()
    end
}
