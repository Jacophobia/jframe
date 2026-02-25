# Physics 3D System

> **Visibility:** Public (Lua + C++ API)
> **Tier:** 3
> **Dependencies:** Types, Entity, Events
> **Lua Paths:** `bestow.physics3d` (high-level), `bestow.physics3d.core` (low-level)

## Purpose

The Physics 3D System provides rigid body dynamics, collision detection, character controllers, and spatial queries for 3D games. It integrates with the Entity System so that physics bodies and character controllers are associated with entities. The high-level API offers entity-centric body creation, transform/velocity access, force application, character controller movement, raycasting, and gravity control. The low-level API exposes BodyHandle-based control, full transform and velocity management, all shape types (box, sphere, capsule, cylinder, convex hull, triangle mesh, height field), material properties, collision filtering, a comprehensive constraint system (fixed, hinge, slider, ball-socket, cone, 6DOF with motors), detailed character controller state, full spatial queries (raycastAll, sphereCast, overlapSphere, overlapBox), world configuration, and debug drawing. Physics runs in the FixedUpdate phase at a deterministic timestep.

## High-Level API: `IPhysics3DSystem`

The simplified API for common game development tasks. Entity-centric, sensible defaults, no lifecycle management.

### Body Creation

| Method | Returns | Description |
|--------|---------|-------------|
| `createBody(Entity entity, const BodyDef3D& def)` | `Result<void>` | Create a 3D physics body for an entity using the provided definition |
| `destroyBody(Entity entity)` | `Result<void>` | Destroy the 3D physics body associated with an entity |

### Transform

| Method | Returns | Description |
|--------|---------|-------------|
| `getPosition(Entity entity)` | `Vec3` | Return the world-space position of the entity's physics body |
| `setPosition(Entity entity, Vec3 pos)` | `Result<void>` | Teleport the entity's physics body to a new position |
| `getRotation(Entity entity)` | `Quat` | Return the current rotation of the entity's physics body as a quaternion |
| `getVelocity(Entity entity)` | `Vec3` | Return the current linear velocity of the entity's physics body |
| `setVelocity(Entity entity, Vec3 vel)` | `Result<void>` | Set the linear velocity of the entity's physics body directly |

### Forces

| Method | Returns | Description |
|--------|---------|-------------|
| `applyForce(Entity entity, Vec3 force)` | `Result<void>` | Apply a continuous force to the entity's body center of mass |
| `applyImpulse(Entity entity, Vec3 impulse)` | `Result<void>` | Apply an instantaneous impulse to the entity's body center of mass |

### Character Controller

| Method | Returns | Description |
|--------|---------|-------------|
| `createCharacterController(Entity entity, const CharacterDef3D& def)` | `Result<void>` | Create a character controller for an entity with capsule collision |
| `moveCharacter(Entity entity, Vec3 displacement, DeltaTime dt)` | `Result<void>` | Move a character controller by a displacement vector over the given timestep |
| `isCharacterGrounded(Entity entity)` | `bool` | Check whether a character controller is currently on the ground |

### Queries

| Method | Returns | Description |
|--------|---------|-------------|
| `raycast(Vec3 origin, Vec3 direction, float maxDist)` | `std::optional<RaycastHit3D>` | Cast a ray and return the closest hit, or nullopt if nothing was hit |

### World

| Method | Returns | Description |
|--------|---------|-------------|
| `setGravity(Vec3 gravity)` | `void` | Set the global gravity vector for the 3D physics world |

### Callbacks

| Method | Returns | Description |
|--------|---------|-------------|
| `onCollision(std::function<void(const CollisionEvent3D&)> cb)` | `SubscriptionId` | Subscribe to 3D collision events between physics bodies |
| `unsubscribe(SubscriptionId id)` | `void` | Remove a collision subscription |

## Low-Level API: `IPhysics3DCore`

Full control API. BodyHandle-based, full transform and velocity control, all shape types, material properties, collision filtering, comprehensive constraints with motors, character controller details, full spatial queries, world config, and debug draw.

### Lifecycle

| Method | Returns | Description |
|--------|---------|-------------|
| `initialize()` | `Result<void>` | Initialize the 3D physics engine and allocate world resources |
| `shutdown()` | `void` | Destroy the physics world and release all resources |
| `update(DeltaTime fixedDt)` | `void` | Step the 3D physics simulation by the fixed timestep; called in FixedUpdate phase |

