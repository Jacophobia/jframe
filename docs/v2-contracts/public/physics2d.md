# Physics 2D System

> **Visibility:** Public (Lua + C++ API)
> **Tier:** 3
> **Dependencies:** Types, Entity, Events
> **Lua Paths:** `bestow.physics` (high-level), `bestow.physics.core` (low-level)

## Purpose

The Physics 2D System provides rigid body dynamics, collision detection, and spatial queries for 2D games. Built on Box2D, it integrates with the Entity System so that physics bodies are associated with entities. The high-level API offers entity-centric body creation, transform/velocity access, force application, raycasting, and ground detection. The low-level API exposes BodyHandle-based control, per-body property tuning, shape management (box, circle, polygon, edge, chain), material properties, collision filtering, constraints (distance, revolute, prismatic, weld), full spatial queries, world configuration, and debug drawing. Physics runs in the FixedUpdate phase at a deterministic timestep.

## High-Level API: `IPhysicsSystem`

The simplified API for common game development tasks. Entity-centric, sensible defaults, no lifecycle management.

### Body Creation

| Method | Returns | Description |
|--------|---------|-------------|
| `createBody(Entity entity, const BodyDef2D& def)` | `Result<void>` | Create a physics body for an entity using the provided definition |
| `destroyBody(Entity entity)` | `Result<void>` | Destroy the physics body associated with an entity |
| `hasBody(Entity entity)` | `bool` | Check whether an entity has an associated physics body |

### Transform

| Method | Returns | Description |
|--------|---------|-------------|
| `getPosition(Entity entity)` | `Vec2` | Return the world-space position of the entity's physics body |
| `setPosition(Entity entity, Vec2 pos)` | `Result<void>` | Teleport the entity's physics body to a new position |
| `getVelocity(Entity entity)` | `Vec2` | Return the current linear velocity of the entity's physics body |
| `setVelocity(Entity entity, Vec2 vel)` | `Result<void>` | Set the linear velocity of the entity's physics body directly |

### Forces

| Method | Returns | Description |
|--------|---------|-------------|
| `applyForce(Entity entity, Vec2 force)` | `Result<void>` | Apply a continuous force to the entity's body center of mass |
| `applyImpulse(Entity entity, Vec2 impulse)` | `Result<void>` | Apply an instantaneous impulse to the entity's body center of mass |

### Queries

| Method | Returns | Description |
|--------|---------|-------------|
| `raycast(Vec2 origin, Vec2 direction, float maxDist)` | `std::optional<RaycastHit2D>` | Cast a ray and return the closest hit, or nullopt if nothing was hit |
| `checkGrounded(Entity entity)` | `GroundCheckResult` | Perform a ground detection check beneath the entity using default parameters |

### World

| Method | Returns | Description |
|--------|---------|-------------|
| `setGravity(Vec2 gravity)` | `void` | Set the global gravity vector for the 2D physics world |

### Callbacks

| Method | Returns | Description |
|--------|---------|-------------|
| `onCollision(std::function<void(const CollisionEvent&)> cb)` | `SubscriptionId` | Subscribe to collision events between physics bodies |
| `onTrigger(std::function<void(const TriggerEvent&)> cb)` | `SubscriptionId` | Subscribe to trigger (sensor) enter/exit events |
| `unsubscribe(SubscriptionId id)` | `void` | Remove a collision or trigger subscription |

## Low-Level API: `IPhysics2DCore`

Full control API. BodyHandle-based, full body property access, shape management, materials, forces, collision filtering, constraints, spatial queries, world config, and debug draw.

### Lifecycle

| Method | Returns | Description |
|--------|---------|-------------|
| `initialize()` | `Result<void>` | Initialize the Box2D world and allocate physics resources |
| `shutdown()` | `void` | Destroy the physics world and release all resources |
| `update(DeltaTime fixedDt)` | `void` | Step the physics simulation by the fixed timestep; called in FixedUpdate phase |

### Body Management

| Method | Returns | Description |
|--------|---------|-------------|
| `createBody(Entity entity, const BodyDef2D& def)` | `Result<BodyHandle>` | Create a physics body for an entity and return its handle |
| `destroyBody(BodyHandle handle)` | `Result<void>` | Destroy a physics body by handle |
| `bodyExists(BodyHandle handle)` | `bool` | Check whether a body handle is still valid |
| `getBody(Entity entity)` | `std::optional<BodyHandle>` | Look up the body handle associated with an entity |

### Body Properties

