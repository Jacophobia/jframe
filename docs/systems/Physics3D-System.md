# Bestow Physics3D System

## Overview

The Physics3D System provides high-performance 3D rigid body physics simulation powered by **Jolt Physics**. It handles collision detection, response, raycasting, character controllers, vehicles, and spatial queries for 3D game entities.

Jolt Physics is a modern, AAA-quality physics engine used in games like Horizon Forbidden West. It provides excellent performance, stability, and features for both large open worlds and detailed simulations.

### Key Features

- **Rigid Body Physics**: Static, Kinematic, and Dynamic bodies with full 3D motion
- **Rich Shape Library**: Box, Sphere, Capsule, Cylinder, Mesh, Convex Hull, Height Field, and Compound shapes
- **Advanced Collision Detection**: Continuous Collision Detection (CCD) for fast-moving objects
- **Constraint System**: Fixed, Hinge, Slider, Cone, Point, and Distance constraints with motors
- **Character Controller**: Kinematic character movement with step climbing and slope handling
- **Vehicle Simulation**: Raycast-based vehicle with suspension, steering, and wheel physics
- **Spatial Queries**: Raycasting, shape casting (sphere, box, capsule), and overlap queries
- **Contact Information**: Query active contacts with detailed contact point data
- **Compound Shapes**: Build complex collision shapes from multiple primitives
- **Terrain Support**: Height field shapes for large-scale terrain
- **Debug Visualization**: Visual debugging of collision shapes and contacts

### Architecture

```
IPhysics3DSystem (interface)
    ↓
JoltPhysics3DSystem (implementation)
    ↓
Jolt Physics Engine
```

**Coordinate System:**
- Uses standard 3D coordinates (meters as units)
- Right-handed coordinate system: +X right, +Y up, +Z forward (typically)
- Gravity defaults to `{0, -9.81, 0}` (Earth gravity in m/s²)

**Entity Mapping:**
- Each `Entity` can have one physics body, one character controller, or one vehicle
- Bodies can have multiple collision shapes (compound shapes)
- Transform synchronization keeps entity transforms in sync with physics

---

## Getting Started

### Initialization

```cpp
import bestow;
import bestow.physics3d;
import bestow.physics3d.impl;

// Create and initialize the physics system
auto physics3D = bestow::createPhysics3DSystem();
auto* impl = dynamic_cast<JoltPhysics3DSystem*>(physics3D.get());
impl->initialize();

// Set world gravity
physics3D->setGravity(Vec3{0.0f, -9.81f, 0.0f});
```

### Update Loop

```cpp
void updateFixed(DeltaTime dt) {
    // Update physics simulation with substeps for stability
    physics3D->update(dt, 4);  // 4 substeps recommended

    // Synchronize entity transforms from physics
    std::vector<Entity> physicsEntities = physics3D->getAllBodies();
    physics3D->syncTransforms(physicsEntities);
}
```

---

## Creating Physics Bodies

### Body Types

| Type | Behavior | Use Case |
|------|----------|----------|
| **Static** | Immovable, infinite mass | Terrain, buildings, walls |
| **Kinematic** | Moved manually, affects dynamics | Moving platforms, doors |
| **Dynamic** | Affected by forces/gravity/collisions | Player, enemies, props, vehicles |

### Basic Body Creation

```cpp
// Create a dynamic physics body (player capsule)
Entity player = entities->createEntity();

PhysicsBodyDef3D playerDef{
    .type = BodyType3D::Dynamic,
    .transform = Transform3D{
        .position = Vec3{0.0f, 5.0f, 0.0f},
        .rotation = Quat{1.0f, 0.0f, 0.0f, 0.0f}  // Identity (no rotation)
    },
    .shapeType = ShapeType3D::Capsule,
    .shapeRadius = 0.5f,
    .shapeHalfHeight = 0.9f,  // Total height = 2 * (halfHeight + radius) = 2.8m
    .density = 1000.0f,        // kg/m³ (water density)
    .friction = 0.5f,
    .restitution = 0.0f,       // No bounce
    .linearDamping = 0.05f,
    .angularDamping = 0.05f,
    .layer = CollisionLayers3D::Character,
    .mask = CollisionLayers3D::World | CollisionLayers3D::Dynamic
};

physics3D->createBody(player, playerDef);
```

### Shape Types

#### Box Shape

```cpp
PhysicsBodyDef3D crateDef{
    .type = BodyType3D::Dynamic,
    .transform = Transform3D{.position = Vec3{0.0f, 1.0f, 0.0f}},
    .shapeType = ShapeType3D::Box,
    .shapeHalfExtents = Vec3{0.5f, 0.5f, 0.5f},  // 1m cube
    .density = 500.0f
};
physics3D->createBody(crate, crateDef);
```

#### Sphere Shape

```cpp
PhysicsBodyDef3D ballDef{
    .type = BodyType3D::Dynamic,
    .transform = Transform3D{.position = Vec3{0.0f, 5.0f, 0.0f}},
    .shapeType = ShapeType3D::Sphere,
    .shapeRadius = 0.5f,
    .density = 1000.0f,
    .restitution = 0.8f  // Bouncy ball
};
physics3D->createBody(ball, ballDef);
```

#### Capsule Shape

Perfect for characters - smooth collisions and natural rolling:

```cpp
PhysicsBodyDef3D characterDef{
    .type = BodyType3D::Dynamic,
    .transform = Transform3D{.position = Vec3{0.0f, 2.0f, 0.0f}},
    .shapeType = ShapeType3D::Capsule,
    .shapeRadius = 0.4f,
    .shapeHalfHeight = 0.8f,  // Total height = 2*(0.8+0.4) = 2.4m
    .density = 1000.0f
};
physics3D->createBody(character, characterDef);
```

#### Cylinder Shape

```cpp
PhysicsBodyDef3D barrelDef{
    .type = BodyType3D::Dynamic,
    .transform = Transform3D{.position = Vec3{0.0f, 1.0f, 0.0f}},
    .shapeType = ShapeType3D::Cylinder,
    .shapeRadius = 0.5f,
    .shapeHalfHeight = 1.0f,
    .density = 800.0f
};
physics3D->createBody(barrel, barrelDef);
```

