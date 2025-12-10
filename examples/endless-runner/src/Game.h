// examples/endless-runner/src/Game.h
// Endless runner game class

#pragma once

import bestow;
import bestow.core;

#if defined(BESTOW_DEV_TOOLS)
import bestow.dev;
#endif

namespace endless_runner {

// Camera component for smooth following
struct Camera2D {
    bestow::Entity target;
    float smoothing = 5.0f;
    float zoom = 1.0f;
    bestow::Vec2 position = {0.0f, 0.0f};
    bestow::Vec2 offset = {0.0f, 0.0f};
    float shakeIntensity = 0.0f;
    float shakeDuration = 0.0f;

    void shake(float intensity, float duration) {
        shakeIntensity = intensity;
        shakeDuration = duration;
    }

    void update(bestow::DeltaTime dt) {
        if (shakeDuration > 0.0f) {
            shakeDuration -= dt;
            if (shakeDuration <= 0.0f) {
                shakeIntensity = 0.0f;
            }
        }
    }

    bestow::Camera getCamera() const {
        bestow::Camera cam;
        cam.transform.x = position.x;
        cam.transform.y = position.y;
        cam.zoom = zoom;
        cam.viewportSize = {1280, 720};
        return cam;
    }
};

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
    bestow::Entity player_;
    bestow::Entity camera_;

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
