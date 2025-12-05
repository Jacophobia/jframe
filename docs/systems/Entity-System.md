# Bestow Entity System

## Overview

The Entity System is Bestow's implementation of the Entity-Component-System (ECS) architectural pattern, built on top of the powerful [EnTT library](https://github.com/skypjack/entt). The ECS pattern separates game objects into three fundamental concepts:

- **Entities**: Lightweight identifiers (essentially just numbers) that represent game objects
- **Components**: Pure data structures that hold state (position, health, sprite, etc.)
- **Systems**: Logic that operates on entities with specific component combinations

This architecture provides:
- High performance through cache-friendly data layouts
- Composition-based design instead of inheritance hierarchies
- Flexible runtime composition of behaviors
- Efficient iteration over entities with specific component sets

### Key Features

- Fast entity creation and destruction
- Type-safe component management via templates
- Efficient queries and iteration using EnTT views
- Direct registry access for advanced patterns
- Zero-overhead abstraction over EnTT

## Core Concepts

### Entity

An `Entity` is an opaque identifier (typedef of `entt::entity`) that uniquely represents a game object. It has no behavior or data on its own - it's simply a handle used to associate components together.

```cpp
import bestow;
import bestow.entity;

// Create an entity
Entity player = entitySystem->createEntity();

// Check if entity is valid
if (entitySystem->isValid(player)) {
    // Entity exists and can be used
}

// Destroy when done
entitySystem->destroyEntity(player);
```

### Components

Components are plain data structures (POD types recommended) that store state. Define them as simple structs:

```cpp
struct Position {
    float x;
    float y;
};

struct Velocity {
    float dx;
    float dy;
};

struct Health {
    int current;
    int maximum;
};

struct PlayerTag {};  // Empty component used as a marker
```

### Systems

While the Entity System manages entities and components, your game logic lives in system functions that operate on entities with specific components. These are typically implemented as regular functions or classes in your game code.

## Basic Usage

### Creating Entities

```cpp
import bestow.entity;
import bestow.entity.impl;

// Create the entity system
auto entitySystem = bestow::createEntitySystem();

// Create a simple entity
Entity enemy = entitySystem->createEntity();

// Create multiple entities
std::vector<Entity> platforms;
for (int i = 0; i < 10; ++i) {
    platforms.push_back(entitySystem->createEntity());
}

// Check entity count
std::size_t count = entitySystem->entityCount();
```

### Adding Components

Use the `emplace<T>()` method to add components to entities. It constructs the component in-place and returns a reference:

```cpp
// Add components with constructor arguments
Entity player = entitySystem->createEntity();
auto& pos = entitySystem->emplace<Position>(player, 100.0f, 200.0f);
auto& vel = entitySystem->emplace<Velocity>(player, 0.0f, 0.0f);
auto& health = entitySystem->emplace<Health>(player, 100, 100);

// Add a tag component (no data)
entitySystem->emplace<PlayerTag>(player);

// Using Transform2D from bestow.types
entitySystem->emplace<Transform2D>(player, Transform2D{
    .x = 100.0f,
    .y = 200.0f,
    .rotation = 0.0f,
    .scaleX = 1.0f,
    .scaleY = 1.0f
});
```

### Accessing Components

#### Safe Access - get()

Use `get<T>()` when you know the entity has the component. Throws if the component doesn't exist:

```cpp
// Get component reference
Position& pos = entitySystem->get<Position>(player);
pos.x += 10.0f;

// Get const reference
const Health& health = std::as_const(*entitySystem).get<Health>(player);
std::cout << "Health: " << health.current << "/" << health.maximum << "\n";
```

#### Checked Access - tryGet()

Use `tryGet<T>()` for safe, pointer-based access that returns nullptr if the component doesn't exist:

```cpp
// Returns pointer or nullptr
if (Position* pos = entitySystem->tryGet<Position>(entity)) {
    pos->x += 10.0f;
} else {
    // Entity doesn't have a Position component
}

// Const version
if (const Health* health = std::as_const(*entitySystem).tryGet<Health>(entity)) {
    std::cout << "Health: " << health->current << "\n";
}
```

