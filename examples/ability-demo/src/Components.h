// examples/ability-demo/src/Components.h
// Game-specific components for the ability demo

#pragma once

import bestow;

namespace abilitydemo {

// Player tag component (needs dummy member for EnTT emplace return)
struct PlayerTag { char _dummy = 0; };

// Platform tag component
struct PlatformTag { char _dummy = 0; };

// Jump zone tag component
struct JumpZoneTag { char _dummy = 0; };

// Collectable tag - grants abilities when picked up
struct CollectableTag {
    std::string abilityToGrant;  // Name of the ability to grant (e.g., "Dash")
};

// Size component - stores entity dimensions (used for both physics and rendering)
struct Size2D {
    float width = 32.0f;
    float height = 32.0f;
};

// Player controller component
struct PlayerController {
    float moveSpeed = 200.0f;
    float jumpForce = 400.0f;
    bool isGrounded = false;
};

// Camera follow component
struct Camera2D {
    bestow::Entity target;
    float smoothing = 5.0f;
    bestow::Vec2 offset = {0.0f, -50.0f};
    bestow::Vec2 position = {400.0f, 300.0f};
};

// Visual effect component (for showing active effects)
struct EffectVisual {
    bestow::Color tintColor = bestow::Color::white();
    float pulseTimer = 0.0f;
};

// ============================================================================
// Enemy Components
// ============================================================================

// Enemy tag - type determines behavior
struct EnemyTag {
    std::string type;  // "walker", "jumper", "shooter", "flying"
    int health = 30;
    int damage = 10;
    float detectionRange = 200.0f;
};

// Walker enemy - patrols left-right
struct WalkerAI {
    float patrolLeft = 0.0f;
    float patrolRight = 0.0f;
    float speed = 80.0f;
    bool movingRight = true;
};

// Jumper enemy - jumps toward player
struct JumperAI {
    float jumpForce = 350.0f;
    float jumpCooldown = 1.5f;
    float jumpTimer = 0.0f;
};

// Shooter enemy - fires projectiles
struct ShooterAI {
    float fireRate = 2.0f;
    float fireTimer = 0.0f;
    float projectileSpeed = 300.0f;
};

// Flying enemy - moves up and down
struct FlyingAI {
    float topY = 0.0f;
    float bottomY = 0.0f;
    float speed = 60.0f;
    bool movingUp = true;
};

// Boss - multiple phases
struct BossTag {
    int maxHealth = 500;
    int phase = 1;  // 1, 2, or 3
    float attackCooldown = 2.0f;
};

// ============================================================================
// Projectile Component
// ============================================================================

struct Projectile {
    float velocityX = 0.0f;
    float velocityY = 0.0f;
    int damage = 5;
    bool isEnemyProjectile = false;
    float lifetime = 3.0f;
};

// ============================================================================
// Level Mechanic Components
// ============================================================================

// Switch that triggers doors
struct SwitchTag {
    int targetDoorId = 0;
    bool activated = false;
};

// Door that blocks paths
struct DoorTag {
    int doorId = 0;
    bool isOpen = false;
};

// Breakable surface (ground pound destroys)
struct BreakableTag {
    int hits = 1;  // hits required to break
};

// Moving platform
struct MovingPlatform {
    float startX = 0.0f;
    float startY = 0.0f;
    float endX = 0.0f;
    float endY = 0.0f;
    float speed = 50.0f;
    float progress = 0.0f;
    bool forward = true;
};

// Health pickup
struct HealthPickup {
    int healAmount = 25;
};

// Checkpoint
struct Checkpoint {
    bool activated = false;
};

// Trigger zone for events
struct TriggerZone {
    std::string event;  // "boss_start", "door_open", "spawn_enemies", etc.
    bool triggered = false;
};

// ============================================================================
// Combat Components
// ============================================================================

// Sword hitbox (temporary entity during attacks)
struct SwordHitbox {
    int damage = 10;
    float lifetime = 0.2f;
};

// Damage flash effect
struct DamageFlash {
    float timer = 0.0f;
    float duration = 0.1f;
};

// Knockback on hit
struct Knockback {
    float forceX = 0.0f;
    float forceY = 0.0f;
    float duration = 0.2f;
    float timer = 0.0f;
};

}  // namespace abilitydemo
