// examples/endless-runner/src/Game.h
// Endless runner game class

#pragma once

import bestow;
import bestow.core;
import bestow.camera.impl;
import bestow.components;
import bestow.config.impl;

#if defined(BESTOW_DEV_TOOLS)
import bestow.dev;
#endif

namespace endless_runner {

class Game : public bestow::core::Application {
public:
    Game() = default;
    ~Game() override = default;

    bool initialize(bestow::core::Engine& engine) override;
    void updateFixed(bestow::DeltaTime dt) override;
    void render(float alpha) override;
    void shutdown() override;

private:
    void setupPlayer();
    void loadInitialSection();
    void restartGame();
    void generateNextSection();
    void despawnBehindCamera();
    void createPlatform(float x, float y, float width, float height);
    void createCoin(float x, float y, int value);
    void createEnemy(float x, float y);
    void handlePlayerInput(bestow::DeltaTime dt);
    void updateCamera(bestow::DeltaTime dt);
    void syncPhysicsToTransforms();
    void onTriggerEnter(const bestow::EventData& data);
    void onCollision(const bestow::EventData& data);
    void renderEntities();
    void renderHUD();

    bestow::core::Engine* engine_ = nullptr;
    std::unique_ptr<bestow::IConfigSystem> config_;
    bestow::Entity player_;
    std::unique_ptr<bestow::CameraSystem> cameraSystem_;

    // Sprite sheets
    bestow::SpriteSheet playerSheet_;
    bestow::AnimatedSprite playerSprite_;
    bestow::SpriteSheet coinSheet_;
    bestow::SpriteSheet enemySheet_;
    bestow::SpriteSheet platformSheet_;

    // Asset handles
    bestow::AssetHandle playerTextureHandle_;
    bestow::AssetHandle coinTextureHandle_;
    bestow::AssetHandle enemyTextureHandle_;
    bestow::AssetHandle platformTextureHandle_;
    bestow::AssetHandle fontHandle_;
    bestow::AssetHandle levelAssetHandle_;
    bestow::AssetHandle jumpSoundHandle_;
    bestow::AssetHandle coinSoundHandle_;
    bestow::AssetHandle hurtSoundHandle_;

    bestow::LevelId currentLevelId_{};
    bestow::SubscriptionId triggerSubscription_;
    bestow::SubscriptionId collisionSubscription_;

    // Game state
    int score_ = 0;
    int health_ = 100;
    bool gameOver_ = false;
    float distance_ = 0.0f;

    // Endless runner specific
    float runSpeed_ = 300.0f;
    float sectionWidth_ = 800.0f;
    float nextSectionX_ = 0.0f;
    float despawnX_ = -200.0f;
    int sectionsGenerated_ = 0;

    // Config values
    float jumpForce_ = 700.0f;
    float groundY_ = 550.0f;
    float playerBodyWidth_ = 50.0f;
    float playerBodyHeight_ = 80.0f;

#if defined(BESTOW_DEV_TOOLS)
    bestow::dev::HotReloadManager hotReload_;
    bestow::dev::DevOverlay devOverlay_;
#endif
};

}  // namespace endless_runner
