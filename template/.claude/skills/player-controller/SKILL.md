---
name: player-controller
description: Implement player movement, jumping, and abilities in Bestow. Use when creating player characters, implementing movement controls, or adding player abilities.
---

# Player Controller

Implement responsive player movement with jumping, abilities, and smooth controls.

## Basic 3D Movement

### Simple Movement System

```lua
-- systems/movement.lua
return {
    speed = 8.0,

    update = function(dt)
        local self = app.systems.movement
        local state = app.main.state

        if not state.player then return end

        -- Get input (Dvorak and QWERTY support)
        local movement = Vec3.new(0, 0, 0)

        if bestow.input.isKeyDown(Keys.Comma) or bestow.input.isKeyDown(Keys.W) then
            movement.z = -1
        end
        if bestow.input.isKeyDown(Keys.O) or bestow.input.isKeyDown(Keys.S) then
            movement.z = 1
        end
        if bestow.input.isKeyDown(Keys.A) then
            movement.x = -1
        end
        if bestow.input.isKeyDown(Keys.E) or bestow.input.isKeyDown(Keys.D) then
            movement.x = 1
        end

        -- Arrow keys as fallback
        if bestow.input.isKeyDown(Keys.Up) then movement.z = -1 end
        if bestow.input.isKeyDown(Keys.Down) then movement.z = 1 end
        if bestow.input.isKeyDown(Keys.Left) then movement.x = -1 end
        if bestow.input.isKeyDown(Keys.Right) then movement.x = 1 end

        -- Normalize diagonal movement
        if movement:length() > 0 then
            movement = movement:normalize() * self.speed * dt

            local pos = bestow.entity.getField(state.player, "Transform3D", "position")
            bestow.entity.setField(state.player, "Transform3D", "position", pos + movement)
        end
    end
}
```

## Character Controller with Physics

For proper collision handling:

```lua
-- entities/player.lua
return {
    create = function(position)
        local self = app.entities.player
        local entity = bestow.entity.create()

        bestow.entity.addComponent(entity, "Transform3D", {
            position = position or Vec3.new(0, 1, 0),
            rotation = Quat.identity(),
            scale = Vec3.new(1, 1, 1)
        })

        bestow.entity.addComponent(entity, "MeshRenderer", {
            mesh = "meshes/player.obj",
            material = "materials/player"
        })

        -- Character controller for physics-based movement
        local charHandle = bestow.physics3d.createCharacter(entity, {
            radius = 0.4,
            height = 1.8,
            stepHeight = 0.35,
            maxSlopeAngle = 45.0,
            mass = 80.0
        })

        bestow.entity.addComponent(entity, "CharacterController", {
            handle = charHandle
        })

        bestow.entity.addComponent(entity, "PlayerState", {
            velocityY = 0,
            grounded = false,
            canJump = true,
            coyoteTime = 0,
            jumpBuffer = 0
        })

        bestow.entity.addComponent(entity, "Health", {
            current = 100,
            max = 100
        })

        return entity
    end
}

-- systems/player.lua
return {
    -- Configuration
    moveSpeed = 8.0,
    jumpForce = 12.0,
    gravity = -30.0,
    coyoteTimeDuration = 0.15,  -- Time after leaving ground you can still jump
    jumpBufferDuration = 0.1,   -- Time before landing a jump press is remembered

    update = function(dt)
        local self = app.systems.player
        local state = app.main.state
        local player = state.player

        if not player then return end

        local controller = bestow.entity.getComponent(player, "CharacterController")
        local playerState = bestow.entity.getComponent(player, "PlayerState")

        -- Ground check
        local groundInfo = bestow.physics3d.getCharacterGroundInfo(controller.handle)
        local wasGrounded = playerState.grounded
        playerState.grounded = groundInfo.grounded

        -- Coyote time (allows jumping briefly after leaving ground)
        if playerState.grounded then
            playerState.coyoteTime = self.coyoteTimeDuration
        else
            playerState.coyoteTime = playerState.coyoteTime - dt
        end

        -- Jump buffer (remembers jump press briefly)
        if bestow.input.wasKeyJustPressed(Keys.Space) then
            playerState.jumpBuffer = self.jumpBufferDuration
        else
            playerState.jumpBuffer = playerState.jumpBuffer - dt
        end

        -- Apply gravity
        if not playerState.grounded then
            playerState.velocityY = playerState.velocityY + self.gravity * dt
        else
            if playerState.velocityY < 0 then
                playerState.velocityY = 0
            end
        end

        -- Jump (with coyote time and jump buffering)
        local canJump = playerState.coyoteTime > 0
        local wantsJump = playerState.jumpBuffer > 0

        if canJump and wantsJump then
            playerState.velocityY = self.jumpForce
            playerState.coyoteTime = 0
            playerState.jumpBuffer = 0
            app.systems.audio.playSfx("jump")
        end

        -- Variable jump height (release to fall faster)
        if playerState.velocityY > 0 and not bestow.input.isKeyDown(Keys.Space) then
            playerState.velocityY = playerState.velocityY * 0.5
        end

        -- Horizontal movement
        local moveInput = Vec3.new(0, 0, 0)

        if bestow.input.isKeyDown(Keys.Comma) or bestow.input.isKeyDown(Keys.W) then
            moveInput.z = -1
        end
        if bestow.input.isKeyDown(Keys.O) or bestow.input.isKeyDown(Keys.S) then
            moveInput.z = 1
        end
        if bestow.input.isKeyDown(Keys.A) then
            moveInput.x = -1
        end
        if bestow.input.isKeyDown(Keys.E) or bestow.input.isKeyDown(Keys.D) then
            moveInput.x = 1
        end

        -- Build velocity
        local velocity = Vec3.new(0, playerState.velocityY, 0)

        if moveInput:length() > 0 then
            moveInput = moveInput:normalize()
            velocity.x = moveInput.x * self.moveSpeed
            velocity.z = moveInput.z * self.moveSpeed

            -- Face movement direction
            local angle = math.atan2(moveInput.x, -moveInput.z)
            local rotation = Quat.fromAxisAngle(Vec3.new(0, 1, 0), angle)
            bestow.entity.setField(player, "Transform3D", "rotation", rotation)
        end

        -- Move character
        bestow.physics3d.moveCharacter(controller.handle, velocity, dt)

        -- Save state
        bestow.entity.setComponent(player, "PlayerState", playerState)

        -- Landing sound
        if playerState.grounded and not wasGrounded and playerState.velocityY < -5 then
            app.systems.audio.playSfx("land")
        end
    end
}
```

