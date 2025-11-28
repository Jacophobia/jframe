# EventSystem API

The `EventSystem` provides publish/subscribe messaging for decoupled communication.

## Overview

```cpp
auto& events = sys.events;

// Subscribe to event
auto id = events->subscribe(Events::Collision, [](const EventData& data) {
    auto& collision = std::get<CollisionEvent>(data);
    // Handle collision
});

// Publish immediate
events->publish(Events::Collision, collisionData);

// Queue deferred
events->queue(Events::ItemCollected, itemData);

// Process queue (once per frame)
events->processQueue();

// Unsubscribe
events->unsubscribe(id);
```

## Publishing Events

### publish(const EventType& type, const EventData& data)

```cpp
void publish(const EventType& type, const EventData& data);
```

Publishes an event immediately. All subscribers are notified right away.

**Example:**

```cpp
CollisionEvent collision{
    .entityA = player,
    .entityB = enemy,
    .contactPoint = {100.0f, 200.0f},
    .normal = {0.0f, -1.0f},
    .impulse = 50.0f
};

events->publish(Events::Collision, collision);
```

---

### queue(const EventType& type, const EventData& data)

```cpp
void queue(const EventType& type, const EventData& data);
```

Queues an event for deferred processing. Event is dispatched during `processQueue()`.

**Use for:** Events that should not be processed during physics simulation or iteration.

**Example:**

```cpp
// Queue events during collision handling
events->queue(Events::EntityDamaged, damageData);
events->queue(Events::ItemCollected, itemData);

// Process all queued events at end of frame
events->processQueue();
```

---

## Subscribing to Events

### subscribe(const EventType& type, EventCallback callback)

```cpp
using EventCallback = std::function<void(const EventData&)>;
SubscriptionId subscribe(const EventType& type, EventCallback callback);
```

Subscribes to an event type. Returns a subscription ID for later unsubscribing.

**Example:**

```cpp
// Subscribe to collision events
auto collisionSub = events->subscribe(Events::Collision, [](const EventData& data) {
    auto& collision = std::get<CollisionEvent>(data);

    // Handle collision
    if (entities->allOf<Player>(collision.entityA)) {
        // Player collided with something
    }
});

// Subscribe to trigger events
auto triggerSub = events->subscribe(Events::TriggerEnter, [](const EventData& data) {
    auto& trigger = std::get<TriggerEvent>(data);

    if (entities->allOf<Player>(trigger.entityA) &&
        entities->allOf<Coin>(trigger.entityB)) {
        // Player collected coin
        entities->destroyEntity(trigger.entityB);
        score += 10;
    }
});
```

---

### unsubscribe(SubscriptionId id)

```cpp
void unsubscribe(SubscriptionId id);
```

Unsubscribes from an event using the subscription ID.

**Example:**

```cpp
auto id = events->subscribe(Events::Collision, myCallback);

// Later: unsubscribe
events->unsubscribe(id);
```

---

### unsubscribeAll(const EventType& type)

```cpp
void unsubscribeAll(const EventType& type);
```

Unsubscribes all callbacks for a specific event type.

---

## Processing Queue

### processQueue()

```cpp
void processQueue();
```

Processes all queued events in order. Call once per frame.

**Example:**

```cpp
void updateFixed(DeltaTime dt) override {
    // Physics and game logic...
    physics->update(dt);

    // Process all queued events
    events->processQueue();
}
```

---

### clearQueue()

```cpp
void clearQueue();
```

Clears all queued events without processing them.

---

### queueSize()

```cpp
std::size_t queueSize() const;
```

Returns the number of events currently queued.

---

## Event Types

Predefined event type constants:

```cpp
namespace Events {
    inline constexpr const char* Collision = "collision";
    inline constexpr const char* TriggerEnter = "trigger_enter";
    inline constexpr const char* TriggerExit = "trigger_exit";
    inline constexpr const char* LevelLoaded = "level_loaded";
    inline constexpr const char* LevelUnloaded = "level_unloaded";
    inline constexpr const char* EntityDamaged = "entity_damaged";
    inline constexpr const char* EntityDied = "entity_died";
    inline constexpr const char* ItemCollected = "item_collected";
    inline constexpr const char* Checkpoint = "checkpoint";
    inline constexpr const char* PlayerDeath = "player_death";
    inline constexpr const char* GameSaved = "game_saved";
    inline constexpr const char* GameLoaded = "game_loaded";
}
```

