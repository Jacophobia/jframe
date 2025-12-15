# AI System Developer Guide

## Overview

The Bestow AI System provides game AI capabilities including behavior trees, navigation meshes, pathfinding, and spatial queries. Built on industry-standard libraries (BehaviorTree.CPP and Recast/Detour), it integrates seamlessly with the physics system for steering behaviors and collision detection.

**Key Features:**
- **Behavior Trees**: Complex AI logic with blackboard data sharing (per-entity `std::any` storage)
- **NavMesh Pathfinding**: Recast/Detour-based navigation for 2D/2.5D games
- **Patrol Behavior**: Built-in back-and-forth movement with automatic velocity updates
- **Steering Behaviors**: Navigation target storage and movement parameters
- **Spatial Queries**: Efficient entity searches using physics system (findEntitiesInRadius, findClosestEntity)
- **Line of Sight**: Raycast-based visibility checks (hasLineOfSight)

**Dependencies:**
- Physics System (required for spatial queries, steering, and raycasting)
- Asset System (for loading behavior trees and navmeshes)

**Important:** Entities using AI features (patrol, navigation) must have physics bodies created via the Physics System.

---

## Core Concepts

### 1. Behavior Trees

> **STATUS: PLANNED** - Full BehaviorTree.CPP integration is planned but not yet implemented. The blackboard data storage is available now for use with your own custom AI logic.

Behavior trees organize AI decision-making hierarchically. Each entity can have:
- **One behavior tree** (loaded from asset via AssetSystem) - *PLANNED*
- **Blackboard data** (per-entity key-value storage using `std::any`) - *AVAILABLE NOW*
- **Execution state** (managed automatically during update) - *PLANNED*

**Node Types (via BehaviorTree.CPP):** *PLANNED FEATURE*
- **Action nodes**: Perform tasks (move, attack, play animation)
- **Condition nodes**: Check state (health low?, player visible?)
- **Composite nodes**: Control flow (sequence, selector, parallel)
- **Decorator nodes**: Modify behavior (repeat, invert, timeout)

**Current Status:**
- Blackboard data storage is fully functional
- Behavior tree loading, parsing, and execution are planned for a future release
- For now, use the blackboard with your own custom AI state machines or decision logic

### 2. Navigation

Navigation uses **Recast/Detour** for robust pathfinding:
- **NavMesh**: Walkable surface representation
- **Pathfinding**: A* on navmesh polygons
- **Dynamic queries**: Point containment, closest point, path calculation

**Coordinate System:**
- Bestow uses 2D (x, y) coordinates
- Detour uses 3D (x, y=0, z) internally
- The AI system handles conversion automatically

### 3. Patrol Behavior

The patrol system provides automatic back-and-forth movement:
- **Automatic velocity management**: AI system updates X velocity each frame based on direction
- **Boundary detection**: Flips direction at `startX ± range`
- **Y velocity preservation**: Maintains vertical velocity (for gravity, jumping)
- **Requires physics body**: Gracefully skips entities without physics bodies

### 4. Steering Behaviors

Simple movement behaviors integrated with physics:
- **Navigation target**: Store a target position (for your custom AI logic to use)
- **Max speed/acceleration**: Configurable movement limits (stored but not automatically applied)

**Important:** Navigation target is STORAGE ONLY. The AI system does NOT automatically move entities toward their targets. You must implement movement yourself using physics velocity or custom steering logic.

---

## API Reference

### Lifecycle

```cpp
class IAISystem {
    // Called each frame to update AI behaviors
    virtual void update(DeltaTime dt) = 0;
};
```

**Usage:**
```cpp
// In your game loop (called automatically by Engine)
aiSystem->update(deltaTime);
```

**What update() does:**
- Updates patrol behaviors (sets velocity for entities with patrol component)
- Processes behavior tree execution (when BehaviorTree.CPP is integrated)
- Does NOT automatically move entities toward navigation targets

---

### Behavior Trees

#### Attach/Detach

