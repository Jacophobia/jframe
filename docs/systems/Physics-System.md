# Bestow Physics System

## Overview

The Bestow Physics System provides 2D rigid body physics simulation powered by **Box2D 3.0**. It handles collision detection, response, raycasting, and spatial queries for game entities.

### Key Features

- Rigid body physics (Static, Dynamic, Kinematic)
- Box shapes with configurable size, friction, restitution
- Collision detection with customizable layers and masks
- Sensor/trigger volumes for overlap detection
- Raycasting and spatial queries (AABB, circle)
- Force and impulse application
- Ground detection for platformer mechanics
- Pixel-based coordinate system (auto-converts to Box2D meters)

### Architecture

```
IPhysicsSystem (interface)
    ↓
Box2DPhysicsSystem (implementation)
    ↓
Box2D 3.0 World (b2WorldId)
```

**Coordinate System:**
- Game code uses **pixels** (e.g., position = `{100, 200}`)
- Box2D uses **meters** internally
- Conversion: `100 pixels = 1 meter` (configurable via `PIXELS_PER_METER`)

**Entity Mapping:**
- Each `Entity` has at most one physics body (`b2BodyId`)
- Bodies store collision layers, masks, and sensor flags in metadata

---

## Creating Physics Bodies

### Body Types

| Type | Behavior | Use Case |
|------|----------|----------|
| **Static** | Immovable, zero velocity | Terrain, walls, platforms |
| **Dynamic** | Affected by forces/gravity | Player, enemies, projectiles |
| **Kinematic** | Moves via velocity, ignores forces | Moving platforms, elevators |

### Basic Body Creation

```cpp
import bestow.physics;

// Create a dynamic player body
Entity player = entities->createEntity();
PhysicsBodyDef playerDef{
    .type = BodyType::Dynamic,
    .transform = {.x = 100.0f, .y = 200.0f},  // Position in pixels
    .size = {32.0f, 64.0f},                    // Width/Height (pixels)
    .fixedRotation = true,                     // Prevent rotation
    .density = 1.0f,
    .friction = 0.3f,
    .restitution = 0.0f                        // No bounce
};
physics->createBody(player, playerDef);

// Create a static ground platform
Entity ground = entities->createEntity();
PhysicsBodyDef groundDef{
    .type = BodyType::Static,
    .transform = {.x = 400.0f, .y = 50.0f},
    .size = {800.0f, 32.0f},
    .friction = 0.5f
};
physics->createBody(ground, groundDef);
```

### Body Definition Properties

```cpp
struct PhysicsBodyDef {
    BodyType type = BodyType::Dynamic;
    Transform2D transform;                   // Position and rotation
    Vec2 size = {32.0f, 32.0f};             // Collision box (pixels)
    bool fixedRotation = true;              // Lock rotation (good for platformers)
    float linearDamping = 0.0f;             // Velocity decay (0-1)
    float angularDamping = 0.0f;            // Rotation decay (0-1)
    float density = 1.0f;                   // Mass = density * area
    float friction = 0.3f;                  // Surface friction (0-1)
    float restitution = 0.0f;               // Bounciness (0-1)
    bool isSensor = false;                  // Trigger volume (no collision)
};
```

### Destroying Bodies

```cpp
// Remove physics body when entity is destroyed
physics->destroyBody(player);

// Check if entity has a body
if (physics->hasBody(player)) {
    // Body exists
}
```

**Important:** Creating a body for an entity that already has one will replace the old body.

---

## Body Properties

### Position and Rotation

```cpp
// Get/Set position (pixels)
Vec2 pos = physics->getPosition(player);
physics->setPosition(player, {150.0f, 250.0f});

// Get/Set rotation (radians)
float angle = physics->getRotation(player);
physics->setRotation(player, 1.57f);  // 90 degrees
```

### Velocity

```cpp
// Get/Set linear velocity (pixels/second)
Vec2 vel = physics->getVelocity(player);
physics->setVelocity(player, {200.0f, 0.0f});  // Move right at 200 px/s

// Get/Set angular velocity (radians/second)
float angularVel = physics->getAngularVelocity(player);
physics->setAngularVelocity(player, 3.14f);  // Spin 180°/s
```

### Changing Body Type at Runtime

```cpp
// Convert dynamic body to kinematic (useful for cutscenes)
physics->setBodyType(player, BodyType::Kinematic);

// Check current type
BodyType type = physics->getBodyType(player);
if (type == BodyType::Dynamic) {
    // Apply forces...
}
```

