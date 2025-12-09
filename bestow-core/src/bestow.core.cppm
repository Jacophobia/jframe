// bestow-core/src/bestow.core.cppm
// Core utilities and application framework
//
// STATUS: Implemented 2025-11-25
// COMPLETE: Timer, FrameTimer, Easing, JobSystem, UUID, Logging
// PARTIAL: Application (needs game loop integration with systems)
// BLOCKED: None
// TESTS: CoreSystemTests.cpp added (pending build verification)

module;

#include <spdlog/spdlog.h>
#include <taskflow/taskflow.hpp>

export module bestow.core;

import std;
import bestow;

export namespace bestow::core {

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
// Engine Configuration
//==========================================================================

struct GraphicsConfig {
    int width = 1280;
    int height = 720;
    std::string title = "Bestow Application";
    bool vsync = true;
    Color clearColor = Color{26, 26, 26, 255};  // Dark gray (0.1 * 255 ≈ 26)
};

//==========================================================================
// Forward Declarations
//==========================================================================

class Engine;

//==========================================================================
// Engine Builder
//==========================================================================

class EngineBuilder {
public:
    EngineBuilder();
    ~EngineBuilder();

    // System configuration (fluent interface)
    EngineBuilder& withEvents();
    EngineBuilder& withEntities();
    EngineBuilder& withPhysics();
    EngineBuilder& withPhysics3D();
    EngineBuilder& withGraphics(GraphicsConfig config);
    EngineBuilder& withGraphics3D(GraphicsConfig config);
    EngineBuilder& withAudio();
    EngineBuilder& withInput();
    EngineBuilder& withAssets(std::string_view basePath);
    EngineBuilder& withSave(std::string_view savePath);
    EngineBuilder& withLevel();
    EngineBuilder& withAI();
    EngineBuilder& withCamera(Size viewportSize);
    EngineBuilder& withGAS();
    EngineBuilder& withBlueprints();
    EngineBuilder& withUI();
    EngineBuilder& withGameStates();

    // Build the engine
    std::expected<Engine, std::string> build();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

//==========================================================================
// Engine
//==========================================================================

class Engine {
public:
    Engine();
    ~Engine();

    // Move-only
    Engine(Engine&&) noexcept;
    Engine& operator=(Engine&&) noexcept;
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    // Get the engine aggregate (all systems)
    BestowEngine& systems();
    const BestowEngine& systems() const;

    // Game loop control
    void run(class Application& app);
    void quit();
    bool isRunning() const;

private:
    friend class EngineBuilder;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

//==========================================================================
// Application Base
//==========================================================================

class Application {
public:
    Application() = default;
    virtual ~Application() = default;

    // Non-copyable
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    // Lifecycle hooks (called by Engine)
    virtual bool initialize(Engine& engine) = 0;
    virtual void updateFixed(DeltaTime dt) = 0;
    virtual void render(float alpha) = 0;
    virtual void shutdown() = 0;
};

//==========================================================================
// Job System
//==========================================================================

class JobSystem {
public:
    JobSystem() : executor_(), taskflow_() {}
    ~JobSystem() = default;

    // Non-copyable, non-movable (tf::Executor is not movable)
    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;
    JobSystem(JobSystem&&) = delete;
    JobSystem& operator=(JobSystem&&) = delete;

    // Submit a job for parallel execution
    // Jobs are batched until wait() is called
    template<typename F>
    void submit(F&& func) {
        taskflow_.emplace(std::forward<F>(func));
    }

    // Submit multiple jobs that can run in parallel
    template<typename F>
    void submitBatch(std::span<F> funcs) {
        for (auto& func : funcs) {
            taskflow_.emplace(std::move(func));
        }
    }

    // Wait for all submitted jobs to complete
    // Note: Jobs submitted during execution will remain in the taskflow
    // and require a subsequent wait() call
    void wait() {
        if (!taskflow_.empty()) {
            // Create a new taskflow for the current batch
            tf::Taskflow currentBatch = std::move(taskflow_);
            taskflow_ = tf::Taskflow{};

            // Run only the current batch
            executor_.run(currentBatch).wait();
        }
    }

    // Get number of worker threads
    std::size_t workerCount() const {
        return executor_.num_workers();
    }

    // Check if there are pending jobs
    bool hasPendingJobs() const {
        return !taskflow_.empty();
    }

private:
    tf::Executor executor_;
    tf::Taskflow taskflow_;
};

//==========================================================================
// UUID Generation
//==========================================================================

inline UUID generateUUID() {
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<UUID> dis;
    return dis(gen);
}

}  // namespace bestow::core
