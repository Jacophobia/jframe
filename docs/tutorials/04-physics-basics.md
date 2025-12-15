# Tutorial 04: Physics Basics

In this tutorial, you'll learn how to create physics bodies, apply forces and impulses, handle collisions, and implement platformer-specific features like ground detection. Bestow uses Box2D for 2D physics simulation.

## What You'll Build

By the end of this tutorial, you'll have:
- Static, kinematic, and dynamic physics bodies
- Collision layers and filtering
- Force and impulse application
- Ground detection for jumping
- Collision callbacks for game events
- One-way platforms
- Moving platforms

## Prerequisites

Before starting, complete Tutorials 01-03 and understand:
- Entity creation and components
- Input handling
- Basic rendering
- The update loop

## Understanding Physics Bodies

Bestow's physics system wraps Box2D and integrates with the Entity-Component-System. Every physics object is an Entity with a physics body.

### Body Types

| Type | Description | Use Cases |
|------|-------------|-----------|
| **Static** | Never moves, infinite mass | Ground, walls, platforms |
| **Kinematic** | Moves but ignores forces | Moving platforms, doors |
| **Dynamic** | Affected by gravity and forces | Player, enemies, projectiles, boxes |

### Key Concepts

- **Body**: Physical representation of an entity
- **Fixture**: Shape attached to body (defines collision geometry)
- **Force**: Gradual acceleration over time (like a jet engine)
- **Impulse**: Instant velocity change (like a jump or explosion)
- **Collision Layer**: What category this body belongs to
- **Collision Mask**: Which layers this body can collide with

## Step 1: Creating Physics Bodies

Define and create bodies for different entity types.

### Static Body (Ground)

```cpp
// In Game.cpp - initialize()
auto& sys = engine_->systems();

// Create ground entity
bestow::Entity ground = sys.entities->createEntity();

// Define physics body
bestow::PhysicsBodyDef groundDef{
    .type = bestow::BodyType::Static,          // Never moves
    .transform = {
        .x = 400.0f,                           // Center X
        .y = 50.0f                             // Center Y
    },
    .size = {800.0f, 100.0f},                  // Width x Height
    .fixedRotation = true,                     // Prevent rotation
    .density = 1.0f,                           // Not used for static
    .friction = 0.5f,                          // Surface friction
    .restitution = 0.0f,                       // Bounciness (0 = no bounce)
    .isSensor = false                          // Solid collision
};

// Create the body
sys.physics->createBody(ground, groundDef);
```

### Dynamic Body (Player)

```cpp
// Create player entity
bestow::Entity player = sys.entities->createEntity();

// Define dynamic body
bestow::PhysicsBodyDef playerDef{
    .type = bestow::BodyType::Dynamic,         // Affected by forces
    .transform = {.x = 400.0f, .y = 300.0f},
    .size = {32.0f, 48.0f},                    // Width x Height
    .fixedRotation = true,                     // Prevent rotation (important for platformers!)
    .density = 1.0f,                           // Mass calculation
    .friction = 0.3f,                          // Surface friction
    .restitution = 0.0f,                       // No bounce
    .gravityScale = 1.0f,                      // Affected by gravity
    .isSensor = false
};

sys.physics->createBody(player, playerDef);
```

### Kinematic Body (Moving Platform)

```cpp
// Create moving platform
bestow::Entity platform = sys.entities->createEntity();

bestow::PhysicsBodyDef platformDef{
    .type = bestow::BodyType::Kinematic,       // Moves but ignores forces
    .transform = {.x = 300.0f, .y = 200.0f},
    .size = {100.0f, 20.0f},
    .fixedRotation = true,
    .friction = 0.5f,
    .isSensor = false
};

sys.physics->createBody(platform, platformDef);
```

## Step 2: Applying Forces and Impulses

Move dynamic bodies using physics.

### Velocity Control (Direct)

```cpp
// In Game.cpp - updateFixed()
auto& sys = engine_->systems();

// Get current velocity
bestow::Vec2 velocity = sys.physics->getVelocity(player_);

// Modify horizontal velocity based on input
float horizontal = 0.0f;
if (sys.input->isActionActive("move_right")) horizontal += 1.0f;
if (sys.input->isActionActive("move_left")) horizontal -= 1.0f;

constexpr float MOVE_SPEED = 200.0f;
velocity.x = horizontal * MOVE_SPEED;

// Set new velocity
sys.physics->setVelocity(player_, velocity);
```

