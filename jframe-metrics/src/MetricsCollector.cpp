// jframe-metrics/src/MetricsCollector.cpp
// MetricsCollector implementation

module;

#include <spdlog/spdlog.h>

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

module jframe.metrics;

namespace jframe::metrics {

MetricsCollector& MetricsCollector::instance() {
    static MetricsCollector instance;
    return instance;
}

void MetricsCollector::beginSystemUpdate(SystemId system) {
    auto idx = static_cast<std::size_t>(system);
    systemStartTimes_[idx] = std::chrono::high_resolution_clock::now();

#ifdef TRACY_ENABLE
    // Tracy zone is handled by the caller using ZoneScoped macros
#endif
}

void MetricsCollector::endSystemUpdate(SystemId system) {
    auto idx = static_cast<std::size_t>(system);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double, std::milli>(
        end - systemStartTimes_[idx]);

    systemTimings_[idx].record(duration.count());

#ifdef TRACY_ENABLE
    // Send to Tracy as a plot for this system
    std::string plotName = std::string(systemIdToString(system)) + " (ms)";
    TracyPlot(plotName.c_str(), duration.count());
#endif
}

const SystemTimingData& MetricsCollector::getSystemTiming(SystemId system) const {
    return systemTimings_[static_cast<std::size_t>(system)];
}

void MetricsCollector::trackCodePath(std::string_view name) {
    std::lock_guard lock(codePathMutex_);
    codePaths_[std::string(name)].hit();
}

const std::unordered_map<std::string, CodePathStats>& MetricsCollector::getCodePaths() const {
    return codePaths_;
}

void MetricsCollector::beginFrame() {
    frameStartTime_ = std::chrono::high_resolution_clock::now();

#ifdef TRACY_ENABLE
    FrameMark;
#endif
}

void MetricsCollector::endFrame() {
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double, std::milli>(end - frameStartTime_);
    lastFrameTotalMs_ = duration.count();
    frameCount_++;

    // Update code path frame counters
    {
        std::lock_guard lock(codePathMutex_);
        for (auto& [name, stats] : codePaths_) {
            stats.endFrame();
        }
    }

#ifdef TRACY_ENABLE
    TracyPlot("Frame Time (ms)", lastFrameTotalMs_);
    TracyPlot("FPS", 1000.0 / lastFrameTotalMs_);
#endif
}

MetricsCollector::FrameMetrics MetricsCollector::getCurrentFrameMetrics() const {
    FrameMetrics metrics;
    metrics.frameNumber = frameCount_;
    metrics.totalFrameTimeMs = lastFrameTotalMs_;

    for (std::size_t i = 0; i < static_cast<std::size_t>(SystemId::Count); ++i) {
        metrics.systemTimesMs[i] = systemTimings_[i].lastFrameTimeMs;
    }

    return metrics;
}

void MetricsCollector::reset() {
    for (auto& timing : systemTimings_) {
        timing.reset();
    }

    {
        std::lock_guard lock(codePathMutex_);
        codePaths_.clear();
    }

    frameCount_ = 0;
    lastFrameTotalMs_ = 0.0;

    spdlog::info("[Metrics] All metrics reset");
}

std::string MetricsCollector::exportToJson() const {
    std::ostringstream json;
    json << "{\n";
    json << "  \"frameCount\": " << frameCount_ << ",\n";
    json << "  \"lastFrameTimeMs\": " << std::fixed << std::setprecision(4)
         << lastFrameTotalMs_ << ",\n";

    // System timings
    json << "  \"systems\": {\n";
    bool first = true;
    for (std::size_t i = 0; i < static_cast<std::size_t>(SystemId::Count); ++i) {
        const auto& timing = systemTimings_[i];
        if (timing.callCount > 0) {
            if (!first) json << ",\n";
            first = false;

            json << "    \"" << systemIdToString(static_cast<SystemId>(i)) << "\": {\n"
                 << "      \"callCount\": " << timing.callCount << ",\n"
                 << "      \"totalTimeMs\": " << timing.totalTimeMs << ",\n"
                 << "      \"avgTimeMs\": " << timing.avgTimeMs << ",\n"
                 << "      \"minTimeMs\": " << timing.minTimeMs << ",\n"
                 << "      \"maxTimeMs\": " << timing.maxTimeMs << ",\n"
                 << "      \"lastFrameTimeMs\": " << timing.lastFrameTimeMs << "\n"
                 << "    }";
        }
    }
    json << "\n  },\n";

    // Code paths
    json << "  \"codePaths\": {\n";
    first = true;
    {
        std::lock_guard lock(codePathMutex_);
        for (const auto& [name, stats] : codePaths_) {
            if (!first) json << ",\n";
            first = false;

            json << "    \"" << name << "\": {\n"
                 << "      \"callCount\": " << stats.callCount << ",\n"
                 << "      \"framesSinceLastCall\": " << stats.framesSinceLastCall << "\n"
                 << "    }";
        }
    }
    json << "\n  }\n";

    json << "}\n";
    return json.str();
}

void MetricsCollector::setTracyPlot(const char* name, double value) {
#ifdef TRACY_ENABLE
    TracyPlot(name, value);
#else
    (void)name;
    (void)value;
#endif
}

void MetricsCollector::sendTracyMessage(const char* message) {
#ifdef TRACY_ENABLE
    TracyMessage(message, strlen(message));
#else
    (void)message;
#endif
}

}  // namespace jframe::metrics
