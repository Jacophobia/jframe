# PhysicsSystem API

The `PhysicsSystem` provides 2D physics simulation using Box2D.

## Overview

```cpp
auto& physics = sys.physics;

// Create physics body
PhysicsBodyDef def{
    .type = BodyType::Dynamic,
    .transform = {.x = 100.0f, .y = 200.0f},
    .size = {32.0f, 48.0f},
    .fixedRotation = true,
    .density = 1.0f,
    .friction = 0.3f
};
physics->createBody(playerEntity, def);

// Update simulation
physics->update(dt);

// Apply forces
physics->applyImpulse(playerEntity, Vec2{0.0f, -500.0f});

// Raycast
auto hit = physics->raycast(origin, direction, 100.0f);
if (hit) {
    // Hit something
}
```

## Body Management

### createBody(Entity entity, const PhysicsBodyDef& def)

```cpp
void createBody(Entity entity, const PhysicsBodyDef& def);
```

Creates a physics body and attaches it to an entity.

**Parameters:**
- `entity`: Entity to attach body to
- `def`: Body definition

**Example:**

```cpp
PhysicsBodyDef platformDef{
    .type = BodyType::Static,
    .transform = {.x = 400.0f, .y = 500.0f},
    .size = {200.0f, 32.0f},
    .friction = 0.8f
};
physics->createBody(platformEntity, platformDef);
```

---

### PhysicsBodyDef

```cpp
struct PhysicsBodyDef {
    BodyType type = BodyType::Dynamic;
    Transform2D transform;
    Vec2 size = {32.0f, 32.0f};  // Collision box size (pixels)
    bool fixedRotation = true;
    float linearDamping = 0.0f;
    float angularDamping = 0.0f;
    float density = 1.0f;
    float friction = 0.3f;
    float restitution = 0.0f;  // Bounciness (0.0 = no bounce, 1.0 = perfect bounce)
    bool isSensor = false;     // Trigger volume (no collision response)
};
```

**Body Types:**
- `BodyType::Static` - Immovable (platforms, walls)
- `BodyType::Dynamic` - Affected by forces and gravity (player, enemies)
- `BodyType::Kinematic` - Moved manually, affects dynamics (moving platforms)

---

### destroyBody(Entity entity)

```cpp
void destroyBody(Entity entity);
```

Destroys the physics body attached to an entity.

---

### hasBody(Entity entity)

```cpp
bool hasBody(Entity entity) const;
```

Checks if an entity has a physics body.

---

## Body Properties

### setBodyType(Entity entity, BodyType type)

```cpp
void setBodyType(Entity entity, BodyType type);
BodyType getBodyType(Entity entity) const;
```

Changes the body type at runtime.

**Example:**

```cpp
// Make platform kinematic to move it
physics->setBodyType(platform, BodyType::Kinematic);
physics->setVelocity(platform, Vec2{50.0f, 0.0f});
```

---

### Position

```cpp
void setPosition(Entity entity, Vec2 position);
Vec2 getPosition(Entity entity) const;
```

Gets or sets the body's position in world space.

**Example:**

```cpp
Vec2 pos = physics->getPosition(player);
physics->setPosition(enemy, Vec2{100.0f, 200.0f});
```

---

### Rotation

```cpp
void setRotation(Entity entity, float radians);
float getRotation(Entity entity) const;
```

Gets or sets the body's rotation in radians.

---

### Velocity

```cpp
void setVelocity(Entity entity, Vec2 velocity);
Vec2 getVelocity(Entity entity) const;
```

Gets or sets the body's linear velocity (pixels per second).

**Example:**

```cpp
// Jump
physics->setVelocity(player, Vec2{0.0f, -500.0f});

// Stop horizontal movement
Vec2 vel = physics->getVelocity(player);
physics->setVelocity(player, Vec2{0.0f, vel.y});
```

---

### Angular Velocity

```cpp
void setAngularVelocity(Entity entity, float velocity);
float getAngularVelocity(Entity entity) const;
```

Gets or sets the body's rotation speed (radians per second).

---