### Apply Force (Gradual Acceleration)

```cpp
// Apply continuous force (like a jet engine)
bestow::Vec2 force = {1000.0f, 0.0f};  // Force in Newtons
sys.physics->applyForce(player_, force);

// Apply force at world point (creates torque)
bestow::Vec2 worldPoint = {playerPos.x, playerPos.y + 10.0f};
sys.physics->applyForceAtPoint(player_, force, worldPoint);
```

### Apply Impulse (Instant Velocity Change)

```cpp
// Jump - instant upward velocity change
if (sys.input->wasActionJustPressed("jump")) {
    auto groundCheck = sys.physics->checkGrounded(player_);
    if (groundCheck.grounded) {
        bestow::Vec2 jumpImpulse = {0.0f, -500.0f};  // Negative Y = up
        sys.physics->applyImpulse(player_, jumpImpulse);
    }
}

// Explosion knockback
bestow::Vec2 explosionPos = {300.0f, 200.0f};
bestow::Vec2 playerPos = sys.physics->getPosition(player_);

bestow::Vec2 direction = {
    playerPos.x - explosionPos.x,
    playerPos.y - explosionPos.y
};

float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
if (length > 0.0f) {
    direction.x /= length;
    direction.y /= length;
}

constexpr float EXPLOSION_FORCE = 1000.0f;
bestow::Vec2 knockback = {
    direction.x * EXPLOSION_FORCE,
    direction.y * EXPLOSION_FORCE
};

sys.physics->applyImpulse(player_, knockback);
```

## Step 3: Collision Layers and Filtering

Control which bodies collide with each other using layers and masks.

### Collision Layers

```cpp
// Define collision layers (bit flags)
namespace CollisionLayer {
    constexpr uint16_t None = 0;
    constexpr uint16_t Ground = 1 << 0;      // 0x0001
    constexpr uint16_t Player = 1 << 1;      // 0x0002
    constexpr uint16_t Enemy = 1 << 2;       // 0x0004
    constexpr uint16_t Projectile = 1 << 3;  // 0x0008
    constexpr uint16_t Pickup = 1 << 4;      // 0x0010
    constexpr uint16_t Trigger = 1 << 5;     // 0x0020
    constexpr uint16_t All = 0xFFFF;
}
```

### Setting Collision Filters

```cpp
// Player collides with ground and enemies
bestow::PhysicsBodyDef playerDef{
    .type = bestow::BodyType::Dynamic,
    .transform = {400.0f, 300.0f},
    .size = {32.0f, 48.0f},
    .fixedRotation = true,
    .collisionLayer = CollisionLayer::Player,
    .collisionMask = CollisionLayer::Ground | CollisionLayer::Enemy | CollisionLayer::Pickup
};

// Projectile collides with enemies and ground (not player)
bestow::PhysicsBodyDef projectileDef{
    .type = bestow::BodyType::Dynamic,
    .transform = {100.0f, 200.0f},
    .size = {8.0f, 8.0f},
    .fixedRotation = false,
    .collisionLayer = CollisionLayer::Projectile,
    .collisionMask = CollisionLayer::Ground | CollisionLayer::Enemy
};

// Pickup (coin) is a sensor - triggers events but doesn't collide
bestow::PhysicsBodyDef pickupDef{
    .type = bestow::BodyType::Static,
    .transform = {200.0f, 150.0f},
    .size = {16.0f, 16.0f},
    .isSensor = true,  // No collision, just triggers
    .collisionLayer = CollisionLayer::Pickup,
    .collisionMask = CollisionLayer::Player
};

sys.physics->createBody(player_, playerDef);
sys.physics->createBody(projectile_, projectileDef);
sys.physics->createBody(coin_, pickupDef);
```

### Understanding Layers vs Masks

- **collisionLayer**: What category IS this body?
- **collisionMask**: What categories CAN this body collide with?

```cpp
// Example: Friendly fire system
constexpr uint16_t PlayerProjectile = 1 << 6;
constexpr uint16_t EnemyProjectile = 1 << 7;

// Player projectile only hits enemies
bestow::PhysicsBodyDef playerBulletDef{
    .collisionLayer = CollisionLayer::PlayerProjectile,
    .collisionMask = CollisionLayer::Enemy | CollisionLayer::Ground
};

// Enemy projectile only hits player
bestow::PhysicsBodyDef enemyBulletDef{
    .collisionLayer = CollisionLayer::EnemyProjectile,
    .collisionMask = CollisionLayer::Player | CollisionLayer::Ground
};
```