## Sprint and Crouch

```lua
-- Add to player system
sprint = {
    multiplier = 1.5,
    active = false
},

crouch = {
    multiplier = 0.5,
    active = false,
    originalHeight = 1.8,
    crouchHeight = 1.0
},

updateSprint = function(dt)
    local self = app.systems.player

    -- Sprint with Shift
    self.sprint.active = bestow.input.isKeyDown(Keys.LeftShift)

    -- Crouch with Ctrl or C
    local wantsCrouch = bestow.input.isKeyDown(Keys.LeftCtrl) or
                        bestow.input.isKeyDown(Keys.C)

    if wantsCrouch and not self.crouch.active then
        self.crouch.active = true
        -- Adjust character height
        -- bestow.physics3d.setCharacterHeight(controller.handle, self.crouch.crouchHeight)
    elseif not wantsCrouch and self.crouch.active then
        -- Check if we can stand up (raycast above)
        self.crouch.active = false
    end
end,

getSpeedMultiplier = function()
    local self = app.systems.player
    local mult = 1.0

    if self.sprint.active then
        mult = mult * self.sprint.multiplier
    end
    if self.crouch.active then
        mult = mult * self.crouch.multiplier
    end

    return mult
end
```

## Dash Ability

```lua
-- Add to player system
dash = {
    speed = 30.0,
    duration = 0.2,
    cooldown = 1.0,
    timer = 0,
    cooldownTimer = 0,
    direction = Vec3.new(0, 0, 0)
},

updateDash = function(dt)
    local self = app.systems.player
    local state = app.main.state
    local player = state.player

    -- Cooldown
    self.dash.cooldownTimer = math.max(0, self.dash.cooldownTimer - dt)

    -- Active dash
    if self.dash.timer > 0 then
        self.dash.timer = self.dash.timer - dt

        -- Move in dash direction
        local controller = bestow.entity.getComponent(player, "CharacterController")
        local velocity = self.dash.direction * self.dash.speed
        bestow.physics3d.moveCharacter(controller.handle, velocity, dt)

        return true  -- Skip normal movement
    end

    -- Start dash (Shift + direction, or dedicated button)
    if self.dash.cooldownTimer <= 0 then
        if bestow.input.wasKeyJustPressed(Keys.LeftShift) then
            -- Get current movement direction
            local moveDir = self.getCurrentMoveDirection()
            if moveDir:length() > 0 then
                self.dash.direction = moveDir:normalize()
                self.dash.timer = self.dash.duration
                self.dash.cooldownTimer = self.dash.cooldown

                app.systems.audio.playSfx("dash")
                -- Could add visual effect
            end
        end
    end

    return false  -- Normal movement continues
end
```

## Wall Jump

