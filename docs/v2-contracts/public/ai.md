# AI System

> **Visibility:** Public (Lua + C++ API)
> **Tier:** 4
> **Dependencies:** Types, Entity, Physics3D, Assets
> **Lua Paths:** `bestow.ai` (high-level), `bestow.ai.core` (low-level)

## Purpose

The AI System provides behavior trees, navigation mesh pathfinding, steering behaviors, perception, and spatial queries for game entities. The high-level API covers the most common AI tasks -- attaching behavior trees, setting navigation targets, querying spatial relationships -- while the low-level API exposes the full range of AI configurability including blackboard management, patrol paths, crowd simulation, perception tuning, and direct navigation mesh operations. All pathfinding uses Recast/Detour under the hood, and behavior trees are defined in Lua via BehaviorTree.CPP integration.

## High-Level API: `IAISystem`

The simplified AI interface for common game development tasks. Path-based tree loading, entity-centric navigation, and convenience spatial queries. No lifecycle methods -- the engine calls `update()` internally.

### Behavior Trees

| Method | Returns | Description |
|--------|---------|-------------|
| `attachBehaviorTree(Entity entity, std::string_view treePath)` | `Result<void>` | Load a behavior tree from the given Lua asset path and attach it to the entity |
| `detachBehaviorTree(Entity entity)` | `Result<void>` | Remove the behavior tree from the entity, stopping all AI evaluation |
| `setBlackboard(Entity entity, std::string_view key, const BlackboardValue& value)` | `Result<void>` | Set a key-value pair on the entity's behavior tree blackboard |

### Navigation

| Method | Returns | Description |
|--------|---------|-------------|
| `setNavigationTarget(Entity entity, Vec3 target)` | `Result<void>` | Set a world-space position for the entity to navigate toward using the loaded nav mesh |
| `clearNavigationTarget(Entity entity)` | `Result<void>` | Cancel the entity's current navigation target and stop movement |

### Spatial Queries

| Method | Returns | Description |
|--------|---------|-------------|
| `findInRadius(Vec3 center, float radius)` | `std::vector<Entity>` | Find all entities within the given radius of a world-space position |
| `hasLineOfSight(Vec3 from, Vec3 to)` | `bool` | Check whether there is an unobstructed line between two world-space points |

## Low-Level API: `IAICore`

Full control API. Exposes behavior tree internals, navigation mesh management, steering configuration, patrol paths, perception parameters, crowd simulation weights, and detailed spatial queries with collision layer masks.

### Lifecycle

| Method | Returns | Description |
|--------|---------|-------------|
| `update(DeltaTime dt)` | `void` | Tick all behavior trees, update steering, process perception, and advance crowd simulation |

### Behavior Trees

| Method | Returns | Description |
|--------|---------|-------------|
| `attachBehaviorTree(Entity entity, AssetHandle treeAsset)` | `Result<void>` | Attach a behavior tree loaded from an asset handle to the entity |
| `detachBehaviorTree(Entity entity)` | `Result<void>` | Remove the behavior tree from the entity and clean up blackboard state |
| `hasBehaviorTree(Entity entity)` | `bool` | Check whether an entity currently has an attached behavior tree |

### Blackboard

| Method | Returns | Description |
|--------|---------|-------------|
| `setBlackboardValue(Entity entity, std::string_view key, const BlackboardValue& value)` | `Result<void>` | Set a typed key-value pair on the entity's blackboard for use in behavior tree conditions |
| `getBlackboardValue(Entity entity, std::string_view key)` | `std::optional<BlackboardValue>` | Retrieve a value from the entity's blackboard, or nullopt if the key does not exist |
| `clearBlackboard(Entity entity)` | `Result<void>` | Remove all key-value pairs from the entity's blackboard |

### Navigation Mesh

| Method | Returns | Description |
|--------|---------|-------------|
| `loadNavMesh(AssetHandle asset)` | `Result<void>` | Load a navigation mesh from the given asset into the AI system for pathfinding |
| `unloadNavMesh()` | `void` | Unload the currently loaded navigation mesh and free associated memory |
| `hasNavMesh()` | `bool` | Check whether a navigation mesh is currently loaded |
| `findPath3D(Vec3 start, Vec3 end, float agentRadius)` | `Result<std::vector<Vec3>>` | Compute a 3D path between two points on the nav mesh using the given agent radius |
| `findPath2D(Vec2 start, Vec2 end, float agentRadius)` | `Result<std::vector<Vec2>>` | Compute a 2D path between two points, projecting onto the nav mesh and returning XZ coordinates |
| `isPointOnNavMesh(Vec3 point)` | `bool` | Check whether the given world-space point lies on the loaded navigation mesh |
| `getClosestPointOnNavMesh(Vec3 point)` | `std::optional<Vec3>` | Find the nearest point on the nav mesh to the given position, or nullopt if no mesh is loaded |

