# Tutorial 3: Physics and Collision

This tutorial covers JFrame's physics system, which is built on Box2D 3.0. You'll learn about body types, collision layers, collision callbacks, and ground detection.

## Physics System Overview

JFrame's physics system provides:

- 2D rigid body physics with gravity
- Collision detection and response
- Raycasting and spatial queries
- Collision filtering by layers
- Ground detection for platformers
- Sensor/trigger volumes

## Body Types

There are three types of physics bodies:

### 1. Dynamic Bodies

Affected by gravity and forces. Used for players, enemies, boxes.

```cpp
jframe::PhysicsBodyDef playerDef{
    .type = jframe::BodyType::Dynamic,
    .transform = {.x = 100.0f, .y = 200.0f},
    .size = {32.0f, 48.0f},
    .fixedRotation = true  // Prevent rotation
};
sys.physics->createBody(player, playerDef);
```

### 2. Static Bodies

Never move. Used for ground, walls, platforms.

```cpp
jframe::PhysicsBodyDef groundDef{
    .type = jframe::BodyType::Static,
    .transform = {.x = 400.0f, .y = 50.0f},
    .size = {800.0f, 100.0f}
};
sys.physics->createBody(ground, groundDef);
```

### 3. Kinematic Bodies

Move programmatically but aren't affected by forces. Used for moving platforms, elevators.

```cpp
jframe::PhysicsBodyDef platformDef{
    .type = jframe::BodyType::Kinematic,
    .transform = {.x = 300.0f, .y = 200.0f},
    .size = {100.0f, 20.0f}
};
sys.physics->createBody(movingPlatform, platformDef);

// Move it manually
sys.physics->setVelocity(movingPlatform, {50.0f, 0.0f});
```

## Creating Physics Bodies

### Basic Body Creation

```cpp
// Create entity
jframe::Entity entity = sys.entities->createEntity();

// Define physics body
jframe::PhysicsBodyDef bodyDef{
    .type = jframe::BodyType::Dynamic,
    .transform = {.x = 100.0f, .y = 200.0f},
    .size = {32.0f, 32.0f},
    .fixedRotation = true,
    .linearDamping = 0.0f,
    .gravityScale = 1.0f
};

// Create the body
sys.physics->createBody(entity, bodyDef);
```

### Body Properties

```cpp
// Position and rotation
sys.physics->setPosition(entity, {200.0f, 300.0f});
jframe::Vec2 pos = sys.physics->getPosition(entity);

sys.physics->setRotation(entity, 0.785f);  // Radians
float rotation = sys.physics->getRotation(entity);

// Velocity
sys.physics->setVelocity(entity, {100.0f, 200.0f});
jframe::Vec2 vel = sys.physics->getVelocity(entity);

sys.physics->setAngularVelocity(entity, 1.5f);
float angVel = sys.physics->getAngularVelocity(entity);

// Get body size
jframe::Vec2 size = sys.physics->getBodySize(entity);
```

### Applying Forces

```cpp
// Apply a force (gradual acceleration)
sys.physics->applyForce(entity, {1000.0f, 0.0f});

// Apply an impulse (instant velocity change)
sys.physics->applyImpulse(entity, {500.0f, 500.0f});

// Apply torque (rotation force)
sys.physics->applyTorque(entity, 100.0f);

// Apply force at a point (creates rotation)
jframe::Vec2 force = {1000.0f, 0.0f};
jframe::Vec2 point = {16.0f, 16.0f};  // Offset from center
sys.physics->applyForce(entity, force, point);
```

## Collision Layers and Masks

Collision layers control which objects collide with each other.

### Predefined Layers

```cpp
namespace jframe::CollisionLayers {
    inline constexpr CollisionLayer Player      = 0x0001;
    inline constexpr CollisionLayer Enemy       = 0x0002;
    inline constexpr CollisionLayer Projectile  = 0x0004;
    inline constexpr CollisionLayer Terrain     = 0x0008;
    inline constexpr CollisionLayer Trigger     = 0x0010;
    inline constexpr CollisionLayer Collectible = 0x0020;
    inline constexpr CollisionLayer Ground      = 0x0040;
}
```

### Setting Collision Layers

