# Entity System

> **Visibility:** Public (Lua + C++ API)
> **Tier:** 2
> **Dependencies:** Types, Events
> **Lua Paths:** `bestow.entity` (high-level), `bestow.entity.core` (low-level)

## Purpose

The Entity System is the central ECS (Entity-Component-System) facade for Bestow. It manages entity lifecycles, component storage, hierarchy relationships, tagging, naming, and queries. The high-level API provides string-based component access suitable for Lua scripting, while the low-level API exposes the full typed template interface backed by EnTT for maximum C++ performance, along with batch operations, serialization, and change detection.

## High-Level API: `IEntitySystem`

The simplified API for common game development tasks. Entity-centric, name-based component access with sensible defaults. No lifecycle methods -- the engine manages `initialize()`, `shutdown()`, and `update()` internally.

### Lifecycle

| Method | Returns | Description |
|--------|---------|-------------|
| `create()` | `Entity` | Create a new entity and return its handle |
| `destroy(Entity entity)` | `Result<void>` | Destroy an entity and all its components |
| `isValid(Entity entity)` | `bool` | Check whether an entity handle refers to a living entity |
| `count()` | `std::size_t` | Return the total number of active entities |

### Component Access (Name-Based)

| Method | Returns | Description |
|--------|---------|-------------|
| `addComponent(Entity entity, std::string_view type, const ComponentData& data)` | `Result<void>` | Add a component to an entity by type name with initial field values |
| `removeComponent(Entity entity, std::string_view type)` | `Result<void>` | Remove a named component from an entity |
| `hasComponent(Entity entity, std::string_view type)` | `bool` | Check whether an entity has a component of the given type name |
| `getComponent(Entity entity, std::string_view type)` | `Result<ComponentData>` | Retrieve all fields of a named component as a type-erased data map |
| `setField(Entity entity, std::string_view type, std::string_view field, const SceneParam& value)` | `Result<void>` | Set a single field on a named component without replacing the entire component |

### Queries

| Method | Returns | Description |
|--------|---------|-------------|
| `findByComponent(std::string_view type)` | `std::vector<Entity>` | Return all entities that have the named component type |
| `findByName(std::string_view name)` | `std::optional<Entity>` | Find a single entity by its debug name, or nullopt if none matches |
| `findByTag(std::string_view tag)` | `std::vector<Entity>` | Return all entities that carry the given tag |

### Hierarchy

| Method | Returns | Description |
|--------|---------|-------------|
| `setParent(Entity child, Entity parent)` | `Result<void>` | Establish a parent-child relationship between two entities |
| `getChildren(Entity entity)` | `std::vector<Entity>` | Return the immediate children of an entity |

## Low-Level API: `IEntityCore`

Full control API. Exposes typed template access, batch operations, hierarchy traversal, tags, names, serialization, change detection, and component type registration for the Lua bridge.

### Lifecycle

| Method | Returns | Description |
|--------|---------|-------------|
| `createEntity()` | `Entity` | Create a new entity and return its handle |
| `destroyEntity(Entity entity)` | `Result<void>` | Destroy an entity and remove all associated components |
| `isValid(Entity entity)` | `bool` | Check whether an entity handle is still valid |
| `entityCount()` | `std::size_t` | Return the total number of living entities |
| `clear()` | `void` | Destroy all entities and components in the registry |

### Typed Component Access (C++ Templates)

| Method | Returns | Description |
|--------|---------|-------------|
| `emplace<T>(Entity entity, Args&&... args)` | `T&` | Construct component T in-place on the entity and return a reference |
| `remove<T>(Entity entity)` | `void` | Remove component T from the entity |
| `get<T>(Entity entity)` | `T&` | Get a reference to component T on the entity; undefined if missing |
| `tryGet<T>(Entity entity)` | `T*` | Get a pointer to component T, or nullptr if the entity lacks it |
| `has<T>(Entity entity)` | `bool` | Check whether the entity has component T |
| `view<Ts...>()` | `auto` | Return an EnTT view over all entities with the listed component types |
| `group<Ts...>()` | `auto` | Return an EnTT owning group for high-performance iteration |

### Type-Erased Component Access (Lua Bridge)