```lua
-- Add to player system
wallJump = {
    checkDistance = 0.5,
    jumpForce = Vec3.new(8, 10, 0),  -- Away and up
    cooldown = 0.2,
    timer = 0
},

checkWallJump = function()
    local self = app.systems.player
    local state = app.main.state
    local player = state.player

    local playerState = bestow.entity.getComponent(player, "PlayerState")

    -- Only check when in air
    if playerState.grounded then return nil end

    self.wallJump.timer = math.max(0, self.wallJump.timer - dt)
    if self.wallJump.timer > 0 then return nil end

    local pos = bestow.entity.getField(player, "Transform3D", "position")

    -- Check left and right for walls
    local directions = {
        { dir = Vec3.new(-1, 0, 0), name = "left" },
        { dir = Vec3.new(1, 0, 0), name = "right" },
        { dir = Vec3.new(0, 0, -1), name = "forward" },
        { dir = Vec3.new(0, 0, 1), name = "back" }
    }

    for _, check in ipairs(directions) do
        local hit = bestow.physics3d.raycast(
            pos + Vec3.new(0, 0.5, 0),  -- From middle of player
            check.dir,
            self.wallJump.checkDistance
        )

        if hit and hit.normal then
            -- Wall found, return jump direction (away from wall)
            return {
                wallNormal = hit.normal,
                jumpDir = hit.normal * self.wallJump.jumpForce.x +
                          Vec3.new(0, self.wallJump.jumpForce.y, 0)
            }
        end
    end

    return nil
end,

performWallJump = function(wallInfo)
    local self = app.systems.player
    local state = app.main.state
    local player = state.player

    local playerState = bestow.entity.getComponent(player, "PlayerState")

    playerState.velocityY = wallInfo.jumpDir.y
    self.wallJump.timer = self.wallJump.cooldown

    -- Apply horizontal force
    local controller = bestow.entity.getComponent(player, "CharacterController")
    local velocity = wallInfo.jumpDir
    bestow.physics3d.moveCharacter(controller.handle, velocity * 0.1, 1)

    app.systems.audio.playSfx("walljump")
    bestow.entity.setComponent(player, "PlayerState", playerState)
end
```

## Double Jump

```lua
-- In PlayerState component
bestow.entity.addComponent(entity, "PlayerState", {
    velocityY = 0,
    grounded = false,
    jumpsRemaining = 2,
    maxJumps = 2
})

-- In update
if playerState.grounded then
    playerState.jumpsRemaining = playerState.maxJumps
end

if bestow.input.wasKeyJustPressed(Keys.Space) and playerState.jumpsRemaining > 0 then
    playerState.velocityY = self.jumpForce

    -- Reduce force for subsequent jumps
    if playerState.jumpsRemaining < playerState.maxJumps then
        playerState.velocityY = playerState.velocityY * 0.8
    end

    playerState.jumpsRemaining = playerState.jumpsRemaining - 1
    app.systems.audio.playSfx("jump")
end
```

## Attack System

```lua
-- Add attack state to player
bestow.entity.addComponent(entity, "CombatState", {
    attacking = false,
    attackTimer = 0,
    comboCount = 0,
    comboTimer = 0
})

-- systems/player_combat.lua
return {
    attackDuration = 0.3,
    comboDuration = 0.5,  -- Time to chain next attack
    attackDamage = { 10, 15, 25 },  -- Damage per combo hit

    update = function(dt)
        local self = app.systems.player_combat
        local state = app.main.state
        local player = state.player

        if not player then return end

        local combat = bestow.entity.getComponent(player, "CombatState")

        -- Update timers
        combat.attackTimer = math.max(0, combat.attackTimer - dt)
        combat.comboTimer = math.max(0, combat.comboTimer - dt)

        if combat.comboTimer <= 0 then
            combat.comboCount = 0
        end

        -- Attack input
        if bestow.input.wasMouseButtonJustPressed(0) then  -- Left click
            if combat.attackTimer <= 0 then
                self.performAttack(player, combat)
            end
        end

        bestow.entity.setComponent(player, "CombatState", combat)
    end,

    performAttack = function(player, combat)
        local self = app.systems.player_combat

        combat.attacking = true
        combat.attackTimer = self.attackDuration
        combat.comboCount = (combat.comboCount % 3) + 1
        combat.comboTimer = self.comboDuration

        -- Play attack animation
        local animator = bestow.entity.getComponent(player, "Animator")
        if animator then
            bestow.animation.play(animator.handle, "attack" .. combat.comboCount)
        end

        -- Damage enemies in range
        local pos = bestow.entity.getField(player, "Transform3D", "position")
        local rot = bestow.entity.getField(player, "Transform3D", "rotation")
        local forward = rot * Vec3.new(0, 0, -1)

        local attackPoint = pos + forward * 1.5 + Vec3.new(0, 1, 0)
        local enemies = bestow.physics3d.overlapSphere(attackPoint, 1.5)

        local damage = self.attackDamage[combat.comboCount]

        for _, enemy in ipairs(enemies) do
            if bestow.entity.hasComponent(enemy, "Enemy") then
                app.systems.combat.damage(enemy, damage, player)
            end
        end

        app.systems.audio.playSfx("attack" .. combat.comboCount)
    end
}
```

## Best Practices

1. **Support Dvorak AND QWERTY** - Always check both key layouts
2. **Use character controllers** - Not raw physics bodies for players
3. **Implement coyote time** - Makes platforming feel fair
4. **Add jump buffering** - Remembers jump presses
5. **Variable jump height** - Release to fall faster
6. **Smooth rotation** - Lerp facing direction
7. **Play sounds on actions** - Jump, land, attack feedback
8. **Store state in components** - For save/load compatibility
