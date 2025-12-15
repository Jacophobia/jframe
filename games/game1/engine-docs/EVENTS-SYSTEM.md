# Bestow Events System Guide

## Overview

The **Events System** provides a decoupled, type-safe mechanism for inter-system communication in Bestow. Instead of systems directly calling each other, they publish events and subscribe to notifications, enabling loose coupling, testability, and flexibility.

### Key Benefits

1. **Decoupling** - Systems don't need references to each other
2. **Testability** - Easy to mock and test event-driven behavior
3. **Flexibility** - Multiple subscribers can respond to the same event
4. **Centralized Communication** - Single place to observe all system interactions
5. **Deferred Processing** - Events can be queued and processed later for predictable timing

### When to Use Events vs Direct Calls

**Use Events for:**
- Cross-system notifications (asset loaded, level changed, entity created)
- Decoupled communication where the sender doesn't know or care about receivers
- Events with multiple potential subscribers
- Game logic events (score changed, player died, checkpoint reached)
- Debugging and logging system interactions

**Use Direct Calls for:**
- Performance-critical paths where event dispatch overhead matters (e.g., rendering loops)
- Simple 1:1 relationships within the same system
- Operations that require immediate return values

---

## Core Concepts

### EventType

An event identifier, represented as a `std::string`. Use the built-in constants from the `Events` namespace for common events, or define your own custom strings.

```cpp
using EventType = std::string;

// Built-in event types
Events::Collision
Events::LevelLoaded
Events::EntityDied
// ... and many more

// Custom event types
"player_scored"
"powerup_collected"
"boss_phase_changed"
```

### EventData

A type-safe variant that can hold different event payload types. Bestow provides several built-in data types:

```cpp
using EventData = std::variant<
    EntityEventData,
    DamageEventData,
    LevelEventData,
    CollisionEvent,
    TriggerEvent,
    CollisionEvent3D,
    TriggerEvent3D,
    AssetEventData,
    ConfigEventData,
    ShaderReloadEventData,
    StateChangeEventData,
    FileChangeEventData,
    std::any  // For custom data
>;
```

#### Common Event Data Types

```cpp
// Entity-related events
struct EntityEventData {
    Entity entity;
    std::optional<Entity> otherEntity;
};

// Damage events
struct DamageEventData {
    Entity target;
    Entity source;
    int amount;
    Vec2 knockback;
};

// Collision events (2D)
struct CollisionEvent {
    Entity entityA;
    Entity entityB;
    Vec2 contactPoint;
    Vec2 normal;
    float impulse;
};

// Trigger events (2D)
struct TriggerEvent {
    Entity entityA;
    Entity entityB;
    Vec2 contactPoint;
};

// Asset events
struct AssetEventData {
    AssetHandle handle;
    AssetType type;
    AssetState state;
    std::string error;
};
```

### SubscriptionId

A unique identifier returned when you subscribe to an event. Store this to unsubscribe later.

```cpp
using SubscriptionId = UUID;  // std::uint64_t

SubscriptionId id = events->subscribe("my_event", [](const EventData&) {
    // Handle event
});

// Later, unsubscribe
events->unsubscribe(id);
```

### Immediate vs Deferred Dispatch

The Events System supports two dispatch modes:

#### Immediate Dispatch - `publish()`

Events are delivered **immediately and synchronously** to all subscribers. Use this when:
- The event must be processed right now
- Subscribers need to react before the next line of code executes
- You're in a safe context (not inside another callback)

```cpp
// Publish immediately - all callbacks execute NOW
events->publish(Events::EntityDied, EntityEventData{player});
// All subscribers have been notified by the time we reach this line
```

#### Deferred Dispatch - `queue()` + `processQueue()`

Events are **queued and processed later** during your main loop. Use this when:
- You want to batch event processing
- You're publishing from a callback and want to avoid reentrancy
- You want predictable event processing timing (once per frame)
- You want to control exactly when events are processed

```cpp
// Queue event - nothing happens yet
events->queue(Events::AssetLoaded, assetData);

// Later, in your main loop
void update(DeltaTime dt) {
    // Process all queued events in FIFO order
    events->processQueue();
}
```

**Important:** Queued events are processed in **FIFO order** (first-in, first-out).