| Method | Returns | Description |
|--------|---------|-------------|
| `setBodyType(BodyHandle h, BodyType type)` | `Result<void>` | Change a body's type (Static, Kinematic, or Dynamic) |
| `getBodyType(BodyHandle h)` | `BodyType` | Return the current type of a body |
| `setPosition(BodyHandle h, Vec2 pos)` | `Result<void>` | Teleport a body to a new position |
| `getPosition(BodyHandle h)` | `Vec2` | Return a body's current world-space position |
| `setRotation(BodyHandle h, float radians)` | `Result<void>` | Set a body's rotation angle in radians |
| `getRotation(BodyHandle h)` | `float` | Return a body's current rotation in radians |
| `setVelocity(BodyHandle h, Vec2 vel)` | `Result<void>` | Set a body's linear velocity directly |
| `getVelocity(BodyHandle h)` | `Vec2` | Return a body's current linear velocity |
| `setAngularVelocity(BodyHandle h, float omega)` | `Result<void>` | Set a body's angular velocity in radians per second |
| `getAngularVelocity(BodyHandle h)` | `float` | Return a body's current angular velocity |
| `setGravityScale(BodyHandle h, float scale)` | `Result<void>` | Set the gravity multiplier for a body (0 = no gravity, 1 = normal) |
| `getMass(BodyHandle h)` | `float` | Return the total mass of a body in kilograms |
| `getBodySize(BodyHandle h)` | `Vec2` | Return the AABB size of the body's collision shapes |

### Shape Management

| Method | Returns | Description |
|--------|---------|-------------|
| `addBoxShape(BodyHandle h, Vec2 halfExtents, Vec2 offset = {}, float density = 1.0f)` | `Result<void>` | Add a box collision shape to a body with half-width/height and optional offset |
| `addCircleShape(BodyHandle h, float radius, Vec2 offset = {}, float density = 1.0f)` | `Result<void>` | Add a circle collision shape with radius and optional offset |
| `addPolygonShape(BodyHandle h, std::span<const Vec2> vertices, float density = 1.0f)` | `Result<void>` | Add a convex polygon collision shape defined by vertex positions |
| `addEdgeShape(BodyHandle h, Vec2 start, Vec2 end)` | `Result<void>` | Add a line segment collision shape between two points |
| `addChainShape(BodyHandle h, std::span<const Vec2> vertices, bool loop = false)` | `Result<void>` | Add a chain of connected edges; set loop=true to close the shape |

### Material Properties

| Method | Returns | Description |
|--------|---------|-------------|
| `setFriction(BodyHandle h, float friction)` | `Result<void>` | Set the friction coefficient for all shapes on a body (0.0 to 1.0) |
| `setRestitution(BodyHandle h, float restitution)` | `Result<void>` | Set the bounciness for all shapes on a body (0.0 = no bounce, 1.0 = perfect bounce) |
| `setDensity(BodyHandle h, float density)` | `Result<void>` | Set the density for all shapes on a body; recalculates mass |

### Forces

| Method | Returns | Description |
|--------|---------|-------------|
| `applyForce(BodyHandle h, Vec2 force, Vec2 point)` | `Result<void>` | Apply a continuous force at a world-space point on the body |
| `applyForceToCenter(BodyHandle h, Vec2 force)` | `Result<void>` | Apply a continuous force to the body's center of mass |
| `applyImpulse(BodyHandle h, Vec2 impulse, Vec2 point)` | `Result<void>` | Apply an instantaneous impulse at a world-space point |
| `applyImpulseToCenter(BodyHandle h, Vec2 impulse)` | `Result<void>` | Apply an instantaneous impulse to the center of mass |
| `applyTorque(BodyHandle h, float torque)` | `Result<void>` | Apply a continuous rotational torque to the body |

### Collision Filtering

| Method | Returns | Description |
|--------|---------|-------------|
| `setCollisionLayer(BodyHandle h, std::uint16_t layer)` | `Result<void>` | Set which collision layer this body belongs to (bitmask) |
| `setCollisionMask(BodyHandle h, std::uint16_t mask)` | `Result<void>` | Set which collision layers this body can collide with (bitmask) |
| `setSensor(BodyHandle h, bool isSensor)` | `Result<void>` | Make the body a sensor (detects overlaps but has no collision response) |
| `isSensor(BodyHandle h)` | `bool` | Check whether a body is configured as a sensor |

### Constraints