| Method | Returns | Description |
|--------|---------|-------------|
| `addComponentByName(Entity entity, std::string_view typeName, const ComponentData& data)` | `Result<void>` | Add a component by its registered string name with field data |
| `getComponentByName(Entity entity, std::string_view typeName)` | `Result<ComponentData>` | Retrieve a component's fields by type name as a ComponentData map |
| `setComponentByName(Entity entity, std::string_view typeName, const ComponentData& data)` | `Result<void>` | Replace all fields of a named component with the provided data |
| `removeComponentByName(Entity entity, std::string_view typeName)` | `Result<void>` | Remove a component from an entity by its registered type name |
| `hasComponentByName(Entity entity, std::string_view typeName)` | `bool` | Check whether an entity has a component with the given type name |

### Component Type Registration

| Method | Returns | Description |
|--------|---------|-------------|
| `registerComponentType(std::string_view name, ComponentTypeInfo info)` | `Result<void>` | Register a component type so it can be accessed by name from Lua |
| `getRegisteredTypes()` | `std::vector<std::string>` | List all component type names that have been registered |
| `getTypeInfo(std::string_view name)` | `std::optional<ComponentTypeInfo>` | Retrieve the registration info for a named component type |

### Batch Operations

| Method | Returns | Description |
|--------|---------|-------------|
| `createEntities(std::size_t count)` | `std::vector<Entity>` | Create multiple entities at once and return their handles |
| `destroyEntities(std::span<const Entity> entities)` | `void` | Destroy multiple entities in a single batch call |

### Queries

| Method | Returns | Description |
|--------|---------|-------------|
| `query(std::span<const std::string> withComponents, std::span<const std::string> withoutComponents)` | `std::vector<Entity>` | Find entities matching component inclusion/exclusion filters by name |
| `countWith(std::span<const std::string> components)` | `std::size_t` | Count entities that have all of the named components |
| `findFirst(std::span<const std::string> components)` | `std::optional<Entity>` | Return the first entity matching the component filter, or nullopt |

### Hierarchy

| Method | Returns | Description |
|--------|---------|-------------|
| `setParent(Entity child, Entity parent)` | `Result<void>` | Establish a parent-child relationship between two entities |
| `removeParent(Entity child)` | `Result<void>` | Detach an entity from its parent without destroying it |
| `getParent(Entity entity)` | `std::optional<Entity>` | Return the parent of an entity, or nullopt if it is a root |
| `getChildren(Entity entity)` | `std::vector<Entity>` | Return the immediate children of an entity |
| `isDescendantOf(Entity entity, Entity ancestor)` | `bool` | Check whether entity is a descendant of ancestor at any depth |

### Tags

| Method | Returns | Description |
|--------|---------|-------------|
| `addTag(Entity entity, std::string_view tag)` | `Result<void>` | Attach a string tag to an entity |
| `removeTag(Entity entity, std::string_view tag)` | `Result<void>` | Remove a tag from an entity |
| `hasTag(Entity entity, std::string_view tag)` | `bool` | Check whether an entity carries a specific tag |
| `findByTag(std::string_view tag)` | `std::vector<Entity>` | Return all entities that carry the given tag |

### Names

| Method | Returns | Description |
|--------|---------|-------------|
| `setName(Entity entity, std::string_view name)` | `Result<void>` | Assign a debug name to an entity for identification in scripts and tools |
| `getName(Entity entity)` | `std::optional<std::string_view>` | Retrieve the debug name of an entity, or nullopt if unnamed |
| `findByName(std::string_view name)` | `std::optional<Entity>` | Find the first entity with the given name, or nullopt |

### Serialization

| Method | Returns | Description |
|--------|---------|-------------|
| `serializeEntity(Entity entity)` | `Result<std::string>` | Serialize an entity and all its components to a string representation |
| `deserializeEntity(std::string_view data)` | `Result<Entity>` | Recreate an entity from a previously serialized string |

### Change Detection

| Method | Returns | Description |
|--------|---------|-------------|
| `wasModified(Entity entity)` | `bool` | Check whether any component on the entity was modified since the last clear |
| `clearModifiedFlags()` | `void` | Reset all modification flags across the entire registry |

## Types

### ComponentData

