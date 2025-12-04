// bestow-metrics/src/bestow.metrics.cppm
// Comprehensive observability and profiling for Bestow
//
// This module provides:
// - Tracy profiler integration with wrapper macros
// - Per-system timing metrics
// - Code path tracking (function call counters)
// - Usage frequency analysis
// - Metrics export for dashboards
//
// STATUS: Implemented 2025-12-03
// COMPLETE: Tracy integration, SystemMetrics, CodePathTracker, MetricsCollector
// PARTIAL: None
// BLOCKED: None

module;

#include <spdlog/spdlog.h>

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#include <tracy/TracyC.h>
#endif

export module bestow.metrics;

import std;
import bestow.types;

export namespace bestow::metrics {

//=============================================================================
// Tracy Integration Macros (wrapped for conditional compilation)
//=============================================================================

// These are implemented as inline functions to work with C++23 modules
// Use the BESTOW_* macros in implementation files for zero-cost abstraction

#ifdef TRACY_ENABLE
    inline constexpr bool kTracyEnabled = true;
#else
    inline constexpr bool kTracyEnabled = false;
#endif

//=============================================================================
// System Identifier
//=============================================================================

enum class SystemId : std::uint8_t {
    Core = 0,
    Events,
    Entity,
    Graphics,
    Audio,
    Input,
    Physics,
    Assets,
    Save,
    Level,
    AI,
    Camera,
    Config,
    GAS,
    Blueprints,
    DevTools,
    Application,  // User application code
    Count
};

inline constexpr std::string_view systemIdToString(SystemId id) {
    switch (id) {
        case SystemId::Core: return "Core";
        case SystemId::Events: return "Events";
        case SystemId::Entity: return "Entity";
        case SystemId::Graphics: return "Graphics";
        case SystemId::Audio: return "Audio";
        case SystemId::Input: return "Input";
        case SystemId::Physics: return "Physics";
        case SystemId::Assets: return "Assets";
        case SystemId::Save: return "Save";
        case SystemId::Level: return "Level";
        case SystemId::AI: return "AI";
        case SystemId::Camera: return "Camera";
        case SystemId::Config: return "Config";
        case SystemId::GAS: return "GAS";
        case SystemId::Blueprints: return "Blueprints";
        case SystemId::DevTools: return "DevTools";
        case SystemId::Application: return "Application";
        default: return "Unknown";
    }
}

//=============================================================================
// Timing Metrics for a Single System
//=============================================================================

struct SystemTimingData {
    std::uint64_t callCount = 0;           // Total number of update() calls
    double totalTimeMs = 0.0;               // Cumulative time in milliseconds
    double lastFrameTimeMs = 0.0;           // Time spent in last frame
    double minTimeMs = std::numeric_limits<double>::max();
    double maxTimeMs = 0.0;
    double avgTimeMs = 0.0;                 // Running average

    // Rolling history for graphs (last 120 frames)
    static constexpr std::size_t kHistorySize = 120;
    std::array<float, kHistorySize> history{};
    std::size_t historyIndex = 0;

    void record(double timeMs) {
        callCount++;
        totalTimeMs += timeMs;
        lastFrameTimeMs = timeMs;
        minTimeMs = std::min(minTimeMs, timeMs);
        maxTimeMs = std::max(maxTimeMs, timeMs);
        avgTimeMs = totalTimeMs / static_cast<double>(callCount);

        history[historyIndex] = static_cast<float>(timeMs);
        historyIndex = (historyIndex + 1) % kHistorySize;
    }

    void reset() {
        callCount = 0;
        totalTimeMs = 0.0;
        lastFrameTimeMs = 0.0;
        minTimeMs = std::numeric_limits<double>::max();
        maxTimeMs = 0.0;
        avgTimeMs = 0.0;
        history.fill(0.0f);
        historyIndex = 0;
    }
};

//=============================================================================
// Code Path Tracking
//=============================================================================

struct CodePathStats {
    std::uint64_t callCount = 0;            // How many times this path was hit
    std::uint64_t lastFrameCount = 0;       // Calls in the last frame
    std::uint64_t framesSinceLastCall = 0;  // Frames since this was last hit
    bool calledThisFrame = false;

