# Bestow Entity System Guide

The Entity System is Bestow's implementation of the Entity-Component-System (ECS) architecture pattern, powered by EnTT. It provides high-performance entity and component management for game development.

## Table of Contents

1. [Overview](#overview)
2. [Core Concepts](#core-concepts)
3. [API Reference](#api-reference)
4. [Best Practices](#best-practices)
5. [Common Patterns](#common-patterns)
6. [Performance Tips](#performance-tips)
7. [Code Examples](#code-examples)

---

## Overview

The Entity System provides:

- **Fast entity creation/destruction** - Millions of entities with minimal overhead
- **Cache-friendly component storage** - Components stored in contiguous memory for optimal CPU cache usage
- **Flexible queries** - Filter entities by component types efficiently
- **Type-safe operations** - Compile-time type checking for components
- **Direct EnTT access** - Full power of EnTT registry when needed

### What is ECS?

**Entity-Component-System (ECS)** is a data-oriented design pattern that separates:

- **Entities** - Unique identifiers (just integers)
- **Components** - Pure data structures (no logic)
- **Systems** - Logic that operates on entities with specific component combinations

### Why ECS?

Traditional object-oriented game development uses deep inheritance hierarchies (GameObject → Character → Enemy → FlyingEnemy). ECS favors **composition over inheritance**:

```cpp
// Traditional OOP - rigid inheritance
class FlyingEnemy : public Enemy {
    // Coupled behavior and data
};

// ECS - flexible composition
Entity enemy = entities->createEntity();
entities->emplace<Transform>(enemy, x, y);
entities->emplace<Sprite>(enemy, "enemy.png");
entities->emplace<Health>(enemy, 100);
entities->emplace<FlyingAI>(enemy);
```

**Benefits:**
- No inheritance hell
- Easy to add/remove behaviors at runtime
- Better cache locality and performance
- Natural parallelization opportunities

---

## Core Concepts

### Entity

An **Entity** is just a unique identifier (internally `entt::entity`, typically a 32-bit integer). It has no behavior or data itself - it's a handle that binds components together.

```cpp
Entity player = entities->createEntity();
Entity enemy = entities->createEntity();
```

### Component

A **Component** is a plain data structure (POD or aggregate type). Components should contain ONLY data, no logic.

```cpp
// Good - data only
struct Transform {
    float x, y;
    float rotation;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
};

// Good - data only
struct Health {
    int current;
    int maximum;
};

// Bad - contains logic (should be in a System instead)
struct BadComponent {
    void update() { /* ... */ }  // Don't do this!
};
```

### System

A **System** contains the logic that operates on entities with specific components. In Bestow, systems are typically functions or classes that query entities and process their components.

```cpp
// System logic - operates on entities with Transform and Velocity
void updateMovement(IEntitySystem& entities, DeltaTime dt) {
    for (auto [entity, transform, velocity] :
         entities.view<Transform, Velocity>().each()) {
        transform.x += velocity.dx * dt.count();
        transform.y += velocity.dy * dt.count();
    }
}
```

---

## API Reference

### Entity Lifecycle

#### `createEntity()`

Create a new entity.

```cpp
Entity createEntity();
```

**Example:**
```cpp
Entity player = entities->createEntity();
```

---

#### `destroyEntity(entity)`

Destroy an entity and all its components.

```cpp
void destroyEntity(Entity entity);
```

**Example:**
```cpp
entities->destroyEntity(player);
```

**Notes:**
- Safe to call on already-destroyed entities (no-op)
- All components are automatically removed
- Entity handle becomes invalid

---

#### `isValid(entity)`

Check if an entity still exists.

```cpp
bool isValid(Entity entity) const;
```

**Example:**
```cpp
if (entities->isValid(player)) {
    // Entity still exists
}
```

**Important:** Always check validity before using an entity handle stored from a previous frame.

---

#### `entityCount()`

Get the total number of alive entities.

```cpp
std::size_t entityCount() const;
```

**Example:**
```cpp
std::size_t total = entities->entityCount();
```

---

### Component Management

#### `emplace<T>(entity, args...)`

Add a component to an entity with constructor arguments.

```cpp
template<typename T, typename... Args>
T& emplace(Entity entity, Args&&... args);
```

**Returns:** Reference to the newly created component.

**Example:**
```cpp
// Add component with constructor args
auto& health = entities->emplace<Health>(player, 100, 100);

// Add component with aggregate initialization
auto& transform = entities->emplace<Transform>(player, 100.0f, 200.0f, 0.0f);

// Add tag component (empty struct)
entities->emplace<EnemyTag>(enemy);
```

**Note:** If the entity already has this component type, the behavior is undefined. Use `get()` to modify existing components.

---

#### `get<T>(entity)`

Get a reference to an entity's component.

```cpp
template<typename T>
T& get(Entity entity);

template<typename T>
const T& get(Entity entity) const;
```

**Returns:** Reference to the component.

**Throws:** If the entity doesn't have the component.

**Example:**
```cpp
// Modify component
auto& health = entities->get<Health>(player);
health.current -= 10;

// Read component
const auto& transform = entities->get<Transform>(player);
float x = transform.x;
```

---

#### `tryGet<T>(entity)`

Safely try to get a component (returns nullptr if not present).

```cpp
template<typename T>
T* tryGet(Entity entity);

template<typename T>
const T* tryGet(Entity entity) const;
```

**Returns:** Pointer to component, or `nullptr` if not present.

**Example:**
```cpp
if (auto* health = entities->tryGet<Health>(enemy)) {
    health->current -= damage;
} else {
    // Entity doesn't have health component
}
```

**Use case:** When you're not sure if an entity has a component.

---

#### `remove<T>(entity)`

Remove a component from an entity.

```cpp
template<typename T>
void remove(Entity entity);
```

**Example:**
```cpp
// Remove invincibility after power-up expires
entities->remove<Invincible>(player);
```

---

#### `allOf<T...>(entity)`

Check if an entity has ALL specified components.

```cpp
template<typename... Ts>
bool allOf(Entity entity) const;
```

**Example:**
```cpp
// Check single component
if (entities->allOf<Health>(enemy)) {
    // Enemy can take damage
}

// Check multiple components
if (entities->allOf<Transform, Sprite, Health>(entity)) {
    // Entity is a renderable, damageable game object
}
```

---

#### `anyOf<T...>(entity)`

Check if an entity has ANY of the specified components.

```cpp
template<typename... Ts>
bool anyOf(Entity entity) const;
```

**Example:**
```cpp
// Check if entity can move
if (entities->anyOf<Velocity, PhysicsBody>(entity)) {
    // Entity has some form of movement
}
```

---

### Querying and Iteration

#### `view<T...>()`

Create a view for iterating entities with specific components.

```cpp
template<typename... Components>
auto view();

template<typename... Components>
auto view() const;
```

**Returns:** EnTT view object (can be iterated with range-based for loop).

**Example:**
```cpp
// Iterate entities with Transform
for (auto entity : entities->view<Transform>()) {
    auto& transform = entities->get<Transform>(entity);
    // Process transform
}

// Iterate with multiple components
for (auto entity : entities->view<Transform, Velocity>()) {
    auto& transform = entities->get<Transform>(entity);
    auto& velocity = entities->get<Velocity>(entity);

    transform.x += velocity.dx * dt.count();
    transform.y += velocity.dy * dt.count();
}

// EnTT's each() provides unpacked access
for (auto [entity, transform, velocity] :
     entities->view<Transform, Velocity>().each()) {
    transform.x += velocity.dx * dt.count();
    transform.y += velocity.dy * dt.count();
}
```

**Performance:** Views are extremely fast - they don't allocate memory or copy entities.

---

#### `groupCount<T...>()`

Count entities with specific components.

```cpp
template<typename... Components>
std::size_t groupCount() const;
```

**Example:**
```cpp
std::size_t enemyCount = entities->groupCount<Enemy>();
std::size_t flyingEnemyCount = entities->groupCount<Enemy, Flying>();
```

**Note:** This is O(n) - it iterates all matching entities. Don't call every frame for large entity counts.

---

#### `hasAny<T...>()`

Check if ANY entities exist with specific components.

```cpp
template<typename... Components>
bool hasAny() const;
```

**Example:**
```cpp
// Check if any enemies remain
if (!entities->hasAny<Enemy>()) {
    // Level complete!
}

// Check if player exists
if (entities->hasAny<Player>()) {
    // Game is active
}
```

---

#### `first<T...>()`

Get the first entity with specific components.

```cpp
template<typename... Components>
std::optional<Entity> first() const;
```

**Returns:** The first matching entity, or `std::nullopt` if none exist.

**Example:**
```cpp
// Find player entity
if (auto playerEntity = entities->first<Player>()) {
    auto& transform = entities->get<Transform>(*playerEntity);
    // Use player transform
}
```

**Use case:** Getting singleton entities (player, camera, game manager).

---

#### `single<T...>()`

Get the single entity with specific components (returns nullopt if 0 or 2+ entities).

```cpp
template<typename... Components>
std::optional<Entity> single() const;
```

**Returns:** The entity if exactly one exists, `std::nullopt` otherwise.

**Example:**
```cpp
// Enforce singleton pattern
auto player = entities->single<Player>();
if (!player) {
    // Error: either no player or multiple players exist
}
```

**Use case:** Validating singleton entities.

---

#### `collect<T...>()`

Collect all entities with specific components into a vector.

```cpp
template<typename... Components>
std::vector<Entity> collect() const;
```

**Returns:** Vector of all matching entities.

**Example:**
```cpp
// Get all enemies to process
auto enemies = entities->collect<Enemy>();

// Safe to destroy during iteration
for (Entity enemy : enemies) {
    if (shouldRemove(enemy)) {
        entities->destroyEntity(enemy);
    }
}
```

**Use case:** When you need to modify entities during iteration (like destroying them).

**Performance:** Allocates a vector and copies entity handles. Use `view()` for simple iteration.

---

#### `collectExcluding<Include, Exclude...>()`

Collect entities WITH certain components but WITHOUT others.

```cpp
template<typename Include, typename... Exclude>
std::vector<Entity> collectExcluding() const;
```

**Example:**
```cpp
// Get all living enemies (exclude Dead tag)
auto aliveEnemies = entities->collectExcluding<Enemy, Dead>();

// Get all renderable entities except UI elements
auto worldObjects = entities->collectExcluding<Sprite, UIElement>();
```

---

#### `query(selector)`

Advanced query with predicates.

```cpp
std::vector<Entity> query(const EntitySelector& selector) const;

struct EntitySelector {
    std::vector<entt::id_type> requiredComponents;
    std::vector<entt::id_type> excludedComponents;
    std::optional<std::function<bool(Entity)>> predicate;
};
```

**Example:**
```cpp
// Find all enemies with low health
EntitySelector selector{};
selector.predicate = [&](Entity e) {
    auto* health = entities->tryGet<Health>(e);
    return health && health->current < 20;
};
auto lowHealthEnemies = entities->query(selector);
```

**Use case:** Complex queries with custom logic. For simple component filtering, use `view()` instead.

---

#### `each(callback)`

Iterate all valid entities.

```cpp
void each(std::function<void(Entity)> callback);
```

**Example:**
```cpp
// Process all entities
entities->each([&](Entity e) {
    // Handle entity
});
```

---

### Advanced: Direct Registry Access

#### `getRegistry()`

Access the underlying EnTT registry for advanced operations.

```cpp
entt::registry& getRegistry();
const entt::registry& getRegistry() const;
```

**Example:**
```cpp
auto& registry = entities->getRegistry();

// Use EnTT groups (cached multi-component queries)
auto group = registry.group<Transform>(entt::get<Velocity>);
for (auto entity : group) {
    auto& [transform, velocity] = group.get(entity);
    // Process
}

// Use EnTT exclusion filters
for (auto entity : registry.view<Sprite>(entt::exclude<Hidden>)) {
    // Only visible sprites
}
```

**Use case:** When you need EnTT-specific features not exposed by IEntitySystem.

---

## Best Practices

### 1. Component Design

**DO:**
- Keep components small and focused
- Use plain data types (POD/aggregates)
- Give components clear, single-purpose names
- Use tags for boolean states

```cpp
// Good - small, focused components
struct Position { float x, y; };
struct Velocity { float dx, dy; };
struct Health { int current, maximum; };
struct PlayerTag {};  // Empty tag
```

**DON'T:**
- Put logic in components
- Make "god components" with too much data
- Use pointers or complex ownership

```cpp
// Bad - too much data, unclear purpose
struct GameObject {
    float x, y, z;
    float rotation;
    std::string texturePath;
    int health;
    bool isAlive;
    void update();  // Components shouldn't have methods!
};
```

### 2. Use Tags for States

Tags are zero-size components used for marking entities:

```cpp
struct PlayerTag {};
struct EnemyTag {};
struct DeadTag {};
struct InvincibleTag {};

// Check state
if (entities->allOf<InvincibleTag>(player)) {
    // Player cannot take damage
}

// Add state
entities->emplace<DeadTag>(enemy);

// Remove state
entities->remove<DeadTag>(enemy);
```

**Note:** Tags must have at least one member to avoid EnTT limitations with empty types:

```cpp
// Correct tag definition
struct EnemyTag {
    bool _ = false;  // Dummy member
};
```

### 3. Entity Lifecycle Management

**Always check validity before using stored entity handles:**

```cpp
class GameSystem {
    Entity cachedPlayer_;

    void update(IEntitySystem& entities) {
        // Entity might have been destroyed since last frame
        if (entities->isValid(cachedPlayer_)) {
            auto& transform = entities->get<Transform>(cachedPlayer_);
            // Safe to use
        } else {
            // Find player again
            cachedPlayer_ = *entities->first<Player>();
        }
    }
};
```

**Destroying entities during iteration:**

```cpp
// WRONG - undefined behavior
for (auto entity : entities->view<Enemy>()) {
    if (shouldDestroy(entity)) {
        entities->destroyEntity(entity);  // Invalidates iterator!
    }
}

// CORRECT - collect first
auto enemies = entities->collect<Enemy>();
for (Entity enemy : enemies) {
    if (shouldDestroy(enemy)) {
        entities->destroyEntity(enemy);  // Safe
    }
}
```

### 4. Efficient Iteration

**Use `view()` for read-only iteration:**

```cpp
// Fast - no allocations
for (auto [entity, transform, sprite] :
     entities->view<Transform, Sprite>().each()) {
    // Process
}
```

**Use `collect()` only when necessary:**

```cpp
// Slower - allocates vector
auto enemies = entities->collect<Enemy>();
```

### 5. Component Ownership

**Components should never own entities or other components:**

```cpp
// BAD - circular reference, unclear ownership
struct BadParent {
    std::vector<Entity> children;  // Don't store entities in components
};

// GOOD - use a separate system or relationship manager
struct Parent {
    Entity parent;  // Single reference is OK
};

// In your system:
for (auto [entity, parent] : entities->view<Parent>().each()) {
    if (entities->isValid(parent.parent)) {
        // Use parent entity
    }
}
```

---

## Common Patterns

### Pattern 1: Creating a Player Entity

```cpp
Entity createPlayer(IEntitySystem& entities, float x, float y) {
    Entity player = entities.createEntity();

    // Position and movement
    entities.emplace<Transform>(player, x, y, 0.0f);
    entities.emplace<Velocity>(player, 0.0f, 0.0f);

    // Visual
    entities.emplace<Sprite>(player, "player.png");
    entities.emplace<Animation>(player, "idle");

    // Gameplay
    entities.emplace<Health>(player, 100, 100);
    entities.emplace<PlayerInput>(player);

    // Tags
    entities.emplace<PlayerTag>(player);

    return player;
}
```

### Pattern 2: Querying by Component

```cpp
// Find all enemies in range
std::vector<Entity> findEnemiesInRange(
    IEntitySystem& entities,
    float x, float y,
    float radius
) {
    std::vector<Entity> result;

    for (auto [entity, transform] :
         entities.view<Transform, EnemyTag>().each()) {
        float dx = transform.x - x;
        float dy = transform.y - y;
        float distSq = dx * dx + dy * dy;

        if (distSq <= radius * radius) {
            result.push_back(entity);
        }
    }

    return result;
}
```

### Pattern 3: System Update Pattern

```cpp
class MovementSystem {
public:
    void update(IEntitySystem& entities, DeltaTime dt) {
        // Update all entities with position and velocity
        for (auto [entity, transform, velocity] :
             entities.view<Transform, Velocity>().each()) {
            transform.x += velocity.dx * dt.count();
            transform.y += velocity.dy * dt.count();
        }
    }
};
```

### Pattern 4: Component Communication

```cpp
// PATTERN 1: Direct component access
void applyDamage(IEntitySystem& entities, Entity target, int damage) {
    if (auto* health = entities.tryGet<Health>(target)) {
        health->current -= damage;

        if (health->current <= 0) {
            entities.emplace<DeadTag>(target);
        }
    }
}

// PATTERN 2: Through events (better decoupling)
struct DamageEvent {
    Entity target;
    int damage;
};

// Damage system listens for events
class DamageSystem {
    void onDamageEvent(const DamageEvent& evt, IEntitySystem& entities) {
        if (auto* health = entities.tryGet<Health>(evt.target)) {
            health->current -= evt.damage;

            if (health->current <= 0) {
                entities.emplace<DeadTag>(evt.target);
            }
        }
    }
};
```

### Pattern 5: Singleton Entities

```cpp
// Ensure only one player exists
Entity getOrCreatePlayer(IEntitySystem& entities) {
    // Try to find existing player
    if (auto player = entities.single<PlayerTag>()) {
        return *player;
    }

    // Create new player if none exists
    return createPlayer(entities, 0.0f, 0.0f);
}

// Access singleton player
void updateCamera(IEntitySystem& entities) {
    if (auto player = entities.first<PlayerTag>()) {
        auto& transform = entities.get<Transform>(*player);
        // Focus camera on player
    }
}
```

### Pattern 6: Parent-Child Relationships

```cpp
struct Parent {
    Entity parent;
};

struct Children {
    std::vector<Entity> children;
};

// Create parent-child relationship
void attachChild(IEntitySystem& entities, Entity parent, Entity child) {
    // Add parent reference to child
    entities.emplace<Parent>(child, parent);

    // Add child to parent's list
    auto& children = entities.get<Children>(parent);
    children.children.push_back(child);
}

// Update children based on parent transform
void updateHierarchy(IEntitySystem& entities) {
    for (auto [child, childTransform, parent] :
         entities.view<Transform, Parent>().each()) {

        if (entities.isValid(parent.parent)) {
            auto& parentTransform = entities.get<Transform>(parent.parent);

            // Apply parent's transform to child
            childTransform.x += parentTransform.x;
            childTransform.y += parentTransform.y;
            childTransform.rotation += parentTransform.rotation;
        }
    }
}
```

### Pattern 7: Pooling Enemies

```cpp
// Reuse dead enemies instead of destroying them
struct DeadTag {};
struct ActiveTag {};

Entity spawnEnemy(IEntitySystem& entities, float x, float y) {
    // Try to recycle a dead enemy
    if (auto recycled = entities.first<EnemyTag, DeadTag>()) {
        entities.remove<DeadTag>(*recycled);
        entities.emplace<ActiveTag>(*recycled);

        // Reset position
        auto& transform = entities.get<Transform>(*recycled);
        transform.x = x;
        transform.y = y;

        // Reset health
        auto& health = entities.get<Health>(*recycled);
        health.current = health.maximum;

        return *recycled;
    }

    // Create new enemy if none available
    Entity enemy = entities.createEntity();
    entities.emplace<EnemyTag>(enemy);
    entities.emplace<ActiveTag>(enemy);
    entities.emplace<Transform>(enemy, x, y);
    entities.emplace<Health>(enemy, 50, 50);
    return enemy;
}

void killEnemy(IEntitySystem& entities, Entity enemy) {
    entities.remove<ActiveTag>(enemy);
    entities.emplace<DeadTag>(enemy);
    // Don't destroy - reuse later
}
```

---

## Performance Tips

### 1. Component Storage and Cache Efficiency

**How EnTT stores components:**

Components of the same type are stored in contiguous memory (structure-of-arrays). This means iterating entities with the same components is cache-friendly.

```cpp
// FAST - cache friendly iteration
for (auto [entity, transform, velocity] :
     entities.view<Transform, Velocity>().each()) {
    // All Transform data is in contiguous memory
    // All Velocity data is in contiguous memory
    transform.x += velocity.dx * dt.count();
}

// SLOWER - random memory access
auto allEntities = entities.collect<Transform>();
for (Entity e : allEntities) {
    if (auto* velocity = entities.tryGet<Velocity>(e)) {
        // tryGet() has pointer indirection overhead
    }
}
```

**Tip:** Group related data in the same component for better cache locality.

```cpp
// GOOD - related data together
struct Transform {
    float x, y;
    float rotation;
};

// BAD - split data across components
struct Position { float x, y; };
struct Rotation { float angle; };
// Now you need two memory fetches instead of one
```

### 2. Avoid Frequent Entity Creation/Destruction

**Problem:** Creating/destroying entities has overhead.

**Solution:** Use object pooling with tags (see Pattern 7 above).

```cpp
// Instead of:
entities.destroyEntity(bullet);  // Expensive

// Do this:
entities.emplace<InactiveTag>(bullet);  // Cheap
```

### 3. Use `view()` Instead of `collect()`

**`view()` is a lightweight, non-allocating iterator:**

```cpp
// FAST - no allocation
for (auto entity : entities.view<Enemy>()) {
    // Process
}

// SLOW - allocates vector
auto enemies = entities.collect<Enemy>();
for (Entity e : enemies) {
    // Process
}
```

**Exception:** Use `collect()` when you need to modify entities during iteration.

### 4. Minimize Component Size

**Smaller components = better cache usage:**

```cpp
// GOOD - 12 bytes
struct Transform {
    float x, y, z;
};

// BAD - 256 bytes (cache line pollution)
struct HugeComponent {
    float data[64];
};

// BETTER - store large data separately
struct HugeComponent {
    AssetHandle dataHandle;  // 8 bytes - points to actual data
};
```

### 5. Use Empty Tag Components for States

**Tags have zero overhead (no memory allocated):**

```cpp
// Efficient state checking
if (entities.allOf<InvincibleTag>(player)) {
    // No memory access - just a bitfield check
}

// Less efficient
if (entities.get<PlayerState>(player).isInvincible) {
    // Must fetch entire PlayerState component from memory
}
```

### 6. Avoid `groupCount()` in Hot Paths

**`groupCount()` iterates all matching entities:**

```cpp
// BAD - O(n) every frame
void update() {
    if (entities.groupCount<Enemy>() == 0) {
        levelComplete();
    }
}

// GOOD - check once, cache result
void onEnemyKilled(Entity enemy) {
    if (!entities.hasAny<Enemy>()) {
        levelComplete();
    }
}
```

### 7. Use EnTT Groups for Hot Paths

For frequently iterated combinations, use EnTT groups (cached queries):

```cpp
// Cache the query for repeated use
auto& registry = entities.getRegistry();
auto group = registry.group<Transform>(entt::get<Velocity>);

// Very fast iteration - components already organized
for (auto entity : group) {
    auto& [transform, velocity] = group.get(entity);
    transform.x += velocity.dx * dt.count();
}
```

**When to use groups:**
- Iterating the same component combination every frame
- Performance-critical systems (rendering, physics)

**When NOT to use groups:**
- Rarely-used queries
- Component combinations that change frequently

---

## Code Examples

### Example 1: Complete Gameplay System

```cpp
import bestow;

// Components
struct Transform { float x, y, rotation; };
struct Velocity { float dx, dy; };
struct Health { int current, maximum; };
struct Sprite { std::string texture; };

// Tags
struct PlayerTag { bool _ = false; };
struct EnemyTag { bool _ = false; };
struct DeadTag { bool _ = false; };

// Systems
class GameSystem {
public:
    void update(IEntitySystem& entities, DeltaTime dt) {
        updateMovement(entities, dt);
        checkCollisions(entities);
        removeDeadEntities(entities);
    }

private:
    void updateMovement(IEntitySystem& entities, DeltaTime dt) {
        for (auto [entity, transform, velocity] :
             entities.view<Transform, Velocity>().each()) {
            transform.x += velocity.dx * dt.count();
            transform.y += velocity.dy * dt.count();
        }
    }

    void checkCollisions(IEntitySystem& entities) {
        // Get player
        auto player = entities.first<PlayerTag>();
        if (!player) return;

        auto& playerTransform = entities.get<Transform>(*player);

        // Check collisions with enemies
        for (auto [enemy, enemyTransform] :
             entities.view<Transform, EnemyTag>().each()) {

            float dx = playerTransform.x - enemyTransform.x;
            float dy = playerTransform.y - enemyTransform.y;
            float distSq = dx * dx + dy * dy;

            if (distSq < 32 * 32) {  // Collision radius
                // Damage player
                auto& health = entities.get<Health>(*player);
                health.current -= 10;

                if (health.current <= 0) {
                    entities.emplace<DeadTag>(*player);
                }
            }
        }
    }

    void removeDeadEntities(IEntitySystem& entities) {
        auto dead = entities.collect<DeadTag>();
        for (Entity entity : dead) {
            entities.destroyEntity(entity);
        }
    }
};
```

### Example 2: Spawn System with Pooling

```cpp
class SpawnSystem {
public:
    Entity spawnEnemy(IEntitySystem& entities, float x, float y) {
        Entity enemy = getPooledEnemy(entities);

        // Reset transform
        auto& transform = entities.get<Transform>(enemy);
        transform.x = x;
        transform.y = y;
        transform.rotation = 0.0f;

        // Reset health
        auto& health = entities.get<Health>(enemy);
        health.current = health.maximum;

        // Activate
        if (entities.allOf<InactiveTag>(enemy)) {
            entities.remove<InactiveTag>(enemy);
        }

        return enemy;
    }

    void despawnEnemy(IEntitySystem& entities, Entity enemy) {
        // Don't destroy - just deactivate
        entities.emplace<InactiveTag>(enemy);

        // Stop movement
        auto& velocity = entities.get<Velocity>(enemy);
        velocity.dx = 0.0f;
        velocity.dy = 0.0f;
    }

private:
    Entity getPooledEnemy(IEntitySystem& entities) {
        // Try to reuse inactive enemy
        if (auto recycled = entities.first<EnemyTag, InactiveTag>()) {
            return *recycled;
        }

        // Create new enemy
        Entity enemy = entities.createEntity();
        entities.emplace<EnemyTag>(enemy);
        entities.emplace<Transform>(enemy, 0.0f, 0.0f, 0.0f);
        entities.emplace<Velocity>(enemy, 0.0f, 0.0f);
        entities.emplace<Health>(enemy, 50, 50);
        entities.emplace<Sprite>(enemy, "enemy.png");

        return enemy;
    }
};
```

### Example 3: Camera Follow System

```cpp
class CameraSystem {
public:
    void update(IEntitySystem& entities, DeltaTime dt) {
        // Find player
        auto player = entities.first<PlayerTag>();
        if (!player) return;

        // Find camera
        auto camera = entities.first<CameraTag>();
        if (!camera) return;

        auto& playerTransform = entities.get<Transform>(*player);
        auto& cameraTransform = entities.get<Transform>(*camera);

        // Smooth follow
        const float followSpeed = 5.0f;
        float t = 1.0f - std::exp(-followSpeed * dt.count());

        cameraTransform.x += (playerTransform.x - cameraTransform.x) * t;
        cameraTransform.y += (playerTransform.y - cameraTransform.y) * t;
    }
};
```

### Example 4: Debug Entity Inspector

```cpp
void debugPrintEntity(IEntitySystem& entities, Entity entity) {
    std::cout << "Entity " << static_cast<uint32_t>(entity) << ":\n";

    if (entities.allOf<Transform>(entity)) {
        auto& t = entities.get<Transform>(entity);
        std::cout << "  Transform: (" << t.x << ", " << t.y << ")\n";
    }

    if (entities.allOf<Velocity>(entity)) {
        auto& v = entities.get<Velocity>(entity);
        std::cout << "  Velocity: (" << v.dx << ", " << v.dy << ")\n";
    }

    if (entities.allOf<Health>(entity)) {
        auto& h = entities.get<Health>(entity);
        std::cout << "  Health: " << h.current << "/" << h.maximum << "\n";
    }

    if (entities.allOf<PlayerTag>(entity)) {
        std::cout << "  [Player]\n";
    }

    if (entities.allOf<EnemyTag>(entity)) {
        std::cout << "  [Enemy]\n";
    }
}

void debugPrintAllEntities(IEntitySystem& entities) {
    std::cout << "=== Entity List (" << entities.entityCount() << " total) ===\n";

    entities.each([&](Entity entity) {
        debugPrintEntity(entities, entity);
    });
}
```

---

## Advanced Topics

### Using EnTT Exclusion Filters

```cpp
// Get sprites that are NOT hidden
auto& registry = entities.getRegistry();
for (auto entity : registry.view<Sprite>(entt::exclude<HiddenTag>)) {
    auto& sprite = registry.get<Sprite>(entity);
    // Render sprite
}
```

### Using EnTT Storage Iteration

```cpp
// Iterate ONLY Transform components (no entity access)
auto& registry = entities.getRegistry();
for (auto& transform : registry.storage<Transform>()) {
    transform.x += 1.0f;  // Bulk update
}
```

### Custom Component Pools

EnTT uses a default packed array storage, but you can customize per-component:

```cpp
// In your initialization code
auto& registry = entities.getRegistry();

// Use stable storage (entities don't move in memory)
registry.storage<Transform>().reserve(1000);
```

---

## Troubleshooting

### "Component not found" exception

```cpp
// BAD - throws if component missing
auto& health = entities.get<Health>(enemy);

// GOOD - safe check
if (auto* health = entities.tryGet<Health>(enemy)) {
    health->current -= 10;
}
```

### Iterator invalidation during entity destruction

```cpp
// BAD - destroys while iterating
for (auto entity : entities.view<Enemy>()) {
    entities.destroyEntity(entity);  // Crashes!
}

// GOOD - collect first
auto enemies = entities.collect<Enemy>();
for (Entity enemy : enemies) {
    entities.destroyEntity(enemy);
}
```

### Stale entity handles

```cpp
// BAD - entity might be destroyed
class System {
    Entity cachedEntity_;

    void update() {
        auto& health = entities.get<Health>(cachedEntity_);  // Crash if destroyed!
    }
};

// GOOD - always validate
class System {
    Entity cachedEntity_;

    void update() {
        if (entities.isValid(cachedEntity_)) {
            auto& health = entities.get<Health>(cachedEntity_);
        }
    }
};
```

---

## Summary

The Entity System provides a fast, flexible foundation for game development using the ECS pattern:

- **Entities** are unique IDs that bind components
- **Components** are pure data structures
- **Systems** contain the logic that operates on entities with specific components
- Use `view()` for fast iteration
- Use tags for entity states
- Keep components small and focused
- Always validate entity handles before use

For more information:
- **EnTT Documentation**: https://github.com/skypjack/entt/wiki
- **Bestow Examples**: `/examples/` directory in the repository

---

**File:** `/Users/jaaaacob/Documents/GameDev/jframe/games/game1/engine-docs/ENTITY-SYSTEM.md`

**Interface:** `/Users/jaaaacob/Documents/GameDev/jframe/bestow-contract/src/bestow.entity.cppm`

**Implementation:** `/Users/jaaaacob/Documents/GameDev/jframe/bestow-entity/src/bestow.entity.impl.cppm`

**Tests:** `/Users/jaaaacob/Documents/GameDev/jframe/tests/unit/EntitySystemTests.cpp`
