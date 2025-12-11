# Bestow Physics System Developer Guide

A comprehensive guide to using physics in Bestow games.

## Table of Contents
1. [Overview](#overview)
2. [Core Concepts](#core-concepts)
3. [2D Physics (Box2D)](#2d-physics-box2d)
4. [3D Physics (Jolt)](#3d-physics-jolt)
5. [Collision Detection](#collision-detection)
6. [Best Practices](#best-practices)
7. [Common Patterns](#common-patterns)
8. [Code Examples](#code-examples)

---

## Overview

Bestow provides two physics systems for different game types:

| System | Backend | Use Case |
|--------|---------|----------|
| **2D Physics** | Box2D 3.0 | Platformers, top-down games, 2D arcade games |
| **3D Physics** | Jolt Physics | 3D games, first-person, third-person, racing |

Both systems follow the same architectural pattern:
- **Contract-based**: Use `IPhysicsSystem` and `IPhysics3DSystem` interfaces
- **Entity-centric**: Physics bodies are attached to entities
- **Event-driven**: Collision and trigger callbacks for game logic
- **High performance**: Optimized C++ backends with sub-stepping

### Units and Coordinate System

#### 2D Physics
- **Units**: Pixels (internally converted to meters at 100 pixels/meter)
- **Coordinate System**: Y-down screen coordinates (positive Y = down)
- **Gravity**: Default `{0.0f, 980.0f}` (9.8 m/s² × 100 pixels/meter)
- **Angles**: Radians (0 = right, increases counter-clockwise)

#### 3D Physics
- **Units**: Meters (standard physics units)
- **Coordinate System**: Y-up (positive Y = up)
- **Gravity**: Default `{0.0f, -9.8f, 0.0f}`
- **Angles**: Radians and quaternions

---

## Core Concepts

### Body Types

All physics systems support three body types:

```cpp
enum class BodyType {
    Static,      // Immovable objects (walls, floors, platforms)
    Kinematic,   // Script-controlled movement (moving platforms, doors)
    Dynamic      // Gravity and force-driven (player, enemies, projectiles)
};
```

| Type | Movement | Collision | Use Cases |
|------|----------|-----------|-----------|
| **Static** | Never moves | Collides with Dynamic/Kinematic | Terrain, walls, immovable obstacles |
| **Kinematic** | Set velocity directly | Collides with Dynamic | Moving platforms, elevators, doors |
| **Dynamic** | Physics-driven (forces, gravity) | Collides with all types | Player, enemies, physics objects |

### Shapes (2D)

2D physics uses Box2D's shape system:
- **Box**: Rectangle defined by width/height (most common)
- **Circle**: Defined by radius
- **Polygon**: Custom convex polygons
- **Chain**: For terrain edges
- **Edge**: Single line segment

Bestow's 2D system currently uses **box shapes** by default:

```cpp
PhysicsBodyDef def{
    .type = BodyType::Dynamic,
    .size = {32.0f, 40.0f}  // Width × Height in pixels
};
```

### Shapes (3D)

3D physics uses Jolt's comprehensive shape library:
- **Box**: Rectangular prism (half-extents)
- **Sphere**: Defined by radius
- **Capsule**: Cylinder with hemisphere ends (ideal for characters)
- **Cylinder**: Standard cylinder
- **ConvexHull**: Convex mesh
- **Mesh**: Arbitrary triangle mesh (static only)
- **HeightField**: Terrain from height data
- **Compound**: Multiple shapes combined

### Fixtures and Material Properties

Physics bodies have material properties that affect collision response:

```cpp
PhysicsBodyDef def{
    .density = 1.0f,        // Mass per unit area/volume
    .friction = 0.3f,       // Surface friction (0 = ice, 1 = rubber)
    .restitution = 0.0f     // Bounciness (0 = no bounce, 1 = perfectly elastic)
};
```

**Friction** controls sliding:
- `0.0` = Ice (no friction)
- `0.3` = Default (slight friction)
- `0.6` = Wood on wood
- `1.0` = Rubber (high friction)

**Restitution** controls bouncing:
- `0.0` = Inelastic (no bounce)
- `0.3` = Basketball
- `0.7` = Tennis ball
- `1.0` = Perfect bounce (infinite)

### Sensors (Triggers)

Sensors detect overlap without physical collision response:

```cpp
PhysicsBodyDef def{
    .type = BodyType::Static,
    .isSensor = true  // Detects overlap, no collision response
};
physics->createBody(entity, def);

// Listen for sensor events
physics->setTriggerEnterCallback([](const TriggerEvent& event) {
    // event.entityA = sensor body
    // event.entityB = visiting body
});
```

**Use cases**:
- Level boundaries (kill zones, goal areas)
- Item pickups (coins, power-ups)
- Detection zones (enemy sight range, proximity triggers)
- Checkpoints

---

## 2D Physics (Box2D)

### Getting the Physics System

```cpp
import bestow;

class Game : public IApplication {
public:
    Game(IPhysicsSystem& physics, IEntitySystem& entities)
        : physics_(&physics), entities_(&entities) {}

private:
    IPhysicsSystem* physics_;
    IEntitySystem* entities_;
};
```

### Creating Physics Bodies

#### Basic Dynamic Body

```cpp
Entity player = entities_->createEntity();

PhysicsBodyDef def{
    .type = BodyType::Dynamic,
    .transform = {.x = 100.0f, .y = 200.0f},
    .size = {32.0f, 40.0f},      // 32px wide, 40px tall
    .fixedRotation = true,        // Prevent rotation (for characters)
    .friction = 0.3f,
    .restitution = 0.0f
};

physics_->createBody(player, def);
```

#### Static Ground Platform

```cpp
Entity ground = entities_->createEntity();

PhysicsBodyDef def{
    .type = BodyType::Static,
    .transform = {.x = 400.0f, .y = 500.0f},
    .size = {800.0f, 32.0f}  // Wide platform
};

physics_->createBody(ground, def);
physics_->setCollisionLayer(ground, CollisionLayers::Ground);
```

#### Kinematic Moving Platform

```cpp
Entity platform = entities_->createEntity();

PhysicsBodyDef def{
    .type = BodyType::Kinematic,
    .transform = {.x = 200.0f, .y = 300.0f},
    .size = {100.0f, 16.0f}
};

physics_->createBody(platform, def);

// In update loop:
void update(DeltaTime dt) {
    // Move platform by setting velocity
    Vec2 velocity = {50.0f * sin(time), 0.0f};  // Oscillate left-right
    physics_->setVelocity(platform, velocity);
}
```

### Body Properties

#### Position and Rotation

```cpp
// Get/Set position
Vec2 pos = physics_->getPosition(entity);
physics_->setPosition(entity, {100.0f, 200.0f});

// Get/Set rotation (radians)
float angle = physics_->getRotation(entity);
physics_->setRotation(entity, 1.57f);  // 90 degrees
```

#### Velocity

```cpp
// Get/Set linear velocity
Vec2 velocity = physics_->getVelocity(entity);
physics_->setVelocity(entity, {100.0f, 0.0f});  // Move right

// Get/Set angular velocity (radians/sec)
float angularVel = physics_->getAngularVelocity(entity);
physics_->setAngularVelocity(entity, 2.0f);  // Spin
```

#### Body Type Changes

```cpp
// Change body type at runtime
physics_->setBodyType(entity, BodyType::Kinematic);

BodyType type = physics_->getBodyType(entity);
```

### Applying Forces and Impulses

#### Forces (Gradual Acceleration)

Forces are applied over time and integrated by the physics engine:

```cpp
// Apply force at center of mass
Vec2 force = {1000.0f, 0.0f};
physics_->applyForce(entity, force);

// Apply force at a point (causes rotation)
Vec2 force = {500.0f, 0.0f};
Vec2 point = {0.0f, 10.0f};  // Offset from center
physics_->applyForce(entity, force, point);

// Apply torque (rotational force)
physics_->applyTorque(entity, 100.0f);
```

**Use forces for**:
- Continuous acceleration (jet engines, thrusters)
- Wind effects
- Magnetic/gravity fields

#### Impulses (Instant Velocity Change)

Impulses instantly change velocity:

```cpp
// Apply impulse at center
Vec2 impulse = {500.0f, -300.0f};
physics_->applyImpulse(entity, impulse);

// Apply impulse at point
Vec2 impulse = {200.0f, 0.0f};
Vec2 point = {0.0f, 5.0f};
physics_->applyImpulse(entity, impulse, point);
```

**Use impulses for**:
- Jumping (instant upward velocity)
- Explosions
- Instant direction changes
- Knockback effects

### Collision Filtering

Collision layers and masks control what collides with what:

```cpp
// Common layer definitions (bestow.physics)
namespace CollisionLayers {
    inline constexpr CollisionLayer Player      = 0x0001;
    inline constexpr CollisionLayer Enemy       = 0x0002;
    inline constexpr CollisionLayer Projectile  = 0x0004;
    inline constexpr CollisionLayer Terrain     = 0x0008;
    inline constexpr CollisionLayer Trigger     = 0x0010;
    inline constexpr CollisionLayer Collectible = 0x0020;
    inline constexpr CollisionLayer Ground      = 0x0040;
}

// Set collision layer (what I am)
physics_->setCollisionLayer(player, CollisionLayers::Player);

// Set collision mask (what I collide with)
CollisionMask mask = CollisionLayers::Enemy |
                     CollisionLayers::Terrain |
                     CollisionLayers::Ground;
physics_->setCollisionMask(player, mask);
```

**Example: Player projectile that doesn't hit player**:

```cpp
Entity projectile = entities_->createEntity();
PhysicsBodyDef def{.type = BodyType::Dynamic, .size = {8.0f, 8.0f}};
physics_->createBody(projectile, def);

// Set as projectile layer
physics_->setCollisionLayer(projectile, CollisionLayers::Projectile);

// Only collide with enemies and terrain (not player or other projectiles)
CollisionMask mask = CollisionLayers::Enemy | CollisionLayers::Terrain;
physics_->setCollisionMask(projectile, mask);
```

### Spatial Queries

#### AABB Query (Rectangle)

Find all bodies in a rectangular region:

```cpp
Vec2 min = {100.0f, 100.0f};
Vec2 max = {200.0f, 200.0f};

std::vector<Entity> entities = physics_->queryAABB(min, max);

for (Entity e : entities) {
    // Process each entity in region
}
```

#### Circle Query (Radius)

Find all bodies within a radius:

```cpp
Vec2 center = {200.0f, 300.0f};
float radius = 50.0f;

std::vector<Entity> entities = physics_->queryCircle(center, radius);
```

**Use cases**:
- Area-of-effect damage
- Enemy detection radius
- Proximity-based spawning

#### Raycasting

Cast a ray and find the first hit:

```cpp
Vec2 origin = {100.0f, 100.0f};
Vec2 direction = {1.0f, 0.0f};  // Normalized direction
float maxDistance = 500.0f;

std::optional<RaycastHit> hit = physics_->raycast(origin, direction, maxDistance);

if (hit) {
    Entity hitEntity = hit->entity;
    Vec2 hitPoint = hit->point;
    Vec2 hitNormal = hit->normal;
    float distance = hit->distance;

    // Process hit
}
```

**Find all hits along ray**:

```cpp
std::vector<RaycastHit> hits = physics_->raycastAll(origin, direction, maxDistance);

for (const RaycastHit& hit : hits) {
    // Process each hit (sorted by distance)
}
```

**Raycast with collision filtering**:

```cpp
// Only hit enemies
CollisionMask mask = CollisionLayers::Enemy;
auto hit = physics_->raycast(origin, direction, maxDistance, mask);
```

**Use cases**:
- Line-of-sight checks
- Shooting/projectiles
- Ground detection
- Laser beams

### Ground Detection

Bestow provides a specialized ground check for platformers:

```cpp
GroundCheckParams params{
    .rayDistance = 5.0f,           // How far below to check (pixels)
    .slopeToleranceDeg = 60.0f,    // Max slope angle to consider "ground"
    .groundMask = CollisionLayers::Ground | CollisionLayers::Terrain
};

GroundCheckResult result = physics_->checkGrounded(playerEntity, params);

if (result.grounded) {
    Entity groundEntity = result.groundEntity;  // What we're standing on
    Vec2 contactPoint = result.contactPoint;
    Vec2 surfaceNormal = result.surfaceNormal;
    float slopeAngle = result.slopeAngle;       // Degrees from horizontal

    // Player is on ground - allow jumping
}
```

**How it works**:
1. Casts a ray downward from the body's center
2. Ray length = body half-height + rayDistance
3. Checks if hit entity's layer is in groundMask
4. Validates slope angle against slopeToleranceDeg
5. Returns detailed ground contact information

**Use in platformer movement**:

```cpp
void updatePlayerMovement(Entity player, DeltaTime dt) {
    auto grounded = physics_->checkGrounded(player);

    if (grounded.grounded && input->isKeyPressed(Key::Space)) {
        // Jump
        physics_->applyImpulse(player, {0.0f, -500.0f});
    }

    // Move horizontally
    Vec2 velocity = physics_->getVelocity(player);
    float moveSpeed = grounded.grounded ? 200.0f : 100.0f;  // Slower in air
    velocity.x = input->getAxis(Axis::Horizontal) * moveSpeed;
    physics_->setVelocity(player, velocity);
}
```

### World Settings

```cpp
// Set gravity (pixels/s²)
physics_->setGravity({0.0f, 980.0f});  // Standard Earth gravity (Y-down)
physics_->setGravity({0.0f, 0.0f});    // Zero gravity (space game)
physics_->setGravity({0.0f, -500.0f}); // Low gravity (moon)

Vec2 gravity = physics_->getGravity();
```

### Updating Physics

Physics must be updated each frame:

```cpp
void update(DeltaTime dt) {
    // Update physics simulation
    physics_->update(dt);

    // Physics has updated body positions
    // Sync with rendering if needed
}
```

**Fixed timestep recommended**:

```cpp
class Game {
    float accumulator_ = 0.0f;
    const float FIXED_DT = 1.0f / 60.0f;  // 60 FPS physics

    void update(DeltaTime dt) {
        accumulator_ += dt;

        while (accumulator_ >= FIXED_DT) {
            physics_->update(FIXED_DT);
            accumulator_ -= FIXED_DT;
        }
    }
};
```

---

## 3D Physics (Jolt)

### Getting the 3D Physics System

```cpp
import bestow;

class Game3D : public IApplication {
public:
    Game3D(IPhysics3DSystem& physics, IEntitySystem& entities)
        : physics_(&physics), entities_(&entities) {}

private:
    IPhysics3DSystem* physics_;
    IEntitySystem* entities_;
};
```

### Creating 3D Bodies

#### Dynamic Box

```cpp
Entity crate = entities_->createEntity();

PhysicsBodyDef3D def{
    .type = BodyType3D::Dynamic,
    .transform = {.position = {0.0f, 5.0f, 0.0f}},
    .shapeType = ShapeType3D::Box,
    .shapeHalfExtents = {0.5f, 0.5f, 0.5f},  // 1m × 1m × 1m box
    .density = 1000.0f,
    .friction = 0.5f,
    .restitution = 0.3f
};

physics_->createBody(crate, def);
```

#### Static Ground Plane

```cpp
Entity ground = entities_->createEntity();

PhysicsBodyDef3D def{
    .type = BodyType3D::Static,
    .transform = {.position = {0.0f, 0.0f, 0.0f}},
    .shapeType = ShapeType3D::Box,
    .shapeHalfExtents = {50.0f, 0.1f, 50.0f}  // Large thin box
};

physics_->createBody(ground, def);
```

#### Character Capsule

```cpp
Entity character = entities_->createEntity();

PhysicsBodyDef3D def{
    .type = BodyType3D::Dynamic,
    .transform = {.position = {0.0f, 2.0f, 0.0f}},
    .shapeType = ShapeType3D::Capsule,
    .shapeRadius = 0.3f,
    .shapeHalfHeight = 0.9f,  // Total height = 2.4m (0.9*2 + 0.3*2)
    .friction = 0.0f,          // No friction for smooth movement
    .linearDamping = 0.05f,
    .angularDamping = 1.0f,
    .gravityFactor = 1.0f
};

physics_->createBody(character, def);
```

### 3D Body Properties

```cpp
// Position
auto result = physics_->getPosition(entity);
if (result) {
    Vec3 pos = result.value();
}
physics_->setPosition(entity, {10.0f, 5.0f, -20.0f});

// Rotation (quaternion)
auto rotResult = physics_->getRotation(entity);
if (rotResult) {
    Quat rotation = rotResult.value();
}
physics_->setRotation(entity, {1.0f, 0.0f, 0.0f, 0.0f});  // Identity

// Velocity
physics_->setLinearVelocity(entity, {5.0f, 0.0f, 0.0f});
auto velResult = physics_->getLinearVelocity(entity);

physics_->setAngularVelocity(entity, {0.0f, 1.0f, 0.0f});
```

### 3D Forces and Impulses

```cpp
// Apply force at center
physics_->applyForce(entity, {100.0f, 0.0f, 0.0f});

// Apply force at point
physics_->applyForceAtPoint(entity, {50.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

// Apply torque
physics_->applyTorque(entity, {0.0f, 10.0f, 0.0f});

// Impulses
physics_->applyImpulse(entity, {5.0f, 10.0f, 0.0f});
physics_->applyImpulseAtPoint(entity, {3.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f});
physics_->applyAngularImpulse(entity, {0.0f, 2.0f, 0.0f});
```

### 3D Raycasting

```cpp
Vec3 origin = {0.0f, 10.0f, 0.0f};
Vec3 direction = {0.0f, -1.0f, 0.0f};  // Down
float maxDistance = 20.0f;

QueryFilter3D filter{
    .layerMask = CollisionLayers3D::Static | CollisionLayers3D::Dynamic,
    .ignoreSensors = true
};

auto hit = physics_->raycast(origin, direction, maxDistance, filter);

if (hit) {
    Entity hitEntity = hit->entity;
    Vec3 hitPoint = hit->point;
    Vec3 hitNormal = hit->normal;
    float distance = hit->distance;
}
```

### Shape Casting (3D)

More advanced than raycasting - casts a volume instead of a line:

#### Sphere Cast

```cpp
Vec3 origin = {0.0f, 1.0f, 0.0f};
float radius = 0.5f;
Vec3 direction = {1.0f, 0.0f, 0.0f};
float maxDistance = 10.0f;

auto hit = physics_->sphereCast(origin, radius, direction, maxDistance);
```

#### Box Cast

```cpp
Vec3 origin = {0.0f, 1.0f, 0.0f};
Vec3 halfExtents = {0.5f, 0.5f, 0.5f};
Quat rotation = {1.0f, 0.0f, 0.0f, 0.0f};
Vec3 direction = {0.0f, 0.0f, 1.0f};
float maxDistance = 5.0f;

auto hit = physics_->boxCast(origin, halfExtents, rotation, direction, maxDistance);
```

**Use cases**:
- Character controller ground detection (sphere cast down)
- Predictive collision (cast player shape forward)
- Thick raycast for bullets

### 3D Overlap Queries

```cpp
// Sphere overlap
Vec3 center = {0.0f, 5.0f, 0.0f};
float radius = 3.0f;
auto entities = physics_->overlapSphere(center, radius);

// Box overlap
Vec3 center = {0.0f, 0.0f, 0.0f};
Vec3 halfExtents = {2.0f, 2.0f, 2.0f};
Quat rotation = {1.0f, 0.0f, 0.0f, 0.0f};
auto entities = physics_->overlapBox(center, halfExtents, rotation);

// AABB query
Vec3 min = {-5.0f, 0.0f, -5.0f};
Vec3 max = {5.0f, 10.0f, 5.0f};
auto entities = physics_->queryAABB(min, max);
```

### Compound Shapes (Multi-Shape Bodies)

Create complex collision shapes by combining primitives:

```cpp
Entity vehicle = entities_->createEntity();

// Create compound shape definition
CompoundShapeDef compoundDef;

// Add main body box
BoxShapeDef bodyBox;
bodyBox.halfExtents = {1.0f, 0.5f, 2.0f};
bodyBox.localPosition = {0.0f, 0.5f, 0.0f};
compoundDef.boxes.push_back(bodyBox);

// Add cabin box
BoxShapeDef cabinBox;
cabinBox.halfExtents = {0.8f, 0.3f, 0.8f};
cabinBox.localPosition = {0.0f, 1.1f, 0.0f};
compoundDef.boxes.push_back(cabinBox);

Transform3D transform{.position = {0.0f, 1.0f, 0.0f}};
physics_->createCompoundBody(vehicle, BodyType3D::Dynamic, transform, compoundDef);
```

### Character Controllers

Specialized physics controller for character movement:

```cpp
CharacterControllerDef charDef{
    .radius = 0.3f,
    .height = 1.8f,
    .stepHeight = 0.35f,
    .maxSlopeAngle = 45.0f,
    .mass = 80.0f,
    .layer = CollisionLayers3D::Character
};

physics_->createCharacter(playerEntity, charDef);

// Move character
Vec3 desiredVelocity = {5.0f, 0.0f, 0.0f};
physics_->moveCharacter(playerEntity, desiredVelocity, dt);

// Check if grounded
auto groundInfo = physics_->getCharacterGroundInfo(playerEntity);
if (groundInfo && groundInfo->grounded) {
    // Can jump
}
```

### Constraints (Joints)

Connect bodies with constraints:

#### Fixed Constraint

```cpp
ConstraintDef3D fixedDef;
fixedDef.type = ConstraintType3D::Fixed;
fixedDef.bodyA = bodyA;
fixedDef.bodyB = bodyB;

auto constraintId = physics_->createConstraint(fixedDef);
```

#### Hinge (Door, Wheel)

```cpp
HingeConstraintDef hingeDef;
hingeDef.bodyA = door;
hingeDef.bodyB = frame;
hingeDef.pivotA = {-0.5f, 0.0f, 0.0f};  // Left edge of door
hingeDef.pivotB = {0.0f, 0.0f, 0.0f};
hingeDef.axisA = {0.0f, 1.0f, 0.0f};    // Vertical axis
hingeDef.axisB = {0.0f, 1.0f, 0.0f};
hingeDef.hasLimits = true;
hingeDef.minAngle = 0.0f;
hingeDef.maxAngle = 1.57f;  // 90 degrees

auto hinge = physics_->createConstraint(hingeDef);
```

#### Slider (Piston)

```cpp
SliderConstraintDef sliderDef;
sliderDef.bodyA = piston;
sliderDef.bodyB = cylinder;
sliderDef.axisA = {0.0f, 1.0f, 0.0f};
sliderDef.hasLimits = true;
sliderDef.minDistance = 0.0f;
sliderDef.maxDistance = 2.0f;

auto slider = physics_->createConstraint(sliderDef);
```

### 3D Physics Update

```cpp
void update(DeltaTime dt) {
    // Update physics (with substeps for stability)
    int substeps = 2;
    physics_->update(dt, substeps);

    // Sync transforms to entities
    std::vector<Entity> dynamicEntities = getDynamicEntities();
    physics_->syncTransforms(dynamicEntities);
}
```

---

## Collision Detection

### Collision Callbacks (2D)

```cpp
physics_->setCollisionCallback([this](const CollisionEvent& event) {
    Entity entityA = event.entityA;
    Entity entityB = event.entityB;
    Vec2 contactPoint = event.contactPoint;
    Vec2 normal = event.normal;
    float impulse = event.impulse;

    // Handle collision
    if (isPlayer(entityA) && isEnemy(entityB)) {
        damagePlayer(entityA);
    }
});
```

### Trigger Callbacks (2D)

Sensors generate trigger events instead of collision events:

```cpp
// Set trigger enter callback
physics_->setTriggerEnterCallback([this](const TriggerEvent& event) {
    Entity sensor = event.entityA;
    Entity visitor = event.entityB;

    // Player entered checkpoint
    if (isCheckpoint(sensor) && isPlayer(visitor)) {
        activateCheckpoint(sensor);
    }
});

// Set trigger exit callback
physics_->setTriggerExitCallback([this](const TriggerEvent& event) {
    Entity sensor = event.entityA;
    Entity visitor = event.entityB;

    // Player left danger zone
    if (isDangerZone(sensor) && isPlayer(visitor)) {
        deactivateDangerEffect();
    }
});
```

### 3D Collision Callbacks

```cpp
physics_->setCollisionCallback([](const CollisionEvent3D& event) {
    Entity entityA = event.entityA;
    Entity entityB = event.entityB;
    // Handle 3D collision
});

physics_->setTriggerEnterCallback([](const TriggerEvent3D& event) {
    // Handle trigger enter
});

physics_->setTriggerExitCallback([](const TriggerEvent3D& event) {
    // Handle trigger exit
});
```

### Contact Filtering

Prevent specific objects from colliding using layers/masks:

```cpp
// Player doesn't collide with collectibles (but triggers sensor events)
Entity coin = entities_->createEntity();
PhysicsBodyDef coinDef{
    .type = BodyType::Static,
    .size = {16.0f, 16.0f},
    .isSensor = true  // Trigger only
};
physics_->createBody(coin, coinDef);
physics_->setCollisionLayer(coin, CollisionLayers::Collectible);

// Player setup
physics_->setCollisionLayer(player, CollisionLayers::Player);
CollisionMask playerMask = CollisionLayers::Enemy |
                           CollisionLayers::Terrain |
                           CollisionLayers::Ground;
// Note: Collectibles NOT in mask - won't physically collide
physics_->setCollisionMask(player, playerMask);

// But triggers still work!
physics_->setTriggerEnterCallback([](const TriggerEvent& event) {
    if (isCoin(event.entityA)) {
        collectCoin(event.entityA, event.entityB);
    }
});
```

---

## Best Practices

### 1. Physics Scale and Units

#### 2D Games
- **Use pixels as the logical unit** (100 pixels = 1 meter internally)
- **Keep objects between 1-100 meters** in Box2D units (100-10,000 pixels)
- **Avoid tiny objects** (< 10 pixels) - they're unstable
- **Avoid huge objects** (> 10,000 pixels) - they reduce precision

```cpp
// Good: 32×40 pixel character (0.32×0.4 meters)
PhysicsBodyDef playerDef{.size = {32.0f, 40.0f}};

// Bad: 2×3 pixel character (too small)
PhysicsBodyDef tinyDef{.size = {2.0f, 3.0f}};

// Bad: 50,000×1,000 pixel object (too large)
PhysicsBodyDef hugeDef{.size = {50000.0f, 1000.0f}};
```

#### 3D Games
- **Use meters** (real-world scale)
- **Character: 1.5-2.0 meters tall**
- **Objects: 0.1-10 meters** for best stability
- **Avoid objects smaller than 0.01 meters or larger than 1000 meters**

### 2. Fixed Timestep Integration

Always use a fixed timestep for physics simulation:

```cpp
class Game {
    float accumulator_ = 0.0f;
    const float FIXED_DT = 1.0f / 60.0f;  // 60 FPS physics
    const float MAX_ACCUMULATOR = 0.25f;  // Prevent spiral of death

    void update(DeltaTime dt) {
        // Cap delta time to prevent huge jumps
        dt = std::min(dt, MAX_ACCUMULATOR);
        accumulator_ += dt;

        // Fixed timestep updates
        while (accumulator_ >= FIXED_DT) {
            physics_->update(FIXED_DT);
            accumulator_ -= FIXED_DT;
        }
    }
};
```

**Why fixed timestep?**
- Deterministic physics (same input = same output)
- Stable simulation (no jittering from variable dt)
- Prevents tunneling at low frame rates
- Better networked multiplayer synchronization

### 3. Avoiding Tunneling

Tunneling occurs when fast objects pass through thin objects:

**Solutions**:

#### Use Continuous Collision Detection (CCD)

```cpp
// 3D: Enable CCD for fast-moving objects
PhysicsBodyDef3D bulletDef{
    .type = BodyType3D::Dynamic,
    .motionQuality = MotionQuality::LinearCast  // Enable CCD
};
```

#### Increase Sub-Steps

```cpp
// More substeps = smaller time slices = less tunneling
physics_->update(dt, 4);  // 4 substeps per frame
```

#### Limit Maximum Velocity

```cpp
void limitVelocity(Entity entity, float maxSpeed) {
    Vec2 vel = physics_->getVelocity(entity);
    float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);

    if (speed > maxSpeed) {
        vel = vel * (maxSpeed / speed);
        physics_->setVelocity(entity, vel);
    }
}
```

#### Make Walls Thicker

```cpp
// Instead of thin walls:
PhysicsBodyDef thinWall{.size = {10.0f, 500.0f}};  // 10px thick - BAD

// Use thicker walls:
PhysicsBodyDef thickWall{.size = {32.0f, 500.0f}};  // 32px thick - GOOD
```

### 4. Performance Considerations

#### Minimize Body Count

```cpp
// Bad: Create 1000 individual bodies for a brick wall
for (int i = 0; i < 1000; ++i) {
    createBrick();  // 1000 physics bodies!
}

// Good: Create 1 static compound body
Entity wall = entities_->createEntity();
// ... create compound shape with all bricks
```

#### Use Appropriate Body Types

```cpp
// Bad: Static objects as Dynamic
PhysicsBodyDef wallDef{
    .type = BodyType::Dynamic  // Unnecessary computation
};

// Good: Static objects as Static
PhysicsBodyDef wallDef{
    .type = BodyType::Static   // No integration needed
};
```

#### Disable Sleeping Carefully

```cpp
// Bodies automatically sleep when at rest
// Only disable for objects that must always be active
PhysicsBodyDef3D alwaysActiveDef{
    .allowSleep = false  // Disable sleeping (use sparingly)
};
```

#### Use Sensors for Non-Physical Detection

```cpp
// Bad: Full physics body for pickup detection
PhysicsBodyDef coinDef{
    .type = BodyType::Dynamic,  // Unnecessary physics
    .density = 0.1f
};

// Good: Sensor for pickup detection
PhysicsBodyDef coinDef{
    .type = BodyType::Static,
    .isSensor = true  // No collision response, just detection
};
```

### 5. Damping for Controlled Movement

Use damping to slow objects naturally:

```cpp
PhysicsBodyDef playerDef{
    .linearDamping = 0.1f,   // Reduces linear velocity over time
    .angularDamping = 0.5f   // Reduces rotation over time
};
```

**Linear damping** (for movement):
- `0.0` = No damping (space, ice)
- `0.1` = Slight damping (default)
- `0.5` = Medium damping (water)
- `1.0` = Heavy damping (thick liquid)

**Angular damping** (for rotation):
- `0.0` = Spins forever
- `0.5` = Gradual slowdown
- `1.0` = Stops rotation quickly

### 6. Material Combination

When two bodies collide, Box2D/Jolt combines their materials:

```cpp
// Combined friction = sqrt(frictionA * frictionB)
// Combined restitution = max(restitutionA, restitutionB)

// Ice floor (low friction) + rubber ball (high friction)
// Combined friction will be low (ice dominates)

// Concrete floor (no bounce) + bouncy ball (high restitution)
// Combined restitution will be high (ball bounces)
```

---

## Common Patterns

### Platformer Physics

Complete platformer character controller:

```cpp
class PlatformerController {
public:
    PlatformerController(IPhysicsSystem* physics, IInputSystem* input)
        : physics_(physics), input_(input) {}

    void update(Entity player, DeltaTime dt) {
        // Ground check
        GroundCheckParams groundParams{
            .rayDistance = 5.0f,
            .slopeToleranceDeg = 50.0f,
            .groundMask = CollisionLayers::Ground | CollisionLayers::Terrain
        };
        auto grounded = physics_->checkGrounded(player, groundParams);

        // Get current velocity
        Vec2 velocity = physics_->getVelocity(player);

        // Horizontal movement
        float moveInput = input_->getAxis(Axis::Horizontal);

        if (grounded.grounded) {
            // On ground: direct control
            velocity.x = moveInput * MOVE_SPEED;

            // Jump
            if (input_->isKeyPressed(Key::Space)) {
                velocity.y = JUMP_VELOCITY;
            }
        } else {
            // In air: reduced control
            float airControl = 0.3f;
            velocity.x += moveInput * MOVE_SPEED * airControl * dt;

            // Clamp air speed
            velocity.x = std::clamp(velocity.x, -MOVE_SPEED, MOVE_SPEED);
        }

        // Apply velocity
        physics_->setVelocity(player, velocity);
    }

private:
    IPhysicsSystem* physics_;
    IInputSystem* input_;

    const float MOVE_SPEED = 200.0f;
    const float JUMP_VELOCITY = -500.0f;  // Negative = up (Y-down coords)
};
```

### Top-Down Movement

Character movement for top-down games:

```cpp
class TopDownController {
public:
    void update(Entity character, DeltaTime dt) {
        // Get input
        Vec2 moveDir = {
            input_->getAxis(Axis::Horizontal),
            input_->getAxis(Axis::Vertical)
        };

        // Normalize diagonal movement
        float length = std::sqrt(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
        if (length > 1.0f) {
            moveDir = moveDir / length;
        }

        // Set velocity directly (kinematic-style movement)
        Vec2 velocity = moveDir * MOVE_SPEED;
        physics_->setVelocity(character, velocity);

        // Optional: Face movement direction
        if (length > 0.1f) {
            float angle = std::atan2(moveDir.y, moveDir.x);
            physics_->setRotation(character, angle);
        }
    }

private:
    const float MOVE_SPEED = 150.0f;
};
```

### One-Way Platforms

Platforms you can jump through from below:

```cpp
class OneWayPlatform {
public:
    void setup(Entity platform) {
        // Create platform as sensor
        PhysicsBodyDef def{
            .type = BodyType::Static,
            .size = {100.0f, 16.0f},
            .isSensor = true  // Allows passing through
        };
        physics_->createBody(platform, def);
        physics_->setCollisionLayer(platform, CollisionLayers::Ground);

        // Track platform
        oneWayPlatforms_.insert(platform);
    }

    void handleCollision(const CollisionEvent& event) {
        // Detect player collision with one-way platform
        Entity platform = isOneWayPlatform(event.entityA) ? event.entityA : event.entityB;
        Entity player = isOneWayPlatform(event.entityA) ? event.entityB : event.entityA;

        if (platform && player) {
            Vec2 playerPos = physics_->getPosition(player);
            Vec2 platformPos = physics_->getPosition(platform);
            Vec2 playerVel = physics_->getVelocity(player);

            // Only collide if:
            // 1. Player is above platform
            // 2. Player is moving down (or stationary)
            bool abovePlatform = playerPos.y < platformPos.y;
            bool movingDown = playerVel.y >= 0.0f;

            if (abovePlatform && movingDown) {
                // Enable collision (make platform solid)
                physics_->setSensor(platform, false);
            } else {
                // Disable collision (let player pass through)
                physics_->setSensor(platform, true);
            }
        }
    }

private:
    std::set<Entity> oneWayPlatforms_;
};
```

**Better approach: Use collision filtering with raycast**:

```cpp
void updateOneWayPlatform(Entity player, Entity platform) {
    Vec2 playerPos = physics_->getPosition(player);
    Vec2 platformPos = physics_->getPosition(platform);

    // Player is above platform AND pressing down?
    bool pressingDown = input_->isKeyPressed(Key::Down);
    bool abovePlatform = playerPos.y < (platformPos.y - 10.0f);

    if (pressingDown && !abovePlatform) {
        // Temporarily disable collision
        CollisionMask mask = physics_->getCollisionMask(player);
        mask &= ~CollisionLayers::Ground;  // Remove Ground from mask
        physics_->setCollisionMask(player, mask);

        // Re-enable after 0.2 seconds
        scheduledEvents_.push({0.2f, [this, player]() {
            CollisionMask mask = physics_->getCollisionMask(player);
            mask |= CollisionLayers::Ground;
            physics_->setCollisionMask(player, mask);
        }});
    }
}
```

### Moving Platforms

Platforms that carry the player:

```cpp
class MovingPlatform {
public:
    void createPlatform(Entity platform) {
        PhysicsBodyDef def{
            .type = BodyType::Kinematic,  // Move via velocity
            .transform = {.x = 200.0f, .y = 300.0f},
            .size = {100.0f, 16.0f}
        };
        physics_->createBody(platform, def);
        physics_->setCollisionLayer(platform, CollisionLayers::Ground);

        // Store initial position for oscillation
        platformData_[platform] = {.startPos = {200.0f, 300.0f}};
    }

    void updatePlatform(Entity platform, DeltaTime dt) {
        auto& data = platformData_[platform];
        data.time += dt;

        // Oscillate left-right
        float offset = std::sin(data.time * 2.0f) * 100.0f;
        Vec2 targetPos = data.startPos + Vec2{offset, 0.0f};

        // Calculate velocity to reach target
        Vec2 currentPos = physics_->getPosition(platform);
        Vec2 velocity = (targetPos - currentPos) / dt;

        physics_->setVelocity(platform, velocity);
    }

    void attachPlayerToPlatform(Entity player, Entity platform) {
        // Store platform velocity to apply to player
        Vec2 platformVel = physics_->getVelocity(platform);
        Vec2 playerVel = physics_->getVelocity(player);

        // Add platform velocity to player (player moves with platform)
        playerVel.x += platformVel.x;
        physics_->setVelocity(player, playerVel);
    }

private:
    struct PlatformData {
        Vec2 startPos;
        float time = 0.0f;
    };
    std::unordered_map<Entity, PlatformData> platformData_;
};
```

### Projectile System

Simple projectile physics:

```cpp
class ProjectileSystem {
public:
    Entity createProjectile(Vec2 position, Vec2 direction, float speed) {
        Entity projectile = entities_->createEntity();

        PhysicsBodyDef def{
            .type = BodyType::Dynamic,
            .transform = {.x = position.x, .y = position.y},
            .size = {8.0f, 8.0f},
            .density = 0.1f,
            .friction = 0.0f,
            .restitution = 0.0f
        };

        physics_->createBody(projectile, def);
        physics_->setCollisionLayer(projectile, CollisionLayers::Projectile);

        // Projectiles don't collide with each other or the shooter
        CollisionMask mask = CollisionLayers::Enemy | CollisionLayers::Terrain;
        physics_->setCollisionMask(projectile, mask);

        // Set initial velocity
        Vec2 velocity = direction * speed;
        physics_->setVelocity(projectile, velocity);

        // Disable gravity (straight projectile)
        // Alternative: Use gravity for arcing projectiles

        return projectile;
    }

    void handleProjectileHit(const CollisionEvent& event) {
        Entity projectile = event.entityA;
        Entity target = event.entityB;

        // Apply damage
        if (auto* health = entities_->get<HealthComponent>(target)) {
            health->takeDamage(projectileDamage_);
        }

        // Destroy projectile
        physics_->destroyBody(projectile);
        entities_->destroyEntity(projectile);
    }

private:
    float projectileDamage_ = 10.0f;
};
```

### Explosion Force

Apply radial force from an explosion:

```cpp
void applyExplosionForce(Vec2 center, float radius, float force) {
    // Query all entities in radius
    std::vector<Entity> affected = physics_->queryCircle(center, radius);

    for (Entity entity : affected) {
        Vec2 entityPos = physics_->getPosition(entity);
        Vec2 direction = entityPos - center;

        float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        if (distance < 0.001f) continue;  // Avoid division by zero

        // Normalize direction
        direction = direction / distance;

        // Calculate force (inverse square falloff)
        float falloff = 1.0f - (distance / radius);
        float magnitude = force * falloff * falloff;

        // Apply impulse
        Vec2 impulse = direction * magnitude;
        physics_->applyImpulse(entity, impulse);
    }
}
```

### Ragdoll Physics (3D)

Create a simple ragdoll from connected bodies:

```cpp
class Ragdoll {
public:
    void createRagdoll(Vec3 position) {
        // Create body parts
        Entity torso = createBodyPart({0.3f, 0.5f, 0.2f}, position);
        Entity head = createBodyPart({0.2f, 0.2f, 0.2f}, position + Vec3{0.0f, 0.7f, 0.0f});
        Entity armL = createBodyPart({0.1f, 0.3f, 0.1f}, position + Vec3{-0.4f, 0.3f, 0.0f});
        Entity armR = createBodyPart({0.1f, 0.3f, 0.1f}, position + Vec3{0.4f, 0.3f, 0.0f});
        Entity legL = createBodyPart({0.15f, 0.4f, 0.15f}, position + Vec3{-0.2f, -0.9f, 0.0f});
        Entity legR = createBodyPart({0.15f, 0.4f, 0.15f}, position + Vec3{0.2f, -0.9f, 0.0f});

        // Connect with constraints
        createHingeJoint(torso, head, {0.0f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f});
        createHingeJoint(torso, armL, {-0.3f, 0.3f, 0.0f}, {1.0f, 0.0f, 0.0f});
        createHingeJoint(torso, armR, {0.3f, 0.3f, 0.0f}, {1.0f, 0.0f, 0.0f});
        createHingeJoint(torso, legL, {-0.2f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f});
        createHingeJoint(torso, legR, {0.2f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f});
    }

private:
    Entity createBodyPart(Vec3 halfExtents, Vec3 position) {
        Entity part = entities_->createEntity();
        PhysicsBodyDef3D def{
            .type = BodyType3D::Dynamic,
            .transform = {.position = position},
            .shapeType = ShapeType3D::Box,
            .shapeHalfExtents = halfExtents,
            .density = 1000.0f
        };
        physics_->createBody(part, def);
        return part;
    }

    void createHingeJoint(Entity bodyA, Entity bodyB, Vec3 pivot, Vec3 axis) {
        HingeConstraintDef hingeDef;
        hingeDef.bodyA = bodyA;
        hingeDef.bodyB = bodyB;
        hingeDef.pivotA = pivot;
        hingeDef.pivotB = {0.0f, 0.0f, 0.0f};
        hingeDef.axisA = axis;
        hingeDef.axisB = axis;
        hingeDef.hasLimits = true;
        hingeDef.minAngle = -1.0f;
        hingeDef.maxAngle = 1.0f;

        physics_->createConstraint(hingeDef);
    }
};
```

---

## Code Examples

### Complete 2D Platformer Example

```cpp
import bestow;

class PlatformerGame : public IApplication {
public:
    PlatformerGame(
        IPhysicsSystem& physics,
        IEntitySystem& entities,
        IInputSystem& input
    ) : physics_(&physics), entities_(&entities), input_(&input) {}

    void initialize() override {
        // Setup gravity
        physics_->setGravity({0.0f, 980.0f});

        // Create player
        player_ = entities_->createEntity();
        PhysicsBodyDef playerDef{
            .type = BodyType::Dynamic,
            .transform = {.x = 100.0f, .y = 100.0f},
            .size = {32.0f, 40.0f},
            .fixedRotation = true,
            .friction = 0.3f,
            .density = 1.0f
        };
        physics_->createBody(player_, playerDef);
        physics_->setCollisionLayer(player_, CollisionLayers::Player);

        // Create ground
        Entity ground = entities_->createEntity();
        PhysicsBodyDef groundDef{
            .type = BodyType::Static,
            .transform = {.x = 400.0f, .y = 550.0f},
            .size = {800.0f, 100.0f}
        };
        physics_->createBody(ground, groundDef);
        physics_->setCollisionLayer(ground, CollisionLayers::Ground);

        // Create platforms
        createPlatform({200.0f, 400.0f}, {150.0f, 20.0f});
        createPlatform({500.0f, 300.0f}, {150.0f, 20.0f});
        createPlatform({300.0f, 200.0f}, {150.0f, 20.0f});

        // Setup collision callback
        physics_->setCollisionCallback([this](const CollisionEvent& event) {
            handleCollision(event);
        });
    }

    void update(DeltaTime dt) override {
        handleInput(dt);
        physics_->update(dt);
    }

private:
    void createPlatform(Vec2 position, Vec2 size) {
        Entity platform = entities_->createEntity();
        PhysicsBodyDef def{
            .type = BodyType::Static,
            .transform = {.x = position.x, .y = position.y},
            .size = size
        };
        physics_->createBody(platform, def);
        physics_->setCollisionLayer(platform, CollisionLayers::Ground);
    }

    void handleInput(DeltaTime dt) {
        // Check if grounded
        GroundCheckParams groundParams{
            .rayDistance = 5.0f,
            .slopeToleranceDeg = 50.0f,
            .groundMask = CollisionLayers::Ground
        };
        auto grounded = physics_->checkGrounded(player_, groundParams);

        // Get current velocity
        Vec2 velocity = physics_->getVelocity(player_);

        // Horizontal movement
        float moveInput = 0.0f;
        if (input_->isKeyDown(Key::Right)) moveInput += 1.0f;
        if (input_->isKeyDown(Key::Left)) moveInput -= 1.0f;

        if (grounded.grounded) {
            // On ground: direct control
            velocity.x = moveInput * MOVE_SPEED;

            // Jump
            if (input_->isKeyPressed(Key::Space)) {
                velocity.y = JUMP_VELOCITY;
            }
        } else {
            // In air: reduced control
            velocity.x += moveInput * MOVE_SPEED * AIR_CONTROL * dt;
            velocity.x = std::clamp(velocity.x, -MOVE_SPEED, MOVE_SPEED);
        }

        // Apply velocity
        physics_->setVelocity(player_, velocity);
    }

    void handleCollision(const CollisionEvent& event) {
        // Handle collisions
    }

    IPhysicsSystem* physics_;
    IEntitySystem* entities_;
    IInputSystem* input_;
    Entity player_;

    static constexpr float MOVE_SPEED = 200.0f;
    static constexpr float JUMP_VELOCITY = -500.0f;
    static constexpr float AIR_CONTROL = 0.3f;
};
```

### Complete 3D First-Person Example

```cpp
import bestow;

class FirstPersonGame : public IApplication {
public:
    FirstPersonGame(
        IPhysics3DSystem& physics,
        IEntitySystem& entities,
        IInputSystem& input
    ) : physics_(&physics), entities_(&entities), input_(&input) {}

    void initialize() override {
        // Setup gravity
        physics_->setGravity({0.0f, -9.8f, 0.0f});

        // Create character controller
        player_ = entities_->createEntity();
        CharacterControllerDef charDef{
            .radius = 0.3f,
            .height = 1.8f,
            .stepHeight = 0.35f,
            .maxSlopeAngle = 45.0f,
            .mass = 80.0f,
            .layer = CollisionLayers3D::Character
        };
        physics_->createCharacter(player_, charDef);
        physics_->setCharacterPosition(player_, {0.0f, 2.0f, 0.0f});

        // Create ground
        Entity ground = entities_->createEntity();
        PhysicsBodyDef3D groundDef{
            .type = BodyType3D::Static,
            .transform = {.position = {0.0f, 0.0f, 0.0f}},
            .shapeType = ShapeType3D::Box,
            .shapeHalfExtents = {50.0f, 0.5f, 50.0f}
        };
        physics_->createBody(ground, groundDef);

        // Create some obstacles
        for (int i = 0; i < 10; ++i) {
            createCrate({
                static_cast<float>(rand() % 20 - 10),
                1.0f,
                static_cast<float>(rand() % 20 - 10)
            });
        }
    }

    void update(DeltaTime dt) override {
        handleMovement(dt);
        handleLook(dt);
        physics_->update(dt, 2);  // 2 substeps
    }

private:
    void createCrate(Vec3 position) {
        Entity crate = entities_->createEntity();
        PhysicsBodyDef3D def{
            .type = BodyType3D::Dynamic,
            .transform = {.position = position},
            .shapeType = ShapeType3D::Box,
            .shapeHalfExtents = {0.5f, 0.5f, 0.5f},
            .density = 1000.0f
        };
        physics_->createBody(crate, def);
    }

    void handleMovement(DeltaTime dt) {
        // Get input
        Vec3 moveDir = {0.0f, 0.0f, 0.0f};

        if (input_->isKeyDown(Key::W)) moveDir.z -= 1.0f;
        if (input_->isKeyDown(Key::S)) moveDir.z += 1.0f;
        if (input_->isKeyDown(Key::A)) moveDir.x -= 1.0f;
        if (input_->isKeyDown(Key::D)) moveDir.x += 1.0f;

        // Normalize
        float length = std::sqrt(moveDir.x * moveDir.x + moveDir.z * moveDir.z);
        if (length > 0.001f) {
            moveDir = moveDir / length;
        }

        // Apply camera rotation to movement
        moveDir = rotateByYaw(moveDir, cameraYaw_);

        // Get ground info
        auto groundInfo = physics_->getCharacterGroundInfo(player_);

        // Jump
        Vec3 velocity = moveDir * MOVE_SPEED;
        if (groundInfo && groundInfo->grounded && input_->isKeyPressed(Key::Space)) {
            velocity.y = JUMP_VELOCITY;
        }

        // Move character
        physics_->moveCharacter(player_, velocity, dt);
    }

    void handleLook(DeltaTime dt) {
        // Mouse look
        Vec2 mouseDelta = input_->getMouseDelta();
        cameraYaw_ -= mouseDelta.x * LOOK_SENSITIVITY;
        cameraPitch_ -= mouseDelta.y * LOOK_SENSITIVITY;
        cameraPitch_ = std::clamp(cameraPitch_, -1.5f, 1.5f);
    }

    Vec3 rotateByYaw(Vec3 v, float yaw) {
        float c = std::cos(yaw);
        float s = std::sin(yaw);
        return {v.x * c - v.z * s, v.y, v.x * s + v.z * c};
    }

    IPhysics3DSystem* physics_;
    IEntitySystem* entities_;
    IInputSystem* input_;
    Entity player_;

    float cameraYaw_ = 0.0f;
    float cameraPitch_ = 0.0f;

    static constexpr float MOVE_SPEED = 5.0f;
    static constexpr float JUMP_VELOCITY = 5.0f;
    static constexpr float LOOK_SENSITIVITY = 0.002f;
};
```

---

## Summary

Bestow's physics systems provide:

- **2D Physics (Box2D)**: Pixel-based, Y-down coordinates, perfect for platformers and 2D games
- **3D Physics (Jolt)**: Meter-based, Y-up coordinates, full 3D simulation with characters and vehicles
- **Unified API**: Similar patterns across 2D and 3D
- **Performance**: Optimized backends with sub-stepping
- **Flexibility**: Bodies, sensors, queries, raycasts, constraints

**Key Takeaways**:
1. Use fixed timestep (60 Hz recommended)
2. Keep objects within reasonable scale (0.1-10 meters)
3. Use appropriate body types (Static/Kinematic/Dynamic)
4. Enable CCD for fast-moving objects
5. Use sensors for triggers, not collision response
6. Filter collisions with layers/masks
7. Apply damping for controlled movement

For more examples, see the Bestow demos in `examples/` or the unit tests in `tests/unit/PhysicsSystemTests.cpp`.