You can also define custom event types as strings.

---

## Event Data Types

Events use `std::variant` for type-safe data:

```cpp
using EventData = std::variant<
    EntityEventData,
    DamageEventData,
    LevelEventData,
    CollisionEvent,
    TriggerEvent,
    std::any  // For custom data
>;
```

### CollisionEvent

```cpp
struct CollisionEvent {
    Entity entityA;
    Entity entityB;
    Vec2 contactPoint;
    Vec2 normal;
    float impulse;
};
```

**Published by:** PhysicsSystem on physical collisions

---

### TriggerEvent

```cpp
struct TriggerEvent {
    Entity entityA;
    Entity entityB;
    Vec2 contactPoint;
};
```

**Published by:** PhysicsSystem on sensor enter/exit

---

### EntityEventData

```cpp
struct EntityEventData {
    Entity entity;
    std::optional<Entity> otherEntity;
};
```

**Use for:** Generic entity events

---

### DamageEventData

```cpp
struct DamageEventData {
    Entity target;
    Entity source;
    int amount;
    Vec2 knockback;
};
```

**Example:**

```cpp
events->publish(Events::EntityDamaged, DamageEventData{
    .target = enemy,
    .source = player,
    .amount = 25,
    .knockback = {100.0f, -50.0f}
});
```

---

### LevelEventData

```cpp
struct LevelEventData {
    LevelId levelId;
    LevelEvent event;
};
```

**Published by:** LevelSystem on level state changes

---

## Common Patterns

### Collision Damage System

```cpp
void setupCollisionDamage() {
    events->subscribe(Events::Collision, [](const EventData& data) {
        auto& collision = std::get<CollisionEvent>(data);

        // Player hitting enemy
        if (entities->allOf<Player>(collision.entityA) &&
            entities->allOf<Enemy>(collision.entityB)) {

            // Damage player
            events->queue(Events::EntityDamaged, DamageEventData{
                .target = collision.entityA,
                .source = collision.entityB,
                .amount = 10,
                .knockback = {100.0f, -200.0f}
            });
        }

        // Projectile hitting enemy
        if (entities->allOf<Projectile>(collision.entityA) &&
            entities->allOf<Enemy>(collision.entityB)) {

            // Damage enemy
            events->queue(Events::EntityDamaged, DamageEventData{
                .target = collision.entityB,
                .source = collision.entityA,
                .amount = 25
            });

            // Destroy projectile
            entities->destroyEntity(collision.entityA);
        }
    });

    // Process damage events
    events->subscribe(Events::EntityDamaged, [](const EventData& data) {
        auto& damage = std::get<DamageEventData>(data);

        auto* health = entities->tryGet<Health>(damage.target);
        if (health) {
            health->current -= damage.amount;

            // Apply knockback
            if (damage.knockback.x != 0.0f || damage.knockback.y != 0.0f) {
                physics->applyImpulse(damage.target, damage.knockback);
            }

            // Check death
            if (health->current <= 0) {
                events->queue(Events::EntityDied, EntityEventData{
                    .entity = damage.target,
                    .otherEntity = damage.source
                });
            }
        }
    });
}
```

---

### Collectible System

```cpp
void setupCollectibles() {
    events->subscribe(Events::TriggerEnter, [](const EventData& data) {
        auto& trigger = std::get<TriggerEvent>(data);

        if (!entities->allOf<Player>(trigger.entityA)) return;

        // Coin
        if (auto* coin = entities->tryGet<Coin>(trigger.entityB)) {
            score += coin->value;
            entities->destroyEntity(trigger.entityB);

            events->queue(Events::ItemCollected, EntityEventData{
                .entity = trigger.entityB,
                .otherEntity = trigger.entityA
            });

            audio->playPositional({
                .asset = coinSound,
                .position = {pos.x, pos.y, 0.0f}
            });
        }

        // Power-up
        if (auto* powerup = entities->tryGet<Powerup>(trigger.entityB)) {
            applyPowerup(trigger.entityA, powerup->type);
            entities->destroyEntity(trigger.entityB);
        }
    });
}
```

