-- Example: Full Movement Input Setup with Action Builder
-- Shows Dvorak + QWERTY + Gamepad support

return {
    init = function()
        local k = KeyCode
        local ga = GamepadAxis
        local gb = GamepadButton

        -- Forward (Dvorak: , / QWERTY: W)
        for _, key in ipairs({k.Comma, k.W}) do
            bestow.action.builder():duringPhase("gameplay"):whenActive(key):emitAction("MoveForward"):continuously()
        end
        -- Back (Dvorak: O / QWERTY: S)
        for _, key in ipairs({k.O, k.S}) do
            bestow.action.builder():duringPhase("gameplay"):whenActive(key):emitAction("MoveBack"):continuously()
        end
        -- Left (same on both)
        bestow.action.builder():duringPhase("gameplay"):whenActive(k.A):emitAction("MoveLeft"):continuously()
        -- Right (Dvorak: E / QWERTY: D)
        for _, key in ipairs({k.E, k.D}) do
            bestow.action.builder():duringPhase("gameplay"):whenActive(key):emitAction("MoveRight"):continuously()
        end

        -- Gamepad analog sticks
        bestow.action.builder():duringPhase("gameplay"):whenActive(ga.LeftX):withDeadzone(0.15):emitAction("MoveAxisX"):continuously()
        bestow.action.builder():duringPhase("gameplay"):whenActive(ga.LeftY):withDeadzone(0.15):emitAction("MoveAxisY"):continuously()

        -- Jump (keyboard + gamepad)
        bestow.action.builder():duringPhase("gameplay"):whenPressed(k.Space):emitAction("Jump"):discretely()
        bestow.action.builder():duringPhase("gameplay"):whenPressed(gb.A):emitAction("Jump"):discretely()

        -- Sprint (hold)
        bestow.action.builder():duringPhase("gameplay"):whenHeld(k.LeftShift, 0.0):emitAction("Sprint"):continuously()

        -- Pause (pushes new phase)
        bestow.action.builder():duringPhase("gameplay"):whenPressed(k.Escape):pushPhase("pause"):discretely()
        bestow.action.builder():duringPhase("pause"):whenPressed(k.Escape):popPhase():discretely()

        -- Start in gameplay phase
        bestow.input.pushPhase("gameplay")
    end,

    update = function(dt)
        -- Read digital movement
        local moveX, moveZ = 0, 0
        if bestow.input.isActionActive("MoveLeft") then moveX = moveX - 1 end
        if bestow.input.isActionActive("MoveRight") then moveX = moveX + 1 end
        if bestow.input.isActionActive("MoveForward") then moveZ = moveZ - 1 end
        if bestow.input.isActionActive("MoveBack") then moveZ = moveZ + 1 end

        -- Override with analog stick if present
        local stickX = bestow.input.getActionValue("MoveAxisX") or 0
        local stickY = bestow.input.getActionValue("MoveAxisY") or 0
        if math.abs(stickX) > 0.01 then moveX = stickX end
        if math.abs(stickY) > 0.01 then moveZ = stickY end

        -- Normalize diagonal movement
        local moveDir = Vec3.new(moveX, 0, moveZ)
        if moveDir:lengthSquared() > 1.0 then
            moveDir = moveDir:normalize()
        end

        -- Sprint multiplier
        local speed = 8.0
        if bestow.input.isActionActive("Sprint") then
            speed = speed * 1.8
        end

        -- Jump
        if bestow.input.wasActionJustPressed("Jump") then
            -- Handle jump
        end

        return true
    end
}