### Checking for Components

```cpp
// Check if entity has a specific component
if (entitySystem->allOf<Position>(player)) {
    // Player has Position
}

// Check if entity has all of multiple components
if (entitySystem->allOf<Position, Velocity, Health>(player)) {
    // Player has all three components
}

// Check if entity has any of multiple components
if (entitySystem->anyOf<PlayerTag, EnemyTag>(entity)) {
    // Entity has at least one tag
}
```

### Removing Components

```cpp
// Remove a single component
entitySystem->remove<Velocity>(player);

// Remove multiple components
entitySystem->remove<Velocity>(player);
entitySystem->remove<Health>(player);
```

### Destroying Entities

```cpp
// Destroy an entity and all its components
entitySystem->destroyEntity(player);

// Entity is now invalid
bool valid = entitySystem->isValid(player);  // false
```

## Iteration and Queries

### Views - Type-Safe Iteration

Views provide efficient, type-safe iteration over entities with specific components. This is the primary way to implement game systems:

```cpp
// Iterate over entities with Position component
for (auto entity : entitySystem->view<Position>()) {
    Position& pos = entitySystem->get<Position>(entity);
    std::cout << "Entity at " << pos.x << ", " << pos.y << "\n";
}

// Iterate over entities with multiple components
for (auto entity : entitySystem->view<Position, Velocity>()) {
    Position& pos = entitySystem->get<Position>(entity);
    Velocity& vel = entitySystem->get<Velocity>(entity);

    // Update position based on velocity
    pos.x += vel.dx;
    pos.y += vel.dy;
}

// Get components directly from the view (more efficient)
auto view = entitySystem->view<Position, Velocity>();
for (auto [entity, pos, vel] : view.each()) {
    pos.x += vel.dx * deltaTime;
    pos.y += vel.dy * deltaTime;
}
```

### Const Views

Use const views when you only need read access:

```cpp
const IEntitySystem& constSystem = *entitySystem;

for (auto entity : constSystem.view<const Position, const Health>()) {
    const Position& pos = constSystem.get<Position>(entity);
    const Health& health = constSystem.get<Health>(entity);

    // Read-only access
    renderHealthBar(pos, health);
}
```

### Filtering Views

Views can exclude entities with specific components:

```cpp
// Get entities with Position but without Velocity (static objects)
auto view = entitySystem->view<Position>(entt::exclude<Velocity>);
for (auto entity : view) {
    // Process static objects
}
```

### Iterating All Entities

Use `each()` to iterate over all entities regardless of components:

```cpp
entitySystem->each([](Entity entity) {
    std::cout << "Processing entity " << static_cast<uint32_t>(entity) << "\n";
});
```

## Common Patterns

### Physics System Example

```cpp
void updatePhysics(IEntitySystem& entities, float deltaTime) {
    // Apply velocity to position
    auto movableView = entities.view<Position, Velocity>();
    for (auto [entity, pos, vel] : movableView.each()) {
        pos.x += vel.dx * deltaTime;
        pos.y += vel.dy * deltaTime;
    }

    // Apply gravity to dynamic objects
    auto dynamicView = entities.view<Velocity, DynamicTag>();
    for (auto [entity, vel] : dynamicView.each()) {
        vel.dy += 980.0f * deltaTime;  // Gravity
    }
}
```

### Rendering System Example

```cpp
void renderSprites(IEntitySystem& entities, IGraphicsSystem& graphics) {
    // Render entities with sprite and transform
    auto view = entities.view<const Sprite, const Transform2D>();

    // Process in render layer order
    std::vector<Entity> sortedEntities(view.begin(), view.end());
    std::sort(sortedEntities.begin(), sortedEntities.end(),
        [&](Entity a, Entity b) {
            const Sprite& spriteA = entities.get<Sprite>(a);
            const Sprite& spriteB = entities.get<Sprite>(b);
            return spriteA.layer < spriteB.layer;
        });

    for (Entity entity : sortedEntities) {
        const Sprite& sprite = entities.get<Sprite>(entity);
        const Transform2D& transform = entities.get<Transform2D>(entity);
        graphics.drawSprite(sprite, transform);
    }
}
```