> **PLANNED FEATURE** - These methods exist in the interface but behavior tree execution is not yet implemented. Use blackboard data with custom AI logic for now.

```cpp
// Attach a behavior tree asset to an entity
void attachBehaviorTree(Entity entity, AssetHandle treeAsset);

// Remove behavior tree from entity
void detachBehaviorTree(Entity entity);

// Check if entity has a behavior tree
bool hasBehaviorTree(Entity entity) const;
```

**Example (PLANNED):**
```cpp
Entity enemy = entities->createEntity();

// Load and attach behavior tree (PLANNED - not yet functional)
AssetHandle enemyAI = assets->registerAsset(AssetType::BehaviorTree,
                                             ":assets:/ai/enemy_patrol.xml");
assets->loadAsset(enemyAI);
aiSystem->attachBehaviorTree(enemy, enemyAI);

// Later: check if entity has tree
if (aiSystem->hasBehaviorTree(enemy)) {
    // Entity has AI behavior
}

// Remove behavior tree
aiSystem->detachBehaviorTree(enemy);
```

**Notes:**
- Attaching a new tree replaces the old tree
- Detaching a tree does NOT clear blackboard data
- Safe to detach from non-existent entities (no-op)

#### Blackboard Data

The blackboard stores per-entity data accessible to behavior tree nodes and your custom AI logic.

```cpp
// Set a value in the blackboard
void setBehaviorTreeBlackboard(Entity entity,
                                const std::string& key,
                                const std::any& value);

// Get a value from the blackboard
std::any getBehaviorTreeBlackboard(Entity entity,
                                    const std::string& key) const;
```

**Supported Types:**
- Primitives: `int`, `float`, `bool`, `std::string`
- Custom types: `Vec2`, `Entity`, user structs
- Store using `std::any`, retrieve with `std::any_cast<T>()`

**Example:**
```cpp
// Store various types
aiSystem->setBehaviorTreeBlackboard(enemy, "health", 100);
aiSystem->setBehaviorTreeBlackboard(enemy, "alert", false);
aiSystem->setBehaviorTreeBlackboard(enemy, "patrol_point", Vec2{500.0f, 300.0f});
aiSystem->setBehaviorTreeBlackboard(enemy, "state", std::string("idle"));

// Retrieve values (must use correct type)
int health = std::any_cast<int>(
    aiSystem->getBehaviorTreeBlackboard(enemy, "health"));

bool isAlert = std::any_cast<bool>(
    aiSystem->getBehaviorTreeBlackboard(enemy, "alert"));

Vec2 target = std::any_cast<Vec2>(
    aiSystem->getBehaviorTreeBlackboard(enemy, "patrol_point"));
```

**Important Notes:**
- Blackboard persists even if behavior tree is detached
- Empty `std::any` returned for non-existent keys (check with `.has_value()`)
- Values are independent between entities
- Can overwrite with different type (type-safe via `std::any_cast`)
- Returns empty `std::any` for non-existent entities

**Type Safety:**
```cpp
// WRONG: Wrong type cast will throw
aiSystem->setBehaviorTreeBlackboard(enemy, "health", 100.0f); // float
int health = std::any_cast<int>(
    aiSystem->getBehaviorTreeBlackboard(enemy, "health")); // THROWS!

// CORRECT: Matching types
aiSystem->setBehaviorTreeBlackboard(enemy, "health", 100.0f);
float health = std::any_cast<float>(
    aiSystem->getBehaviorTreeBlackboard(enemy, "health"));

// SAFE: Check before casting
std::any value = aiSystem->getBehaviorTreeBlackboard(enemy, "health");
if (value.has_value()) {
    float health = std::any_cast<float>(value);
}
```

---

### Navigation

#### NavMesh Management

```cpp
// Load a navmesh asset
void loadNavMesh(AssetHandle navMeshAsset);

// Unload the current navmesh
void unloadNavMesh();

// Check if a navmesh is loaded
bool hasNavMesh() const;
```

