// examples/platformer-demo/src/Game.h
// Game class demonstrating all Bestow engine features

#pragma once

import bestow;
import bestow.core;
import bestow.camera.impl;
import bestow.components;
import bestow.config.impl;

#if defined(BESTOW_DEV_TOOLS)
import bestow.dev;
#endif

namespace platformer_demo {

// Player animation states
enum class PlayerState {
    Idle,
    Running,
    Jumping,
    Falling
};

class Game : public bestow::core::Application {
public:
    Game() = default;
    ~Game() override = default;

    // Application lifecycle
    bool initialize(bestow::core::Engine& engine) override;
    void updateFixed(bestow::DeltaTime dt) override;
    void render(float alpha) override;
    void shutdown() override;

private:
    // Initialization helpers
    void setupPlayer();
    void loadLevel();
    void createPlatform(float x, float y, float width, float height);
    void createCoin(float x, float y, int value);
    void createEnemy(float x, float y, float patrolRange);

    // Update helpers
    void handlePlayerInput(bestow::DeltaTime dt);
    void updatePlayerAnimation();
    void updateCamera(bestow::DeltaTime dt);
    void syncPhysicsToTransforms();
    void checkTriggers();

    // Event handlers
    void onTriggerEnter(const bestow::EventData& data);
    void onCollision(const bestow::EventData& data);

    // Rendering helpers
    void renderHUD();
    void renderEntities();

    // Engine reference
    bestow::core::Engine* engine_ = nullptr;

    // Config system
    std::unique_ptr<bestow::IConfigSystem> config_;

    // Core entities
    bestow::Entity player_;

    // Camera system
    std::unique_ptr<bestow::CameraSystem> cameraSystem_;

    // Player sprite sheet and animations
    bestow::SpriteSheet playerSheet_;
    bestow::AnimatedSprite playerSprite_;
    PlayerState playerState_ = PlayerState::Idle;

    // Static sprite sheets (single-frame for simple sprites)
    bestow::SpriteSheet coinSheet_;
    bestow::SpriteSheet enemySheet_;
    bestow::SpriteSheet platformSheet_;

    // Note: Enemies and coins are tracked via ECS using EnemyTag/CollectibleTag
    // with EnemyPatrol component for patrol behavior

    // Asset handles
    bestow::AssetHandle playerTextureHandle_;
    bestow::AssetHandle coinTextureHandle_;
    bestow::AssetHandle enemyTextureHandle_;
    bestow::AssetHandle platformTextureHandle_;
    bestow::AssetHandle fontHandle_;
    bestow::AssetHandle levelAssetHandle_;

    // Level tracking
    bestow::LevelId currentLevelId_{};

    // Audio handles
    bestow::AssetHandle jumpSoundHandle_;
    bestow::AssetHandle coinSoundHandle_;
    bestow::AssetHandle hurtSoundHandle_;

    // Event subscriptions
    bestow::SubscriptionId triggerSubscription_;
    bestow::SubscriptionId collisionSubscription_;

    // Game state
    int score_ = 0;
    int health_ = 100;
    bool gameOver_ = false;

    // Config-driven gameplay values (read once at init, used every frame)
    float playerMoveSpeed_ = 400.0f;
    float playerJumpForce_ = 800.0f;
    int playerMaxJumps_ = 2;
    float groundedVelocityThreshold_ = 50.0f;
    float runningAnimThreshold_ = 10.0f;
    float jumpingAnimThreshold_ = -10.0f;
    float jumpSoundVolume_ = 0.8f;
    float coinSoundVolume_ = 0.7f;
    float hurtSoundVolume_ = 1.0f;
    float cameraShakeMagnitude_ = 10.0f;
    float cameraShakeDuration_ = 0.3f;

    // Config-driven physics values
    float playerBodyWidth_ = 50.0f;
    float playerBodyHeight_ = 80.0f;
    float playerDensity_ = 1.0f;
    float playerFriction_ = 0.0f;
    float playerRestitution_ = 0.0f;
    float coinBodySize_ = 30.0f;
    float enemyBodySize_ = 40.0f;
    float enemyDensity_ = 1.0f;
    float enemyFriction_ = 0.3f;
    int enemyDamage_ = 20;
    float enemyPatrolSpeed_ = 50.0f;
    float platformFriction_ = 0.5f;

    // Config-driven HUD values
    int hudHealthBarX_ = 10;
    int hudHealthBarY_ = 10;
    int hudHealthBarWidth_ = 200;
    int hudHealthBarHeight_ = 20;
    bestow::Color hudHealthBarBgColor_{50, 50, 50, 255};
    bestow::Color hudHealthBarFillColor_{0, 255, 0, 255};
    // Score text config
    int hudScoreX_ = 620;
    int hudScoreY_ = 10;
    float hudScoreFontSize_ = 16.0f;
    std::string hudScorePrefix_ = "SCORE: ";
    bestow::Color hudScoreColor_{255, 215, 0, 255};
    // Game over text config
    std::string hudGameOverText_ = "GAME OVER";
    float hudGameOverFontSize_ = 32.0f;
    bestow::Color hudGameOverColor_{200, 0, 0, 255};

#if defined(BESTOW_DEV_TOOLS)
    bestow::dev::HotReloadManager hotReload_;
    bestow::dev::DevOverlay devOverlay_;
#endif
};

}  // namespace platformer_demo