### Collision System Example

```cpp
void checkCollisions(IEntitySystem& entities) {
    auto view = entities.view<Position, CollisionBox>();

    std::vector<Entity> colliders(view.begin(), view.end());

    for (size_t i = 0; i < colliders.size(); ++i) {
        for (size_t j = i + 1; j < colliders.size(); ++j) {
            Entity a = colliders[i];
            Entity b = colliders[j];

            const Position& posA = entities.get<Position>(a);
            const Position& posB = entities.get<Position>(b);
            const CollisionBox& boxA = entities.get<CollisionBox>(a);
            const CollisionBox& boxB = entities.get<CollisionBox>(b);

            if (checkAABBCollision(posA, boxA, posB, boxB)) {
                handleCollision(entities, a, b);
            }
        }
    }
}
```

### Player Creation Example

```cpp
Entity createPlayer(IEntitySystem& entities, float x, float y) {
    Entity player = entities.createEntity();

    // Add components
    entities.emplace<Transform2D>(player, Transform2D{
        .x = x,
        .y = y,
        .rotation = 0.0f,
        .scaleX = 1.0f,
        .scaleY = 1.0f
    });

    entities.emplace<Velocity>(player, 0.0f, 0.0f);

    entities.emplace<Health>(player, 100, 100);

    entities.emplace<Sprite>(player, Sprite{
        .textureHandle = &playerTexture,
        .layer = 10
    });

    entities.emplace<PlayerTag>(player);

    return player;
}
```

### Component Cleanup Pattern

```cpp
// Remove all components from an entity before destruction
void cleanupEntity(IEntitySystem& entities, Entity entity) {
    // Cleanup logic for specific components
    if (entities.allOf<AudioEmitter>(entity)) {
        AudioEmitter& emitter = entities.get<AudioEmitter>(entity);
        emitter.stop();
    }

    if (entities.allOf<ParticleEmitter>(entity)) {
        ParticleEmitter& particles = entities.get<ParticleEmitter>(entity);
        particles.destroy();
    }

    // Destroy the entity (removes all components automatically)
    entities.destroyEntity(entity);
}
```

### Factory Pattern

```cpp
class EntityFactory {
public:
    EntityFactory(IEntitySystem& entities, IAssetSystem& assets)
        : entities_(entities), assets_(assets) {}

    Entity createEnemy(const std::string& type, float x, float y) {
        Entity enemy = entities_.createEntity();

        entities_.emplace<Transform2D>(enemy, Transform2D{.x = x, .y = y});
        entities_.emplace<EnemyTag>(enemy);

        if (type == "walker") {
            entities_.emplace<WalkerAI>(enemy);
            entities_.emplace<Velocity>(enemy, 50.0f, 0.0f);
        } else if (type == "flyer") {
            entities_.emplace<FlyerAI>(enemy);
            entities_.emplace<Velocity>(enemy, 30.0f, 0.0f);
        }

        return enemy;
    }

private:
    IEntitySystem& entities_;
    IAssetSystem& assets_;
};
```

## Advanced Usage

### Direct Registry Access

For advanced use cases, you can access the underlying EnTT registry directly:

```cpp
entt::registry& registry = entitySystem->getRegistry();

// Use EnTT features directly
registry.emplace<Position>(entity, 10.0f, 20.0f);

// Advanced EnTT patterns
registry.on_construct<Health>().connect<&onHealthAdded>();
registry.on_destroy<Health>().connect<&onHealthRemoved>();
```

### Signals and Events

EnTT provides signals for component lifecycle events. Access them through the registry:

```cpp
void onHealthAdded(entt::registry& reg, Entity entity) {
    Health& health = reg.get<Health>(entity);
    std::cout << "Health added to entity with " << health.maximum << " max HP\n";
}

void onHealthRemoved(entt::registry& reg, Entity entity) {
    std::cout << "Health removed from entity\n";
}

// Connect signals
entt::registry& reg = entitySystem->getRegistry();
reg.on_construct<Health>().connect<&onHealthAdded>();
reg.on_destroy<Health>().connect<&onHealthRemoved>();

// Disconnect when done
reg.on_construct<Health>().disconnect<&onHealthAdded>();
```