---

## Forces, Impulses, and Torque

### Forces (Gradual Acceleration)

Forces accumulate over time and are cleared each physics step. Good for continuous effects like thrusters or magnets.

```cpp
// Apply force at center of mass
Vec2 force{500.0f, 0.0f};  // Push right
physics->applyForce(player, force);

// Apply force at specific point (causes rotation)
Vec2 force{100.0f, 0.0f};
Vec2 point{0.0f, 10.0f};  // Point offset from center
physics->applyForce(player, force, point);
```

### Impulses (Instant Velocity Change)

Impulses immediately change velocity. Good for jumps, explosions, or knockback.

```cpp
// Jump impulse
Vec2 jumpImpulse{0.0f, -300.0f};  // Upward boost
physics->applyImpulse(player, jumpImpulse);

// Knockback from hit
Vec2 knockback{-150.0f, -100.0f};
physics->applyImpulse(enemy, knockback);
```

### Torque (Rotational Force)

```cpp
// Apply torque (causes rotation)
float torque = 50.0f;
physics->applyTorque(player, torque);
```

### Platformer Movement Example

```cpp
void updatePlayerMovement(Entity player, DeltaTime dt) {
    // Get input
    float moveX = input->getAxis("Horizontal");  // -1 to 1

    // Set horizontal velocity directly for precise platformer control
    Vec2 vel = physics->getVelocity(player);
    vel.x = moveX * 200.0f;  // Max speed: 200 px/s
    physics->setVelocity(player, vel);

    // Jump if grounded
    if (input->isPressed("Jump")) {
        auto groundCheck = physics->checkGrounded(player);
        if (groundCheck.grounded) {
            Vec2 jumpImpulse{0.0f, -300.0f};
            physics->applyImpulse(player, jumpImpulse);
        }
    }
}
```

---

## Collision Detection and Filtering

### Collision Layers

Collision layers define **what something is**. Each body belongs to one layer.

```cpp
// Predefined layers (from bestow.physics)
namespace CollisionLayers {
    inline constexpr CollisionLayer Player      = 0x0001;
    inline constexpr CollisionLayer Enemy       = 0x0002;
    inline constexpr CollisionLayer Projectile  = 0x0004;
    inline constexpr CollisionLayer Terrain     = 0x0008;
    inline constexpr CollisionLayer Trigger     = 0x0010;
    inline constexpr CollisionLayer Collectible = 0x0020;
    inline constexpr CollisionLayer Ground      = 0x0040;
}

// Assign layer to entity
physics->setCollisionLayer(player, CollisionLayers::Player);
physics->setCollisionLayer(ground, CollisionLayers::Ground | CollisionLayers::Terrain);

// Get layer
CollisionLayer layer = physics->getCollisionLayer(player);
```

### Collision Masks

Collision masks define **what something collides with**. Use bitwise OR to combine layers.

```cpp
// Player collides with enemies, terrain, and ground
CollisionMask playerMask = CollisionLayers::Enemy
                         | CollisionLayers::Terrain
                         | CollisionLayers::Ground;
physics->setCollisionMask(player, playerMask);

// Enemy collides with player and terrain only
CollisionMask enemyMask = CollisionLayers::Player | CollisionLayers::Terrain;
physics->setCollisionMask(enemy, enemyMask);

// Projectile collides with everything except other projectiles
CollisionMask projectileMask = 0xFFFF & ~CollisionLayers::Projectile;
physics->setCollisionMask(projectile, projectileMask);
```

### Collision Callbacks

```cpp
// Register collision callback (called when two bodies collide)
physics->setCollisionCallback([](const CollisionEvent& event) {
    Entity entityA = event.entityA;
    Entity entityB = event.entityB;
    Vec2 contactPoint = event.contactPoint;
    Vec2 normal = event.normal;
    float impulse = event.impulse;

    // Handle collision logic
    if (isPlayer(entityA) && isEnemy(entityB)) {
        damagePlayer(entityA);
    }
});
```

**Collision Event Fields:**
- `entityA`, `entityB`: Colliding entities
- `contactPoint`: Where collision occurred (pixels)
- `normal`: Surface normal (unit vector)
- `impulse`: Collision force magnitude

---

## Sensors and Triggers

Sensors detect overlaps without physical collision response. Perfect for triggers, checkpoints, and collectibles.

### Creating a Sensor

