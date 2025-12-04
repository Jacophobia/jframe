// jframe-camera/src/jframe.camera.impl.cppm
// Camera system implementation module

module;

#include <glm/glm.hpp>
// MSVC C++23 module compatibility - use full EnTT header
#include <jframe/entt_compat.hpp>

export module jframe.camera.impl;

import jframe.types;
import jframe.camera;
import std;

export namespace jframe {

class CameraSystem : public ICameraSystem {
public:
    explicit CameraSystem(Size viewportSize);
    ~CameraSystem() override = default;

    // Target following
    void setTarget(Entity target) override;
    void clearTarget() override;
    Entity getTarget() const override;

    // Follow behavior
    void setFollowSmoothing(float smoothing) override;
    void setOffset(Vec2 offset) override;
    void setDeadzone(Vec2 size) override;

    // Bounds
    void setBounds(float minX, float maxX, float minY, float maxY) override;
    void clearBounds() override;

    // Effects
    void shake(float intensity, float duration) override;
    void stopShake() override;

    // Zoom
    void setZoom(float zoom) override;
    float getZoom() const override;

    // Update and state
    void update(DeltaTime dt, Vec2 targetPosition) override;
    Camera getCamera() const override;
    Vec2 getPosition() const override;

    // Coordinate conversion
    Vec2 screenToWorld(Vec2 screenPos) const override;
    Vec2 worldToScreen(Vec2 worldPos) const override;

    // Additional utility
    void setViewportSize(Size size);

private:
    // Camera state
    Vec2 position_;
    Vec2 targetPosition_;
    float zoom_;
    Size viewportSize_;

    // Target following
    Entity target_;
    Vec2 offset_;
    float smoothing_;
    Vec2 deadzoneSize_;

    // Bounds
    bool hasBounds_;
    float minX_, maxX_, minY_, maxY_;

    // Shake effect
    bool isShaking_;
    float shakeIntensity_;
    float shakeDuration_;
    float shakeTimer_;
    Vec2 shakeOffset_;

    // Internal helper methods
    void updateShake(DeltaTime dt);
    void applySmoothing(DeltaTime dt, Vec2 target);
    void applyDeadzone(Vec2 target);
    void applyBounds();
    Vec2 generateShakeOffset() const;
};

// Factory function
inline std::unique_ptr<ICameraSystem> createCameraSystem(Size viewportSize) {
    return std::make_unique<CameraSystem>(viewportSize);
}

}  // namespace jframe