### Groups (Performance Optimization)

For frequently accessed component combinations, use EnTT groups for better cache locality:

```cpp
// Create a group for entities with both Position and Velocity
auto group = entitySystem->getRegistry().group<Position>(entt::get<Velocity>);

// Iteration is faster due to better memory layout
for (auto entity : group) {
    Position& pos = group.get<Position>(entity);
    Velocity& vel = group.get<Velocity>(entity);

    pos.x += vel.dx;
    pos.y += vel.dy;
}
```

### Entity Relationships

Implement parent-child relationships with components:

```cpp
struct Parent {
    Entity parent;
};

struct Children {
    std::vector<Entity> children;
};

void attachChild(IEntitySystem& entities, Entity parent, Entity child) {
    entities.emplace<Parent>(child, parent);

    if (Children* children = entities.tryGet<Children>(parent)) {
        children->children.push_back(child);
    } else {
        entities.emplace<Children>(parent, std::vector{child});
    }
}

void updateHierarchy(IEntitySystem& entities) {
    // Update child transforms relative to parent
    auto view = entities.view<Transform2D, Parent>();
    for (auto [entity, transform, parent] : view.each()) {
        if (const Transform2D* parentTransform =
                entities.tryGet<Transform2D>(parent.parent)) {
            // Apply parent transform
            transform.x += parentTransform->x;
            transform.y += parentTransform->y;
            transform.rotation += parentTransform->rotation;
        }
    }
}
```

## Integration with Other Systems

### Physics System Integration

```cpp
// Physics bodies are linked to entities
Entity platform = entities->createEntity();
entities->emplace<Transform2D>(platform, Transform2D{.x = 100, .y = 400});

PhysicsBodyDef bodyDef{
    .type = BodyType::Static,
    .transform = entities->get<Transform2D>(platform),
    .size = {200.0f, 20.0f}
};

physics->createBody(platform, bodyDef);
```

### Graphics System Integration

```cpp
// Sprite components are rendered by the graphics system
Entity sprite = entities->createEntity();
entities->emplace<Transform2D>(sprite, Transform2D{.x = 200, .y = 150});
entities->emplace<Sprite>(sprite, Sprite{
    .textureHandle = &texture,
    .sourceRect = {{0, 0}, {32, 32}},
    .layer = 5
});
```

### Event System Integration

```cpp
// Listen for entity-related events
events->subscribe("entity.destroyed", [&](const EventData& data) {
    if (const auto* entityEvent = std::get_if<EntityEventData>(&data)) {
        Entity destroyed = entityEvent->entity;
        // Clean up references to destroyed entity
    }
});

// Emit entity events
events->emit("entity.created", EntityEventData{.entity = newEntity});
```

### Level System Integration

```cpp
// Level system creates entities from level data
void loadLevel(ILevelSystem& levels, IEntitySystem& entities) {
    std::vector<EntityDef> entityDefs = levels.getEntityDefinitions(currentLevel);

    for (const EntityDef& def : entityDefs) {
        Entity entity = entities.createEntity();
        entities.emplace<Transform2D>(entity, def.transform);

        // Apply properties based on type
        if (def.type == "platform") {
            entities.emplace<PlatformTag>(entity);
        } else if (def.type == "enemy") {
            entities.emplace<EnemyTag>(entity);
            entities.emplace<Health>(entity, 50, 50);
        }
    }
}
```

## Performance Considerations

### Memory Layout

EnTT stores components in contiguous arrays (one per component type), which provides excellent cache locality when iterating:

```cpp
// This is cache-friendly - components are stored contiguously
for (auto [entity, pos, vel] : entities->view<Position, Velocity>().each()) {
    pos.x += vel.dx;
    pos.y += vel.dy;
}
```

### View Creation Cost

Views are cheap to create (essentially free), so create them in tight loops without worry:

```cpp
void update(IEntitySystem& entities) {
    // Creating view each frame is fine
    for (auto [entity, pos, vel] : entities->view<Position, Velocity>().each()) {
        pos.x += vel.dx;
        pos.y += vel.dy;
    }
}
```

