# Blueprints

> **Visibility:** Protected (peer-system only, not exposed to Lua)
> **Tier:** 3
> **Dependencies:** Types, Entity, Assets, Config
> **Consumers:** Scene, Level loading

## Purpose

The Blueprint system provides a data-driven entity factory that transforms Lua table definitions into fully assembled entities with components, physics bodies, and metadata. Blueprints support single inheritance (a child blueprint merges its parent's components and properties before applying its own), property overrides at spawn time via `SceneParams`, and hot reload of definitions without restarting the engine. It is a protected system because game code never interacts with the blueprint registry directly -- entity creation flows through the Scene system, which delegates to `IBlueprintCore` internally to resolve blueprint names, apply inheritance chains, and stamp out entities.

## High-Level API

There is no high-level API for the Blueprint system. Blueprints are not exposed to Lua game code. Instead, game developers create entities through the Scene system (`bestow.scene.spawn("enemy", x, y)`), which internally resolves the named blueprint, applies inheritance and overrides, and delegates to `IBlueprintCore` for entity assembly. Peer systems -- Scene and Level loading -- are the sole consumers of this contract.

## Low-Level API: `IBlueprintCore`

Full control over the blueprint registry: loading definitions from Lua assets, querying the registry, spawning entities with overrides, and registering custom component creators so that Lua-declared component names map to C++ `emplace` calls.

### Loading

| Method | Returns | Description |
|--------|---------|-------------|
| `loadBlueprints(AssetHandle luaAsset)` | `Result<void>` | Parse a Lua asset containing blueprint definitions and add them to the registry. The asset must have been loaded through `IAssetCore` first. Existing blueprints with the same name are overwritten. Returns `ParseError` if the Lua source is malformed or references unknown component types. |
| `loadBlueprintsFromString(std::string_view luaSource)` | `Result<void>` | Parse raw Lua source containing blueprint definitions. Useful for testing and procedural blueprint generation. Same overwrite and error semantics as `loadBlueprints()`. |
| `reloadBlueprints()` | `void` | Re-parse all previously loaded blueprint sources, updating definitions in-place. Called by the hot reload pipeline when a blueprint Lua file changes on disk. Existing entities are not retroactively updated -- only future spawns use the new definitions. |
| `clearBlueprints()` | `void` | Remove all loaded blueprint definitions from the registry. Component registrations are preserved. |

### Queries

| Method | Returns | Description |
|--------|---------|-------------|
| `hasBlueprint(std::string_view name)` | `bool` | Check whether a blueprint with the given name exists in the registry. |
| `getBlueprintNames()` | `std::vector<std::string>` | Return the names of all registered blueprints, in no particular order. |
| `getBlueprint(std::string_view name)` | `Result<BlueprintDef>` | Retrieve the full resolved definition of a blueprint (with inheritance already applied) for inspection. Returns `NotFound` if the name does not exist in the registry. |

### Entity Creation

| Method | Returns | Description |
|--------|---------|-------------|
| `spawn(std::string_view blueprintName, Vec2 position)` | `Result<Entity>` | Create an entity from a blueprint at the given 2D position. The blueprint's component list is iterated in order, and each registered `ComponentCreator` is invoked with default properties. If the blueprint has a `physics` block, a physics body is also created. Returns `NotFound` if the blueprint does not exist. |
| `spawn(std::string_view blueprintName, Vec2 position, Vec2 size)` | `Result<Entity>` | Create an entity from a blueprint at the given 2D position with explicit size, overriding any size declared in the blueprint's transform or physics definition. |
| `spawn(std::string_view blueprintName, Vec2 position, const SceneParams& overrides)` | `Result<Entity>` | Create an entity from a blueprint at the given 2D position with property overrides. Overrides are merged on top of the blueprint's default properties before component creators are invoked. |
| `spawn(std::string_view blueprintName, Vec3 position)` | `Result<Entity>` | Create an entity from a blueprint at the given 3D position. Identical to the 2D variant but places the entity in 3D space. |
| `spawn(std::string_view blueprintName, Vec3 position, const SceneParams& overrides)` | `Result<Entity>` | Create an entity from a blueprint at the given 3D position with property overrides. |

### Component Registration

| Method | Returns | Description |
|--------|---------|-------------|
| `registerComponent(std::string_view name, ComponentCreator creator)` | `void` | Register a factory function that creates a named component type on an entity. The name must match the component name used in Lua blueprint definitions (e.g. `"Health"`, `"Sprite"`, `"Transform2D"`). Registering the same name twice overwrites the previous creator. |
| `isComponentRegistered(std::string_view name)` | `bool` | Check whether a component type has a registered creator function. Useful for validation during blueprint loading -- the system can warn about blueprints that reference unregistered component types. |

## Types

### BlueprintDef

```cpp
struct BlueprintDef {
    std::string name;
    std::string inherits;
    std::vector<ComponentDef> components;
    std::optional<BlueprintPhysicsDef> physics;
    SceneParams metadata;
};
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `name` | `std::string` | `""` | Unique blueprint identifier, matching the key used in Lua (e.g. `"player"`, `"coin"`, `"enemy_goblin"`). |
| `inherits` | `std::string` | `""` | Parent blueprint name for single inheritance. When non-empty, the parent's components and physics definition are merged first, then the child's definitions are applied on top. Empty string means no parent. |
| `components` | `std::vector<ComponentDef>` | `{}` | Ordered list of components to attach when spawning an entity. Components are created in list order, allowing later components to depend on earlier ones. |
| `physics` | `std::optional<BlueprintPhysicsDef>` | `nullopt` | Optional physics body configuration. When present, a physics body is automatically created alongside the entity. When inheriting, the child's physics block fully replaces the parent's (no field-level merge). |
| `metadata` | `SceneParams` | `{}` | Arbitrary key-value metadata attached to the blueprint. Not used by the spawn pipeline directly, but available for systems that need to query blueprint properties without spawning (e.g. UI tooltips, editor inspectors). |

### ComponentDef

```cpp
struct ComponentDef {
    std::string name;
    SceneParams properties;
};
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `name` | `std::string` | `""` | Component type name (e.g. `"Health"`, `"Transform2D"`, `"Sprite"`). Must match a name registered via `registerComponent()`. |
| `properties` | `SceneParams` | `{}` | Initial property values for the component, passed to the `ComponentCreator` function. Values use `SceneParam` variant type rather than `std::any`. |

### BlueprintPhysicsDef

```cpp
struct BlueprintPhysicsDef {
    std::string bodyType = "dynamic";
    std::optional<Vec2> size;
    bool sensor = false;
    bool fixedRotation = true;
    float density = 1.0f;
    float friction = 0.3f;
    float restitution = 0.0f;
    float linearDamping = 0.0f;
    std::string collisionLayer;
};
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `bodyType` | `std::string` | `"dynamic"` | Physics body type: `"static"` (immovable), `"dynamic"` (fully simulated), or `"kinematic"` (programmatically moved, collides with dynamic bodies). |
| `size` | `std::optional<Vec2>` | `nullopt` | Explicit collision box size. When `nullopt`, the physics system derives size from the entity's transform or sprite component. |
| `sensor` | `bool` | `false` | When `true`, the body generates collision events but does not produce physical responses (overlap-only trigger). |
| `fixedRotation` | `bool` | `true` | When `true`, the physics engine locks the body's rotation angle. Standard for 2D platformer characters. |
| `density` | `float` | `1.0` | Body density in kg/m^2, used to compute mass from the collision shape area. Higher density means heavier bodies. |
| `friction` | `float` | `0.3` | Surface friction coefficient (0.0 = ice, 1.0 = rubber). Controls how quickly sliding objects decelerate on contact. |
| `restitution` | `float` | `0.0` | Bounciness coefficient. 0.0 means no bounce (energy fully absorbed), 1.0 means perfectly elastic bounce. |
| `linearDamping` | `float` | `0.0` | Linear velocity damping factor. Simulates air resistance. Higher values cause the body to slow down faster when no forces are applied. |
| `collisionLayer` | `std::string` | `""` | Named collision layer for filtering (e.g. `"player"`, `"enemy"`, `"projectile"`). Empty string means default layer. Layer filtering rules are defined in the physics system configuration. |

### SceneParams

```cpp
using SceneParams = std::unordered_map<std::string, SceneParam>;
```

A key-value map where values are `SceneParam` -- a variant type that replaces V1's `std::any` with a closed set of types (`float`, `int`, `bool`, `std::string`, `Vec2`, `Vec3`, `Color`, and nested `SceneParams`). This provides type safety at the boundary between Lua data and C++ component creators.

### ComponentCreator

```cpp
using ComponentCreator = std::function<void(Entity, IEntityCore&, const SceneParams&)>;
```

A function that receives the target entity, the entity system, and the resolved property values (blueprint defaults merged with spawn-time overrides), and emplaces the appropriate component(s) on the entity.

## Examples

### Registering component creators

```cpp
void registerBuiltinComponents(IBlueprintCore& blueprints, IPhysicsCore& physics) {
    blueprints.registerComponent("Transform2D", [](Entity e, IEntityCore& ecs, const SceneParams& props) {
        float x = std::get<float>(props.at("x"));
        float y = std::get<float>(props.at("y"));
        ecs.emplace<Transform2D>(e, Vec2{x, y});
    });

    blueprints.registerComponent("Health", [](Entity e, IEntityCore& ecs, const SceneParams& props) {
        int max = std::get<int>(props.at("max"));
        ecs.emplace<Health>(e, max, max);
    });

    blueprints.registerComponent("Sprite", [](Entity e, IEntityCore& ecs, const SceneParams& props) {
        auto texturePath = std::get<std::string>(props.at("texture"));
        ecs.emplace<Sprite>(e, texturePath);
    });
}
```

### Loading blueprints from an asset

```cpp
void LevelLoader::loadBlueprintsForLevel(IBlueprintCore& blueprints, IAssetCore& assets) {
    auto handle = assets.load(":assets:/blueprints/enemies.lua");
    if (!handle) {
        LOG_ERROR("Failed to load enemy blueprints: {}", handle.error().message);
        return;
    }

    auto result = blueprints.loadBlueprints(*handle);
    if (!result) {
        LOG_ERROR("Failed to parse enemy blueprints: {}", result.error().message);
        return;
    }

    LOG_INFO("Loaded {} blueprints", blueprints.getBlueprintNames().size());
}
```

### Spawning entities with overrides

```cpp
void SceneSystem::spawnFromBlueprint(
    IBlueprintCore& blueprints,
    std::string_view name, Vec2 pos
) {
    // Simple spawn with defaults
    auto entity = blueprints.spawn("coin", Vec2{200.0f, 300.0f});
    if (!entity) {
        LOG_ERROR("Failed to spawn '{}': {}", name, entity.error().message);
        return;
    }

    // Spawn with property overrides
    SceneParams overrides;
    overrides["health.max"] = 200;
    overrides["sprite.tint"] = Color{1.0f, 0.0f, 0.0f, 1.0f};

    auto boss = blueprints.spawn("enemy_goblin", pos, overrides);
    if (!boss) {
        LOG_ERROR("Failed to spawn boss: {}", boss.error().message);
    }
}
```

### Querying blueprint definitions

```cpp
void EditorInspector::showBlueprintInfo(IBlueprintCore& blueprints, std::string_view name) {
    auto def = blueprints.getBlueprint(name);
    if (!def) {
        LOG_WARN("Blueprint '{}' not found", name);
        return;
    }

    LOG_INFO("Blueprint: {}", def->name);
    if (!def->inherits.empty()) {
        LOG_INFO("  Inherits: {}", def->inherits);
    }
    LOG_INFO("  Components: {}", def->components.size());
    for (const auto& comp : def->components) {
        LOG_INFO("    - {} ({} properties)", comp.name, comp.properties.size());
    }
    if (def->physics) {
        LOG_INFO("  Physics: {} body", def->physics->bodyType);
    }
}
```
