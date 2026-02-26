# Physics 3D System

> **Visibility:** Public (Lua + C++ API)
> **Tier:** 3
> **Dependencies:** Types, Entity, Events
> **Lua Paths:** `bestow.physics3d` (high-level), `bestow.physics3d.core` (low-level)

## Purpose

The Physics 3D System provides rigid body dynamics, collision detection, character controllers, vehicle simulation, and spatial queries for 3D games. Built on Jolt Physics, it integrates with the Entity System so that physics bodies, character controllers, and vehicles are all associated with entities. The high-level API offers entity-centric operations with sensible defaults: creating bodies from simple shape descriptions, applying forces and impulses, querying velocities, performing raycasts, and checking sphere overlaps. The low-level API exposes BodyHandle-based control over the full simulation: compound and height-field shapes, mass properties and inertia tensors, continuous collision detection, collision filtering, sleep management, bounding box queries, comprehensive constraint system (fixed, hinge, slider, cone, point, distance with motors), detailed contact queries, full character controller state, vehicle dynamics with per-wheel state, shape casting (sphere, box, capsule), AABB queries, world configuration, debug line rendering, and simulation statistics. Physics runs in the FixedUpdate phase at a deterministic timestep.

## High-Level API: `IPhysics3DSystem`

The simplified API for common 3D game development tasks. Entity-centric with sensible defaults, no handles to manage. All body and character operations take an `Entity` directly.

### Body Management

| Method | Returns | Description |
|--------|---------|-------------|
| `addBody(Entity entity, std::string_view shapeType, Vec3 halfExtents)` | `Result<void>` | Add a physics body to an entity with a common shape type ("Box", "Sphere", "Capsule") and half-extents; uses dynamic body type and default material properties |
| `removeBody(Entity entity)` | `Result<void>` | Remove the physics body from an entity and release its resources |
| `hasBody(Entity entity)` | `bool` | Check whether an entity has an associated physics body |

### Velocity

| Method | Returns | Description |
|--------|---------|-------------|
| `setVelocity(Entity entity, Vec3 velocity)` | `Result<void>` | Set the linear velocity of the entity's physics body directly |
| `getVelocity(Entity entity)` | `Result<Vec3>` | Return the current linear velocity of the entity's physics body |

### Forces

| Method | Returns | Description |
|--------|---------|-------------|
| `applyForce(Entity entity, Vec3 force)` | `Result<void>` | Apply a continuous force to the entity's body center of mass; accumulates until the next simulation step |
| `applyImpulse(Entity entity, Vec3 impulse)` | `Result<void>` | Apply an instantaneous impulse to the entity's body center of mass; immediately changes velocity |

### World

| Method | Returns | Description |
|--------|---------|-------------|
| `setGravity(Vec3 gravity)` | `void` | Set the global gravity vector for the 3D physics world |
| `getGravity()` | `Vec3` | Return the current global gravity vector |

### Queries

| Method | Returns | Description |
|--------|---------|-------------|
| `raycast(Vec3 origin, Vec3 direction, float maxDistance)` | `std::optional<RaycastHit3D>` | Cast a ray and return the closest hit, or nullopt if nothing was hit |
| `overlapSphere(Vec3 center, float radius)` | `std::vector<Entity>` | Return all entities whose physics bodies overlap a sphere region |

## Low-Level API: `IPhysics3DCore`

Full control API. BodyHandle-based body management, all shape types including compound and height field, full transform and velocity control, force application at arbitrary world points, material and mass property tuning, continuous collision detection, collision layer filtering, sleep state, bounding boxes, comprehensive constraint system with motors, detailed contact queries, character controllers with ground state, vehicle simulation with per-wheel state, full spatial queries (raycast, raycastAll, sphereCast, boxCast, capsuleCast, overlapSphere, overlapBox, queryAABB), world configuration, collision and trigger callbacks, debug line rendering, and simulation statistics.

### Lifecycle

| Method | Returns | Description |
|--------|---------|-------------|
| `update(DeltaTime dt, int subSteps)` | `void` | Step the 3D physics simulation by the given timestep with the specified number of substeps for stability |
| `syncTransforms(std::span<const Entity> entities)` | `void` | Synchronize entity transforms from physics body positions; call after `update()` to apply simulation results |

### Body Management

| Method | Returns | Description |
|--------|---------|-------------|
| `createBody(Entity entity, const PhysicsBodyDef3D& def)` | `Result<BodyHandle>` | Create a 3D physics body for an entity and return its handle |
| `destroyBody(BodyHandle handle)` | `Result<void>` | Destroy a 3D physics body by handle and release all associated shapes and constraints |
| `hasBody(Entity entity)` | `bool` | Check whether an entity has an associated physics body |
| `getAllBodies()` | `std::vector<BodyHandle>` | Return handles for all active physics bodies in the world |

### Compound Shapes

| Method | Returns | Description |
|--------|---------|-------------|
| `createCompoundBody(Entity entity, BodyType3D type, const Transform3D& transform, const CompoundShapeDef& shape)` | `Result<BodyHandle>` | Create a body with a compound shape composed of multiple sub-shapes |
| `addShape(BodyHandle handle, const BoxShapeDef& shape)` | `Result<uint32_t>` | Add a box sub-shape to an existing body and return the shape index |
| `addShape(BodyHandle handle, const SphereShapeDef& shape)` | `Result<uint32_t>` | Add a sphere sub-shape to an existing body and return the shape index |
| `addShape(BodyHandle handle, const CapsuleShapeDef& shape)` | `Result<uint32_t>` | Add a capsule sub-shape to an existing body and return the shape index |
| `removeShape(BodyHandle handle, uint32_t shapeIndex)` | `Result<void>` | Remove a sub-shape from a body by its index |
| `getShapeCount(BodyHandle handle)` | `Result<uint32_t>` | Return the number of collision shapes attached to a body |

### Height Fields

| Method | Returns | Description |
|--------|---------|-------------|
| `createHeightFieldBody(Entity entity, const Transform3D& transform, const HeightFieldShapeDef& shape)` | `Result<BodyHandle>` | Create a static body with a height-field terrain shape from a grid of height values |

### Body Type

| Method | Returns | Description |
|--------|---------|-------------|
| `setBodyType(BodyHandle handle, BodyType3D type)` | `Result<void>` | Change a body's dynamics type (Static, Kinematic, or Dynamic) |
| `getBodyType(BodyHandle handle)` | `Result<BodyType3D>` | Return the current dynamics type of a body |

### Transform