```cpp
using namespace jframe::CollisionLayers;

// Player collides with terrain, enemies, and collectibles
sys.physics->setCollisionLayer(player, Player);
sys.physics->setCollisionMask(player, Terrain | Enemy | Collectible | Ground);

// Enemy collides with terrain, player, and projectiles
sys.physics->setCollisionLayer(enemy, Enemy);
sys.physics->setCollisionMask(enemy, Terrain | Player | Projectile | Ground);

// Projectile collides with terrain and enemies only
sys.physics->setCollisionLayer(projectile, Projectile);
sys.physics->setCollisionMask(projectile, Terrain | Enemy);

// Ground/platforms (static terrain)
sys.physics->setCollisionLayer(ground, Terrain | Ground);
sys.physics->setCollisionMask(ground, 0xFFFF);  // Collides with everything
```

### Custom Collision Matrix

```cpp
// Example: Create collision rules for a game

// Player setup
sys.physics->setCollisionLayer(player, Player);
sys.physics->setCollisionMask(player,
    Terrain | Ground | Enemy | Collectible | Trigger);

// Enemy setup
sys.physics->setCollisionLayer(enemy, Enemy);
sys.physics->setCollisionMask(enemy,
    Terrain | Ground | Player | Projectile);

// Collectible setup (sensor - no physical collision)
sys.physics->setCollisionLayer(collectible, Collectible);
sys.physics->setCollisionMask(collectible, Player);
sys.physics->setSensor(collectible, true);  // Trigger only

// Terrain
sys.physics->setCollisionLayer(terrain, Terrain | Ground);
sys.physics->setCollisionMask(terrain, 0xFFFF);
```

## Sensors and Triggers

Sensors detect overlaps without causing physical collision.

```cpp
// Create a trigger zone (e.g., checkpoint, level exit)
jframe::Entity trigger = sys.entities->createEntity();

jframe::PhysicsBodyDef triggerDef{
    .type = jframe::BodyType::Static,
    .transform = {.x = 500.0f, .y = 300.0f},
    .size = {100.0f, 200.0f}
};
sys.physics->createBody(trigger, triggerDef);

// Make it a sensor (no collision, only triggers)
sys.physics->setSensor(trigger, true);
sys.physics->setCollisionLayer(trigger, jframe::CollisionLayers::Trigger);
sys.physics->setCollisionMask(trigger, jframe::CollisionLayers::Player);
```

## Collision Callbacks

Receive collision events through the event system.

```cpp
// Subscribe to collision events
collisionSub_ = sys.events->subscribe(jframe::Events::Collision,
    [this](const jframe::EventData& data) {
        handleCollision(data);
    });

// Handle collision
void handleCollision(const jframe::EventData& data) {
    try {
        const auto& collision = std::get<jframe::CollisionEvent>(data);

        // Check what collided
        jframe::Entity entityA = collision.entityA;
        jframe::Entity entityB = collision.entityB;

        // Collision details
        jframe::Vec2 contactPoint = collision.contactPoint;
        jframe::Vec2 normal = collision.normal;
        bool isBegin = collision.isBegin;  // true = collision start, false = end

        // Example: Player hit enemy
        if (isPlayerEnemyCollision(entityA, entityB)) {
            if (isBegin) {
                damagePlayer(10);
            }
        }

        // Example: Projectile hit enemy
        if (isProjectileEnemyCollision(entityA, entityB)) {
            if (isBegin) {
                damageEnemy(entityB, 25);
                destroyProjectile(entityA);
            }
        }

    } catch (const std::bad_variant_access&) {
        // Not a collision event
    }
}
```

### Collision Event Structure

```cpp
struct CollisionEvent {
    Entity entityA;
    Entity entityB;
    Vec2 contactPoint;
    Vec2 normal;
    bool isBegin;  // true = collision started, false = collision ended
};
```

## Ground Detection

JFrame provides a helper for detecting if a character is standing on the ground.

```cpp
// Check if player is grounded
jframe::GroundCheckParams params{
    .rayLength = 5.0f,          // How far down to check
    .rayOffsetX = 10.0f,        // Check multiple rays for stability
    .groundLayers = jframe::CollisionLayers::Ground | jframe::CollisionLayers::Terrain
};

jframe::GroundCheckResult result = sys.physics->checkGrounded(player, params);

if (result.isGrounded) {
    // Player is on the ground
    jframe::Vec2 groundPoint = result.groundPoint;
    jframe::Vec2 groundNormal = result.groundNormal;
    jframe::Entity groundEntity = result.groundEntity;

    // Can jump
    if (jumpPressed) {
        sys.physics->setVelocity(player, {vel.x, 400.0f});
    }
} else {
    // Player is in the air
    // Apply air control, coyote time, etc.
}
```

### Platformer Ground Detection Example