### Static Terrain/Walls

```cpp
// Ground plane
PhysicsBodyDef3D groundDef{
    .type = BodyType3D::Static,
    .transform = Transform3D{.position = Vec3{0.0f, 0.0f, 0.0f}},
    .shapeType = ShapeType3D::Box,
    .shapeHalfExtents = Vec3{50.0f, 0.1f, 50.0f},  // 100m x 100m ground
    .friction = 0.8f
};
physics3D->createBody(ground, groundDef);
```

### Body Definition Properties

```cpp
struct PhysicsBodyDef3D {
    BodyType3D type = BodyType3D::Dynamic;
    Transform3D transform;                    // Initial position and rotation
    ShapeType3D shapeType = ShapeType3D::Box;
    Vec3 shapeHalfExtents{0.5f};             // For Box
    float shapeRadius = 0.5f;                 // For Sphere/Capsule/Cylinder
    float shapeHalfHeight = 0.5f;             // For Capsule/Cylinder
    float density = 1000.0f;                  // kg/m³ (mass computed from volume)
    float friction = 0.5f;                    // Surface friction (0-1+)
    float restitution = 0.3f;                 // Bounciness (0-1)
    float linearDamping = 0.05f;              // Velocity decay over time
    float angularDamping = 0.05f;             // Rotation decay over time
    float gravityFactor = 1.0f;               // Gravity multiplier (0=no gravity)
    bool allowSleep = true;                   // Sleep inactive bodies for performance
    CollisionLayer3D layer = 0x0001;          // What layer this body is on
    CollisionMask3D mask = 0xFFFF;            // What layers it collides with
    bool isSensor = false;                    // Trigger volume (no collision response)
    MotionQuality motionQuality = MotionQuality::Discrete;  // CCD for fast objects
    std::optional<MassProperties> massProperties;  // Override auto-computed mass
};
```

---

## Compound Shapes

Compound shapes let you build complex collision geometry from multiple primitives.

### Creating Compound Bodies

```cpp
// Build a table from boxes
CompoundShapeDef tableDef;

// Table top
BoxShapeDef top;
top.halfExtents = Vec3{1.0f, 0.05f, 0.6f};
top.localPosition = Vec3{0.0f, 0.8f, 0.0f};
top.density = 500.0f;
tableDef.boxes.push_back(top);

// Table legs
for (int i = 0; i < 4; ++i) {
    BoxShapeDef leg;
    leg.halfExtents = Vec3{0.05f, 0.4f, 0.05f};
    float x = (i % 2) ? 0.9f : -0.9f;
    float z = (i / 2) ? 0.5f : -0.5f;
    leg.localPosition = Vec3{x, 0.4f, z};
    leg.density = 500.0f;
    tableDef.boxes.push_back(leg);
}

// Create compound body
auto result = physics3D->createCompoundBody(
    table,
    BodyType3D::Static,
    Transform3D{.position = Vec3{0.0f, 0.0f, 0.0f}},
    tableDef
);
```

### Adding Shapes to Existing Bodies

```cpp
// Add a sphere on top of a box
BoxShapeDef baseShape;
baseShape.halfExtents = Vec3{1.0f, 0.2f, 1.0f};
auto shapeIndex = physics3D->addShape(entity, baseShape);

SphereShapeDef topShape;
topShape.radius = 0.5f;
topShape.localPosition = Vec3{0.0f, 0.7f, 0.0f};
physics3D->addShape(entity, topShape);

// Remove a shape
physics3D->removeShape(entity, shapeIndex);
```

---

## Height Field Terrain

For large-scale terrain, use height fields for optimal performance.

### Creating Height Field

```cpp
HeightFieldShapeDef terrainDef;

// Generate 256x256 height map
terrainDef.width = 256;
terrainDef.length = 256;
terrainDef.scale = Vec3{1.0f, 10.0f, 1.0f};  // XZ=1m per sample, Y=10m max height

terrainDef.heights.resize(256 * 256);
for (std::uint32_t z = 0; z < 256; ++z) {
    for (std::uint32_t x = 0; x < 256; ++x) {
        // Simple hills using sine waves
        float fx = x / 256.0f * 6.28f;
        float fz = z / 256.0f * 6.28f;
        terrainDef.heights[z * 256 + x] = std::sin(fx) * std::cos(fz);
    }
}

auto result = physics3D->createHeightFieldBody(
    terrain,
    Transform3D{.position = Vec3{-128.0f, 0.0f, -128.0f}},
    terrainDef
);
```

---

## Body Properties and Manipulation

### Transform

```cpp
// Get/Set position
auto pos = physics3D->getPosition(entity).value();
physics3D->setPosition(entity, Vec3{10.0f, 5.0f, 0.0f});

// Get/Set rotation
auto rot = physics3D->getRotation(entity).value();
physics3D->setRotation(entity, Quat{1.0f, 0.0f, 0.0f, 0.0f});

// Get/Set full transform
auto transform = physics3D->getTransform(entity).value();
physics3D->setTransform(entity, Transform3D{
    .position = Vec3{0.0f, 10.0f, 0.0f},
    .rotation = Quat{0.707f, 0.0f, 0.707f, 0.0f}  // 90° around Y
});
```

### Velocity

```cpp
// Get/Set linear velocity
auto vel = physics3D->getLinearVelocity(entity).value();
physics3D->setLinearVelocity(entity, Vec3{5.0f, 0.0f, 0.0f});  // 5 m/s right

// Get/Set angular velocity (axis-angle representation, magnitude = rad/s)
auto angVel = physics3D->getAngularVelocity(entity).value();
physics3D->setAngularVelocity(entity, Vec3{0.0f, 3.14f, 0.0f});  // Spin around Y
```

### Body Type

```cpp
// Change body type at runtime
physics3D->setBodyType(platform, BodyType3D::Kinematic);

// Query body type
auto type = physics3D->getBodyType(entity).value();
if (type == BodyType3D::Dynamic) {
    // Can apply forces
}
```

