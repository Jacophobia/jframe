-- Example: Complete Game Architecture
-- Shows the correct structure for a full Bestow game with all patterns applied

----------------------------------------------------------------------
-- main.lua (Entry Point)
----------------------------------------------------------------------
return {
    title = "Forest Adventure",
    width = 1280,
    height = 720,

    init = function()
        -- 1. State first
        app.main.state = {
            player = nil,
            score = 0,
            lives = 3,
            phase = "playing",
            currentLevel = nil,
            levelEntities = {},
            running = true
        }

        -- 2. Input actions (Dvorak + QWERTY)
        local k = KeyCode
        for _, key in ipairs({k.Comma, k.W}) do
            bestow.action.builder():duringPhase("gameplay"):whenActive(key):emitAction("MoveForward"):continuously()
        end
        for _, key in ipairs({k.O, k.S}) do
            bestow.action.builder():duringPhase("gameplay"):whenActive(key):emitAction("MoveBack"):continuously()
        end
        bestow.action.builder():duringPhase("gameplay"):whenActive(k.A):emitAction("MoveLeft"):continuously()
        for _, key in ipairs({k.E, k.D}) do
            bestow.action.builder():duringPhase("gameplay"):whenActive(key):emitAction("MoveRight"):continuously()
        end
        bestow.action.builder():duringPhase("gameplay"):whenPressed(k.Space):emitAction("Jump"):discretely()
        bestow.action.builder():duringPhase("gameplay"):whenPressed(k.Escape):emitAction("Pause"):discretely()

        -- 3. Rendering environment
        bestow.graphics3d.setCamera({
            position = Vec3.new(0, 10, -15),
            rotation = Quat.lookAt(Vec3.new(0, -0.5, 1):normalize(), Vec3.up()),
            fov = 45.0, near = 0.1, far = 500.0
        })
        bestow.graphics3d.setAmbientLight(Color.new(0.3, 0.3, 0.4, 1), 0.4)
        bestow.graphics3d.setDirectionalLight({
            direction = Vec3.new(-0.5, -1, -0.5):normalize(),
            color = Color.new(1, 0.95, 0.8, 1),
            intensity = 1.0
        })

        -- 4. Initialize systems
        app.systems.combat.init()
        app.systems.score.init()
        app.systems.collectibles.init()

        -- 5. Initialize UI
        bestow.ui.initialize()

        -- 6. Load first level (LAST)
        bestow.phase.push("gameplay")
        app.levels.forest.load()
    end,

    update = function(dt)
        local state = app.main.state
        bestow.timer.update(dt)
        bestow.ui.processInput()

        if state.phase == "playing" then
            app.systems.movement.update(dt)
            app.systems.enemy_ai.update(dt)
            app.systems.combat.update(dt)
            app.systems.collectibles.update(dt)
            app.systems.camera.update(dt)
        end

        return state.running
    end,

    render = function()
        bestow.graphics3d.beginFrame()
        bestow.ui.render()
        bestow.graphics3d.endFrame()
    end,

    shutdown = function()
        app.systems.collectibles.shutdown()
        app.systems.score.shutdown()
        app.systems.combat.shutdown()
        bestow.ui.shutdown()
    end,

    -- Helper for spawning level objects
    spawnObject = function(obj)
        if obj.type == "enemy" then
            return app.entities.enemies[obj.params.kind].create(obj.position)
        elseif obj.type == "coin" then
            return app.entities.collectibles.coin.create(obj.position)
        end
        return nil
    end,

    loadLevel = function(levelName)
        local state = app.main.state
        if state.currentLevel then
            app.levels[state.currentLevel].unload()
        end
        app.levels[levelName].load()
    end
}

----------------------------------------------------------------------
-- entities/player.lua (Pure Factory)
----------------------------------------------------------------------
-- return {
--     create = function(position)
--         local entity = bestow.entity.create()
--
--         bestow.entity.addComponent(entity, "Transform3D", {
--             position = position or Vec3.new(0, 1, 0),
--             rotation = Quat.identity(),
--             scale = Vec3.one()
--         })
--         bestow.entity.addComponent(entity, "MeshRenderer", {
--             mesh = "meshes/player.obj",
--             material = "materials/player"
--         })
--         bestow.entity.addComponent(entity, "Health", { current = 100, max = 100 })
--         bestow.entity.addComponent(entity, "PlayerTag", {})
--
--         bestow.physics3d.createBody(entity, {
--             type = "Dynamic",
--             shapeType = "Capsule",
--             radius = 0.4,
--             height = 1.8,
--             mass = 70.0
--         })
--
--         return entity
--     end
-- }

