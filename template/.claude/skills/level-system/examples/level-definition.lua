-- Example: Level Definition and Loading
-- Shows level data, spawning objects, transitions, and cleanup

-- levels/forest.lua
return {
    name = "Forest Clearing",

    spawns = {
        player = Vec3.new(0, 1, 0),
        checkpoint1 = Vec3.new(50, 1, 0),
        exit = Vec3.new(100, 1, 0)
    },

    lighting = {
        ambient = { color = Color.new(0.3, 0.35, 0.4, 1), intensity = 0.4 },
        sun = {
            direction = Vec3.new(-0.5, -1, -0.5):normalize(),
            color = Color.new(1, 0.95, 0.8, 1),
            intensity = 1.0
        }
    },

    fog = {
        enabled = true,
        color = Color.new(0.6, 0.7, 0.8, 1),
        startDistance = 40,
        endDistance = 150
    },

    objects = {
        { type = "enemy", position = Vec3.new(20, 0, 5), params = { kind = "goblin" } },
        { type = "enemy", position = Vec3.new(35, 0, -3), params = { kind = "goblin" } },
        { type = "coin", position = Vec3.new(10, 1.5, 0) },
        { type = "coin", position = Vec3.new(12, 1.5, 0) },
        { type = "coin", position = Vec3.new(14, 1.5, 0) },
        { type = "checkpoint", position = Vec3.new(50, 0, 0), params = { id = "checkpoint1" } },
        { type = "door", position = Vec3.new(100, 0, 0), params = { to = "cave", spawn = "entrance" } },
    },

    load = function()
        local self = app.levels.forest
        local state = app.main.state

        -- Set up lighting
        bestow.graphics3d.setAmbientLight(self.lighting.ambient.color, self.lighting.ambient.intensity)
        bestow.graphics3d.setDirectionalLight(self.lighting.sun)
        bestow.graphics3d.setFog(self.fog)

        -- Create ground
        state.levelEntities = {}
        local ground = bestow.entity.create()
        bestow.entity.addComponent(ground, "Transform3D", {
            position = Vec3.zero(),
            rotation = Quat.identity(),
            scale = Vec3.new(200, 1, 100)
        })
        bestow.entity.addComponent(ground, "MeshRenderer", {
            mesh = "meshes/plane.obj",
            material = "materials/grass"
        })
        bestow.physics3d.createBody(ground, {
            type = "Static",
            shapeType = "Box",
            halfExtents = Vec3.new(100, 0.5, 50)
        })
        table.insert(state.levelEntities, ground)

        -- Spawn all objects
        for _, obj in ipairs(self.objects) do
            local entity = app.main.spawnLevelObject(obj)
            if entity then
                table.insert(state.levelEntities, entity)
            end
        end

        -- Spawn player
        local spawnPoint = state.nextSpawnPoint or "player"
        local spawnPos = self.spawns[spawnPoint] or self.spawns.player
        state.player = app.entities.player.spawnAt(spawnPos)
        state.nextSpawnPoint = nil
        state.currentLevel = "forest"
    end,

    unload = function()
        local state = app.main.state
        if state.levelEntities then
            for _, entity in ipairs(state.levelEntities) do
                if bestow.entity.isValid(entity) then
                    bestow.entity.destroy(entity)
                end
            end
            state.levelEntities = {}
        end
    end
}

-- In main.lua:
-- app.main.loadLevel = function(levelName)
--     local state = app.main.state
--     if state.currentLevel then
--         app.levels[state.currentLevel].unload()
--     end
--     app.levels[levelName].load()
-- end
--
-- app.main.spawnLevelObject = function(obj)
--     if obj.type == "enemy" then
--         return app.entities.enemies[obj.params.kind].create(obj.position)
--     elseif obj.type == "coin" then
--         return app.entities.collectibles.coin.create(obj.position)
--     end
-- end