```cpp
struct PlayerController {
    bool isGrounded = false;
    float coyoteTime = 0.0f;
    static constexpr float kCoyoteTimeMax = 0.15f;
};

void updatePlayerMovement(jframe::DeltaTime dt) {
    auto& sys = engine_->systems();
    auto* controller = sys.entities->tryGet<PlayerController>(player_);

    // Check grounded state
    jframe::GroundCheckParams params{
        .rayLength = 5.0f,
        .rayOffsetX = 8.0f,  // Check left and right edges
        .groundLayers = jframe::CollisionLayers::Ground
    };

    auto groundCheck = sys.physics->checkGrounded(player_, params);
    bool wasGrounded = controller->isGrounded;
    controller->isGrounded = groundCheck.isGrounded;

    // Update coyote time (grace period after leaving ground)
    if (!controller->isGrounded && wasGrounded) {
        controller->coyoteTime = PlayerController::kCoyoteTimeMax;
    } else if (controller->isGrounded) {
        controller->coyoteTime = PlayerController::kCoyoteTimeMax;
    } else {
        controller->coyoteTime -= dt;
    }

    // Can jump if grounded OR within coyote time
    if (jumpPressed && (controller->isGrounded || controller->coyoteTime > 0.0f)) {
        jump();
        controller->coyoteTime = 0.0f;
    }
}
```

## Spatial Queries

Query physics bodies in a region.

### AABB Query

```cpp
// Find all entities in a rectangular area
jframe::Vec2 min = {100.0f, 100.0f};
jframe::Vec2 max = {300.0f, 300.0f};

std::vector<jframe::Entity> entities = sys.physics->queryAABB(min, max);

for (jframe::Entity entity : entities) {
    // Do something with entity
    jframe::core::logInfo(std::format("Found entity in area"));
}
```

### Circle Query

```cpp
// Find all entities within a radius
jframe::Vec2 center = {400.0f, 300.0f};
float radius = 100.0f;

std::vector<jframe::Entity> entities = sys.physics->queryCircle(center, radius);

// Example: Explosion damage
for (jframe::Entity entity : entities) {
    if (auto* health = sys.entities->tryGet<Health>(entity)) {
        health->current -= 50;
    }
}
```

### Raycasting

```cpp
// Cast a ray to find the first hit
jframe::Vec2 origin = {100.0f, 300.0f};
jframe::Vec2 direction = {1.0f, 0.0f};  // Normalized
float maxDistance = 500.0f;

auto hit = sys.physics->raycast(origin, direction, maxDistance);

if (hit) {
    jframe::Entity hitEntity = hit->entity;
    jframe::Vec2 hitPoint = hit->point;
    jframe::Vec2 hitNormal = hit->normal;
    float distance = hit->distance;

    jframe::core::logInfo(std::format("Hit at ({}, {}), distance: {}",
        hitPoint.x, hitPoint.y, distance));
}
```

### Raycast with Filtering

```cpp
// Only hit enemies
jframe::CollisionMask mask = jframe::CollisionLayers::Enemy;
auto hit = sys.physics->raycast(origin, direction, maxDistance, mask);

if (hit) {
    // Hit an enemy
    damageEnemy(hit->entity, 25);
}
```

### Raycast All

```cpp
// Get all entities hit by a ray
auto hits = sys.physics->raycastAll(origin, direction, maxDistance);

for (const auto& hit : hits) {
    jframe::core::logInfo(std::format("Hit entity at distance: {}", hit.distance));
}
```

## World Settings

```cpp
// Set gravity
sys.physics->setGravity({0.0f, -980.0f});  // 980 pixels/s^2 downward

// Get current gravity
jframe::Vec2 gravity = sys.physics->getGravity();

// Example: Low gravity level
sys.physics->setGravity({0.0f, -300.0f});

// Example: Side-scrolling with horizontal gravity
sys.physics->setGravity({-500.0f, 0.0f});

// Example: Zero gravity (space level)
sys.physics->setGravity({0.0f, 0.0f});
```

## Common Patterns

### One-Way Platforms

```cpp
// Create a platform the player can jump through from below
jframe::Entity platform = sys.entities->createEntity();

jframe::PhysicsBodyDef platformDef{
    .type = jframe::BodyType::Static,
    .transform = {.x = 300.0f, .y = 200.0f},
    .size = {200.0f, 20.0f}
};
sys.physics->createBody(platform, platformDef);

// In collision callback
void handleCollision(const jframe::EventData& data) {
    const auto& collision = std::get<jframe::CollisionEvent>(data);

    // If player is moving upward and hits platform from below, ignore collision
    if (isPlatform(collision.entityB) && collision.isBegin) {
        jframe::Vec2 vel = sys.physics->getVelocity(player_);
        if (vel.y > 0) {
            // Moving up - pass through
            // (This requires setting platform as sensor temporarily)
        }
    }
}
```

