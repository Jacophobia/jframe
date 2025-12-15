# Bestow Entity System Guide

The Entity System is Bestow's implementation of the Entity-Component-System (ECS) architecture pattern, powered by EnTT 3.x. It provides high-performance entity and component management for game development.

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

### Technology Stack

- **EnTT 3.x** - Modern C++ ECS library by Michele Caini
- **Entity Type**: `entt::entity` (aliased as `bestow::Entity`)
- **Storage**: Sparse set with packed arrays for cache-friendly iteration

### Key Implementation Notes

Bestow's `IEntitySystem` is a thin wrapper around EnTT 3.x. Understanding these implementation details will help you use the system effectively:

1. **Entity is `entt::entity`** - The `Entity` type is aliased directly to `entt::entity` (typically a 32-bit unsigned integer with version bits for recycling)

2. **Iteration pattern** - `view<T...>()` returns an EnTT view for range-based for loops:
   ```cpp
   for (auto entity : entities->view<Transform>()) {
       auto& transform = entities->get<Transform>(entity);  // Use get() to access components
   }
   ```

3. **`get()` panics, not throws** - `get<T>(entity)` will panic (assertion failure in debug, undefined behavior in release) if the component doesn't exist. Use `tryGet<T>(entity)` when uncertain.

4. **Direct registry access for advanced features** - Use `getRegistry()` to access EnTT-specific features like `.each()` unpacking, groups, or custom storage configurations

---

## Quick Reference

### Common Operations Cheat Sheet

```cpp
// Create and destroy
Entity e = entities->createEntity();
entities->destroyEntity(e);
bool exists = entities->isValid(e);
size_t total = entities->entityCount();

// Add/remove components
entities->emplace<Health>(e, 100, 100);
entities->remove<Health>(e);

// Check components
bool has = entities->allOf<Health>(e);
bool hasAny = entities->anyOf<Health, Shield>(e);

// Access components (SAFE - returns nullptr if missing)
if (auto* health = entities->tryGet<Health>(e)) {
    health->current -= 10;
}

// Access components (FAST - panics if missing, use when certain)
auto& transform = entities->get<Transform>(e);

// Iterate entities with components
for (auto entity : entities->view<Transform, Velocity>()) {
    auto& t = entities->get<Transform>(entity);
    auto& v = entities->get<Velocity>(entity);
    // Update...
}

// Find specific entities
auto player = entities->first<PlayerTag>();        // First match
auto boss = entities->single<BossTag>();          // Only if exactly one exists
auto enemies = entities->collect<EnemyTag>();     // All matches (allocates vector)

// Count
size_t count = entities->groupCount<Enemy>();
bool anyEnemies = entities->hasAny<Enemy>();

// Advanced: Direct EnTT access
auto& registry = entities->getRegistry();
for (auto [entity, t, v] : registry.view<Transform, Velocity>().each()) {
    // Components unpacked directly - no get() needed
}
```

---

## Core Concepts

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

### Entity

An **Entity** is just a unique identifier (internally `entt::entity`, a 32-bit integer with version bits for safe recycling). It has no behavior or data itself - it's a handle that binds components together.

```cpp
Entity player = entities->createEntity();
Entity enemy = entities->createEntity();
```

**Entity Recycling:** When you destroy an entity, EnTT recycles the ID by incrementing a version counter. This prevents stale handles from accidentally accessing new entities.

---

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

**Tag Components:** Use empty structs for categorization (e.g., `PlayerTag`, `EnemyTag`). These have zero memory overhead.

```cpp
struct PlayerTag {};
struct EnemyTag {};
```

---

### System

A **System** contains the logic that operates on entities with specific components. In Bestow, systems are typically functions or classes that query entities and process their components.

```cpp
// System logic - operates on entities with Transform and Velocity
void updateMovement(IEntitySystem& entities, DeltaTime dt) {
    for (auto entity : entities.view<Transform, Velocity>()) {
        auto& transform = entities.get<Transform>(entity);
        auto& velocity = entities.get<Velocity>(entity);

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

**Returns:** A valid entity handle.

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

**Note:** This iterates all entities to count valid ones - O(n) complexity.

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

**Panics:** If the entity doesn't have the component (assertion failure in debug, undefined behavior in release).

**Example:**
```cpp
// Modify component
auto& health = entities->get<Health>(player);
health.current -= 10;

// Read component
const auto& transform = entities->get<Transform>(player);
float x = transform.x;
```

**Important:** Only use `get()` when you're **certain** the entity has the component. Otherwise, use `tryGet()` for safety.

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
```

**Performance:** Views are extremely fast - they don't allocate memory or copy entities.

**Advanced:** For direct component unpacking without `get()` calls, use `getRegistry()` and EnTT's `.each()`:

```cpp
auto& registry = entities->getRegistry();
for (auto [entity, transform, velocity] : registry.view<Transform, Velocity>().each()) {
    // Components unpacked directly - no get() calls needed
    transform.x += velocity.dx * dt;
    transform.y += velocity.dy * dt;
}
```

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