**Note:** The current implementation of `queue()` is **not thread-safe**. Always queue events from the main thread. If you need to publish events from worker threads, consider using a thread-safe queue externally and processing it on the main thread.

---

## API Reference

### IEventSystem Interface

```cpp
class IEventSystem {
public:
    //======================================================================
    // Publishing
    //======================================================================

    // Immediate dispatch - callbacks execute NOW
    virtual void publish(const EventType& type, const EventData& data) = 0;

    // Deferred dispatch - callbacks execute during processQueue()
    virtual void queue(const EventType& type, const EventData& data) = 0;

    //======================================================================
    // Subscribing
    //======================================================================

    // Subscribe to an event type
    virtual SubscriptionId subscribe(const EventType& type,
                                     EventCallback callback) = 0;

    // Unsubscribe a specific callback
    virtual void unsubscribe(SubscriptionId id) = 0;

    // Unsubscribe ALL callbacks for an event type
    virtual void unsubscribeAll(const EventType& type) = 0;

    //======================================================================
    // Processing
    //======================================================================

    // Process all queued events (call once per frame)
    virtual void processQueue() = 0;

    // Clear the queue without processing (rarely needed)
    virtual void clearQueue() = 0;

    // Get the number of queued events
    virtual std::size_t queueSize() const = 0;
};
```

### EventCallback

Callbacks receive an `EventData` reference. Use `std::get<>` to extract the specific data type:

```cpp
using EventCallback = std::function<void(const EventData&)>;

// Example callback
auto callback = [](const EventData& data) {
    // Extract the specific event data type
    auto& collision = std::get<CollisionEvent>(data);

    // Use the data
    std::println("Collision between {} and {}",
                 static_cast<int>(collision.entityA),
                 static_cast<int>(collision.entityB));
};
```

---

## Built-in Events

### Physics Events (2D)

```cpp
Events::Collision        // 2D collision occurred
Events::TriggerEnter     // 2D trigger entered
Events::TriggerExit      // 2D trigger exited
```

**Data Type:** `CollisionEvent` or `TriggerEvent`

**CollisionEvent Structure:**
```cpp
struct CollisionEvent {
    Entity entityA;
    Entity entityB;
    Vec2 contactPoint;
    Vec2 normal;
    float impulse;
};
```

**TriggerEvent Structure:**
```cpp
struct TriggerEvent {
    Entity entityA;
    Entity entityB;
    Vec2 contactPoint;
};
```

**Example:**
```cpp
events->subscribe(Events::Collision, [](const EventData& data) {
    auto& collision = std::get<CollisionEvent>(data);

    std::println("Collision at ({}, {})",
                 collision.contactPoint.x,
                 collision.contactPoint.y);
    std::println("Normal: ({}, {})", collision.normal.x, collision.normal.y);
    std::println("Impulse: {}", collision.impulse);
});
```

### Physics Events (3D)

```cpp
Events::Collision3D      // 3D collision occurred
Events::TriggerEnter3D   // 3D trigger entered
Events::TriggerExit3D    // 3D trigger exited
```

**CollisionEvent3D Structure:**
```cpp
struct CollisionEvent3D {
    Entity entityA;
    Entity entityB;
    Vec3 contactPoint;
    Vec3 contactNormal;
    float impulse;
    float penetrationDepth;
};
```

**TriggerEvent3D Structure:**
```cpp
struct TriggerEvent3D {
    Entity entityA;
    Entity entityB;
};
```

**Data Type:** `CollisionEvent3D` or `TriggerEvent3D`

### Level Events

```cpp
Events::LevelLoaded      // Level finished loading
Events::LevelUnloaded    // Level unloaded
Events::Checkpoint       // Checkpoint reached
```

**LevelEventData Structure:**
```cpp
struct LevelEventData {
    LevelId levelId;
    LevelEvent event;
};
```

**EntityEventData Structure:**
```cpp
struct EntityEventData {
    Entity entity;
    std::optional<Entity> otherEntity;
};
```

**Data Type:** `LevelEventData` or `EntityEventData`

### Entity Events

```cpp
Events::EntityDamaged    // Entity took damage
Events::EntityDied       // Entity health reached zero
Events::ItemCollected    // Collectible item picked up
Events::PlayerDeath      // Player entity died (game over)
```