**Example:**
```cpp
// Load level navmesh
AssetHandle navMesh = assets->registerAsset(AssetType::NavMesh,
                                             ":assets:/navmeshes/level1.bin");
assets->loadAsset(navMesh);
aiSystem->loadNavMesh(navMesh);

if (aiSystem->hasNavMesh()) {
    // Ready for pathfinding
}

// When changing levels
aiSystem->unloadNavMesh();
```

**Notes:**
- Loading a new navmesh replaces the old one
- Safe to unload when no navmesh is loaded (no-op)
- NavMesh format is binary (generated by Recast)

#### Pathfinding

```cpp
struct NavMeshQuery {
    Vec2 start;              // Starting position
    Vec2 end;                // Goal position
    float agentRadius = 0.5f; // Agent size (for collision avoidance)
};

struct NavigationPath {
    std::vector<Vec2> waypoints;  // Path waypoints (start to end)
    float totalLength = 0.0f;     // Total path distance
    bool isComplete = false;      // True if path reaches goal exactly
};

// Find a path on the navmesh
std::optional<NavigationPath> findPath(const NavMeshQuery& query) const;
```

**Example:**
```cpp
// Find path from enemy to player
Vec2 enemyPos = physics->getPosition(enemy);
Vec2 playerPos = physics->getPosition(player);

NavMeshQuery query{
    .start = enemyPos,
    .end = playerPos,
    .agentRadius = 0.5f  // Enemy collision radius
};

if (auto path = aiSystem->findPath(query)) {
    // Path found! Follow waypoints
    for (const Vec2& waypoint : path->waypoints) {
        // Use waypoint for navigation
    }

    float distance = path->totalLength;
    bool reachable = path->isComplete;
} else {
    // No path found or no navmesh loaded
}
```

**Return Values:**
- Returns `std::nullopt` if no navmesh loaded
- Returns `std::nullopt` if pathfinding fails
- Returns straight-line path if navmesh loaded but empty
- Path with zero length if start equals end

**Agent Radius:**
- Used for obstacle avoidance
- Larger radius = wider berth around obstacles
- Zero radius is valid (point agent)

#### Point Queries

```cpp
// Check if a point is on the navmesh
bool isPointOnNavMesh(Vec2 point) const;

// Find the closest point on the navmesh
std::optional<Vec2> getClosestPointOnNavMesh(Vec2 point) const;
```

**Example:**
```cpp
// Validate spawn position
Vec2 spawnPos{1000.0f, 500.0f};
if (!aiSystem->isPointOnNavMesh(spawnPos)) {
    // Snap to nearest walkable position
    if (auto nearest = aiSystem->getClosestPointOnNavMesh(spawnPos)) {
        spawnPos = *nearest;
    }
}

// Create enemy at valid position
physics->createBody(enemy, PhysicsBodyDef{
    .type = BodyType::Dynamic,
    .transform = {.x = spawnPos.x, .y = spawnPos.y}
});
```

**Return Values:**
- `isPointOnNavMesh()`: Returns `false` if no navmesh loaded or point not on mesh
- `getClosestPointOnNavMesh()`: Returns `std::nullopt` if no navmesh loaded

---

### Patrol Behavior

Built-in horizontal patrol behavior with automatic velocity updates.

```cpp
struct PatrolBehavior {
    float startX = 0.0f;      // Center X position of patrol
    float range = 100.0f;     // Distance to patrol in each direction
    float speed = 50.0f;      // Movement speed
    bool movingRight = true;  // Current direction (updated by system)
};

void setPatrolBehavior(Entity entity, const PatrolBehavior& patrol);
void clearPatrolBehavior(Entity entity);
std::optional<PatrolBehavior> getPatrolBehavior(Entity entity) const;
```