## Step 4: Ground Detection

Implement reliable ground detection for jumping.

### Simple Ground Check

```cpp
// In Game.cpp - updateFixed()
auto& sys = engine_->systems();

// Check if player is on ground
auto groundCheck = sys.physics->checkGrounded(player_);

if (groundCheck.grounded) {
    // Player is standing on something
    bestow::core::logInfo("On ground");

    // Can jump
    if (sys.input->wasActionJustPressed("jump")) {
        sys.physics->applyImpulse(player_, {0.0f, -500.0f});
    }
} else {
    // Player is in the air
    bestow::core::logInfo("Airborne");
}
```

### GroundCheckResult Structure

```cpp
struct GroundCheckResult {
    bool grounded;              // Is entity on ground?
    bestow::Entity groundEntity; // What entity are we standing on?
    bestow::Vec2 normal;        // Surface normal (usually {0, -1})
    float distance;             // Distance to ground
};
```

### Custom Ground Check Distance

```cpp
// Check ground within specific distance
float checkDistance = 5.0f;  // Check 5 pixels below
auto groundCheck = sys.physics->checkGrounded(player_, checkDistance);

if (groundCheck.grounded && groundCheck.distance < 2.0f) {
    // Very close to ground - can jump
}
```

### Coyote Time (Late Jump)

Allow jumping shortly after leaving a platform:

```cpp
// In Game.h
private:
    float coyoteTimer_ = 0.0f;
    constexpr float COYOTE_TIME = 0.1f;  // 100ms grace period

// In Game.cpp - updateFixed()
auto groundCheck = sys.physics->checkGrounded(player_);

if (groundCheck.grounded) {
    coyoteTimer_ = COYOTE_TIME;  // Reset timer
} else {
    coyoteTimer_ -= dt;  // Count down
}

// Can jump if grounded OR within coyote time
if (sys.input->wasActionJustPressed("jump") && coyoteTimer_ > 0.0f) {
    sys.physics->applyImpulse(player_, {0.0f, -500.0f});
    coyoteTimer_ = 0.0f;  // Consume jump
}
```

### Jump Buffer (Early Jump)

Allow jump input slightly before landing:

```cpp
// In Game.h
private:
    float jumpBufferTimer_ = 0.0f;
    constexpr float JUMP_BUFFER_TIME = 0.1f;

// In Game.cpp - updateFixed()
// Record jump input
if (sys.input->wasActionJustPressed("jump")) {
    jumpBufferTimer_ = JUMP_BUFFER_TIME;
}

// Count down buffer
if (jumpBufferTimer_ > 0.0f) {
    jumpBufferTimer_ -= dt;
}

// Execute buffered jump when landing
auto groundCheck = sys.physics->checkGrounded(player_);
if (groundCheck.grounded && jumpBufferTimer_ > 0.0f) {
    sys.physics->applyImpulse(player_, {0.0f, -500.0f});
    jumpBufferTimer_ = 0.0f;  // Consume buffer
}
```

## Step 5: Collision Callbacks

React to collisions with game logic.

### Subscribe to Collision Events

```cpp
// In Game.h
private:
    bestow::SubscriptionId collisionSubId_;

// In Game.cpp - initialize()
auto& sys = engine_->systems();

collisionSubId_ = sys.events->subscribe(
    bestow::Events::Collision,
    [this](const bestow::EventData& data) {
        auto& event = std::get<bestow::CollisionEvent>(data);
        onCollision(event);
    }
);

// In shutdown()
sys.events->unsubscribe(collisionSubId_);
```

### Handle Collision Event

```cpp
void Game::onCollision(const bestow::CollisionEvent& event) {
    auto& sys = engine_->systems();

    // Check if player collided with enemy
    if (event.entityA == player_ && isEnemy(event.entityB)) {
        takeDamage(10);
    } else if (event.entityB == player_ && isEnemy(event.entityA)) {
        takeDamage(10);
    }

    // Check if player collected a coin
    if (event.entityA == player_ && isCoin(event.entityB)) {
        collectCoin(event.entityB);
    } else if (event.entityB == player_ && isCoin(event.entityA)) {
        collectCoin(event.entityA);
    }

    // Check if projectile hit enemy
    if (isProjectile(event.entityA) && isEnemy(event.entityB)) {
        damageEnemy(event.entityB, 25);
        destroyProjectile(event.entityA);
    }
}

bool Game::isEnemy(bestow::Entity entity) {
    return engine_->systems().entities->has<EnemyComponent>(entity);
}

bool Game::isCoin(bestow::Entity entity) {
    return engine_->systems().entities->has<CoinComponent>(entity);
}

void Game::collectCoin(bestow::Entity coin) {
    auto& sys = engine_->systems();

    // Add score
    score_ += 10;

    // Play sound
    sys.audio->playSound(coinSound_);

    // Destroy coin entity
    sys.entities->destroyEntity(coin);
}
```