| Method | Returns | Description |
|--------|---------|-------------|
| `createDistanceJoint(BodyHandle a, BodyHandle b, Vec2 anchorA, Vec2 anchorB)` | `Result<ConstraintHandle>` | Create a distance joint maintaining a fixed distance between two anchor points |
| `createRevoluteJoint(BodyHandle a, BodyHandle b, Vec2 anchor, bool enableLimits = false, float lower = 0, float upper = 0)` | `Result<ConstraintHandle>` | Create a revolute (hinge) joint allowing rotation around an anchor point |
| `createPrismaticJoint(BodyHandle a, BodyHandle b, Vec2 anchor, Vec2 axis)` | `Result<ConstraintHandle>` | Create a prismatic (slider) joint allowing translation along an axis |
| `createWeldJoint(BodyHandle a, BodyHandle b, Vec2 anchor)` | `Result<ConstraintHandle>` | Create a weld joint rigidly connecting two bodies at an anchor point |
| `destroyConstraint(ConstraintHandle handle)` | `Result<void>` | Destroy a constraint and free both bodies |

### Queries

| Method | Returns | Description |
|--------|---------|-------------|
| `queryAABB(Vec2 min, Vec2 max)` | `std::vector<BodyHandle>` | Return all bodies whose AABBs overlap the given rectangle |
| `queryCircle(Vec2 center, float radius)` | `std::vector<BodyHandle>` | Return all bodies overlapping a circle region |
| `raycast(Vec2 origin, Vec2 direction, float maxDist, std::uint16_t mask = 0xFFFF)` | `std::optional<RaycastHit2D>` | Cast a ray and return the closest hit filtered by collision mask |
| `raycastAll(Vec2 origin, Vec2 direction, float maxDist, std::uint16_t mask = 0xFFFF)` | `std::vector<RaycastHit2D>` | Cast a ray and return all hits along its path, sorted by distance |

### Ground Detection

| Method | Returns | Description |
|--------|---------|-------------|
| `checkGrounded(BodyHandle h, const GroundCheckParams& params = {})` | `GroundCheckResult` | Perform a configurable ground detection check beneath a body |

### World Configuration

| Method | Returns | Description |
|--------|---------|-------------|
| `setGravity(Vec2 gravity)` | `void` | Set the global gravity vector for the physics world |
| `getGravity()` | `Vec2` | Return the current global gravity vector |
| `setTimeScale(float scale)` | `void` | Scale the physics simulation speed (1.0 = normal, 0.5 = half speed) |
| `getTimeScale()` | `float` | Return the current physics time scale multiplier |

### Callbacks

| Method | Returns | Description |
|--------|---------|-------------|
| `onCollision(std::function<void(const CollisionEvent&)> cb)` | `SubscriptionId` | Subscribe to collision events between physics bodies |
| `onTrigger(std::function<void(const TriggerEvent&)> cb)` | `SubscriptionId` | Subscribe to sensor trigger enter/exit events |
| `unsubscribe(SubscriptionId id)` | `void` | Remove a collision or trigger subscription |

### Debug

| Method | Returns | Description |
|--------|---------|-------------|
| `setDebugDraw(bool enabled)` | `void` | Enable or disable physics debug wireframe rendering |
| `isDebugDrawEnabled()` | `bool` | Check whether debug draw is currently enabled |

## Types

### BodyDef2D

Definition struct for creating a 2D physics body.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `type` | `BodyType` | `Static` | The body dynamics type (Static, Kinematic, or Dynamic) |
| `position` | `Vec2` | `{0, 0}` | Initial world-space position of the body |
| `rotation` | `float` | `0` | Initial rotation in radians |
| `fixedRotation` | `bool` | `false` | If true, the body cannot rotate |
| `linearDamping` | `float` | `0` | Linear velocity damping (higher = more drag) |
| `angularDamping` | `float` | `0` | Angular velocity damping (higher = more rotational drag) |
| `gravityScale` | `float` | `1.0f` | Gravity multiplier for this body (0 = no gravity) |
| `bullet` | `bool` | `false` | Enable continuous collision detection for fast-moving objects |
| `shape` | `std::optional<std::variant<BoxShape, CircleShape>>` | `nullopt` | Optional convenience shape created with the body |

### BodyDef2D::BoxShape

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `halfExtents` | `Vec2` | `{0, 0}` | Half-width and half-height of the box shape |
| `density` | `float` | `1.0f` | Density in kg/m^2 for mass calculation |