### Body Management

| Method | Returns | Description |
|--------|---------|-------------|
| `createBody(Entity entity, const BodyDef3D& def)` | `Result<BodyHandle>` | Create a 3D physics body for an entity and return its handle |
| `destroyBody(BodyHandle handle)` | `Result<void>` | Destroy a 3D physics body by handle |
| `bodyExists(BodyHandle handle)` | `bool` | Check whether a body handle is still valid |
| `getBody(Entity entity)` | `std::optional<BodyHandle>` | Look up the body handle associated with an entity |

### Transform

| Method | Returns | Description |
|--------|---------|-------------|
| `setPosition(BodyHandle h, Vec3 pos)` | `Result<void>` | Teleport a body to a new position |
| `getPosition(BodyHandle h)` | `Vec3` | Return a body's current world-space position |
| `setRotation(BodyHandle h, Quat rot)` | `Result<void>` | Set a body's rotation as a quaternion |
| `getRotation(BodyHandle h)` | `Quat` | Return a body's current rotation as a quaternion |
| `setTransform(BodyHandle h, Vec3 pos, Quat rot)` | `Result<void>` | Set both position and rotation in a single call |

### Velocity

| Method | Returns | Description |
|--------|---------|-------------|
| `setLinearVelocity(BodyHandle h, Vec3 vel)` | `Result<void>` | Set a body's linear velocity directly |
| `getLinearVelocity(BodyHandle h)` | `Vec3` | Return a body's current linear velocity |
| `setAngularVelocity(BodyHandle h, Vec3 omega)` | `Result<void>` | Set a body's angular velocity as a 3D vector (axis * magnitude) |
| `getAngularVelocity(BodyHandle h)` | `Vec3` | Return a body's current angular velocity |

### Forces

| Method | Returns | Description |
|--------|---------|-------------|
| `applyForce(BodyHandle h, Vec3 force)` | `Result<void>` | Apply a continuous force to the body's center of mass |
| `applyForceAtPoint(BodyHandle h, Vec3 force, Vec3 point)` | `Result<void>` | Apply a continuous force at a specific world-space point on the body |
| `applyImpulse(BodyHandle h, Vec3 impulse)` | `Result<void>` | Apply an instantaneous impulse to the center of mass |
| `applyImpulseAtPoint(BodyHandle h, Vec3 impulse, Vec3 point)` | `Result<void>` | Apply an instantaneous impulse at a specific world-space point |
| `applyTorque(BodyHandle h, Vec3 torque)` | `Result<void>` | Apply a continuous rotational torque to the body |

### Shape Types

| Method | Returns | Description |
|--------|---------|-------------|
| `addBoxShape(BodyHandle h, Vec3 halfExtents, Vec3 offset = {}, Quat rotation = {}, float density = 1.0f)` | `Result<void>` | Add a box collision shape with half-extents, optional offset and rotation |
| `addSphereShape(BodyHandle h, float radius, Vec3 offset = {}, float density = 1.0f)` | `Result<void>` | Add a sphere collision shape with radius and optional offset |
| `addCapsuleShape(BodyHandle h, float halfHeight, float radius, Vec3 offset = {}, float density = 1.0f)` | `Result<void>` | Add a capsule collision shape (cylinder with hemisphere caps) |
| `addCylinderShape(BodyHandle h, float halfHeight, float radius, float density = 1.0f)` | `Result<void>` | Add a cylinder collision shape with half-height and radius |
| `addConvexHullShape(BodyHandle h, std::span<const Vec3> vertices, float density = 1.0f)` | `Result<void>` | Add a convex hull collision shape from a set of vertices |
| `addMeshShape(BodyHandle h, std::span<const Vec3> vertices, std::span<const std::uint32_t> indices)` | `Result<void>` | Add a triangle mesh collision shape (static bodies only) |
| `addHeightFieldShape(BodyHandle h, int width, int height, std::span<const float> heights, Vec3 scale = {1,1,1})` | `Result<void>` | Add a height field terrain shape from a grid of height values |

### Material Properties

| Method | Returns | Description |
|--------|---------|-------------|
| `setFriction(BodyHandle h, float friction)` | `Result<void>` | Set the friction coefficient for all shapes on a body |
| `setRestitution(BodyHandle h, float restitution)` | `Result<void>` | Set the bounciness for all shapes on a body |

