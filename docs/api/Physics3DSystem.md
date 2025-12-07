# Physics3DSystem API

The `Physics3DSystem` provides 3D physics simulation using Jolt Physics.

## Overview

```cpp
auto& physics3D = sys.physics3D;

// Create physics body
PhysicsBodyDef3D def{
    .type = BodyType3D::Dynamic,
    .transform = Transform3D{.position = Vec3{0.0f, 5.0f, 0.0f}},
    .shapeType = ShapeType3D::Capsule,
    .shapeRadius = 0.5f,
    .shapeHalfHeight = 0.9f,
    .density = 1000.0f,
    .friction = 0.5f
};
physics3D->createBody(playerEntity, def);

// Update simulation
physics3D->update(dt, 4);  // 4 substeps for stability

// Synchronize transforms
std::vector<Entity> entities = physics3D->getAllBodies();
physics3D->syncTransforms(entities);

// Apply forces
physics3D->applyImpulse(playerEntity, Vec3{0.0f, 5.0f, 0.0f});

// Raycast
auto hit = physics3D->raycast(origin, direction, 100.0f);
if (hit) {
    // Hit something
}
```

## Body Management

### createBody(Entity entity, const PhysicsBodyDef3D& def)

```cpp
Result<void, Physics3DError> createBody(Entity entity, const PhysicsBodyDef3D& def);
```

Creates a physics body and attaches it to an entity.

**Parameters:**
- `entity`: Entity to attach body to
- `def`: Body definition

**Example:**

```cpp
PhysicsBodyDef3D sphereDef{
    .type = BodyType3D::Dynamic,
    .transform = Transform3D{.position = Vec3{0.0f, 5.0f, 0.0f}},
    .shapeType = ShapeType3D::Sphere,
    .shapeRadius = 0.5f,
    .density = 1000.0f,
    .restitution = 0.8f  // Bouncy
};
auto result = physics3D->createBody(ball, sphereDef);
```

---

### PhysicsBodyDef3D

```cpp
struct PhysicsBodyDef3D {
    BodyType3D type = BodyType3D::Dynamic;
    Transform3D transform;
    ShapeType3D shapeType = ShapeType3D::Box;
    Vec3 shapeHalfExtents{0.5f};     // For Box
    float shapeRadius = 0.5f;         // For Sphere/Capsule/Cylinder
    float shapeHalfHeight = 0.5f;     // For Capsule/Cylinder
    float density = 1000.0f;          // kg/m³
    float friction = 0.5f;            // Surface friction (0-1+)
    float restitution = 0.3f;         // Bounciness (0-1)
    float linearDamping = 0.05f;      // Velocity decay
    float angularDamping = 0.05f;     // Rotation decay
    float gravityFactor = 1.0f;       // Gravity multiplier
    bool allowSleep = true;           // Sleep when inactive
    CollisionLayer3D layer = 0x0001;  // Collision layer
    CollisionMask3D mask = 0xFFFF;    // What to collide with
    bool isSensor = false;            // Trigger volume
    MotionQuality motionQuality = MotionQuality::Discrete;  // CCD setting
    std::optional<MassProperties> massProperties;  // Override mass
};
```

**Body Types:**
- `BodyType3D::Static` - Immovable (terrain, walls)
- `BodyType3D::Kinematic` - Moved manually (moving platforms)
- `BodyType3D::Dynamic` - Affected by physics (player, objects)

**Shape Types:**
- `ShapeType3D::Box` - Box collision
- `ShapeType3D::Sphere` - Sphere collision
- `ShapeType3D::Capsule` - Capsule (best for characters)
- `ShapeType3D::Cylinder` - Cylinder collision
- `ShapeType3D::Mesh` - Triangle mesh (static only)
- `ShapeType3D::ConvexHull` - Convex mesh
- `ShapeType3D::HeightField` - Terrain heightmap
- `ShapeType3D::Compound` - Multiple shapes

---

### destroyBody(Entity entity)

```cpp
Result<void, Physics3DError> destroyBody(Entity entity);
```

Destroys the physics body attached to an entity.

---

### hasBody(Entity entity)

