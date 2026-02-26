# Event System

> **Visibility:** Public (Lua + C++ API)
> **Tier:** 1
> **Dependencies:** Types
> **Lua Paths:** `bestow.events` (single-level, no Core/System split)

## Purpose

The Event System provides a decoupled publish-subscribe event bus for inter-system communication. It supports both immediate dispatch and deferred (queued) event processing, priority-based subscription ordering, and a typed variant payload that eliminates `std::any`. Because events are simple enough to expose in a single contract, there is no Core/System split -- all methods live under `bestow.events` in Lua and `IEventCore` in C++.

## Single-Level API: `IEventCore`

The complete event bus API. Supports immediate publish, deferred queue, prioritized subscriptions, and introspection.

### Immediate Dispatch

| Method | Returns | Description |
|--------|---------|-------------|
| `publish(std::string_view type, const EventData& data)` | `void` | Immediately dispatch an event to all subscribers of the given type |

### Deferred Dispatch

| Method | Returns | Description |
|--------|---------|-------------|
| `queue(std::string_view type, const EventData& data)` | `void` | Enqueue an event for deferred dispatch on the next `processQueue()` call |
| `processQueue()` | `void` | Dispatch all queued events in FIFO order, clearing the queue afterward |
| `clearQueue()` | `void` | Discard all queued events without dispatching them |
| `queueSize()` | `std::size_t` | Return the number of events currently waiting in the deferred queue |

### Subscriptions

| Method | Returns | Description |
|--------|---------|-------------|
| `subscribe(std::string_view type, std::function<void(const EventData&)> callback)` | `SubscriptionId` | Register a callback for events of the given type; returns a handle for unsubscription |
| `subscribePriority(std::string_view type, int priority, std::function<void(const EventData&)> callback)` | `SubscriptionId` | Register a prioritized callback; higher priority values are invoked first |
| `unsubscribe(SubscriptionId id)` | `void` | Remove a previously registered subscription by its handle |
| `unsubscribeAll(std::string_view type)` | `void` | Remove all subscriptions for a given event type |

### Queries

| Method | Returns | Description |
|--------|---------|-------------|
| `subscriberCount(std::string_view type)` | `std::size_t` | Return the number of active subscribers for a given event type |
| `getRegisteredTypes()` | `std::vector<std::string>` | List all event type strings that currently have at least one subscriber |

## Types

### EventData

The typed variant payload for all events. Replaces `std::any` with a closed set of concrete event structs.

```cpp
using EventData = std::variant<
    CollisionEvent,
    TriggerEvent,
    CollisionEvent3D,
    TriggerEvent3D,
    ActionEventData,
    PhaseEventData,
    AssetChangedEvent,
    SceneChangedEvent,
    EntityCreatedEvent,
    EntityDestroyedEvent,
    CustomEvent
>;
```

| Variant | Category | Description |
|---------|----------|-------------|
| `CollisionEvent` | Physics 2D | Two 2D bodies made contact |
| `TriggerEvent` | Physics 2D | An entity entered or exited a 2D sensor |
| `CollisionEvent3D` | Physics 3D | Two 3D bodies made contact |
| `TriggerEvent3D` | Physics 3D | An entity entered or exited a 3D sensor |
| `ActionEventData` | Input | An input action was triggered |
| `PhaseEventData` | Input | The input phase stack changed |
| `AssetChangedEvent` | Assets | An asset was loaded, unloaded, or hot-reloaded |
| `SceneChangedEvent` | Scene | The active scene changed |
| `EntityCreatedEvent` | Entity | An entity was created |
| `EntityDestroyedEvent` | Entity | An entity was destroyed |
| `CustomEvent` | User | A user-defined event originating from Lua or game code |

### CollisionEvent

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `entityA` | `Entity` | `NullEntity` | The first entity involved in the collision |
| `entityB` | `Entity` | `NullEntity` | The second entity involved in the collision |
| `contactPoint` | `Vec2` | `{0, 0}` | World-space position of the contact point |
| `normal` | `Vec2` | `{0, 0}` | Collision normal pointing from A to B |
| `impulse` | `float` | `0.0f` | Impulse magnitude applied to resolve the collision |

### TriggerEvent

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `entityA` | `Entity` | `NullEntity` | The entity that entered or exited the trigger |
| `entityB` | `Entity` | `NullEntity` | The sensor entity that was triggered |
| `contactPoint` | `Vec2` | `{0, 0}` | World-space position where the overlap was detected |