Type-erased component storage used by the Lua bridge for name-based component access.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `fields` | `std::unordered_map<std::string, ComponentFieldValue>` | `{}` | Map of field names to their values |

### ComponentFieldValue

The variant type for individual component field values.

```cpp
using ComponentFieldValue = std::variant<
    float, int, bool, std::string,
    Vec2, Vec3, Vec4, Quat,
    Color, Entity
>;
```

| Variant | Description |
|---------|-------------|
| `float` | Single-precision floating point value |
| `int` | Integer value |
| `bool` | Boolean value |
| `std::string` | String value |
| `Vec2` | 2D vector (x, y) |
| `Vec3` | 3D vector (x, y, z) |
| `Vec4` | 4D vector (x, y, z, w) |
| `Quat` | Quaternion rotation (x, y, z, w) |
| `Color` | RGBA color (r, g, b, a) |
| `Entity` | Reference to another entity |

### ComponentTypeInfo

Registration metadata for a component type, enabling name-based access from Lua.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `name` | `std::string` | `""` | The unique type name used in Lua (e.g., `"Transform2D"`) |
| `fields` | `std::vector<std::pair<std::string, std::string>>` | `{}` | List of (fieldName, fieldTypeName) pairs describing the component layout |
| `sizeBytes` | `std::size_t` | `0` | Size of the component struct in bytes |
| `emplaceFromData` | `std::function<void(Entity, const ComponentData&)>` | `nullptr` | Callback that constructs the component on an entity from a ComponentData map |
| `extractToData` | `std::function<ComponentData(Entity)>` | `nullptr` | Callback that reads the component from an entity into a ComponentData map |

## Lua Examples

```lua
-- High-level: Create an entity with components
local player = bestow.entity.create()
bestow.entity.addComponent(player, "Transform2D", { x = 100, y = 200 })
bestow.entity.addComponent(player, "Health", { current = 100, max = 100 })
bestow.entity.setField(player, "Transform2D", "x", 150)

local data, err = bestow.entity.getComponent(player, "Transform2D")
if err then print("Error: " .. err.message) end

local enemies = bestow.entity.findByTag("enemy")
local boss = bestow.entity.findByName("FinalBoss")

bestow.entity.setParent(sword, player)
local kids = bestow.entity.getChildren(player)

-- Low-level: Batch creation and type-erased queries
local entities = bestow.entity.core.createEntities(100)
local withTransform = bestow.entity.core.query({"Transform2D", "Velocity"}, {"Dead"})
local count = bestow.entity.core.countWith({"Health"})

bestow.entity.core.addTag(player, "player")
bestow.entity.core.setName(player, "Hero")
local isChild = bestow.entity.core.isDescendantOf(sword, player)

local serialized = bestow.entity.core.serializeEntity(player)
local clone = bestow.entity.core.deserializeEntity(serialized)

if bestow.entity.core.wasModified(player) then
    -- sync to network
end
bestow.entity.core.clearModifiedFlags()
```

## C++ Examples

```cpp
// High-level usage
Entity player = entity->create();
entity->addComponent(player, "Transform2D", {{"x", 100.0f}, {"y", 200.0f}});
entity->setField(player, "Health", "current", 50.0f);

auto enemies = entity->findByTag("enemy");
auto boss = entity->findByName("FinalBoss");

entity->setParent(weapon, player);

// Low-level typed access (C++ only)
auto& transform = entityCore->emplace<Transform2D>(player, Vec2{100, 200}, 0.0f, Vec2{1, 1});
auto& health = entityCore->emplace<Health>(player, 100, 100);

if (auto* hp = entityCore->tryGet<Health>(player)) {
    hp->current -= damage;
}

auto view = entityCore->view<Transform2D, Velocity>();
for (auto [entity, t, v] : view.each()) {
    t.position += v.linear * dt;
}

// Batch creation
auto entities = entityCore->createEntities(1000);

// Hierarchy
entityCore->setParent(child, parent);
bool isDesc = entityCore->isDescendantOf(child, root);

// Serialization
auto json = entityCore->serializeEntity(player);
auto clone = entityCore->deserializeEntity(json.value());

// Change detection
if (entityCore->wasModified(player)) {
    // handle dirty entity
}
entityCore->clearModifiedFlags();
```