----------------------------------------------------------------------
-- systems/movement.lua (System with Init/Update/Shutdown)
----------------------------------------------------------------------
-- local MOVE_SPEED = 8.0
--
-- return {
--     update = function(dt)
--         local self = app.systems.movement
--         local state = app.main.state
--         if not state.player or not bestow.entity.isValid(state.player) then return end
--
--         local moveX, moveZ = 0, 0
--         if bestow.input.isActionActive("MoveLeft") then moveX = moveX - 1 end
--         if bestow.input.isActionActive("MoveRight") then moveX = moveX + 1 end
--         if bestow.input.isActionActive("MoveForward") then moveZ = moveZ - 1 end
--         if bestow.input.isActionActive("MoveBack") then moveZ = moveZ + 1 end
--
--         local moveDir = Vec3.new(moveX, 0, moveZ)
--         if moveDir:lengthSquared() > 1.0 then
--             moveDir = moveDir:normalize()
--         end
--
--         local groundInfo = bestow.physics3d.getCharacterGroundInfo(state.player)
--         local gravityY = groundInfo.isGrounded and -1 or -20 * dt
--         bestow.physics3d.moveCharacter(
--             state.player,
--             Vec3.new(moveDir.x * MOVE_SPEED, gravityY, moveDir.z * MOVE_SPEED),
--             dt
--         )
--     end,
--
--     shutdown = function() end
-- }

----------------------------------------------------------------------
-- systems/combat.lua (Event-Driven System)
----------------------------------------------------------------------
-- return {
--     deathSubId = nil,
--
--     init = function()
--         local self = app.systems.combat
--         if self.deathSubId then bestow.events.unsubscribe(self.deathSubId) end
--         self.deathSubId = bestow.events.subscribe("entity_damaged", {},
--             app.systems.combat, "onDamaged")
--     end,
--
--     onDamaged = function(event)
--         local self = app.systems.combat
--         if event.remainingHealth <= 0 then
--             bestow.events.emit("entity_died", {
--                 entity = event.target,
--                 killer = event.source
--             })
--             bestow.entity.destroy(event.target)
--         end
--     end,
--
--     dealDamage = function(target, amount, source)
--         local health = bestow.entity.getComponent(target, "Health")
--         if not health then return end
--         health.current = math.max(0, health.current - amount)
--         bestow.entity.setComponent(target, "Health", health)
--         bestow.events.emit("entity_damaged", {
--             target = target, source = source,
--             amount = amount, remainingHealth = health.current
--         })
--     end,
--
--     shutdown = function()
--         local self = app.systems.combat
--         if self.deathSubId then bestow.events.unsubscribe(self.deathSubId) end
--     end
-- }

----------------------------------------------------------------------
-- levels/forest.lua (Declarative + Load/Unload)
----------------------------------------------------------------------
-- return {
--     name = "Forest",
--     spawns = { player = Vec3.new(0, 1, 0) },
--     objects = {
--         { type = "enemy", position = Vec3.new(20, 0, 5), params = { kind = "goblin" } },
--         { type = "coin", position = Vec3.new(10, 1.5, 0) },
--     },
--
--     load = function()
--         local self = app.levels.forest
--         local state = app.main.state
--         state.levelEntities = {}
--
--         -- Create ground
--         local ground = bestow.entity.create()
--         bestow.entity.addComponent(ground, "Transform3D", {
--             position = Vec3.zero(), rotation = Quat.identity(),
--             scale = Vec3.new(200, 1, 100)
--         })
--         bestow.entity.addComponent(ground, "MeshRenderer", {
--             mesh = "meshes/plane.obj", material = "materials/grass"
--         })
--         bestow.physics3d.createBody(ground, {
--             type = "Static", shapeType = "Box",
--             shapeHalfExtents = Vec3.new(100, 0.5, 50)
--         })
--         table.insert(state.levelEntities, ground)
--
--         for _, obj in ipairs(self.objects) do
--             local entity = app.main.spawnObject(obj)
--             if entity then table.insert(state.levelEntities, entity) end
--         end
--
--         state.player = app.entities.player.create(self.spawns.player)
--         state.currentLevel = "forest"
--     end,
--
--     unload = function()
--         local state = app.main.state
--         if state.levelEntities then
--             for _, e in ipairs(state.levelEntities) do
--                 if bestow.entity.isValid(e) then bestow.entity.destroy(e) end
--             end
--             state.levelEntities = {}
--         end
--     end
-- }
