-- Example: Character Controller with Physics Bodies
-- Shows character controller, static terrain, dynamic crates, and raycasting

local MOVE_SPEED = 8.0
local JUMP_FORCE = 12.0
local GRAVITY = -30.0

return {
    title = "Physics Example",
    width = 1280,
    height = 720,

    init = function()
        app.main.state = { velocityY = 0 }
        local state = app.main.state

        -- Set up input (Action Builder)
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
        bestow.input.pushPhase("gameplay")

        -- Create player with character controller
        state.player = bestow.entity.create()
        bestow.entity.addComponent(state.player, "Transform3D", {
            position = Vec3.new(0, 2, 0),
            rotation = Quat.identity(),
            scale = Vec3.one()
        })
        bestow.physics3d.createCharacter(state.player, {
            radius = 0.4,
            height = 1.8,
            stepHeight = 0.35,
            maxSlopeAngle = 45.0,
            mass = 80.0
        })

        -- Ground (static body)
        local ground = bestow.entity.create()
        bestow.entity.addComponent(ground, "Transform3D", {
            position = Vec3.zero(),
            rotation = Quat.identity(),
            scale = Vec3.new(50, 1, 50)
        })
        bestow.physics3d.createBody(ground, {
            type = "Static",
            shapeType = "Box",
            halfExtents = Vec3.new(25, 0.5, 25)
        })

        -- Dynamic crates
        for i = 1, 5 do
            local crate = bestow.entity.create()
            bestow.entity.addComponent(crate, "Transform3D", {
                position = Vec3.new(i * 2 - 6, 5 + i, 5),
                rotation = Quat.identity(),
                scale = Vec3.one()
            })
            bestow.entity.addComponent(crate, "MeshRenderer", {
                mesh = "meshes/cube.obj",
                material = "materials/crate"
            })
            bestow.physics3d.createBody(crate, {
                type = "Dynamic",
                shapeType = "Box",
                halfExtents = Vec3.new(0.5, 0.5, 0.5),
                mass = 10.0,
                friction = 0.5,
                restitution = 0.3
            })
        end
    end,

    update = function(dt)
        local state = app.main.state
        if not state.player then return true end

        -- Movement input from Action Builder
        local moveX, moveZ = 0, 0
        if bestow.input.isActionActive("MoveLeft") then moveX = moveX - 1 end
        if bestow.input.isActionActive("MoveRight") then moveX = moveX + 1 end
        if bestow.input.isActionActive("MoveForward") then moveZ = moveZ - 1 end
        if bestow.input.isActionActive("MoveBack") then moveZ = moveZ + 1 end

        local moveDir = Vec3.new(moveX, 0, moveZ)
        if moveDir:lengthSquared() > 1.0 then
            moveDir = moveDir:normalize()
        end

        -- Ground check and gravity
        local groundInfo = bestow.physics3d.getCharacterGroundInfo(state.player)
        local grounded = groundInfo and groundInfo.grounded

        if grounded then
            state.velocityY = -1
            if bestow.input.wasActionJustPressed("Jump") then
                state.velocityY = JUMP_FORCE
            end
        else
            state.velocityY = state.velocityY + GRAVITY * dt
        end

        -- Apply movement
        local velocity = Vec3.new(moveDir.x * MOVE_SPEED, state.velocityY, moveDir.z * MOVE_SPEED)
        bestow.physics3d.moveCharacter(state.player, velocity, dt)

        -- Raycast down from player for debug
        local playerPos = bestow.entity.getField(state.player, "Transform3D", "position")
        local hit = bestow.physics3d.raycast(playerPos, Vec3.new(0, -1, 0), 100)
        if hit then
            bestow.graphics3d.drawDebugSphere(hit.point, 0.1, Color.red())
        end

        return true
    end,

    render = function()
        bestow.graphics3d.beginFrame()
        bestow.graphics3d.endFrame()
    end
}
