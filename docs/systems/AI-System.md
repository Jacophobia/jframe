# Bestow AI System Documentation

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Behavior Trees](#behavior-trees)
4. [Navigation System](#navigation-system)
5. [Steering Behaviors](#steering-behaviors)
6. [Patrol Behaviors](#patrol-behaviors)
7. [Spatial Queries](#spatial-queries)
8. [Common AI Patterns](#common-ai-patterns)
9. [Integration Examples](#integration-examples)
10. [Performance Considerations](#performance-considerations)

## Overview

The Bestow AI System provides tools for creating intelligent game entities through:

- **Behavior Trees**: Hierarchical decision-making structures for complex AI behaviors
- **Navigation Meshes**: Pathfinding on walkable surfaces using Recast/Detour
- **Steering Behaviors**: Movement patterns like seek, flee, and arrive
- **Patrol Behaviors**: Simple back-and-forth patrol movement
- **Spatial Queries**: Finding nearby entities, line-of-sight checks, and proximity detection

The AI system integrates tightly with the Physics System for spatial queries and the Asset System for loading behavior trees and navigation meshes.

### Key Features

- Per-entity blackboard for behavior tree data storage
- Pathfinding with agent radius support
- Physics-based line-of-sight and spatial queries
- Built-in patrol behavior for common enemy patterns
- Navigation target tracking with speed/acceleration limits

## Architecture

### Dependencies

```cpp
import bestow.ai;        // AI system interface
import bestow.physics;   // Required: spatial queries, line-of-sight
import bestow.assets;    // Required: behavior tree/navmesh loading
```

### System Creation

```cpp
#include <memory>
import bestow.ai.impl;

// Create dependencies first
auto physicsSystem = createPhysicsSystem();
auto assetSystem = std::make_unique<AssetSystem>();

// Create AI system
auto aiSystem = createAISystem(physicsSystem.get(), assetSystem.get());

// Initialize (optional - required for some implementations)
auto* implPtr = dynamic_cast<AISystem*>(aiSystem.get());
if (implPtr) {
    implPtr->initialize();
}
```

### Update Loop

The AI system should be updated every fixed timestep:

```cpp
void Game::updateFixed(DeltaTime dt) {
    aiSystem->update(dt);

    // The AI system will:
    // - Execute behavior trees for all entities
    // - Update patrol behaviors
    // - Apply steering forces to entities with navigation targets
}
```

## Behavior Trees

Behavior trees provide hierarchical decision-making for AI entities. Each entity can have an attached behavior tree asset and a blackboard for storing runtime data.

### Attaching a Behavior Tree

```cpp
Entity enemy = entities->createEntity();

// Load behavior tree asset
AssetHandle treeAsset = assets->registerAsset(
    AssetType::BehaviorTree,
    "ai/enemy_patrol.lua"
);

// Attach to entity
aiSystem->attachBehaviorTree(enemy, treeAsset);
```

### Blackboard System

The blackboard is a key-value store using `std::any` for storing arbitrary data types:

```cpp
// Store different data types
aiSystem->setBehaviorTreeBlackboard(enemy, "health", 100);
aiSystem->setBehaviorTreeBlackboard(enemy, "target", playerEntity);
aiSystem->setBehaviorTreeBlackboard(enemy, "patrolState", std::string("forward"));
aiSystem->setBehaviorTreeBlackboard(enemy, "aggressive", true);
aiSystem->setBehaviorTreeBlackboard(enemy, "targetPosition", Vec2{100.0f, 200.0f});

// Retrieve blackboard values
std::any healthData = aiSystem->getBehaviorTreeBlackboard(enemy, "health");
if (healthData.has_value()) {
    int health = std::any_cast<int>(healthData);
    std::cout << "Enemy health: " << health << std::endl;
}

// Check for non-existent keys
std::any missing = aiSystem->getBehaviorTreeBlackboard(enemy, "nonexistent");
if (!missing.has_value()) {
    std::cout << "Key not found in blackboard" << std::endl;
}
```

### Detaching Behavior Trees

```cpp
// Remove behavior tree from entity
aiSystem->detachBehaviorTree(enemy);

// Check if entity has a tree
if (aiSystem->hasBehaviorTree(enemy)) {
    std::cout << "Enemy has active behavior tree" << std::endl;
}
```

### Behavior Tree Structure

While the current implementation focuses on blackboard management and tree attachment, a full behavior tree system typically includes:

#### Node Types

- **Composite Nodes**: Sequence, Selector, Parallel
- **Decorator Nodes**: Inverter, Repeater, UntilFail
- **Leaf Nodes**: Actions and Conditions

#### Example Lua Behavior Tree (Future)

```lua
-- ai/enemy_patrol.lua
return {
    type = "Selector",
    children = {
        -- Combat behavior
        {
            type = "Sequence",
            children = {
                { type = "Condition", check = "hasTarget" },
                { type = "Condition", check = "targetInRange" },
                { type = "Action", action = "attackTarget" }
            }
        },
        -- Patrol behavior
        {
            type = "Sequence",
            children = {
                { type = "Action", action = "moveToWaypoint" },
                { type = "Action", action = "waitAtWaypoint" }
            }
        }
    }
}
```

## Navigation System

The navigation system uses Recast/Detour for pathfinding on navigation meshes.

### Loading a Navigation Mesh

```cpp
// Register navmesh asset
AssetHandle navMesh = assets->registerAsset(
    AssetType::NavMesh,
    "levels/level1_navmesh.bin"
);

// Load navmesh into AI system
aiSystem->loadNavMesh(navMesh);

// Check if navmesh is loaded
if (aiSystem->hasNavMesh()) {
    std::cout << "Navigation mesh loaded successfully" << std::endl;
}
```

### Unloading Navigation Mesh

```cpp
// Unload current navmesh
aiSystem->unloadNavMesh();

// Safe to call even if no navmesh is loaded
aiSystem->unloadNavMesh();  // No-op
```

### Pathfinding

Find a path between two points on the navigation mesh:

```cpp
// Create path query
NavMeshQuery query{
    .start = {playerPos.x, playerPos.y},
    .end = {targetPos.x, targetPos.y},
    .agentRadius = 0.5f  // Agent size for collision avoidance
};

// Find path
std::optional<NavigationPath> path = aiSystem->findPath(query);

if (path.has_value()) {
    std::cout << "Path found with " << path->waypoints.size()
              << " waypoints" << std::endl;
    std::cout << "Total path length: " << path->totalLength << std::endl;
    std::cout << "Path complete: " << path->isComplete << std::endl;

    // Iterate through waypoints
    for (const Vec2& waypoint : path->waypoints) {
        std::cout << "Waypoint: (" << waypoint.x << ", "
                  << waypoint.y << ")" << std::endl;
    }
} else {
    std::cout << "No path found - target unreachable" << std::endl;
}
```

### Navigation Path Structure

```cpp
struct NavigationPath {
    std::vector<Vec2> waypoints;  // Points along the path
    float totalLength = 0.0f;     // Total distance in world units
    bool isComplete = false;      // True if path reaches goal
};
```

### Point Queries

Check if a point is on the navigation mesh:

```cpp
Vec2 position{100.0f, 50.0f};

// Check if point is valid
if (aiSystem->isPointOnNavMesh(position)) {
    std::cout << "Position is walkable" << std::endl;
}

// Get closest valid point on navmesh
std::optional<Vec2> closest = aiSystem->getClosestPointOnNavMesh(position);
if (closest.has_value()) {
    std::cout << "Closest navmesh point: ("
              << closest->x << ", " << closest->y << ")" << std::endl;
}
```

### Pathfinding Without Navigation Mesh

If no navigation mesh is loaded, `findPath()` returns a straight-line path:

```cpp
// No navmesh loaded
NavMeshQuery query{
    .start = {0.0f, 0.0f},
    .end = {100.0f, 100.0f}
};

std::optional<NavigationPath> path = aiSystem->findPath(query);
if (path.has_value()) {
    // Returns 2 waypoints: start and end
    // totalLength = Euclidean distance
    // isComplete = true
}
```

## Steering Behaviors

Steering behaviors provide smooth movement toward navigation targets.

### Setting Navigation Targets

```cpp
Entity enemy = entities->createEntity();

// Set target position
Vec2 targetPos{500.0f, 300.0f};
aiSystem->setNavigationTarget(enemy, targetPos);

// Configure movement parameters
aiSystem->setMaxSpeed(enemy, 150.0f);           // Units per second
aiSystem->setMaxAcceleration(enemy, 750.0f);    // Units per second squared

// Check current target
std::optional<Vec2> target = aiSystem->getNavigationTarget(enemy);
if (target.has_value()) {
    std::cout << "Enemy navigating to ("
              << target->x << ", " << target->y << ")" << std::endl;
}

// Clear target
aiSystem->clearNavigationTarget(enemy);
```

### Steering Behavior Integration

The AI system calculates steering forces and applies them through the physics system:

```cpp
void Game::updateAI(DeltaTime dt) {
    aiSystem->update(dt);

    // The update() method will:
    // 1. For each entity with a navigation target:
    //    - Calculate desired velocity toward target
    //    - Apply steering force (desired - current)
    //    - Clamp by maxAcceleration
    //    - Update velocity through physics system
}
```

### Common Steering Patterns

#### Seek (Move Toward Target)

```cpp
void seekTarget(Entity entity, Vec2 target) {
    aiSystem->setNavigationTarget(entity, target);
    aiSystem->setMaxSpeed(entity, 100.0f);
    aiSystem->setMaxAcceleration(entity, 500.0f);
}
```

#### Flee (Move Away From Target)

```cpp
void fleeTarget(Entity entity, Vec2 threat) {
    Vec2 entityPos = physics->getPosition(entity);
    Vec2 fleeDirection{
        entityPos.x - threat.x,
        entityPos.y - threat.y
    };

    // Normalize and scale
    float length = std::sqrt(fleeDirection.x * fleeDirection.x +
                            fleeDirection.y * fleeDirection.y);
    if (length > 0.001f) {
        fleeDirection.x /= length;
        fleeDirection.y /= length;
    }

    Vec2 fleeTarget{
        entityPos.x + fleeDirection.x * 1000.0f,
        entityPos.y + fleeDirection.y * 1000.0f
    };

    aiSystem->setNavigationTarget(entity, fleeTarget);
    aiSystem->setMaxSpeed(entity, 200.0f);  // Flee faster
}
```

#### Arrive (Slow Down Near Target)

```cpp
void arriveAtTarget(Entity entity, Vec2 target) {
    Vec2 entityPos = physics->getPosition(entity);
    float dx = target.x - entityPos.x;
    float dy = target.y - entityPos.y;
    float distance = std::sqrt(dx * dx + dy * dy);

    // Slow down within arrival radius
    constexpr float ARRIVAL_RADIUS = 100.0f;

    if (distance < ARRIVAL_RADIUS) {
        float speedScale = distance / ARRIVAL_RADIUS;
        aiSystem->setMaxSpeed(entity, 100.0f * speedScale);
    } else {
        aiSystem->setMaxSpeed(entity, 100.0f);
    }

    aiSystem->setNavigationTarget(entity, target);
    aiSystem->setMaxAcceleration(entity, 500.0f);
}
```

## Patrol Behaviors

Simple patrol behavior for back-and-forth movement.

### Setting Up Patrol

```cpp
Entity enemy = entities->createEntity();

// Create physics body (required for patrol to work)
PhysicsBodyDef bodyDef{
    .type = BodyType::Dynamic,
    .transform = {.x = 200.0f, .y = 100.0f}
};
physics->createBody(enemy, bodyDef);

// Configure patrol behavior
PatrolBehavior patrol{
    .startX = 200.0f,      // Center X position
    .range = 150.0f,       // Patrol 150 units left and right
    .speed = 75.0f,        // Move at 75 units/second
    .movingRight = true    // Initial direction
};

aiSystem->setPatrolBehavior(enemy, patrol);
```

### Patrol Behavior Details

The patrol behavior automatically:
- Moves the entity back and forth between `startX - range` and `startX + range`
- Flips direction when reaching bounds
- Updates the `movingRight` flag for sprite flipping
- Applies horizontal velocity through the physics system

```cpp
// The AI system update loop handles patrol:
void AISystem::update(DeltaTime dt) {
    for (auto& [entityId, ai] : aiComponents_) {
        if (ai.patrol.has_value()) {
            Vec2 pos = physicsSystem_->getPosition(entity);

            // Calculate bounds
            float leftBound = patrol.startX - patrol.range;
            float rightBound = patrol.startX + patrol.range;

            // Flip direction at bounds
            if (patrol.movingRight && pos.x >= rightBound) {
                patrol.movingRight = false;
            } else if (!patrol.movingRight && pos.x <= leftBound) {
                patrol.movingRight = true;
            }

            // Apply velocity
            float xVel = patrol.movingRight ? patrol.speed : -patrol.speed;
            physicsSystem_->setVelocity(entity, {xVel, currentVel.y});
        }
    }
}
```

### Reading Patrol State

```cpp
// Get current patrol state
std::optional<PatrolBehavior> patrol = aiSystem->getPatrolBehavior(enemy);
if (patrol.has_value()) {
    // Use movingRight for sprite rendering
    if (patrol->movingRight) {
        renderer->setFlipX(enemy, false);
    } else {
        renderer->setFlipX(enemy, true);
    }
}

// Clear patrol behavior
aiSystem->clearPatrolBehavior(enemy);
```

### Patrol with Platformer Physics

```cpp
// Platformer enemy with patrol
Entity platformerEnemy = entities->createEntity();

PhysicsBodyDef bodyDef{
    .type = BodyType::Dynamic,
    .transform = {.x = 300.0f, .y = 200.0f},
    .fixedRotation = true  // Don't rotate when turning
};
physics->createBody(platformerEnemy, bodyDef);

PatrolBehavior patrol{
    .startX = 300.0f,
    .range = 200.0f,      // Patrol 200 units in each direction
    .speed = 100.0f,
    .movingRight = true
};

aiSystem->setPatrolBehavior(platformerEnemy, patrol);

// Note: Y velocity is preserved by patrol behavior,
// allowing gravity to work normally for platformer physics
```

## Spatial Queries

The AI system provides spatial queries through integration with the physics system.

### Find Entities in Radius

Find all entities within a circular area:

```cpp
Vec2 center{250.0f, 250.0f};
float radius = 100.0f;
CollisionMask mask = 0xFFFF;  // All layers

std::vector<Entity> nearby = aiSystem->findEntitiesInRadius(center, radius, mask);

std::cout << "Found " << nearby.size() << " entities nearby" << std::endl;

for (Entity entity : nearby) {
    Vec2 pos = physics->getPosition(entity);
    float dx = pos.x - center.x;
    float dy = pos.y - center.y;
    float distance = std::sqrt(dx * dx + dy * dy);

    std::cout << "Entity " << static_cast<uint32_t>(entity)
              << " at distance " << distance << std::endl;
}
```

### Find Closest Entity

Find the single closest entity to a position:

```cpp
Vec2 searchPos{100.0f, 100.0f};
CollisionMask mask = 0x0001;  // Only search layer 1

std::optional<Entity> closest = aiSystem->findClosestEntity(searchPos, mask);

if (closest.has_value()) {
    Vec2 pos = physics->getPosition(closest.value());
    float dx = pos.x - searchPos.x;
    float dy = pos.y - searchPos.y;
    float distance = std::sqrt(dx * dx + dy * dy);

    std::cout << "Closest entity at distance: " << distance << std::endl;
} else {
    std::cout << "No entities found" << std::endl;
}
```

### Line of Sight

Check if there's a clear path between two points:

```cpp
Vec2 enemyPos = physics->getPosition(enemy);
Vec2 playerPos = physics->getPosition(player);
CollisionMask obstacleMask = 0x0002;  // Only walls block LOS

bool canSeePlayer = aiSystem->hasLineOfSight(enemyPos, playerPos, obstacleMask);

if (canSeePlayer) {
    std::cout << "Enemy can see player - start attacking!" << std::endl;
    // Trigger aggressive behavior
    aiSystem->setBehaviorTreeBlackboard(enemy, "aggressive", true);
    aiSystem->setNavigationTarget(enemy, playerPos);
} else {
    std::cout << "Player hidden behind obstacle" << std::endl;
}
```

### Line of Sight Implementation

The line-of-sight check performs a physics raycast:

```cpp
bool AISystem::hasLineOfSight(Vec2 from, Vec2 to,
                              CollisionMask obstacleMask) const {
    // Calculate direction and distance
    Vec2 direction{to.x - from.x, to.y - from.y};
    float distance = std::sqrt(direction.x * direction.x +
                              direction.y * direction.y);

    if (distance < 0.0001f) return true;  // Same position

    // Raycast for obstacles
    auto hit = physicsSystem_->raycast(from, direction, distance, obstacleMask);

    // No hit = clear line of sight
    return !hit.has_value();
}
```

## Common AI Patterns

### Guard AI

Enemy that patrols and chases player when in range:

```cpp
class GuardAI {
public:
    void update(Entity guard, IAISystem* ai, IPhysicsSystem* physics,
                Entity player, DeltaTime dt) {
        Vec2 guardPos = physics->getPosition(guard);
        Vec2 playerPos = physics->getPosition(player);

        // Calculate distance to player
        float dx = playerPos.x - guardPos.x;
        float dy = playerPos.y - guardPos.y;
        float distance = std::sqrt(dx * dx + dy * dy);

        constexpr float DETECTION_RADIUS = 200.0f;
        constexpr CollisionMask OBSTACLE_MASK = 0x0002;  // Walls

        // Check if player is in range and visible
        bool playerDetected = (distance < DETECTION_RADIUS) &&
                             ai->hasLineOfSight(guardPos, playerPos, OBSTACLE_MASK);

        if (playerDetected) {
            // Chase player
            ai->clearPatrolBehavior(guard);
            ai->setNavigationTarget(guard, playerPos);
            ai->setMaxSpeed(guard, 150.0f);  // Chase speed
            ai->setBehaviorTreeBlackboard(guard, "state", std::string("chase"));
        } else {
            // Return to patrol
            if (!ai->getPatrolBehavior(guard).has_value()) {
                PatrolBehavior patrol{
                    .startX = guardPos.x,
                    .range = 100.0f,
                    .speed = 75.0f,
                    .movingRight = true
                };
                ai->setPatrolBehavior(guard, patrol);
            }
            ai->clearNavigationTarget(guard);
            ai->setBehaviorTreeBlackboard(guard, "state", std::string("patrol"));
        }
    }
};
```

### Fleeing AI

Enemy that runs away when player gets too close:

```cpp
class FleeingAI {
public:
    void update(Entity coward, IAISystem* ai, IPhysicsSystem* physics,
                Entity player, DeltaTime dt) {
        Vec2 cowardPos = physics->getPosition(coward);
        Vec2 playerPos = physics->getPosition(player);

        float dx = playerPos.x - cowardPos.x;
        float dy = playerPos.y - cowardPos.y;
        float distance = std::sqrt(dx * dx + dy * dy);

        constexpr float FLEE_RADIUS = 150.0f;

        if (distance < FLEE_RADIUS) {
            // Calculate flee direction (away from player)
            Vec2 fleeDir{-dx, -dy};
            float length = std::sqrt(fleeDir.x * fleeDir.x +
                                    fleeDir.y * fleeDir.y);

            if (length > 0.001f) {
                fleeDir.x /= length;
                fleeDir.y /= length;
            }

            // Set flee target far away
            Vec2 fleeTarget{
                cowardPos.x + fleeDir.x * 500.0f,
                cowardPos.y + fleeDir.y * 500.0f
            };

            ai->setNavigationTarget(coward, fleeTarget);
            ai->setMaxSpeed(coward, 200.0f);  // Panic speed
            ai->setBehaviorTreeBlackboard(coward, "state", std::string("fleeing"));
        } else {
            // Safe distance - idle
            ai->clearNavigationTarget(coward);
            ai->setBehaviorTreeBlackboard(coward, "state", std::string("idle"));
        }
    }
};
```

### Swarm AI

Multiple entities that maintain distance from each other:

```cpp
class SwarmAI {
public:
    void update(Entity agent, IAISystem* ai, IPhysicsSystem* physics,
                Vec2 swarmCenter, DeltaTime dt) {
        Vec2 agentPos = physics->getPosition(agent);

        constexpr float SEPARATION_RADIUS = 50.0f;
        constexpr float COHESION_RADIUS = 200.0f;
        constexpr CollisionMask SWARM_MASK = 0x0004;  // Swarm layer

        // Find nearby swarm members
        std::vector<Entity> nearby = ai->findEntitiesInRadius(
            agentPos, COHESION_RADIUS, SWARM_MASK
        );

        Vec2 separationForce{0.0f, 0.0f};
        Vec2 cohesionForce{0.0f, 0.0f};
        int nearbyCount = 0;

        for (Entity other : nearby) {
            if (other == agent) continue;

            Vec2 otherPos = physics->getPosition(other);
            float dx = otherPos.x - agentPos.x;
            float dy = otherPos.y - agentPos.y;
            float distance = std::sqrt(dx * dx + dy * dy);

            if (distance < SEPARATION_RADIUS && distance > 0.001f) {
                // Too close - separate
                separationForce.x -= dx / distance;
                separationForce.y -= dy / distance;
            }

            // Cohesion (move toward average position)
            cohesionForce.x += dx;
            cohesionForce.y += dy;
            nearbyCount++;
        }

        // Calculate target position
        Vec2 target = agentPos;

        if (nearbyCount > 0) {
            cohesionForce.x /= nearbyCount;
            cohesionForce.y /= nearbyCount;

            target.x = agentPos.x + separationForce.x * 2.0f + cohesionForce.x * 0.5f;
            target.y = agentPos.y + separationForce.y * 2.0f + cohesionForce.y * 0.5f;
        } else {
            // No nearby members - move toward swarm center
            target = swarmCenter;
        }

        ai->setNavigationTarget(agent, target);
        ai->setMaxSpeed(agent, 100.0f);
        ai->setMaxAcceleration(agent, 400.0f);
    }
};
```

### Turret AI

Stationary enemy that rotates to track player:

```cpp
class TurretAI {
public:
    void update(Entity turret, IAISystem* ai, IPhysicsSystem* physics,
                Entity player, DeltaTime dt) {
        Vec2 turretPos = physics->getPosition(turret);
        Vec2 playerPos = physics->getPosition(player);

        constexpr float ATTACK_RANGE = 300.0f;
        constexpr CollisionMask OBSTACLE_MASK = 0x0002;

        // Check if player is in range and visible
        float dx = playerPos.x - turretPos.x;
        float dy = playerPos.y - turretPos.y;
        float distance = std::sqrt(dx * dx + dy * dy);

        bool canTarget = (distance < ATTACK_RANGE) &&
                        ai->hasLineOfSight(turretPos, playerPos, OBSTACLE_MASK);

        if (canTarget) {
            // Store target position in blackboard for weapon system
            ai->setBehaviorTreeBlackboard(turret, "targetPos", playerPos);
            ai->setBehaviorTreeBlackboard(turret, "hasTarget", true);

            // Calculate aim angle
            float angle = std::atan2(dy, dx);
            ai->setBehaviorTreeBlackboard(turret, "aimAngle", angle);
        } else {
            // Lost target
            ai->setBehaviorTreeBlackboard(turret, "hasTarget", false);
        }
    }
};
```

## Integration Examples

### Complete Enemy Setup

```cpp
Entity createPatrollingEnemy(Systems& sys, float x, float y, float patrolRange) {
    // Create entity
    Entity enemy = sys.entities->createEntity();

    // Setup physics
    PhysicsBodyDef bodyDef{
        .type = BodyType::Dynamic,
        .transform = {.x = x, .y = y},
        .fixedRotation = true,
        .density = 1.0f,
        .friction = 0.3f
    };
    sys.physics->createBody(enemy, bodyDef);

    // Setup patrol AI
    PatrolBehavior patrol{
        .startX = x,
        .range = patrolRange,
        .speed = 75.0f,
        .movingRight = true
    };
    sys.ai->setPatrolBehavior(enemy, patrol);

    // Setup behavior tree
    AssetHandle treeAsset = sys.assets->registerAsset(
        AssetType::BehaviorTree,
        "ai/enemy_patrol.lua"
    );
    sys.ai->attachBehaviorTree(enemy, treeAsset);

    // Initialize blackboard
    sys.ai->setBehaviorTreeBlackboard(enemy, "health", 100);
    sys.ai->setBehaviorTreeBlackboard(enemy, "aggressive", false);
    sys.ai->setBehaviorTreeBlackboard(enemy, "detectionRadius", 200.0f);

    return enemy;
}
```

### Navigation-Based Movement

```cpp
void moveEnemyToPosition(Entity enemy, Vec2 targetPos,
                         IAISystem* ai, IAssetSystem* assets) {
    // Find path using navmesh
    Vec2 enemyPos = ai->getNavigationTarget(enemy).value_or(Vec2{0, 0});

    NavMeshQuery query{
        .start = enemyPos,
        .end = targetPos,
        .agentRadius = 0.5f
    };

    std::optional<NavigationPath> path = ai->findPath(query);

    if (path.has_value() && !path->waypoints.empty()) {
        // Follow first waypoint
        Vec2 nextWaypoint = path->waypoints[0];
        ai->setNavigationTarget(enemy, nextWaypoint);
        ai->setMaxSpeed(enemy, 120.0f);
        ai->setMaxAcceleration(enemy, 600.0f);

        // Store full path in blackboard for visualization
        ai->setBehaviorTreeBlackboard(enemy, "currentPath", *path);
    } else {
        // No path found - clear navigation
        ai->clearNavigationTarget(enemy);
    }
}
```

### Combat AI State Machine

```cpp
enum class CombatState {
    Idle,
    Patrol,
    Chase,
    Attack,
    Retreat
};

class CombatAI {
public:
    void update(Entity enemy, IAISystem* ai, IPhysicsSystem* physics,
                Entity player, DeltaTime dt) {
        // Get current state from blackboard
        std::any stateData = ai->getBehaviorTreeBlackboard(enemy, "state");
        CombatState state = CombatState::Idle;
        if (stateData.has_value()) {
            state = std::any_cast<CombatState>(stateData);
        }

        Vec2 enemyPos = physics->getPosition(enemy);
        Vec2 playerPos = physics->getPosition(player);

        float dx = playerPos.x - enemyPos.x;
        float dy = playerPos.y - enemyPos.y;
        float distance = std::sqrt(dx * dx + dy * dy);

        // State transitions
        switch (state) {
            case CombatState::Idle:
            case CombatState::Patrol: {
                if (distance < 200.0f &&
                    ai->hasLineOfSight(enemyPos, playerPos)) {
                    state = CombatState::Chase;
                    ai->clearPatrolBehavior(enemy);
                }
                break;
            }

            case CombatState::Chase: {
                if (distance < 50.0f) {
                    state = CombatState::Attack;
                    ai->clearNavigationTarget(enemy);
                } else if (distance > 400.0f) {
                    state = CombatState::Patrol;
                    ai->clearNavigationTarget(enemy);
                } else {
                    ai->setNavigationTarget(enemy, playerPos);
                    ai->setMaxSpeed(enemy, 150.0f);
                }
                break;
            }

            case CombatState::Attack: {
                // Get health from blackboard
                std::any healthData = ai->getBehaviorTreeBlackboard(enemy, "health");
                int health = healthData.has_value() ?
                            std::any_cast<int>(healthData) : 100;

                if (health < 30) {
                    state = CombatState::Retreat;
                } else if (distance > 80.0f) {
                    state = CombatState::Chase;
                }
                break;
            }

            case CombatState::Retreat: {
                if (distance > 300.0f) {
                    state = CombatState::Patrol;
                } else {
                    // Flee away from player
                    Vec2 fleeDir{-dx, -dy};
                    float length = std::sqrt(fleeDir.x * fleeDir.x +
                                            fleeDir.y * fleeDir.y);
                    if (length > 0.001f) {
                        fleeDir.x /= length;
                        fleeDir.y /= length;
                    }

                    Vec2 fleeTarget{
                        enemyPos.x + fleeDir.x * 400.0f,
                        enemyPos.y + fleeDir.y * 400.0f
                    };
                    ai->setNavigationTarget(enemy, fleeTarget);
                    ai->setMaxSpeed(enemy, 200.0f);
                }
                break;
            }
        }

        // Update blackboard
        ai->setBehaviorTreeBlackboard(enemy, "state", state);
    }
};
```

## Performance Considerations

### Spatial Query Optimization

```cpp
// BAD: Query every frame
void update(DeltaTime dt) {
    for (Entity enemy : enemies) {
        Vec2 pos = physics->getPosition(enemy);
        auto nearby = ai->findEntitiesInRadius(pos, 200.0f);  // Expensive!
        // Process nearby entities...
    }
}

// GOOD: Query at intervals
class EnemySystem {
    float spatialQueryTimer_ = 0.0f;
    std::unordered_map<Entity, std::vector<Entity>> cachedNearby_;

public:
    void update(DeltaTime dt) {
        spatialQueryTimer_ += dt;

        // Only query every 0.5 seconds
        if (spatialQueryTimer_ >= 0.5f) {
            spatialQueryTimer_ = 0.0f;

            for (Entity enemy : enemies) {
                Vec2 pos = physics->getPosition(enemy);
                cachedNearby_[enemy] = ai->findEntitiesInRadius(pos, 200.0f);
            }
        }

        // Use cached results
        for (Entity enemy : enemies) {
            const auto& nearby = cachedNearby_[enemy];
            // Process nearby entities...
        }
    }
};
```

### Pathfinding Optimization

```cpp
// BAD: Recalculate path every frame
void update(DeltaTime dt) {
    Vec2 enemyPos = physics->getPosition(enemy);
    Vec2 playerPos = physics->getPosition(player);

    auto path = ai->findPath(NavMeshQuery{enemyPos, playerPos});
    // Follow path...
}

// GOOD: Recalculate when needed
class PathfindingCache {
    NavigationPath currentPath_;
    Vec2 lastTargetPos_{0, 0};
    float recalcTimer_ = 0.0f;

public:
    void update(Entity enemy, Vec2 targetPos, IAISystem* ai,
                IPhysicsSystem* physics, DeltaTime dt) {
        recalcTimer_ += dt;

        float dx = targetPos.x - lastTargetPos_.x;
        float dy = targetPos.y - lastTargetPos_.y;
        float targetMoved = std::sqrt(dx * dx + dy * dy);

        // Recalculate if target moved significantly or timer expired
        if (targetMoved > 50.0f || recalcTimer_ > 1.0f) {
            Vec2 enemyPos = physics->getPosition(enemy);
            auto path = ai->findPath(NavMeshQuery{enemyPos, targetPos});

            if (path.has_value()) {
                currentPath_ = *path;
                lastTargetPos_ = targetPos;
                recalcTimer_ = 0.0f;
            }
        }

        // Follow cached path
        if (!currentPath_.waypoints.empty()) {
            ai->setNavigationTarget(enemy, currentPath_.waypoints[0]);
        }
    }
};
```

### Behavior Tree Optimization

```cpp
// Limit number of AI updates per frame
class AIManager {
    std::vector<Entity> allEnemies_;
    size_t updateIndex_ = 0;

public:
    void update(DeltaTime dt) {
        constexpr size_t ENEMIES_PER_FRAME = 10;

        size_t end = std::min(updateIndex_ + ENEMIES_PER_FRAME,
                             allEnemies_.size());

        for (size_t i = updateIndex_; i < end; ++i) {
            updateEnemyAI(allEnemies_[i], dt);
        }

        updateIndex_ = (end >= allEnemies_.size()) ? 0 : end;
    }
};
```

### Memory Management

```cpp
// Clean up AI components when entities are destroyed
void onEntityDestroyed(Entity entity, IAISystem* ai) {
    // Clear all AI data
    ai->detachBehaviorTree(entity);
    ai->clearNavigationTarget(entity);
    ai->clearPatrolBehavior(entity);

    // AI components are automatically removed from internal maps
}
```

## Best Practices

1. **Use Appropriate Query Frequencies**
   - Line-of-sight checks: Every 0.2-0.5 seconds
   - Spatial queries: Every 0.5-1.0 seconds
   - Pathfinding: Every 1-2 seconds or on significant target movement

2. **Leverage Collision Masks**
   - Use layer masks to filter spatial queries
   - Separate player, enemies, and obstacles into different layers
   - Reduces unnecessary entity processing

3. **Cache Expensive Calculations**
   - Store paths in blackboard
   - Cache nearby entity lists
   - Reuse navigation targets when possible

4. **Blackboard Organization**
   - Use consistent key naming conventions
   - Document expected data types
   - Clear unused blackboard entries

5. **Navigation Mesh Best Practices**
   - Keep navmesh resolution appropriate for game scale
   - Update navmesh when level geometry changes
   - Use agent radius to prevent wall-clipping

6. **Steering Behavior Tuning**
   - maxSpeed: Typical range 50-200 units/second
   - maxAcceleration: 2-5x the maxSpeed value
   - Tune values based on level layout and gameplay feel

7. **Integration with Other Systems**
   - Update AI system after physics step
   - Query physics for entity positions
   - Use events system for AI triggers (e.g., player spotted)

## Troubleshooting

### Pathfinding Issues

**Problem**: `findPath()` returns `std::nullopt`
- Check if navigation mesh is loaded: `hasNavMesh()`
- Verify start/end points are on navmesh: `isPointOnNavMesh()`
- Use `getClosestPointOnNavMesh()` to snap invalid points

**Problem**: Path goes through walls
- Increase agent radius in NavMeshQuery
- Rebuild navigation mesh with proper obstacle geometry

### Patrol Issues

**Problem**: Entity doesn't move during patrol
- Ensure entity has a physics body: `physics->hasBody(entity)`
- Check that physics body is Dynamic, not Static
- Verify patrol behavior is set: `getPatrolBehavior(entity)`

**Problem**: Entity patrols too far
- Adjust `patrol.range` value
- Ensure `patrol.startX` is set to initial spawn position

### Steering Issues

**Problem**: Entity moves too slowly toward target
- Increase `maxSpeed` value
- Increase `maxAcceleration` for faster response

**Problem**: Entity overshoots target
- Decrease `maxSpeed` near target (arrive behavior)
- Reduce `maxAcceleration` for smoother movement

### Spatial Query Issues

**Problem**: `findEntitiesInRadius()` returns empty
- Check if entities have physics bodies
- Verify collision masks match query mask
- Increase search radius

**Problem**: Line-of-sight always returns false
- Verify obstacle mask includes only walls, not entities
- Check raycast origin/target positions are valid

---

## Summary

The Bestow AI System provides a flexible foundation for game AI through:

- **Behavior trees** with per-entity blackboards for complex decision-making
- **Navigation meshes** powered by Recast/Detour for robust pathfinding
- **Steering behaviors** for smooth, natural movement
- **Patrol behaviors** for simple enemy patterns
- **Spatial queries** integrated with the physics system

By combining these features, you can create sophisticated AI behaviors ranging from simple patrolling enemies to complex combat encounters with tactical decision-making.

For questions or feature requests, refer to the Bestow technical documentation or the system implementation guide.