### Body Size

```cpp
Vec2 getBodySize(Entity entity) const;
```

Returns the collision box size in pixels.

---

## Forces and Impulses

### applyForce(Entity entity, Vec2 force, Vec2 point)

```cpp
void applyForce(Entity entity, Vec2 force, Vec2 point = {0, 0});
```

Applies a continuous force over time. Force is accumulated and cleared after physics step.

**Parameters:**
- `force`: Force vector (Newtons)
- `point`: Application point (local coordinates, default = center)

**Use for:** Continuous acceleration (wind, thrusters)

**Example:**

```cpp
// Jetpack thrust
if (input->isActionActive("Thrust")) {
    physics->applyForce(player, Vec2{0.0f, -1000.0f});
}
```

---

### applyImpulse(Entity entity, Vec2 impulse, Vec2 point)

```cpp
void applyImpulse(Entity entity, Vec2 impulse, Vec2 point = {0, 0});
```

Applies an instant velocity change.

**Parameters:**
- `impulse`: Impulse vector (Newton-seconds)
- `point`: Application point (local coordinates, default = center)

**Use for:** Instant velocity changes (jumping, knockback)

**Example:**

```cpp
// Jump
physics->applyImpulse(player, Vec2{0.0f, -500.0f});

// Knockback from collision
physics->applyImpulse(enemy, Vec2{100.0f, -50.0f});
```

---

### applyTorque(Entity entity, float torque)

```cpp
void applyTorque(Entity entity, float torque);
```

Applies rotational force.

---

## Collision Filtering

### setCollisionLayer(Entity entity, CollisionLayer layer)

```cpp
void setCollisionLayer(Entity entity, CollisionLayer layer);
CollisionLayer getCollisionLayer(Entity entity) const;
```

Sets which collision layer this body belongs to.

**Predefined Layers:**

```cpp
namespace CollisionLayers {
    inline constexpr CollisionLayer Player = 0x0001;
    inline constexpr CollisionLayer Enemy = 0x0002;
    inline constexpr CollisionLayer Projectile = 0x0004;
    inline constexpr CollisionLayer Terrain = 0x0008;
    inline constexpr CollisionLayer Trigger = 0x0010;
    inline constexpr CollisionLayer Collectible = 0x0020;
    inline constexpr CollisionLayer Ground = 0x0040;
}
```

**Example:**

```cpp
physics->setCollisionLayer(player, CollisionLayers::Player);
physics->setCollisionLayer(enemy, CollisionLayers::Enemy);
```

---

### setCollisionMask(Entity entity, CollisionMask mask)

```cpp
void setCollisionMask(Entity entity, CollisionMask mask);
```

Sets which layers this body can collide with (bitwise OR).

**Example:**

```cpp
// Player collides with terrain, enemies, and collectibles
physics->setCollisionMask(player,
    CollisionLayers::Terrain |
    CollisionLayers::Enemy |
    CollisionLayers::Collectible
);

// Projectile only collides with enemies and terrain
physics->setCollisionMask(projectile,
    CollisionLayers::Enemy |
    CollisionLayers::Terrain
);
```

---

### setSensor(Entity entity, bool isSensor)

```cpp
void setSensor(Entity entity, bool isSensor);
```

Makes a body a sensor (trigger volume). Sensors detect overlaps but have no collision response.

**Example:**

```cpp
// Create collectible coin as sensor
physics->createBody(coin, coinDef);
physics->setSensor(coin, true);
physics->setCollisionLayer(coin, CollisionLayers::Collectible);
```

---

## Spatial Queries

### raycast(Vec2 origin, Vec2 direction, float maxDistance, CollisionMask mask)

```cpp
std::optional<RaycastHit> raycast(
    Vec2 origin,
    Vec2 direction,
    float maxDistance,
    CollisionMask mask = 0xFFFF
) const;
```

Casts a ray and returns the first hit.

**Returns:** `RaycastHit` if hit, `std::nullopt` if no hit

```cpp
struct RaycastHit {
    Entity entity;
    Vec2 point;
    Vec2 normal;
    float distance;
};
```