| Method | Returns | Description |
|--------|---------|-------------|
| `setTransform(BodyHandle handle, const Transform3D& transform)` | `Result<void>` | Set both position and rotation of a body in a single call |
| `getTransform(BodyHandle handle)` | `Result<Transform3D>` | Return the full transform (position, rotation, scale) of a body |
| `setPosition(BodyHandle handle, Vec3 position)` | `Result<void>` | Teleport a body to a new world-space position |
| `getPosition(BodyHandle handle)` | `Result<Vec3>` | Return a body's current world-space position |
| `setRotation(BodyHandle handle, Quat rotation)` | `Result<void>` | Set a body's rotation as a quaternion |
| `getRotation(BodyHandle handle)` | `Result<Quat>` | Return a body's current rotation as a quaternion |

### Velocity

| Method | Returns | Description |
|--------|---------|-------------|
| `setLinearVelocity(BodyHandle handle, Vec3 velocity)` | `Result<void>` | Set a body's linear velocity directly |
| `getLinearVelocity(BodyHandle handle)` | `Result<Vec3>` | Return a body's current linear velocity |
| `setAngularVelocity(BodyHandle handle, Vec3 velocity)` | `Result<void>` | Set a body's angular velocity as a 3D vector (axis times magnitude in radians per second) |
| `getAngularVelocity(BodyHandle handle)` | `Result<Vec3>` | Return a body's current angular velocity |

### Forces

| Method | Returns | Description |
|--------|---------|-------------|
| `applyForce(BodyHandle handle, Vec3 force)` | `Result<void>` | Apply a continuous force to the body's center of mass |
| `applyForceAtPoint(BodyHandle handle, Vec3 force, Vec3 worldPoint)` | `Result<void>` | Apply a continuous force at a specific world-space point, generating both linear force and torque |
| `applyTorque(BodyHandle handle, Vec3 torque)` | `Result<void>` | Apply a continuous rotational torque to the body |
| `applyImpulse(BodyHandle handle, Vec3 impulse)` | `Result<void>` | Apply an instantaneous impulse to the body's center of mass |
| `applyImpulseAtPoint(BodyHandle handle, Vec3 impulse, Vec3 worldPoint)` | `Result<void>` | Apply an instantaneous impulse at a specific world-space point |
| `applyAngularImpulse(BodyHandle handle, Vec3 impulse)` | `Result<void>` | Apply an instantaneous angular impulse to the body |

### Properties

| Method | Returns | Description |
|--------|---------|-------------|
| `setMass(BodyHandle handle, float mass)` | `Result<void>` | Set the total mass of a body in kilograms; overrides density-based calculation |
| `getMass(BodyHandle handle)` | `Result<float>` | Return the total mass of a body in kilograms |
| `setLinearDamping(BodyHandle handle, float damping)` | `Result<void>` | Set linear velocity damping; higher values cause the body to slow down faster |
| `setAngularDamping(BodyHandle handle, float damping)` | `Result<void>` | Set angular velocity damping; higher values cause rotation to slow down faster |
| `setGravityFactor(BodyHandle handle, float factor)` | `Result<void>` | Set the gravity multiplier for this body (0 = no gravity, 1 = normal, 2 = double) |
| `setFriction(BodyHandle handle, float friction)` | `Result<void>` | Set the friction coefficient for all shapes on a body |
| `setRestitution(BodyHandle handle, float restitution)` | `Result<void>` | Set the bounciness for all shapes on a body (0 = no bounce, 1 = perfect bounce) |

### Mass Properties

| Method | Returns | Description |
|--------|---------|-------------|
| `setMassProperties(BodyHandle handle, const MassProperties& props)` | `Result<void>` | Set explicit mass, center of mass, and inertia tensor for a body |
| `getMassProperties(BodyHandle handle)` | `Result<MassProperties>` | Return the full mass properties of a body |
| `getCenterOfMass(BodyHandle handle)` | `Result<Vec3>` | Return the world-space center of mass of a body |
| `getInertiaTensor(BodyHandle handle)` | `Result<Mat3>` | Return the 3x3 inertia tensor of a body |

### CCD (Continuous Collision Detection)

| Method | Returns | Description |
|--------|---------|-------------|
| `setMotionQuality(BodyHandle handle, MotionQuality quality)` | `Result<void>` | Set the motion quality for a body; use LinearCast for fast-moving objects to prevent tunneling |
| `getMotionQuality(BodyHandle handle)` | `Result<MotionQuality>` | Return the current motion quality setting for a body |

### Collision Filtering

| Method | Returns | Description |
|--------|---------|-------------|
| `setCollisionLayer(BodyHandle handle, CollisionLayer3D layer)` | `Result<void>` | Set which collision layer this body belongs to (bitmask) |
| `setCollisionMask(BodyHandle handle, CollisionMask3D mask)` | `Result<void>` | Set which collision layers this body can interact with (bitmask) |
| `setSensor(BodyHandle handle, bool isSensor)` | `Result<void>` | Make the body a sensor; sensors detect overlaps without generating collision response |

### Sleep

| Method | Returns | Description |
|--------|---------|-------------|
| `isAwake(BodyHandle handle)` | `Result<bool>` | Check whether a body is currently awake and being simulated |
| `wakeUp(BodyHandle handle)` | `Result<void>` | Wake up a sleeping body so it participates in simulation again |
| `putToSleep(BodyHandle handle)` | `Result<void>` | Force a body to sleep; it will not be simulated until woken |

### Bounds

| Method | Returns | Description |
|--------|---------|-------------|
| `getBodyBounds(BodyHandle handle)` | `Result<AABB3D>` | Return the axis-aligned bounding box enclosing all shapes on a body |

### Raycasting

| Method | Returns | Description |
|--------|---------|-------------|
| `raycast(const Ray3D& ray, float maxDistance, const QueryFilter3D& filter = {})` | `std::optional<RaycastHit3D>` | Cast a ray and return the closest hit filtered by the query filter |
| `raycastAll(const Ray3D& ray, float maxDistance, const QueryFilter3D& filter = {})` | `std::vector<RaycastHit3D>` | Cast a ray and return all hits sorted by distance |

### Shape Casting

| Method | Returns | Description |
|--------|---------|-------------|
| `sphereCast(Vec3 origin, float radius, Vec3 direction, float maxDistance, const QueryFilter3D& filter = {})` | `std::optional<ShapeCastHit3D>` | Sweep a sphere along a direction and return the closest hit |
| `boxCast(Vec3 origin, Vec3 halfExtents, Quat rotation, Vec3 direction, float maxDistance, const QueryFilter3D& filter = {})` | `std::optional<ShapeCastHit3D>` | Sweep an oriented box along a direction and return the closest hit |
| `capsuleCast(Vec3 origin, float radius, float halfHeight, Quat rotation, Vec3 direction, float maxDistance, const QueryFilter3D& filter = {})` | `std::optional<ShapeCastHit3D>` | Sweep a capsule along a direction and return the closest hit |