**DamageEventData Structure:**
```cpp
struct DamageEventData {
    Entity target;
    Entity source;
    int amount;
    Vec2 knockback;
};
```

**Data Type:** `DamageEventData` or `EntityEventData`

**Example:**
```cpp
events->subscribe(Events::EntityDamaged, [](const EventData& data) {
    auto& damage = std::get<DamageEventData>(data);

    std::println("Entity {} took {} damage from {}",
                 static_cast<int>(damage.target),
                 damage.amount,
                 static_cast<int>(damage.source));

    // Apply knockback
    auto knockback = damage.knockback;
});
```

### Asset System Events

```cpp
Events::AssetLoaded      // Asset loaded successfully
Events::AssetUnloaded    // Asset unloaded
Events::AssetReloaded    // Asset hot-reloaded (dev tools)
Events::AssetError       // Asset loading failed
```

**AssetEventData Structure:**
```cpp
struct AssetEventData {
    AssetHandle handle;
    AssetType type;
    AssetState state;
    std::string error;  // Empty if no error
};
```

**Data Type:** `AssetEventData`

**Example:**
```cpp
events->subscribe(Events::AssetLoaded, [](const EventData& data) {
    auto& asset = std::get<AssetEventData>(data);

    if (asset.type == AssetType::Texture) {
        std::println("Texture loaded: handle={}", asset.handle.uuid);
    }

    if (asset.state == AssetState::Failed) {
        std::println("Error: {}", asset.error);
    }
});
```

### Config System Events

```cpp
Events::ConfigChanged    // Configuration value changed
```

**ConfigEventData Structure:**
```cpp
struct ConfigEventData {
    std::string key;
    std::string section;
};
```

**Data Type:** `ConfigEventData`

**Example:**
```cpp
events->subscribe(Events::ConfigChanged, [this](const EventData& data) {
    auto& config = std::get<ConfigEventData>(data);

    if (config.key == "volume" && config.section == "audio") {
        // Reload audio settings
        reloadAudioVolume();
    }
});
```

### Shader System Events

```cpp
Events::ShaderReloaded   // Shader hot-reloaded (dev tools)
Events::MaterialReloaded // Material hot-reloaded (dev tools)
```

**ShaderReloadEventData Structure:**
```cpp
struct ShaderReloadEventData {
    std::uint64_t handle;  // ShaderProgramHandle or MaterialHandle
    bool success;
    std::string error;
};
```

**Data Type:** `ShaderReloadEventData`

### Game State Events

```cpp
Events::StateChanged     // Game state changed
Events::StatePushed      // Game state pushed onto stack
Events::StatePopped      // Game state popped from stack
```

**StateChangeEventData Structure:**
```cpp
struct StateChangeEventData {
    std::string oldStateName;
    std::string newStateName;
};
```

**Data Type:** `StateChangeEventData`

### Save System Events

```cpp
Events::GameSaved        // Game saved successfully
Events::GameLoaded       // Game loaded successfully
```

### Dev Tools Events

```cpp
Events::FileChanged      // File changed on disk (hot reload)
```

**FileChangeEventData Structure:**
```cpp
struct FileChangeEventData {
    std::string path;
    std::string fileType;  // "shader", "config", "texture", etc.
};
```

**Data Type:** `FileChangeEventData`

---

## Custom Events

### Defining Custom Event Types

Simply use a unique string identifier:

```cpp
// In your game code
constexpr const char* ScoreChanged = "score_changed";
constexpr const char* PowerupActivated = "powerup_activated";
constexpr const char* BossPhaseChanged = "boss_phase_changed";
constexpr const char* ComboMultiplier = "combo_multiplier";
```

### Creating Custom Event Data

For type-safe custom data, use `std::any`:

```cpp
struct ScoreChangedData {
    int oldScore;
    int newScore;
    Entity scoringEntity;
};

// Publish
ScoreChangedData scoreData{oldScore, newScore, player};
events->publish("score_changed", std::any(scoreData));

// Subscribe
events->subscribe("score_changed", [](const EventData& data) {
    auto scoreData = std::any_cast<ScoreChangedData>(std::get<std::any>(data));

    std::println("Score: {} -> {} (+{})",
                 scoreData.oldScore,
                 scoreData.newScore,
                 scoreData.newScore - scoreData.oldScore);
});
```

