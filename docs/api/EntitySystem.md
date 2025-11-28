# EntitySystem API

The `EntitySystem` provides entity creation and component management using EnTT.

## Overview

```cpp
auto& entities = sys.entities;

// Create entity
Entity player = entities->createEntity();

// Add components
entities->emplace<Transform2D>(player, 100.0f, 200.0f);
entities->emplace<Health>(player, 100, 100);

// Query entities
for (auto [entity, transform, health] : entities->view<Transform2D, Health>().each()) {
    // Process entities with both components
}

// Destroy entity
entities->destroyEntity(player);
```

## Entity Lifecycle

### createEntity()

```cpp
Entity createEntity();
```

Creates a new entity and returns its handle.

**Returns:** `Entity` handle

**Example:**

```cpp
Entity enemy = entities->createEntity();
```

---

### destroyEntity(Entity entity)

```cpp
void destroyEntity(Entity entity);
```

Destroys an entity and all its components.

**Parameters:**
- `entity`: Entity to destroy

**Example:**

```cpp
entities->destroyEntity(enemy);
```

---

### isValid(Entity entity)

```cpp
bool isValid(Entity entity) const;
```

Checks if an entity handle is valid and still exists.

**Returns:** `true` if entity exists, `false` otherwise

**Example:**

```cpp
if (entities->isValid(player)) {
    // Safe to use player
}
```

---

### entityCount()

```cpp
std::size_t entityCount() const;
```

Returns the total number of living entities.

---

## Component Management

### emplace<T>(Entity entity, Args&&... args)

```cpp
template<typename T, typename... Args>
T& emplace(Entity entity, Args&&... args);
```

Constructs and attaches a component to an entity.

**Type Parameters:**
- `T`: Component type
- `Args`: Constructor argument types

**Returns:** Reference to the newly created component

**Example:**

```cpp
// Transform with position
entities->emplace<Transform2D>(player, 100.0f, 200.0f);

// Debug rectangle
entities->emplace<DebugRect>(player,
    Vec2{32.0f, 32.0f},  // size
    Color::red(),        // fill color
    Color::black(),      // outline color
    2.0f,                // outline width
    0,                   // layer
    true                 // filled
);

// Custom component
struct Health {
    int current;
    int max;
};
entities->emplace<Health>(player, 100, 100);
```

---

### remove<T>(Entity entity)

```cpp
template<typename T>
void remove(Entity entity);
```

Removes a component from an entity.

**Example:**

```cpp
entities->remove<DebugRect>(player);
```

---

### get<T>(Entity entity)

```cpp
template<typename T>
T& get(Entity entity);

template<typename T>
const T& get(Entity entity) const;
```

Gets a reference to a component. **Throws if component doesn't exist.**

**Returns:** Reference to component

**Example:**

```cpp
Transform2D& transform = entities->get<Transform2D>(player);
transform.x += 10.0f;
```

---

### tryGet<T>(Entity entity)

```cpp
template<typename T>
T* tryGet(Entity entity);

template<typename T>
const T* tryGet(Entity entity) const;
```

Gets a pointer to a component, or `nullptr` if it doesn't exist.

**Returns:** Pointer to component, or `nullptr`

**Example:**

```cpp
if (auto* health = entities->tryGet<Health>(player)) {
    health->current -= 10;
}
```

---

### allOf<Ts...>(Entity entity)

```cpp
template<typename... Ts>
bool allOf(Entity entity) const;
```

Checks if an entity has all specified components.

**Example:**

```cpp
if (entities->allOf<Transform2D, Health, Sprite>(player)) {
    // Player has all three components
}
```

---

### anyOf<Ts...>(Entity entity)

```cpp
template<typename... Ts>
bool anyOf(Entity entity) const;
```

Checks if an entity has any of the specified components.

**Example:**

```cpp
if (entities->anyOf<Sprite, DebugRect>(entity)) {
    // Entity is visible
}
```

---

## Querying Entities

### view<Components...>()