```cpp
// Trigger volume at level exit
Entity exitTrigger = entities->createEntity();
PhysicsBodyDef triggerDef{
    .type = BodyType::Static,
    .transform = {.x = 800.0f, .y = 300.0f},
    .size = {64.0f, 128.0f},
    .isSensor = true  // No collision, just detection
};
physics->createBody(exitTrigger, triggerDef);
physics->setCollisionLayer(exitTrigger, CollisionLayers::Trigger);
```

### Trigger Callbacks

Trigger callbacks require downcasting to `Box2DPhysicsSystem`:

```cpp
import bestow.physics.impl;

auto* physicsImpl = dynamic_cast<Box2DPhysicsSystem*>(physics.get());

// Called when something enters the sensor
physicsImpl->setTriggerEnterCallback([](const TriggerEvent& event) {
    Entity sensor = event.entityA;
    Entity visitor = event.entityB;

    if (isPlayer(visitor)) {
        loadNextLevel();
    }
});

// Called when something leaves the sensor
physicsImpl->setTriggerExitCallback([](const TriggerEvent& event) {
    Entity sensor = event.entityA;
    Entity visitor = event.entityB;

    // Player left checkpoint area
});
```

### Toggling Sensor at Runtime

```cpp
// Convert existing body to sensor
physics->setSensor(entity, true);

// Make it solid again
physics->setSensor(entity, false);
```

**Note:** Sensor callbacks only fire for bodies where `isSensor = true` at creation time. Use `setSensor()` to toggle event generation.

---

## Raycasting

Raycasting casts a ray through the physics world and returns hits.

### Single Raycast (Closest Hit)

```cpp
Vec2 origin{100.0f, 200.0f};
Vec2 direction{1.0f, 0.0f};  // Ray direction (will be normalized)
float maxDistance = 500.0f;  // Max ray length (pixels)

std::optional<RaycastHit> hit = physics->raycast(origin, direction, maxDistance);

if (hit) {
    Entity hitEntity = hit->entity;
    Vec2 hitPoint = hit->point;
    Vec2 hitNormal = hit->normal;
    float distance = hit->distance;

    // Visualize hit
    drawLine(origin, hitPoint);
}
```

### Raycast with Collision Mask

```cpp
// Only hit enemies
CollisionMask mask = CollisionLayers::Enemy;
auto hit = physics->raycast(origin, direction, maxDistance, mask);

// Hit everything except triggers
CollisionMask maskNoTriggers = 0xFFFF & ~CollisionLayers::Trigger;
auto hit2 = physics->raycast(origin, direction, maxDistance, maskNoTriggers);
```

### Raycast All (All Hits)

```cpp
// Find all entities along ray
std::vector<RaycastHit> hits = physics->raycastAll(origin, direction, maxDistance);

for (const auto& hit : hits) {
    // Hits are sorted by distance (closest first)
    processHit(hit);
}
```

### Line-of-Sight Check

```cpp
bool hasLineOfSight(Entity from, Entity to) {
    Vec2 fromPos = physics->getPosition(from);
    Vec2 toPos = physics->getPosition(to);

    Vec2 direction = {toPos.x - fromPos.x, toPos.y - fromPos.y};
    float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

    // Check if ray hits anything before target
    CollisionMask mask = CollisionLayers::Terrain;
    auto hit = physics->raycast(fromPos, direction, distance, mask);

    return !hit.has_value();  // No obstacles = clear line of sight
}
```

---

## Spatial Queries

### AABB Query (Axis-Aligned Bounding Box)

Find all entities in a rectangular region.

```cpp
Vec2 min{100.0f, 100.0f};
Vec2 max{300.0f, 300.0f};

std::vector<Entity> entitiesInBox = physics->queryAABB(min, max);

for (Entity entity : entitiesInBox) {
    // Process entities in region
}
```

**Use Cases:**
- Screen-based culling (only process on-screen entities)
- Area-of-effect damage
- Finding nearby interactables

### Circle Query

Find all entities within a radius.

```cpp
Vec2 center{400.0f, 300.0f};
float radius = 150.0f;  // Pixels

std::vector<Entity> entitiesInRadius = physics->queryCircle(center, radius);

for (Entity entity : entitiesInRadius) {
    // Apply explosion force
    Vec2 entityPos = physics->getPosition(entity);
    Vec2 direction = {entityPos.x - center.x, entityPos.y - center.y};
    float dist = std::sqrt(direction.x * direction.x + direction.y * direction.y);

    if (dist > 0.0f) {
        float forceMagnitude = 1000.0f * (1.0f - dist / radius);  // Falloff
        Vec2 force = {direction.x / dist * forceMagnitude,
                      direction.y / dist * forceMagnitude};
        physics->applyImpulse(entity, force);
    }
}
```