### Alternative: Extend EventData Variant

For engine-level custom events, you can extend the `EventData` variant by modifying `bestow.types.cppm`:

```cpp
// In bestow.types.cppm
struct ScoreChangedData {
    int oldScore;
    int newScore;
    Entity scoringEntity;
};

using EventData = std::variant<
    EntityEventData,
    DamageEventData,
    // ... other types ...
    ScoreChangedData,  // Add your custom type
    std::any
>;
```

Then use it type-safely:

```cpp
// Publish
events->publish("score_changed", ScoreChangedData{oldScore, newScore, player});

// Subscribe
events->subscribe("score_changed", [](const EventData& data) {
    auto& scoreData = std::get<ScoreChangedData>(data);
    // Use scoreData directly, no std::any_cast needed
});
```

---

## Best Practices

### 1. Event Lifetime and Cleanup

**Always unsubscribe when your subscriber is destroyed:**

```cpp
class MyGameSystem {
public:
    MyGameSystem(IEventSystem& events) : events_(&events) {
        // Subscribe in constructor or init()
        collisionSubId_ = events_->subscribe(Events::Collision,
            [this](const EventData& data) {
                onCollision(data);
            });
    }

    ~MyGameSystem() {
        // CRITICAL: Unsubscribe in destructor
        events_->unsubscribe(collisionSubId_);
    }

private:
    void onCollision(const EventData& data) {
        auto& collision = std::get<CollisionEvent>(data);
        // Handle collision
    }

    IEventSystem* events_;
    SubscriptionId collisionSubId_;
};
```

**RAII Helper for Automatic Cleanup:**

```cpp
class ScopedSubscription {
public:
    ScopedSubscription(IEventSystem& events, const EventType& type, EventCallback callback)
        : events_(&events) {
        id_ = events_->subscribe(type, std::move(callback));
    }

    ~ScopedSubscription() {
        events_->unsubscribe(id_);
    }

    // Non-copyable, movable
    ScopedSubscription(const ScopedSubscription&) = delete;
    ScopedSubscription& operator=(const ScopedSubscription&) = delete;
    ScopedSubscription(ScopedSubscription&&) = default;
    ScopedSubscription& operator=(ScopedSubscription&&) = default;

private:
    IEventSystem* events_;
    SubscriptionId id_;
};

// Usage
class MySystem {
    ScopedSubscription collisionSub_;

public:
    MySystem(IEventSystem& events)
        : collisionSub_(events, Events::Collision, [this](const EventData& data) {
            onCollision(data);
        }) {}
    // Automatically unsubscribes when MySystem is destroyed
};
```

### 2. Avoiding Event Storms

**Event storms** occur when events trigger other events in a chain reaction. To avoid:

**Bad - Event Storm:**
```cpp
// System A
events->subscribe(Events::EntityDied, [this](const EventData& data) {
    events->publish(Events::ScoreChanged, scoreData);  // Triggers System B
});

// System B
events->subscribe(Events::ScoreChanged, [this](const EventData& data) {
    events->publish(Events::UIUpdate, uiData);  // Triggers System C
});

// System C
events->subscribe(Events::UIUpdate, [this](const EventData& data) {
    events->publish(Events::SoundEffect, soundData);  // Triggers System D
});
// This creates a deep call stack and unpredictable ordering
```

**Good - Use Queued Events:**
```cpp
// System A
events->subscribe(Events::EntityDied, [this](const EventData& data) {
    // Queue instead of publish - breaks the chain
    events->queue(Events::ScoreChanged, scoreData);
});

// System B
events->subscribe(Events::ScoreChanged, [this](const EventData& data) {
    events->queue(Events::UIUpdate, uiData);
});

// System C
events->subscribe(Events::UIUpdate, [this](const EventData& data) {
    events->queue(Events::SoundEffect, soundData);
});

// In main loop
void update(DeltaTime dt) {
    // All events process in predictable order
    events->processQueue();
}
```

### 3. Performance Considerations

**Event Publishing Has Overhead:**
- Each `publish()` call iterates over all subscribers
- Each callback invocation has function call overhead
- Use direct calls for performance-critical inner loops