```cpp
bool hasBody(Entity entity) const;
```

Checks if an entity has a physics body.

---

## Transform

### setTransform(Entity entity, const Transform3D& transform)

```cpp
Result<void, Physics3DError> setTransform(Entity entity, const Transform3D& transform);
Result<Transform3D, Physics3DError> getTransform(Entity entity) const;
```

Gets or sets the body's transform (position and rotation).

**Example:**

```cpp
Transform3D t{
    .position = Vec3{10.0f, 5.0f, 0.0f},
    .rotation = Quat{0.707f, 0.0f, 0.707f, 0.0f}  // 90° around Y
};
physics3D->setTransform(entity, t);
```

---

### Position

```cpp
Result<void, Physics3DError> setPosition(Entity entity, Vec3 position);
Result<Vec3, Physics3DError> getPosition(Entity entity) const;
```

Gets or sets the body's position in world space.

**Example:**

```cpp
auto posResult = physics3D->getPosition(player);
if (posResult) {
    Vec3 pos = posResult.value();
    physics3D->setPosition(enemy, pos + Vec3{5.0f, 0.0f, 0.0f});
}
```

---

### Rotation

```cpp
Result<void, Physics3DError> setRotation(Entity entity, Quat rotation);
Result<Quat, Physics3DError> getRotation(Entity entity) const;
```

Gets or sets the body's rotation as a quaternion.

---

## Velocity

### Linear Velocity

```cpp
Result<void, Physics3DError> setLinearVelocity(Entity entity, Vec3 velocity);
Result<Vec3, Physics3DError> getLinearVelocity(Entity entity) const;
```

Gets or sets the body's linear velocity (m/s).

**Example:**

```cpp
// Launch upward
physics3D->setLinearVelocity(projectile, Vec3{0.0f, 20.0f, 0.0f});

// Stop horizontal movement
auto vel = physics3D->getLinearVelocity(player).value();
physics3D->setLinearVelocity(player, Vec3{0.0f, vel.y, 0.0f});
```

---

### Angular Velocity

```cpp
Result<void, Physics3DError> setAngularVelocity(Entity entity, Vec3 velocity);
Result<Vec3, Physics3DError> getAngularVelocity(Entity entity) const;
```

Gets or sets the body's rotation speed (axis-angle, magnitude in rad/s).

---

## Forces and Impulses

### applyForce(Entity entity, Vec3 force)

```cpp
Result<void, Physics3DError> applyForce(Entity entity, Vec3 force);
Result<void, Physics3DError> applyForceAtPoint(Entity entity, Vec3 force, Vec3 worldPoint);
```

Applies a continuous force. Force accumulates and is cleared after physics step.

**Use for:** Continuous acceleration (thrusters, wind)

**Example:**

```cpp
// Jetpack thrust
if (input->isActionActive("Thrust")) {
    physics3D->applyForce(player, Vec3{0.0f, -1000.0f, 0.0f});
}

// Apply force at edge (causes rotation)
Vec3 worldPoint = physics3D->getPosition(crate).value() + Vec3{0.5f, 0.0f, 0.0f};
physics3D->applyForceAtPoint(crate, Vec3{0.0f, 0.0f, 500.0f}, worldPoint);
```

---

### applyImpulse(Entity entity, Vec3 impulse)

```cpp
Result<void, Physics3DError> applyImpulse(Entity entity, Vec3 impulse);
Result<void, Physics3DError> applyImpulseAtPoint(Entity entity, Vec3 impulse, Vec3 worldPoint);
```

Applies an instant velocity change.

**Use for:** Instant effects (jumping, explosions, knockback)

**Example:**

```cpp
// Jump
physics3D->applyImpulse(player, Vec3{0.0f, 5.0f, 0.0f});

// Explosion knockback
Vec3 direction = glm::normalize(targetPos - explosionPos);
physics3D->applyImpulse(target, direction * 10.0f);
```

---

### applyTorque(Entity entity, Vec3 torque)

```cpp
Result<void, Physics3DError> applyTorque(Entity entity, Vec3 torque);
Result<void, Physics3DError> applyAngularImpulse(Entity entity, Vec3 impulse);
```