### Collision Filtering

| Method | Returns | Description |
|--------|---------|-------------|
| `setCollisionLayer(BodyHandle h, std::uint16_t layer)` | `Result<void>` | Set which collision layer this body belongs to (bitmask) |
| `setCollisionMask(BodyHandle h, std::uint16_t mask)` | `Result<void>` | Set which collision layers this body can interact with (bitmask) |
| `setSensor(BodyHandle h, bool isSensor)` | `Result<void>` | Make the body a sensor (detects overlaps without collision response) |

### Constraints

| Method | Returns | Description |
|--------|---------|-------------|
| `createFixedConstraint(BodyHandle a, BodyHandle b, Vec3 anchor)` | `Result<ConstraintHandle>` | Create a fixed constraint rigidly connecting two bodies at an anchor point |
| `createHingeConstraint(BodyHandle a, BodyHandle b, Vec3 anchor, Vec3 axis, float minAngle = 0, float maxAngle = 0)` | `Result<ConstraintHandle>` | Create a hinge constraint allowing rotation around an axis with optional angle limits |
| `createSliderConstraint(BodyHandle a, BodyHandle b, Vec3 axis, float minDist = 0, float maxDist = 0)` | `Result<ConstraintHandle>` | Create a slider constraint allowing translation along an axis with optional distance limits |
| `createBallSocketConstraint(BodyHandle a, BodyHandle b, Vec3 pivotA, Vec3 pivotB)` | `Result<ConstraintHandle>` | Create a ball-and-socket constraint allowing free rotation around pivot points |
| `createConeConstraint(BodyHandle a, BodyHandle b, Vec3 anchor, Vec3 axis, float halfAngle)` | `Result<ConstraintHandle>` | Create a cone constraint limiting rotation to a cone-shaped region |
| `createSixDOFConstraint(BodyHandle a, BodyHandle b, Vec3 linearMin, Vec3 linearMax, Vec3 angularMin, Vec3 angularMax)` | `Result<ConstraintHandle>` | Create a 6 degrees-of-freedom constraint with per-axis linear and angular limits |
| `setConstraintMotor(ConstraintHandle h, float targetVelocity, float maxForce)` | `Result<void>` | Attach a motor to a constraint driving it toward a target velocity |
| `destroyConstraint(ConstraintHandle h)` | `Result<void>` | Destroy a constraint and free the connected bodies |

### Character Controller

| Method | Returns | Description |
|--------|---------|-------------|
| `createCharacterController(Entity entity, const CharacterDef3D& def)` | `Result<BodyHandle>` | Create a character controller with capsule collision and return its handle |
| `moveCharacter(BodyHandle h, Vec3 displacement, DeltaTime dt)` | `Result<void>` | Move a character controller by a displacement vector |
| `isCharacterGrounded(BodyHandle h)` | `bool` | Check whether a character controller is currently on the ground |
| `getCharacterGroundNormal(BodyHandle h)` | `Vec3` | Return the surface normal beneath a grounded character |
| `getCharacterGroundState(BodyHandle h)` | `CharacterGroundState` | Return the detailed ground state of a character controller |

### Queries

| Method | Returns | Description |
|--------|---------|-------------|
| `raycast(Vec3 origin, Vec3 direction, float maxDist, std::uint16_t mask = 0xFFFF)` | `std::optional<RaycastHit3D>` | Cast a ray and return the closest hit filtered by collision mask |
| `raycastAll(Vec3 origin, Vec3 direction, float maxDist, std::uint16_t mask = 0xFFFF)` | `std::vector<RaycastHit3D>` | Cast a ray and return all hits sorted by distance |
| `sphereCast(Vec3 origin, Vec3 direction, float radius, float maxDist, std::uint16_t mask = 0xFFFF)` | `std::optional<RaycastHit3D>` | Sweep a sphere along a direction and return the closest hit |
| `overlapSphere(Vec3 center, float radius, std::uint16_t mask = 0xFFFF)` | `std::vector<BodyHandle>` | Return all bodies overlapping a sphere region |
| `overlapBox(Vec3 center, Vec3 halfExtents, Quat rotation = {}, std::uint16_t mask = 0xFFFF)` | `std::vector<BodyHandle>` | Return all bodies overlapping an oriented box region |