    void hit() {
        callCount++;
        lastFrameCount++;
        calledThisFrame = true;
    }

    void endFrame() {
        if (!calledThisFrame) {
            framesSinceLastCall++;
        } else {
            framesSinceLastCall = 0;
        }
        calledThisFrame = false;
        lastFrameCount = 0;
    }
};

//=============================================================================
// Scoped Timer - RAII timer for measuring code blocks
//=============================================================================

class ScopedTimer {
public:
    using Clock = std::chrono::high_resolution_clock;

    explicit ScopedTimer(double& targetMs)
        : targetMs_(targetMs)
        , start_(Clock::now())
    {}

    ~ScopedTimer() {
        auto end = Clock::now();
        auto duration = std::chrono::duration<double, std::milli>(end - start_);
        targetMs_ = duration.count();
    }

    // Non-copyable, non-movable
    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;
    ScopedTimer(ScopedTimer&&) = delete;
    ScopedTimer& operator=(ScopedTimer&&) = delete;

private:
    double& targetMs_;
    Clock::time_point start_;
};

//=============================================================================
// Metrics Collector - Central hub for all observability data
//=============================================================================

class MetricsCollector {
public:
    // Singleton access
    static MetricsCollector& instance();

    // System timing
    void beginSystemUpdate(SystemId system);
    void endSystemUpdate(SystemId system);
    const SystemTimingData& getSystemTiming(SystemId system) const;

    // Code path tracking
    void trackCodePath(std::string_view name);
    const std::unordered_map<std::string, CodePathStats>& getCodePaths() const;

    // Frame management
    void beginFrame();
    void endFrame();
    std::uint64_t frameCount() const { return frameCount_; }

    // Get all system timings for the current frame
    struct FrameMetrics {
        std::uint64_t frameNumber;
        double totalFrameTimeMs;
        std::array<double, static_cast<std::size_t>(SystemId::Count)> systemTimesMs;
    };
    FrameMetrics getCurrentFrameMetrics() const;

    // Reset all metrics
    void reset();

    // Export metrics as JSON string
    std::string exportToJson() const;

    // Tracy integration helpers
    void setTracyPlot(const char* name, double value);
    void sendTracyMessage(const char* message);

private:
    MetricsCollector() = default;

    // Per-system timing data
    std::array<SystemTimingData, static_cast<std::size_t>(SystemId::Count)> systemTimings_{};

    // Currently timing system (for nested timing protection)
    std::array<std::chrono::high_resolution_clock::time_point,
               static_cast<std::size_t>(SystemId::Count)> systemStartTimes_{};

    // Code path tracking
    std::unordered_map<std::string, CodePathStats> codePaths_;
    mutable std::mutex codePathMutex_;

    // Frame tracking
    std::uint64_t frameCount_ = 0;
    std::chrono::high_resolution_clock::time_point frameStartTime_;
    double lastFrameTotalMs_ = 0.0;
};

//=============================================================================
// Scoped System Timer - RAII wrapper for system timing
//=============================================================================

class ScopedSystemTimer {
public:
    explicit ScopedSystemTimer(SystemId system)
        : system_(system)
    {
        MetricsCollector::instance().beginSystemUpdate(system_);
    }

    ~ScopedSystemTimer() {
        MetricsCollector::instance().endSystemUpdate(system_);
    }