### Overlap Queries

| Method | Returns | Description |
|--------|---------|-------------|
| `overlapSphere(Vec3 center, float radius, const QueryFilter3D& filter = {})` | `std::vector<Entity>` | Return all entities whose bodies overlap a sphere region |
| `overlapBox(Vec3 center, Vec3 halfExtents, Quat rotation, const QueryFilter3D& filter = {})` | `std::vector<Entity>` | Return all entities whose bodies overlap an oriented box region |
| `queryAABB(Vec3 min, Vec3 max, const QueryFilter3D& filter = {})` | `std::vector<Entity>` | Return all entities whose body AABBs overlap the given axis-aligned box |

### Constraints

| Method | Returns | Description |
|--------|---------|-------------|
| `createConstraint(const ConstraintDef3D& def)` | `Result<ConstraintHandle>` | Create a constraint between two bodies and return its handle; the specific constraint type is determined by the def's type field |
| `destroyConstraint(ConstraintHandle handle)` | `Result<void>` | Destroy a constraint and release the connected bodies |
| `setConstraintEnabled(ConstraintHandle handle, bool enabled)` | `Result<void>` | Enable or disable a constraint without destroying it |
| `getConstraints(BodyHandle handle)` | `std::vector<ConstraintHandle>` | Return all constraints attached to a body |
| `setHingeLimits(ConstraintHandle handle, float minAngle, float maxAngle)` | `Result<void>` | Set the angular limits on a hinge constraint (radians) |
| `setHingeMotor(ConstraintHandle handle, float targetVelocity, float maxTorque)` | `Result<void>` | Attach or update a motor on a hinge constraint driving it toward a target angular velocity |
| `setSliderLimits(ConstraintHandle handle, float minDistance, float maxDistance)` | `Result<void>` | Set the linear limits on a slider constraint |
| `setSliderMotor(ConstraintHandle handle, float targetVelocity, float maxForce)` | `Result<void>` | Attach or update a motor on a slider constraint driving it toward a target linear velocity |
| `getConstraintForce(ConstraintHandle handle)` | `Result<float>` | Return the current force magnitude applied by a constraint; useful for breakable constraints |

### Contacts

| Method | Returns | Description |
|--------|---------|-------------|
| `getContacts(BodyHandle handle)` | `std::vector<ContactPair3D>` | Return all active contact pairs involving a body |
| `getAllContacts()` | `std::vector<ContactPair3D>` | Return all active contact pairs in the entire world |
| `areInContact(BodyHandle a, BodyHandle b)` | `bool` | Check whether two specific bodies are currently in contact |
| `getContactPair(BodyHandle a, BodyHandle b)` | `std::optional<ContactPair3D>` | Return the contact pair between two specific bodies, or nullopt if they are not in contact |

### Character Controller

| Method | Returns | Description |
|--------|---------|-------------|
| `createCharacter(Entity entity, const CharacterControllerDef& def)` | `Result<void>` | Create a character controller with capsule collision for an entity |
| `destroyCharacter(Entity entity)` | `Result<void>` | Destroy the character controller for an entity |
| `moveCharacter(Entity entity, Vec3 velocity, DeltaTime dt)` | `Result<void>` | Move a character controller by a velocity vector over the given timestep; handles slopes, steps, and collisions automatically |
| `getCharacterPosition(Entity entity)` | `Result<Vec3>` | Return the world-space position of a character controller |
| `setCharacterPosition(Entity entity, Vec3 position)` | `Result<void>` | Teleport a character controller to a new position |
| `getCharacterGroundInfo(Entity entity)` | `Result<CharacterGroundInfo>` | Return detailed ground state including ground normal, contact point, slope angle, and ground entity |
| `getCharacterVelocity(Entity entity)` | `Result<Vec3>` | Return the current velocity of a character controller |

### Vehicle

| Method | Returns | Description |
|--------|---------|-------------|
| `createVehicle(Entity entity, const VehicleDef& def)` | `Result<void>` | Create a vehicle with the specified wheel configuration for an entity |
| `destroyVehicle(Entity entity)` | `Result<void>` | Destroy the vehicle for an entity |
| `updateVehicle(Entity entity, float throttle, float steering, float brake)` | `Result<void>` | Update vehicle inputs; throttle and brake range from 0 to 1, steering from -1 (left) to 1 (right) |
| `getWheelTransform(Entity entity, uint32_t wheelIndex)` | `Result<WheelState>` | Return the full state of a wheel including its world transform, grounded status, and contact information |
| `isWheelGrounded(Entity entity, uint32_t wheelIndex)` | `Result<bool>` | Check whether a specific wheel is in contact with the ground |
| `getVehicleSpeed(Entity entity)` | `Result<float>` | Return the current forward speed of the vehicle in world units per second |

### World

| Method | Returns | Description |
|--------|---------|-------------|
| `setGravity(Vec3 gravity)` | `void` | Set the global gravity vector for the 3D physics world |
| `getGravity()` | `Vec3` | Return the current global gravity vector |

### Callbacks

| Method | Returns | Description |
|--------|---------|-------------|
| `subscribe(Physics3DEvent type, std::function<void(const EventData&)> callback)` | `SubscriptionId` | Subscribe to a physics event (Collision, TriggerEnter, TriggerExit) |
| `unsubscribe(SubscriptionId id)` | `void` | Remove a physics event subscription |

### Debug

| Method | Returns | Description |
|--------|---------|-------------|
| `setDebugDraw(bool enabled)` | `void` | Enable or disable physics debug wireframe rendering |
| `getDebugLines()` | `std::vector<DebugLine3D>` | Return all debug lines for the current frame; pass these to your renderer for visualization |

### Stats

| Method | Returns | Description |
|--------|---------|-------------|
| `getStats()` | `PhysicsStats3D` | Return simulation statistics for the current frame including body counts, constraint counts, and timing |

## Types

### BodyType3D

Body dynamics type controlling how the physics engine simulates the body.

```cpp
enum class BodyType3D : std::uint8_t {
    Static,       // Never moves; used for terrain, walls, and static geometry
    Kinematic,    // Moved programmatically via velocity; not affected by forces
    Dynamic       // Fully simulated; responds to gravity, forces, and collisions
};
```