**Example:**

```cpp
// Check if player can see enemy
Vec2 toEnemy = enemyPos - playerPos;
float distance = glm::length(toEnemy);
Vec2 direction = toEnemy / distance;

auto hit = physics->raycast(playerPos, direction, distance);
if (hit && hit->entity == enemy) {
    // Line of sight clear
}
```

---

### raycastAll(Vec2 origin, Vec2 direction, float maxDistance, CollisionMask mask)

```cpp
std::vector<RaycastHit> raycastAll(
    Vec2 origin,
    Vec2 direction,
    float maxDistance,
    CollisionMask mask = 0xFFFF
) const;
```

Casts a ray and returns all hits in order.

**Example:**

```cpp
// Get all enemies in a line
auto hits = physics->raycastAll(origin, direction, 500.0f,
                                 CollisionLayers::Enemy);
for (const auto& hit : hits) {
    // Process each enemy
}
```

---

### queryAABB(Vec2 min, Vec2 max)

```cpp
std::vector<Entity> queryAABB(Vec2 min, Vec2 max) const;
```

Returns all bodies overlapping an axis-aligned bounding box.

**Example:**

```cpp
// Find all entities in a rectangular area
auto entities = physics->queryAABB(
    Vec2{100.0f, 100.0f},
    Vec2{200.0f, 200.0f}
);
```

---

### queryCircle(Vec2 center, float radius)

```cpp
std::vector<Entity> queryCircle(Vec2 center, float radius) const;
```

Returns all bodies overlapping a circle.

**Example:**

```cpp
// Find all enemies in explosion radius
auto affected = physics->queryCircle(explosionPos, 100.0f);
for (Entity e : affected) {
    if (entities->allOf<Enemy>(e)) {
        // Apply damage
    }
}
```

---

## Ground Detection

### checkGrounded(Entity entity, const GroundCheckParams& params)

```cpp
GroundCheckResult checkGrounded(
    Entity entity,
    const GroundCheckParams& params = {}
) const;
```

Checks if an entity is standing on valid ground.

**Parameters:**

```cpp
struct GroundCheckParams {
    float rayDistance = 5.0f;          // How far below to check (pixels)
    float slopeToleranceDeg = 60.0f;   // Max slope angle considered "ground"
    CollisionMask groundMask = 0xFFFF; // Which layers count as ground
};
```

**Returns:**

```cpp
struct GroundCheckResult {
    bool grounded = false;
    Entity groundEntity{};              // What we're standing on
    Vec2 contactPoint{};                // Where we're touching
    Vec2 surfaceNormal{0.0f, -1.0f};    // Surface orientation
    float slopeAngle = 0.0f;            // Angle in degrees from vertical
};
```

**Example:**

```cpp
// Check if player can jump
GroundCheckParams params{
    .rayDistance = 5.0f,
    .groundMask = CollisionLayers::Terrain | CollisionLayers::Ground
};

auto result = physics->checkGrounded(player, params);
if (result.grounded && input->wasActionJustPressed("Jump")) {
    physics->applyImpulse(player, Vec2{0.0f, -500.0f});
}

// Check slope for movement
if (result.grounded && result.slopeAngle > 45.0f) {
    // Too steep to climb
}
```

---

## World Settings

### setGravity(Vec2 gravity)

```cpp
void setGravity(Vec2 gravity);
Vec2 getGravity() const;
```

Sets the world gravity vector.

**Default:** `Vec2{0.0f, 980.0f}` (downward, ~10 m/s^2 in pixels)

**Example:**

```cpp
// Standard gravity
physics->setGravity(Vec2{0.0f, 980.0f});

// Low gravity
physics->setGravity(Vec2{0.0f, 300.0f});

// Zero gravity
physics->setGravity(Vec2{0.0f, 0.0f});

// Side-scrolling with wind
physics->setGravity(Vec2{50.0f, 980.0f});
```

---

## Collision Callbacks

### setCollisionCallback(CollisionCallback callback)

