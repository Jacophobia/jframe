// bestow-luabind/src/docs/metrics_binding_doc.cpp
// API documentation for bestow.metrics

module bestow.luabind;

import std;

namespace bestow {

void registerMetricsDoc(DocRegistry& registry) {
    SystemDoc sys;
    sys.name = "metrics";
    sys.qualifiedName = "bestow.metrics";
    sys.description = "Profiling and observability system. Provides per-system timing, zone profiling, code path tracking, Tracy integration, and metrics export for performance analysis.";

    //=========================================================================
    // Types
    //=========================================================================

    sys.types.push_back(TypeDoc{
        .name = "SystemTimingData",
        .qualifiedName = "SystemTimingData",
        .description = "Timing statistics for a single engine system, returned by getSystemTiming().",
        .fields = {
            {.name = "callCount", .type = "number", .description = "Total number of update calls recorded"},
            {.name = "totalTimeMs", .type = "number", .description = "Cumulative time in milliseconds"},
            {.name = "lastFrameTimeMs", .type = "number", .description = "Time spent in the last frame (ms)"},
            {.name = "minTimeMs", .type = "number", .description = "Minimum recorded frame time (ms)"},
            {.name = "maxTimeMs", .type = "number", .description = "Maximum recorded frame time (ms)"},
            {.name = "avgTimeMs", .type = "number", .description = "Running average frame time (ms)"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "FrameMetrics",
        .qualifiedName = "FrameMetrics",
        .description = "Complete metrics for the current frame, including per-system breakdowns.",
        .fields = {
            {.name = "frameNumber", .type = "number", .description = "Current frame number"},
            {.name = "totalFrameTimeMs", .type = "number", .description = "Total frame time in milliseconds"},
            {.name = "systems", .type = "table", .description = "Table mapping system name strings to their frame time in milliseconds"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "ScopedZone",
        .qualifiedName = "ScopedZone",
        .description = "A scoped profiling zone object returned by scopedZone(). Automatically ends timing when garbage collected, or can be ended early with finish().",
        .fields = {
            {.name = "_name", .type = "string", .description = "Internal zone name", .readOnly = true},
        },
        .methods = {
            {.name = "finish", .qualifiedName = "ScopedZone:finish", .description = "Manually end the zone timing. If not called, the zone ends when the object is garbage collected."},
        },
        .example = "local zone = bestow.metrics.scopedZone(\"Physics\")\n-- do physics work\nzone:finish()",
    });

    //=========================================================================
    // Frame Management
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "beginFrame",
        .qualifiedName = "bestow.metrics.beginFrame",
        .description = "Mark the beginning of a new frame for metrics collection. Call at the start of your game loop.",
        .seeAlso = {"bestow.metrics.endFrame", "bestow.metrics.frameCount"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "endFrame",
        .qualifiedName = "bestow.metrics.endFrame",
        .description = "Mark the end of the current frame. Finalizes frame timing and updates code path statistics.",
        .seeAlso = {"bestow.metrics.beginFrame"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "frameCount",
        .qualifiedName = "bestow.metrics.frameCount",
        .description = "Get the total number of frames recorded since startup or last reset.",
        .returns = {{.type = "number", .description = "Total frame count"}},
    });

    //=========================================================================
    // Zone Timing
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "beginZone",
        .qualifiedName = "bestow.metrics.beginZone",
        .description = "Begin timing a named zone. Call endZone() with the same name to stop timing and record the duration.",
        .params = {
            {.name = "name", .type = "string", .description = "Unique name for the zone (e.g., \"AI Update\", \"Render\")"},
        },
        .example = "bestow.metrics.beginZone(\"AI Update\")\n-- do AI work\nbestow.metrics.endZone(\"AI Update\")",
        .seeAlso = {"bestow.metrics.endZone", "bestow.metrics.getZoneTime", "bestow.metrics.scopedZone"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "endZone",
        .qualifiedName = "bestow.metrics.endZone",
        .description = "End timing a named zone and record its duration. The zone must have been started with beginZone().",
        .params = {
            {.name = "name", .type = "string", .description = "Name of the zone to end (must match a prior beginZone call)"},
        },
        .seeAlso = {"bestow.metrics.beginZone"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getZoneTime",
        .qualifiedName = "bestow.metrics.getZoneTime",
        .description = "Get the last recorded duration for a named zone in milliseconds.",
        .params = {
            {.name = "name", .type = "string", .description = "Zone name"},
        },
        .returns = {{.type = "number", .description = "Duration in milliseconds (0.0 if the zone was never recorded)"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "scopedZone",
        .qualifiedName = "bestow.metrics.scopedZone",
        .description = "Create a scoped zone that automatically records timing. The zone ends when finish() is called or when the returned object is garbage collected.",
        .params = {
            {.name = "name", .type = "string", .description = "Unique name for the zone"},
        },
        .returns = {{.type = "ScopedZone", .description = "A zone object with a finish() method"}},
        .example = "local zone = bestow.metrics.scopedZone(\"LevelLoad\")\n-- do expensive work\nzone:finish()  -- or let it be garbage collected",
        .seeAlso = {"bestow.metrics.beginZone", "bestow.metrics.endZone"},
    });

    //=========================================================================
    // Code Path Tracking
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "trackCodePath",
        .qualifiedName = "bestow.metrics.trackCodePath",
        .description = "Increment the call counter for a named code path. Useful for tracking how often specific branches or functions are executed.",
        .params = {
            {.name = "name", .type = "string", .description = "Code path identifier (e.g., \"collision.narrow_phase\", \"ai.pathfind\")"},
        },
        .example = "function onCollision(a, b)\n    bestow.metrics.trackCodePath(\"collision.handler\")\n    -- handle collision\nend",
    });

    //=========================================================================
    // Tracy Plots
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "plot",
        .qualifiedName = "bestow.metrics.plot",
        .description = "Plot a named value for visualization in Tracy profiler. Values are graphed over time.",
        .params = {
            {.name = "name", .type = "string", .description = "Plot name (e.g., \"EntityCount\", \"MemoryMB\")"},
            {.name = "value", .type = "number", .description = "Value to plot"},
        },
        .example = "bestow.metrics.plot(\"EntityCount\", entityCount)\nbestow.metrics.plot(\"FPS\", 1.0 / dt)",
    });

    sys.methods.push_back(MethodDoc{
        .name = "message",
        .qualifiedName = "bestow.metrics.message",
        .description = "Send a text message to the Tracy profiler timeline. Useful for marking events or logging debug information.",
        .params = {
            {.name = "text", .type = "string", .description = "Message text to display in Tracy"},
        },
        .example = "bestow.metrics.message(\"Level loaded: dungeon_01\")",
    });

    //=========================================================================
    // System Timing
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "beginSystem",
        .qualifiedName = "bestow.metrics.beginSystem",
        .description = "Begin timing a named engine system. Valid system names: \"core\", \"events\", \"entity\", \"graphics\", \"audio\", \"input\", \"physics\", \"assets\", \"save\", \"level\", \"ai\", \"camera\", \"config\", \"gas\", \"blueprints\", \"devtools\", \"application\".",
        .params = {
            {.name = "name", .type = "string", .description = "System name (lowercase)"},
        },
        .seeAlso = {"bestow.metrics.endSystem", "bestow.metrics.getSystemTiming"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "endSystem",
        .qualifiedName = "bestow.metrics.endSystem",
        .description = "End timing a named engine system and record its duration.",
        .params = {
            {.name = "name", .type = "string", .description = "System name (must match a prior beginSystem call)"},
        },
        .seeAlso = {"bestow.metrics.beginSystem"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getSystemTiming",
        .qualifiedName = "bestow.metrics.getSystemTiming",
        .description = "Get timing statistics for a named engine system.",
        .params = {
            {.name = "name", .type = "string", .description = "System name (lowercase)"},
        },
        .returns = {{.type = "SystemTimingData|nil", .description = "Timing data table with callCount, totalTimeMs, lastFrameTimeMs, minTimeMs, maxTimeMs, avgTimeMs fields, or nil if the system name is invalid"}},
        .example = "local timing = bestow.metrics.getSystemTiming(\"physics\")\nif timing then\n    print(\"Physics avg:\", timing.avgTimeMs, \"ms\")\nend",
    });

    //=========================================================================
    // Metrics Export
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "getSummary",
        .qualifiedName = "bestow.metrics.getSummary",
        .description = "Get a human-readable summary of all system metrics. Useful for logging or debug overlays.",
        .returns = {{.type = "string", .description = "Formatted text summary of all system timings"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "exportJson",
        .qualifiedName = "bestow.metrics.exportJson",
        .description = "Export all metrics as a JSON string. Useful for dashboards, external analysis tools, or saving profiling snapshots.",
        .returns = {{.type = "string", .description = "JSON string containing all system timings and code path data"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "reset",
        .qualifiedName = "bestow.metrics.reset",
        .description = "Reset all metrics counters and timing data to zero.",
    });

    //=========================================================================
    // Frame Metrics
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "getFrameMetrics",
        .qualifiedName = "bestow.metrics.getFrameMetrics",
        .description = "Get complete metrics for the current frame, including per-system time breakdowns.",
        .returns = {{.type = "FrameMetrics", .description = "Table with frameNumber, totalFrameTimeMs, and systems (a table mapping system names to their frame time in ms)"}},
        .example = "local fm = bestow.metrics.getFrameMetrics()\nprint(\"Frame\", fm.frameNumber, \"total:\", fm.totalFrameTimeMs, \"ms\")\nfor sys, ms in pairs(fm.systems) do\n    print(\"  \", sys, ms, \"ms\")\nend",
    });

    //=========================================================================
    // Tracy Status
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "isTracyEnabled",
        .qualifiedName = "bestow.metrics.isTracyEnabled",
        .description = "Check if Tracy profiler integration is enabled at compile time.",
        .returns = {{.type = "boolean", .description = "true if Tracy is enabled in the current build"}},
    });

    registry.addSystem(std::move(sys));
}

} // namespace bestow