**Example:**
```cpp
// Create patrolling enemy
Entity enemy = entities->createEntity();

// Create physics body (REQUIRED for patrol)
physics->createBody(enemy, PhysicsBodyDef{
    .type = BodyType::Dynamic,
    .transform = {.x = 300.0f, .y = 100.0f}
});

// Setup patrol: moves between X=200 and X=400 at 75 units/sec
aiSystem->setPatrolBehavior(enemy, PatrolBehavior{
    .startX = 300.0f,
    .range = 100.0f,   // Patrols from 200 to 400
    .speed = 75.0f,
    .movingRight = true
});

// AI system will automatically (during update):
// - Set X velocity based on direction
// - Flip direction at range boundaries
// - Preserve Y velocity (for gravity, jumping)
```

**Behavior Details:**
- Automatically flips direction at `startX - range` and `startX + range`
- Updates X velocity, preserves Y velocity
- Requires physics body (gracefully skips entities without)
- `movingRight` field is updated by the system (read it to know current direction)

**Checking Direction:**
```cpp
// Get current patrol state
if (auto patrol = aiSystem->getPatrolBehavior(enemy)) {
    if (patrol->movingRight) {
        // Enemy is moving right, flip sprite
        sprite->flipX = false;
    } else {
        // Enemy is moving left, flip sprite
        sprite->flipX = true;
    }
}
```

**Notes:**
- Setting new patrol behavior replaces old behavior
- Safe to clear on non-existent entities (no-op)
- Returns `std::nullopt` for entities without patrol behavior

---

### Steering Behaviors

#### Navigation Target

```cpp
// Set a target position to move toward
void setNavigationTarget(Entity entity, Vec2 target);

// Clear navigation target (stop seeking)
void clearNavigationTarget(Entity entity);

// Get current navigation target
std::optional<Vec2> getNavigationTarget(Entity entity) const;
```

**Example:**
```cpp
// Store target for later use
Vec2 playerPos = physics->getPosition(player);
aiSystem->setNavigationTarget(enemy, playerPos);

// Later: retrieve and use in your AI logic
if (auto target = aiSystem->getNavigationTarget(enemy)) {
    // Implement your own movement logic here
    Vec2 enemyPos = physics->getPosition(enemy);
    Vec2 direction = {target->x - enemyPos.x, target->y - enemyPos.y};
    // ... calculate velocity and apply
}

// Stop chasing
aiSystem->clearNavigationTarget(enemy);
```

**CRITICAL:** Navigation target is STORAGE ONLY. The AI System does NOT automatically move entities toward their targets. You must implement movement yourself:

```cpp
// Example: Simple seek behavior (you implement this)
void seekTarget(Entity entity, Vec2 target, float dt) {
    Vec2 pos = physics->getPosition(entity);
    Vec2 toTarget = {target.x - pos.x, target.y - pos.y};
    float distance = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y);

    if (distance > 0.001f) {
        Vec2 velocity = {
            (toTarget.x / distance) * maxSpeed,
            (toTarget.y / distance) * maxSpeed
        };
        physics->setVelocity(entity, velocity);
    }
}
```

**Notes:**
- Setting new target replaces old target
- Safe to clear on non-existent entities (no-op)
- Returns `std::nullopt` for entities without target

#### Movement Parameters

```cpp
// Set maximum movement speed
void setMaxSpeed(Entity entity, float speed);

// Set maximum acceleration
void setMaxAcceleration(Entity entity, float acceleration);
```

**Example:**
```cpp
// Fast scout enemy
aiSystem->setMaxSpeed(scout, 200.0f);
aiSystem->setMaxAcceleration(scout, 1000.0f);

// Slow tank enemy
aiSystem->setMaxSpeed(tank, 50.0f);
aiSystem->setMaxAcceleration(tank, 250.0f);
```

**CRITICAL:** These values are STORAGE ONLY. The AI System does NOT use these values automatically. They are conveniences for your custom steering logic to query if needed.

**Notes:**
- Values are stored per-entity
- Safe to set on non-existent entities (no-op)
- No getter methods (stored for your use in custom AI)

---

### Spatial Queries

Efficient entity searches using the physics system's spatial partitioning.

#### Radius Search

