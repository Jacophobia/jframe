// bestow-luabind/src/docs/events_binding_doc.cpp
// API documentation for bestow.events

module bestow.luabind;

import std;

namespace bestow {

void registerEventsDoc(DocRegistry& registry) {
    SystemDoc sys;
    sys.name = "events";
    sys.qualifiedName = "bestow.events";
    sys.description = "Hot-reload-safe event publish/subscribe system for decoupled inter-system communication.";

    sys.methods.push_back(MethodDoc{
        .name = "subscribe",
        .qualifiedName = "bestow.events.subscribe",
        .description = "Subscribe to an event type. Uses table + method name pattern for hot-reload safety. The method is looked up by name at dispatch time.",
        .params = {
            {.name = "eventType", .type = "string", .description = "Event type name (e.g., \"Hit\", \"Collision\", \"AssetLoaded\")"},
            {.name = "pattern", .type = "table", .description = "Filter pattern table (can be empty {})"},
            {.name = "table", .type = "table", .description = "The table containing the callback method (e.g., self)"},
            {.name = "methodName", .type = "string", .description = "Name of the method to call when event fires"},
        },
        .returns = {{.type = "SubscriptionId", .description = "ID for unsubscribing later"}},
        .example = "local id = bestow.events.subscribe(\"Hit\", {}, self, \"onHit\")\n\n-- The method receives (self, eventData, scope):\nonHit = function(self, event, scope)\n  print(\"Hit by: \" .. event.source)\nend,",
        .seeAlso = {"bestow.events.unsubscribe", "bestow.events.emit"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "unsubscribe",
        .qualifiedName = "bestow.events.unsubscribe",
        .description = "Unsubscribe from an event by ID.",
        .params = {
            {.name = "id", .type = "SubscriptionId", .description = "Subscription ID returned by subscribe()"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "unsubscribeAll",
        .qualifiedName = "bestow.events.unsubscribeAll",
        .description = "Unsubscribe all subscriptions owned by a table. Call when an entity/component is destroyed.",
        .params = {
            {.name = "owner", .type = "table", .description = "The table whose subscriptions should be removed"},
        },
        .example = "-- In cleanup/destroy:\nbestow.events.unsubscribeAll(self)",
    });

    sys.methods.push_back(MethodDoc{
        .name = "emit",
        .qualifiedName = "bestow.events.emit",
        .description = "Emit an event to all matching subscribers. Dispatches synchronously.",
        .params = {
            {.name = "eventType", .type = "string", .description = "Event type name"},
            {.name = "data", .type = "table", .description = "Event data table passed to all handlers"},
        },
        .example = "bestow.events.emit(\"Hit\", {\n  target = self.entity,\n  source = attacker,\n  amount = 10,\n})",
        .seeAlso = {"bestow.events.subscribe"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "count",
        .qualifiedName = "bestow.events.count",
        .description = "Get the number of active subscriptions (for debugging).",
        .returns = {{.type = "number", .description = "Count of active subscriptions"}},
    });

    registry.addSystem(std::move(sys));
}

} // namespace bestow