Applies rotational force or impulse.

---

## Body Properties

### Mass

```cpp
Result<void, Physics3DError> setMass(Entity entity, float mass);
Result<float, Physics3DError> getMass(Entity entity) const;
```

Gets or sets body mass (kg). By default, mass is computed from density and volume.

---

### Damping

```cpp
Result<void, Physics3DError> setLinearDamping(Entity entity, float damping);
Result<void, Physics3DError> setAngularDamping(Entity entity, float damping);
```

Sets velocity/rotation decay over time (0 = no damping, higher = more damping).

---

### Gravity Factor

```cpp
Result<void, Physics3DError> setGravityFactor(Entity entity, float factor);
```

Multiplier for world gravity (0 = no gravity, 1 = normal, 2 = double gravity).

**Example:**

```cpp
// Floating platform (no gravity)
physics3D->setGravityFactor(platform, 0.0f);

// Heavy object
physics3D->setGravityFactor(boulder, 2.0f);
```

---

### Friction and Restitution

```cpp
Result<void, Physics3DError> setFriction(Entity entity, float friction);
Result<void, Physics3DError> setRestitution(Entity entity, float restitution);
```

**Friction:** Surface friction (0 = ice, 1+ = sticky)
**Restitution:** Bounciness (0 = no bounce, 1 = perfect bounce)

---

## Collision Filtering

### setCollisionLayer(Entity entity, CollisionLayer3D layer)

```cpp
Result<void, Physics3DError> setCollisionLayer(Entity entity, CollisionLayer3D layer);
```

Sets which collision layer this body belongs to.

**Predefined Layers:**

```cpp
namespace CollisionLayers3D {
    inline constexpr CollisionLayer3D World     = 0x0001;
    inline constexpr CollisionLayer3D Character = 0x0002;
    inline constexpr CollisionLayer3D Dynamic   = 0x0004;
    inline constexpr CollisionLayer3D Trigger   = 0x0008;
    inline constexpr CollisionLayer3D Debris    = 0x0010;
    inline constexpr CollisionLayer3D Vehicle   = 0x0020;
}
```

**Example:**

```cpp
physics3D->setCollisionLayer(player, CollisionLayers3D::Character);
physics3D->setCollisionLayer(enemy, CollisionLayers3D::Dynamic);
```

---

### setCollisionMask(Entity entity, CollisionMask3D mask)

```cpp
Result<void, Physics3DError> setCollisionMask(Entity entity, CollisionMask3D mask);
```

Sets which layers this body can collide with (bitwise OR).

**Example:**

```cpp
// Character collides with world and dynamic objects
physics3D->setCollisionMask(player,
    CollisionLayers3D::World | CollisionLayers3D::Dynamic
);

// Projectile only hits characters and world
physics3D->setCollisionMask(bullet,
    CollisionLayers3D::Character | CollisionLayers3D::World
);
```

---

### setSensor(Entity entity, bool isSensor)

```cpp
Result<void, Physics3DError> setSensor(Entity entity, bool isSensor);
```

Makes a body a sensor (trigger volume). Sensors detect overlaps without collision response.

**Example:**

```cpp
// Create trigger zone
physics3D->createBody(checkpoint, checkpointDef);
physics3D->setSensor(checkpoint, true);
```

---

## Raycasting

### raycast(Vec3 origin, Vec3 direction, float maxDistance, const QueryFilter3D& filter)

```cpp
std::optional<RaycastHit3D> raycast(
    Vec3 origin,
    Vec3 direction,
    float maxDistance,
    const QueryFilter3D& filter = {}
) const;
```

Casts a ray and returns the first hit.

**Returns:** `RaycastHit3D` if hit, `std::nullopt` if no hit

```cpp
struct RaycastHit3D {
    Entity entity;
    Vec3 point;
    Vec3 normal;
    float distance;
};
```

**Example:**

```cpp
// Check if player can see enemy
Vec3 toEnemy = enemyPos - playerPos;
Vec3 direction = glm::normalize(toEnemy);
float distance = glm::length(toEnemy);

QueryFilter3D filter{
    .layerMask = CollisionLayers3D::World,
    .ignoreEntity = player,
    .ignoreSensors = true
};

auto hit = physics3D->raycast(playerPos, direction, distance, filter);
if (!hit || hit->entity == enemy) {
    // Line of sight clear
}
```