```cpp
// Find all entities within a radius
std::vector<Entity> findEntitiesInRadius(
    Vec2 center,
    float radius,
    CollisionMask mask = 0xFFFF  // Collision layer filter
) const;
```

**Example:**
```cpp
// Find all enemies near player
Vec2 playerPos = physics->getPosition(player);
std::vector<Entity> nearbyEnemies = aiSystem->findEntitiesInRadius(
    playerPos,
    200.0f,  // 200 unit radius
    0x0002   // Enemy collision layer
);

for (Entity enemy : nearbyEnemies) {
    // React to nearby enemy
}
```

**Performance:**
- Uses physics system's optimized spatial queries
- O(log n) via broad-phase acceleration
- Returns empty vector if no entities found
- Returns empty vector if physics system unavailable

**Notes:**
- Negative radius returns empty vector
- Zero radius returns empty vector
- Collision mask filtering may not be fully implemented yet

#### Closest Entity

```cpp
// Find the closest entity to a position
std::optional<Entity> findClosestEntity(
    Vec2 position,
    CollisionMask mask = 0xFFFF
) const;
```

**Example:**
```cpp
// Find nearest health pickup
Vec2 playerPos = physics->getPosition(player);
auto nearest = aiSystem->findClosestEntity(playerPos, LAYER_PICKUP);

if (nearest) {
    Vec2 pickupPos = physics->getPosition(*nearest);
    // Navigate to pickup
}
```

**Search Parameters:**
- Searches within 2000 unit radius (hardcoded max distance)
- Returns `std::nullopt` if no entities in range
- Returns `std::nullopt` if physics system unavailable
- Collision mask parameter exists but may not filter correctly yet

#### Line of Sight

```cpp
// Check if there's a clear path between two points
bool hasLineOfSight(
    Vec2 from,
    Vec2 to,
    CollisionMask obstacleMask = 0xFFFF  // What blocks vision
) const;
```

**Example:**
```cpp
// Can enemy see player?
Vec2 enemyPos = physics->getPosition(enemy);
Vec2 playerPos = physics->getPosition(player);

if (aiSystem->hasLineOfSight(enemyPos, playerPos, LAYER_WALLS)) {
    // No walls blocking - enemy can see player
    aiSystem->setBehaviorTreeBlackboard(enemy, "player_visible", true);
} else {
    // Walls blocking - enemy cannot see player
    aiSystem->setBehaviorTreeBlackboard(enemy, "player_visible", false);
}
```

**Behavior:**
- Uses physics raycast internally
- Returns `true` if no collision between points
- Returns `true` for identical from/to (same position)
- Returns `false` if physics system unavailable

---

## Common NPC AI Patterns

### Pattern 1: Simple Patrol Enemy

Back-and-forth movement using the built-in patrol system.

```cpp
void createPatrolEnemy(Vec2 position, float patrolRange) {
    Entity enemy = entities->createEntity();

    // Physics body (REQUIRED)
    physics->createBody(enemy, PhysicsBodyDef{
        .type = BodyType::Dynamic,
        .transform = {.x = position.x, .y = position.y},
        .fixedRotation = true
    });

    // Collision shape
    physics->attachBox(enemy, 16.0f, 16.0f);

    // Patrol behavior - AI system handles velocity automatically
    aiSystem->setPatrolBehavior(enemy, PatrolBehavior{
        .startX = position.x,
        .range = patrolRange,
        .speed = 50.0f,
        .movingRight = true
    });
}

// In game update:
void update(DeltaTime dt) {
    aiSystem->update(dt); // Handles patrol velocity
    physics->update(dt);   // Applies velocity to position
}
```

### Pattern 2: Guard with Alert State

Enemy patrols normally, chases player when spotted.