### CollisionEvent Structure

```cpp
struct CollisionEvent {
    bestow::Entity entityA;       // First colliding entity
    bestow::Entity entityB;       // Second colliding entity
    bestow::Vec2 point;           // Contact point in world space
    bestow::Vec2 normal;          // Collision normal (direction)
    float impulse;                // Collision impulse magnitude
    bool isSensor;                // Was this a sensor trigger?
};
```

## Step 6: One-Way Platforms

Create platforms you can jump through from below.

### Creating One-Way Platform

```cpp
// In Game.cpp - initialize()
auto& sys = engine_->systems();

bestow::Entity oneWayPlatform = sys.entities->createEntity();

bestow::PhysicsBodyDef platformDef{
    .type = bestow::BodyType::Static,
    .transform = {400.0f, 300.0f},
    .size = {200.0f, 20.0f},
    .fixedRotation = true,
    .collisionLayer = CollisionLayer::Ground,
    .collisionMask = CollisionLayer::Player,
    .isOneWay = true  // Enable one-way collision
};

sys.physics->createBody(oneWayPlatform, platformDef);

// Tag as one-way for custom logic
struct OneWayPlatform {};
sys.entities->emplace<OneWayPlatform>(oneWayPlatform);
```

### Custom One-Way Logic

```cpp
void Game::onCollision(const bestow::CollisionEvent& event) {
    auto& sys = engine_->systems();

    // Check if player is colliding with one-way platform
    bestow::Entity platform = bestow::NullEntity;

    if (event.entityA == player_ && sys.entities->has<OneWayPlatform>(event.entityB)) {
        platform = event.entityB;
    } else if (event.entityB == player_ && sys.entities->has<OneWayPlatform>(event.entityA)) {
        platform = event.entityA;
    }

    if (platform != bestow::NullEntity) {
        // Check if player is moving down
        bestow::Vec2 playerVel = sys.physics->getVelocity(player_);

        if (playerVel.y < 0.0f) {
            // Player moving down - disable collision
            sys.physics->setCollisionEnabled(player_, platform, false);
        } else {
            // Player moving up or static - enable collision
            sys.physics->setCollisionEnabled(player_, platform, true);
        }

        // Check if player pressed "drop down" key
        if (sys.input->isActionActive("drop_down")) {
            sys.physics->setCollisionEnabled(player_, platform, false);
        }
    }
}
```

## Step 7: Moving Platforms

Create platforms that carry the player.

### Basic Moving Platform

```cpp
// In Game.h
struct MovingPlatform {
    bestow::Vec2 startPos;
    bestow::Vec2 endPos;
    float speed;
    float t = 0.0f;  // Interpolation parameter (0 to 1)
    int direction = 1;  // 1 = forward, -1 = backward
};

// In Game.cpp - initialize()
auto& sys = engine_->systems();

bestow::Entity platform = sys.entities->createEntity();

bestow::PhysicsBodyDef platformDef{
    .type = bestow::BodyType::Kinematic,  // IMPORTANT: Use kinematic!
    .transform = {200.0f, 300.0f},
    .size = {100.0f, 20.0f},
    .fixedRotation = true
};

sys.physics->createBody(platform, platformDef);

// Add moving platform component
sys.entities->emplace<MovingPlatform>(platform, MovingPlatform{
    .startPos = {200.0f, 300.0f},
    .endPos = {600.0f, 300.0f},
    .speed = 100.0f  // Pixels per second
});
```

### Update Moving Platforms

