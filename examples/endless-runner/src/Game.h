// examples/endless-runner/src/Game.h
// Endless runner game class

#pragma once

import jframe;
import jframe.core;
import jframe.camera.impl;
import jframe.components;
import jframe.config.impl;

#if defined(JFRAME_DEV_TOOLS)
import jframe.dev;
#endif

namespace endless_runner {

class Game : public jframe::core::Application {
public:
    Game() = default;
    ~Game() override = default;

    bool initialize(jframe::core::Engine& engine) override;
    void updateFixed(jframe::DeltaTime dt) override;
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
    void handlePlayerInput(jframe::DeltaTime dt);
    void updateCamera(jframe::DeltaTime dt);
    void syncPhysicsToTransforms();
    void onTriggerEnter(const jframe::EventData& data);
    void onCollision(const jframe::EventData& data);
    void renderEntities();
    void renderHUD();

    jframe::core::Engine* engine_ = nullptr;
    std::unique_ptr<jframe::IConfigSystem> config_;
    jframe::Entity player_;
    std::unique_ptr<jframe::CameraSystem> cameraSystem_;

    // Sprite sheets
    jframe::SpriteSheet playerSheet_;
    jframe::AnimatedSprite playerSprite_;
    jframe::SpriteSheet coinSheet_;
    jframe::SpriteSheet enemySheet_;
    jframe::SpriteSheet platformSheet_;

    // Asset handles
    jframe::AssetHandle playerTextureHandle_;
    jframe::AssetHandle coinTextureHandle_;
    jframe::AssetHandle enemyTextureHandle_;
    jframe::AssetHandle platformTextureHandle_;
    jframe::AssetHandle fontHandle_;
    jframe::AssetHandle levelAssetHandle_;
    jframe::AssetHandle jumpSoundHandle_;
    jframe::AssetHandle coinSoundHandle_;
    jframe::AssetHandle hurtSoundHandle_;

    jframe::LevelId currentLevelId_{};
    jframe::SubscriptionId triggerSubscription_;
    jframe::SubscriptionId collisionSubscription_;

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

#if defined(JFRAME_DEV_TOOLS)
    jframe::dev::HotReloadManager hotReload_;
    jframe::dev::DevOverlay devOverlay_;
#endif
};

}  // namespace endless_runner
