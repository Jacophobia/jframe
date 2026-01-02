-- games/lua-demo/entities/player.lua
-- Player entity definition
--
-- This file defines the player entity blueprint. It will be accessible
-- as app.entities.player after loading.

return {
    -- Default component values for creating a player
    defaults = {
        Transform3D = {
            position = Vec3.new(0, 1, 0),
            rotation = Quat.identity(),
            scale = Vec3.new(1, 1, 1)
        },
        MeshRenderer = {
            mesh = "primitives/cube",
            material = "materials/player"
        },
        Velocity = {
            linear = Vec3.new(0, 0, 0),
            angular = Vec3.new(0, 0, 0)
        },
        PlayerController = {
            moveSpeed = 5.0,
            rotateSpeed = 180.0,
            jumpForce = 10.0
        }
    },

    -- Create a new player entity with optional overrides
    create = function(overrides)
        local self = app.entities.player  -- Access fresh each time (hot reload safe)
        overrides = overrides or {}

        -- Create the entity
        local entity = bestow.entity.create()

        -- Add components with defaults and overrides
        for compName, defaults in pairs(self.defaults) do
            local data = {}
            -- Copy defaults
            for k, v in pairs(defaults) do
                data[k] = v
            end
            -- Apply overrides
            if overrides[compName] then
                for k, v in pairs(overrides[compName]) do
                    data[k] = v
                end
            end
            bestow.entity.addComponent(entity, compName, data)
        end

        return entity
    end,

    -- Helper to spawn player at a specific position
    spawnAt = function(position)
        local self = app.entities.player
        return self.create({
            Transform3D = { position = position }
        })
    end
}