---

### raycastAll(Vec3 origin, Vec3 direction, float maxDistance, const QueryFilter3D& filter)

```cpp
std::vector<RaycastHit3D> raycastAll(
    Vec3 origin,
    Vec3 direction,
    float maxDistance,
    const QueryFilter3D& filter = {}
) const;
```

Casts a ray and returns all hits in order (closest first).

**Example:**

```cpp
// Piercing shot - hit all enemies in line
auto hits = physics3D->raycastAll(origin, direction, 50.0f);
for (const auto& hit : hits) {
    if (entities->allOf<Enemy>(hit.entity)) {
        damageEnemy(hit.entity, 25);
    }
}
```

---

## Shape Casting

### sphereCast / boxCast / capsuleCast

```cpp
std::optional<ShapeCastHit3D> sphereCast(
    Vec3 origin, float radius, Vec3 direction, float maxDistance,
    const QueryFilter3D& filter = {}
) const;

std::optional<ShapeCastHit3D> boxCast(
    Vec3 origin, Vec3 halfExtents, Quat rotation, Vec3 direction, float maxDistance,
    const QueryFilter3D& filter = {}
) const;

std::optional<ShapeCastHit3D> capsuleCast(
    Vec3 origin, float radius, float halfHeight, Quat rotation, Vec3 direction,
    float maxDistance, const QueryFilter3D& filter = {}
) const;
```

Casts a shape through space to predict collisions.

**Example:**

```cpp
// Check if character can move forward
Vec3 charPos = physics3D->getPosition(character).value();
Vec3 moveDir = getMovementDirection();

auto hit = physics3D->capsuleCast(
    charPos, 0.4f, 0.8f, Quat{1,0,0,0}, moveDir, 2.0f,
    QueryFilter3D{.ignoreEntity = character}
);

if (hit) {
    // Can't move full distance - limit to safe distance
    float safeDistance = std::max(0.0f, hit->distance - 0.1f);
}
```

---

## Overlap Queries

### overlapSphere(Vec3 center, float radius, const QueryFilter3D& filter)

```cpp
std::vector<Entity> overlapSphere(Vec3 center, float radius,
                                   const QueryFilter3D& filter = {}) const;
```

Returns all bodies overlapping a sphere.

**Example:**

```cpp
// Explosion damage
Vec3 explosionPos{0.0f, 1.0f, 0.0f};
float radius = 5.0f;

auto affected = physics3D->overlapSphere(explosionPos, radius);
for (Entity entity : affected) {
    Vec3 pos = physics3D->getPosition(entity).value();
    Vec3 dir = glm::normalize(pos - explosionPos);
    float dist = glm::length(pos - explosionPos);
    float force = 1000.0f * (1.0f - dist / radius);

    physics3D->applyImpulse(entity, dir * force);
    applyDamage(entity, 50 * (1.0f - dist / radius));
}
```

---

### overlapBox(Vec3 center, Vec3 halfExtents, Quat rotation, const QueryFilter3D& filter)

```cpp
std::vector<Entity> overlapBox(Vec3 center, Vec3 halfExtents, Quat rotation,
                                const QueryFilter3D& filter = {}) const;
```

Returns all bodies overlapping an oriented box.

---

### queryAABB(Vec3 min, Vec3 max, const QueryFilter3D& filter)

```cpp
std::vector<Entity> queryAABB(Vec3 min, Vec3 max,
                               const QueryFilter3D& filter = {}) const;
```

Returns all bodies overlapping an axis-aligned bounding box.

**Example:**

```cpp
// Find all entities in a region
Vec3 min{-10.0f, 0.0f, -10.0f};
Vec3 max{10.0f, 5.0f, 10.0f};

auto entities = physics3D->queryAABB(min, max);
```

---

## Contact Queries

### getContacts(Entity entity)

```cpp
std::vector<ContactPair3D> getContacts(Entity entity) const;
```

