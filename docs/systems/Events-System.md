# Bestow Events System Documentation

## Overview

The Bestow Events System provides a decoupled, event-driven communication mechanism between systems and game entities. It implements the publish-subscribe pattern, allowing systems to communicate without direct dependencies on each other.

### Key Features

- **Immediate dispatch** - Events processed synchronously when published
- **Deferred dispatch** - Events queued and processed in batches
- **Type-safe event data** - Uses `std::variant` for flexible, type-safe payloads
- **Multiple subscribers** - Many listeners can subscribe to the same event
- **Thread-safe queuing** - Queue operations protected by mutex
- **Zero dependencies** - Only depends on `bestow.types` module

### Architecture

```
┌─────────────────┐         ┌─────────────────┐
│  Publisher A    │────────>│   EventSystem   │
└─────────────────┘         │                 │
                            │  - publish()    │
┌─────────────────┐         │  - queue()      │
│  Publisher B    │────────>│  - processQueue│
└─────────────────┘         └────────┬────────┘
                                     │
                    ┌────────────────┼────────────────┐
                    │                │                │
                    v                v                v
            ┌───────────┐    ┌───────────┐    ┌───────────┐
            │Subscriber1│    │Subscriber2│    │Subscriber3│
            └───────────┘    └───────────┘    └───────────┘
```

## Module Imports

```cpp
import bestow.events;       // Interface only
import bestow.events.impl;  // Implementation + factory
import bestow.types;        // Event data types
```

## Creating an Event System

```cpp
#include <memory>
import bestow.events;
import bestow.events.impl;

// Create the event system
std::unique_ptr<IEventSystem> events = createEventSystem();
```

## Event Types

Bestow provides predefined event type constants in the `Events` namespace:

```cpp
namespace bestow::Events {
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

### Custom Event Types

You can define custom event types as string literals:

```cpp
// In your game code
constexpr const char* BOSS_DEFEATED = "boss_defeated";
constexpr const char* DIALOGUE_STARTED = "dialogue_started";
constexpr const char* ACHIEVEMENT_UNLOCKED = "achievement_unlocked";
```

## Event Data Types

Events carry data using `EventData`, which is a `std::variant` supporting multiple payload types:

```cpp
using EventData = std::variant<
    EntityEventData,
    DamageEventData,
    LevelEventData,
    CollisionEvent,
    TriggerEvent,
    std::any
>;
```

### Built-in Event Data Structures

#### EntityEventData
For basic entity-related events:
```cpp
struct EntityEventData {
    Entity entity;
    std::optional<Entity> otherEntity;
};
```

#### DamageEventData
For damage and combat events:
```cpp
struct DamageEventData {
    Entity target;
    Entity source;
    int amount;
    Vec2 knockback;
};
```

#### LevelEventData
For level loading/unloading events:
```cpp
struct LevelEventData {
    LevelId levelId;
    LevelEvent event;
};
```

#### CollisionEvent
For physics collision events:
```cpp
struct CollisionEvent {
    Entity entityA;
    Entity entityB;
    Vec2 contactPoint;
    Vec2 normal;
    float impulse;
};
```

#### TriggerEvent
For trigger zone events:
```cpp
struct TriggerEvent {
    Entity entityA;
    Entity entityB;
    Vec2 contactPoint;
};
```

## Subscribing to Events

### Basic Subscription

Subscribe to an event type with a callback function:

```cpp
import bestow.events;

SubscriptionId id = events->subscribe(Events::Collision,
    [](const EventData& data) {
        // Handle collision event
        auto& collision = std::get<CollisionEvent>(data);
        // Process collision...
    });
```

### Accessing Event Data

Use `std::get<T>()` to extract typed data from the variant:

```cpp
events->subscribe(Events::EntityDamaged, [](const EventData& data) {
    // Extract DamageEventData from variant
    const auto& damage = std::get<DamageEventData>(data);

    std::println("Entity {} took {} damage from entity {}",
        static_cast<int>(damage.target),
        damage.amount,
        static_cast<int>(damage.source));
});
```

### Safe Data Access

Use `std::get_if<T>()` for safe access without exceptions:

```cpp
events->subscribe("mixed_event", [](const EventData& data) {
    if (auto* damage = std::get_if<DamageEventData>(&data)) {
        // Handle as damage event
        std::println("Damage: {}", damage->amount);
    }
    else if (auto* entity = std::get_if<EntityEventData>(&data)) {
        // Handle as entity event
        std::println("Entity event");
    }
});
```

### Member Function Callbacks

Capture `this` to call member functions:

```cpp
class GameSystem {
public:
    void initialize(IEventSystem* events) {
        collisionSub_ = events->subscribe(Events::Collision,
            [this](const EventData& data) {
                this->onCollision(data);
            });
    }

private:
    void onCollision(const EventData& data) {
        auto& collision = std::get<CollisionEvent>(data);
        // Handle collision...
    }