**Benchmark:**
```cpp
// DON'T do this in a rendering loop
for (const auto& entity : allEntities) {
    events->publish("entity_rendered", EntityEventData{entity});  // BAD
}

// DO use direct calls
for (const auto& entity : allEntities) {
    renderer->renderEntity(entity);  // GOOD
}
```

**Queue for Batch Processing:**
```cpp
// Batch small events for processing later
void onPlayerMoved(Vec2 newPosition) {
    events->queue("player_moved", PositionData{newPosition});
}

// Process all movement events once per frame
void update(DeltaTime dt) {
    events->processQueue();  // Efficient batch processing
}
```

### 4. Event Ordering

Subscribers are notified **in the order they subscribed**:

```cpp
events->subscribe("test", [](const EventData&) { std::println("First"); });
events->subscribe("test", [](const EventData&) { std::println("Second"); });
events->subscribe("test", [](const EventData&) { std::println("Third"); });

events->publish("test", EntityEventData{});
// Output:
// First
// Second
// Third
```

**Don't rely on subscription order for critical game logic.** If order matters, use a dedicated sequencing system or state machine.

### 5. Thread Safety

**Neither publishing nor queuing is currently thread-safe.** Always interact with the events system from the main thread only.

```cpp
// UNSAFE - Don't call from worker threads
std::thread worker([&events]() {
    events->publish(Events::AssetLoaded, assetData);  // DANGER: Not thread-safe
    events->queue(Events::AssetLoaded, assetData);    // DANGER: Not thread-safe
});

// SAFE - Use external thread-safe queue
std::queue<EventData> workerQueue;
std::mutex queueMutex;

std::thread worker([&workerQueue, &queueMutex]() {
    std::lock_guard<std::mutex> lock(queueMutex);
    workerQueue.push(assetData);  // Thread-safe external queue
});

// Process on main thread
void update(DeltaTime dt) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        while (!workerQueue.empty()) {
            events->queue(Events::AssetLoaded, workerQueue.front());
            workerQueue.pop();
        }
    }
    events->processQueue();
}
```

### 6. Debugging Events

**Log all events during development:**

```cpp
#if defined(BESTOW_DEV_TOOLS)
class EventLogger {
public:
    EventLogger(IEventSystem& events) {
        // Subscribe to ALL common events
        for (const char* eventType : {
            Events::Collision, Events::TriggerEnter, Events::LevelLoaded,
            Events::EntityDied, Events::AssetLoaded, Events::ConfigChanged
        }) {
            events.subscribe(eventType, [eventType](const EventData& data) {
                std::println("[EVENT] {}", eventType);
            });
        }
    }
};
#endif
```

---

## Code Examples

### Example 1: Handling Collisions

```cpp
class PlayerCollisionSystem {
public:
    PlayerCollisionSystem(IEventSystem& events, IEntitySystem& entities)
        : events_(&events), entities_(&entities) {

        // Subscribe to 2D collision events
        collisionSubId_ = events_->subscribe(Events::Collision,
            [this](const EventData& data) {
                onCollision(data);
            });
    }

    ~PlayerCollisionSystem() {
        events_->unsubscribe(collisionSubId_);
    }

private:
    void onCollision(const EventData& data) {
        auto& collision = std::get<CollisionEvent>(data);

        // Check if player is involved
        Entity player = findPlayer();
        if (collision.entityA != player && collision.entityB != player) {
            return;  // Not a player collision
        }

        Entity other = (collision.entityA == player) ? collision.entityB : collision.entityA;

        // Check what the player collided with
        if (entities_->has<EnemyComponent>(other)) {
            // Player hit enemy - queue damage event
            events_->queue(Events::EntityDamaged, DamageEventData{
                .target = player,
                .source = other,
                .amount = 10,
                .knockback = {collision.normal.x * -200.0f, collision.normal.y * -200.0f}
            });
        }
        else if (entities_->has<CollectibleComponent>(other)) {
            // Player collected item
            events_->queue(Events::ItemCollected, EntityEventData{
                .entity = other,
                .otherEntity = player
            });
        }
    }

    Entity findPlayer() {
        // Implementation to find player entity
        return Entity{};  // Placeholder
    }

    IEventSystem* events_;
    IEntitySystem* entities_;
    SubscriptionId collisionSubId_;
};
```

### Example 2: Level Transitions