### World Configuration

| Method | Returns | Description |
|--------|---------|-------------|
| `setGravity(Vec3 gravity)` | `void` | Set the global gravity vector for the 3D physics world |
| `getGravity()` | `Vec3` | Return the current global gravity vector |
| `setTimeScale(float scale)` | `void` | Scale the physics simulation speed (1.0 = normal, 0.5 = half speed) |

### Callbacks

| Method | Returns | Description |
|--------|---------|-------------|
| `onCollision(std::function<void(const CollisionEvent3D&)> cb)` | `SubscriptionId` | Subscribe to 3D collision events between physics bodies |
| `onTrigger(std::function<void(const TriggerEvent3D&)> cb)` | `SubscriptionId` | Subscribe to 3D sensor trigger enter/exit events |
| `unsubscribe(SubscriptionId id)` | `void` | Remove a collision or trigger subscription |

### Debug

| Method | Returns | Description |
|--------|---------|-------------|
| `setDebugDraw(bool enabled)` | `void` | Enable or disable physics debug wireframe rendering |

## Types

### BodyDef3D

Definition struct for creating a 3D physics body.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `type` | `BodyType` | `Static` | The body dynamics type (Static, Kinematic, or Dynamic) |
| `position` | `Vec3` | `{0, 0, 0}` | Initial world-space position of the body |
| `rotation` | `Quat` | `{0, 0, 0, 1}` | Initial rotation as a quaternion |
| `fixedRotation` | `bool` | `false` | If true, the body cannot rotate |
| `linearDamping` | `float` | `0` | Linear velocity damping (higher = more drag) |
| `angularDamping` | `float` | `0` | Angular velocity damping |
| `gravityScale` | `float` | `1.0f` | Gravity multiplier for this body |
| `mass` | `float` | `0` | Explicit mass override; 0 means auto-calculate from shapes and density |

### CharacterDef3D

Definition struct for creating a 3D character controller.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `height` | `float` | `1.8f` | Total height of the character capsule in world units |
| `radius` | `float` | `0.3f` | Radius of the character capsule |
| `mass` | `float` | `80.0f` | Mass of the character in kilograms |
| `maxSlopeAngle` | `float` | `50.0f` | Maximum slope angle in degrees the character can walk up |
| `stepHeight` | `float` | `0.35f` | Maximum step height the character can automatically climb |
| `position` | `Vec3` | `{0, 0, 0}` | Initial world-space position of the character |

### CharacterGroundState

Detailed ground state for character controllers.

```cpp
enum class CharacterGroundState : std::uint8_t {
    OnGround,        // Standing on walkable ground
    OnSteepGround,   // On ground steeper than maxSlopeAngle
    InAir,           // Not touching any ground
    NotSupported     // On ground that cannot support the character
};
```

| Value | Description |
|-------|-------------|
| `OnGround` | Character is standing on ground within the walkable slope angle |
| `OnSteepGround` | Character is on ground steeper than the configured maxSlopeAngle |
| `InAir` | Character is not in contact with any ground surface |
| `NotSupported` | Character is on a surface that cannot support it (e.g., moving platform edge) |

### RaycastHit3D

Result of a 3D raycast or sphere cast query.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `entity` | `Entity` | `NullEntity` | The entity associated with the hit body |
| `point` | `Vec3` | `{0, 0, 0}` | World-space position where the ray intersected the body |
| `normal` | `Vec3` | `{0, 0, 0}` | Surface normal at the intersection point |
| `distance` | `float` | `0.0f` | Distance from the ray origin to the hit point |
| `shapeIndex` | `std::uint32_t` | `0` | Index of the specific shape that was hit on the body |

### CollisionEvent3D

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `entityA` | `Entity` | `NullEntity` | The first entity involved in the collision |
| `entityB` | `Entity` | `NullEntity` | The second entity involved in the collision |
| `contactPoint` | `Vec3` | `{0, 0, 0}` | World-space position of the contact point |
| `contactNormal` | `Vec3` | `{0, 0, 0}` | Collision normal pointing from A to B |
| `impulse` | `float` | `0.0f` | Impulse magnitude applied to resolve the collision |
| `penetrationDepth` | `float` | `0.0f` | Depth of penetration between the two bodies |

