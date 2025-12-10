// bestow-contract/src/bestow.utils.cppm
// Core utilities that can be used by any system without circular dependencies
// This module does NOT depend on the bestow aggregate module

module;

#include <spdlog/spdlog.h>

export module bestow.utils;

import std;
import bestow.types;

export namespace bestow::utils {

//==========================================================================
// Logging
//==========================================================================

inline void logInfo(const std::string& message) {
    spdlog::info(message);
}

inline void logWarn(const std::string& message) {
    spdlog::warn(message);
}

inline void logError(const std::string& message) {
    spdlog::error(message);
}

inline void logDebug(const std::string& message) {
    spdlog::debug(message);
}

//==========================================================================
// Timer
//==========================================================================

class Timer {
public:
    Timer() : start_(std::chrono::high_resolution_clock::now()) {}

    void reset() {
        start_ = std::chrono::high_resolution_clock::now();
    }

    float elapsedSeconds() const {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<float>(now - start_).count();
    }

    float elapsedMilliseconds() const {
        return elapsedSeconds() * 1000.0f;
    }

private:
    std::chrono::high_resolution_clock::time_point start_;
};

//==========================================================================
// Frame Timer
//==========================================================================

class FrameTimer {
public:
    DeltaTime tick() {
        auto now = std::chrono::high_resolution_clock::now();
        DeltaTime dt = std::chrono::duration<float>(now - lastFrame_).count();
        lastFrame_ = now;
        frameCount_++;
        return dt;
    }

    std::uint64_t frameCount() const { return frameCount_; }

private:
    std::chrono::high_resolution_clock::time_point lastFrame_ =
        std::chrono::high_resolution_clock::now();
    std::uint64_t frameCount_ = 0;
};

//==========================================================================
// Easing Functions
//==========================================================================

namespace Easing {

inline float linear(float t) {
    return t;
}

inline float easeInQuad(float t) {
    return t * t;
}

inline float easeOutQuad(float t) {
    return t * (2.0f - t);
}

inline float easeInOutQuad(float t) {
    return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}

inline float easeInCubic(float t) {
    return t * t * t;
}

inline float easeOutCubic(float t) {
    float f = t - 1.0f;
    return f * f * f + 1.0f;
}

inline float easeInOutCubic(float t) {
    return t < 0.5f ? 4.0f * t * t * t
                    : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f;
}

inline float easeInSine(float t) {
    return 1.0f - std::cos(t * std::numbers::pi_v<float> / 2.0f);
}

inline float easeOutSine(float t) {
    return std::sin(t * std::numbers::pi_v<float> / 2.0f);
}

inline float easeInOutSine(float t) {
    return 0.5f * (1.0f - std::cos(std::numbers::pi_v<float> * t));
}

inline float easeInExpo(float t) {
    return t == 0.0f ? 0.0f : std::pow(2.0f, 10.0f * (t - 1.0f));
}

inline float easeOutExpo(float t) {
    return t == 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);
}

inline float easeOutBounce(float t) {
    if (t < 1.0f / 2.75f) {
        return 7.5625f * t * t;
    } else if (t < 2.0f / 2.75f) {
        t -= 1.5f / 2.75f;
        return 7.5625f * t * t + 0.75f;
    } else if (t < 2.5f / 2.75f) {
        t -= 2.25f / 2.75f;
        return 7.5625f * t * t + 0.9375f;
    } else {
        t -= 2.625f / 2.75f;
        return 7.5625f * t * t + 0.984375f;
    }
}

inline float easeInBounce(float t) {
    return 1.0f - easeOutBounce(1.0f - t);
}

}  // namespace Easing

//==========================================================================
// Math Utilities
//==========================================================================

namespace Math {

// Constants
inline constexpr float PI = std::numbers::pi_v<float>;
inline constexpr float TWO_PI = 2.0f * PI;
inline constexpr float HALF_PI = PI / 2.0f;
inline constexpr float DEG_TO_RAD = PI / 180.0f;
inline constexpr float RAD_TO_DEG = 180.0f / PI;

// Angle conversion
[[nodiscard]] constexpr float degreesToRadians(float degrees) noexcept {
    return degrees * DEG_TO_RAD;
}

[[nodiscard]] constexpr float radiansToDegrees(float radians) noexcept {
    return radians * RAD_TO_DEG;
}

// Linear interpolation
[[nodiscard]] constexpr float lerp(float a, float b, float t) noexcept {
    return a + t * (b - a);
}

[[nodiscard]] inline Vec2 lerp(Vec2 a, Vec2 b, float t) noexcept {
    return Vec2{lerp(a.x, b.x, t), lerp(a.y, b.y, t)};
}

[[nodiscard]] inline Vec3 lerp(Vec3 a, Vec3 b, float t) noexcept {
    return Vec3{lerp(a.x, b.x, t), lerp(a.y, b.y, t), lerp(a.z, b.z, t)};
}

// Smoothstep interpolation
[[nodiscard]] constexpr float smoothstep(float edge0, float edge1, float x) noexcept {
    float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// Random number generation
[[nodiscard]] inline float randomFloat(float min = -1.0f, float max = 1.0f) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(min, max);
    return dist(gen);
}

[[nodiscard]] inline int randomInt(int min, int max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(min, max);
    return dist(gen);
}

}  // namespace Math

//==========================================================================
// UUID Generation
//==========================================================================

inline UUID generateUUID() {
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<UUID> dis;
    return dis(gen);
}

}  // namespace bestow::utils

// Also expose in bestow::core namespace for backwards compatibility
export namespace bestow::core {
    using bestow::utils::logInfo;
    using bestow::utils::logWarn;
    using bestow::utils::logError;
    using bestow::utils::logDebug;
    using bestow::utils::Timer;
    using bestow::utils::FrameTimer;
    namespace Easing = bestow::utils::Easing;
    namespace Math = bestow::utils::Math;
    using bestow::utils::generateUUID;
}