**Performance:** Early-exit iteration - stops as soon as first entity is found.

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

**Implementation Note:** Uses `view.size()` for O(1) check when possible.

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

**Note:** The first template parameter is the required component, all subsequent parameters are excluded components.

---

#### `query(selector)`

Type-erased query with predicate support.

```cpp
std::vector<Entity> query(const EntitySelector& selector) const;
```

**Example:**
```cpp
EntitySelector selector{
    .requiredComponents = {},
    .excludedComponents = {},
    .predicate = [](Entity e) {
        // Custom filter logic
        return true;
    }
};
auto result = entities->query(selector);
```

**Use case:** Dynamic queries where component types aren't known at compile-time.

**Note:** The type-erased component filtering (`requiredComponents`, `excludedComponents`) is not fully implemented in the current version. Use the predicate for custom filtering.

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

// Use EnTT's .each() for unpacked iteration (faster than view + get)
for (auto [entity, transform, velocity] :
     registry.view<Transform, Velocity>().each()) {
    // Components directly accessible, no get() calls needed
    transform.x += velocity.dx * dt;
    transform.y += velocity.dy * dt;
}

// Use EnTT groups (cached multi-component queries)
auto group = registry.group<Transform>(entt::get<Velocity>);
for (auto entity : group) {
    auto [transform, velocity] = group.get<Transform, Velocity>(entity);
    // Process
}

// Use EnTT exclusion filters
for (auto entity : registry.view<Sprite>(entt::exclude<Hidden>)) {
    // Only visible sprites
}
```

**Use case:** When you need EnTT-specific features not exposed by IEntitySystem.

---

### Iteration Helper

#### `each(callback)`

Iterate all valid entities with a callback.

```cpp
void each(std::function<void(Entity)> callback);
```

**Example:**
```cpp
entities->each([](Entity e) {
    std::cout << "Entity: " << static_cast<uint32_t>(e) << std::endl;
});
```

**Note:** This iterates ALL entities regardless of components. For component-filtered iteration, use `view()`.

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

---

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

---

### 3. Entity Lifecycle Management

**Always check validity before using stored entity handles:**

```cpp
class GameSystem {
    Entity cachedPlayer_;

