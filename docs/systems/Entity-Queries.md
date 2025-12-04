# Bestow Entity Query System

## Overview

The Entity Query System extends Bestow's Entity System with automatic entity tracking by component type. This eliminates the need for games to manually maintain `std::vector<Entity>` collections for each entity type.

**Module:** `bestow.entity` (extended interface)
**Implementation:** `bestow-entity/`
**Status:** Planned Enhancement

## Problem Statement

Currently, games must manually track entities by type:

```cpp
// Current approach - manual collection management
class Game {
    std::vector<Entity> platforms_;
    std::vector<Entity> enemies_;
    std::vector<Entity> collectables_;
    std::vector<Entity> projectiles_;
    // ... 12+ more collections

    void createEnemy(float x, float y) {
        Entity e = entities->createEntity();
        entities->emplace<EnemyTag>(e);
        enemies_.push_back(e);  // Must remember to add
    }

    void destroyEnemy(Entity e) {
        entities->destroyEntity(e);
        // Must remember to remove from collection
        enemies_.erase(std::remove(enemies_.begin(), enemies_.end(), e), enemies_.end());
    }
};
```

This leads to:
- Boilerplate code in every game
- Bug-prone manual synchronization
- Memory overhead from duplicate tracking
- Forgetting to remove destroyed entities

## Proposed Solution

### Automatic Entity Groups

Entity groups automatically track all entities with a specific component:

```cpp
// New approach - automatic tracking
void Game::update(DeltaTime dt) {
    // Get all entities with EnemyTag - automatically maintained
    for (Entity enemy : entities->group<EnemyTag>()) {
        updateEnemy(enemy, dt);
    }

    // Get all entities with multiple components
    for (Entity projectile : entities->group<Projectile, Transform2D>()) {
        updateProjectile(projectile, dt);
    }
}

void Game::createEnemy(float x, float y) {
    Entity e = entities->createEntity();
    entities->emplace<EnemyTag>(e);
    // Automatically added to EnemyTag group
}

void Game::destroyEnemy(Entity e) {
    entities->destroyEntity(e);
    // Automatically removed from all groups
}
```

### Interface Extension

```cpp
class IEntitySystem {
    // ... existing methods ...

    // Entity Groups - automatic tracking by component
    template<typename... Components>
    EntityGroup group();

    template<typename... Components>
    const EntityGroup group() const;

    // Group with exclusions
    template<typename... Include, typename... Exclude>
    EntityGroup group(Exclude<Exclude...>);

    // Count entities in group
    template<typename... Components>
    std::size_t groupCount() const;

    // Check if any entities match
    template<typename... Components>
    bool hasAny() const;

    // Get first matching entity (or null)
    template<typename... Components>
    std::optional<Entity> first() const;

    // Get single entity (throws if count != 1)
    template<typename... Components>
    Entity single() const;
};

// EntityGroup is a lightweight wrapper over EnTT view
class EntityGroup {
public:
    auto begin() const;
    auto end() const;
    std::size_t size() const;
    bool empty() const;

    // Collect to vector (for modification during iteration)
    std::vector<Entity> collect() const;

    // Iterate with components unpacked
    template<typename Func>
    void each(Func&& func) const;
};
```

## Usage Examples

### Basic Group Iteration

```cpp
// Iterate all enemies
for (Entity enemy : entities->group<EnemyTag>()) {
    auto& health = entities->get<Health>(enemy);
    if (health.current <= 0) {
        toDestroy.push_back(enemy);
    }
}

// Iterate with component access
entities->group<Position, Velocity>().each([&](Entity e, Position& pos, Velocity& vel) {
    pos.x += vel.dx * dt;
    pos.y += vel.dy * dt;
});
```

### Filtering Groups

```cpp
// Get dynamic bodies only (has Velocity, excludes StaticTag)
for (Entity e : entities->group<Position, Velocity>(Exclude<StaticTag>{})) {
    // Process dynamic entities
}

// Get living enemies
for (Entity e : entities->group<EnemyTag, Health>(Exclude<DeadTag>{})) {
    updateEnemyAI(e);
}
```

