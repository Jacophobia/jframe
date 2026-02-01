-- Example: Entity Blueprint Pattern
-- Shows creating reusable entity factories with component overrides

-- entities/player.lua
return {
    defaults = {
        Transform3D = {
            position = Vec3.new(0, 1, 0),
            rotation = Quat.identity(),
            scale = Vec3.one()
        },
        Health = {
            current = 100,
            max = 100
        },
        PlayerTag = {}
    },

    create = function(overrides)
        local self = app.entities.player
        local entity = bestow.entity.create()

        for compName, defaults in pairs(self.defaults) do
            local data = {}
            for k, v in pairs(defaults) do data[k] = v end
            if overrides and overrides[compName] then
                for k, v in pairs(overrides[compName]) do data[k] = v end
            end
            bestow.entity.addComponent(entity, compName, data)
        end

        return entity
    end,

    spawnAt = function(position)
        local self = app.entities.player
        return self.create({
            Transform3D = { position = position }
        })
    end
}

-- Usage in main.lua:
-- local player = app.entities.player.create()
-- local player2 = app.entities.player.spawnAt(Vec3.new(10, 1, 0))
--
-- -- Read/write components:
-- local health = bestow.entity.getComponent(player, "Health")
-- health.current = health.current - 25
-- bestow.entity.setComponent(player, "Health", health)
--
-- -- Or use field access for single values:
-- local pos = bestow.entity.getField(player, "Transform3D", "position")
-- bestow.entity.setField(player, "Transform3D", "position", pos + Vec3.new(1, 0, 0))
--
-- -- Check and iterate:
-- if bestow.entity.hasComponent(player, "PlayerTag") then ... end
-- bestow.entity.each(function(entity)
--     if bestow.entity.hasComponent(entity, "Health") then
--         -- Process entity with health
--     end
-- end)
