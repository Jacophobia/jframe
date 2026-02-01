---
name: player-controller
description: Implement player movement, jumping, character controllers, and abilities in Bestow. Use when creating a player character with movement, jumping, sprinting, camera-relative movement, or any player control logic.
---

# Player Controller

## Basic 3D Player with Character Controller

```lua
-- systems/player.lua
local MOVE_SPEED = 8.0
local JUMP_FORCE = 12.0
local GRAVITY = -30.0
local SPRINT_MULTIPLIER = 1.8

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

        -- Create physics character controller
        bestow.physics3d.createCharacter(state.player, {
            radius = 0.4,
            height = 1.8,
            stepHeight = 0.35,
            maxSlopeAngle = 45.0,
            mass = 80.0
        })

        -- Set up input actions
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

        bestow.input.pushPhase("gameplay")
    end,

    update = function(dt)
        local self = app.systems.player
        local state = app.main.state
        if not state.player then return end

        -- Get movement input
        local moveX, moveZ = 0, 0
        if bestow.input.isActionActive("MoveLeft") then moveX = moveX - 1 end
        if bestow.input.isActionActive("MoveRight") then moveX = moveX + 1 end
        if bestow.input.isActionActive("MoveForward") then moveZ = moveZ - 1 end
        if bestow.input.isActionActive("MoveBack") then moveZ = moveZ + 1 end

        -- Analog override
        local stickX = bestow.input.getActionValue("MoveAxisX") or 0
        local stickY = bestow.input.getActionValue("MoveAxisY") or 0
        if math.abs(stickX) > 0.01 then moveX = stickX end
        if math.abs(stickY) > 0.01 then moveZ = stickY end

        -- Normalize diagonal movement
        local moveDir = Vec3.new(moveX, 0, moveZ)
        if moveDir:lengthSquared() > 1.0 then
            moveDir = moveDir:normalize()
        end

        -- Sprint
        local speed = MOVE_SPEED
        if bestow.input.isActionActive("Sprint") then
            speed = speed * SPRINT_MULTIPLIER
        end

        -- Ground check
        local groundInfo = bestow.physics3d.getCharacterGroundInfo(state.player)
        local grounded = groundInfo and groundInfo.grounded

        -- Gravity and jumping
        if grounded then
            self.velocityY = -1  -- Small downward force to stay grounded
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
```

## Camera-Relative Movement

Move the player relative to the camera direction (third-person):

```lua
update = function(dt)
    local self = app.systems.player
    local state = app.main.state

    -- Get raw input
    local inputX, inputZ = 0, 0
    if bestow.input.isActionActive("MoveLeft") then inputX = inputX - 1 end
    if bestow.input.isActionActive("MoveRight") then inputX = inputX + 1 end
    if bestow.input.isActionActive("MoveForward") then inputZ = inputZ - 1 end
    if bestow.input.isActionActive("MoveBack") then inputZ = inputZ + 1 end

    if inputX == 0 and inputZ == 0 then return end

    -- Get camera forward/right (flattened to XZ plane)
    local cam = bestow.graphics3d.getCamera()
    local camForward = cam.transform.rotation:rotateVector(Vec3.forward())
    camForward = Vec3.new(camForward.x, 0, camForward.z):normalize()
    local camRight = cam.transform.rotation:rotateVector(Vec3.right())
    camRight = Vec3.new(camRight.x, 0, camRight.z):normalize()

    -- Build movement direction relative to camera
    local moveDir = camRight * inputX + camForward * inputZ
    if moveDir:lengthSquared() > 1.0 then
        moveDir = moveDir:normalize()
    end

    -- Face movement direction
    if moveDir:lengthSquared() > 0.01 then
        local newRot = Quat.fromAxisAngle(Vec3.up(),
            math.atan2(-moveDir.x, -moveDir.z))
        bestow.entity.setField(state.player, "Transform3D", "rotation", newRot)
    end

    -- Apply velocity
    local velocity = Vec3.new(moveDir.x * MOVE_SPEED, self.velocityY, moveDir.z * MOVE_SPEED)
    bestow.physics3d.moveCharacter(state.player, velocity, dt)
end
```

