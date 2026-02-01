---
name: physics-system
description: Implement physics, collision detection, raycasting, and character controllers in Bestow. Use when adding physics bodies, detecting collisions, performing raycasts, or creating platformer mechanics.
---

# Physics System

Bestow provides both 2D (Box2D) and 3D (Jolt) physics systems.

## Creating Physics Bodies (3D)

### Static Bodies (Terrain, Walls)

```lua
local wall = bestow.entity.create()
bestow.entity.addComponent(wall, "Transform3D", {
    position = Vec3.new(0, 0, 0),
    rotation = Quat.identity(),
    scale = Vec3.new(10, 5, 1)
})

bestow.physics3d.createBody(wall, {
    type = "Static",
    shapeType = "Box",
    halfExtents = Vec3.new(5, 2.5, 0.5)  -- Half of the actual size
})
```

### Dynamic Bodies (Moving Objects)

```lua
local crate = bestow.entity.create()
bestow.entity.addComponent(crate, "Transform3D", {
    position = Vec3.new(0, 5, 0),
    rotation = Quat.identity(),
    scale = Vec3.new(1, 1, 1)
})

bestow.physics3d.createBody(crate, {
    type = "Dynamic",
    shapeType = "Box",
    halfExtents = Vec3.new(0.5, 0.5, 0.5),
    mass = 10.0,
    friction = 0.5,
    restitution = 0.3  -- Bounciness
})
```

### Shape Types

```lua
-- Box
{ shapeType = "Box", halfExtents = Vec3.new(1, 1, 1) }

-- Sphere
{ shapeType = "Sphere", radius = 0.5 }

-- Capsule (for characters)
{ shapeType = "Capsule", radius = 0.4, height = 1.8 }

-- Cylinder
{ shapeType = "Cylinder", radius = 0.5, height = 2.0 }
```

## Character Controller

For player movement with proper collision:

```lua
-- Create character controller (attached to entity)
local player = bestow.entity.create()
bestow.entity.addComponent(player, "Transform3D", {
    position = Vec3.new(0, 1, 0),
    rotation = Quat.identity(),
    scale = Vec3.one()
})

bestow.physics3d.createCharacter(player, {
    radius = 0.4,
    height = 1.8,
    stepHeight = 0.35,      -- Can step up this high
    maxSlopeAngle = 45.0,   -- Can walk on slopes up to this
    mass = 80.0
})
```

### Moving the Character

```lua
-- In movement system update (use Action Builder, not direct polling)
local velocity = Vec3.new(0, 0, 0)
local moveX, moveZ = 0, 0
if bestow.input.isActionActive("MoveLeft") then moveX = moveX - 1 end
if bestow.input.isActionActive("MoveRight") then moveX = moveX + 1 end
if bestow.input.isActionActive("MoveForward") then moveZ = moveZ - 1 end
if bestow.input.isActionActive("MoveBack") then moveZ = moveZ + 1 end

local moveDir = Vec3.new(moveX, 0, moveZ)
if moveDir:lengthSquared() > 1.0 then
    moveDir = moveDir:normalize()
end

-- Apply gravity if not grounded
local groundInfo = bestow.physics3d.getCharacterGroundInfo(player)
if not (groundInfo and groundInfo.grounded) then
    self.velocityY = self.velocityY + GRAVITY * dt
else
    self.velocityY = -1  -- Small downward force to stay grounded
end

-- Move the character (handles collision)
local velocity = Vec3.new(moveDir.x * speed, self.velocityY, moveDir.z * speed)
bestow.physics3d.moveCharacter(player, velocity, dt)
```

### Ground Detection

```lua
-- Pass the entity directly (not a separate handle)
local groundInfo = bestow.physics3d.getCharacterGroundInfo(player)

if groundInfo and groundInfo.grounded then
    -- Character is on ground
    print("Surface normal: " .. tostring(groundInfo.normal))
    print("Slope angle: " .. groundInfo.slopeAngle)
end
```

## Raycasting

### Basic Raycast

```lua
-- Cast a ray from origin in direction
local hit = bestow.physics3d.raycast(
    Vec3.new(0, 5, 0),      -- Origin
    Vec3.new(0, -1, 0),     -- Direction (normalized)
    100                      -- Max distance
)

if hit then
    print("Hit entity: " .. hit.entity)
    print("Hit point: " .. hit.point)
    print("Surface normal: " .. hit.normal)
    print("Distance: " .. hit.distance)
end
```

### Raycast from Camera (Mouse Picking)

```lua
local function getMouseWorldPosition()
    local mouseScreen = bestow.input.getMousePosition()
    local ray = bestow.graphics3d.screenToRay(mouseScreen)

    local hit = bestow.physics3d.raycast(ray.origin, ray.direction, 1000)
    if hit then
        return hit.point, hit.entity
    end
    return nil, nil
end

-- Usage
local clickPos, clickedEntity = getMouseWorldPosition()
if clickedEntity then
    -- Player clicked on something
end
```

### Raycast All (Multiple Hits)

```lua
local hits = bestow.physics3d.raycastAll(origin, direction, distance)

for _, hit in ipairs(hits) do
    print("Hit: " .. hit.entity .. " at distance " .. hit.distance)
end
```

## Collision Queries

### Overlap Sphere

```lua
-- Find all entities within radius
local entities = bestow.physics3d.overlapSphere(
    Vec3.new(0, 0, 0),  -- Center
    5.0                  -- Radius
)

for _, entity in ipairs(entities) do
    -- Entity is within 5 units
end
```

### Overlap Box

```lua
local entities = bestow.physics3d.overlapBox(
    Vec3.new(0, 0, 0),        -- Center
    Vec3.new(2, 1, 2),        -- Half extents
    Quat.identity()           -- Rotation
)
```