```cpp
class LevelTransitionSystem {
public:
    LevelTransitionSystem(IEventSystem& events, ILevelSystem& levels)
        : events_(&events), levels_(&levels) {

        // Subscribe to level events
        levelLoadedSub_ = events_->subscribe(Events::LevelLoaded,
            [this](const EventData& data) { onLevelLoaded(data); });

        levelUnloadedSub_ = events_->subscribe(Events::LevelUnloaded,
            [this](const EventData& data) { onLevelUnloaded(data); });
    }

    ~LevelTransitionSystem() {
        events_->unsubscribe(levelLoadedSub_);
        events_->unsubscribe(levelUnloadedSub_);
    }

    void transitionToLevel(LevelId newLevel) {
        // Queue level transition
        LevelTransition transition{
            .fromLevel = currentLevel_,
            .toLevel = newLevel,
            .spawnPoint = "player_start",
            .unloadPrevious = true
        };

        // Unload current level
        if (currentLevel_ != 0) {
            levels_->unloadLevel(currentLevel_);
        }

        // Load new level
        levels_->loadLevel(newLevel);
        currentLevel_ = newLevel;
    }

private:
    void onLevelLoaded(const EventData& data) {
        auto& levelData = std::get<LevelEventData>(data);
        std::println("Level {} loaded successfully", levelData.levelId);

        // Fade in, spawn player, etc.
    }

    void onLevelUnloaded(const EventData& data) {
        auto& levelData = std::get<LevelEventData>(data);
        std::println("Level {} unloaded", levelData.levelId);
    }

    IEventSystem* events_;
    ILevelSystem* levels_;
    LevelId currentLevel_ = 0;
    SubscriptionId levelLoadedSub_;
    SubscriptionId levelUnloadedSub_;
};
```

### Example 3: Custom Game Events (Score System)

```cpp
// Define custom event data
struct ScoreChangedData {
    Entity scoringEntity;
    int oldScore;
    int newScore;
    int delta;
    std::string reason;  // "enemy_killed", "coin_collected", etc.
};

class ScoreSystem {
public:
    ScoreSystem(IEventSystem& events) : events_(&events) {
        // Subscribe to events that award points
        enemyKilledSub_ = events_->subscribe(Events::EntityDied,
            [this](const EventData& data) { onEnemyDied(data); });

        itemCollectedSub_ = events_->subscribe(Events::ItemCollected,
            [this](const EventData& data) { onItemCollected(data); });
    }

    ~ScoreSystem() {
        events_->unsubscribe(enemyKilledSub_);
        events_->unsubscribe(itemCollectedSub_);
    }

private:
    void addScore(Entity entity, int points, std::string_view reason) {
        int oldScore = scores_[entity];
        int newScore = oldScore + points;
        scores_[entity] = newScore;

        // Publish custom score changed event
        ScoreChangedData scoreData{
            .scoringEntity = entity,
            .oldScore = oldScore,
            .newScore = newScore,
            .delta = points,
            .reason = std::string(reason)
        };

        events_->publish("score_changed", std::any(scoreData));
    }

    void onEnemyDied(const EventData& data) {
        auto& entityData = std::get<EntityEventData>(data);
        if (entityData.otherEntity) {
            // Award points to the entity that killed the enemy
            addScore(*entityData.otherEntity, 100, "enemy_killed");
        }
    }

    void onItemCollected(const EventData& data) {
        auto& entityData = std::get<EntityEventData>(data);
        if (entityData.otherEntity) {
            // Award points to the collector
            addScore(*entityData.otherEntity, 10, "coin_collected");
        }
    }

    IEventSystem* events_;
    std::unordered_map<Entity, int> scores_;
    SubscriptionId enemyKilledSub_;
    SubscriptionId itemCollectedSub_;
};
```

### Example 4: Event-Driven UI Updates