Returns all active contact pairs for an entity.

**Example:**

```cpp
auto contacts = physics3D->getContacts(player);
for (const auto& pair : contacts) {
    Entity other = (pair.entityA == player) ? pair.entityB : pair.entityA;

    for (const auto& contact : pair.contacts) {
        Vec3 point = contact.worldPositionOnA;
        Vec3 normal = contact.worldNormalOnB;
        float depth = contact.penetrationDepth;
    }
}
```

---

### areInContact(Entity a, Entity b)

```cpp
bool areInContact(Entity a, Entity b) const;
```

Checks if two entities are currently touching.

**Example:**

```cpp
if (physics3D->areInContact(playerFeet, ground)) {
    // Player is on ground
}
```

---

## Character Controller

### createCharacter(Entity entity, const CharacterControllerDef& def)

```cpp
Result<void, Physics3DError> createCharacter(Entity entity,
                                              const CharacterControllerDef& def);
```

Creates a kinematic character controller with step climbing and slope handling.

**Example:**

```cpp
CharacterControllerDef charDef{
    .radius = 0.3f,
    .height = 1.8f,
    .stepHeight = 0.35f,     // Can climb 35cm steps
    .maxSlopeAngle = 45.0f,  // Can walk on 45° slopes
    .mass = 80.0f,
    .layer = CollisionLayers3D::Character,
    .mask = CollisionLayers3D::World | CollisionLayers3D::Dynamic
};

physics3D->createCharacter(player, charDef);
```

---

### moveCharacter(Entity entity, Vec3 velocity, DeltaTime dt)

```cpp
Result<void, Physics3DError> moveCharacter(Entity entity, Vec3 velocity, DeltaTime dt);
```

Moves character with collision detection, step climbing, and slope limiting.

**Example:**

```cpp
Vec3 moveInput = getMovementInput();  // Normalized direction
float speed = 5.0f;

auto groundInfo = physics3D->getCharacterGroundInfo(character).value();
Vec3 velocity = moveInput * speed;

if (groundInfo.grounded) {
    // Jump
    if (input->wasPressed("Jump")) {
        velocity.y = 7.0f;
    }
} else {
    // Apply gravity
    auto currentVel = physics3D->getCharacterVelocity(character).value();
    velocity.y = currentVel.y - 9.81f * dt;
}

physics3D->moveCharacter(character, velocity, dt);
```

---

### getCharacterGroundInfo(Entity entity)

```cpp
Result<CharacterGroundInfo, Physics3DError> getCharacterGroundInfo(Entity entity) const;
```

Gets ground detection information.

```cpp
struct CharacterGroundInfo {
    bool grounded = false;
    Entity groundEntity{};
    Vec3 contactPoint{0.0f};
    Vec3 normal{0.0f, 1.0f, 0.0f};
    float slopeAngle = 0.0f;  // Degrees
};
```

---

## Constraints

### createConstraint(const ConstraintDef3D& def)

```cpp
Result<UUID, Physics3DError> createConstraint(const ConstraintDef3D& def);
```

Creates a constraint (joint) between two bodies.

**Constraint Types:**
- `ConstraintType3D::Fixed` - Weld bodies together
- `ConstraintType3D::Hinge` - Rotate around axis (door hinge)
- `ConstraintType3D::Slider` - Slide along axis (piston)
- `ConstraintType3D::Point` - Ball-and-socket joint
- `ConstraintType3D::Distance` - Maintain distance (rope)
- `ConstraintType3D::Cone` - Cone-limited rotation (ragdoll)

**Example:**

```cpp
// Door hinge
HingeConstraintDef hingeDef;
hingeDef.bodyA = door;
hingeDef.bodyB = doorFrame;
hingeDef.pivotA = Vec3{-1.0f, 0.0f, 0.0f};  // Left edge
hingeDef.axisA = Vec3{0.0f, 1.0f, 0.0f};    // Rotate around Y
hingeDef.hasLimits = true;
hingeDef.minAngle = 0.0f;
hingeDef.maxAngle = 1.57f;  // 90 degrees

auto hingeId = physics3D->createConstraint(hingeDef).value();
```

---