---

### Achievement System

```cpp
class AchievementSystem {
    std::unordered_set<std::string> unlockedAchievements_;
    int enemiesKilled_ = 0;
    int coinsCollected_ = 0;

public:
    void initialize(IEventSystem* events) {
        events->subscribe(Events::EntityDied, [this](const EventData& data) {
            auto& death = std::get<EntityEventData>(data);

            if (entities->allOf<Enemy>(death.entity)) {
                enemiesKilled_++;

                if (enemiesKilled_ >= 100) {
                    unlock("Slayer");
                }
            }
        });

        events->subscribe(Events::ItemCollected, [this](const EventData& data) {
            auto& item = std::get<EntityEventData>(data);

            if (entities->allOf<Coin>(item.entity)) {
                coinsCollected_++;

                if (coinsCollected_ >= 1000) {
                    unlock("Rich");
                }
            }
        });

        events->subscribe(Events::LevelLoaded, [this](const EventData& data) {
            auto& level = std::get<LevelEventData>(data);

            if (level.levelId == finalLevelId) {
                unlock("The End");
            }
        });
    }

    void unlock(const std::string& name) {
        if (!unlockedAchievements_.contains(name)) {
            unlockedAchievements_.insert(name);
            showAchievementNotification(name);
        }
    }
};
```

---

### Custom Events

```cpp
// Define custom event type
constexpr const char* BossDefeated = "boss_defeated";

struct BossDefeatedData {
    Entity boss;
    float timeToDefeat;
    int damageDealt;
};

// Publish custom event
events->publish(BossDefeated, BossDefeatedData{
    .boss = bossEntity,
    .timeToDefeat = 123.5f,
    .damageDealt = 5000
});

// Subscribe to custom event
events->subscribe(BossDefeated, [](const EventData& data) {
    auto& boss = std::get<BossDefeatedData>(data);

    // Award bonus points for fast defeat
    if (boss.timeToDefeat < 60.0f) {
        score += 1000;
    }
});
```

---

### Event Logging

```cpp
void setupEventLogging(IEventSystem* events) {
    auto logEvent = [](const std::string& type) {
        events->subscribe(type.c_str(), [type](const EventData& data) {
            logInfo("Event: " + type);
        });
    };

    logEvent(Events::Collision);
    logEvent(Events::EntityDamaged);
    logEvent(Events::EntityDied);
    logEvent(Events::ItemCollected);
}
```

---

### Event Replay System

```cpp
class EventRecorder {
    struct RecordedEvent {
        float timestamp;
        EventType type;
        EventData data;
    };

    std::vector<RecordedEvent> events_;
    float currentTime_ = 0.0f;
    bool recording_ = false;

public:
    void startRecording(IEventSystem* events) {
        recording_ = true;
        events_.clear();
        currentTime_ = 0.0f;

        // Subscribe to all events
        events->subscribe(Events::Collision, [this](const EventData& data) {
            if (recording_) {
                events_.push_back({currentTime_, Events::Collision, data});
            }
        });
        // ... subscribe to other events
    }

    void update(DeltaTime dt) {
        if (recording_) {
            currentTime_ += dt;
        }
    }

    void replay(IEventSystem* events) {
        float replayTime = 0.0f;
        for (const auto& e : events_) {
            // Wait until event timestamp
            while (replayTime < e.timestamp) {
                replayTime += dt;
                // Update game...
            }

            events->publish(e.type, e.data);
        }
    }
};
```

---

## Performance Tips

1. **Use `queue()` during iteration** - Avoid modifying entity lists during iteration
2. **Unsubscribe when done** - Clean up subscriptions to prevent leaks
3. **Limit subscribers** - Too many subscribers can slow down event dispatch
4. **Batch events** - Queue related events and process together
5. **Use `processQueue()` strategically** - Call at safe points in update loop

## See Also

- [PhysicsSystem](PhysicsSystem.md) - Collision and trigger events
- [LevelSystem](LevelSystem.md) - Level load events
- [EntitySystem](EntitySystem.md) - Entity-based event handling
- [GASSystem](GASSystem.md) - Ability and effect events