```cpp
class GuardAI {
public:
    void update(DeltaTime dt) {
        Vec2 guardPos = physics->getPosition(guard_);
        Vec2 playerPos = physics->getPosition(player_);

        // Check if player is visible
        bool canSeePlayer = aiSystem->hasLineOfSight(guardPos, playerPos);
        float distToPlayer = distance(guardPos, playerPos);

        if (canSeePlayer && distToPlayer < 300.0f) {
            // Alert state - chase player
            if (isPatrolling_) {
                aiSystem->clearPatrolBehavior(guard_);
                isPatrolling_ = false;
            }
            aiSystem->setBehaviorTreeBlackboard(guard_, "state", std::string("alert"));
            chasePlayer(dt);
        } else {
            // Patrol state
            if (!isPatrolling_) {
                aiSystem->setPatrolBehavior(guard_, PatrolBehavior{
                    .startX = guardPos.x,
                    .range = 100.0f,
                    .speed = 50.0f
                });
                isPatrolling_ = true;
            }
            aiSystem->setBehaviorTreeBlackboard(guard_, "state", std::string("patrol"));
        }
    }

private:
    void chasePlayer(DeltaTime dt) {
        Vec2 guardPos = physics->getPosition(guard_);
        Vec2 playerPos = physics->getPosition(player_);

        // Simple chase logic
        Vec2 direction = {playerPos.x - guardPos.x, playerPos.y - guardPos.y};
        float dist = std::sqrt(direction.x * direction.x + direction.y * direction.y);

        if (dist > 0.001f) {
            Vec2 velocity = {
                (direction.x / dist) * 100.0f, // Chase speed
                (direction.y / dist) * 100.0f
            };
            physics->setVelocity(guard_, velocity);
        }
    }

    Entity guard_;
    Entity player_;
    bool isPatrolling_ = true;
};
```

### Pattern 3: Pathfinding Enemy

Uses navmesh to navigate around obstacles.

```cpp
class PathfindingEnemy {
public:
    void setDestination(Vec2 destination) {
        destination_ = destination;
        recalculatePath();
    }

    void update(DeltaTime dt) {
        if (!destination_.has_value()) return;

        // Recalculate path periodically
        recalcTimer_ += dt;
        if (recalcTimer_ >= 1.0f) { // Every second
            recalculatePath();
            recalcTimer_ = 0.0f;
        }

        // Follow current path
        if (!path_.empty() && waypointIndex_ < path_.size()) {
            Vec2 waypoint = path_[waypointIndex_];
            Vec2 selfPos = physics->getPosition(self_);

            float dist = distance(selfPos, waypoint);
            if (dist < 15.0f) { // Reached waypoint
                waypointIndex_++;
                if (waypointIndex_ >= path_.size()) {
                    // Reached destination
                    destination_.reset();
                    physics->setVelocity(self_, {0.0f, 0.0f});
                    return;
                }
            }

            // Move toward waypoint
            Vec2 direction = {waypoint.x - selfPos.x, waypoint.y - selfPos.y};
            float d = std::sqrt(direction.x * direction.x + direction.y * direction.y);
            if (d > 0.001f) {
                Vec2 velocity = {
                    (direction.x / d) * 120.0f,
                    (direction.y / d) * 120.0f
                };
                physics->setVelocity(self_, velocity);
            }
        }
    }

private:
    void recalculatePath() {
        if (!destination_.has_value() || !aiSystem->hasNavMesh()) {
            return;
        }

        Vec2 selfPos = physics->getPosition(self_);
        auto result = aiSystem->findPath({
            .start = selfPos,
            .end = *destination_,
            .agentRadius = 0.5f
        });

        if (result) {
            path_ = result->waypoints;
            waypointIndex_ = 0;
        } else {
            path_.clear();
        }
    }

    Entity self_;
    std::optional<Vec2> destination_;
    std::vector<Vec2> path_;
    size_t waypointIndex_ = 0;
    float recalcTimer_ = 0.0f;
};
```

### Pattern 4: Turret with Line of Sight

Stationary enemy that shoots when player is visible.