| Value | Description |
|-------|-------------|
| `Static` | Immovable body with infinite mass; used for terrain, walls, and permanent level geometry |
| `Kinematic` | Moved programmatically via set velocity or position; pushes dynamic bodies but is not affected by them |
| `Dynamic` | Fully simulated body; responds to gravity, forces, impulses, and collisions with other bodies |

### ShapeType3D

Collision shape types available for physics bodies.

```cpp
enum class ShapeType3D : std::uint8_t {
    Box,          // Axis-aligned box defined by half-extents
    Sphere,       // Sphere defined by radius
    Capsule,      // Cylinder with hemisphere caps
    Cylinder,     // Cylinder along the Y axis
    Mesh,         // Triangle mesh (static bodies only)
    ConvexHull,   // Convex hull from a set of vertices
    HeightField,  // Terrain height map
    Compound      // Multiple sub-shapes combined
};
```

### MotionQuality

Controls the collision detection quality for a body.

```cpp
enum class MotionQuality : std::uint8_t {
    Discrete,    // Standard discrete collision detection; fast but can miss thin objects
    LinearCast   // Continuous collision detection; prevents tunneling for fast-moving bodies
};
```

### PhysicsBodyDef3D

Definition struct for creating a 3D physics body.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `type` | `BodyType3D` | `Dynamic` | Body dynamics type |
| `transform` | `Transform3D` | Identity | Initial world-space transform |
| `shapeType` | `ShapeType3D` | `Box` | Primary collision shape type |
| `shapeHalfExtents` | `Vec3` | `{0.5, 0.5, 0.5}` | Half-extents for box shapes |
| `shapeRadius` | `float` | `0.5f` | Radius for sphere, capsule, and cylinder shapes |
| `shapeHalfHeight` | `float` | `0.5f` | Half-height for capsule and cylinder shapes |
| `density` | `float` | `1000.0f` | Density in kg/m^3 for mass calculation |
| `friction` | `float` | `0.5f` | Surface friction coefficient |
| `restitution` | `float` | `0.3f` | Bounciness (0 = no bounce, 1 = perfect bounce) |
| `linearDamping` | `float` | `0.05f` | Linear velocity damping |
| `angularDamping` | `float` | `0.05f` | Angular velocity damping |
| `gravityFactor` | `float` | `1.0f` | Gravity multiplier for this body |
| `allowSleep` | `bool` | `true` | Whether the body can enter sleep state when at rest |
| `layer` | `CollisionLayer3D` | `0x0001` | Collision layer bitmask this body belongs to |
| `mask` | `CollisionMask3D` | `0xFFFF` | Collision mask controlling which layers this body interacts with |
| `isSensor` | `bool` | `false` | If true, detects overlaps without generating collision response |
| `motionQuality` | `MotionQuality` | `Discrete` | Collision detection quality; use LinearCast for fast-moving objects |
| `massProperties` | `std::optional<MassProperties>` | `nullopt` | Explicit mass properties; if nullopt, computed from density and shape |

### ShapeDef3D

Base shape definition with common properties shared by all shape types.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `type` | `ShapeType3D` | `Box` | Shape type discriminator |
| `density` | `float` | `1000.0f` | Density in kg/m^3 |
| `friction` | `float` | `0.5f` | Surface friction coefficient |
| `restitution` | `float` | `0.3f` | Bounciness |
| `localPosition` | `Vec3` | `{0, 0, 0}` | Offset from body origin in local space |
| `localRotation` | `Quat` | Identity | Rotation relative to body in local space |

### BoxShapeDef

Box collision shape extending ShapeDef3D.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `halfExtents` | `Vec3` | `{0.5, 0.5, 0.5}` | Half-width, half-height, and half-depth of the box |

### SphereShapeDef

Sphere collision shape extending ShapeDef3D.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `radius` | `float` | `0.5f` | Radius of the sphere |

### CapsuleShapeDef

Capsule collision shape (cylinder with hemisphere caps) extending ShapeDef3D.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `radius` | `float` | `0.5f` | Radius of the capsule cylinder and end caps |
| `halfHeight` | `float` | `0.5f` | Half-height of the cylindrical portion (total height = 2 * halfHeight + 2 * radius) |

### CylinderShapeDef

Cylinder collision shape extending ShapeDef3D.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `radius` | `float` | `0.5f` | Radius of the cylinder |
| `halfHeight` | `float` | `0.5f` | Half-height of the cylinder |

### MeshShapeDef

Triangle mesh collision shape extending ShapeDef3D. Only usable with static bodies.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `vertices` | `std::vector<Vec3>` | `{}` | Vertex positions of the triangle mesh |
| `indices` | `std::vector<uint32_t>` | `{}` | Triangle indices (3 indices per triangle) |

### ConvexHullShapeDef

Convex hull collision shape extending ShapeDef3D.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `vertices` | `std::vector<Vec3>` | `{}` | Point cloud from which the convex hull is computed |

### HeightFieldShapeDef

Height-field terrain shape extending ShapeDef3D.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `heights` | `std::vector<float>` | `{}` | Row-major grid of height values |
| `width` | `uint32_t` | `0` | Number of samples along the X axis |
| `length` | `uint32_t` | `0` | Number of samples along the Z axis |
| `scale` | `Vec3` | `{1, 1, 1}` | Scale applied to the height field (x = horizontal X, y = height, z = horizontal Z) |

### CompoundShapeDef

Compound shape combining multiple sub-shapes into a single body.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `boxes` | `std::vector<BoxShapeDef>` | `{}` | Box sub-shapes with individual local transforms |
| `spheres` | `std::vector<SphereShapeDef>` | `{}` | Sphere sub-shapes |
| `capsules` | `std::vector<CapsuleShapeDef>` | `{}` | Capsule sub-shapes |
| `cylinders` | `std::vector<CylinderShapeDef>` | `{}` | Cylinder sub-shapes |
| `convexHulls` | `std::vector<ConvexHullShapeDef>` | `{}` | Convex hull sub-shapes |

### MassProperties

Explicit mass configuration for a body.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `mass` | `float` | `1.0f` | Total mass in kilograms |
| `centerOfMass` | `Vec3` | `{0, 0, 0}` | Center of mass offset from body origin in local space |
| `inertiaTensor` | `Mat3` | Identity | 3x3 inertia tensor matrix |
| `autoCompute` | `bool` | `true` | If true, mass and inertia are computed from shapes and density; explicit values are ignored |

### ConstraintType3D

Types of constraints (joints) available between bodies.