---

## Forces and Impulses

### Forces (Continuous)

Forces accumulate over time and are cleared each physics step. Use for continuous effects.

```cpp
// Apply force at center of mass
physics3D->applyForce(entity, Vec3{1000.0f, 0.0f, 0.0f});

// Apply force at specific world point (causes torque)
Vec3 worldPoint = physics3D->getPosition(entity).value() + Vec3{0.0f, 1.0f, 0.0f};
physics3D->applyForceAtPoint(entity, Vec3{500.0f, 0.0f, 0.0f}, worldPoint);

// Apply torque directly
physics3D->applyTorque(entity, Vec3{0.0f, 100.0f, 0.0f});  // Spin around Y
```

### Impulses (Instant)

Impulses immediately change velocity. Use for instant effects like jumps or explosions.

```cpp
// Jump impulse
physics3D->applyImpulse(player, Vec3{0.0f, 5.0f, 0.0f});

// Explosion knockback
Vec3 knockback = glm::normalize(targetPos - explosionPos) * 10.0f;
physics3D->applyImpulse(target, knockback);

// Angular impulse
physics3D->applyAngularImpulse(entity, Vec3{0.0f, 2.0f, 0.0f});
```

### Mass Properties

```cpp
// Get mass (computed from density and volume)
float mass = physics3D->getMass(entity).value();

// Override mass manually
physics3D->setMass(entity, 100.0f);

// Get/Set full mass properties
auto massProps = physics3D->getMassProperties(entity).value();

MassProperties customMass{
    .mass = 50.0f,
    .centerOfMass = Vec3{0.0f, -0.2f, 0.0f},  // Slightly below center
    .inertiaTensor = Mat3{1.0f},  // Simplified inertia
    .autoCompute = false
};
physics3D->setMassProperties(entity, customMass);

// Query center of mass and inertia
Vec3 com = physics3D->getCenterOfMass(entity).value();
Mat3 inertia = physics3D->getInertiaTensor(entity).value();
```

---

## Collision Detection and Filtering

### Collision Layers

Collision layers define **what something is**. Each body belongs to layers (bit flags).

```cpp
// Predefined layers (from bestow.types)
namespace CollisionLayers3D {
    inline constexpr CollisionLayer3D World      = 0x0001;
    inline constexpr CollisionLayer3D Character  = 0x0002;
    inline constexpr CollisionLayer3D Dynamic    = 0x0004;
    inline constexpr CollisionLayer3D Trigger    = 0x0008;
    inline constexpr CollisionLayer3D Debris     = 0x0010;
    inline constexpr CollisionLayer3D Vehicle    = 0x0020;
}

// Assign layer
physics3D->setCollisionLayer(player, CollisionLayers3D::Character);
physics3D->setCollisionLayer(ground, CollisionLayers3D::World);
```

### Collision Masks

Collision masks define **what something collides with**. Use bitwise OR to combine layers.

```cpp
// Character collides with world and dynamic objects
CollisionMask3D characterMask = CollisionLayers3D::World | CollisionLayers3D::Dynamic;
physics3D->setCollisionMask(player, characterMask);

// Dynamic objects collide with everything
CollisionMask3D dynamicMask = 0xFFFF;
physics3D->setCollisionMask(crate, dynamicMask);

// Trigger only overlaps with characters
CollisionMask3D triggerMask = CollisionLayers3D::Character;
physics3D->setCollisionMask(trigger, triggerMask);
```

### Sensors (Trigger Volumes)

Sensors detect overlaps without physical collision response.

```cpp
// Create trigger volume
PhysicsBodyDef3D triggerDef{
    .type = BodyType3D::Static,
    .transform = Transform3D{.position = Vec3{10.0f, 0.0f, 0.0f}},
    .shapeType = ShapeType3D::Box,
    .shapeHalfExtents = Vec3{2.0f, 3.0f, 2.0f},
    .isSensor = true,
    .layer = CollisionLayers3D::Trigger
};
physics3D->createBody(checkpoint, triggerDef);

// Toggle sensor at runtime
physics3D->setSensor(entity, true);
```

### Collision Callbacks

```cpp
// Register collision callback (non-sensors)
physics3D->setCollisionCallback([](const CollisionEvent3D& event) {
    Entity a = event.entityA;
    Entity b = event.entityB;
    Vec3 contactPoint = event.contactPoint;
    Vec3 normal = event.contactNormal;
    float impulse = event.impulse;
    float penetration = event.penetrationDepth;

    // Handle collision
    applyDamage(a, impulse * 0.1f);
});

// Trigger enter callback (sensors)
physics3D->setTriggerEnterCallback([](const TriggerEvent3D& event) {
    if (isPlayer(event.entityB)) {
        activateCheckpoint(event.entityA);
    }
});

// Trigger exit callback
physics3D->setTriggerExitCallback([](const TriggerEvent3D& event) {
    if (isPlayer(event.entityB)) {
        deactivateCheckpoint(event.entityA);
    }
});
```

---

## Contact Queries

Get detailed information about active collisions.

### Query Contacts

```cpp
// Get all contacts for an entity
std::vector<ContactPair3D> contacts = physics3D->getContacts(player);
for (const auto& pair : contacts) {
    Entity other = (pair.entityA == player) ? pair.entityB : pair.entityA;

    for (const auto& contact : pair.contacts) {
        Vec3 pointA = contact.worldPositionOnA;
        Vec3 pointB = contact.worldPositionOnB;
        Vec3 normal = contact.worldNormalOnB;
        float depth = contact.penetrationDepth;
        float friction = contact.combinedFriction;
        float restitution = contact.combinedRestitution;
    }
}

// Check if two entities are touching
if (physics3D->areInContact(playerFeet, ground)) {
    // Player is on ground
}

// Get specific contact pair
auto contactPair = physics3D->getContactPair(entityA, entityB);
if (contactPair) {
    float totalImpulse = contactPair->impulse;
}

// Get all active contacts in world
std::vector<ContactPair3D> allContacts = physics3D->getAllContacts();
```