    SubscriptionId collisionSub_;
};
```

### Multiple Subscriptions

Store subscription IDs for later cleanup:

```cpp
class Enemy {
public:
    void setupEventHandlers(IEventSystem* events) {
        damageId_ = events->subscribe(Events::EntityDamaged,
            [this](const EventData& data) { onDamage(data); });

        deathId_ = events->subscribe(Events::EntityDied,
            [this](const EventData& data) { onDeath(data); });
    }

    void cleanup(IEventSystem* events) {
        events->unsubscribe(damageId_);
        events->unsubscribe(deathId_);
    }

private:
    SubscriptionId damageId_;
    SubscriptionId deathId_;
};
```

## Publishing Events

### Immediate Publishing

Events are dispatched synchronously to all subscribers:

```cpp
// Create event data
CollisionEvent collision{
    .entityA = playerEntity,
    .entityB = wallEntity,
    .contactPoint = {100.0f, 200.0f},
    .normal = {0.0f, -1.0f},
    .impulse = 5.0f
};

// Publish immediately - all subscribers notified now
events->publish(Events::Collision, collision);
```

### When to Use Immediate Publishing

- **Critical gameplay events** - Player death, level completion
- **Input events** - Require immediate response
- **State changes** - Need synchronous notification
- **Single-threaded context** - No concurrency concerns

### Queued Publishing

Events are stored and processed later in a batch:

```cpp
// Queue event for later processing
EntityEventData entityEvent{
    .entity = collectibleEntity,
    .otherEntity = playerEntity
};

events->queue(Events::ItemCollected, entityEvent);

// ... later in main loop ...
events->processQueue();  // All queued events dispatched now
```

### When to Use Queued Publishing

- **Non-critical events** - UI updates, statistics
- **Batch processing** - Many events at once
- **Thread safety** - Publishing from worker threads
- **Frame-based dispatch** - Process once per frame

## Processing Queued Events

### Main Loop Integration

```cpp
void gameLoop(IEventSystem* events, DeltaTime dt) {
    // 1. Process input
    inputSystem->update(dt);

    // 2. Update game logic
    gameLogic->update(dt);

    // 3. Process all queued events
    events->processQueue();  // Dispatches all queued events

    // 4. Render
    renderer->render();
}
```

### Queue Management

```cpp
// Check queue size
std::size_t pending = events->queueSize();
std::println("Pending events: {}", pending);

// Clear queue without processing
events->clearQueue();  // Discards all queued events
```

## Unsubscribing from Events

### Unsubscribe by ID

```cpp
// Save subscription ID when subscribing
SubscriptionId id = events->subscribe(Events::Collision, callback);

// Unsubscribe later
events->unsubscribe(id);  // This callback no longer receives events
```

### Unsubscribe All

Remove all subscribers from an event type:

```cpp
// Remove all collision event subscribers
events->unsubscribeAll(Events::Collision);
```

### RAII Wrapper Pattern

Manage subscription lifetime automatically:

```cpp
class ScopedSubscription {
public:
    ScopedSubscription(IEventSystem* events,
                      const EventType& type,
                      EventCallback callback)
        : events_(events)
        , id_(events->subscribe(type, std::move(callback))) {}

    ~ScopedSubscription() {
        events_->unsubscribe(id_);
    }

    // Non-copyable
    ScopedSubscription(const ScopedSubscription&) = delete;
    ScopedSubscription& operator=(const ScopedSubscription&) = delete;

private:
    IEventSystem* events_;
    SubscriptionId id_;
};

// Usage
void setupHandlers(IEventSystem* events) {
    damageSubscription_ = std::make_unique<ScopedSubscription>(
        events, Events::EntityDamaged,
        [this](const EventData& data) { onDamage(data); }
    );
}
```

## Common Patterns

### Pattern 1: System Communication

Different systems communicate without direct coupling:

```cpp
// Physics system publishes collision
void PhysicsSystem::detectCollisions() {
    for (auto& collision : detectedCollisions) {
        CollisionEvent event{
            .entityA = collision.first,
            .entityB = collision.second,
            .contactPoint = collision.point,
            .normal = collision.normal,
            .impulse = collision.impulse
        };
        events_->publish(Events::Collision, event);
    }
}

