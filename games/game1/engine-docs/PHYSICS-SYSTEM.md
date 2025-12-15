# Bestow Physics System Developer Guide

A practical guide to using 2D physics in your Bestow game.

## Table of Contents
1. [Overview](#overview)
2. [Quick Start](#quick-start)
3. [Creating Physics Bodies](#creating-physics-bodies)
4. [Body Properties](#body-properties)
5. [Forces and Impulses](#forces-and-impulses)
6. [Collision Filtering](#collision-filtering)
7. [Spatial Queries](#spatial-queries)
8. [Ground Detection](#ground-detection)
9. [Collision Callbacks](#collision-callbacks)
10. [Best Practices](#best-practices)
11. [Common Patterns](#common-patterns)

---

## Overview

Bestow's 2D physics system is powered by **Box2D 3.0** and provides:

- **Entity-centric**: Physics bodies are attached to entities
- **Box shapes**: Rectangular collision volumes (default)
- **Collision filtering**: 16-bit layer system for controlling what collides
- **Spatial queries**: AABB, circle, and raycasting
- **Ground detection**: Built-in platformer ground checks
- **Callbacks**: Collision and sensor (trigger) events

### Units and Scale

- **Units**: Pixels (your game logic uses pixels directly)
- **Internal conversion**: 100 pixels = 1 meter for Box2D
- **Coordinate System**: Y-down screen coordinates (positive Y = down)
- **Gravity**: Default `{0, 980}` pixels/s² (9.8 m/s² × 100)
- **Rotation**: Radians (0 = right, increases counter-clockwise)

**Why 100 pixels/meter?** Box2D works best with objects between 0.1-10 meters. A 32px sprite becomes 0.32m in Box2D's world, which is perfect for stability.

---

## Quick Start

Here's a complete example of creating a player with physics:

```cpp
import bestow;

// Create entity
Entity player = entities->createEntity();

// Define physics body
PhysicsBodyDef playerDef{
    .type = BodyType::Dynamic,           // Affected by gravity and forces
    .transform = {.x = 100, .y = 200},   // Starting position (pixels)
    .size = {32.0f, 40.0f},              // Width x Height (pixels)
    .fixedRotation = true,               // Don't rotate (good for characters)
    .linearDamping = 0.0f,               // No air resistance
    .density = 1.0f,                     // Standard density
    .friction = 0.3f,                    // Surface friction
    .restitution = 0.0f                  // No bouncing
};

// Create the physics body
physics->createBody(player, playerDef);

// Set collision filtering
physics->setCollisionLayer(player, CollisionLayers::Player);
physics->setCollisionMask(player, 0xFFFF);  // Collide with everything

// Apply jump impulse
if (isGrounded) {
    physics->applyImpulse(player, {0, -10000});  // Negative Y = up
}
```

That's it! Your entity now has physics.

---

## Creating Physics Bodies

### Body Types

There are three body types in Bestow physics:

| Type | Movement | Collision | Use Cases |
|------|----------|-----------|-----------|
| **Static** | Never moves | Collides with Dynamic/Kinematic | Terrain, walls, immovable obstacles |
| **Kinematic** | Set velocity directly | Collides with Dynamic | Moving platforms, elevators, doors |
| **Dynamic** | Physics-driven (forces, gravity) | Collides with all types | Player, enemies, physics objects |

### PhysicsBodyDef Structure

All physics bodies are created using `PhysicsBodyDef`:

```cpp
struct PhysicsBodyDef {
    BodyType type = BodyType::Dynamic;  // Static, Kinematic, or Dynamic
    Transform2D transform;               // Initial position {x, y}
    Vec2 size = {32.0f, 32.0f};         // Width and height (pixels)
    bool fixedRotation = true;           // Prevent rotation?
    float linearDamping = 0.0f;          // Velocity damping (0 = none)
    float angularDamping = 0.0f;         // Rotational damping
    float density = 1.0f;                // Mass per unit area
    float friction = 0.3f;               // Surface friction (0-1)
    float restitution = 0.0f;            // Bounciness (0-1)
    bool isSensor = false;               // Trigger vs solid collision
};
```

### Material Properties Explained

**Friction** controls how surfaces slide against each other:
- `0.0` = Ice (no friction, very slippery)
- `0.3` = Default (slight friction)
- `0.6` = Wood on wood
- `1.0` = Rubber (high friction, sticky)

**Restitution** controls how much objects bounce:
- `0.0` = Inelastic (no bounce, lands flat)
- `0.3` = Basketball
- `0.7` = Tennis ball
- `1.0` = Perfect bounce (bounces forever)

**Density** affects mass:
- Higher density = heavier object = harder to move
- Mass is calculated as: `density × area`
- Default `1.0` is good for most objects

**Linear Damping** slows down movement over time:
- `0.0` = No damping (like space - never slows down)
- `0.1` = Slight air resistance
- `0.5` = Heavy air resistance (underwater)

### Examples

#### Dynamic Player Character

```cpp
PhysicsBodyDef playerDef{
    .type = BodyType::Dynamic,
    .transform = {.x = 100, .y = 200},
    .size = {32.0f, 40.0f},      // 32px wide, 40px tall
    .fixedRotation = true,        // Don't rotate
    .friction = 0.3f,
    .restitution = 0.0f          // Don't bounce
};
physics->createBody(player, playerDef);
```

#### Static Ground Platform

```cpp
PhysicsBodyDef groundDef{
    .type = BodyType::Static,
    .transform = {.x = 400, .y = 500},
    .size = {800.0f, 32.0f}      // Wide platform
};
physics->createBody(ground, groundDef);
physics->setCollisionLayer(ground, CollisionLayers::Ground);
```

#### Kinematic Moving Platform

```cpp
PhysicsBodyDef platformDef{
    .type = BodyType::Kinematic,
    .transform = {.x = 200, .y = 300},
    .size = {100.0f, 16.0f}
};
physics->createBody(platform, platformDef);
physics->setCollisionLayer(platform, CollisionLayers::Ground);

// In update loop, move by setting velocity:
void update(float dt) {
    Vec2 velocity = {50.0f * sin(time), 0.0f};  // Oscillate
    physics->setVelocity(platform, velocity);
}
```

#### Sensor (Trigger)

```cpp
PhysicsBodyDef sensorDef{
    .type = BodyType::Static,
    .transform = {.x = 500, .y = 300},
    .size = {64.0f, 64.0f},
    .isSensor = true             // No collision, just detection
};
physics->createBody(trigger, sensorDef);
```

Sensors are perfect for:
- Collectible items (coins, power-ups)
- Goal zones (level exit, checkpoint)
- Damage zones (lava, spikes)
- Detection triggers (enemy awareness)

---

## Body Properties

Once you've created a physics body, you can query and modify its properties.

### Position and Rotation

```cpp
// Get position (pixels)
Vec2 pos = physics->getPosition(entity);
// pos.x and pos.y are in pixels

// Set position (pixels)
physics->setPosition(entity, {100.0f, 200.0f});

// Get rotation (radians)
float angle = physics->getRotation(entity);

// Set rotation (radians)
physics->setRotation(entity, 1.57f);  // 90 degrees

// Get body size
Vec2 size = physics->getBodySize(entity);
```

### Velocity

```cpp
// Get linear velocity (pixels/second)
Vec2 velocity = physics->getVelocity(entity);

// Set linear velocity (pixels/second)
physics->setVelocity(entity, {100.0f, 0.0f});  // Move right at 100 px/s

// Get/Set angular velocity (radians/second)
float angularVel = physics->getAngularVelocity(entity);
physics->setAngularVelocity(entity, 2.0f);     // Spin
```

### Body Type Changes

You can change a body's type at runtime:

```cpp
// Change to kinematic (script-controlled)
physics->setBodyType(entity, BodyType::Kinematic);

// Change to dynamic (physics-driven)
physics->setBodyType(entity, BodyType::Dynamic);

// Query current type
BodyType type = physics->getBodyType(entity);
```

### Body Management

```cpp
// Check if entity has a physics body
if (physics->hasBody(entity)) {
    // Do physics stuff
}

// Destroy physics body
physics->destroyBody(entity);
```

---

## Forces and Impulses

There are two ways to move physics bodies: **forces** (gradual) and **impulses** (instant).

### Forces (Gradual Acceleration)

Forces are applied continuously and integrated over time:

```cpp
// Apply force at center (default)
physics->applyForce(entity, {1000.0f, 0.0f});

// Apply force at a specific point (causes rotation)
Vec2 force = {500.0f, 0.0f};
Vec2 point = {10.0f, 20.0f};  // World coordinates
physics->applyForce(entity, force, point);

// Apply torque (rotational force)
physics->applyTorque(entity, 100.0f);
```

**Use forces for:**
- Continuous acceleration (thrusters, engines)
- Wind effects
- Magnetic/gravity fields
- Gradual pushing/pulling

### Impulses (Instant Velocity Change)

Impulses instantly modify velocity (perfect for one-time events):

```cpp
// Apply impulse at center (most common)
physics->applyImpulse(entity, {0, -10000});  // Jump!

// Apply impulse at specific point
Vec2 impulse = {500.0f, 0.0f};
Vec2 point = {10.0f, 20.0f};  // World coordinates
physics->applyImpulse(entity, impulse, point);
```

**Use impulses for:**
- Jumping (instant upward kick)
- Explosions
- Knockback effects
- Instant direction changes

**Example: Platformer Jump**

```cpp
void jump() {
    auto grounded = physics->checkGrounded(player);
    if (grounded.grounded) {
        // Apply upward impulse (negative Y = up)
        physics->applyImpulse(player, {0, -10000});
    }
}
```

---

## Collision Filtering

Collision filtering uses a 16-bit layer system to control what collides with what.

### How It Works

Every body has two properties:
- **Collision Layer** (16-bit): What category I am
- **Collision Mask** (16-bit): What categories I collide with

Bodies only collide if: `(bodyA.mask & bodyB.layer) && (bodyB.mask & bodyA.layer)`

### Predefined Layers

```cpp
namespace CollisionLayers {
    constexpr CollisionLayer Player      = 0x0001;  // Bit 0
    constexpr CollisionLayer Enemy       = 0x0002;  // Bit 1
    constexpr CollisionLayer Projectile  = 0x0004;  // Bit 2
    constexpr CollisionLayer Terrain     = 0x0008;  // Bit 3
    constexpr CollisionLayer Trigger     = 0x0010;  // Bit 4
    constexpr CollisionLayer Collectible = 0x0020;  // Bit 5
    constexpr CollisionLayer Ground      = 0x0040;  // Bit 6
}
```

### Setting Layers and Masks

```cpp
// Set what category this body is (layer)
physics->setCollisionLayer(player, CollisionLayers::Player);

// Set what categories this body collides with (mask)
physics->setCollisionMask(player, 0xFFFF);  // Collide with everything

// Or be selective:
CollisionMask mask = CollisionLayers::Enemy |
                     CollisionLayers::Terrain |
                     CollisionLayers::Ground;
physics->setCollisionMask(player, mask);  // Doesn't collide with projectiles

// Get current layer (for debugging)
CollisionLayer layer = physics->getCollisionLayer(entity);
```

### Example: Player Projectile

Make a projectile that hits enemies but not the player who shot it:

```cpp
// Create projectile body
PhysicsBodyDef def{
    .type = BodyType::Dynamic,
    .size = {8.0f, 8.0f},
    .isSensor = false  // Solid collision
};
physics->createBody(projectile, def);

// I am a projectile
physics->setCollisionLayer(projectile, CollisionLayers::Projectile);

// I collide with enemies and terrain only (NOT player or other projectiles)
CollisionMask mask = CollisionLayers::Enemy | CollisionLayers::Terrain;
physics->setCollisionMask(projectile, mask);
```

### Sensor (Trigger) Mode

Make a body detect overlap without physical collision:

```cpp
// Make body a sensor (trigger)
physics->setSensor(entity, true);

// Or set in PhysicsBodyDef:
PhysicsBodyDef def{.isSensor = true};
```

---

## Spatial Queries

Spatial queries let you find physics bodies in an area.

### AABB Query (Rectangular Area)

Find all bodies within a rectangular region:

```cpp
Vec2 min = {100.0f, 100.0f};
Vec2 max = {200.0f, 200.0f};

std::vector<Entity> entities = physics->queryAABB(min, max);

for (Entity e : entities) {
    // Do something with entities in box
}
```

**Use cases:** Area-of-effect attacks, spawn zones, region triggers

### Circle Query (Radial Area)

Find all bodies within a radius:

```cpp
Vec2 center = {200.0f, 300.0f};
float radius = 50.0f;

std::vector<Entity> entities = physics->queryCircle(center, radius);
```

**Use cases:** Explosion damage, enemy detection radius, proximity checks

### Raycasting

Cast a ray and find what it hits:

```cpp
Vec2 origin = {100.0f, 100.0f};
Vec2 direction = {1.0f, 0.0f};  // Must be normalized!
float maxDistance = 500.0f;

auto hit = physics->raycast(origin, direction, maxDistance);

if (hit) {
    Entity hitEntity = hit->entity;
    Vec2 hitPoint = hit->point;
    Vec2 hitNormal = hit->normal;
    float distance = hit->distance;

    // Hit something!
}
```

**Find all hits along the ray:**

```cpp
std::vector<RaycastHit> hits = physics->raycastAll(origin, direction, maxDistance);
// Sorted by distance (closest first)

for (const auto& hit : hits) {
    // Process each hit
}
```

**Raycast with collision filtering:**

```cpp
// Only hit enemies (ignore terrain, players, etc.)
CollisionMask mask = CollisionLayers::Enemy;
auto hit = physics->raycast(origin, direction, maxDistance, mask);
```

**Use cases:** Line-of-sight, shooting, laser beams, ground detection

---

## Ground Detection

For platformers, Bestow provides a dedicated ground check system:

### Basic Usage

```cpp
GroundCheckParams params{
    .rayDistance = 5.0f,        // How far below body to check (pixels)
    .slopeToleranceDeg = 60.0f, // Max slope angle considered "ground"
    .groundMask = CollisionLayers::Ground | CollisionLayers::Terrain
};

GroundCheckResult result = physics->checkGrounded(player, params);

if (result.grounded) {
    // Player is standing on something!
    Entity groundEntity = result.groundEntity;  // What we're on
    Vec2 contactPoint = result.contactPoint;    // Where we touch
    Vec2 surfaceNormal = result.surfaceNormal;  // Surface direction
    float slopeAngle = result.slopeAngle;       // Degrees from horizontal
}
```

### How It Works

1. Casts a ray downward from the body's center
2. Ray length = `body_half_height + rayDistance`
3. Checks if hit entity's layer is in `groundMask`
4. Validates slope angle ≤ `slopeToleranceDeg`
5. Returns detailed ground contact info

### Platformer Movement Example

```cpp
void updatePlayer(Entity player, float dt) {
    // Check if on ground
    auto grounded = physics->checkGrounded(player);

    if (grounded.grounded && input->isKeyJustPressed(Key::Space)) {
        // Jump!
        physics->applyImpulse(player, {0, -10000});
    }

    // Move horizontally
    Vec2 vel = physics->getVelocity(player);
    float moveSpeed = grounded.grounded ? 200.0f : 150.0f;  // Slower in air
    vel.x = input->getAxis(Axis::Horizontal) * moveSpeed;
    physics->setVelocity(player, vel);
}
```

---

## Collision Callbacks

Subscribe to collision events to handle physics interactions:

### Setting Up Callbacks

```cpp
physics->setCollisionCallback([this](const CollisionEvent& event) {
    Entity entityA = event.entityA;
    Entity entityB = event.entityB;
    Vec2 contactPoint = event.contactPoint;
    Vec2 normal = event.normal;
    float impulse = event.impulse;  // Collision force

    // Handle the collision
    if (isPlayer(entityA) && isEnemy(entityB)) {
        damagePlayer(entityA);
    }
});
```

### CollisionEvent Structure

```cpp
struct CollisionEvent {
    Entity entityA;      // First colliding entity
    Entity entityB;      // Second colliding entity
    Vec2 contactPoint;   // Where they touched (world coordinates, pixels)
    Vec2 normal;         // Collision normal
    float impulse;       // Impact force
};
```

### Important Notes

- **Callback is called for solid (non-sensor) collisions only**
- Sensor collisions generate separate trigger events (Box2D implementation detail)
- Use `isSensor = false` in PhysicsBodyDef for physical collisions
- Use `isSensor = true` for triggers/pickups

---

## Best Practices

### 1. Physics Scale and Units

**Keep objects between 10-1000 pixels** (0.1-10 meters in Box2D units):

```cpp
// Good: 32×40 pixel character (0.32×0.4 meters)
PhysicsBodyDef playerDef{.size = {32.0f, 40.0f}};

// Bad: 2×3 pixel character (too small, unstable)
PhysicsBodyDef tinyDef{.size = {2.0f, 3.0f}};

// Bad: 50,000×1,000 pixel object (too large, precision loss)
PhysicsBodyDef hugeDef{.size = {50000.0f, 1000.0f}};
```

### 2. Fixed Timestep Integration

Always use a fixed timestep for physics simulation:

```cpp
class Game {
    float accumulator_ = 0.0f;
    const float FIXED_DT = 1.0f / 60.0f;  // 60 Hz physics

    void update(float dt) {
        accumulator_ += dt;

        // Update physics in fixed steps
        while (accumulator_ >= FIXED_DT) {
            physics->update(FIXED_DT);
            accumulator_ -= FIXED_DT;
        }

        // Update rendering, input, etc. with variable dt
    }
};
```

**Why fixed timestep?**
- Deterministic (same inputs = same results)
- Stable simulation (no frame rate jitter)
- Prevents tunneling
- Easier to debug

### 3. Prevent Tunneling (Fast Objects Passing Through Walls)

Solutions:

**a) Make walls thicker:**

```cpp
// Thin wall - bad
PhysicsBodyDef wallDef{.size = {10.0f, 500.0f}};

// Thick wall - good
PhysicsBodyDef wallDef{.size = {32.0f, 500.0f}};
```

**b) Limit maximum velocity:**

```cpp
void limitVelocity(Entity entity, float maxSpeed) {
    Vec2 vel = physics->getVelocity(entity);
    float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);

    if (speed > maxSpeed) {
        vel = vel * (maxSpeed / speed);
        physics->setVelocity(entity, vel);
    }
}
```

### 4. Use Sensors for Non-Physical Detection

Don't use full physics bodies when you just need detection:

```cpp
// Bad: Full physics body for a coin
PhysicsBodyDef coinDef{
    .type = BodyType::Dynamic,  // Unnecessary
    .density = 0.1f
};

// Good: Sensor for pickup detection
PhysicsBodyDef coinDef{
    .type = BodyType::Static,
    .isSensor = true  // Just detection, no collision response
};
```

### 5. Use Collision Filtering Wisely

Don't let everything collide with everything:

```cpp
// Good: Use collision filtering
physics->setCollisionLayer(player, CollisionLayers::Player);
physics->setCollisionMask(player,
    CollisionLayers::Enemy |
    CollisionLayers::Terrain |
    CollisionLayers::Ground);  // Ignore projectiles, collectibles
```

### 6. Set Ground Layer on Platforms

For ground detection to work, platforms need the Ground layer:

```cpp
// Create platform
PhysicsBodyDef platformDef{
    .type = BodyType::Static,
    .size = {200.0f, 32.0f}
};
physics->createBody(platform, platformDef);

// IMPORTANT: Mark as ground for checkGrounded() to detect it
physics->setCollisionLayer(platform, CollisionLayers::Ground);
```

---

## Common Patterns

### Platformer Character Controller

Complete platformer movement with jumping and air control:

```cpp
class PlatformerController {
public:
    void update(Entity player, float dt) {
        // Check if on ground
        GroundCheckParams params{
            .rayDistance = 5.0f,
            .slopeToleranceDeg = 60.0f,
            .groundMask = CollisionLayers::Ground
        };
        auto grounded = physics->checkGrounded(player, params);

        // Get current velocity
        Vec2 vel = physics->getVelocity(player);

        // Horizontal input (Dvorak-friendly: A and E keys)
        float moveInput = 0.0f;
        if (input->isKeyHeld(Key::A)) moveInput -= 1.0f;  // Left
        if (input->isKeyHeld(Key::E)) moveInput += 1.0f;  // Right

        if (grounded.grounded) {
            // On ground: direct control
            vel.x = moveInput * moveSpeed;

            // Jump (Dvorak: Space or Comma key)
            if (input->isKeyJustPressed(Key::Space) ||
                input->isKeyJustPressed(Key::Comma)) {
                physics->applyImpulse(player, {0, jumpForce});
            }
        } else {
            // In air: reduced control
            vel.x += moveInput * moveSpeed * airControl * dt;
            vel.x = std::clamp(vel.x, -moveSpeed, moveSpeed);
        }

        physics->setVelocity(player, vel);
    }

private:
    const float moveSpeed = 200.0f;
    const float jumpForce = -10000.0f;  // Negative = up
    const float airControl = 0.3f;
};
```

### Top-Down Movement

Character controller for top-down games (twin-stick, RPG, etc.):

```cpp
class TopDownController {
public:
    void update(Entity character, float dt) {
        // Get input direction (Dvorak: ,AOE keys)
        Vec2 moveDir = {0.0f, 0.0f};
        if (input->isKeyHeld(Key::A)) moveDir.x -= 1.0f;  // Left
        if (input->isKeyHeld(Key::E)) moveDir.x += 1.0f;  // Right
        if (input->isKeyHeld(Key::Comma)) moveDir.y -= 1.0f;  // Up
        if (input->isKeyHeld(Key::O)) moveDir.y += 1.0f;  // Down

        // Normalize diagonal movement
        float length = std::sqrt(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
        if (length > 1.0f) {
            moveDir.x /= length;
            moveDir.y /= length;
        }

        // Set velocity directly
        Vec2 velocity = {moveDir.x * moveSpeed, moveDir.y * moveSpeed};
        physics->setVelocity(character, velocity);

        // Optional: Face movement direction
        if (length > 0.1f) {
            float angle = std::atan2(moveDir.y, moveDir.x);
            physics->setRotation(character, angle);
        }
    }

private:
    const float moveSpeed = 150.0f;
};
```

### Moving Platforms

Platforms that carry the player:

```cpp
class MovingPlatform {
public:
    void create(Entity platform, Vec2 start, Vec2 end, float speed) {
        PhysicsBodyDef def{
            .type = BodyType::Kinematic,  // Kinematic for script-controlled movement
            .transform = {.x = start.x, .y = start.y},
            .size = {100.0f, 16.0f}
        };
        physics->createBody(platform, def);
        physics->setCollisionLayer(platform, CollisionLayers::Ground);

        // Store movement data
        data_[platform] = {start, end, speed};
        movingToEnd_[platform] = true;
    }

    void update(float dt) {
        for (auto& [entity, data] : data_) {
            Vec2 pos = physics->getPosition(entity);

            // Calculate target (ping-pong between start and end)
            Vec2 target = movingToEnd_[entity] ? data.end : data.start;
            Vec2 dir = {target.x - pos.x, target.y - pos.y};
            float dist = std::sqrt(dir.x * dir.x + dir.y * dir.y);

            if (dist < 2.0f) {
                // Reached target, switch direction
                movingToEnd_[entity] = !movingToEnd_[entity];
                continue;
            }

            // Normalize direction
            dir.x /= dist;
            dir.y /= dist;

            // Set velocity (kinematic bodies move by velocity)
            Vec2 vel = {dir.x * data.speed, dir.y * data.speed};
            physics->setVelocity(entity, vel);
        }
    }

private:
    struct PlatformData {
        Vec2 start;
        Vec2 end;
        float speed;
    };
    std::unordered_map<Entity, PlatformData> data_;
    std::unordered_map<Entity, bool> movingToEnd_;
};
```

### Projectile System

Simple projectile physics with collision:

```cpp
class ProjectileSystem {
public:
    Entity createProjectile(Vec2 pos, Vec2 direction, float speed) {
        Entity projectile = entities->createEntity();

        PhysicsBodyDef def{
            .type = BodyType::Dynamic,
            .transform = {.x = pos.x, .y = pos.y},
            .size = {8.0f, 8.0f},
            .density = 0.1f,
            .friction = 0.0f,
            .linearDamping = 0.0f  // No slowdown
        };
        physics->createBody(projectile, def);

        // Set collision filtering
        physics->setCollisionLayer(projectile, CollisionLayers::Projectile);
        physics->setCollisionMask(projectile,
            CollisionLayers::Enemy | CollisionLayers::Terrain);

        // Set velocity
        Vec2 vel = {direction.x * speed, direction.y * speed};
        physics->setVelocity(projectile, vel);

        return projectile;
    }

    void onCollision(const CollisionEvent& event) {
        if (isProjectile(event.entityA)) {
            // Projectile hit something
            Entity target = event.entityB;

            // Deal damage
            if (entities->has<Health>(target)) {
                auto& health = entities->get<Health>(target);
                health.current -= 10;
            }

            // Destroy projectile
            physics->destroyBody(event.entityA);
            entities->destroyEntity(event.entityA);
        }
    }
};
```

### Explosion Force

Apply radial force outward from a point:

```cpp
void applyExplosion(Vec2 center, float radius, float force) {
    // Query all bodies in radius
    std::vector<Entity> affected = physics->queryCircle(center, radius);

    for (Entity entity : affected) {
        Vec2 pos = physics->getPosition(entity);
        Vec2 dir = {pos.x - center.x, pos.y - center.y};

        float distance = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        if (distance < 0.001f) continue;

        // Normalize direction
        dir.x /= distance;
        dir.y /= distance;

        // Calculate force with falloff (inverse square)
        float falloff = 1.0f - (distance / radius);
        float magnitude = force * falloff * falloff;

        // Apply impulse
        Vec2 impulse = {dir.x * magnitude, dir.y * magnitude};
        physics->applyImpulse(entity, impulse);
    }
}
```

### Knockback Effect

Apply knockback when hit:

```cpp
void applyKnockback(Entity entity, Vec2 direction, float force) {
    // Normalize direction
    float len = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (len > 0.001f) {
        direction.x /= len;
        direction.y /= len;
    }

    // Apply impulse
    Vec2 impulse = {direction.x * force, direction.y * force};
    physics->applyImpulse(entity, impulse);
}

// Usage in collision handler:
void onPlayerHit(Entity player, Entity enemy) {
    Vec2 playerPos = physics->getPosition(player);
    Vec2 enemyPos = physics->getPosition(enemy);

    // Calculate knockback direction (away from enemy)
    Vec2 dir = {playerPos.x - enemyPos.x, playerPos.y - enemyPos.y};

    applyKnockback(player, dir, 5000.0f);
}
```

---

## Summary

Bestow's 2D physics system provides everything you need for platformers, top-down games, and 2D arcade games:

- **Box2D 3.0** backend for robust 2D physics
- **Pixel-based units** (100 pixels = 1 meter internally)
- **Entity-centric** API that integrates with ECS
- **Collision filtering** with 16-bit layers and masks
- **Ground detection** built-in for platformers
- **Spatial queries** (AABB, circle, raycast)
- **Sensors/Triggers** for non-physical detection
- **Callbacks** for collision events

**Key Takeaways:**

1. Use **fixed timestep** (60 Hz recommended)
2. Keep objects **10-1000 pixels** for stability
3. Use appropriate **body types** (Static/Kinematic/Dynamic)
4. Apply **collision filtering** to reduce unnecessary checks
5. Use **sensors** for triggers, not collision response
6. Check **ground state** before jumping
7. Apply **impulses** for instant velocity changes
8. Apply **forces** for continuous acceleration
9. Set **Ground layer** on platforms for `checkGrounded()` to work

**For more details, see:**
- Physics tests: `/tests/unit/PhysicsSystemTests.cpp`
- Box2D documentation: https://box2d.org/documentation/
