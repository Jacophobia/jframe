// examples/platformer/src/Components.h
// Game-specific components for the platformer example

#pragma once

import bestow;

namespace platformer {

// Player movement component
struct PlayerController {
    float moveSpeed = 200.0f;
    float jumpForce = 450.0f;
    float airControl = 0.3f;
    bool isGrounded = false;
    float coyoteTime = 0.0f;
    static constexpr float kCoyoteTimeMax = 0.1f;
};

// Simple 2D velocity component
struct Velocity {
    float dx = 0.0f;
    float dy = 0.0f;
};

// Health component
struct Health {
    int current = 100;
    int maximum = 100;
    float invincibilityTime = 0.0f;
    float invincibilityDuration = 1.0f;
};

// Camera follow component
struct Camera2D {
    bestow::Entity target;
    float smoothing = 0.1f;
    bestow::Vec2 offset = {0.0f, 100.0f};
    bestow::Vec2 position = {0.0f, 0.0f};
};

// Enemy AI component
struct EnemyAI {
    bestow::Vec2 patrolStart = {0.0f, 0.0f};
    float patrolRange = 150.0f;
    float moveSpeed = 50.0f;
    int damage = 10;
    bool movingRight = true;
};

// Collectible component
struct Collectible {
    enum class Type { Coin, Health, PowerUp };
    Type type = Type::Coin;
    int value = 10;
    bool collected = false;
};

// Save data structures
struct PlayerSaveData {
    bestow::Vec2 position;
    int health;
    int score;
    float playtime;

    void serialize(bestow::ISaveArchive& archive) const {
        archive.writeFloat("pos_x", position.x);
        archive.writeFloat("pos_y", position.y);
        archive.writeInt("health", health);
        archive.writeInt("score", score);
        archive.writeFloat("playtime", playtime);
    }

    void deserialize(const bestow::ILoadArchive& archive) {
        position.x = archive.readFloat("pos_x");
        position.y = archive.readFloat("pos_y");
        health = archive.readInt("health");
        score = archive.readInt("score");
        playtime = archive.readFloat("playtime");
    }
};

struct GameState {
    int score = 0;
    float playtime = 0.0f;
    int coinsCollected = 0;
    bool checkpointReached = false;
    bestow::Vec2 checkpointPosition = {100.0f, 400.0f};
};

// Tag components (empty tags need at least one byte for EnTT)
struct PlayerTag { char _dummy = 0; };
struct EnemyTag { char _dummy = 0; };
struct CollectibleTag { char _dummy = 0; };
struct PlatformTag { char _dummy = 0; };

}  // namespace platformer
