// bestow-luabind/src/bindings/metrics_binding.cpp
// Metrics/Profiling system Lua bindings

module;

#include <bestow/sol2_compat.hpp>
#include <spdlog/spdlog.h>

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

module bestow.luabind;

import std;
import bestow.metrics;

namespace bestow {

void bindMetricsSystem(sol::state& lua) {
    //=========================================================================
    // bestow.metrics table
    //=========================================================================

    sol::table bestow = lua["bestow"];
    sol::table metricsTable = lua.create_table();

    auto& collector = metrics::MetricsCollector::instance();

    //-------------------------------------------------------------------------
    // Frame Management
    //-------------------------------------------------------------------------

    metricsTable["beginFrame"] = [&collector]() {
        collector.beginFrame();
    };

    metricsTable["endFrame"] = [&collector]() {
        collector.endFrame();
    };

    metricsTable["frameCount"] = [&collector]() {
        return collector.frameCount();
    };

    //-------------------------------------------------------------------------
    // Zone Timing (for profiling code blocks)
    //-------------------------------------------------------------------------

    // Simple zone timing - returns a table with begin/end functions
    // Usage: local zone = bestow.metrics.zone("MyFunction")
    //        zone:begin()
    //        ... code to profile ...
    //        zone:finish()  -- or just let it go out of scope

    // Track active zones for RAII-style cleanup
    static std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> activeZones;
    static std::unordered_map<std::string, double> zoneTimes;

    metricsTable["beginZone"] = [](const std::string& name) {
        activeZones[name] = std::chrono::high_resolution_clock::now();
#ifdef TRACY_ENABLE
        // Tracy zones need to be scoped, so we use messages instead for Lua
        std::string msg = "BEGIN: " + name;
        TracyMessage(msg.c_str(), msg.size());
#endif
    };

    metricsTable["endZone"] = [&collector](const std::string& name) {
        auto it = activeZones.find(name);
        if (it != activeZones.end()) {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<double, std::milli>(end - it->second);
            zoneTimes[name] = duration.count();
            activeZones.erase(it);

#ifdef TRACY_ENABLE
            std::string msg = "END: " + name + " (" + std::to_string(duration.count()) + "ms)";
            TracyMessage(msg.c_str(), msg.size());
            TracyPlot(name.c_str(), duration.count());
#endif
        }
    };

    metricsTable["getZoneTime"] = [](const std::string& name) -> double {
        auto it = zoneTimes.find(name);
        return it != zoneTimes.end() ? it->second : 0.0;
    };

    //-------------------------------------------------------------------------
    // Code Path Tracking
    //-------------------------------------------------------------------------

    metricsTable["trackCodePath"] = [&collector](const std::string& name) {
        collector.trackCodePath(name);
    };

    //-------------------------------------------------------------------------
    // Tracy Plots (for graphing values over time)
    //-------------------------------------------------------------------------

    metricsTable["plot"] = [&collector](const std::string& name, double value) {
        collector.setTracyPlot(name.c_str(), value);
    };

    metricsTable["message"] = [&collector](const std::string& text) {
        collector.sendTracyMessage(text.c_str());
    };

    //-------------------------------------------------------------------------
    // System Timing
    //-------------------------------------------------------------------------

    // Map Lua system names to SystemId
    static const std::unordered_map<std::string, metrics::SystemId> systemNameMap = {
        {"core", metrics::SystemId::Core},
        {"events", metrics::SystemId::Events},
        {"entity", metrics::SystemId::Entity},
        {"graphics", metrics::SystemId::Graphics},
        {"audio", metrics::SystemId::Audio},
        {"input", metrics::SystemId::Input},
        {"physics", metrics::SystemId::Physics},
        {"assets", metrics::SystemId::Assets},
        {"save", metrics::SystemId::Save},
        {"level", metrics::SystemId::Level},
        {"ai", metrics::SystemId::AI},
        {"camera", metrics::SystemId::Camera},
        {"config", metrics::SystemId::Config},
        {"gas", metrics::SystemId::GAS},
        {"blueprints", metrics::SystemId::Blueprints},
        {"devtools", metrics::SystemId::DevTools},
        {"application", metrics::SystemId::Application}
    };

    metricsTable["beginSystem"] = [&collector](const std::string& name) {
        auto it = systemNameMap.find(name);
        if (it != systemNameMap.end()) {
            collector.beginSystemUpdate(it->second);
        }
    };

    metricsTable["endSystem"] = [&collector](const std::string& name) {
        auto it = systemNameMap.find(name);
        if (it != systemNameMap.end()) {
            collector.endSystemUpdate(it->second);
        }
    };

    metricsTable["getSystemTiming"] = [&collector, &lua](const std::string& name) -> sol::object {
        auto it = systemNameMap.find(name);
        if (it == systemNameMap.end()) {
            return sol::nil;
        }

        const auto& timing = collector.getSystemTiming(it->second);
        sol::table result = lua.create_table();
        result["callCount"] = timing.callCount;
        result["totalTimeMs"] = timing.totalTimeMs;
        result["lastFrameTimeMs"] = timing.lastFrameTimeMs;
        result["minTimeMs"] = timing.minTimeMs;
        result["maxTimeMs"] = timing.maxTimeMs;
        result["avgTimeMs"] = timing.avgTimeMs;
        return result;
    };

    //-------------------------------------------------------------------------
    // Metrics Export
    //-------------------------------------------------------------------------

    metricsTable["getSummary"] = []() {
        return metrics::getMetricsSummary();
    };

    metricsTable["exportJson"] = [&collector]() {
        return collector.exportToJson();
    };

    metricsTable["reset"] = [&collector]() {
        collector.reset();
    };

    //-------------------------------------------------------------------------
    // Frame Metrics
    //-------------------------------------------------------------------------

    metricsTable["getFrameMetrics"] = [&collector, &lua]() {
        auto metrics = collector.getCurrentFrameMetrics();
        sol::table result = lua.create_table();
        result["frameNumber"] = metrics.frameNumber;
        result["totalFrameTimeMs"] = metrics.totalFrameTimeMs;

        sol::table systems = lua.create_table();
        for (std::size_t i = 0; i < static_cast<std::size_t>(metrics::SystemId::Count); ++i) {
            auto sysId = static_cast<metrics::SystemId>(i);
            systems[std::string(metrics::systemIdToString(sysId))] = metrics.systemTimesMs[i];
        }
        result["systems"] = systems;

        return result;
    };

    //-------------------------------------------------------------------------
    // Tracy Status
    //-------------------------------------------------------------------------

    metricsTable["isTracyEnabled"] = []() {
        return metrics::kTracyEnabled;
    };

    //-------------------------------------------------------------------------
    // Convenience: Scoped Zone Helper
    //-------------------------------------------------------------------------

    // Returns a zone object that can be used with pcall for safe cleanup
    // Usage:
    //   local zone = bestow.metrics.scopedZone("MyFunction")
    //   -- do work
    //   zone:finish()

    sol::table zoneMetatable = lua.create_table();
    zoneMetatable["__gc"] = [](sol::table self) {
        std::string name = self["_name"];
        auto it = activeZones.find(name);
        if (it != activeZones.end()) {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<double, std::milli>(end - it->second);
            zoneTimes[name] = duration.count();
            activeZones.erase(it);
#ifdef TRACY_ENABLE
            TracyPlot(name.c_str(), duration.count());
#endif
        }
    };

    metricsTable["scopedZone"] = [&lua, zoneMetatable](const std::string& name) {
        activeZones[name] = std::chrono::high_resolution_clock::now();
#ifdef TRACY_ENABLE
        std::string msg = "ZONE: " + name;
        TracyMessage(msg.c_str(), msg.size());
#endif

        sol::table zone = lua.create_table();
        zone["_name"] = name;
        zone["finish"] = [](sol::table self) {
            std::string zoneName = self["_name"];
            auto it = activeZones.find(zoneName);
            if (it != activeZones.end()) {
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration<double, std::milli>(end - it->second);
                zoneTimes[zoneName] = duration.count();
                activeZones.erase(it);
#ifdef TRACY_ENABLE
                TracyPlot(zoneName.c_str(), duration.count());
#endif
            }
        };
        zone[sol::metatable_key] = zoneMetatable;
        return zone;
    };

    bestow["metrics"] = metricsTable;

    spdlog::debug("[LuaBind] Bound MetricsCollector -> bestow.metrics (Tracy: {})",
                  metrics::kTracyEnabled ? "enabled" : "disabled");
}

}  // namespace bestow