## Collision Layers

Filter what collides with what:

```lua
-- Predefined layers
Layers.Default    -- 0
Layers.Static     -- 1
Layers.Dynamic    -- 2
Layers.Character  -- 3
Layers.Projectile -- 4
Layers.Trigger    -- 5

-- Set layer and mask on body
bestow.physics3d.setCollisionLayer(entity, Layers.Character)
bestow.physics3d.setCollisionMask(entity, Layers.Static | Layers.Dynamic)
```

## Applying Forces

```lua
-- Apply force (continuous, like thrust)
bestow.physics3d.applyForce(entity, Vec3.new(0, 100, 0))

-- Apply force at point (creates torque)
bestow.physics3d.applyForceAtPoint(
    entity,
    Vec3.new(100, 0, 0),    -- Force
    Vec3.new(0, 1, 0)       -- Point (world space)
)

-- Apply impulse (instant, like explosion)
bestow.physics3d.applyImpulse(entity, Vec3.new(0, 500, 0))

-- Apply torque (rotation)
bestow.physics3d.applyTorque(entity, Vec3.new(0, 10, 0))
```

## Getting/Setting Velocity

```lua
-- Get current velocity
local vel = bestow.physics3d.getVelocity(entity)
local angVel = bestow.physics3d.getAngularVelocity(entity)

-- Set velocity directly
bestow.physics3d.setVelocity(entity, Vec3.new(0, 10, 0))
bestow.physics3d.setAngularVelocity(entity, Vec3.new(0, 0, 0))
```

## Collision Callbacks

Listen for collision events using the table+method pattern (hot-reload safe):

```lua
-- systems/collision_handler.lua
return {
    init = function()
        local self = app.systems.collision_handler

        -- Subscribe with table+method pattern (NOT closures)
        self.collisionSubId = bestow.events.subscribe("collision_3d", {},
            app.systems.collision_handler, "onCollision")

        self.triggerSubId = bestow.events.subscribe("trigger_enter_3d", {},
            app.systems.collision_handler, "onTriggerEnter")
    end,

    onCollision = function(event)
        local state = app.main.state
        if event.entityA == state.player or event.entityB == state.player then
            local other = event.entityA == state.player and event.entityB or event.entityA
            if bestow.entity.hasComponent(other, "DamageDealer") then
                app.systems.combat.damagePlayer(event.impulse)
            end
        end
    end,

    onTriggerEnter = function(event)
        local state = app.main.state
        if event.entityA == state.player then
            if bestow.entity.hasComponent(event.entityB, "Checkpoint") then
                app.systems.checkpoints.activate(event.entityB)
            end
        end
    end,

    shutdown = function()
        local self = app.systems.collision_handler
        if self.collisionSubId then bestow.events.unsubscribe(self.collisionSubId) end
        if self.triggerSubId then bestow.events.unsubscribe(self.triggerSubId) end
    end
}
```

## 2D Physics (Box2D)

For 2D games, use the 2D physics system:

```lua
-- Create 2D body
bestow.physics.createBody(entity, {
    type = "Dynamic",  -- "Static", "Kinematic", "Dynamic"
    transform = { x = 100, y = 200 },
    size = Vec2.new(32, 48),
    density = 1.0,
    friction = 0.3,
    restitution = 0.0,
    fixedRotation = true  -- Prevent rotation (for characters)
})

-- 2D Ground check
local result = bestow.physics.checkGrounded(entity, {
    rayDistance = 5.0,
    slopeToleranceDeg = 45.0,
    groundMask = Layers.Terrain
})

if result.grounded then
    -- Can jump
end
```

## Platformer Jump Pattern

```lua
-- systems/player.lua
return {
    jumpForce = 12.0,
    gravity = -30.0,
    velocityY = 0,

    update = function(dt)
        local self = app.systems.player
        local state = app.main.state
        if not state.player then return end

        -- Check ground (pass entity directly)
        local groundInfo = bestow.physics3d.getCharacterGroundInfo(state.player)
        local grounded = groundInfo and groundInfo.grounded

        -- Apply gravity
        if grounded then
            self.velocityY = -1  -- Small downward force to stay grounded
        else
            self.velocityY = self.velocityY + self.gravity * dt
        end

        -- Jump (use Action Builder, not direct polling)
        if grounded and bestow.input.wasActionJustPressed("Jump") then
            self.velocityY = self.jumpForce
        end

        -- Build velocity (get movement from Action Builder)
        local moveX, moveZ = 0, 0
        if bestow.input.isActionActive("MoveLeft") then moveX = moveX - 1 end
        if bestow.input.isActionActive("MoveRight") then moveX = moveX + 1 end
        if bestow.input.isActionActive("MoveForward") then moveZ = moveZ - 1 end
        if bestow.input.isActionActive("MoveBack") then moveZ = moveZ + 1 end

        local moveDir = Vec3.new(moveX, 0, moveZ)
        if moveDir:lengthSquared() > 1.0 then moveDir = moveDir:normalize() end

        local velocity = Vec3.new(moveDir.x * 8.0, self.velocityY, moveDir.z * 8.0)
        bestow.physics3d.moveCharacter(state.player, velocity, dt)
    end
}
```

## Best Practices

1. **Use character controllers for players** - Not dynamic bodies
2. **Make terrain static** - Much more efficient
3. **Use collision layers** - Filter unnecessary collision checks
4. **Raycast for ground detection** - More reliable than collision
5. **Apply impulses for instant forces** - Jumps, explosions
6. **Apply forces for continuous thrust** - Engines, wind
7. **Use triggers for pickups** - No physical response needed