```cpp
enum class ConstraintType3D : std::uint8_t {
    Fixed,      // Rigidly connects two bodies with no relative movement
    Point,      // Ball-and-socket allowing free rotation around a point
    Distance,   // Maintains a distance range between two anchor points
    Hinge,      // Allows rotation around a single axis with optional limits and motor
    Slider,     // Allows translation along a single axis with optional limits and motor
    Cone,       // Limits rotation to a cone-shaped region around a twist axis
    SixDOF      // Six degrees of freedom with per-axis linear and angular limits
};
```

### ConstraintDef3D

Base constraint definition shared by all constraint types.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `type` | `ConstraintType3D` | `Fixed` | The type of constraint to create |
| `bodyA` | `Entity` | -- | First body entity |
| `bodyB` | `Entity` | -- | Second body entity |
| `pivotA` | `Vec3` | `{0, 0, 0}` | Anchor point in body A's local space |
| `pivotB` | `Vec3` | `{0, 0, 0}` | Anchor point in body B's local space |
| `collideConnected` | `bool` | `false` | Whether the two connected bodies should collide with each other |

### HingeConstraintDef

Hinge constraint definition extending ConstraintDef3D.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `axisA` | `Vec3` | `{0, 1, 0}` | Hinge axis in body A's local space |
| `axisB` | `Vec3` | `{0, 1, 0}` | Hinge axis in body B's local space |
| `hasLimits` | `bool` | `false` | Whether angular limits are enforced |
| `minAngle` | `float` | `-pi` | Minimum rotation angle in radians |
| `maxAngle` | `float` | `pi` | Maximum rotation angle in radians |
| `hasMotor` | `bool` | `false` | Whether the motor is active |
| `motorTargetVelocity` | `float` | `0` | Target angular velocity in radians per second |
| `motorMaxTorque` | `float` | `0` | Maximum torque the motor can apply |

### SliderConstraintDef

Slider constraint definition extending ConstraintDef3D.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `axisA` | `Vec3` | `{1, 0, 0}` | Slide axis in body A's local space |
| `axisB` | `Vec3` | `{1, 0, 0}` | Slide axis in body B's local space |
| `hasLimits` | `bool` | `false` | Whether linear limits are enforced |
| `minDistance` | `float` | `-1.0f` | Minimum slide distance |
| `maxDistance` | `float` | `1.0f` | Maximum slide distance |
| `hasMotor` | `bool` | `false` | Whether the motor is active |
| `motorTargetVelocity` | `float` | `0` | Target linear velocity |
| `motorMaxForce` | `float` | `0` | Maximum force the motor can apply |

### ConeConstraintDef

Cone constraint definition extending ConstraintDef3D.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `twistAxisA` | `Vec3` | `{1, 0, 0}` | Twist axis in body A's local space |
| `twistAxisB` | `Vec3` | `{1, 0, 0}` | Twist axis in body B's local space |
| `halfConeAngle` | `float` | `0.785f` | Half-angle of the cone in radians (default ~45 degrees) |

### PointConstraintDef

Point (ball-and-socket) constraint definition extending ConstraintDef3D. No additional fields beyond the base `pivotA` and `pivotB`.

### DistanceConstraintDef

Distance constraint definition extending ConstraintDef3D.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `minDistance` | `float` | `0.0f` | Minimum allowed distance between anchor points |
| `maxDistance` | `float` | `1.0f` | Maximum allowed distance between anchor points |

### CharacterControllerDef

Definition struct for creating a character controller.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `radius` | `float` | `0.3f` | Radius of the character capsule |
| `height` | `float` | `1.8f` | Total height of the character capsule in world units |
| `stepHeight` | `float` | `0.35f` | Maximum step height the character can automatically climb |
| `maxSlopeAngle` | `float` | `45.0f` | Maximum slope angle in degrees the character can walk up |
| `mass` | `float` | `80.0f` | Mass of the character in kilograms |
| `layer` | `CollisionLayer3D` | `CollisionLayers3D::Character` | Collision layer for the character |
| `mask` | `CollisionMask3D` | `0xFFFF` | Collision mask controlling which layers the character interacts with |

### CharacterGroundInfo

Detailed ground state information for a character controller.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `state` | `CharacterGroundState` | `InAir` | The character's current ground state |
| `groundEntity` | `Entity` | `NullEntity` | The entity the character is standing on, if any |
| `groundNormal` | `Vec3` | `{0, 1, 0}` | Surface normal at the ground contact point |
| `groundPoint` | `Vec3` | `{0, 0, 0}` | World-space position of the ground contact |
| `slopeAngle` | `float` | `0.0f` | Angle of the ground surface from horizontal in degrees |

### CharacterGroundState

```cpp
enum class CharacterGroundState : std::uint8_t {
    OnGround,        // Standing on walkable ground within maxSlopeAngle
    OnSteepGround,   // On ground steeper than maxSlopeAngle
    InAir,           // Not touching any ground surface
    Sliding          // Sliding down a steep surface
};
```

### VehicleDef

Definition struct for creating a vehicle.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `wheels` | `std::vector<WheelDef>` | `{}` | Wheel definitions; typically 4 wheels for a car |
| `maxEngineForce` | `float` | `10000.0f` | Maximum engine force in Newtons |
| `maxBrakeForce` | `float` | `5000.0f` | Maximum braking force in Newtons |
| `maxSteeringAngle` | `float` | `0.5f` | Maximum steering angle in radians |

### WheelDef

Definition for a single vehicle wheel.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `connectionPoint` | `Vec3` | `{0, 0, 0}` | Wheel connection point relative to vehicle body origin |
| `suspensionDirection` | `Vec3` | `{0, -1, 0}` | Direction of suspension travel (typically straight down) |
| `suspensionLength` | `float` | `0.3f` | Maximum suspension travel distance |
| `suspensionStiffness` | `float` | `35.0f` | Spring stiffness of the suspension |
| `suspensionDamping` | `float` | `4.4f` | Damping coefficient of the suspension |
| `radius` | `float` | `0.4f` | Wheel radius |
| `friction` | `float` | `1.0f` | Tire friction coefficient |
| `isDriven` | `bool` | `false` | Whether engine torque is applied to this wheel |
| `isSteered` | `bool` | `false` | Whether this wheel responds to steering input |

### WheelState

Runtime state of a single vehicle wheel.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `transform` | `Transform3D` | Identity | World-space transform of the wheel including suspension compression and steering rotation |
| `grounded` | `bool` | `false` | Whether the wheel is in contact with the ground |
| `suspensionLength` | `float` | `0.0f` | Current suspension compression distance |
| `contactPoint` | `Vec3` | `{0, 0, 0}` | World-space contact point with the ground (valid only when grounded) |
| `contactNormal` | `Vec3` | `{0, 1, 0}` | Ground surface normal at the contact point |