```cpp
class TurretAI {
public:
    void update(DeltaTime dt) {
        Vec2 turretPos = physics->getPosition(turret_);
        Vec2 playerPos = physics->getPosition(player_);

        // Check line of sight
        bool canSeePlayer = aiSystem->hasLineOfSight(turretPos, playerPos);
        float distToPlayer = distance(turretPos, playerPos);

        if (canSeePlayer && distToPlayer < 500.0f) {
            // Player is visible and in range
            shootCooldown_ -= dt;
            if (shootCooldown_ <= 0.0f) {
                shoot(playerPos);
                shootCooldown_ = 1.0f; // 1 second between shots
            }
            aiSystem->setBehaviorTreeBlackboard(turret_, "target_visible", true);
        } else {
            // Player not visible
            aiSystem->setBehaviorTreeBlackboard(turret_, "target_visible", false);
        }
    }

private:
    void shoot(Vec2 target) {
        // Create projectile toward target
        Vec2 turretPos = physics->getPosition(turret_);
        // ... projectile creation logic
    }

    Entity turret_;
    Entity player_;
    float shootCooldown_ = 0.0f;
};
```

### Pattern 5: Flocking Behavior

Multiple enemies move together as a group.

```cpp
class FlockingEnemy {
public:
    void update(DeltaTime dt) {
        Vec2 selfPos = physics->getPosition(self_);

        // Find nearby flock members
        std::vector<Entity> nearby = aiSystem->findEntitiesInRadius(
            selfPos, 100.0f, ENEMY_LAYER
        );

        Vec2 separation = {0.0f, 0.0f};
        Vec2 alignment = {0.0f, 0.0f};
        Vec2 cohesion = {0.0f, 0.0f};
        int count = 0;

        for (Entity other : nearby) {
            if (other == self_) continue;

            Vec2 otherPos = physics->getPosition(other);
            Vec2 diff = {selfPos.x - otherPos.x, selfPos.y - otherPos.y};
            float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);

            // Separation: Avoid crowding
            if (dist < 30.0f && dist > 0.001f) {
                separation.x += diff.x / dist;
                separation.y += diff.y / dist;
            }

            // Alignment: Match velocity
            Vec2 otherVel = physics->getVelocity(other);
            alignment.x += otherVel.x;
            alignment.y += otherVel.y;

            // Cohesion: Move toward center
            cohesion.x += otherPos.x;
            cohesion.y += otherPos.y;

            count++;
        }

        if (count > 0) {
            // Average alignment
            alignment.x /= count;
            alignment.y /= count;

            // Average cohesion, then seek toward it
            cohesion.x = cohesion.x / count - selfPos.x;
            cohesion.y = cohesion.y / count - selfPos.y;
        }

        // Combine forces
        Vec2 desired = {
            separation.x * 1.5f + alignment.x * 1.0f + cohesion.x * 1.0f,
            separation.y * 1.5f + alignment.y * 1.0f + cohesion.y * 1.0f
        };

        // Clamp to max speed
        float mag = std::sqrt(desired.x * desired.x + desired.y * desired.y);
        if (mag > maxSpeed_) {
            desired.x = (desired.x / mag) * maxSpeed_;
            desired.y = (desired.y / mag) * maxSpeed_;
        }

        physics->setVelocity(self_, desired);
    }

private:
    Entity self_;
    float maxSpeed_ = 80.0f;
};
```

---

## Best Practices

### 1. Always Create Physics Bodies for AI Entities

```cpp
// WRONG: No physics body
Entity enemy = entities->createEntity();
aiSystem->setPatrolBehavior(enemy, patrol); // Will silently fail

// CORRECT: Create physics body first
Entity enemy = entities->createEntity();
physics->createBody(enemy, bodyDef);
aiSystem->setPatrolBehavior(enemy, patrol); // Now works
```

### 2. Check Optional Returns

```cpp
// WRONG: Assuming path exists
auto path = aiSystem->findPath(query);
for (const Vec2& wp : path->waypoints) { ... } // CRASH if nullopt

// CORRECT: Check before use
if (auto path = aiSystem->findPath(query)) {
    for (const Vec2& wp : path->waypoints) { ... }
}
```

