// bestow-luabind/src/docs/timer_binding_doc.cpp
// API documentation for bestow.timer

module bestow.luabind;

import std;

namespace bestow {

void registerTimerDoc(DocRegistry& registry) {
    SystemDoc sys;
    sys.name = "timer";
    sys.qualifiedName = "bestow.timer";
    sys.description = "Hot-reload-safe timer system for delayed and repeating callbacks.";

    sys.methods.push_back(MethodDoc{
        .name = "after",
        .qualifiedName = "bestow.timer.after",
        .description = "Schedule a method to be called after a delay. Uses table + method name pattern for hot-reload safety — the method is looked up by name when the timer fires.",
        .params = {
            {.name = "delay", .type = "number", .description = "Time in seconds before the callback fires"},
            {.name = "table", .type = "table", .description = "The table containing the method (e.g., self)"},
            {.name = "methodName", .type = "string", .description = "Name of the method to call on the table"},
            {.name = "...", .type = "any", .description = "Extra arguments passed to the method", .optional = true},
        },
        .returns = {{.type = "TimerId", .description = "ID for cancellation, pausing, or resuming"}},
        .example = "-- Schedule clearInvulnerability to be called after 2 seconds\nlocal id = bestow.timer.after(2.0, self, \"clearInvulnerability\")\n\n-- With extra arguments\nbestow.timer.after(1.0, self, \"spawnEnemy\", \"goblin\", 3)",
        .seeAlso = {"bestow.timer.every", "bestow.timer.cancel"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "every",
        .qualifiedName = "bestow.timer.every",
        .description = "Schedule a repeating method call at a fixed interval. Uses table + method name for hot-reload safety.",
        .params = {
            {.name = "interval", .type = "number", .description = "Time in seconds between calls"},
            {.name = "table", .type = "table", .description = "The table containing the method (e.g., self)"},
            {.name = "methodName", .type = "string", .description = "Name of the method to call"},
            {.name = "...", .type = "any", .description = "Extra arguments passed to the method", .optional = true},
        },
        .returns = {{.type = "TimerId", .description = "ID for cancellation, pausing, or resuming"}},
        .example = "-- Tick every 0.5 seconds\nlocal id = bestow.timer.every(0.5, self, \"tick\")",
        .seeAlso = {"bestow.timer.after", "bestow.timer.cancel"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "cancel",
        .qualifiedName = "bestow.timer.cancel",
        .description = "Cancel a scheduled timer.",
        .params = {
            {.name = "id", .type = "TimerId", .description = "Timer ID returned by after() or every()"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "pause",
        .qualifiedName = "bestow.timer.pause",
        .description = "Pause a timer. The remaining time is preserved.",
        .params = {
            {.name = "id", .type = "TimerId", .description = "Timer ID to pause"},
        },
        .seeAlso = {"bestow.timer.resume"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "resume",
        .qualifiedName = "bestow.timer.resume",
        .description = "Resume a paused timer.",
        .params = {
            {.name = "id", .type = "TimerId", .description = "Timer ID to resume"},
        },
        .seeAlso = {"bestow.timer.pause"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "isActive",
        .qualifiedName = "bestow.timer.isActive",
        .description = "Check if a timer exists and is not paused.",
        .params = {
            {.name = "id", .type = "TimerId", .description = "Timer ID to check"},
        },
        .returns = {{.type = "boolean", .description = "true if the timer is active (exists and not paused)"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "cancelAll",
        .qualifiedName = "bestow.timer.cancelAll",
        .description = "Cancel all active timers.",
    });

    sys.methods.push_back(MethodDoc{
        .name = "cancelFor",
        .qualifiedName = "bestow.timer.cancelFor",
        .description = "Cancel all timers owned by a specific table. Call this when an entity/component is destroyed.",
        .params = {
            {.name = "owner", .type = "table", .description = "The table whose timers should be cancelled"},
        },
        .example = "-- In a destroy/cleanup callback:\nbestow.timer.cancelFor(self)",
    });

    sys.methods.push_back(MethodDoc{
        .name = "count",
        .qualifiedName = "bestow.timer.count",
        .description = "Get the number of active timers.",
        .returns = {{.type = "number", .description = "Count of active timers"}},
    });

    registry.addSystem(std::move(sys));
}

} // namespace bestow
