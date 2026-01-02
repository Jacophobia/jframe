// bestow-physics3d/src/FloatingOriginSystem.cpp
// Floating origin system implementation for large-world rendering

module;

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/quaternion_double.hpp>
#include <spdlog/spdlog.h>
#include <cmath>

module bestow.physics3d.impl;

import std;
import bestow.services;

namespace bestow {

FloatingOriginSystem::FloatingOriginSystem(IEventSystem* events)
    : pIEventSystem_(events)
    , config_{}
    , currentOrigin_{0.0, 0.0, 0.0}
    , cameraWorldPosition_{0.0, 0.0, 0.0}
    , shiftEpoch_{0}
    , framesSinceLastShift_{0}
{
    spdlog::info("[FloatingOriginSystem] Initialized with grid size {}m, threshold {}m",
                 config_.gridSize, config_.shiftThreshold);
}

FloatingOriginSystem::~FloatingOriginSystem() = default;

void FloatingOriginSystem::setCameraWorldPosition(const Vec3d& worldPosition) {
    cameraWorldPosition_ = worldPosition;
}

Vec3d FloatingOriginSystem::getOrigin() const {
    return currentOrigin_;
}

Vec3 FloatingOriginSystem::getCameraRenderPosition() const {
    return toRenderPosition(cameraWorldPosition_);
}

Vec3d FloatingOriginSystem::getCameraWorldPosition() const {
    return cameraWorldPosition_;
}

Vec3 FloatingOriginSystem::toRenderPosition(const Vec3d& worldPosition) const {
    // Convert world position to camera-relative single precision
    Vec3d relativePos = worldPosition - currentOrigin_;
    return Vec3(
        static_cast<float>(relativePos.x),
        static_cast<float>(relativePos.y),
        static_cast<float>(relativePos.z)
    );
}

Vec3d FloatingOriginSystem::toWorldPosition(const Vec3& renderPosition) const {
    return currentOrigin_ + Vec3d(
        static_cast<double>(renderPosition.x),
        static_cast<double>(renderPosition.y),
        static_cast<double>(renderPosition.z)
    );
}

Transform3D FloatingOriginSystem::toRenderTransform(const CelestialTransform& celestial) const {
    Vec3d relativePos = celestial.position - currentOrigin_;

    return Transform3D{
        .position = Vec3(
            static_cast<float>(relativePos.x),
            static_cast<float>(relativePos.y),
            static_cast<float>(relativePos.z)
        ),
        .rotation = Quat(
            static_cast<float>(celestial.rotation.w),
            static_cast<float>(celestial.rotation.x),
            static_cast<float>(celestial.rotation.y),
            static_cast<float>(celestial.rotation.z)
        ),
        .scale = celestial.scale
    };
}

std::uint32_t FloatingOriginSystem::getShiftEpoch() const {
    return shiftEpoch_;
}

bool FloatingOriginSystem::needsRebase(std::uint32_t lastEpoch) const {
    return lastEpoch < shiftEpoch_;
}

void FloatingOriginSystem::forceOriginShift(const Vec3d& newOrigin) {
    performOriginShift(newOrigin);
}

void FloatingOriginSystem::setConfig(const FloatingOriginConfig& config) {
    config_ = config;
    spdlog::info("[FloatingOriginSystem] Config updated: grid {}m, threshold {}m",
                 config_.gridSize, config_.shiftThreshold);
}

FloatingOriginConfig FloatingOriginSystem::getConfig() const {
    return config_;
}

void FloatingOriginSystem::update(DeltaTime dt) {
    ++framesSinceLastShift_;

    // Check if we're past the cooldown period
    if (framesSinceLastShift_ < config_.shiftCooldownFrames) {
        return;
    }

    // Check if shift is needed based on camera position relative to origin
    Vec3d cameraRelative = cameraWorldPosition_ - currentOrigin_;

    bool needsShift =
        std::abs(cameraRelative.x) > config_.shiftThreshold ||
        std::abs(cameraRelative.y) > config_.shiftThreshold ||
        std::abs(cameraRelative.z) > config_.shiftThreshold;

    if (needsShift) {
        // Calculate new origin snapped to grid
        Vec3d newOrigin{
            std::floor(cameraWorldPosition_.x / config_.gridSize) * config_.gridSize,
            std::floor(cameraWorldPosition_.y / config_.gridSize) * config_.gridSize,
            std::floor(cameraWorldPosition_.z / config_.gridSize) * config_.gridSize
        };

        // Only shift if origin actually changes
        if (newOrigin != currentOrigin_) {
            performOriginShift(newOrigin);
        }
    }
}

void FloatingOriginSystem::performOriginShift(const Vec3d& newOrigin) {
    Vec3d oldOrigin = currentOrigin_;
    Vec3d shiftDelta = newOrigin - oldOrigin;

    // Update state
    currentOrigin_ = newOrigin;
    ++shiftEpoch_;
    framesSinceLastShift_ = 0;

    spdlog::debug("[FloatingOriginSystem] Origin shifted: ({:.1f}, {:.1f}, {:.1f}) -> ({:.1f}, {:.1f}, {:.1f}), epoch {}",
                  oldOrigin.x, oldOrigin.y, oldOrigin.z,
                  newOrigin.x, newOrigin.y, newOrigin.z,
                  shiftEpoch_);

    // Publish event so other systems can react
    if (pIEventSystem_) {
        OriginShiftEventData eventData{
            .oldOrigin = oldOrigin,
            .newOrigin = newOrigin,
            .shiftDelta = shiftDelta,
            .newEpoch = shiftEpoch_
        };

        pIEventSystem_->publish("system.origin_shifted", eventData);
    }
}

}  // namespace bestow
