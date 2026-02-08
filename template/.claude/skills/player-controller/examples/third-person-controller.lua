-- Example: Third-Person Player Controller
-- Shows camera-relative movement, jumping, sprinting, and character controller

local MOVE_SPEED = 8.0
local JUMP_FORCE = 12.0
local GRAVITY = -30.0
local SPRINT_MULTIPLIER = 1.8

-- systems/player.lua
return {
    velocityY = 0,

    init = function()
        local self = app.systems.player
        local state = app.main.state

        -- Create player entity
        state.player = bestow.entity.create()
        bestow.entity.addComponent(state.player, "Transform3D", {
            position = Vec3.new(0, 1, 0),
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

        -- Set up input with Action Builder (Dvorak + QWERTY + Gamepad)
        local k = KeyCode
        local ga = GamepadAxis
        local gb = GamepadButton

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
        bestow.action.builder():duringPhase("gameplay"):whenActive(ga.LeftX):withDeadzone(0.15):emitAction("MoveAxisX"):continuously()
        bestow.action.builder():duringPhase("gameplay"):whenActive(ga.LeftY):withDeadzone(0.15):emitAction("MoveAxisY"):continuously()
        bestow.action.builder():duringPhase("gameplay"):whenPressed(k.Space):emitAction("Jump"):discretely()
        bestow.action.builder():duringPhase("gameplay"):whenPressed(gb.A):emitAction("Jump"):discretely()
        bestow.action.builder():duringPhase("gameplay"):whenHeld(k.LeftShift, 0.0):emitAction("Sprint"):continuously()

        bestow.phase.push("gameplay")
    end,

    update = function(dt)
        local self = app.systems.player
        local state = app.main.state
        if not state.player then return end

        -- Get raw input
        local inputX, inputZ = 0, 0
        if bestow.input.isActionActive("MoveLeft") then inputX = inputX - 1 end
        if bestow.input.isActionActive("MoveRight") then inputX = inputX + 1 end
        if bestow.input.isActionActive("MoveForward") then inputZ = inputZ - 1 end
        if bestow.input.isActionActive("MoveBack") then inputZ = inputZ + 1 end

        -- Analog override
        local stickX = bestow.input.getActionValue("MoveAxisX") or 0
        local stickY = bestow.input.getActionValue("MoveAxisY") or 0
        if math.abs(stickX) > 0.01 then inputX = stickX end
        if math.abs(stickY) > 0.01 then inputZ = stickY end

        -- Camera-relative movement
        local moveDir = Vec3.zero()
        if math.abs(inputX) > 0.01 or math.abs(inputZ) > 0.01 then
            local cam = bestow.graphics3d.getCamera()
            local camForward = cam.rotation:rotateVector(Vec3.forward())
            camForward = Vec3.new(camForward.x, 0, camForward.z):normalize()
            local camRight = cam.rotation:rotateVector(Vec3.right())
            camRight = Vec3.new(camRight.x, 0, camRight.z):normalize()

            moveDir = camRight * inputX + camForward * inputZ
            if moveDir:lengthSquared() > 1.0 then
                moveDir = moveDir:normalize()
            end

            -- Face movement direction
            if moveDir:lengthSquared() > 0.01 then
                local newRot = Quat.fromAxisAngle(Vec3.up(),
                    math.atan2(-moveDir.x, -moveDir.z))
                bestow.entity.setField(state.player, "Transform3D", "rotation", newRot)
            end
        end

        -- Sprint
        local speed = MOVE_SPEED
        if bestow.input.isActionActive("Sprint") then
            speed = speed * SPRINT_MULTIPLIER
        end

        -- Ground check, gravity, jump
        local groundInfo = bestow.physics3d.getCharacterGroundInfo(state.player)
        local grounded = groundInfo and groundInfo.state == CharacterGroundState.OnGround

        if grounded then
            self.velocityY = -1
            if bestow.input.wasActionJustPressed("Jump") then
                self.velocityY = JUMP_FORCE
            end
        else
            self.velocityY = self.velocityY + GRAVITY * dt
        end

        -- Apply movement
        local velocity = Vec3.new(moveDir.x * speed, self.velocityY, moveDir.z * speed)
        bestow.physics3d.moveCharacter(state.player, velocity, dt)
    end
}