### Pushable Box

```cpp
jframe::Entity box = sys.entities->createEntity();

jframe::PhysicsBodyDef boxDef{
    .type = jframe::BodyType::Dynamic,
    .transform = {.x = 300.0f, .y = 200.0f},
    .size = {40.0f, 40.0f},
    .fixedRotation = true,
    .linearDamping = 2.0f,  // Slows down quickly when not pushed
    .gravityScale = 2.0f    // Heavier than normal
};
sys.physics->createBody(box, boxDef);

// Player can push it by walking into it (physics handles automatically)
```

### Jump Pad

```cpp
jframe::Entity jumpPad = sys.entities->createEntity();

jframe::PhysicsBodyDef padDef{
    .type = jframe::BodyType::Static,
    .transform = {.x = 400.0f, .y = 100.0f},
    .size = {60.0f, 20.0f}
};
sys.physics->createBody(jumpPad, padDef);
sys.physics->setSensor(jumpPad, true);

// In collision callback
if (isJumpPad(collision.entityB) && collision.isBegin) {
    // Launch player upward
    jframe::Vec2 vel = sys.physics->getVelocity(player_);
    vel.y = 800.0f;  // High jump
    sys.physics->setVelocity(player_, vel);
}
```

### Moving Platform

```cpp
struct MovingPlatform {
    jframe::Vec2 startPos;
    jframe::Vec2 endPos;
    float speed = 100.0f;
    float time = 0.0f;
    float duration = 4.0f;
};

void updateMovingPlatforms(jframe::DeltaTime dt) {
    auto view = sys.entities->getRegistry().view<MovingPlatform>();

    for (auto [entity, platform] : view.each()) {
        platform.time += dt;

        // Ping-pong between start and end
        float t = std::abs(std::fmod(platform.time / platform.duration, 2.0f) - 1.0f);

        jframe::Vec2 newPos = {
            platform.startPos.x + t * (platform.endPos.x - platform.startPos.x),
            platform.startPos.y + t * (platform.endPos.y - platform.startPos.y)
        };

        sys.physics->setPosition(entity, newPos);
    }
}
```

## Best Practices

### 1. Use Appropriate Body Types

- **Dynamic** - Objects that move and react to forces
- **Static** - Objects that never move
- **Kinematic** - Objects that move programmatically

### 2. Set Collision Layers Carefully

Only check collisions that matter for gameplay:

```cpp
// Bad: Everything collides with everything
sys.physics->setCollisionMask(entity, 0xFFFF);

// Good: Only necessary collisions
sys.physics->setCollisionMask(player, Terrain | Enemy | Collectible);
```

### 3. Use Sensors for Triggers

Don't create physical collision for triggers:

```cpp
// Checkpoint, level exit, damage zone, etc.
sys.physics->setSensor(trigger, true);
```

### 4. Fixed Rotation for Characters

Prevent characters from tipping over:

```cpp
jframe::PhysicsBodyDef playerDef{
    .type = jframe::BodyType::Dynamic,
    .fixedRotation = true  // IMPORTANT for platformers
};
```

### 5. Clean Up Physics Bodies

```cpp
// When destroying an entity with physics
sys.physics->destroyBody(entity);
sys.entities->destroyEntity(entity);
```

## Troubleshooting

**Entity falls through the ground**
- Make sure ground is `BodyType::Static`
- Check collision layers and masks
- Verify ground was created before physics update

**Collision callback not firing**
- Check collision layers - entities must have matching layer/mask
- Verify you subscribed to `Events::Collision`
- Make sure both entities have physics bodies

**Player jitters on ground**
- Set `fixedRotation = true`
- Increase position iterations in physics config
- Use ground detection instead of checking velocity

**Objects move too slow/fast**
- Adjust gravity: `sys.physics->setGravity({0, -980})`
- Tune velocity and force values
- Use `linearDamping` to control how quickly things slow down

## Next Steps

Now you understand physics and collision! Next tutorials:

- **Tutorial 4: Input and Controls** - Advanced input handling and rebinding
- **Tutorial 5: Audio** - Adding sounds and music to your game

## Further Reading

- Box2D Manual: https://box2d.org/documentation/
- JFrame Physics System API: `docs/systems/physics.md`
- Example: `examples/platformer/src/Game.cpp`