### BodyDef2D::CircleShape

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `radius` | `float` | `0` | Radius of the circle shape |
| `density` | `float` | `1.0f` | Density in kg/m^2 for mass calculation |

### RaycastHit2D

Result of a 2D raycast query.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `body` | `BodyHandle` | `invalid` | Handle of the body that was hit |
| `entity` | `Entity` | `NullEntity` | The entity associated with the hit body |
| `point` | `Vec2` | `{0, 0}` | World-space position where the ray intersected the body |
| `normal` | `Vec2` | `{0, 0}` | Surface normal at the intersection point |
| `fraction` | `float` | `0.0f` | Fraction along the ray (0.0 = origin, 1.0 = maxDist) |

### GroundCheckResult

Result of a ground detection query.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `grounded` | `bool` | `false` | Whether the body is considered to be on the ground |
| `groundEntity` | `Entity` | `NullEntity` | The entity being stood on, if grounded |
| `contactPoint` | `Vec2` | `{0, 0}` | World-space position of the ground contact |
| `surfaceNormal` | `Vec2` | `{0, -1}` | Surface normal at the ground contact (default: pointing up) |
| `slopeAngle` | `float` | `0.0f` | Angle of the ground surface in degrees from horizontal |

### GroundCheckParams

Configuration parameters for ground detection.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `rayDistance` | `float` | `5.0f` | How far below the body to cast the ground check ray (in pixels) |
| `slopeToleranceDeg` | `float` | `60.0f` | Maximum slope angle in degrees that counts as walkable ground |
| `groundMask` | `std::uint16_t` | `0xFFFF` | Collision mask filtering which layers count as ground |

### BodyType

```cpp
enum class BodyType : std::uint8_t {
    Static,       // Never moves (walls, floors, platforms)
    Kinematic,    // Moves by velocity only, not affected by forces
    Dynamic       // Fully simulated, responds to forces and collisions
};
```

| Value | Description |
|-------|-------------|
| `Static` | Immovable body; used for walls, floors, and static level geometry |
| `Kinematic` | Moved programmatically via velocity; not affected by forces or collisions with static bodies |
| `Dynamic` | Fully simulated body; responds to gravity, forces, impulses, and collisions |

### CollisionEvent

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `entityA` | `Entity` | `NullEntity` | The first entity involved in the collision |
| `entityB` | `Entity` | `NullEntity` | The second entity involved in the collision |
| `contactPoint` | `Vec2` | `{0, 0}` | World-space position of the contact point |
| `normal` | `Vec2` | `{0, 0}` | Collision normal pointing from A to B |
| `impulse` | `float` | `0.0f` | Impulse magnitude applied to resolve the collision |

### TriggerEvent

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `entityA` | `Entity` | `NullEntity` | The entity that entered or exited the trigger |
| `entityB` | `Entity` | `NullEntity` | The sensor entity that was triggered |
| `contactPoint` | `Vec2` | `{0, 0}` | World-space position where overlap was detected |

### BodyHandle

```cpp
using BodyHandle = Handle<struct BodyTag>;
```

A strong typed handle identifying a specific physics body. Distinct from other handle types at compile time.

### ConstraintHandle

```cpp
using ConstraintHandle = Handle<struct ConstraintTag>;
```

A strong typed handle identifying a specific physics constraint (joint).

## Lua Examples