### Safe Iteration with Modification

```cpp
// Collect first when destroying during iteration
auto enemies = entities->group<EnemyTag>().collect();
for (Entity enemy : enemies) {
    if (shouldDestroy(enemy)) {
        entities->destroyEntity(enemy);  // Safe - iterating copy
    }
}
```

### Singleton Queries

```cpp
// Get the player (exactly one expected)
Entity player = entities->single<PlayerTag>();

// Get first matching or none
if (auto boss = entities->first<BossTag>()) {
    updateBoss(*boss);
}

// Check existence
if (entities->hasAny<ActivePowerupTag>()) {
    showPowerupUI();
}
```

### Group Count

```cpp
// Display enemy count
int enemyCount = entities->groupCount<EnemyTag>();
ui->drawText("Enemies: " + std::to_string(enemyCount));

// Check completion
if (entities->groupCount<EnemyTag>() == 0) {
    completeLevel();
}
```

## Implementation Plan

### Phase 1: Core Group API

**Files to modify:**
- `bestow-contract/src/bestow.entity.cppm` - Add group methods to interface
- `bestow-entity/src/EntitySystem.cpp` - Implement using EnTT views

**Implementation:**

```cpp
// In IEntitySystem
template<typename... Components>
auto group() {
    return EntityGroup<Components...>{getRegistry().view<Components...>()};
}

template<typename... Components>
std::size_t groupCount() const {
    return getRegistry().view<Components...>().size();
}

template<typename... Components>
bool hasAny() const {
    return !getRegistry().view<Components...>().empty();
}

template<typename... Components>
std::optional<Entity> first() const {
    auto view = getRegistry().view<Components...>();
    if (view.empty()) return std::nullopt;
    return *view.begin();
}
```

### Phase 2: EntityGroup Wrapper

**Files to create:**
- `bestow-entity/src/EntityGroup.hpp` - Group wrapper class

```cpp
template<typename... Components>
class EntityGroup {
    entt::view<Components...> view_;
public:
    explicit EntityGroup(entt::view<Components...> view) : view_(view) {}

    auto begin() const { return view_.begin(); }
    auto end() const { return view_.end(); }
    std::size_t size() const { return view_.size_hint(); }
    bool empty() const { return view_.empty(); }

    std::vector<Entity> collect() const {
        return {view_.begin(), view_.end()};
    }

    template<typename Func>
    void each(Func&& func) const {
        for (auto [entity, comps...] : view_.each()) {
            func(entity, comps...);
        }
    }
};
```

### Phase 3: Exclusion Support

```cpp
template<typename... Ts>
struct Exclude {};

template<typename... Include, typename... Excluded>
auto group(Exclude<Excluded...>) {
    return EntityGroup{getRegistry().view<Include...>(entt::exclude<Excluded...>)};
}
```

### Phase 4: Tests

**File:** `tests/unit/EntityGroupTests.cpp`