## Double Jump

```lua
local MAX_JUMPS = 2

return {
    velocityY = 0,
    jumpsRemaining = MAX_JUMPS,

    update = function(dt)
        local self = app.systems.player
        local state = app.main.state

        local groundInfo = bestow.physics3d.getCharacterGroundInfo(state.player)
        local grounded = groundInfo and groundInfo.grounded

        if grounded then
            self.jumpsRemaining = MAX_JUMPS
            self.velocityY = -1
        else
            self.velocityY = self.velocityY + GRAVITY * dt
        end

        if bestow.input.wasActionJustPressed("Jump") and self.jumpsRemaining > 0 then
            self.velocityY = JUMP_FORCE
            self.jumpsRemaining = self.jumpsRemaining - 1
        end

        -- ... apply movement
    end
}
```

## Coyote Time and Jump Buffering

```lua
local COYOTE_TIME = 0.12     -- Time after leaving ground you can still jump
local JUMP_BUFFER = 0.15     -- Time before landing a jump press is remembered

return {
    velocityY = 0,
    lastGroundedTime = 0,
    lastJumpPressTime = -999,
    hasJumped = false,

    update = function(dt)
        local self = app.systems.player
        local state = app.main.state
        local time = bestow.core.time()

        local groundInfo = bestow.physics3d.getCharacterGroundInfo(state.player)
        local grounded = groundInfo and groundInfo.grounded

        if grounded then
            self.lastGroundedTime = time
            self.hasJumped = false
            self.velocityY = -1
        else
            self.velocityY = self.velocityY + GRAVITY * dt
        end

        -- Buffer jump input
        if bestow.input.wasActionJustPressed("Jump") then
            self.lastJumpPressTime = time
        end

        -- Check for jump (coyote time + buffer)
        local canCoyoteJump = (time - self.lastGroundedTime) < COYOTE_TIME
        local hasBufferedJump = (time - self.lastJumpPressTime) < JUMP_BUFFER

        if hasBufferedJump and (grounded or canCoyoteJump) and not self.hasJumped then
            self.velocityY = JUMP_FORCE
            self.hasJumped = true
            self.lastJumpPressTime = -999  -- Consume the buffer
        end

        -- ... apply movement
    end
}
```

## 2D Platformer (Box2D)

```lua
return {
    init = function()
        local state = app.main.state
        state.player = bestow.entity.create()
        bestow.entity.addComponent(state.player, "Transform2D", {
            x = 100, y = 300, rotation = 0, scaleX = 1, scaleY = 1
        })
        bestow.physics.createBody(state.player, {
            type = "Dynamic",
            transform = { x = 100, y = 300 },
            size = Vec2.new(32, 48),
            fixedRotation = true,
            density = 1.0,
            friction = 0.3
        })
    end,

    update = function(dt)
        local self = app.systems.player
        local state = app.main.state
        local vel = bestow.physics.getVelocity(state.player)

        -- Horizontal movement
        local moveX = 0
        if bestow.input.isKeyDown(KeyCode.A) then moveX = -1 end
        if bestow.input.isKeyDown(KeyCode.E) or bestow.input.isKeyDown(KeyCode.D) then moveX = 1 end
        bestow.physics.setVelocity(state.player, Vec2.new(moveX * 300, vel.y))

        -- Jump
        local ground = bestow.physics.checkGrounded(state.player)
        if ground.grounded and bestow.input.wasKeyJustPressed(KeyCode.Space) then
            bestow.physics.applyImpulse(state.player, Vec2.new(0, -500))
        end
    end
}
```

## Best Practices

1. **Use character controllers** for player movement (not rigid bodies)
2. **Always support Dvorak + QWERTY** via Action Builder
3. **Normalize diagonal input** to prevent faster diagonal movement
4. **Apply a small downward force when grounded** to maintain contact
5. **Use coyote time** for forgiving jump windows
6. **Use camera-relative movement** for third-person games
7. **Store velocityY in the system** (survives hot reload via self-reference pattern)