```lua
-- High-level: Create physics bodies
local _, err = bestow.physics.createBody(player, {
    type = "Dynamic",
    position = { x = 100, y = 200 },
    fixedRotation = true,
    shape = { type = "Box", halfExtents = { x = 16, y = 24 } }
})

bestow.physics.createBody(floor, {
    type = "Static",
    position = { x = 400, y = 500 },
    shape = { type = "Box", halfExtents = { x = 400, y = 16 } }
})

-- High-level: Transform and velocity
local pos = bestow.physics.getPosition(player)
bestow.physics.setPosition(player, { x = 200, y = 100 })
local vel = bestow.physics.getVelocity(player)
bestow.physics.setVelocity(player, { x = 0, y = -300 })

-- High-level: Forces
bestow.physics.applyForce(player, { x = 500, y = 0 })
bestow.physics.applyImpulse(player, { x = 0, y = -200 })

-- High-level: Queries
local hit = bestow.physics.raycast({ x = 100, y = 100 }, { x = 1, y = 0 }, 500)
if hit then
    print("Hit entity at " .. hit.point.x .. ", " .. hit.point.y)
end

local ground = bestow.physics.checkGrounded(player)
if ground.grounded then
    -- can jump
end

-- High-level: Callbacks
local colId = bestow.physics.onCollision(function(event)
    print("Collision: " .. tostring(event.entityA) .. " vs " .. tostring(event.entityB))
end)

local trigId = bestow.physics.onTrigger(function(event)
    print("Trigger entered!")
end)

bestow.physics.unsubscribe(colId)

-- Low-level: Body handle-based control
local body = bestow.physics.core.createBody(player, def)
bestow.physics.core.setBodyType(body, "Dynamic")
bestow.physics.core.setAngularVelocity(body, 3.14)
bestow.physics.core.setGravityScale(body, 0.5)

-- Low-level: Shapes
bestow.physics.core.addBoxShape(body, { x = 16, y = 24 })
bestow.physics.core.addCircleShape(body, 10, { x = 0, y = -12 })

-- Low-level: Material
bestow.physics.core.setFriction(body, 0.5)
bestow.physics.core.setRestitution(body, 0.3)

-- Low-level: Collision filtering
bestow.physics.core.setCollisionLayer(body, 0x0001)
bestow.physics.core.setCollisionMask(body, 0x0003)
bestow.physics.core.setSensor(body, true)

-- Low-level: Constraints
local joint = bestow.physics.core.createRevoluteJoint(bodyA, bodyB,
    { x = 100, y = 200 }, true, -1.57, 1.57)
bestow.physics.core.destroyConstraint(joint)

-- Low-level: World config
bestow.physics.core.setGravity({ x = 0, y = 980 })
bestow.physics.core.setTimeScale(0.5)

-- Low-level: Debug
bestow.physics.core.setDebugDraw(true)
```

## C++ Examples

```cpp
// High-level: Entity-centric physics
physics->createBody(player, BodyDef2D{
    .type = BodyType::Dynamic,
    .position = {100, 200},
    .fixedRotation = true,
    .shape = BodyDef2D::BoxShape{{16, 24}, 1.0f}
});

Vec2 pos = physics->getPosition(player);
physics->setVelocity(player, {0, -300});
physics->applyImpulse(player, {0, -200});

auto hit = physics->raycast({100, 100}, {1, 0}, 500);
if (hit) {
    handleRayHit(hit->entity, hit->point);
}

auto ground = physics->checkGrounded(player);
if (ground.grounded) {
    allowJump();
}

auto subId = physics->onCollision([](const CollisionEvent& e) {
    handleCollision(e.entityA, e.entityB, e.impulse);
});

// Low-level: Handle-based body management
auto bodyResult = physicsCore->createBody(player, def);
BodyHandle body = bodyResult.value();

physicsCore->setBodyType(body, BodyType::Dynamic);
physicsCore->setGravityScale(body, 0.0f);
physicsCore->setAngularVelocity(body, 3.14f);

// Low-level: Shapes
physicsCore->addBoxShape(body, {16, 24}, {0, 0}, 1.0f);
physicsCore->addCircleShape(body, 10.0f, {0, -12});
physicsCore->addChainShape(body, groundVertices, true);

// Low-level: Material
physicsCore->setFriction(body, 0.5f);
physicsCore->setRestitution(body, 0.3f);
physicsCore->setDensity(body, 2.0f);

// Low-level: Forces at points
physicsCore->applyForce(body, {500, 0}, {120, 200});
physicsCore->applyImpulseToCenter(body, {0, -200});
physicsCore->applyTorque(body, 100.0f);

// Low-level: Collision filtering
physicsCore->setCollisionLayer(body, 0x0001);
physicsCore->setCollisionMask(body, 0x0003);
physicsCore->setSensor(body, false);

// Low-level: Constraints
auto joint = physicsCore->createRevoluteJoint(bodyA, bodyB, {100, 200}, true, -1.57f, 1.57f);
auto weld = physicsCore->createWeldJoint(bodyA, bodyB, {150, 200});

// Low-level: Queries
auto bodies = physicsCore->queryAABB({0, 0}, {800, 600});
auto allHits = physicsCore->raycastAll({100, 100}, {1, 0}, 500, 0x00FF);

// Low-level: Ground check with custom params
auto ground = physicsCore->checkGrounded(body, GroundCheckParams{
    .rayDistance = 8.0f,
    .slopeToleranceDeg = 45.0f,
    .groundMask = 0x0003
});

// Low-level: World config
physicsCore->setGravity({0, 980});
physicsCore->setTimeScale(0.5f);
physicsCore->setDebugDraw(true);
```