```cpp
class GameUI {
public:
    GameUI(IEventSystem& events) : events_(&events) {
        // Subscribe to game events for UI updates
        scoreChangedSub_ = events_->subscribe("score_changed",
            [this](const EventData& data) { onScoreChanged(data); });

        healthChangedSub_ = events_->subscribe(Events::EntityDamaged,
            [this](const EventData& data) { onHealthChanged(data); });

        levelLoadedSub_ = events_->subscribe(Events::LevelLoaded,
            [this](const EventData& data) { onLevelLoaded(data); });
    }

    ~GameUI() {
        events_->unsubscribe(scoreChangedSub_);
        events_->unsubscribe(healthChangedSub_);
        events_->unsubscribe(levelLoadedSub_);
    }

private:
    void onScoreChanged(const EventData& data) {
        auto scoreData = std::any_cast<ScoreChangedData>(std::get<std::any>(data));

        // Update score display with animation
        scoreText_.setText(std::format("Score: {}", scoreData.newScore));

        // Show score popup
        if (scoreData.delta > 0) {
            showScorePopup(scoreData.delta, scoreData.reason);
        }
    }

    void onHealthChanged(const EventData& data) {
        auto& damage = std::get<DamageEventData>(data);

        if (isPlayer(damage.target)) {
            // Update health bar
            updateHealthBar(damage.target);

            // Show damage indicator
            showDamageFlash();
        }
    }

    void onLevelLoaded(const EventData& data) {
        auto& levelData = std::get<LevelEventData>(data);

        // Show level name overlay
        showLevelNameOverlay(levelData.levelId);
    }

    void showScorePopup(int points, const std::string& reason) {
        // Implementation
    }

    void updateHealthBar(Entity player) {
        // Implementation
    }

    void showDamageFlash() {
        // Implementation
    }

    void showLevelNameOverlay(LevelId levelId) {
        // Implementation
    }

    bool isPlayer(Entity entity) {
        // Implementation
        return false;
    }

    IEventSystem* events_;
    SubscriptionId scoreChangedSub_;
    SubscriptionId healthChangedSub_;
    SubscriptionId levelLoadedSub_;

    struct {
        void setText(const std::string&) {}
    } scoreText_;
};
```

### Example 5: Asset System Integration

```cpp
class TextureManager {
public:
    TextureManager(IEventSystem& events, IAssetSystem& assets)
        : events_(&events), assets_(&assets) {

        // Subscribe to asset events
        assetLoadedSub_ = events_->subscribe(Events::AssetLoaded,
            [this](const EventData& data) { onAssetLoaded(data); });

        assetReloadedSub_ = events_->subscribe(Events::AssetReloaded,
            [this](const EventData& data) { onAssetReloaded(data); });
    }

    ~TextureManager() {
        events_->unsubscribe(assetLoadedSub_);
        events_->unsubscribe(assetReloadedSub_);
    }

private:
    void onAssetLoaded(const EventData& data) {
        auto& asset = std::get<AssetEventData>(data);

        if (asset.type != AssetType::Texture) return;

        if (asset.state == AssetState::Loaded) {
            std::println("Texture loaded successfully: {}", asset.handle.uuid);
            // Texture is ready to use
        }
        else if (asset.state == AssetState::Failed) {
            std::println("Texture load failed: {}", asset.error);
        }
    }

    void onAssetReloaded(const EventData& data) {
        auto& asset = std::get<AssetEventData>(data);

        if (asset.type == AssetType::Texture) {
            std::println("Texture hot-reloaded: {}", asset.handle.uuid);
            // GPU texture has been updated - no action needed
        }
    }

    IEventSystem* events_;
    IAssetSystem* assets_;
    SubscriptionId assetLoadedSub_;
    SubscriptionId assetReloadedSub_;
};
```

### Example 6: Boss Fight Phase System

