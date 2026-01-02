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
-- Create character controller
local player = bestow.entity.create()
bestow.entity.addComponent(player, "Transform3D", {
    position = Vec3.new(0, 1, 0),
    rotation = Quat.identity(),
    scale = Vec3.new(1, 1, 1)
})

local charHandle = bestow.physics3d.createCharacter(player, {
    radius = 0.4,
    height = 1.8,
    stepHeight = 0.35,      -- Can step up this high
    maxSlopeAngle = 45.0,   -- Can walk on slopes up to this
    mass = 80.0
})

-- Store handle for later use
bestow.entity.addComponent(player, "CharacterController", {
    handle = charHandle
})
```

### Moving the Character

```lua
-- In movement system update
local controller = bestow.entity.getComponent(player, "CharacterController")
local velocity = Vec3.new(0, 0, 0)

-- Get input
if bestow.input.isKeyDown(Keys.Comma) or bestow.input.isKeyDown(Keys.W) then
    velocity.z = -1
end
-- ... more input handling

-- Normalize and scale
if velocity:length() > 0 then
    velocity = velocity:normalize() * speed
end

-- Apply gravity if not grounded
local groundInfo = bestow.physics3d.getCharacterGroundInfo(controller.handle)
if not groundInfo.grounded then
    velocity.y = velocity.y - 9.81 * dt
end

-- Move the character (handles collision)
bestow.physics3d.moveCharacter(controller.handle, velocity, dt)
```

### Ground Detection

```lua
local groundInfo = bestow.physics3d.getCharacterGroundInfo(charHandle)

if groundInfo.grounded then
    -- Character is on ground
    print("Standing on surface at: " .. groundInfo.contactPoint)
    print("Surface normal: " .. groundInfo.normal)
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
    local ray = bestow.camera3d.screenToRay(mouseScreen)

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

Listen for collision events:

```lua
-- Subscribe to collision events
bestow.events.subscribe("collision_3d", function(event)
    local entityA = event.entityA
    local entityB = event.entityB
    local point = event.contactPoint
    local normal = event.contactNormal
    local impulse = event.impulse

    -- Check if player involved
    if entityA == app.main.state.player or entityB == app.main.state.player then
        local other = entityA == app.main.state.player and entityB or entityA

        if bestow.entity.hasComponent(other, "DamageDealer") then
            app.systems.combat.damagePlayer(impulse)
        end
    end
end)

-- Trigger events (for sensors)
bestow.events.subscribe("trigger_enter_3d", function(event)
    if event.entityA == app.main.state.player then
        if bestow.entity.hasComponent(event.entityB, "Checkpoint") then
            app.systems.save.setCheckpoint(event.entityB)
        end
    end
end)
```

## 2D Physics (Box2D)

For 2D games, use the 2D physics system:

```lua
-- Create 2D body
bestow.physics.createBody(entity, {
    type = "Dynamic",  -- "Static", "Kinematic", "Dynamic"
    x = 100, y = 200,
    width = 32, height = 48,
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
    grounded = false,

    update = function(dt)
        local self = app.systems.player
        local state = app.main.state

        -- Check ground
        local charHandle = bestow.entity.getComponent(state.player, "CharacterController").handle
        local groundInfo = bestow.physics3d.getCharacterGroundInfo(charHandle)
        self.grounded = groundInfo.grounded

        -- Apply gravity
        if not self.grounded then
            self.velocityY = self.velocityY + self.gravity * dt
        else
            self.velocityY = 0
        end

        -- Jump
        if self.grounded and bestow.input.wasKeyJustPressed(Keys.Space) then
            self.velocityY = self.jumpForce
            self.grounded = false
            app.systems.audio.playSfx("jump")
        end

        -- Build velocity
        local velocity = Vec3.new(0, self.velocityY, 0)
        -- ... add horizontal movement

        -- Move character
        bestow.physics3d.moveCharacter(charHandle, velocity, dt)
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