---

## Raycasting

Cast rays through the physics world to detect hits.

### Single Raycast

```cpp
Vec3 origin{0.0f, 1.0f, 0.0f};
Vec3 direction{0.0f, -1.0f, 0.0f};  // Downward
float maxDistance = 10.0f;

auto hit = physics3D->raycast(origin, direction, maxDistance);
if (hit) {
    Entity hitEntity = hit->entity;
    Vec3 hitPoint = hit->point;
    Vec3 hitNormal = hit->normal;
    float distance = hit->distance;

    // Spawn impact effect
    createImpact(hitPoint, hitNormal);
}
```

### Raycast with Filtering

```cpp
QueryFilter3D filter{
    .layerMask = CollisionLayers3D::World | CollisionLayers3D::Dynamic,
    .ignoreEntity = player,  // Don't hit the player
    .ignoreSensors = true    // Ignore trigger volumes
};

auto hit = physics3D->raycast(origin, direction, 100.0f, filter);
```

### Raycast All

```cpp
// Get all hits along ray
std::vector<RaycastHit3D> hits = physics3D->raycastAll(origin, direction, 100.0f);

// Hits are sorted by distance (closest first)
for (const auto& hit : hits) {
    processHit(hit.entity, hit.point, hit.distance);
}
```

### Line of Sight Check

```cpp
bool hasLineOfSight(Entity from, Entity to) {
    Vec3 fromPos = physics3D->getPosition(from).value();
    Vec3 toPos = physics3D->getPosition(to).value();

    Vec3 direction = toPos - fromPos;
    float distance = glm::length(direction);
    direction = glm::normalize(direction);

    QueryFilter3D filter{
        .layerMask = CollisionLayers3D::World,
        .ignoreEntity = from,
        .ignoreSensors = true
    };

    auto hit = physics3D->raycast(fromPos, direction, distance, filter);
    return !hit.has_value() || hit->entity == to;
}
```

---

## Shape Casting

Cast entire shapes through space to predict collisions.

### Sphere Cast

Perfect for projectile prediction or character movement validation.

```cpp
Vec3 origin{0.0f, 1.0f, 0.0f};
float radius = 0.5f;
Vec3 direction{1.0f, 0.0f, 0.0f};
float maxDistance = 10.0f;

auto hit = physics3D->sphereCast(origin, radius, direction, maxDistance);
if (hit) {
    // Sphere would collide at hit.point
    Vec3 penetration = hit->penetrationDepth;
}
```

### Box Cast

```cpp
Vec3 origin{0.0f, 1.0f, 0.0f};
Vec3 halfExtents{0.5f, 1.0f, 0.5f};
Quat rotation{1.0f, 0.0f, 0.0f, 0.0f};  // No rotation
Vec3 direction{0.0f, -1.0f, 0.0f};

auto hit = physics3D->boxCast(origin, halfExtents, rotation, direction, 5.0f);
```

### Capsule Cast

Ideal for character movement prediction.

```cpp
Vec3 origin{0.0f, 2.0f, 0.0f};
float radius = 0.4f;
float halfHeight = 0.8f;
Quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
Vec3 moveDirection{1.0f, 0.0f, 0.0f};

QueryFilter3D filter{
    .ignoreEntity = character,
    .ignoreSensors = true
};

auto hit = physics3D->capsuleCast(origin, radius, halfHeight, rotation,
                                   moveDirection, 2.0f, filter);

if (hit) {
    // Would collide - limit movement
    float safeDistance = hit->distance - 0.01f;
}
```

---

## Overlap Queries

Find all entities overlapping a region.

### Sphere Overlap

```cpp
Vec3 center{0.0f, 5.0f, 0.0f};
float radius = 3.0f;

std::vector<Entity> entities = physics3D->overlapSphere(center, radius);

// Apply explosion force
for (Entity entity : entities) {
    Vec3 pos = physics3D->getPosition(entity).value();
    Vec3 dir = glm::normalize(pos - center);
    float dist = glm::length(pos - center);
    float force = 1000.0f * (1.0f - dist / radius);  // Falloff

    physics3D->applyImpulse(entity, dir * force);
}
```

### Box Overlap

```cpp
Vec3 center{0.0f, 1.0f, 0.0f};
Vec3 halfExtents{5.0f, 2.0f, 5.0f};
Quat rotation{1.0f, 0.0f, 0.0f, 0.0f};

QueryFilter3D filter{
    .layerMask = CollisionLayers3D::Dynamic,
    .ignoreSensors = false
};

std::vector<Entity> entities = physics3D->overlapBox(center, halfExtents,
                                                       rotation, filter);
```

### AABB Query

```cpp
Vec3 min{-10.0f, 0.0f, -10.0f};
Vec3 max{10.0f, 5.0f, 10.0f};

std::vector<Entity> entities = physics3D->queryAABB(min, max);
```

---

## Character Controller

Kinematic character controller with step climbing, slope handling, and ground detection.

### Creating a Character

```cpp
Entity character = entities->createEntity();

CharacterControllerDef charDef{
    .radius = 0.3f,
    .height = 1.8f,
    .stepHeight = 0.35f,       // Can climb 35cm steps
    .maxSlopeAngle = 45.0f,    // Can walk on slopes up to 45°
    .mass = 80.0f,             // 80 kg
    .layer = CollisionLayers3D::Character,
    .mask = CollisionLayers3D::World | CollisionLayers3D::Dynamic
};

physics3D->createCharacter(character, charDef);
```

### Moving the Character

```cpp
void updateCharacter(DeltaTime dt) {
    // Get input
    Vec3 moveInput = getMovementInput();  // Normalized direction

    // Build velocity
    float speed = 5.0f;  // 5 m/s
    Vec3 velocity = moveInput * speed;

    // Check if grounded
    auto groundInfo = physics3D->getCharacterGroundInfo(character).value();
    if (groundInfo.grounded) {
        // Apply gravity
        velocity.y = -1.0f;  // Small downward force to stay grounded

        // Jump
        if (input->isPressed("Jump")) {
            velocity.y = 7.0f;  // Jump velocity
        }
    } else {
        // In air - apply gravity
        auto currentVel = physics3D->getCharacterVelocity(character).value();
        velocity.y = currentVel.y - 9.81f * dt;  // Gravity accumulation
    }

    // Move character (handles collisions, step climbing, slope limiting)
    physics3D->moveCharacter(character, velocity, dt);
}
```