### ContactPoint3D

A single contact point between two colliding bodies.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `worldPositionOnA` | `Vec3` | -- | Contact position on body A's surface in world space |
| `worldPositionOnB` | `Vec3` | -- | Contact position on body B's surface in world space |
| `worldNormalOnB` | `Vec3` | -- | Contact normal pointing from A toward B |
| `penetrationDepth` | `float` | -- | How deeply the two shapes are overlapping |
| `combinedFriction` | `float` | -- | Effective friction at this contact point |
| `combinedRestitution` | `float` | -- | Effective restitution at this contact point |

### ContactPair3D

A pair of bodies in contact with their contact manifold.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `entityA` | `Entity` | `NullEntity` | First entity in the contact pair |
| `entityB` | `Entity` | `NullEntity` | Second entity in the contact pair |
| `contacts` | `std::vector<ContactPoint3D>` | `{}` | All contact points in the manifold |
| `impulse` | `float` | `0.0f` | Total impulse applied to resolve the contact |
| `isActive` | `bool` | `false` | Whether this contact pair is currently active |

### QueryFilter3D

Filter parameters for spatial queries (raycast, overlap, shape cast).

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `layerMask` | `CollisionMask3D` | `0xFFFF` | Only include bodies on these collision layers |
| `ignoreEntity` | `std::optional<Entity>` | `nullopt` | An entity to exclude from query results |
| `ignoreSensors` | `bool` | `true` | Whether to skip sensor bodies in query results |

### RaycastHit3D

Result of a 3D raycast query.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `entity` | `Entity` | `NullEntity` | The entity associated with the hit body |
| `point` | `Vec3` | `{0, 0, 0}` | World-space position where the ray intersected the body |
| `normal` | `Vec3` | `{0, 0, 0}` | Surface normal at the intersection point |
| `distance` | `float` | `0.0f` | Distance from the ray origin to the hit point |
| `shapeIndex` | `uint32_t` | `0` | Index of the specific shape that was hit on a compound body |

### ShapeCastHit3D

Result of a shape cast query (sphere, box, or capsule sweep).

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `entity` | `Entity` | `NullEntity` | The entity associated with the hit body |
| `point` | `Vec3` | `{0, 0, 0}` | World-space contact point |
| `normal` | `Vec3` | `{0, 0, 0}` | Surface normal at the contact point |
| `distance` | `float` | `0.0f` | Distance the shape traveled before hitting |
| `penetrationDepth` | `Vec3` | `{0, 0, 0}` | Penetration depth vector if the shapes are initially overlapping |

### PhysicsStats3D

Simulation statistics for profiling and debugging.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `activeBodies` | `uint32_t` | `0` | Number of bodies currently awake and being simulated |
| `sleepingBodies` | `uint32_t` | `0` | Number of bodies currently sleeping |
| `constraints` | `uint32_t` | `0` | Number of active constraints |
| `characters` | `uint32_t` | `0` | Number of active character controllers |
| `vehicles` | `uint32_t` | `0` | Number of active vehicles |
| `updateTimeMs` | `float` | `0.0f` | Time spent in the physics update step in milliseconds |
| `collisionPairs` | `uint32_t` | `0` | Number of collision pairs processed this frame |

### CollisionLayers3D

Predefined collision layer constants for common use cases.

```cpp
namespace CollisionLayers3D {
    inline constexpr CollisionLayer3D Default    = 0x0001;
    inline constexpr CollisionLayer3D Static     = 0x0002;
    inline constexpr CollisionLayer3D Dynamic    = 0x0004;
    inline constexpr CollisionLayer3D Character  = 0x0008;
    inline constexpr CollisionLayer3D Projectile = 0x0010;
    inline constexpr CollisionLayer3D Trigger    = 0x0020;
    inline constexpr CollisionLayer3D Debris     = 0x0040;
    inline constexpr CollisionLayer3D Vehicle    = 0x0080;
}
```

| Constant | Value | Description |
|----------|-------|-------------|
| `Default` | `0x0001` | General-purpose default layer |
| `Static` | `0x0002` | Static world geometry (terrain, buildings, walls) |
| `Dynamic` | `0x0004` | Dynamic objects (crates, barrels, physics props) |
| `Character` | `0x0008` | Player and NPC character controllers |
| `Projectile` | `0x0010` | Bullets, arrows, thrown objects |
| `Trigger` | `0x0020` | Trigger volumes (sensor bodies) |
| `Debris` | `0x0040` | Visual debris that may not interact with everything |
| `Vehicle` | `0x0080` | Vehicle bodies and wheel colliders |

## Lua Mapping

All `Result<T>` methods map to Lua two-value returns `(value, err)`. On success, `err` is `nil`. On failure, `value` is `nil` and `err` is a table with `category`, `message`, and `system` fields. Infallible methods return values directly. Enums map to strings (e.g., `"Dynamic"`, `"Static"`). Vec3 maps to a table `{x, y, z}`. Quat maps to `{x, y, z, w}`. Structs map to Lua tables with matching field names. The `QueryFilter3D` defaults are applied automatically when not provided.

High-level functions are accessed via `bestow.physics3d.<method>`. Low-level functions are accessed via `bestow.physics3d.core.<method>`.

## Examples

### Lua