**Use Cases:**
- Explosion damage/knockback
- Enemy awareness radius
- Proximity triggers

---

## Ground Detection (Platformers)

The `checkGrounded()` function performs a raycast downward to detect valid ground surfaces.

### Basic Ground Check

```cpp
// Check if player is on ground
GroundCheckResult groundCheck = physics->checkGrounded(player);

if (groundCheck.grounded) {
    // Player is on ground
    Entity groundEntity = groundCheck.groundEntity;
    Vec2 contactPoint = groundCheck.contactPoint;
    Vec2 surfaceNormal = groundCheck.surfaceNormal;
    float slopeAngle = groundCheck.slopeAngle;
}
```

### Custom Ground Check Parameters

```cpp
GroundCheckParams params{
    .rayDistance = 10.0f,               // Check 10 pixels below body
    .slopeToleranceDeg = 45.0f,         // Max 45° slope counts as ground
    .groundMask = CollisionLayers::Ground | CollisionLayers::Terrain
};

GroundCheckResult result = physics->checkGrounded(player, params);
```

### Platformer Jump Logic

```cpp
void handleJump(Entity player) {
    // Only jump if on ground
    GroundCheckParams params{
        .rayDistance = 5.0f,
        .slopeToleranceDeg = 60.0f,
        .groundMask = CollisionLayers::Ground
    };

    auto groundCheck = physics->checkGrounded(player, params);

    if (groundCheck.grounded) {
        // Apply jump impulse
        Vec2 jumpImpulse{0.0f, -350.0f};
        physics->applyImpulse(player, jumpImpulse);

        // Visual feedback: spawn dust at contact point
        spawnDustEffect(groundCheck.contactPoint);
    }
}
```

### Slope Detection

```cpp
void updateSlopeMovement(Entity player) {
    auto groundCheck = physics->checkGrounded(player);

    if (groundCheck.grounded) {
        if (groundCheck.slopeAngle > 30.0f) {
            // Steep slope: slow down player
            Vec2 vel = physics->getVelocity(player);
            vel.x *= 0.5f;
            physics->setVelocity(player, vel);
        }
    }
}
```

---

## World Settings

### Gravity

```cpp
// Get default gravity (standard: {0, -980} pixels/s²)
Vec2 gravity = physics->getGravity();

// Moon gravity
physics->setGravity({0.0f, -163.0f});

// Zero gravity (space level)
physics->setGravity({0.0f, 0.0f});

// Reverse gravity
physics->setGravity({0.0f, 980.0f});
```

### Simulation Step

The physics system is updated each frame:

```cpp
// In game loop
DeltaTime dt = 1.0f / 60.0f;  // 60 FPS
physics->update(dt);

// Box2D uses fixed sub-stepping (4 substeps by default)
```

**Note:** Box2D automatically performs 4 substeps per frame for stability (configurable in `Box2DPhysicsSystem::SUB_STEP_COUNT`).

---

## Complete Platformer Example

### Setup

```cpp
import bestow;
import bestow.physics.impl;

// Initialize physics system
auto physics = createPhysicsSystem();
auto* physicsImpl = dynamic_cast<Box2DPhysicsSystem*>(physics.get());
physicsImpl->initialize();

// Set gravity
physics->setGravity({0.0f, -980.0f});
```

### Create Player

```cpp
Entity player = entities->createEntity();

PhysicsBodyDef playerDef{
    .type = BodyType::Dynamic,
    .transform = {.x = 100.0f, .y = 300.0f},
    .size = {32.0f, 64.0f},         // 32px wide, 64px tall
    .fixedRotation = true,          // No spinning
    .linearDamping = 0.0f,          // Instant stops (manual control)
    .density = 1.0f,
    .friction = 0.0f,               // No sliding friction (manual control)
    .restitution = 0.0f             // No bounce
};

physics->createBody(player, playerDef);
physics->setCollisionLayer(player, CollisionLayers::Player);
physics->setCollisionMask(player, CollisionLayers::Terrain | CollisionLayers::Ground);
```

### Create Platforms