### Ground Detection

```cpp
auto groundInfo = physics3D->getCharacterGroundInfo(character).value();

if (groundInfo.grounded) {
    Entity groundEntity = groundInfo.groundEntity;
    Vec3 groundNormal = groundInfo.normal;
    float slopeAngle = groundInfo.slopeAngle;  // Degrees
    Vec3 contactPoint = groundInfo.contactPoint;

    if (slopeAngle > 30.0f) {
        // Steep slope - slide down
    }
}
```

---

## Constraints

Constraints connect two bodies with physical joints. Supports motors, limits, and breakable connections.

### Fixed Constraint

Welds two bodies together rigidly.

```cpp
ConstraintDef3D fixedDef{
    .type = ConstraintType3D::Fixed,
    .bodyA = bodyA,
    .bodyB = bodyB,
    .pivotA = Vec3{0.0f, 0.0f, 0.0f},  // Local anchor on A
    .pivotB = Vec3{0.0f, 0.0f, 0.0f},  // Local anchor on B
    .collideConnected = false           // Don't collide connected bodies
};

auto constraintId = physics3D->createConstraint(fixedDef).value();
```

### Hinge Constraint

Allows rotation around a single axis (like a door).

```cpp
HingeConstraintDef hingeDef;
hingeDef.bodyA = door;
hingeDef.bodyB = doorFrame;
hingeDef.pivotA = Vec3{-1.0f, 0.0f, 0.0f};  // Left edge of door
hingeDef.pivotB = Vec3{0.0f, 0.0f, 0.0f};
hingeDef.axisA = Vec3{0.0f, 1.0f, 0.0f};    // Rotate around Y
hingeDef.axisB = Vec3{0.0f, 1.0f, 0.0f};
hingeDef.hasLimits = true;
hingeDef.minAngle = 0.0f;
hingeDef.maxAngle = 1.57f;  // 90 degrees

auto hingeId = physics3D->createConstraint(hingeDef).value();
```

### Hinge with Motor

```cpp
// Create powered hinge (like a motor-driven wheel)
HingeConstraintDef motorDef;
motorDef.bodyA = wheel;
motorDef.bodyB = axle;
motorDef.axisA = Vec3{1.0f, 0.0f, 0.0f};
motorDef.hasMotor = true;
motorDef.motorTargetVelocity = 10.0f;  // 10 rad/s
motorDef.motorMaxTorque = 100.0f;

auto motorId = physics3D->createConstraint(motorDef).value();

// Change motor speed at runtime
physics3D->setHingeMotor(motorId, 20.0f, 150.0f);
```

### Slider Constraint

Allows linear motion along an axis (like a piston).

```cpp
SliderConstraintDef sliderDef;
sliderDef.bodyA = piston;
sliderDef.bodyB = cylinder;
sliderDef.axisA = Vec3{0.0f, 1.0f, 0.0f};  // Slide up/down
sliderDef.hasLimits = true;
sliderDef.minDistance = -0.5f;
sliderDef.maxDistance = 0.5f;

auto sliderId = physics3D->createConstraint(sliderDef).value();
```

### Point Constraint

Connects two points (ball-and-socket joint).

```cpp
PointConstraintDef pointDef;
pointDef.bodyA = ragdollUpperArm;
pointDef.bodyB = ragdollShoulder;
pointDef.pivotA = Vec3{0.0f, 0.5f, 0.0f};  // Top of upper arm
pointDef.pivotB = Vec3{0.0f, -0.2f, 0.0f}; // Bottom of shoulder

auto pointId = physics3D->createConstraint(pointDef).value();
```

### Distance Constraint

Keeps two bodies at a fixed distance range (like a rope).

```cpp
DistanceConstraintDef distDef;
distDef.bodyA = grapplingHook;
distDef.bodyB = player;
distDef.minDistance = 0.5f;
distDef.maxDistance = 10.0f;  // Rope length

auto ropeId = physics3D->createConstraint(distDef).value();
```

### Cone Constraint

Limits rotation to a cone (ragdoll joints).

```cpp
ConeConstraintDef coneDef;
coneDef.bodyA = ragdollForearm;
coneDef.bodyB = ragdollUpperArm;
coneDef.pivotA = Vec3{0.0f, 0.3f, 0.0f};
coneDef.pivotB = Vec3{0.0f, -0.3f, 0.0f};
coneDef.twistAxisA = Vec3{0.0f, 1.0f, 0.0f};
coneDef.halfConeAngle = 0.785f;  // 45 degrees

auto elbowId = physics3D->createConstraint(coneDef).value();
```

### Constraint Management

```cpp
// Disable/enable constraint
physics3D->setConstraintEnabled(constraintId, false);

// Get all constraints attached to a body
std::vector<UUID> constraints = physics3D->getConstraints(entity);

// Check constraint force (for breakable constraints)
float force = physics3D->getConstraintForce(constraintId).value();
if (force > 1000.0f) {
    physics3D->destroyConstraint(constraintId);  // Break!
}

// Destroy constraint
physics3D->destroyConstraint(constraintId);
```

---

## Vehicle Simulation

Raycast-based vehicle with suspension, steering, and wheel physics.

### Creating a Vehicle