### Component Design

Keep components small and focused for better cache performance:

```cpp
// Good - small, focused components
struct Position { float x, y; };
struct Velocity { float dx, dy; };
struct Health { int current, maximum; };

// Less ideal - large component with rarely-used data
struct GameObject {
    float x, y;
    float dx, dy;
    int health, maxHealth;
    std::string name;            // Large, rarely accessed
    std::vector<Item> inventory; // Large, rarely accessed
};
```

### Sparse vs Dense Components

- Components present on most entities should be dense (e.g., Position)
- Components present on few entities can be sparse (e.g., BossAI)
- EnTT optimizes this automatically

## Best Practices

### Component Naming

Use descriptive names that clearly indicate data:

```cpp
// Good
struct Position { float x, y; };
struct PlayerTag {};
struct Health { int current, maximum; };

// Avoid
struct Player {};  // Too generic
struct Data {};    // Not descriptive
```

### Avoid Component Logic

Keep components as pure data. Put logic in systems:

```cpp
// Good - data only
struct Velocity {
    float dx;
    float dy;
};

// Bad - logic in component
struct Velocity {
    float dx;
    float dy;

    void update(Position& pos) {  // Don't do this
        pos.x += dx;
        pos.y += dy;
    }
};
```

### Use Tags for Categorization

Empty components make excellent tags:

```cpp
struct PlayerTag {};
struct EnemyTag {};
struct StaticTag {};

// Easy to query
for (auto entity : entities->view<EnemyTag>()) {
    // Process all enemies
}
```

### Prefer Composition Over State

Instead of state machines in components, use component combinations:

```cpp
// Instead of:
struct Player {
    enum State { Idle, Running, Jumping } state;
};

// Use:
struct Idle {};
struct Running {};
struct Jumping {};

// Change state by swapping components
entities->remove<Idle>(player);
entities->emplace<Running>(player);
```

### Validate Entity Handles

Always check entity validity when storing entity references:

```cpp
struct Target {
    Entity entity;
};

void updateAI(IEntitySystem& entities, Entity entity) {
    Target* target = entities.tryGet<Target>(entity);
    if (target && entities.isValid(target->entity)) {
        // Target is valid
    }
}
```

## Testing

Example test patterns for entity system usage:

```cpp
#include <gtest/gtest.h>
import bestow.entity;
import bestow.entity.impl;

TEST(GameSystemTest, PlayerCreation) {
    auto entities = bestow::createEntitySystem();

    Entity player = createPlayer(*entities, 100, 200);

    ASSERT_TRUE(entities->isValid(player));
    EXPECT_TRUE(entities->allOf<Transform2D, Health, PlayerTag>(player));

    const Transform2D& transform = entities->get<Transform2D>(player);
    EXPECT_FLOAT_EQ(transform.x, 100.0f);
    EXPECT_FLOAT_EQ(transform.y, 200.0f);
}

TEST(GameSystemTest, PhysicsUpdate) {
    auto entities = bestow::createEntitySystem();

    Entity entity = entities->createEntity();
    entities->emplace<Position>(entity, 0.0f, 0.0f);
    entities->emplace<Velocity>(entity, 10.0f, 5.0f);

    updatePhysics(*entities, 1.0f);

    const Position& pos = entities->get<Position>(entity);
    EXPECT_FLOAT_EQ(pos.x, 10.0f);
    EXPECT_FLOAT_EQ(pos.y, 5.0f);
}
```

## Common Pitfalls

### Invalid Entity References

```cpp
// BAD - Entity becomes invalid after destruction
Entity enemy = entities->createEntity();
entities->destroyEntity(enemy);
entities->get<Position>(enemy);  // CRASH! Entity is invalid

// GOOD - Always validate
if (entities->isValid(enemy)) {
    entities->get<Position>(enemy);
}
```

### Iterator Invalidation