```cpp
void createPlatform(Vec2 position, Vec2 size) {
    Entity platform = entities->createEntity();

    PhysicsBodyDef platformDef{
        .type = BodyType::Static,
        .transform = {.x = position.x, .y = position.y},
        .size = size,
        .friction = 0.5f
    };

    physics->createBody(platform, platformDef);
    physics->setCollisionLayer(platform, CollisionLayers::Ground | CollisionLayers::Terrain);
}

// Main ground
createPlatform({400.0f, 50.0f}, {800.0f, 32.0f});

// Floating platforms
createPlatform({200.0f, 200.0f}, {128.0f, 16.0f});
createPlatform({500.0f, 350.0f}, {128.0f, 16.0f});
```

### Player Movement

```cpp
constexpr float MOVE_SPEED = 200.0f;
constexpr float JUMP_FORCE = 350.0f;
constexpr float AIR_CONTROL = 0.6f;

void updatePlayer(DeltaTime dt) {
    // Check if grounded
    GroundCheckParams groundParams{
        .rayDistance = 8.0f,
        .slopeToleranceDeg = 60.0f,
        .groundMask = CollisionLayers::Ground
    };
    auto groundCheck = physics->checkGrounded(player, groundParams);

    // Horizontal movement
    float moveInput = input->getAxis("Horizontal");  // -1 to 1
    Vec2 vel = physics->getVelocity(player);

    if (groundCheck.grounded) {
        // Full control on ground
        vel.x = moveInput * MOVE_SPEED;
    } else {
        // Reduced control in air
        vel.x = std::lerp(vel.x, moveInput * MOVE_SPEED, AIR_CONTROL * dt * 60.0f);
    }

    physics->setVelocity(player, vel);

    // Jump
    if (input->isPressed("Jump") && groundCheck.grounded) {
        Vec2 jumpImpulse{0.0f, -JUMP_FORCE};
        physics->applyImpulse(player, jumpImpulse);
    }

    // Variable jump height (release early for lower jump)
    if (input->isReleased("Jump") && vel.y < 0.0f) {
        vel.y *= 0.5f;
        physics->setVelocity(player, vel);
    }
}
```

### Collectible Coins

```cpp
Entity createCoin(Vec2 position) {
    Entity coin = entities->createEntity();

    PhysicsBodyDef coinDef{
        .type = BodyType::Static,
        .transform = {.x = position.x, .y = position.y},
        .size = {24.0f, 24.0f},
        .isSensor = true  // No collision, just detection
    };

    physics->createBody(coin, coinDef);
    physics->setCollisionLayer(coin, CollisionLayers::Collectible);

    return coin;
}

// Trigger callback for coin collection
physicsImpl->setTriggerEnterCallback([](const TriggerEvent& event) {
    if (isCoin(event.entityA) && isPlayer(event.entityB)) {
        collectCoin(event.entityA);
    }
});
```

### Enemy Patrol

```cpp
void updatePatrollingEnemy(Entity enemy, DeltaTime dt) {
    // Move enemy back and forth
    static float direction = 1.0f;

    Vec2 vel = physics->getVelocity(enemy);
    vel.x = direction * 100.0f;
    physics->setVelocity(enemy, vel);

    // Check for ledge or wall ahead
    Vec2 pos = physics->getPosition(enemy);
    Vec2 rayOrigin = {pos.x + direction * 20.0f, pos.y};
    Vec2 rayDirection = {0.0f, 1.0f};

    auto hit = physics->raycast(rayOrigin, rayDirection, 50.0f, CollisionLayers::Terrain);

    if (!hit) {
        // Ledge ahead: turn around
        direction *= -1.0f;
    }
}
```

### Collision Damage

```cpp
physics->setCollisionCallback([](const CollisionEvent& event) {
    Entity entityA = event.entityA;
    Entity entityB = event.entityB;

    // Player hit enemy
    if (isPlayer(entityA) && isEnemy(entityB)) {
        Vec2 normal = event.normal;

        // Bounce player if stomping from above
        if (normal.y > 0.5f) {
            Vec2 bounceImpulse{0.0f, -200.0f};
            physics->applyImpulse(entityA, bounceImpulse);
            killEnemy(entityB);
        } else {
            damagePlayer(entityA);
        }
    }
});
```

---

## Performance Tips

### Limit Body Count
- Reuse bodies instead of creating/destroying frequently
- Use object pools for projectiles and effects

