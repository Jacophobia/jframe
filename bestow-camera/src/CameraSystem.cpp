// bestow-camera/src/CameraSystem.cpp
// Camera system implementation

module;

#include <glm/glm.hpp>
// MSVC C++23 module compatibility - use full EnTT header
#include <bestow/entt_compat.hpp>

module bestow.camera.impl;

import std;

namespace bestow {

namespace {
    constexpr float kDefaultZoom = 1.0f;
    constexpr float kMinZoom = 0.1f;
    constexpr float kMaxZoom = 10.0f;

    // Linear interpolation for smooth camera movement
    float lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }

    Vec2 lerp(Vec2 a, Vec2 b, float t) {
        return Vec2{lerp(a.x, b.x, t), lerp(a.y, b.y, t)};
    }

    // Random float in range [-1, 1]
    float randomFloat() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        return dist(gen);
    }
}

CameraSystem::CameraSystem(Size viewportSize)
    : position_{0.0f, 0.0f}
    , targetPosition_{0.0f, 0.0f}
    , zoom_{kDefaultZoom}
    , viewportSize_{viewportSize}
    , target_{entt::null}
    , offset_{0.0f, 0.0f}
    , smoothing_{0.0f}
    , deadzoneSize_{0.0f, 0.0f}
    , hasBounds_{false}
    , minX_{0.0f}, maxX_{0.0f}, minY_{0.0f}, maxY_{0.0f}
    , isShaking_{false}
    , shakeIntensity_{0.0f}
    , shakeDuration_{0.0f}
    , shakeTimer_{0.0f}
    , shakeOffset_{0.0f, 0.0f}
{
}

// Target following
void CameraSystem::setTarget(Entity target) {
    target_ = target;
}

void CameraSystem::clearTarget() {
    target_ = entt::null;
}

Entity CameraSystem::getTarget() const {
    return target_;
}

// Follow behavior
void CameraSystem::setFollowSmoothing(float smoothing) {
    smoothing_ = std::clamp(smoothing, 0.0f, 1.0f);
}

void CameraSystem::setOffset(Vec2 offset) {
    offset_ = offset;
}

void CameraSystem::setDeadzone(Vec2 size) {
    deadzoneSize_ = Vec2{std::abs(size.x), std::abs(size.y)};
}

// Bounds
void CameraSystem::setBounds(float minX, float maxX, float minY, float maxY) {
    hasBounds_ = true;
    minX_ = minX;
    maxX_ = maxX;
    minY_ = minY;
    maxY_ = maxY;
}

void CameraSystem::clearBounds() {
    hasBounds_ = false;
}

// Effects
void CameraSystem::shake(float intensity, float duration) {
    isShaking_ = true;
    shakeIntensity_ = intensity;
    shakeDuration_ = duration;
    shakeTimer_ = 0.0f;
}

void CameraSystem::stopShake() {
    isShaking_ = false;
    shakeTimer_ = 0.0f;
    shakeOffset_ = Vec2{0.0f, 0.0f};
}

// Zoom
void CameraSystem::setZoom(float zoom) {
    zoom_ = std::clamp(zoom, kMinZoom, kMaxZoom);
}

float CameraSystem::getZoom() const {
    return zoom_;
}

// Update and state
void CameraSystem::update(DeltaTime dt, Vec2 targetPosition) {
    // Apply offset to target position
    Vec2 adjustedTarget = targetPosition + offset_;

    // Apply deadzone logic
    applyDeadzone(adjustedTarget);

    // Apply smoothing
    applySmoothing(dt, targetPosition_);

    // Apply bounds constraints
    applyBounds();

    // Update shake effect
    updateShake(dt);
}

Camera CameraSystem::getCamera() const {
    Camera camera;
    camera.transform.x = position_.x + shakeOffset_.x;
    camera.transform.y = position_.y + shakeOffset_.y;
    camera.transform.rotation = 0.0f;
    camera.transform.scaleX = 1.0f;
    camera.transform.scaleY = 1.0f;
    camera.zoom = zoom_;
    camera.viewportSize = viewportSize_;
    return camera;
}

Vec2 CameraSystem::getPosition() const {
    return position_;
}