```lua
-- High-level: Add a dynamic box body
bestow.physics3d.addBody(crate, "Box", { x = 0.5, y = 0.5, z = 0.5 })

-- High-level: Velocity and forces
bestow.physics3d.setVelocity(crate, { x = 0, y = 10, z = 0 })
local vel, err = bestow.physics3d.getVelocity(crate)
bestow.physics3d.applyForce(crate, { x = 100, y = 0, z = 0 })
bestow.physics3d.applyImpulse(crate, { x = 0, y = 50, z = 0 })

-- High-level: Gravity
bestow.physics3d.setGravity({ x = 0, y = -9.81, z = 0 })
local gravity = bestow.physics3d.getGravity()

-- High-level: Raycast
local hit = bestow.physics3d.raycast(
    { x = 0, y = 10, z = 0 },
    { x = 0, y = -1, z = 0 },
    100
)
if hit then
    print("Hit entity at distance: " .. hit.distance)
end

-- High-level: Overlap sphere
local nearby = bestow.physics3d.overlapSphere({ x = 0, y = 5, z = 0 }, 10)
for _, entity in ipairs(nearby) do
    print("Entity in range: " .. tostring(entity))
end

-- Low-level: Create body with full definition
local body, err = bestow.physics3d.core.createBody(crate, {
    type = "Dynamic",
    transform = {
        position = { x = 0, y = 5, z = 0 },
        rotation = { x = 0, y = 0, z = 0, w = 1 }
    },
    shapeType = "Box",
    shapeHalfExtents = { x = 0.5, y = 0.5, z = 0.5 },
    density = 1000,
    friction = 0.5,
    restitution = 0.3,
    motionQuality = "LinearCast"
})

-- Low-level: Transform and velocity
bestow.physics3d.core.setPosition(body, { x = 10, y = 5, z = 0 })
bestow.physics3d.core.setRotation(body, { x = 0, y = 0.707, z = 0, w = 0.707 })
bestow.physics3d.core.setLinearVelocity(body, { x = 5, y = 0, z = 0 })
bestow.physics3d.core.setAngularVelocity(body, { x = 0, y = 1, z = 0 })

-- Low-level: Forces at points
bestow.physics3d.core.applyForceAtPoint(body,
    { x = 100, y = 0, z = 0 },
    { x = 1, y = 2, z = 0 })
bestow.physics3d.core.applyTorque(body, { x = 0, y = 10, z = 0 })
bestow.physics3d.core.applyAngularImpulse(body, { x = 0, y = 5, z = 0 })

-- Low-level: Properties
bestow.physics3d.core.setMass(body, 50)
bestow.physics3d.core.setLinearDamping(body, 0.1)
bestow.physics3d.core.setGravityFactor(body, 0.5)
bestow.physics3d.core.setFriction(body, 0.8)
bestow.physics3d.core.setRestitution(body, 0.1)

-- Low-level: CCD for fast-moving objects
bestow.physics3d.core.setMotionQuality(body, "LinearCast")

-- Low-level: Collision filtering
bestow.physics3d.core.setCollisionLayer(body, 0x0004)  -- Dynamic layer
bestow.physics3d.core.setCollisionMask(body, 0x000F)   -- Interact with Default + Static + Dynamic + Character
bestow.physics3d.core.setSensor(body, false)

-- Low-level: Sleep management
if bestow.physics3d.core.isAwake(body) then
    bestow.physics3d.core.putToSleep(body)
end
bestow.physics3d.core.wakeUp(body)

-- Low-level: Compound shapes
local compound, err = bestow.physics3d.core.createCompoundBody(robot, "Dynamic",
    { position = { x = 0, y = 2, z = 0 } },
    {
        boxes = {
            { halfExtents = { x = 0.5, y = 1.0, z = 0.3 }, localPosition = { x = 0, y = 0, z = 0 } }
        },
        spheres = {
            { radius = 0.4, localPosition = { x = 0, y = 1.2, z = 0 } }
        }
    })
bestow.physics3d.core.addShape(compound, { type = "Capsule", radius = 0.2, halfHeight = 0.5 })

-- Low-level: Height field terrain
bestow.physics3d.core.createHeightFieldBody(terrain,
    { position = { x = 0, y = 0, z = 0 } },
    { heights = heightData, width = 256, length = 256, scale = { x = 1, y = 50, z = 1 } })

-- Low-level: Constraints
local hinge, err = bestow.physics3d.core.createConstraint({
    type = "Hinge",
    bodyA = doorEntity,
    bodyB = frameEntity,
    pivotA = { x = -0.5, y = 0, z = 0 },
    pivotB = { x = 0.5, y = 0, z = 0 },
    axisA = { x = 0, y = 1, z = 0 },
    axisB = { x = 0, y = 1, z = 0 },
    hasLimits = true,
    minAngle = -1.57,
    maxAngle = 1.57
})
bestow.physics3d.core.setHingeMotor(hinge, 2.0, 100)

-- Low-level: Contacts
local contacts = bestow.physics3d.core.getContacts(body)
for _, pair in ipairs(contacts) do
    print("Contact with impulse: " .. pair.impulse)
end
local touching = bestow.physics3d.core.areInContact(bodyA, bodyB)

-- Low-level: Character controller
bestow.physics3d.core.createCharacter(player, {
    radius = 0.3,
    height = 1.8,
    mass = 80,
    maxSlopeAngle = 45,
    stepHeight = 0.35
})
bestow.physics3d.core.moveCharacter(player, { x = dx, y = 0, z = dz }, dt)
local info = bestow.physics3d.core.getCharacterGroundInfo(player)
if info.state == "OnGround" then
    -- allow jump
end

-- Low-level: Vehicle
bestow.physics3d.core.createVehicle(car, {
    wheels = {
        { connectionPoint = { x = -0.8, y = 0, z = 1.2 }, isDriven = true, isSteered = true },
        { connectionPoint = { x = 0.8, y = 0, z = 1.2 }, isDriven = true, isSteered = true },
        { connectionPoint = { x = -0.8, y = 0, z = -1.2 }, isDriven = false, isSteered = false },
        { connectionPoint = { x = 0.8, y = 0, z = -1.2 }, isDriven = false, isSteered = false }
    },
    maxEngineForce = 15000,
    maxBrakeForce = 8000,
    maxSteeringAngle = 0.5
})
bestow.physics3d.core.updateVehicle(car, throttle, steering, brake)
local speed = bestow.physics3d.core.getVehicleSpeed(car)
local wheelState = bestow.physics3d.core.getWheelTransform(car, 0)

-- Low-level: Spatial queries with filters
local allHits = bestow.physics3d.core.raycastAll(
    { origin = { x = 0, y = 10, z = 0 }, direction = { x = 0, y = -1, z = 0 } },
    100,
    { layerMask = 0x000F, ignoreSensors = true })
local sphereHit = bestow.physics3d.core.sphereCast(
    { x = 0, y = 2, z = 0 }, 0.5, { x = 1, y = 0, z = 0 }, 50)
local boxHit = bestow.physics3d.core.boxCast(
    { x = 0, y = 1, z = 0 }, { x = 1, y = 1, z = 1 }, { x = 0, y = 0, z = 0, w = 1 },
    { x = 0, y = -1, z = 0 }, 20)
local inBox = bestow.physics3d.core.overlapBox(
    { x = 0, y = 5, z = 0 }, { x = 5, y = 5, z = 5 },
    { x = 0, y = 0, z = 0, w = 1 })
local inAABB = bestow.physics3d.core.queryAABB(
    { x = -10, y = 0, z = -10 }, { x = 10, y = 20, z = 10 })

-- Low-level: Callbacks
local subId = bestow.physics3d.core.subscribe("Collision", function(event)
    print("3D collision: impulse = " .. event.impulse)
end)
bestow.physics3d.core.unsubscribe(subId)

-- Low-level: Debug and stats
bestow.physics3d.core.setDebugDraw(true)
local stats = bestow.physics3d.core.getStats()
print("Active bodies: " .. stats.activeBodies .. ", Update: " .. stats.updateTimeMs .. "ms")
```