// Audio system reacts to collision
void AudioSystem::initialize(IEventSystem* events) {
    events->subscribe(Events::Collision, [this](const EventData& data) {
        auto& collision = std::get<CollisionEvent>(data);
        playSoundEffect("impact.wav", collision.impulse);
    });
}
```

### Pattern 2: Entity Health System

Track damage and death events:

```cpp
class HealthSystem {
public:
    void initialize(IEventSystem* events) {
        events_ = events;
    }

    void applyDamage(Entity target, Entity source, int amount) {
        Health* health = registry_.try_get<Health>(target);
        if (!health) return;

        health->current -= amount;

        // Publish damage event
        DamageEventData damage{
            .target = target,
            .source = source,
            .amount = amount,
            .knockback = calculateKnockback(target, source)
        };
        events_->publish(Events::EntityDamaged, damage);

        // Check for death
        if (health->current <= 0) {
            EntityEventData death{
                .entity = target,
                .otherEntity = source
            };
            events_->publish(Events::EntityDied, death);
        }
    }

private:
    IEventSystem* events_;
    entt::registry registry_;
};
```

### Pattern 3: Level Loading

Coordinate multiple systems during level transitions:

```cpp
class LevelSystem {
public:
    void loadLevel(LevelId levelId) {
        state_ = LevelState::Loading;

        // Notify systems level is loading
        LevelEventData loadStart{
            .levelId = levelId,
            .event = LevelEvent::LoadStarted
        };
        events_->publish(Events::LevelLoaded, loadStart);

        // Load level data...
        loadLevelData(levelId);

        state_ = LevelState::Loaded;

        // Notify systems level is ready
        LevelEventData loadComplete{
            .levelId = levelId,
            .event = LevelEvent::LoadCompleted
        };
        events_->publish(Events::LevelLoaded, loadComplete);
    }

private:
    IEventSystem* events_;
    LevelState state_;
};

// Graphics system responds to level load
void GraphicsSystem::initialize(IEventSystem* events) {
    events->subscribe(Events::LevelLoaded, [this](const EventData& data) {
        auto& levelData = std::get<LevelEventData>(data);
        if (levelData.event == LevelEvent::LoadCompleted) {
            loadLevelTextures(levelData.levelId);
        }
    });
}
```

### Pattern 4: Achievement System

Monitor multiple event types for achievements:

```cpp
class AchievementSystem {
public:
    void initialize(IEventSystem* events) {
        // Track enemy defeats
        events->subscribe(Events::EntityDied, [this](const EventData& data) {
            auto& death = std::get<EntityEventData>(data);
            if (isEnemy(death.entity)) {
                enemyKillCount_++;
                checkAchievements();
            }
        });

        // Track collectibles
        events->subscribe(Events::ItemCollected, [this](const EventData& data) {
            collectibleCount_++;
            checkAchievements();
        });

        // Track checkpoints
        events->subscribe(Events::Checkpoint, [this](const EventData& data) {
            checkpointCount_++;
            checkAchievements();
        });
    }

private:
    void checkAchievements() {
        if (enemyKillCount_ >= 100) {
            unlockAchievement("Warrior");
        }
        if (collectibleCount_ == maxCollectibles_) {
            unlockAchievement("Collector");
        }
    }

    int enemyKillCount_ = 0;
    int collectibleCount_ = 0;
    int checkpointCount_ = 0;
};
```

### Pattern 5: Custom Event Data

Use `std::any` for game-specific events:

```cpp
struct BossPhaseData {
    int phase;
    std::string phaseName;
    float healthPercent;
};

// Publish custom event
BossPhaseData phaseData{
    .phase = 2,
    .phaseName = "Enraged",
    .healthPercent = 0.5f
};
events->publish("boss_phase_change", phaseData);

// Subscribe to custom event
events->subscribe("boss_phase_change", [](const EventData& data) {
    auto anyData = std::get<std::any>(data);
    auto phaseData = std::any_cast<BossPhaseData>(anyData);

    std::println("Boss entered phase {}: {}",
        phaseData.phase,
        phaseData.phaseName);
});
```

## Thread Safety

### Queue Operations

The event queue is thread-safe and can be called from any thread:

```cpp
// Worker thread can safely queue events
void workerThread(IEventSystem* events) {
    // Thread-safe operation
    events->queue(Events::GameSaved, EntityEventData{});
}