// Coordinate conversion
Vec2 CameraSystem::screenToWorld(Vec2 screenPos) const {
    // Convert screen coordinates to world coordinates
    // Screen origin is top-left, world origin is center
    Vec2 halfViewport{
        viewportSize_.width * 0.5f,
        viewportSize_.height * 0.5f
    };

    Vec2 worldPos;
    worldPos.x = (screenPos.x - halfViewport.x) / zoom_ + position_.x;
    worldPos.y = (screenPos.y - halfViewport.y) / zoom_ + position_.y;

    return worldPos;
}

Vec2 CameraSystem::worldToScreen(Vec2 worldPos) const {
    // Convert world coordinates to screen coordinates
    Vec2 halfViewport{
        viewportSize_.width * 0.5f,
        viewportSize_.height * 0.5f
    };

    Vec2 screenPos;
    screenPos.x = (worldPos.x - position_.x) * zoom_ + halfViewport.x;
    screenPos.y = (worldPos.y - position_.y) * zoom_ + halfViewport.y;

    return screenPos;
}

// Additional utility
void CameraSystem::setViewportSize(Size size) {
    viewportSize_ = size;
}

// Internal helper methods
void CameraSystem::updateShake(DeltaTime dt) {
    if (!isShaking_) {
        shakeOffset_ = Vec2{0.0f, 0.0f};
        return;
    }

    shakeTimer_ += dt;

    if (shakeTimer_ >= shakeDuration_) {
        stopShake();
        return;
    }

    // Calculate decay factor (shake intensity decreases over time)
    float progress = shakeTimer_ / shakeDuration_;
    float decayFactor = 1.0f - progress;

    // Generate new shake offset
    shakeOffset_ = generateShakeOffset() * shakeIntensity_ * decayFactor;
}

void CameraSystem::applySmoothing(DeltaTime dt, Vec2 target) {
    if (smoothing_ <= 0.0f) {
        // No smoothing - snap instantly
        position_ = target;
    } else {
        // Smooth interpolation
        // Higher smoothing = slower movement
        float lerpFactor = 1.0f - std::pow(smoothing_, dt * 60.0f);
        position_ = lerp(position_, target, lerpFactor);
    }
}

void CameraSystem::applyDeadzone(Vec2 target) {
    // If there's no deadzone, just update target position
    if (deadzoneSize_.x <= 0.0f && deadzoneSize_.y <= 0.0f) {
        targetPosition_ = target;
        return;
    }

    // Calculate deadzone boundaries
    float halfDeadzoneX = deadzoneSize_.x * 0.5f;
    float halfDeadzoneY = deadzoneSize_.y * 0.5f;

    // Check if target is outside deadzone relative to CURRENT position
    Vec2 delta = target - position_;

    // X-axis deadzone
    if (delta.x > halfDeadzoneX) {
        targetPosition_.x = target.x;
    } else if (delta.x < -halfDeadzoneX) {
        targetPosition_.x = target.x;
    } else {
        // Within deadzone - don't move
        targetPosition_.x = position_.x;
    }

    // Y-axis deadzone
    if (delta.y > halfDeadzoneY) {
        targetPosition_.y = target.y;
    } else if (delta.y < -halfDeadzoneY) {
        targetPosition_.y = target.y;
    } else {
        // Within deadzone - don't move
        targetPosition_.y = position_.y;
    }
}

void CameraSystem::applyBounds() {
    if (!hasBounds_) {
        return;
    }

    // Calculate half viewport size in world coordinates
    float halfViewportWidth = (viewportSize_.width * 0.5f) / zoom_;
    float halfViewportHeight = (viewportSize_.height * 0.5f) / zoom_;

    // Check if bounds are smaller than viewport (handle edge case)
    float minPossibleX = minX_ + halfViewportWidth;
    float maxPossibleX = maxX_ - halfViewportWidth;

    if (minPossibleX > maxPossibleX) {
        // Viewport larger than bounds - center camera within bounds
        position_.x = (minX_ + maxX_) * 0.5f;
    } else {
        position_.x = std::clamp(position_.x, minPossibleX, maxPossibleX);
    }

    float minPossibleY = minY_ + halfViewportHeight;
    float maxPossibleY = maxY_ - halfViewportHeight;

    if (minPossibleY > maxPossibleY) {
        // Viewport larger than bounds - center camera within bounds
        position_.y = (minY_ + maxY_) * 0.5f;
    } else {
        position_.y = std::clamp(position_.y, minPossibleY, maxPossibleY);
    }
}

Vec2 CameraSystem::generateShakeOffset() const {
    return Vec2{randomFloat(), randomFloat()};
}

}  // namespace bestow