```cpp
Entity car = entities->createEntity();

// Create vehicle chassis body first
PhysicsBodyDef3D chassisDef{
    .type = BodyType3D::Dynamic,
    .transform = Transform3D{.position = Vec3{0.0f, 1.0f, 0.0f}},
    .shapeType = ShapeType3D::Box,
    .shapeHalfExtents = Vec3{1.0f, 0.5f, 2.0f},
    .density = 500.0f
};
physics3D->createBody(car, chassisDef);

// Define wheels
VehicleDef vehicleDef;

// Front left wheel
WheelDef frontLeft{
    .connectionPoint = Vec3{-0.9f, -0.3f, 1.2f},
    .suspensionDirection = Vec3{0.0f, -1.0f, 0.0f},
    .suspensionLength = 0.4f,
    .suspensionStiffness = 40.0f,
    .suspensionDamping = 5.0f,
    .radius = 0.4f,
    .friction = 1.2f,
    .isDriven = true,
    .isSteered = true
};
vehicleDef.wheels.push_back(frontLeft);

// Front right wheel
WheelDef frontRight = frontLeft;
frontRight.connectionPoint.x = 0.9f;
vehicleDef.wheels.push_back(frontRight);

// Rear wheels (not steered)
WheelDef rearLeft = frontLeft;
rearLeft.connectionPoint.z = -1.2f;
rearLeft.isSteered = false;
vehicleDef.wheels.push_back(rearLeft);

WheelDef rearRight = rearLeft;
rearRight.connectionPoint.x = 0.9f;
vehicleDef.wheels.push_back(rearRight);

vehicleDef.maxEngineForce = 15000.0f;
vehicleDef.maxBrakeForce = 8000.0f;
vehicleDef.maxSteeringAngle = 0.6f;  // ~34 degrees

physics3D->createVehicle(car, vehicleDef);
```

### Driving the Vehicle

```cpp
void updateVehicle(DeltaTime dt) {
    // Get input
    float throttle = input->getAxis("Vertical");    // -1 to 1
    float steering = input->getAxis("Horizontal");  // -1 to 1
    float brake = input->isPressed("Brake") ? 1.0f : 0.0f;

    // Update vehicle
    physics3D->updateVehicle(car, throttle, steering, brake);

    // Get vehicle speed
    float speedMps = physics3D->getVehicleSpeed(car).value();
    float speedKph = speedMps * 3.6f;

    // Check wheel ground contact
    for (std::uint32_t i = 0; i < 4; ++i) {
        bool grounded = physics3D->isWheelGrounded(car, i).value();

        if (grounded) {
            // Get wheel transform for visual update
            WheelState wheelState = physics3D->getWheelTransform(car, i).value();
            updateWheelVisual(i, wheelState.transform);
        }
    }
}
```

---

## Body State Management

### Sleep State

Bodies sleep when inactive to save CPU time.

```cpp
// Check if body is awake
bool awake = physics3D->isAwake(entity).value();

// Wake up a sleeping body
physics3D->wakeUp(entity);

// Force body to sleep
physics3D->putToSleep(entity);
```

### Bounding Box

```cpp
// Get axis-aligned bounding box
AABB3D bounds = physics3D->getBodyBounds(entity).value();
Vec3 min = bounds.min;
Vec3 max = bounds.max;
Vec3 center = (min + max) * 0.5f;
Vec3 extents = (max - min) * 0.5f;
```

### Body Destruction

```cpp
// Destroy physics body
physics3D->destroyBody(entity);

// Destroy character controller
physics3D->destroyCharacter(entity);

// Destroy vehicle
physics3D->destroyVehicle(entity);

// Check if body exists
if (physics3D->hasBody(entity)) {
    // Body exists
}
```

---

## Continuous Collision Detection (CCD)

Enable CCD for fast-moving objects to prevent tunneling through thin walls.

```cpp
// Create bullet with CCD
PhysicsBodyDef3D bulletDef{
    .type = BodyType3D::Dynamic,
    .transform = Transform3D{.position = gunBarrel},
    .shapeType = ShapeType3D::Sphere,
    .shapeRadius = 0.05f,
    .density = 7800.0f,  // Steel
    .motionQuality = MotionQuality::LinearCast  // Enable CCD
};
physics3D->createBody(bullet, bulletDef);

// Enable CCD on existing body
physics3D->setMotionQuality(entity, MotionQuality::LinearCast);
```

---

## World Settings

### Gravity

```cpp
// Set world gravity
physics3D->setGravity(Vec3{0.0f, -9.81f, 0.0f});  // Earth gravity

// Low gravity (moon-like)
physics3D->setGravity(Vec3{0.0f, -1.62f, 0.0f});

// Zero gravity (space)
physics3D->setGravity(Vec3{0.0f, 0.0f, 0.0f});

// Per-body gravity override
physics3D->setGravityFactor(floatingPlatform, 0.0f);  // Ignore gravity
```

---

## Debug Visualization

```cpp
// Enable debug drawing
physics3D->setDebugDraw(true);

// Get debug lines for rendering
std::vector<DebugLine3D> lines = physics3D->getDebugLines();

for (const auto& line : lines) {
    Vec3 start = line.start;
    Vec3 end = line.end;
    Color color = line.color;

    // Render line with your graphics system
    graphics->drawLine(start, end, color);
}
```

---

## Performance and Statistics

### Query Statistics

```cpp
PhysicsStats3D stats = physics3D->getStats();

std::cout << "Active Bodies: " << stats.activeBodies << "\n";
std::cout << "Sleeping Bodies: " << stats.sleepingBodies << "\n";
std::cout << "Constraints: " << stats.constraints << "\n";
std::cout << "Characters: " << stats.characters << "\n";
std::cout << "Vehicles: " << stats.vehicles << "\n";
std::cout << "Collision Pairs: " << stats.collisionPairs << "\n";
std::cout << "Update Time: " << stats.updateTimeMs << " ms\n";
```

### Optimization Tips

1. **Use appropriate shapes**:
   - Spheres and capsules are fastest
   - Boxes are very fast
   - Convex hulls are moderate
   - Meshes are slowest (use only for static geometry)

2. **Collision filtering**:
   - Set precise layer masks to avoid unnecessary checks
   - Use sensors for non-physical interactions

3. **Let bodies sleep**:
   - Don't wake up bodies unnecessarily
   - Set `allowSleep = true` for most bodies

4. **Substeps**:
   - Use 1-4 substeps for most games
   - Increase for very demanding simulations