```cpp
// Custom event data
struct BossPhaseData {
    Entity boss;
    int oldPhase;
    int newPhase;
    float healthPercent;
};

class BossController {
public:
    BossController(IEventSystem& events, Entity boss)
        : events_(&events), boss_(boss), currentPhase_(1) {

        // Subscribe to damage events to track boss health
        damageSub_ = events_->subscribe(Events::EntityDamaged,
            [this](const EventData& data) { onDamaged(data); });
    }

    ~BossController() {
        events_->unsubscribe(damageSub_);
    }

private:
    void onDamaged(const EventData& data) {
        auto& damage = std::get<DamageEventData>(data);

        if (damage.target != boss_) return;

        // Check if boss entered a new phase
        float healthPercent = getCurrentHealthPercent();
        int newPhase = calculatePhase(healthPercent);

        if (newPhase != currentPhase_) {
            int oldPhase = currentPhase_;
            currentPhase_ = newPhase;

            // Publish phase change event
            BossPhaseData phaseData{
                .boss = boss_,
                .oldPhase = oldPhase,
                .newPhase = newPhase,
                .healthPercent = healthPercent
            };

            events_->publish("boss_phase_changed", std::any(phaseData));
        }
    }

    float getCurrentHealthPercent() {
        // Implementation
        return 0.75f;
    }

    int calculatePhase(float healthPercent) {
        if (healthPercent > 0.66f) return 1;
        if (healthPercent > 0.33f) return 2;
        return 3;
    }

    IEventSystem* events_;
    Entity boss_;
    int currentPhase_;
    SubscriptionId damageSub_;
};

class BossAudioSystem {
public:
    BossAudioSystem(IEventSystem& events, IAudioSystem& audio)
        : events_(&events), audio_(&audio) {

        // Subscribe to boss phase changes
        phaseSub_ = events_->subscribe("boss_phase_changed",
            [this](const EventData& data) { onPhaseChanged(data); });
    }

    ~BossAudioSystem() {
        events_->unsubscribe(phaseSub_);
    }

private:
    void onPhaseChanged(const EventData& data) {
        auto phaseData = std::any_cast<BossPhaseData>(std::get<std::any>(data));

        std::println("Boss entered phase {}", phaseData.newPhase);

        // Change music intensity
        switch (phaseData.newPhase) {
        case 1:
            audio_->playMusic(musicPhase1_, 1.0f);
            break;
        case 2:
            audio_->playMusic(musicPhase2_, 1.2f);  // Faster tempo
            break;
        case 3:
            audio_->playMusic(musicPhase3_, 1.5f);  // Intense finale
            break;
        }
    }

    IEventSystem* events_;
    IAudioSystem* audio_;
    SubscriptionId phaseSub_;
    AssetHandle musicPhase1_, musicPhase2_, musicPhase3_;
};
```

---

## Integration with Other Systems

### With Physics System

```cpp
class PhysicsSystem : public IPhysicsSystem {
    void detectCollisions() {
        // Detect collisions using Box2D, Jolt, etc.
        for (auto& collision : detectedCollisions) {
            // Publish collision event
            events_->publish(Events::Collision, CollisionEvent{
                .entityA = collision.entityA,
                .entityB = collision.entityB,
                .contactPoint = collision.point,
                .normal = collision.normal,
                .impulse = collision.impulse
            });
        }
    }
};
```

### With Asset System

```cpp
class AssetSystem : public IAssetSystem {
    void finishAsyncLoad(AssetHandle handle) {
        // Asset finished loading
        events_->publish(Events::AssetLoaded, AssetEventData{
            .handle = handle,
            .type = handle.type,
            .state = AssetState::Loaded,
            .error = ""
        });
    }
};
```

### With Save System

```cpp
class SaveSystem : public ISaveSystem {
    void saveGame(SaveSlot slot) {
        // Save game data
        bool success = writeSaveFile(slot);

        if (success) {
            events_->publish(Events::GameSaved, EntityEventData{});
        }
    }
};
```

---

## Summary

The **Bestow Events System** provides:

- **Decoupled Communication** - Systems interact without tight coupling
- **Type-Safe Payloads** - `std::variant`-based `EventData` with compile-time safety
- **Flexible Dispatch** - Immediate (`publish`) or deferred (`queue`) processing
- **Thread-Safe Queuing** - Safely publish events from worker threads
- **Built-in Events** - Common game events pre-defined for you
- **Custom Events** - Easy to define game-specific event types
- **Testability** - Mock events for unit testing
- **Debugging** - Centralized place to log all system interactions

**Key Takeaways:**

1. Use `publish()` for immediate synchronous notifications
2. Use `queue()` + `processQueue()` for batch processing and thread safety
3. Always `unsubscribe()` in destructors to avoid dangling callbacks
4. Prefer events for cross-system notifications, direct calls for performance-critical paths
5. Use `std::get<>` to extract typed event data from `EventData` variant

For more information, see:
- `bestow-contract/src/bestow.events.cppm` - Interface definition
- `bestow-contract/src/bestow.types.cppm` - EventData types
- `tests/unit/EventSystemTests.cpp` - Comprehensive test examples
