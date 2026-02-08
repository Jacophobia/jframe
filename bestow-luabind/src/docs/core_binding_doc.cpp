// bestow-luabind/src/docs/core_binding_doc.cpp
// API documentation for bestow.core, bestow.util, bestow.profiler, and top-level logging

module bestow.luabind;

import std;

namespace bestow {

void registerCoreDoc(DocRegistry& registry) {
    // --- bestow.core ---
    SystemDoc core;
    core.name = "core";
    core.qualifiedName = "bestow.core";
    core.description = "Core engine utilities: frame timing, version info, and application lifecycle.";

    core.methods.push_back(MethodDoc{
        .name = "deltaTime",
        .qualifiedName = "bestow.core.deltaTime",
        .description = "Get the time elapsed since the last frame in seconds. Use this to make movement and animations frame-rate independent.",
        .returns = {{.type = "number", .description = "Delta time in seconds"}},
        .example = "local dt = bestow.core.deltaTime()\nlocal speed = 200\nplayer.x = player.x + speed * dt",
    });

    core.methods.push_back(MethodDoc{
        .name = "frameCount",
        .qualifiedName = "bestow.core.frameCount",
        .description = "Get the total number of frames elapsed since the engine started.",
        .returns = {{.type = "number", .description = "Total frame count"}},
    });

    core.properties.push_back(PropertyDoc{
        .name = "version",
        .type = "string",
        .description = "Engine version string (e.g., \"1.0.0\").",
        .readOnly = true,
    });

    core.properties.push_back(PropertyDoc{
        .name = "engine",
        .type = "string",
        .description = "Engine name (\"Bestow\").",
        .readOnly = true,
    });

    core.seeAlso = {"bestow.util", "bestow.profiler"};

    registry.addSystem(std::move(core));

    // --- bestow.util ---
    SystemDoc util;
    util.name = "util";
    util.qualifiedName = "bestow.util";
    util.description = "Safe utility functions for time and date. Replaces sandboxed os.time/os.date.";

    util.methods.push_back(MethodDoc{
        .name = "time",
        .qualifiedName = "bestow.util.time",
        .description = "Get the current Unix timestamp in seconds (like os.time()).",
        .returns = {{.type = "number", .description = "Unix timestamp in seconds"}},
    });

    util.methods.push_back(MethodDoc{
        .name = "clock",
        .qualifiedName = "bestow.util.clock",
        .description = "Get a high-precision time value for benchmarking and elapsed-time measurement.",
        .returns = {{.type = "number", .description = "Time in seconds (high precision)"}},
        .example = "local start = bestow.util.clock()\n-- ... expensive operation ...\nlocal elapsed = bestow.util.clock() - start\nbestow.info(\"Took\", elapsed, \"seconds\")",
    });

    util.methods.push_back(MethodDoc{
        .name = "date",
        .qualifiedName = "bestow.util.date",
        .description = "Get a formatted date string. Uses strftime format specifiers.",
        .params = {
            {.name = "format", .type = "string", .description = "strftime format string", .optional = true, .defaultVal = "\"%Y-%m-%d %H:%M:%S\""},
        },
        .returns = {{.type = "string", .description = "Formatted date string"}},
        .example = "local now = bestow.util.date()              -- \"2024-01-15 14:30:00\"\nlocal day = bestow.util.date(\"%A\")          -- \"Monday\"",
    });

    util.seeAlso = {"bestow.core"};

    registry.addSystem(std::move(util));

    // --- bestow.profiler ---
    SystemDoc profiler;
    profiler.name = "profiler";
    profiler.qualifiedName = "bestow.profiler";
    profiler.description = "Tracy profiler integration. Functions are no-ops when Tracy is not enabled.";

    profiler.methods.push_back(MethodDoc{
        .name = "frameMark",
        .qualifiedName = "bestow.profiler.frameMark",
        .description = "Mark the end of a frame for Tracy. Call once per frame in your main loop.",
    });

    profiler.methods.push_back(MethodDoc{
        .name = "plot",
        .qualifiedName = "bestow.profiler.plot",
        .description = "Send a named numeric value to Tracy for graphing over time.",
        .params = {
            {.name = "name", .type = "string", .description = "Plot series name"},
            {.name = "value", .type = "number", .description = "Numeric value to plot"},
        },
        .example = "bestow.profiler.plot(\"Entity Count\", bestow.entity.count())",
    });

    profiler.methods.push_back(MethodDoc{
        .name = "message",
        .qualifiedName = "bestow.profiler.message",
        .description = "Send a text message to the Tracy profiler log.",
        .params = {
            {.name = "text", .type = "string", .description = "Message text"},
        },
    });

    profiler.methods.push_back(MethodDoc{
        .name = "isEnabled",
        .qualifiedName = "bestow.profiler.isEnabled",
        .description = "Check if Tracy profiling is enabled in this build.",
        .returns = {{.type = "boolean", .description = "true if Tracy is enabled"}},
    });

    registry.addSystem(std::move(profiler));

    // --- bestow.log (top-level logging) ---
    SystemDoc logging;
    logging.name = "log";
    logging.qualifiedName = "bestow.log";
    logging.description = "Structured logging functions on the top-level bestow table. All accept variadic arguments concatenated with spaces. Log output includes the Lua source file and line number.";

    logging.methods.push_back(MethodDoc{
        .name = "debug",
        .qualifiedName = "bestow.debug",
        .description = "Log a debug-level message. Only visible when log level is set to debug.",
        .params = {
            {.name = "...", .type = "any", .description = "Values to log (concatenated with spaces)"},
        },
        .example = "bestow.debug(\"Player position:\", player.x, player.y)",
    });

    logging.methods.push_back(MethodDoc{
        .name = "info",
        .qualifiedName = "bestow.info",
        .description = "Log an info-level message.",
        .params = {
            {.name = "...", .type = "any", .description = "Values to log (concatenated with spaces)"},
        },
    });

    logging.methods.push_back(MethodDoc{
        .name = "warn",
        .qualifiedName = "bestow.warn",
        .description = "Log a warning-level message.",
        .params = {
            {.name = "...", .type = "any", .description = "Values to log (concatenated with spaces)"},
        },
    });

    logging.methods.push_back(MethodDoc{
        .name = "error",
        .qualifiedName = "bestow.error",
        .description = "Log an error-level message.",
        .params = {
            {.name = "...", .type = "any", .description = "Values to log (concatenated with spaces)"},
        },
    });

    registry.addSystem(std::move(logging));
}

} // namespace bestow