### 3. Throttle Expensive Operations

```cpp
// WRONG: Pathfinding every frame
void update(DeltaTime dt) {
    auto path = aiSystem->findPath({selfPos, targetPos}); // Every frame!
}

// CORRECT: Throttle recalculation
void update(DeltaTime dt) {
    recalcTimer += dt;
    if (recalcTimer >= 0.5f) {  // Every 0.5 seconds
        auto path = aiSystem->findPath({selfPos, targetPos});
        recalcTimer = 0.0f;
    }
}
```

### 4. Use Correct Types with Blackboard

```cpp
// WRONG: Type mismatch
aiSystem->setBehaviorTreeBlackboard(enemy, "health", 100.0f); // float
int health = std::any_cast<int>(
    aiSystem->getBehaviorTreeBlackboard(enemy, "health")); // Throws!

// CORRECT: Matching types
aiSystem->setBehaviorTreeBlackboard(enemy, "health", 100.0f);
float health = std::any_cast<float>(
    aiSystem->getBehaviorTreeBlackboard(enemy, "health"));
```

### 5. Update AI System Every Frame

```cpp
// WRONG: Patrol doesn't work
void gameUpdate(DeltaTime dt) {
    physics->update(dt); // Only physics
}

// CORRECT: Update AI before physics
void gameUpdate(DeltaTime dt) {
    aiSystem->update(dt); // Updates patrol velocities
    physics->update(dt);   // Applies velocities
}
```

### 6. Cache Spatial Queries

```cpp
// SLOW: Query every frame
void update(DeltaTime dt) {
    auto enemies = aiSystem->findEntitiesInRadius(playerPos, 500.0f);
    // Process enemies...
}

// FAST: Query on timer
float queryTimer = 0.0f;
std::vector<Entity> cachedEnemies;

void update(DeltaTime dt) {
    queryTimer += dt;
    if (queryTimer >= 0.5f) {  // Query every 0.5 seconds
        cachedEnemies = aiSystem->findEntitiesInRadius(playerPos, 500.0f);
        queryTimer = 0.0f;
    }
    // Use cachedEnemies...
}
```

---

## Common Pitfalls

1. **Missing Physics Body**: Patrol requires physics body (silently fails without)
2. **Not Checking Optionals**: `findPath()`, `getNavigationTarget()`, etc. return `std::nullopt` on failure
3. **Pathfinding Every Frame**: Very expensive, throttle to 0.5-1.0 second intervals
4. **Wrong Type Cast**: Blackboard throws `std::bad_any_cast` if types don't match
5. **Forgetting AI Update**: Patrol won't work if you don't call `aiSystem->update(dt)`
6. **Expecting Automatic Movement**: Navigation target is storage only, you must implement movement

---

## Integration Checklist

When integrating the AI system into your game:

- [ ] **Initialize dependencies first**: Physics and Asset systems must be initialized before AI system
- [ ] **Load navmesh early**: Load level navmesh during level load, before creating AI entities
- [ ] **Create physics bodies**: Entities need physics bodies for patrol and spatial queries
- [ ] **Call update()**: Include `aiSystem->update(dt)` in your game loop
- [ ] **Handle optional results**: Check `std::optional` returns before use
- [ ] **Use blackboard for state**: Store AI state in blackboard for debugging
- [ ] **Cache expensive queries**: Don't pathfind or query every frame
- [ ] **Implement custom movement**: Navigation targets are storage only

---

## Further Reading

- **BehaviorTree.CPP Documentation**: https://www.behaviortree.dev/
- **Recast/Detour Manual**: https://recastnav.com/
- **Steering Behaviors**: "Steering Behaviors For Autonomous Characters" by Craig Reynolds
- **Game AI Pro**: Book series with advanced AI techniques

---

## Version History

- **v1.0** (Current): Initial AI System with patrol, navigation, spatial queries, blackboard
- **Planned**: BehaviorTree.CPP integration, more steering behaviors, influence maps