```cpp
template<typename... Components>
auto view();

template<typename... Components>
auto view() const;
```

Returns an EnTT view for efficient iteration over entities with specified components.

**Best for:** Read-only iteration, fastest performance

**Example:**

```cpp
// Iterate with components
for (auto [entity, transform, velocity] : entities->view<Transform2D, Velocity>().each()) {
    transform.x += velocity.x * dt;
    transform.y += velocity.y * dt;
}

// Iterate entity IDs only
for (Entity entity : entities->view<Enemy>()) {
    // Process enemy
}
```

---

### collect<Components...>()

```cpp
template<typename... Components>
std::vector<Entity> collect() const;
```

Collects all entities with specified components into a vector.

**Best for:** When you need to modify the entity list during iteration

**Example:**

```cpp
// Safe to destroy entities during iteration
auto enemies = entities->collect<Enemy, Health>();
for (Entity e : enemies) {
    if (entities->get<Health>(e).current <= 0) {
        entities->destroyEntity(e);
    }
}
```

---

### collectExcluding<Include, Exclude...>()

```cpp
template<typename Include, typename... Exclude>
std::vector<Entity> collectExcluding() const;
```

Collects entities that have `Include` component but lack any `Exclude` components.

**Example:**

```cpp
// Get entities with Transform2D but without PhysicsBody
auto staticObjects = entities->collectExcluding<Transform2D, PhysicsBody>();
```

---

### first<Components...>()

```cpp
template<typename... Components>
std::optional<Entity> first() const;
```

Returns the first entity with specified components, or `std::nullopt` if none exist.

**Example:**

```cpp
if (auto player = entities->first<Player, Transform2D>()) {
    Transform2D& pos = entities->get<Transform2D>(*player);
    // Use player position
}
```

---

### single<Components...>()

```cpp
template<typename... Components>
std::optional<Entity> single() const;
```

Returns the entity if exactly one exists with specified components, otherwise `std::nullopt`.

**Use for:** Singleton entities (e.g., player, camera target)

**Example:**

```cpp
if (auto player = entities->single<Player>()) {
    // Guaranteed to be the only player entity
    auto& transform = entities->get<Transform2D>(*player);
}
```

---

### groupCount<Components...>()

```cpp
template<typename... Components>
std::size_t groupCount() const;
```

Returns the count of entities with specified components.

**Example:**

```cpp
std::size_t enemyCount = entities->groupCount<Enemy>();
```

---

### hasAny<Components...>()

```cpp
template<typename... Components>
bool hasAny() const;
```

Checks if any entities exist with specified components.

**Example:**

```cpp
if (!entities->hasAny<Enemy>()) {
    // Level complete
}
```

---

## Registry Access

### getRegistry()

```cpp
entt::registry& getRegistry();
const entt::registry& getRegistry() const;
```

Returns the underlying EnTT registry for advanced operations.

**Example:**

```cpp
// Advanced EnTT features
auto& registry = entities->getRegistry();
registry.view<Transform2D>().use<Transform2D>().each([](auto& transform) {
    // Custom iteration
});
```

---

## Common Component Types

JFrame provides built-in component types in `jframe.types`:

### Transform2D

```cpp
struct Transform2D {
    float x = 0.0f;
    float y = 0.0f;
    float rotation = 0.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;

    Vec2 position() const;
    Vec2 scale() const;
};
```

**Use for:** Position, rotation, and scale in 2D space

---

### DebugRect

```cpp
struct DebugRect {
    Vec2 size{32.0f, 32.0f};
    Color fillColor{128, 128, 128, 255};
    Color outlineColor{0, 0, 0, 0};
    float outlineWidth{0.0f};
    RenderLayer layer{0};
    bool filled{true};
};
```

**Use for:** Visual debugging, prototyping without textures

**Example:**

```cpp
entities->emplace<DebugRect>(entity,
    Vec2{32.0f, 32.0f},     // size
    Color::red(),           // fill
    Color::white(),         // outline
    2.0f,                   // outline width
    0,                      // layer
    true                    // filled
);
```