### Steering Behaviors

| Method | Returns | Description |
|--------|---------|-------------|
| `setNavigationTarget(Entity entity, Vec3 target)` | `Result<void>` | Set a world-space navigation target for the entity's steering system |
| `clearNavigationTarget(Entity entity)` | `Result<void>` | Clear the entity's navigation target and stop steering movement |
| `getNavigationTarget(Entity entity)` | `std::optional<Vec3>` | Retrieve the entity's current navigation target, or nullopt if none is set |
| `setMaxSpeed(Entity entity, float speed)` | `Result<void>` | Set the maximum movement speed for the entity's steering behavior |
| `setMaxAcceleration(Entity entity, float accel)` | `Result<void>` | Set the maximum acceleration for the entity's steering behavior |
| `setAvoidanceRadius(Entity entity, float radius)` | `Result<void>` | Set the obstacle avoidance radius around the entity for local steering |

### Patrol Behavior

| Method | Returns | Description |
|--------|---------|-------------|
| `setPatrolPath(Entity entity, std::span<const Vec3> waypoints, bool loop)` | `Result<void>` | Assign a series of waypoints for the entity to patrol; set loop to true for continuous cycling |
| `clearPatrolPath(Entity entity)` | `Result<void>` | Remove the entity's patrol path and stop patrol movement |

### Perception

| Method | Returns | Description |
|--------|---------|-------------|
| `setSightRange(Entity entity, float range)` | `Result<void>` | Set the maximum distance at which the entity can perceive other entities by sight |
| `setSightAngle(Entity entity, float halfAngle)` | `Result<void>` | Set the half-angle of the entity's vision cone in radians |
| `setHearingRange(Entity entity, float range)` | `Result<void>` | Set the maximum distance at which the entity can perceive other entities by sound |
| `getPerceivedEntities(Entity entity)` | `std::vector<Entity>` | Return all entities currently detected by this entity's perception system |

### Spatial Queries

| Method | Returns | Description |
|--------|---------|-------------|
| `findEntitiesInRadius(Vec3 center, float radius, std::uint16_t mask)` | `std::vector<Entity>` | Find all entities within the given radius, optionally filtering by collision layer mask |
| `findClosestEntity(Vec3 position, std::uint16_t mask)` | `std::optional<Entity>` | Find the closest entity to the given position, optionally filtering by collision layer mask |
| `hasLineOfSight(Vec3 from, Vec3 to, std::uint16_t obstacleMask)` | `bool` | Check whether there is an unobstructed line between two points, using the given obstacle layer mask |

### Crowd Simulation

| Method | Returns | Description |
|--------|---------|-------------|
| `addToCrowd(Entity entity)` | `Result<void>` | Register an entity as a member of the crowd simulation for flocking behavior |
| `removeFromCrowd(Entity entity)` | `Result<void>` | Remove an entity from the crowd simulation |
| `setCrowdSeparation(float weight)` | `void` | Set the global separation weight for crowd members to avoid overlap |
| `setCrowdAlignment(float weight)` | `void` | Set the global alignment weight for crowd members to match neighboring velocities |
| `setCrowdCohesion(float weight)` | `void` | Set the global cohesion weight for crowd members to move toward the group center |

## Types

### BlackboardValue

A typed variant for behavior tree blackboard entries, replacing `std::any` with a closed set of supported types.

```cpp
using BlackboardValue = std::variant<
    float, int, bool,
    std::string,
    Vec2, Vec3,
    Entity
>;
```

| Variant | Description |
|---------|-------------|
| `float` | Floating-point value (distances, timers, thresholds) |
| `int` | Integer value (counts, indices, enumerations) |
| `bool` | Boolean value (flags, state toggles) |
| `std::string` | String value (names, state labels) |
| `Vec2` | 2D vector (positions, directions in 2D context) |
| `Vec3` | 3D vector (positions, directions, targets) |
| `Entity` | Reference to another entity (target, ally, threat) |