    void update(IEntitySystem& entities) {
        // Entity might have been destroyed since last frame
        if (entities.isValid(cachedPlayer_)) {
            auto& transform = entities.get<Transform>(cachedPlayer_);
            // Safe to use
        } else {
            // Find player again
            if (auto player = entities.first<Player>()) {
                cachedPlayer_ = *player;
            }
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

---

### 4. Efficient Iteration

**Use `view()` for read-only iteration:**

```cpp
// Fast - no allocations
for (auto entity : entities->view<Transform, Sprite>()) {
    // Process
}
```

**Use direct registry access for unpacked iteration:**

```cpp
// Fastest - components unpacked directly, no get() calls
auto& registry = entities->getRegistry();
for (auto [entity, transform, sprite] : registry.view<Transform, Sprite>().each()) {
    // Process
}
```

**Use `collect()` only when necessary:**

```cpp
// Slower - allocates vector
auto enemies = entities->collect<Enemy>();
```

---

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
for (auto [entity, parent] : entities->getRegistry().view<Parent>().each()) {
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

---

### Pattern 2: Querying by Component

```cpp
// Find all enemies in range
std::vector<Entity> findEnemiesInRange(
    IEntitySystem& entities,
    float x, float y,
    float radius
) {
    std::vector<Entity> result;

    for (auto entity : entities.view<Transform, EnemyTag>()) {
        auto& transform = entities.get<Transform>(entity);

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

---

### Pattern 3: System Update Pattern

```cpp
class MovementSystem {
public:
    void update(IEntitySystem& entities, DeltaTime dt) {
        // Update all entities with position and velocity
        for (auto entity : entities.view<Transform, Velocity>()) {
            auto& transform = entities.get<Transform>(entity);
            auto& velocity = entities.get<Velocity>(entity);

            transform.x += velocity.dx * dt.count();
            transform.y += velocity.dy * dt.count();
        }
    }
};
```

---

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

---

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

---

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
    for (auto child : entities.view<Transform, Parent>()) {
        auto& childTransform = entities.get<Transform>(child);
        auto& parent = entities.get<Parent>(child);

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

---

### Pattern 7: Object Pooling

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
for (auto entity : entities.view<Transform, Velocity>()) {
    // All Transform data is in contiguous memory
    // All Velocity data is in contiguous memory
    auto& transform = entities.get<Transform>(entity);
    auto& velocity = entities.get<Velocity>(entity);
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

---

### 2. Avoid Frequent Entity Creation/Destruction

**Problem:** Creating/destroying entities has overhead.

**Solution:** Use object pooling with tags (see Pattern 7 above).

```cpp
// Instead of:
entities.destroyEntity(bullet);  // Expensive

// Do this:
entities.emplace<InactiveTag>(bullet);  // Cheap
```

---

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

---

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

---

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

---

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

---

### 7. Use EnTT's `.each()` for Unpacked Iteration

For frequently iterated combinations, use direct registry access with `.each()`:

```cpp
// Faster - components unpacked directly, no get() calls
auto& registry = entities.getRegistry();
for (auto [entity, transform, velocity] : registry.view<Transform, Velocity>().each()) {
    transform.x += velocity.dx * dt.count();
}

// Slower - requires get() calls
for (auto entity : entities.view<Transform, Velocity>()) {
    auto& transform = entities.get<Transform>(entity);
    auto& velocity = entities.get<Velocity>(entity);
    transform.x += velocity.dx * dt.count();
}
```

---

### 8. Use EnTT Groups for Performance-Critical Paths

For very hot paths, use EnTT groups (cached queries):

```cpp
// Cache the query for repeated use
auto& registry = entities.getRegistry();
auto group = registry.group<Transform>(entt::get<Velocity>);

// Very fast iteration - components already organized
for (auto entity : group) {
    auto [transform, velocity] = group.get<Transform, Velocity>(entity);
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
struct PlayerTag {};
struct EnemyTag {};
struct DeadTag {};

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
        for (auto entity : entities.view<Transform, Velocity>()) {
            auto& transform = entities.get<Transform>(entity);
            auto& velocity = entities.get<Velocity>(entity);

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
        for (auto enemy : entities.view<Transform, EnemyTag>()) {
            auto& enemyTransform = entities.get<Transform>(enemy);

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

---

### Example 2: Camera Follow System

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

---

### Example 3: Debug Entity Inspector

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

---

### Using EnTT Storage Iteration

```cpp
// Iterate ONLY Transform components (no entity access)
auto& registry = entities.getRegistry();
auto* storage = registry.storage<Transform>();
if (storage) {
    for (auto& transform : *storage) {
        transform.x += 1.0f;  // Bulk update
    }
}
```

---

### Custom Component Pools

EnTT uses a default sparse set storage, but you can customize per-component:

```cpp
// In your initialization code
auto& registry = entities.getRegistry();

// Reserve capacity for known entity count
registry.reserve<Transform>(1000);
registry.reserve<Velocity>(1000);
```

---

## Troubleshooting

### "Component not found" panic/crash

```cpp
// BAD - panics/crashes if component missing
auto& health = entities.get<Health>(enemy);

// GOOD - safe check
if (auto* health = entities.tryGet<Health>(enemy)) {
    health->current -= 10;
}

// ALSO GOOD - when you're certain the component exists
if (entities.allOf<Health>(enemy)) {
    auto& health = entities.get<Health>(enemy);  // Safe - we checked first
    health.current -= 10;
}
```

---

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

---

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

    void update(IEntitySystem& entities) {
        if (entities.isValid(cachedEntity_)) {
            auto& health = entities.get<Health>(cachedEntity_);
        }
    }
};
```

---

## Summary

The Entity System provides a fast, flexible foundation for game development using the ECS pattern:

- **Entities** are unique IDs (`entt::entity`) that bind components together
- **Components** are pure data structures (no logic)
- **Systems** contain the logic that operates on entities with specific components
- Use `view<T...>()` for fast, non-allocating iteration
- Use `collect<T...>()` when you need to modify entities during iteration
- Use `tryGet<T>()` for safe component access, `get<T>()` when certain component exists
- Use `first<T...>()` and `single<T...>()` to find singleton entities
- Use tags (empty structs) for entity states and categorization
- Keep components small and focused for better cache performance
- Always validate cached entity handles with `isValid()` before use
- Use `getRegistry()` for direct EnTT access when you need advanced features

**Key Patterns:**
- `for (auto e : entities->view<T>()) { auto& c = entities->get<T>(e); }` - Standard iteration
- `for (auto [e, c] : registry.view<T>().each()) { }` - Unpacked iteration (faster)
- `auto opt = entities->first<Player>();` - Find singleton
- `if (auto* c = entities->tryGet<T>(e)) { }` - Safe component access
- `auto all = entities->collect<Enemy>();` - Collect for modification during iteration

For more information:
- **EnTT Documentation**: https://github.com/skypjack/entt/wiki
- **Bestow Examples**: `/examples/` directory in the repository

---

**File:** `/Users/jaaaacob/Documents/GameDev/jframe/games/game1/engine-docs/ENTITY-SYSTEM.md`

**Interface:** `/Users/jaaaacob/Documents/GameDev/jframe/bestow-contract/src/bestow.entity.cppm`

**Implementation:** `/Users/jaaaacob/Documents/GameDev/jframe/bestow-entity/src/bestow.entity.impl.cppm`

**Tests:** `/Users/jaaaacob/Documents/GameDev/jframe/tests/unit/EntitySystemTests.cpp`