```cpp
TEST(EntityGroupTest, BasicIteration) {
    auto entities = createEntitySystem();

    Entity e1 = entities->createEntity();
    Entity e2 = entities->createEntity();
    entities->emplace<EnemyTag>(e1);
    entities->emplace<EnemyTag>(e2);

    int count = 0;
    for (Entity e : entities->group<EnemyTag>()) {
        count++;
    }
    EXPECT_EQ(count, 2);
}

TEST(EntityGroupTest, GroupCount) {
    auto entities = createEntitySystem();

    EXPECT_EQ(entities->groupCount<EnemyTag>(), 0);

    Entity e = entities->createEntity();
    entities->emplace<EnemyTag>(e);

    EXPECT_EQ(entities->groupCount<EnemyTag>(), 1);
}

TEST(EntityGroupTest, FirstAndSingle) {
    auto entities = createEntitySystem();

    EXPECT_FALSE(entities->first<PlayerTag>().has_value());

    Entity player = entities->createEntity();
    entities->emplace<PlayerTag>(player);

    EXPECT_EQ(entities->first<PlayerTag>(), player);
    EXPECT_EQ(entities->single<PlayerTag>(), player);
}

TEST(EntityGroupTest, ExcludeFilter) {
    auto entities = createEntitySystem();

    Entity alive = entities->createEntity();
    Entity dead = entities->createEntity();
    entities->emplace<EnemyTag>(alive);
    entities->emplace<EnemyTag>(dead);
    entities->emplace<DeadTag>(dead);

    auto livingEnemies = entities->group<EnemyTag>(Exclude<DeadTag>{});
    EXPECT_EQ(livingEnemies.size(), 1);
}

TEST(EntityGroupTest, CollectForSafeIteration) {
    auto entities = createEntitySystem();

    for (int i = 0; i < 5; i++) {
        Entity e = entities->createEntity();
        entities->emplace<EnemyTag>(e);
    }

    // Safe to destroy during iteration over copy
    auto enemies = entities->group<EnemyTag>().collect();
    for (Entity e : enemies) {
        entities->destroyEntity(e);
    }

    EXPECT_EQ(entities->groupCount<EnemyTag>(), 0);
}
```

## Migration Guide

### Before (Manual Tracking)

```cpp
class Game {
    std::vector<Entity> enemies_;

    void createEnemy(float x, float y) {
        Entity e = entities->createEntity();
        entities->emplace<EnemyTag>(e);
        enemies_.push_back(e);
    }

    void update(DeltaTime dt) {
        for (Entity e : enemies_) {
            updateEnemy(e, dt);
        }
    }

    void destroyEnemy(Entity e) {
        enemies_.erase(std::remove(enemies_.begin(), enemies_.end(), e), enemies_.end());
        entities->destroyEntity(e);
    }
};
```

### After (Automatic Groups)

```cpp
class Game {
    // No collection needed!

    void createEnemy(float x, float y) {
        Entity e = entities->createEntity();
        entities->emplace<EnemyTag>(e);
        // Automatically tracked
    }

    void update(DeltaTime dt) {
        for (Entity e : entities->group<EnemyTag>()) {
            updateEnemy(e, dt);
        }
    }

    void destroyEnemy(Entity e) {
        entities->destroyEntity(e);
        // Automatically removed from groups
    }
};
```

## Performance Considerations

### Memory
- Groups are views, not copies - no additional memory per entity
- `collect()` creates a temporary vector - use only when needed

### Speed
- Group iteration is O(n) where n = matching entities
- Same performance as EnTT views (zero overhead)
- `groupCount()` is O(1) for single-component groups

### Best Practices
1. Prefer group iteration over manual collections
2. Use `collect()` only when modifying entities during iteration
3. Use `first()` for singleton entities instead of storing references
4. Use tag components liberally - they're just one bit per entity

## Related Documentation

- [Entity System](Entity-System.md) - Core entity management
- [Components](../bestow-components/README.md) - Pre-built components
- [Technical Design](../bestow-technical-design.md) - Architecture overview

## Status

| Task | Status |
|------|--------|
| Interface design | ✅ Complete |
| Core implementation | ✅ Complete |
| Exclusion support | ✅ Complete |
| Tests | 🔲 Planned |
| Documentation | ✅ Complete |

## Implementation Notes

The Entity Query System was implemented directly in `bestow-contract/src/bestow.entity.cppm` using EnTT views. The following methods are available:

- `groupCount<Components...>()` - Count entities with components
- `hasAny<Components...>()` - Check if any entities exist with components
- `first<Components...>()` - Get first matching entity or nullopt
- `single<Components...>()` - Get single entity (returns nullopt if 0 or >1)
- `collect<Components...>()` - Collect all matching entities to a vector
- `collectExcluding<Include..., Exclude...>()` - Collect with exclusion filter

These are template methods that leverage EnTT's view system for efficient querying without additional memory overhead.