### NavMeshHandle

Navigation mesh data is loaded through the `IAssetCore` pipeline as `AssetHandle` with `AssetType::NavMesh`. The AI system receives the asset handle and uses it to initialize the internal Recast/Detour navigation mesh. There is no separate `NavMeshHandle` type -- the asset system handle is used directly.

## Lua Examples

```lua
-- High-level: Attach a behavior tree and configure AI
local entity = bestow.entity.create()

local _, err = bestow.ai.attachBehaviorTree(entity, "ai/patrol_guard.lua")
if err then print("AI error: " .. err.message) end

bestow.ai.setBlackboard(entity, "alertLevel", 0)
bestow.ai.setBlackboard(entity, "homePosition", { x = 100, y = 0, z = 50 })

bestow.ai.setNavigationTarget(entity, { x = 200, y = 0, z = 100 })

local nearby = bestow.ai.findInRadius({ x = 100, y = 0, z = 50 }, 20.0)
for _, e in ipairs(nearby) do
    if bestow.ai.hasLineOfSight(
        { x = 100, y = 1.5, z = 50 },
        { x = 110, y = 1.5, z = 60 }
    ) then
        bestow.ai.setBlackboard(entity, "target", e)
    end
end

-- Low-level: Navigation mesh and perception tuning
bestow.ai.core.setSightRange(entity, 30.0)
bestow.ai.core.setSightAngle(entity, 1.0)  -- ~57 degrees half-angle
bestow.ai.core.setHearingRange(entity, 15.0)

bestow.ai.core.setMaxSpeed(entity, 5.0)
bestow.ai.core.setMaxAcceleration(entity, 10.0)
bestow.ai.core.setAvoidanceRadius(entity, 0.5)

bestow.ai.core.setPatrolPath(entity, {
    { x = 0, y = 0, z = 0 },
    { x = 10, y = 0, z = 0 },
    { x = 10, y = 0, z = 10 },
    { x = 0, y = 0, z = 10 },
}, true)

local path, err = bestow.ai.core.findPath3D(
    { x = 0, y = 0, z = 0 },
    { x = 50, y = 0, z = 50 },
    0.5
)
if path then
    for _, waypoint in ipairs(path) do
        print(waypoint.x, waypoint.y, waypoint.z)
    end
end

local perceived = bestow.ai.core.getPerceivedEntities(entity)

-- Crowd simulation
bestow.ai.core.addToCrowd(entity)
bestow.ai.core.setCrowdSeparation(1.5)
bestow.ai.core.setCrowdAlignment(1.0)
bestow.ai.core.setCrowdCohesion(0.8)
```

## C++ Examples

```cpp
// High-level usage
ai->attachBehaviorTree(guard, "ai/patrol_guard.lua");
ai->setBlackboard(guard, "alertLevel", 0);
ai->setNavigationTarget(guard, Vec3{200, 0, 100});

auto nearby = ai->findInRadius(Vec3{100, 0, 50}, 20.0f);
bool canSee = ai->hasLineOfSight(Vec3{100, 1.5f, 50}, Vec3{110, 1.5f, 60});

// Low-level usage
aiCore->loadNavMesh(navMeshAsset);
if (aiCore->hasNavMesh()) {
    auto pathResult = aiCore->findPath3D(start, end, 0.5f);
    if (pathResult) {
        for (const auto& waypoint : pathResult.value()) {
            // Follow waypoint
        }
    }
}

aiCore->setSightRange(guard, 30.0f);
aiCore->setSightAngle(guard, 1.0f);
aiCore->setHearingRange(guard, 15.0f);

aiCore->setMaxSpeed(guard, 5.0f);
aiCore->setMaxAcceleration(guard, 10.0f);
aiCore->setAvoidanceRadius(guard, 0.5f);

std::vector<Vec3> waypoints = {{0,0,0}, {10,0,0}, {10,0,10}, {0,0,10}};
aiCore->setPatrolPath(guard, waypoints, true);

auto perceived = aiCore->getPerceivedEntities(guard);
auto closest = aiCore->findClosestEntity(Vec3{100, 0, 50}, 0xFFFF);

aiCore->addToCrowd(guard);
aiCore->setCrowdSeparation(1.5f);
aiCore->setCrowdAlignment(1.0f);
aiCore->setCrowdCohesion(0.8f);
```