```cpp
// In Game.cpp - updateFixed()
auto& sys = engine_->systems();

auto view = sys.entities->view<MovingPlatform>();
for (auto entity : view) {
    auto& platform = view.get<MovingPlatform>(entity);

    // Calculate movement distance this frame
    float distance = platform.speed * dt;
    float totalDistance = std::sqrt(
        std::pow(platform.endPos.x - platform.startPos.x, 2) +
        std::pow(platform.endPos.y - platform.startPos.y, 2)
    );

    // Update interpolation parameter
    platform.t += (distance / totalDistance) * platform.direction;

    // Reverse direction at endpoints
    if (platform.t >= 1.0f) {
        platform.t = 1.0f;
        platform.direction = -1;
    } else if (platform.t <= 0.0f) {
        platform.t = 0.0f;
        platform.direction = 1;
    }

    // Lerp position
    bestow::Vec2 newPos = {
        platform.startPos.x + (platform.endPos.x - platform.startPos.x) * platform.t,
        platform.startPos.y + (platform.endPos.y - platform.startPos.y) * platform.t
    };

    // Set platform position
    sys.physics->setPosition(entity, newPos);
}
```

### Carrying the Player

The physics system automatically handles player movement when standing on kinematic bodies. No extra code needed!

```cpp
// Box2D automatically:
// 1. Detects player standing on kinematic platform
// 2. Applies platform velocity to player
// 3. Player moves with platform
```

## Complete Example: Platformer Physics

Here's a complete example with jumping, ground detection, and collision handling:

```cpp
// Game.h
#pragma once

import bestow;
import bestow.core;

struct PlayerController {
    float moveSpeed = 200.0f;
    float jumpForce = 500.0f;
    float coyoteTimer = 0.0f;
    float jumpBufferTimer = 0.0f;
    int health = 100;
};

class Game {
public:
    bool initialize(bestow::core::Engine& engine);
    void updateFixed(bestow::DeltaTime dt);
    void render(float alpha);
    void shutdown();

private:
    void handlePlayerMovement(bestow::DeltaTime dt);
    void handlePlayerJump(bestow::DeltaTime dt);
    void onCollision(const bestow::CollisionEvent& event);
    void takeDamage(int amount);

    bestow::core::Engine* engine_ = nullptr;
    bestow::Entity player_;
    bestow::SubscriptionId collisionSubId_;

    static constexpr float COYOTE_TIME = 0.1f;
    static constexpr float JUMP_BUFFER_TIME = 0.1f;
};

// Game.cpp
import std;
import bestow;
import bestow.core;

#include "Game.h"

namespace CollisionLayer {
    constexpr uint16_t Ground = 1 << 0;
    constexpr uint16_t Player = 1 << 1;
    constexpr uint16_t Enemy = 1 << 2;
}

bool Game::initialize(bestow::core::Engine& engine) {
    engine_ = &engine;
    auto& sys = engine.systems();

    // Create ground
    bestow::Entity ground = sys.entities->createEntity();
    sys.physics->createBody(ground, bestow::PhysicsBodyDef{
        .type = bestow::BodyType::Static,
        .transform = {400.0f, 50.0f},
        .size = {800.0f, 100.0f},
        .collisionLayer = CollisionLayer::Ground,
        .collisionMask = CollisionLayer::Player | CollisionLayer::Enemy
    });

    // Create player
    player_ = sys.entities->createEntity();
    sys.physics->createBody(player_, bestow::PhysicsBodyDef{
        .type = bestow::BodyType::Dynamic,
        .transform = {400.0f, 300.0f},
        .size = {32.0f, 48.0f},
        .fixedRotation = true,
        .collisionLayer = CollisionLayer::Player,
        .collisionMask = CollisionLayer::Ground | CollisionLayer::Enemy
    });

    // Add player controller
    sys.entities->emplace<PlayerController>(player_);

    // Subscribe to collisions
    collisionSubId_ = sys.events->subscribe(
        bestow::Events::Collision,
        [this](const bestow::EventData& data) {
            auto& event = std::get<bestow::CollisionEvent>(data);
            onCollision(event);
        }
    );

    return true;
}

void Game::updateFixed(bestow::DeltaTime dt) {
    handlePlayerMovement(dt);
    handlePlayerJump(dt);
}

void Game::handlePlayerMovement(bestow::DeltaTime dt) {
    auto& sys = engine_->systems();
    auto& controller = sys.entities->get<PlayerController>(player_);

    // Get horizontal input
    float horizontal = 0.0f;
    if (sys.input->isActionActive("move_right")) horizontal += 1.0f;
    if (sys.input->isActionActive("move_left")) horizontal -= 1.0f;

    // Set horizontal velocity
    bestow::Vec2 velocity = sys.physics->getVelocity(player_);
    velocity.x = horizontal * controller.moveSpeed;
    sys.physics->setVelocity(player_, velocity);
}

void Game::handlePlayerJump(bestow::DeltaTime dt) {
    auto& sys = engine_->systems();
    auto& controller = sys.entities->get<PlayerController>(player_);

    // Check ground
    auto groundCheck = sys.physics->checkGrounded(player_);

    // Update coyote timer
    if (groundCheck.grounded) {
        controller.coyoteTimer = COYOTE_TIME;
    } else {
        controller.coyoteTimer -= dt;
    }

    // Record jump input
    if (sys.input->wasActionJustPressed("jump")) {
        controller.jumpBufferTimer = JUMP_BUFFER_TIME;
    }

    // Count down buffer
    if (controller.jumpBufferTimer > 0.0f) {
        controller.jumpBufferTimer -= dt;
    }

    // Execute jump (coyote time OR buffered input)
    if (controller.coyoteTimer > 0.0f && controller.jumpBufferTimer > 0.0f) {
        sys.physics->applyImpulse(player_, {0.0f, -controller.jumpForce});
        controller.coyoteTimer = 0.0f;
        controller.jumpBufferTimer = 0.0f;
    }
}

void Game::onCollision(const bestow::CollisionEvent& event) {
    // Handle enemy collision
    if (event.entityA == player_ || event.entityB == player_) {
        // Player collided with something
        // Add your collision logic here
    }
}

void Game::takeDamage(int amount) {
    auto& sys = engine_->systems();
    auto& controller = sys.entities->get<PlayerController>(player_);

    controller.health -= amount;
    bestow::core::logInfo(std::format("Health: {}", controller.health));

    if (controller.health <= 0) {
        bestow::core::logInfo("Player died!");
    }
}

void Game::render(float alpha) {
    auto& sys = engine_->systems();
    bestow::Vec2 pos = sys.physics->getPosition(player_);

    // Draw player
    sys.graphics->drawRect(
        {static_cast<int>(pos.x - 16), static_cast<int>(pos.y - 24), 32, 48},
        bestow::Color::blue()
    );
}

void Game::shutdown() {
    engine_->systems().events->unsubscribe(collisionSubId_);
    bestow::core::logInfo("Shutting down");
}
```

