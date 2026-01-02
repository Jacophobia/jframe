-- games/lua-demo/levels/demo.lua
-- Demo level definition
--
-- Defines the layout and entities for the demo level.

return {
    name = "Demo Level",
    description = "A simple demo level showcasing the Lua-driven engine",

    -- Level bounds
    bounds = {
        min = Vec3.new(-50, -10, -50),
        max = Vec3.new(50, 50, 50)
    },

    -- Ambient lighting
    ambient = {
        color = Color.new(0.2, 0.25, 0.3, 1.0),
        intensity = 0.5
    },

    -- Directional light (sun)
    sun = {
        direction = Vec3.new(-0.5, -1.0, -0.5):normalize(),
        color = Color.new(1.0, 0.95, 0.9, 1.0),
        intensity = 1.2
    },

    -- Fog settings
    fog = {
        enabled = true,
        color = Color.new(0.6, 0.7, 0.8, 1.0),
        density = 0.01,
        startDistance = 20.0,
        endDistance = 100.0
    },

    -- Skybox
    skybox = {
        texture = "assets/textures/skybox_day.hdr"
    },

    -- Ground plane
    ground = {
        material = "assets/materials/grass.mat",
        size = Vec2.new(100, 100),
        tiling = Vec2.new(10, 10)
    },

    -- Entity spawn points
    spawns = {
        player = Vec3.new(0, 1, 0),
        camera = Vec3.new(0, 10, 10)
    },

    -- Static objects in the level
    objects = {
        -- Trees
        {
            type = "tree",
            positions = {
                Vec3.new(5, 0, 5),
                Vec3.new(-8, 0, 3),
                Vec3.new(12, 0, -7),
                Vec3.new(-15, 0, -12),
                Vec3.new(20, 0, 8)
            }
        },
        -- Rocks
        {
            type = "rock",
            positions = {
                Vec3.new(3, 0, -5),
                Vec3.new(-10, 0, 10),
                Vec3.new(7, 0, 15)
            }
        },
        -- Buildings
        {
            type = "building",
            position = Vec3.new(-20, 0, -20),
            rotation = Quat.fromAxisAngle(Vec3.new(0, 1, 0), math.rad(45)),
            scale = Vec3.new(2, 2, 2)
        }
    },

    -- Load the level
    load = function()
        local self = app.levels.demo
        print("[Level] Loading: " .. self.name)

        -- Spawn player at the designated spawn point
        local player = app.entities.player
        if player and player.spawnAt then
            local playerEntity = player.spawnAt(self.spawns.player)
            app.main.state.player = playerEntity
        end

        -- Set up lighting
        if bestow.graphics3d then
            if bestow.graphics3d.setAmbientLight then
                bestow.graphics3d.setAmbientLight(
                    self.ambient.color,
                    self.ambient.intensity
                )
            end

            if bestow.graphics3d.setDirectionalLight then
                bestow.graphics3d.setDirectionalLight(
                    self.sun.direction,
                    self.sun.color,
                    self.sun.intensity
                )
            end

            if bestow.graphics3d.setFog then
                bestow.graphics3d.setFog(
                    self.fog.enabled,
                    self.fog.color,
                    self.fog.density,
                    self.fog.startDistance,
                    self.fog.endDistance
                )
            end
        end

        -- Create static objects
        for _, objectGroup in ipairs(self.objects) do
            if objectGroup.positions then
                -- Multiple instances at different positions
                for _, pos in ipairs(objectGroup.positions) do
                    self.createObject(objectGroup.type, pos)
                end
            else
                -- Single instance
                self.createObject(
                    objectGroup.type,
                    objectGroup.position,
                    objectGroup.rotation,
                    objectGroup.scale
                )
            end
        end

        print("[Level] Loaded!")
        return true
    end,

    -- Helper to create a level object
    createObject = function(objectType, position, rotation, scale)
        local entity = bestow.entity.create()

        rotation = rotation or Quat.identity()
        scale = scale or Vec3.new(1, 1, 1)

        bestow.entity.addComponent(entity, "Transform3D", {
            position = position,
            rotation = rotation,
            scale = scale
        })

        -- TODO: Add mesh based on object type
        bestow.entity.addComponent(entity, "MeshRenderer", {
            mesh = "primitives/" .. objectType,
            material = "materials/" .. objectType
        })

        return entity
    end,

    -- Unload the level
    unload = function()
        local self = app.levels.demo
        print("[Level] Unloading: " .. self.name)

        -- Destroy all entities
        bestow.entity.each(function(entity)
            bestow.entity.destroy(entity)
        end)

        print("[Level] Unloaded!")
    end
}