5. **Compound shapes**:
   - Limit number of child shapes (< 10 if possible)
   - Consider using a single convex hull instead

6. **Height fields**:
   - Use for large terrains instead of mesh shapes
   - Keep resolution reasonable (256x256 or 512x512)

7. **Contact queries**:
   - Cache contact results if queried every frame
   - Use callbacks instead of polling when possible

---

## Complete Examples

### Third-Person Character Controller

```cpp
// Setup
Entity character = entities->createEntity();

PhysicsBodyDef3D charBodyDef{
    .type = BodyType3D::Dynamic,
    .transform = Transform3D{.position = Vec3{0.0f, 2.0f, 0.0f}},
    .shapeType = ShapeType3D::Capsule,
    .shapeRadius = 0.4f,
    .shapeHalfHeight = 0.8f,
    .density = 1000.0f,
    .friction = 0.0f,  // No friction - we control movement
    .layer = CollisionLayers3D::Character
};
physics3D->createBody(character, charBodyDef);

// Update loop
void updateThirdPersonCharacter(DeltaTime dt) {
    // Get camera-relative input
    Vec3 forward = camera->getForward();
    Vec3 right = camera->getRight();
    forward.y = 0.0f;
    right.y = 0.0f;
    forward = glm::normalize(forward);
    right = glm::normalize(right);

    Vec2 input = getMovementInput();  // WASD normalized
    Vec3 moveDir = forward * input.y + right * input.x;

    // Get current velocity
    Vec3 vel = physics3D->getLinearVelocity(character).value();

    // Ground check
    Vec3 charPos = physics3D->getPosition(character).value();
    auto groundHit = physics3D->raycast(
        charPos,
        Vec3{0.0f, -1.0f, 0.0f},
        2.0f,
        QueryFilter3D{.layerMask = CollisionLayers3D::World}
    );

    bool grounded = groundHit && groundHit->distance < 1.3f;

    if (grounded) {
        // Grounded movement
        float speed = input->isPressed("Sprint") ? 8.0f : 4.0f;
        vel.x = moveDir.x * speed;
        vel.z = moveDir.z * speed;
        vel.y = -1.0f;  // Push down to stay grounded

        // Jump
        if (input->wasPressed("Jump")) {
            vel.y = 8.0f;
        }
    } else {
        // Air control (reduced)
        float airControl = 0.3f;
        vel.x += moveDir.x * 20.0f * airControl * dt;
        vel.z += moveDir.z * 20.0f * airControl * dt;

        // Limit air speed
        Vec2 horizontalVel{vel.x, vel.z};
        if (glm::length(horizontalVel) > 10.0f) {
            horizontalVel = glm::normalize(horizontalVel) * 10.0f;
            vel.x = horizontalVel.x;
            vel.z = horizontalVel.y;
        }
    }

    physics3D->setLinearVelocity(character, vel);

    // Face movement direction
    if (glm::length(moveDir) > 0.1f) {
        Quat targetRot = glm::quatLookAt(moveDir, Vec3{0.0f, 1.0f, 0.0f});
        Quat currentRot = physics3D->getRotation(character).value();
        Quat newRot = glm::slerp(currentRot, targetRot, 10.0f * dt);
        physics3D->setRotation(character, newRot);
    }
}
```

### Ragdoll Physics

```cpp
struct RagdollBone {
    Entity entity;
    UUID constraint;
};

std::vector<RagdollBone> createRagdoll(Vec3 position) {
    std::vector<RagdollBone> bones;

    // Pelvis (root)
    Entity pelvis = createRagdollPart(position, Vec3{0.3f, 0.2f, 0.25f});
    bones.push_back({pelvis, UUID{}});

    // Spine
    Entity spine = createRagdollPart(position + Vec3{0,0.5f,0}, Vec3{0.25f,0.3f,0.2f});
    HingeConstraintDef spineJoint;
    spineJoint.bodyA = spine;
    spineJoint.bodyB = pelvis;
    spineJoint.axisA = Vec3{1,0,0};
    spineJoint.hasLimits = true;
    spineJoint.minAngle = -0.3f;
    spineJoint.maxAngle = 0.3f;
    bones.push_back({spine, physics3D->createConstraint(spineJoint).value()});

    // ... (create more bones with constraints)

    return bones;
}
```

### Vehicle with Boost

```cpp
void updateBoostVehicle(Entity vehicle, DeltaTime dt) {
    float throttle = input->getAxis("Vertical");
    float steering = input->getAxis("Horizontal");
    float brake = input->isPressed("Brake") ? 1.0f : 0.0f;

    // Update vehicle simulation
    physics3D->updateVehicle(vehicle, throttle, steering, brake);

    // Boost
    if (input->isPressed("Boost") && boostFuel > 0.0f) {
        Vec3 forward = getVehicleForward(vehicle);
        physics3D->applyForce(vehicle, forward * 50000.0f);
        boostFuel -= dt;
    }

    // Speed limiter
    float speed = physics3D->getVehicleSpeed(vehicle).value();
    if (speed > 50.0f) {  // 180 km/h
        Vec3 vel = physics3D->getLinearVelocity(vehicle).value();
        vel = glm::normalize(vel) * 50.0f;
        physics3D->setLinearVelocity(vehicle, vel);
    }
}
```

---

## Common Pitfalls

### 1. Forgetting to Initialize

```cpp
// WRONG: System won't work without initialization
auto physics3D = createPhysics3DSystem();
physics3D->createBody(entity, def);  // Crashes or fails silently

// CORRECT:
auto physics3D = createPhysics3DSystem();
auto* impl = dynamic_cast<JoltPhysics3DSystem*>(physics3D.get());
impl->initialize();
physics3D->createBody(entity, def);
```

### 2. Incorrect Scale

Jolt works best with realistic scales (1 unit = 1 meter). Avoid tiny or huge objects.

```cpp
// BAD: Too small (millimeter scale)
.shapeRadius = 0.001f  // 1mm sphere - physics will be unstable

// GOOD: Realistic scale (meter scale)
.shapeRadius = 0.5f    // 50cm sphere
```

