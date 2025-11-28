// examples/platformer-demo/src/Game.h
// Game class demonstrating all JFrame engine features

#pragma once

import jframe;
import jframe.core;
import jframe.camera.impl;
import jframe.components;
import jframe.config.impl;

#if defined(JFRAME_DEV_TOOLS)
import jframe.dev;
#endif

namespace platformer_demo {

// Player animation states
enum class PlayerState {
    Idle,
    Running,
    Jumping,
    Falling
};

class Game : public jframe::core::Application {
public:
    Game() = default;
    ~Game() override = default;

    // Application lifecycle
    bool initialize(jframe::core::Engine& engine) override;
    void updateFixed(jframe::DeltaTime dt) override;
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
    void handlePlayerInput(jframe::DeltaTime dt);
    void updatePlayerAnimation();
    void updateCamera(jframe::DeltaTime dt);
    void syncPhysicsToTransforms();
    void checkTriggers();

    // Event handlers
    void onTriggerEnter(const jframe::EventData& data);
    void onCollision(const jframe::EventData& data);

    // Rendering helpers
    void renderHUD();
    void renderEntities();

    // Engine reference
    jframe::core::Engine* engine_ = nullptr;

    // Config system
    std::unique_ptr<jframe::IConfigSystem> config_;

    // Core entities
    jframe::Entity player_;

    // Camera system
    std::unique_ptr<jframe::CameraSystem> cameraSystem_;

    // Player sprite sheet and animations
    jframe::SpriteSheet playerSheet_;
    jframe::AnimatedSprite playerSprite_;
    PlayerState playerState_ = PlayerState::Idle;

    // Static sprite sheets (single-frame for simple sprites)
    jframe::SpriteSheet coinSheet_;
    jframe::SpriteSheet enemySheet_;
    jframe::SpriteSheet platformSheet_;

    // Note: Enemies and coins are tracked via ECS using EnemyTag/CollectibleTag
    // with EnemyPatrol component for patrol behavior

    // Asset handles
    jframe::AssetHandle playerTextureHandle_;
    jframe::AssetHandle coinTextureHandle_;
    jframe::AssetHandle enemyTextureHandle_;
    jframe::AssetHandle platformTextureHandle_;
    jframe::AssetHandle fontHandle_;
    jframe::AssetHandle levelAssetHandle_;

    // Level tracking
    jframe::LevelId currentLevelId_{};

    // Audio handles
    jframe::AssetHandle jumpSoundHandle_;
    jframe::AssetHandle coinSoundHandle_;
    jframe::AssetHandle hurtSoundHandle_;

    // Event subscriptions
    jframe::SubscriptionId triggerSubscription_;
    jframe::SubscriptionId collisionSubscription_;

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
    jframe::Color hudHealthBarBgColor_{50, 50, 50, 255};
    jframe::Color hudHealthBarFillColor_{0, 255, 0, 255};
    // Score text config
    int hudScoreX_ = 620;
    int hudScoreY_ = 10;
    float hudScoreFontSize_ = 16.0f;
    std::string hudScorePrefix_ = "SCORE: ";
    jframe::Color hudScoreColor_{255, 215, 0, 255};
    // Game over text config
    std::string hudGameOverText_ = "GAME OVER";
    float hudGameOverFontSize_ = 32.0f;
    jframe::Color hudGameOverColor_{200, 0, 0, 255};

#if defined(JFRAME_DEV_TOOLS)
    jframe::dev::HotReloadManager hotReload_;
    jframe::dev::DevOverlay devOverlay_;
#endif
};

}  // namespace platformer_demo