### TriggerEvent3D

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `entityA` | `Entity` | `NullEntity` | The entity that entered or exited the 3D trigger |
| `entityB` | `Entity` | `NullEntity` | The 3D sensor entity that was triggered |

### BodyHandle

```cpp
using BodyHandle = Handle<struct BodyTag>;
```

A strong typed handle identifying a specific 3D physics body. Same type as 2D BodyHandle -- the system context determines which physics world is addressed.

### ConstraintHandle

```cpp
using ConstraintHandle = Handle<struct ConstraintTag>;
```

A strong typed handle identifying a specific 3D physics constraint.

## Lua Examples

```lua
-- High-level: Create a dynamic body
bestow.physics3d.createBody(crate, {
    type = "Dynamic",
    position = { x = 0, y = 5, z = 0 },
    mass = 10
})

-- High-level: Create a static floor
bestow.physics3d.createBody(floor, {
    type = "Static",
    position = { x = 0, y = 0, z = 0 }
})

-- High-level: Transform
local pos = bestow.physics3d.getPosition(crate)
bestow.physics3d.setPosition(crate, { x = 10, y = 5, z = 0 })
local rot = bestow.physics3d.getRotation(crate)
local vel = bestow.physics3d.getVelocity(crate)
bestow.physics3d.setVelocity(crate, { x = 0, y = 10, z = 0 })

-- High-level: Forces
bestow.physics3d.applyForce(crate, { x = 100, y = 0, z = 0 })
bestow.physics3d.applyImpulse(crate, { x = 0, y = 50, z = 0 })

-- High-level: Character controller
bestow.physics3d.createCharacterController(player, {
    height = 1.8,
    radius = 0.3,
    mass = 80,
    maxSlopeAngle = 45,
    stepHeight = 0.35,
    position = { x = 0, y = 1, z = 0 }
})
bestow.physics3d.moveCharacter(player, { x = dx, y = 0, z = dz }, dt)
if bestow.physics3d.isCharacterGrounded(player) then
    -- allow jump
end

-- High-level: Raycast
local hit = bestow.physics3d.raycast(
    { x = 0, y = 10, z = 0 },
    { x = 0, y = -1, z = 0 },
    100
)
if hit then
    print("Hit at distance: " .. hit.distance)
end

-- High-level: Gravity and callbacks
bestow.physics3d.setGravity({ x = 0, y = -9.81, z = 0 })
local subId = bestow.physics3d.onCollision(function(event)
    print("3D collision: impulse = " .. event.impulse)
end)

-- Low-level: Body handle control
local body = bestow.physics3d.core.createBody(crate, def)
bestow.physics3d.core.setTransform(body, pos, rot)
bestow.physics3d.core.setLinearVelocity(body, vel)
bestow.physics3d.core.setAngularVelocity(body, { x = 0, y = 1, z = 0 })

-- Low-level: Shape types
bestow.physics3d.core.addBoxShape(body, { x = 0.5, y = 0.5, z = 0.5 })
bestow.physics3d.core.addSphereShape(body, 1.0)
bestow.physics3d.core.addCapsuleShape(body, 0.9, 0.3)
bestow.physics3d.core.addCylinderShape(body, 1.0, 0.5)

-- Low-level: Material
bestow.physics3d.core.setFriction(body, 0.6)
bestow.physics3d.core.setRestitution(body, 0.2)

-- Low-level: Collision filtering
bestow.physics3d.core.setCollisionLayer(body, 0x0004)
bestow.physics3d.core.setCollisionMask(body, 0x000F)

-- Low-level: Constraints
local hinge = bestow.physics3d.core.createHingeConstraint(
    doorBody, frameBody,
    { x = -1, y = 1, z = 0 },  -- anchor
    { x = 0, y = 1, z = 0 },   -- axis
    -1.57, 1.57                  -- angle limits
)
bestow.physics3d.core.setConstraintMotor(hinge, 2.0, 100)

local ball = bestow.physics3d.core.createBallSocketConstraint(
    bodyA, bodyB,
    { x = 0, y = 1, z = 0 },
    { x = 0, y = -1, z = 0 }
)

-- Low-level: Character controller details
local groundState = bestow.physics3d.core.getCharacterGroundState(charBody)
local groundNormal = bestow.physics3d.core.getCharacterGroundNormal(charBody)

-- Low-level: Queries
local allHits = bestow.physics3d.core.raycastAll(origin, direction, 100)
local sphereHit = bestow.physics3d.core.sphereCast(origin, direction, 0.5, 50)
local nearby = bestow.physics3d.core.overlapSphere({ x = 0, y = 0, z = 0 }, 10)
local inBox = bestow.physics3d.core.overlapBox(center, halfExtents, rotation, 0xFFFF)

-- Low-level: World config
bestow.physics3d.core.setGravity({ x = 0, y = -9.81, z = 0 })
bestow.physics3d.core.setTimeScale(0.5)
bestow.physics3d.core.setDebugDraw(true)
```