    // Non-copyable, non-movable
    ScopedSystemTimer(const ScopedSystemTimer&) = delete;
    ScopedSystemTimer& operator=(const ScopedSystemTimer&) = delete;
    ScopedSystemTimer(ScopedSystemTimer&&) = delete;
    ScopedSystemTimer& operator=(ScopedSystemTimer&&) = delete;

private:
    SystemId system_;
};

//=============================================================================
// Code Path Tracker - RAII wrapper for tracking code paths
//=============================================================================

class ScopedCodePath {
public:
    explicit ScopedCodePath(std::string_view name) {
        MetricsCollector::instance().trackCodePath(name);
    }
};

//=============================================================================
// Tracy Helper Macros (for use in implementation files)
//=============================================================================

// Usage in .cpp files:
//   BESTOW_ZONE_SCOPED;                    // Profile current scope
//   BESTOW_ZONE_NAMED("CustomName");       // Profile with custom name
//   BESTOW_FRAME_MARK;                     // Mark frame boundary
//   BESTOW_PLOT("EntityCount", count);     // Plot value over time
//   BESTOW_MESSAGE("Event occurred");      // Log message to timeline

// Helper class for zone naming in Tracy
struct ZoneHelper {
#ifdef TRACY_ENABLE
    static void markFrame() {
        FrameMark;
    }

    static void plotValue(const char* name, double value) {
        TracyPlot(name, value);
    }

    static void message(const char* text, std::size_t len) {
        TracyMessage(text, len);
    }
#else
    static void markFrame() {}
    static void plotValue(const char*, double) {}
    static void message(const char*, std::size_t) {}
#endif
};

//=============================================================================
// Convenience Functions
//=============================================================================

// Get a formatted string of current metrics for logging
inline std::string getMetricsSummary() {
    const auto& collector = MetricsCollector::instance();
    std::ostringstream oss;

    oss << "=== Bestow Metrics Summary (Frame " << collector.frameCount() << ") ===\n";

    for (std::size_t i = 0; i < static_cast<std::size_t>(SystemId::Count); ++i) {
        const auto& timing = collector.getSystemTiming(static_cast<SystemId>(i));
        if (timing.callCount > 0) {
            oss << "  " << systemIdToString(static_cast<SystemId>(i)) << ": "
                << std::fixed << std::setprecision(3)
                << timing.avgTimeMs << "ms avg, "
                << timing.lastFrameTimeMs << "ms last, "
                << timing.callCount << " calls\n";
        }
    }

    return oss.str();
}

// Get percentage breakdown of frame time by system
inline std::vector<std::pair<SystemId, float>> getSystemTimePercentages() {
    const auto& collector = MetricsCollector::instance();
    std::vector<std::pair<SystemId, float>> result;

    double totalTime = 0.0;
    for (std::size_t i = 0; i < static_cast<std::size_t>(SystemId::Count); ++i) {
        totalTime += collector.getSystemTiming(static_cast<SystemId>(i)).lastFrameTimeMs;
    }

    if (totalTime > 0.0) {
        for (std::size_t i = 0; i < static_cast<std::size_t>(SystemId::Count); ++i) {
            const auto& timing = collector.getSystemTiming(static_cast<SystemId>(i));
            if (timing.lastFrameTimeMs > 0.0) {
                float percentage = static_cast<float>(timing.lastFrameTimeMs / totalTime * 100.0);
                result.emplace_back(static_cast<SystemId>(i), percentage);
            }
        }
    }

    // Sort by percentage descending
    std::ranges::sort(result, [](const auto& a, const auto& b) {
        return a.second > b.second;
    });

    return result;
}

}  // namespace bestow::metrics

//=============================================================================
// Macro Definitions (must be used in global module fragment or after import)
//=============================================================================

// These macros provide zero-overhead when Tracy is disabled
// They must be defined after the module declaration for use in implementation files

/*
 * Usage in implementation files (.cpp):
 *
 * #ifdef TRACY_ENABLE
 * #include <tracy/Tracy.hpp>
 * #define BESTOW_ZONE_SCOPED ZoneScoped
 * #define BESTOW_ZONE_NAMED(name) ZoneScopedN(name)
 * #define BESTOW_FRAME_MARK FrameMark
 * #define BESTOW_PLOT(name, value) TracyPlot(name, value)
 * #define BESTOW_MESSAGE(text) TracyMessage(text, strlen(text))
 * #else
 * #define BESTOW_ZONE_SCOPED
 * #define BESTOW_ZONE_NAMED(name)
 * #define BESTOW_FRAME_MARK
 * #define BESTOW_PLOT(name, value)
 * #define BESTOW_MESSAGE(text)
 * #endif
 */