---

### DebugCircle

```cpp
struct DebugCircle {
    float radius{16.0f};
    Color fillColor{128, 128, 128, 255};
    Color outlineColor{0, 0, 0, 0};
    float outlineWidth{0.0f};
    RenderLayer layer{0};
    bool filled{true};
    int segments{32};
};
```

---

### DebugLine

```cpp
struct DebugLine {
    Vec2 endOffset{32.0f, 0.0f};
    Color color{255, 255, 255, 255};
    float thickness{1.0f};
    RenderLayer layer{0};
};
```

---

### Sprite

```cpp
struct Sprite {
    AssetHandle* textureHandle = nullptr;
    Canvas sourceRect;
    Transform2D transform;
    Color tint = Color::white();
    RenderLayer layer = 0;
    Vec2 anchor = {0.5f, 0.5f};
};
```

**Use for:** Rendering textured sprites

---

### AnimatedSprite

```cpp
struct AnimatedSprite {
    SpriteSheet sheet;
    std::unordered_map<std::string, Animation> animations;
    std::string currentAnimation;
    int currentFrameIndex = 0;
    float frameTimer = 0.0f;
    bool playing = true;

    void play(const std::string& animName);
    void update(float dt);
    int getCurrentFrame() const;
};
```

---

## Usage Patterns

### Creating a Player Entity

```cpp
Entity createPlayer(float x, float y) {
    Entity player = entities->createEntity();

    // Position
    entities->emplace<Transform2D>(player, x, y);

    // Visual (debug)
    entities->emplace<DebugRect>(player,
        Vec2{32.0f, 48.0f},
        Color::blue()
    );

    // Physics
    PhysicsBodyDef bodyDef{
        .type = BodyType::Dynamic,
        .transform = {.x = x, .y = y},
        .size = {32.0f, 48.0f},
        .fixedRotation = true
    };
    physics->createBody(player, bodyDef);

    // Gameplay
    entities->emplace<Player>(player);
    entities->emplace<Health>(player, 100, 100);

    return player;
}
```

---

### Updating All Entities with Velocity

```cpp
void updateMovement(DeltaTime dt) {
    for (auto [entity, transform, velocity] :
         entities->view<Transform2D, Velocity>().each()) {
        transform.x += velocity.x * dt;
        transform.y += velocity.y * dt;
    }
}
```

---

### Destroying Entities with a Condition

```cpp
void destroyDeadEnemies() {
    auto enemies = entities->collect<Enemy, Health>();
    for (Entity e : enemies) {
        if (entities->get<Health>(e).current <= 0) {
            entities->destroyEntity(e);
        }
    }
}
```

---

### Finding the Closest Enemy

```cpp
std::optional<Entity> findClosestEnemy(Vec2 position) {
    std::optional<Entity> closest;
    float minDistSq = std::numeric_limits<float>::max();

    for (auto [entity, transform] : entities->view<Enemy, Transform2D>().each()) {
        float dx = transform.x - position.x;
        float dy = transform.y - position.y;
        float distSq = dx * dx + dy * dy;

        if (distSq < minDistSq) {
            minDistSq = distSq;
            closest = entity;
        }
    }

    return closest;
}
```

---

## Performance Tips

1. **Use `view()` for iteration** - Fastest for read-only queries
2. **Use `collect()` only when needed** - When destroying or creating entities during iteration
3. **Cache component access** - Don't call `get()` repeatedly in tight loops
4. **Prefer component queries over flags** - Use separate components instead of `enum` flags
5. **Batch operations** - Process all entities of a type together

## See Also

- [BlueprintFactory](BlueprintFactory.md) - Data-driven entity creation
- [PhysicsSystem](PhysicsSystem.md) - Adding physics to entities
- [GraphicsSystem](GraphicsSystem.md) - Rendering entities
- [GASSystem](GASSystem.md) - Gameplay attributes and abilities