## World Settings

### setGravity(Vec3 gravity)

```cpp
void setGravity(Vec3 gravity);
Vec3 getGravity() const;
```

Sets world gravity vector.

**Default:** `Vec3{0.0f, -9.81f, 0.0f}` (Earth gravity)

**Example:**

```cpp
// Moon gravity
physics3D->setGravity(Vec3{0.0f, -1.62f, 0.0f});

// Zero gravity
physics3D->setGravity(Vec3{0.0f, 0.0f, 0.0f});
```

---

## Collision Callbacks

### setCollisionCallback(Collision3DCallback callback)

```cpp
using Collision3DCallback = std::function<void(const CollisionEvent3D&)>;
void setCollisionCallback(Collision3DCallback callback);
```

Sets callback for physical collisions (non-sensors).

```cpp
struct CollisionEvent3D {
    Entity entityA;
    Entity entityB;
    Vec3 contactPoint;
    Vec3 contactNormal;
    float impulse;
    float penetrationDepth;
};
```

**Example:**

```cpp
physics3D->setCollisionCallback([](const CollisionEvent3D& e) {
    if (entities->allOf<Player>(e.entityA) && entities->allOf<Enemy>(e.entityB)) {
        damagePlayer(e.entityA, 10);
    }
});
```

---

### setTriggerEnterCallback / setTriggerExitCallback

```cpp
using Trigger3DEnterCallback = std::function<void(const TriggerEvent3D&)>;
using Trigger3DExitCallback = std::function<void(const TriggerEvent3D&)>;

void setTriggerEnterCallback(Trigger3DEnterCallback callback);
void setTriggerExitCallback(Trigger3DExitCallback callback);
```

Callbacks for sensor (trigger) overlaps.

**Example:**

```cpp
physics3D->setTriggerEnterCallback([](const TriggerEvent3D& e) {
    if (isCheckpoint(e.entityA) && isPlayer(e.entityB)) {
        activateCheckpoint(e.entityA);
    }
});
```

---

## Update Loop

### update(DeltaTime dt, int subSteps)

```cpp
void update(DeltaTime dt, int subSteps = 1);
```

Steps the physics simulation forward.

**Parameters:**
- `dt`: Time step in seconds
- `subSteps`: Number of substeps (1-8, recommended: 4)

**Call in:** `Application::updateFixed()`

**Example:**

```cpp
void updateFixed(DeltaTime dt) override {
    physics3D->update(dt, 4);
}
```

---

### syncTransforms(std::span<const Entity> entities)

```cpp
void syncTransforms(std::span<const Entity> entities);
```

Synchronizes entity transforms from physics bodies. Call after `update()`.

**Example:**

```cpp
std::vector<Entity> physicsEntities = physics3D->getAllBodies();
physics3D->syncTransforms(physicsEntities);
```

---

## Debug and Statistics

### setDebugDraw(bool enabled)

```cpp
void setDebugDraw(bool enabled);
std::vector<DebugLine3D> getDebugLines() const;
```

Enables debug visualization and retrieves debug geometry.

---

### getStats()

```cpp
PhysicsStats3D getStats() const;
```

Returns performance statistics.

```cpp
struct PhysicsStats3D {
    std::uint32_t activeBodies;
    std::uint32_t sleepingBodies;
    std::uint32_t constraints;
    std::uint32_t characters;
    std::uint32_t vehicles;
    float updateTimeMs;
    std::uint32_t collisionPairs;
};
```

---

## Performance Tips

1. **Use appropriate shapes** - Spheres/capsules fastest, meshes slowest
2. **Set precise collision masks** - Avoid unnecessary collision checks
3. **Let bodies sleep** - Don't wake bodies unnecessarily
4. **Use 1-4 substeps** - More = more stable but slower
5. **Enable CCD for fast objects** - Prevents tunneling through walls
6. **Use sensors for triggers** - Cheaper than physical collision

## See Also

- [Physics3D System](../systems/Physics3D-System.md) - Comprehensive guide with examples
- [EntitySystem](EntitySystem.md) - Attaching physics to entities
- [EventSystem](EventSystem.md) - Collision event handling