```cpp
using CollisionCallback = std::function<void(const CollisionEvent&)>;
void setCollisionCallback(CollisionCallback callback);
```

Sets a callback for physical collisions (non-sensors).

**Note:** By default, collisions are published to the event system as `Events::Collision`.

```cpp
struct CollisionEvent {
    Entity entityA;
    Entity entityB;
    Vec2 contactPoint;
    Vec2 normal;
    float impulse;
};
```

**Example:**

```cpp
physics->setCollisionCallback([](const CollisionEvent& e) {
    // Custom collision handling
    if (entities->allOf<Player>(e.entityA) && entities->allOf<Enemy>(e.entityB)) {
        // Player hit enemy
    }
});
```

**Sensor Callbacks:**

Sensor enter/exit events are published as:
- `Events::TriggerEnter`
- `Events::TriggerExit`

```cpp
events->subscribe(Events::TriggerEnter, [](const EventData& data) {
    auto& trigger = std::get<TriggerEvent>(data);
    // Handle trigger
});
```

---

## Update Loop

### update(DeltaTime dt)

```cpp
void update(DeltaTime dt);
```

Steps the physics simulation forward by `dt` seconds.

**Call in:** `Application::updateFixed()`

**Example:**

```cpp
void updateFixed(DeltaTime dt) override {
    physics->update(dt);
}
```

---

## Common Patterns

### Player Movement with Ground Check

```cpp
void updatePlayerMovement(DeltaTime dt) {
    // Get input
    float moveInput = input->getActionValue("MoveHorizontal");

    // Apply horizontal velocity
    Vec2 vel = physics->getVelocity(player);
    vel.x = moveInput * 200.0f;  // Move speed
    physics->setVelocity(player, vel);

    // Ground check
    auto ground = physics->checkGrounded(player);

    // Jump
    if (ground.grounded && input->wasActionJustPressed("Jump")) {
        physics->applyImpulse(player, Vec2{0.0f, -500.0f});
    }
}
```

---

### One-Way Platforms

```cpp
// Use sensor for top surface
PhysicsBodyDef platformDef{
    .type = BodyType::Static,
    .transform = {.x = 400.0f, .y = 300.0f},
    .size = {100.0f, 16.0f},
    .isSensor = true
};
physics->createBody(platform, platformDef);

// Check if player is falling and above platform
events->subscribe(Events::TriggerEnter, [](const EventData& data) {
    auto& trigger = std::get<TriggerEvent>(data);

    if (entities->allOf<Player>(trigger.entityA)) {
        Vec2 vel = physics->getVelocity(trigger.entityA);
        Vec2 playerPos = physics->getPosition(trigger.entityA);
        Vec2 platformPos = physics->getPosition(trigger.entityB);

        if (vel.y > 0 && playerPos.y < platformPos.y) {
            // Player is falling from above - enable collision
            physics->setSensor(trigger.entityB, false);
        }
    }
});
```

---

### Projectile with Raycast

```cpp
Entity fireProjectile(Vec2 origin, Vec2 direction) {
    // Fast projectile - use raycast instead of physics body
    auto hit = physics->raycast(origin, direction, 1000.0f,
                                 CollisionLayers::Enemy | CollisionLayers::Terrain);

    if (hit) {
        if (entities->allOf<Enemy>(hit->entity)) {
            // Hit enemy
            applyDamage(hit->entity, 25);
        }

        // Spawn impact effect at hit point
        createImpactEffect(hit->point);
    }

    return Entity{};  // No entity needed for instant projectile
}
```

---

## Performance Tips

1. **Use collision layers** - Avoid unnecessary collision checks
2. **Prefer sensors for triggers** - Cheaper than physical collision
3. **Use raycasts for fast projectiles** - More accurate than moving bodies
4. **Batch queries** - Call `queryAABB` once, not per entity
5. **Fixed rotation for characters** - `fixedRotation = true` for stability

## See Also

- [EntitySystem](EntitySystem.md) - Attaching physics to entities
- [EventSystem](EventSystem.md) - Collision event handling
- [InputSystem](InputSystem.md) - Player movement input