## C++ Examples

```cpp
// High-level: Entity-centric 3D physics
physics3D->createBody(crate, BodyDef3D{
    .type = BodyType::Dynamic,
    .position = {0, 5, 0},
    .mass = 10.0f
});

Vec3 pos = physics3D->getPosition(crate);
physics3D->setVelocity(crate, {0, 10, 0});
physics3D->applyImpulse(crate, {0, 50, 0});

// High-level: Character controller
physics3D->createCharacterController(player, CharacterDef3D{
    .height = 1.8f, .radius = 0.3f, .mass = 80.0f,
    .maxSlopeAngle = 45.0f, .stepHeight = 0.35f,
    .position = {0, 1, 0}
});
physics3D->moveCharacter(player, moveDir * speed, dt);
if (physics3D->isCharacterGrounded(player)) {
    // allow jump
}

// High-level: Raycast
auto hit = physics3D->raycast({0, 10, 0}, {0, -1, 0}, 100);
if (hit) {
    handleHit(hit->entity, hit->point, hit->distance);
}

auto subId = physics3D->onCollision([](const CollisionEvent3D& e) {
    handleCollision(e.entityA, e.entityB, e.impulse);
});

// Low-level: Handle-based body management
auto bodyResult = physics3DCore->createBody(crate, def);
BodyHandle body = bodyResult.value();

physics3DCore->setTransform(body, Vec3{10, 5, 0}, Quat{});
physics3DCore->setLinearVelocity(body, {0, 10, 0});
physics3DCore->setAngularVelocity(body, {0, 1, 0});

// Low-level: Shape types
physics3DCore->addBoxShape(body, {0.5f, 0.5f, 0.5f});
physics3DCore->addSphereShape(body, 1.0f);
physics3DCore->addCapsuleShape(body, 0.9f, 0.3f);
physics3DCore->addCylinderShape(body, 1.0f, 0.5f);
physics3DCore->addConvexHullShape(body, hullVertices, 1.0f);
physics3DCore->addMeshShape(body, meshVertices, meshIndices);
physics3DCore->addHeightFieldShape(body, 256, 256, heightData, {1, 50, 1});

// Low-level: Forces at points
physics3DCore->applyForceAtPoint(body, {100, 0, 0}, {1, 2, 0});
physics3DCore->applyImpulseAtPoint(body, {0, 50, 0}, {0, 1, 0});
physics3DCore->applyTorque(body, {0, 10, 0});

// Low-level: Constraints
auto hinge = physics3DCore->createHingeConstraint(
    doorBody, frameBody, {-1, 1, 0}, {0, 1, 0}, -1.57f, 1.57f);
physics3DCore->setConstraintMotor(hinge.value(), 2.0f, 100.0f);

auto sixDof = physics3DCore->createSixDOFConstraint(
    bodyA, bodyB,
    {-1, -1, -1}, {1, 1, 1},   // linear limits
    {-0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}  // angular limits
);

// Low-level: Character controller details
CharacterGroundState state = physics3DCore->getCharacterGroundState(charBody);
Vec3 normal = physics3DCore->getCharacterGroundNormal(charBody);

// Low-level: Spatial queries
auto allHits = physics3DCore->raycastAll({0, 10, 0}, {0, -1, 0}, 100);
auto sphereHit = physics3DCore->sphereCast({0, 2, 0}, {1, 0, 0}, 0.5f, 50);
auto nearby = physics3DCore->overlapSphere({0, 0, 0}, 10.0f, 0x000F);
auto inBox = physics3DCore->overlapBox({0, 5, 0}, {5, 5, 5});

// Low-level: World config
physics3DCore->setGravity({0, -9.81f, 0});
physics3DCore->setTimeScale(0.5f);
physics3DCore->setDebugDraw(true);
```