// Main thread processes queue
void mainLoop(IEventSystem* events) {
    events->processQueue();  // Dispatches events from all threads
}
```

### Publishing from Threads

**Warning**: Immediate publishing is NOT thread-safe:

```cpp
// DON'T DO THIS from a worker thread
void badWorkerThread(IEventSystem* events) {
    events->publish(Events::Collision, collision);  // UNSAFE!
}

// DO THIS instead - use queue
void goodWorkerThread(IEventSystem* events) {
    events->queue(Events::Collision, collision);  // Safe
}
```

## Performance Considerations

### Immediate vs Queued Dispatch

```cpp
// Immediate: ~10-50ns per subscriber (depending on callback complexity)
events->publish(Events::Collision, collision);

// Queued: ~20ns to queue + batch processing cost
events->queue(Events::Collision, collision);
```

### Optimizing Subscriptions

```cpp
// SLOW: Lambda captures large objects by value
events->subscribe(Events::Collision, [largeObject](const EventData& data) {
    // largeObject is copied!
});

// FAST: Capture by reference or pointer
events->subscribe(Events::Collision, [&largeObject](const EventData& data) {
    // No copy
});

// FAST: Capture only what you need
events->subscribe(Events::Collision, [ptr = &smallData](const EventData& data) {
    // Minimal capture
});
```

### Batch Processing

Process related events together:

```cpp
void update(DeltaTime dt) {
    // Collect events during update
    for (auto& collision : physicsCollisions) {
        events_->queue(Events::Collision, collision);
    }

    // Process all at once
    events_->processQueue();
}
```

## Best Practices

### 1. Use Predefined Event Types

```cpp
// GOOD: Use constants
events->subscribe(Events::Collision, callback);

// BAD: String literals prone to typos
events->subscribe("collison", callback);  // Typo!
```

### 2. Store Subscription IDs

```cpp
// GOOD: Save ID for cleanup
class System {
    SubscriptionId subId_;
public:
    void init(IEventSystem* e) {
        subId_ = e->subscribe(Events::Collision, [](auto& d){});
    }
    void shutdown(IEventSystem* e) {
        e->unsubscribe(subId_);
    }
};

// BAD: Leaked subscription
void init(IEventSystem* e) {
    e->subscribe(Events::Collision, [](auto& d){});  // Leaked!
}
```

### 3. Validate Event Data

```cpp
// GOOD: Check variant type
events->subscribe("mixed_event", [](const EventData& data) {
    if (auto* collision = std::get_if<CollisionEvent>(&data)) {
        // Safe access
    }
});

// BAD: Assume type without checking
events->subscribe("mixed_event", [](const EventData& data) {
    auto& collision = std::get<CollisionEvent>(data);  // May throw!
});
```

### 4. Prefer Queue for Non-Critical Events

```cpp
// GOOD: Queue non-critical events
void onItemPickup() {
    events_->queue(Events::ItemCollected, itemData);
}

// BAD: Immediate dispatch may cause recursion
void onItemPickup() {
    events_->publish(Events::ItemCollected, itemData);  // Could recurse!
}
```

### 5. Document Custom Events

```cpp
// GOOD: Document event data type
namespace MyEvents {
    // Publishes: DamageEventData
    inline constexpr const char* BossDamage = "boss_damage";

    // Publishes: std::any containing BossPhaseData
    inline constexpr const char* BossPhase = "boss_phase";
}

// BAD: Undocumented magic string
events->publish("boss_thing", someData);  // What data type?
```

## Complete Example: Platformer Game

```cpp
import bestow;
import bestow.events.impl;

class PlatformerGame {
public:
    void initialize() {
        events_ = createEventSystem();

        setupPhysicsEvents();
        setupGameplayEvents();
        setupUIEvents();
    }

    void update(DeltaTime dt) {
        inputSystem_->update(dt);
        physicsSystem_->update(dt);
        gameplaySystem_->update(dt);

        // Process all queued events from this frame
        events_->processQueue();

        renderSystem_->render();
    }

private:
    void setupPhysicsEvents() {
        // Physics publishes collisions
        physicsSystem_->onCollision([this](Entity a, Entity b, Vec2 point) {
            CollisionEvent collision{
                .entityA = a,
                .entityB = b,
                .contactPoint = point,
                .normal = {0.0f, -1.0f},
                .impulse = 1.0f
            };
            events_->publish(Events::Collision, collision);
        });
    }