### Optimize Queries
- Use spatial queries sparingly (expensive with many bodies)
- Cache query results if they don't change every frame
- Prefer collision callbacks over frequent raycasts

### Collision Filtering
- Set precise collision layers/masks to avoid unnecessary checks
- Use sensors for non-physical interactions

### Physics Scale
- Keep bodies reasonably sized (avoid tiny or huge bodies)
- Default scale: 100 pixels = 1 meter (optimal for Box2D)

---

## Common Pitfalls

### 1. Forgetting to Initialize

```cpp
// WRONG: System won't work without initialization
auto physics = createPhysicsSystem();
physics->createBody(entity, def);  // Fails silently

// CORRECT:
auto physics = createPhysicsSystem();
auto* impl = dynamic_cast<Box2DPhysicsSystem*>(physics.get());
impl->initialize();
physics->createBody(entity, def);
```

### 2. Setting Velocity Every Frame

```cpp
// BAD: Overrides physics simulation
void update() {
    physics->setVelocity(player, {100, 0});  // Ignores gravity!
}

// GOOD: Only modify velocity when needed
void handleInput() {
    Vec2 vel = physics->getVelocity(player);
    vel.x = input->getAxis("Horizontal") * 200.0f;  // Keep y velocity
    physics->setVelocity(player, vel);
}
```

### 3. Incorrect Coordinate Scale

```cpp
// BAD: Using meters instead of pixels
PhysicsBodyDef def{
    .transform = {.x = 1.0f, .y = 2.0f},  // Way too small!
    .size = {0.5f, 1.0f}
};

// GOOD: Use pixels (screen coordinates)
PhysicsBodyDef def{
    .transform = {.x = 100.0f, .y = 200.0f},
    .size = {50.0f, 100.0f}
};
```

### 4. Missing Collision Mask

```cpp
// Bodies won't collide if masks don't overlap!
physics->setCollisionLayer(player, CollisionLayers::Player);
physics->setCollisionMask(player, CollisionLayers::Enemy);  // Only collides with enemies

physics->setCollisionLayer(ground, CollisionLayers::Terrain);
physics->setCollisionMask(ground, 0xFFFF);  // Collides with everything

// Player won't collide with ground (mask doesn't include Terrain)
```

---

## API Reference Summary

### Body Management
- `createBody(entity, def)` - Create physics body
- `destroyBody(entity)` - Remove physics body
- `hasBody(entity)` - Check if body exists

### Body Properties
- `setPosition(entity, pos)` / `getPosition(entity)`
- `setRotation(entity, radians)` / `getRotation(entity)`
- `setVelocity(entity, vel)` / `getVelocity(entity)`
- `setAngularVelocity(entity, vel)` / `getAngularVelocity(entity)`
- `setBodyType(entity, type)` / `getBodyType(entity)`

### Forces
- `applyForce(entity, force, point = {0,0})`
- `applyImpulse(entity, impulse, point = {0,0})`
- `applyTorque(entity, torque)`

### Collision Filtering
- `setCollisionLayer(entity, layer)` / `getCollisionLayer(entity)`
- `setCollisionMask(entity, mask)`
- `setSensor(entity, isSensor)`

### Queries
- `queryAABB(min, max)` - Find entities in box
- `queryCircle(center, radius)` - Find entities in circle
- `raycast(origin, direction, maxDist, mask)` - Closest hit
- `raycastAll(origin, direction, maxDist, mask)` - All hits

### World Settings
- `setGravity(gravity)` / `getGravity()`
- `update(dt)` - Step simulation

### Callbacks
- `setCollisionCallback(callback)` - Physical collisions
- `setTriggerEnterCallback(callback)` - Sensor overlap start (requires impl cast)
- `setTriggerExitCallback(callback)` - Sensor overlap end (requires impl cast)

### Ground Detection
- `checkGrounded(entity, params)` - Check if on ground

---

## Further Reading

- **Box2D Manual**: [https://box2d.org/documentation/](https://box2d.org/documentation/)
- **Bestow Technical Design**: `docs/bestow-technical-design.md`
- **Physics System Tests**: `tests/unit/PhysicsSystemTests.cpp`

---

## Version History

- **v1.0** (2024) - Initial implementation with Box2D 3.0
  - Static, Dynamic, Kinematic bodies
  - Box shapes only
  - Collision detection and filtering
  - Raycasting and spatial queries
  - Sensor support
  - Ground detection for platformers