### C++

```cpp
// High-level: Entity-centric operations
physics3D->addBody(crate, "Box", Vec3{0.5f, 0.5f, 0.5f});
physics3D->setVelocity(crate, {0, 10, 0});
physics3D->applyImpulse(crate, {0, 50, 0});
physics3D->setGravity({0, -9.81f, 0});

auto hit = physics3D->raycast({0, 10, 0}, {0, -1, 0}, 100);
if (hit) {
    handleHit(hit->entity, hit->point, hit->distance);
}

auto nearby = physics3D->overlapSphere({0, 5, 0}, 10.0f);
for (Entity e : nearby) {
    applyAreaEffect(e);
}

// Low-level: Full body creation
auto bodyResult = physics3DCore->createBody(crate, PhysicsBodyDef3D{
    .type = BodyType3D::Dynamic,
    .transform = {.position = {0, 5, 0}},
    .shapeType = ShapeType3D::Box,
    .shapeHalfExtents = {0.5f, 0.5f, 0.5f},
    .density = 1000.0f,
    .motionQuality = MotionQuality::LinearCast
});
if (!bodyResult) {
    spdlog::error("Failed: {}", bodyResult.error().message);
    return;
}
BodyHandle body = *bodyResult;

// Low-level: Transform and velocity
physics3DCore->setTransform(body, Transform3D{
    .position = {10, 5, 0},
    .rotation = glm::angleAxis(glm::radians(45.0f), Vec3{0, 1, 0})
});
physics3DCore->setLinearVelocity(body, {5, 0, 0});
physics3DCore->setAngularVelocity(body, {0, 1, 0});

// Low-level: Forces at specific points
physics3DCore->applyForceAtPoint(body, {100, 0, 0}, {1, 2, 0});
physics3DCore->applyTorque(body, {0, 10, 0});
physics3DCore->applyAngularImpulse(body, {0, 5, 0});

// Low-level: Mass properties
physics3DCore->setMassProperties(body, MassProperties{
    .mass = 50.0f,
    .centerOfMass = {0, 0.5f, 0},
    .autoCompute = false
});
auto massProp = physics3DCore->getMassProperties(body);
auto com = physics3DCore->getCenterOfMass(body);
auto inertia = physics3DCore->getInertiaTensor(body);

// Low-level: Compound shapes
auto compoundResult = physics3DCore->createCompoundBody(robot, BodyType3D::Dynamic,
    Transform3D{.position = {0, 2, 0}},
    CompoundShapeDef{
        .boxes = {{.halfExtents = {0.5f, 1.0f, 0.3f}}},
        .spheres = {{.radius = 0.4f, .localPosition = {0, 1.2f, 0}}}
    });

// Low-level: Height field terrain
physics3DCore->createHeightFieldBody(terrain, Transform3D{},
    HeightFieldShapeDef{
        .heights = heightData,
        .width = 256,
        .length = 256,
        .scale = {1, 50, 1}
    });

// Low-level: Constraints with motors
auto hingeResult = physics3DCore->createConstraint(HingeConstraintDef{
    .bodyA = doorEntity,
    .bodyB = frameEntity,
    .pivotA = {-0.5f, 0, 0},
    .pivotB = {0.5f, 0, 0},
    .axisA = {0, 1, 0},
    .axisB = {0, 1, 0},
    .hasLimits = true,
    .minAngle = -1.57f,
    .maxAngle = 1.57f
});
physics3DCore->setHingeMotor(*hingeResult, 2.0f, 100.0f);
float force = *physics3DCore->getConstraintForce(*hingeResult);

// Low-level: Contact queries
auto contacts = physics3DCore->getContacts(body);
bool touching = physics3DCore->areInContact(bodyA, bodyB);
auto pair = physics3DCore->getContactPair(bodyA, bodyB);

// Low-level: Character controller
physics3DCore->createCharacter(player, CharacterControllerDef{
    .radius = 0.3f, .height = 1.8f, .mass = 80.0f,
    .maxSlopeAngle = 45.0f, .stepHeight = 0.35f
});
physics3DCore->moveCharacter(player, moveDir * speed, dt);
auto groundInfo = physics3DCore->getCharacterGroundInfo(player);
if (groundInfo->state == CharacterGroundState::OnGround) {
    // allow jump
}

// Low-level: Vehicle simulation
physics3DCore->createVehicle(car, VehicleDef{
    .wheels = {
        {.connectionPoint = {-0.8f, 0, 1.2f}, .isDriven = true, .isSteered = true},
        {.connectionPoint = {0.8f, 0, 1.2f}, .isDriven = true, .isSteered = true},
        {.connectionPoint = {-0.8f, 0, -1.2f}},
        {.connectionPoint = {0.8f, 0, -1.2f}}
    },
    .maxEngineForce = 15000.0f
});
physics3DCore->updateVehicle(car, throttle, steering, brake);
float speed = *physics3DCore->getVehicleSpeed(car);

// Low-level: Shape casting
auto sphereHit = physics3DCore->sphereCast({0, 2, 0}, 0.5f, {1, 0, 0}, 50.0f);
auto boxHit = physics3DCore->boxCast({0, 1, 0}, {1, 1, 1}, Quat{}, {0, -1, 0}, 20.0f);
auto capsuleHit = physics3DCore->capsuleCast({0, 2, 0}, 0.3f, 0.9f, Quat{}, {0, -1, 0}, 10.0f);

// Low-level: Overlap queries with filters
auto inSphere = physics3DCore->overlapSphere({0, 0, 0}, 10.0f,
    QueryFilter3D{.layerMask = 0x000F, .ignoreSensors = true});
auto inBox = physics3DCore->overlapBox({0, 5, 0}, {5, 5, 5}, Quat{});
auto inAABB = physics3DCore->queryAABB({-10, 0, -10}, {10, 20, 10});

// Low-level: Callbacks
auto subId = physics3DCore->subscribe(Physics3DEvent::Collision,
    [](const EventData& data) {
        auto& e = std::get<CollisionEvent3D>(data);
        handleCollision(e.entityA, e.entityB, e.impulse);
    });
physics3DCore->unsubscribe(subId);

// Low-level: Debug and stats
physics3DCore->setDebugDraw(true);
auto lines = physics3DCore->getDebugLines();
auto stats = physics3DCore->getStats();
spdlog::info("Active: {}, Sleeping: {}, Update: {:.2f}ms",
    stats.activeBodies, stats.sleepingBodies, stats.updateTimeMs);
```