    void setupGameplayEvents() {
        // Player collects item
        events_->subscribe(Events::Collision, [this](const EventData& data) {
            auto& collision = std::get<CollisionEvent>(data);

            if (isPlayer(collision.entityA) && isCollectible(collision.entityB)) {
                // Queue UI update
                EntityEventData collected{
                    .entity = collision.entityB,
                    .otherEntity = collision.entityA
                };
                events_->queue(Events::ItemCollected, collected);

                // Remove collectible
                entitySystem_->destroy(collision.entityB);
            }
        });

        // Entity takes damage
        events_->subscribe(Events::EntityDamaged, [this](const EventData& data) {
            auto& damage = std::get<DamageEventData>(data);

            // Play damage sound
            audioSystem_->playSound("hit.wav");

            // Visual feedback
            vfxSystem_->spawnDamageNumber(damage.target, damage.amount);

            // Apply knockback
            physicsSystem_->applyImpulse(damage.target, damage.knockback);
        });

        // Entity dies
        events_->subscribe(Events::EntityDied, [this](const EventData& data) {
            auto& death = std::get<EntityEventData>(data);

            if (death.entity == playerEntity_) {
                // Player died - respawn
                EntityEventData playerDeath{.entity = death.entity};
                events_->queue(Events::PlayerDeath, playerDeath);
            } else {
                // Enemy died - spawn loot, update score
                spawnLoot(death.entity);
                updateScore(100);
            }

            // Spawn death VFX
            vfxSystem_->spawnExplosion(death.entity);
        });
    }

    void setupUIEvents() {
        // Update UI when items collected
        events_->subscribe(Events::ItemCollected, [this](const EventData& data) {
            auto& collected = std::get<EntityEventData>(data);
            uiSystem_->updateCollectibleCount(++collectibleCount_);
        });

        // Show game over screen
        events_->subscribe(Events::PlayerDeath, [this](const EventData& data) {
            uiSystem_->showGameOver();
        });
    }

    std::unique_ptr<IEventSystem> events_;
    // ... other systems ...
    int collectibleCount_ = 0;
    Entity playerEntity_;
};
```

## API Reference

### IEventSystem Interface

```cpp
class IEventSystem {
public:
    // Publishing
    virtual void publish(const EventType& type, const EventData& data) = 0;
    virtual void queue(const EventType& type, const EventData& data) = 0;

    // Subscribing
    virtual SubscriptionId subscribe(const EventType& type,
                                     EventCallback callback) = 0;
    virtual void unsubscribe(SubscriptionId id) = 0;
    virtual void unsubscribeAll(const EventType& type) = 0;

    // Processing
    virtual void processQueue() = 0;
    virtual void clearQueue() = 0;
    virtual std::size_t queueSize() const = 0;
};
```

### Factory Function

```cpp
// Create event system instance
std::unique_ptr<IEventSystem> createEventSystem();
```

### Type Aliases

```cpp
using EventType = std::string;
using EventCallback = std::function<void(const EventData&)>;
using SubscriptionId = UUID;  // std::uint64_t
```

## Troubleshooting

### Problem: Event Not Received

**Cause**: Subscription not set up before event published

**Solution**: Subscribe during initialization:
```cpp
void initialize() {
    events->subscribe(Events::Collision, callback);  // Subscribe first
}

void later() {
    events->publish(Events::Collision, data);  // Now it works
}
```

### Problem: std::bad_variant_access Exception

**Cause**: Wrong type extracted from EventData variant

**Solution**: Use `std::get_if` for safe access:
```cpp
// Safe version
if (auto* collision = std::get_if<CollisionEvent>(&data)) {
    // Use collision
}
```

### Problem: Callback Invoked After Object Destroyed

**Cause**: Subscription not cleaned up in destructor

**Solution**: Unsubscribe in destructor:
```cpp
~MySystem() {
    events_->unsubscribe(subscriptionId_);
}
```

### Problem: Events Processed Out of Order

**Cause**: Mixing immediate and queued dispatch

**Solution**: Be consistent with dispatch method:
```cpp
// All immediate
events->publish(Events::Collision, c1);
events->publish(Events::Collision, c2);

// Or all queued
events->queue(Events::Collision, c1);
events->queue(Events::Collision, c2);
events->processQueue();  // Processed in order
```

## Further Reading

- **Bestow Technical Design**: `/docs/bestow-technical-design.md`
- **Project Status**: `/docs/PROJECT-STATUS.md`
- **System Implementation Guide**: `/docs/SYSTEM-IMPLEMENTATION-GUIDE.md`
- **Event System Tests**: `/tests/unit/EventSystemTests.cpp`
- **Event System Interface**: `/bestow-contract/src/bestow.events.cppm`
- **Event System Implementation**: `/bestow-events/src/bestow.events.impl.cppm`