### 3. Missing Collision Mask

```cpp
// Bodies won't collide if masks don't match!
physics3D->setCollisionLayer(player, CollisionLayers3D::Character);
physics3D->setCollisionMask(player, CollisionLayers3D::World);  // Only World

physics3D->setCollisionLayer(enemy, CollisionLayers3D::Dynamic);
physics3D->setCollisionMask(enemy, 0xFFFF);

// Player won't collide with enemy (Character not in enemy's mask)
```

### 4. Not Using Result Types

```cpp
// BAD: Ignoring errors
auto pos = physics3D->getPosition(entity).value();  // Crashes if entity invalid

// GOOD: Check result
auto posResult = physics3D->getPosition(entity);
if (posResult) {
    Vec3 pos = posResult.value();
} else {
    // Handle error
}
```

### 5. Setting Velocity Every Frame on Dynamic Bodies

```cpp
// BAD: Overrides physics simulation
void update(DeltaTime dt) {
    physics3D->setLinearVelocity(player, Vec3{5,0,0});  // Ignores gravity!
}

// GOOD: Modify velocity selectively
void handleInput() {
    Vec3 vel = physics3D->getLinearVelocity(player).value();
    vel.x = input->getAxis("Horizontal") * 5.0f;  // Only change X
    physics3D->setLinearVelocity(player, vel);
}
```

---

## API Reference Summary

### Body Management
- `createBody(entity, def)` - Create physics body
- `destroyBody(entity)` - Remove physics body
- `hasBody(entity)` - Check if body exists
- `getAllBodies()` - Get all physics entities
- `createCompoundBody(...)` - Create multi-shape body
- `createHeightFieldBody(...)` - Create terrain

### Transform
- `setTransform(entity, transform)` / `getTransform(entity)`
- `setPosition(entity, pos)` / `getPosition(entity)`
- `setRotation(entity, quat)` / `getRotation(entity)`

### Velocity
- `setLinearVelocity(entity, vel)` / `getLinearVelocity(entity)`
- `setAngularVelocity(entity, vel)` / `getAngularVelocity(entity)`

### Forces
- `applyForce(entity, force)`
- `applyForceAtPoint(entity, force, worldPoint)`
- `applyTorque(entity, torque)`
- `applyImpulse(entity, impulse)`
- `applyImpulseAtPoint(entity, impulse, worldPoint)`
- `applyAngularImpulse(entity, impulse)`

### Properties
- `setMass(entity, mass)` / `getMass(entity)`
- `setLinearDamping(entity, damping)`
- `setAngularDamping(entity, damping)`
- `setGravityFactor(entity, factor)`
- `setFriction(entity, friction)`
- `setRestitution(entity, restitution)`
- `setMassProperties(entity, props)` / `getMassProperties(entity)`

### Collision
- `setCollisionLayer(entity, layer)`
- `setCollisionMask(entity, mask)`
- `setSensor(entity, isSensor)`

### Queries
- `raycast(origin, direction, maxDist, filter)` - Closest hit
- `raycastAll(origin, direction, maxDist, filter)` - All hits
- `sphereCast(...)` / `boxCast(...)` / `capsuleCast(...)` - Shape casting
- `overlapSphere(...)` / `overlapBox(...)` / `queryAABB(...)` - Overlap queries

### Contacts
- `getContacts(entity)` - Get entity's contacts
- `getAllContacts()` - Get all active contacts
- `areInContact(a, b)` - Check if touching
- `getContactPair(a, b)` - Get specific contact

### Constraints
- `createConstraint(def)` - Create joint
- `destroyConstraint(id)` - Remove joint
- `setConstraintEnabled(id, enabled)` - Enable/disable
- `setHingeLimits(...)` / `setHingeMotor(...)` - Hinge control
- `setSliderLimits(...)` / `setSliderMotor(...)` - Slider control
- `getConstraintForce(id)` - Query force (for breaking)

### Character Controller
- `createCharacter(entity, def)` - Create character
- `destroyCharacter(entity)` - Remove character
- `moveCharacter(entity, velocity, dt)` - Move with collision
- `getCharacterGroundInfo(entity)` - Ground detection

### Vehicle
- `createVehicle(entity, def)` - Create vehicle
- `destroyVehicle(entity)` - Remove vehicle
- `updateVehicle(entity, throttle, steering, brake)` - Drive
- `getWheelTransform(entity, wheelIndex)` - Wheel state
- `isWheelGrounded(entity, wheelIndex)` - Wheel contact
- `getVehicleSpeed(entity)` - Current speed

### World
- `setGravity(gravity)` / `getGravity()`
- `update(dt, subSteps)` - Step simulation
- `syncTransforms(entities)` - Update entity transforms

### Callbacks
- `setCollisionCallback(callback)` - Physical collisions
- `setTriggerEnterCallback(callback)` - Sensor enter
- `setTriggerExitCallback(callback)` - Sensor exit

### Debug & Stats
- `setDebugDraw(enabled)` - Toggle debug visualization
- `getDebugLines()` - Get debug geometry
- `getStats()` - Performance statistics

---

## Further Reading

- **Jolt Physics Documentation**: [https://github.com/jrouwe/JoltPhysics](https://github.com/jrouwe/JoltPhysics)
- **Bestow Technical Design**: `docs/bestow-technical-design.md`
- **Entity System**: `docs/systems/Entity-System.md`
- **Events System**: `docs/systems/Events-System.md`

---

## Version History

- **v1.0** (2024) - Initial implementation with Jolt Physics
  - Rigid bodies (Static, Kinematic, Dynamic)
  - Rich shape library (Box, Sphere, Capsule, Cylinder, Mesh, Convex Hull, Height Field, Compound)
  - Constraint system (Fixed, Hinge, Slider, Cone, Point, Distance)
  - Character controller with step climbing and slope handling
  - Vehicle simulation with suspension and wheel physics
  - Raycasting and shape casting
  - Contact queries
  - CCD (Continuous Collision Detection)
  - Debug visualization