## Best Practices

### 1. Use fixedRotation for Platformer Characters

```cpp
// DO THIS for platformers
.fixedRotation = true

// DON'T allow rotation unless you want toppling characters
```

### 2. Use Kinematic for Moving Platforms

```cpp
// DO THIS - kinematic bodies move but aren't affected by forces
.type = bestow::BodyType::Kinematic

// DON'T use Dynamic - forces will affect platform movement
```

### 3. Use Sensors for Triggers

```cpp
// DO THIS for collectibles, triggers, damage zones
.isSensor = true

// DON'T make them solid if they shouldn't block movement
```

### 4. Apply Gravity Scale for Different Fall Speeds

```cpp
// Normal gravity
.gravityScale = 1.0f

// Floaty (slow fall)
.gravityScale = 0.5f

// Heavy (fast fall)
.gravityScale = 2.0f
```

### 5. Check Ground Before Jumping

```cpp
// ALWAYS check ground state
auto groundCheck = sys.physics->checkGrounded(player_);
if (groundCheck.grounded) {
    // Can jump
}

// DON'T allow infinite jumps without checking
```

## Next Steps

You now understand physics fundamentals! Continue learning:

- **Tutorial 05: Audio** - Add sound effects and music to your game
- **Tutorial 06: 3D Platformer** - Expand into 3D physics
- **Advanced Physics** - Raycasting, joints, continuous collision detection

## Troubleshooting

**Player falls through ground**
- Ensure ground body is `BodyType::Static`
- Check collision layers/masks are set correctly
- Verify body sizes are correct
- Use `fixedRotation = true` for player

**Player jumps infinitely**
- Add ground check before allowing jump
- Check `groundCheck.grounded` is true

**Movement is jittery**
- Set velocity in `updateFixed()`, not `render()`
- Use `fixedRotation = true`
- Check physics timestep is stable

**Collision callbacks not firing**
- Verify you subscribed to `Events::Collision`
- Check collision layers/masks allow collision
- Ensure both entities have physics bodies

**Player sticks to walls**
- Reduce friction: `.friction = 0.0f`
- Use circle shape for player body (reduces catching)
- Apply air control differently than ground control

**Moving platforms don't carry player**
- Use `BodyType::Kinematic` for platform
- Don't set velocity manually - use `setPosition()`
- Ensure platform has physics body