```cpp
// BAD - Modifying entities during iteration can invalidate iterators
for (auto entity : entities->view<Health>()) {
    if (entities->get<Health>(entity).current <= 0) {
        entities->destroyEntity(entity);  // Can invalidate iterator!
    }
}

// GOOD - Collect entities to modify, then process
std::vector<Entity> toDestroy;
for (auto entity : entities->view<Health>()) {
    if (entities->get<Health>(entity).current <= 0) {
        toDestroy.push_back(entity);
    }
}
for (Entity entity : toDestroy) {
    entities->destroyEntity(entity);
}
```

### Dangling Component Pointers

```cpp
// BAD - Pointer becomes invalid if components are reallocated
Position* pos = &entities->get<Position>(entity);
entities->createEntity();  // Might trigger reallocation
pos->x = 100;  // DANGER! Pointer might be invalid

// GOOD - Get reference again or work immediately
Position& pos = entities->get<Position>(entity);
pos.x = 100;
```

## Reference

### IEntitySystem Interface

Full interface reference:

```cpp
class IEntitySystem {
    // Entity lifecycle
    Entity createEntity();
    void destroyEntity(Entity entity);
    bool isValid(Entity entity) const;
    std::size_t entityCount() const;

    // Component management (typed)
    template<typename T, typename... Args>
    T& emplace(Entity entity, Args&&... args);

    template<typename T>
    void remove(Entity entity);

    template<typename T>
    T& get(Entity entity);

    template<typename T>
    const T& get(Entity entity) const;

    template<typename T>
    T* tryGet(Entity entity);

    template<typename T>
    const T* tryGet(Entity entity) const;

    // Component queries
    template<typename... Ts>
    bool allOf(Entity entity) const;

    template<typename... Ts>
    bool anyOf(Entity entity) const;

    // Views and iteration
    template<typename... Components>
    auto view();

    template<typename... Components>
    auto view() const;

    void each(std::function<void(Entity)> callback);

    // Registry access
    entt::registry& getRegistry();
    const entt::registry& getRegistry() const;

    // System update
    void update(DeltaTime dt);
};
```

### Common Component Types

Bestow provides standard component types in `bestow.types`:

- `Transform2D` - 2D position, rotation, and scale
- `Sprite` - Renderable sprite with texture and properties
- `AnimatedSprite` - Sprite with animation state
- `Camera` - Camera transform and viewport
- See `bestow.types.cppm` for complete list

### Factory Function

```cpp
import bestow.entity.impl;

std::unique_ptr<IEntitySystem> entities = bestow::createEntitySystem();
```

## Entity Groups (Planned Enhancement)

A planned enhancement will add automatic entity tracking by component type, eliminating manual `std::vector<Entity>` collections:

```cpp
// Current approach - manual tracking
std::vector<Entity> enemies_;

void createEnemy(float x, float y) {
    Entity e = entities->createEntity();
    entities->emplace<EnemyTag>(e);
    enemies_.push_back(e);  // Manual tracking
}

void update(DeltaTime dt) {
    for (Entity e : enemies_) {
        updateEnemy(e, dt);
    }
}

// Planned approach - automatic groups
void createEnemy(float x, float y) {
    Entity e = entities->createEntity();
    entities->emplace<EnemyTag>(e);
    // Automatically tracked!
}

void update(DeltaTime dt) {
    for (Entity e : entities->group<EnemyTag>()) {
        updateEnemy(e, dt);
    }
}
```

See [Entity Queries](Entity-Queries.md) for full documentation of the planned group API.

## Further Reading

- [EnTT Documentation](https://github.com/skypjack/entt/wiki)
- [Entity-Component-System FAQ](https://github.com/SanderMertens/ecs-faq)
- Bestow Technical Design Document
- Bestow Project Status Document

## Related Systems

- **Event System** - Emit and subscribe to entity-related events
- **Physics System** - Associates physics bodies with entities
- **Graphics System** - Renders entities with Sprite components
- **Level System** - Creates entities from level data
- **AI System** - Implements behaviors for entities with AI components
- **Save System** - Serializes entity state for save files
- **[Entity Queries](Entity-Queries.md)** - Automatic entity tracking (planned)
- **[Sprite Renderer](Sprite-Renderer.md)** - Automatic entity rendering (planned)
- **[Blueprint Factory](Blueprint-Factory.md)** - Data-driven entity creation (planned)