### CollisionEvent3D

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `entityA` | `Entity` | `NullEntity` | The first entity involved in the 3D collision |
| `entityB` | `Entity` | `NullEntity` | The second entity involved in the 3D collision |
| `contactPoint` | `Vec3` | `{0, 0, 0}` | World-space position of the contact point |
| `contactNormal` | `Vec3` | `{0, 0, 0}` | Collision normal pointing from A to B |
| `impulse` | `float` | `0.0f` | Impulse magnitude applied to resolve the collision |
| `penetrationDepth` | `float` | `0.0f` | Depth of penetration between the two bodies |

### TriggerEvent3D

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `entityA` | `Entity` | `NullEntity` | The entity that entered or exited the 3D trigger |
| `entityB` | `Entity` | `NullEntity` | The 3D sensor entity that was triggered |

### ActionEventData

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `action` | `std::string` | `""` | The action name that was triggered (e.g., `"Jump"`, `"Attack"`) |
| `phase` | `std::string` | `""` | The input phase that was active when the action fired |
| `source` | `InputSource` | `Keyboard` | The device type that triggered the action |
| `playerIndex` | `int` | `0` | Local multiplayer player index (0-3) |
| `duration` | `float` | `0.0f` | Duration the input was held before this event (for held/released events) |
| `axis` | `Vec2` | `{0, 0}` | Axis values for analog inputs; `(value, 0)` for single-axis, `(x, y)` for sticks |

### PhaseEventData

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `oldPhase` | `std::string` | `""` | The phase that was active before the change |
| `newPhase` | `std::string` | `""` | The phase that is now active |
| `phaseStack` | `std::vector<std::string>` | `{}` | The complete phase stack after the change |

### AssetChangedEvent

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `handle` | `AssetHandle` | `invalid` | Handle of the asset that changed |
| `type` | `AssetType` | `Data` | The type of asset that changed |

### SceneChangedEvent

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `from` | `std::string` | `""` | Name of the scene that was active before the change |
| `to` | `std::string` | `""` | Name of the scene that is now active |

### EntityCreatedEvent

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `entity` | `Entity` | `NullEntity` | The entity that was just created |

### EntityDestroyedEvent

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `entity` | `Entity` | `NullEntity` | The entity that is about to be destroyed |

### CustomEvent

User-defined event for Lua-originated or game-specific events.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `name` | `std::string` | `""` | A user-defined name for this custom event type |
| `data` | `std::unordered_map<std::string, SceneParam>` | `{}` | Arbitrary key-value data associated with the event |

### SceneParam (used in CustomEvent.data)

```cpp
using SceneParam = std::variant<float, int, bool, std::string, Vec2, Vec3>;
```

## Lua Examples

```lua
-- Subscribe to collision events
local subId = bestow.events.subscribe("Collision", function(data)
    print("Collision between " .. tostring(data.entityA) .. " and " .. tostring(data.entityB))
end)

-- Subscribe with priority (higher = called first)
local highPriority = bestow.events.subscribePriority("Collision", 100, function(data)
    -- This runs before default-priority subscribers
end)

-- Publish an event immediately
bestow.events.publish("Collision", {
    entityA = player,
    entityB = enemy,
    contactPoint = { x = 100, y = 200 },
    normal = { x = 0, y = -1 },
    impulse = 5.0
})

-- Queue events for deferred processing
bestow.events.queue("SceneChanged", { from = "menu", to = "gameplay" })
bestow.events.queue("SceneChanged", { from = "gameplay", to = "pause" })
print(bestow.events.queueSize())  -- 2
bestow.events.processQueue()      -- dispatches both

-- Custom events from Lua
bestow.events.publish("Custom", {
    name = "PlayerScored",
    data = { points = 100, combo = 3 }
})

-- Introspection
local count = bestow.events.subscriberCount("Collision")
local types = bestow.events.getRegisteredTypes()

-- Unsubscribe
bestow.events.unsubscribe(subId)
bestow.events.unsubscribeAll("Collision")
```

## C++ Examples

```cpp
// Subscribe to collision events
auto subId = events->subscribe("Collision", [this](const EventData& data) {
    auto& collision = std::get<CollisionEvent>(data);
    handleCollision(collision.entityA, collision.entityB, collision.impulse);
});

// Priority subscription (higher = called first)
auto highPri = events->subscribePriority("Collision", 100, [](const EventData& data) {
    // This callback runs before default-priority subscribers
});

// Immediate publish
events->publish("EntityCreated", EntityCreatedEvent{player});

// Deferred queue (processed later in EarlyUpdate phase)
events->queue("SceneChanged", SceneChangedEvent{"menu", "gameplay"});

// Process all queued events
events->processQueue();

// Custom events
events->publish("Custom", CustomEvent{
    .name = "PlayerScored",
    .data = {{"points", 100}, {"combo", 3}}
});

// Introspection
std::size_t count = events->subscriberCount("Collision");
auto types = events->getRegisteredTypes();

// Cleanup
events->unsubscribe(subId);
```
