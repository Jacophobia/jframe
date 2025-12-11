# Bestow AI System Guide

## Overview

The Bestow AI System provides game AI capabilities including behavior trees, navigation meshes, pathfinding, and spatial queries. Built on industry-standard libraries (BehaviorTree.CPP and Recast/Detour), it integrates seamlessly with the physics system for steering behaviors and collision detection.

**Key Features:**
- **Behavior Trees**: Complex AI logic with blackboard data sharing
- **NavMesh Pathfinding**: Recast/Detour-based navigation for 2D/2.5D games
- **Steering Behaviors**: Built-in patrol, seek, and navigation following
- **Spatial Queries**: Efficient entity searches using physics system
- **Line of Sight**: Raycast-based visibility checks

**Dependencies:**
- Physics System (for spatial queries and steering)
- Asset System (for loading behavior trees and navmeshes)

---

## Core Concepts

### 1. Behavior Trees

Behavior trees organize AI decision-making hierarchically. Each entity can have:
- **One behavior tree** (loaded from asset)
- **Blackboard data** (shared variables for the tree)
- **Execution state** (managed automatically during update)

**Node Types (via BehaviorTree.CPP):**
- **Action nodes**: Perform tasks (move, attack, play animation)
- **Condition nodes**: Check state (health low?, player visible?)
- **Composite nodes**: Control flow (sequence, selector, parallel)
- **Decorator nodes**: Modify behavior (repeat, invert, timeout)

### 2. Navigation

Navigation uses **Recast/Detour** for robust pathfinding:
- **NavMesh**: Walkable surface representation
- **Pathfinding**: A* on navmesh polygons
- **Dynamic queries**: Point containment, closest point, path calculation

**Coordinate System:**
- Bestow uses 2D (x, y) coordinates
- Detour uses 3D (x, y=0, z) internally
- The AI system handles conversion automatically

### 3. Steering Behaviors

Simple movement behaviors integrated with physics:
- **Patrol**: Back-and-forth movement along a path
- **Navigation target**: Move toward a specific position
- **Max speed/acceleration**: Configurable movement limits

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
// In your game loop
aiSystem->update(deltaTime);
```

---

### Behavior Trees

#### Attach/Detach

```cpp
// Attach a behavior tree asset to an entity
void attachBehaviorTree(Entity entity, AssetHandle treeAsset);

// Remove behavior tree from entity
void detachBehaviorTree(Entity entity);

// Check if entity has a behavior tree
bool hasBehaviorTree(Entity entity) const;
```

**Example:**
```cpp
Entity enemy = entities->createEntity();

// Load and attach behavior tree
AssetHandle enemyAI = assets->registerAsset(AssetType::BehaviorTree,
                                             ":assets:/ai/enemy_patrol.xml");
assets->loadAsset(enemyAI);
aiSystem->attachBehaviorTree(enemy, enemyAI);
```

#### Blackboard Data

The blackboard stores per-entity data accessible to behavior tree nodes.

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

// Retrieve values
int health = std::any_cast<int>(
    aiSystem->getBehaviorTreeBlackboard(enemy, "health"));

bool isAlert = std::any_cast<bool>(
    aiSystem->getBehaviorTreeBlackboard(enemy, "alert"));

Vec2 target = std::any_cast<Vec2>(
    aiSystem->getBehaviorTreeBlackboard(enemy, "patrol_point"));
```

**Important Notes:**
- Blackboard persists even if behavior tree is detached
- Empty `std::any` returned for non-existent keys
- Values are independent between entities
- Can overwrite with different type (type-safe via `std::any_cast`)

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
```

**NavMesh Format:**
- Binary format generated by Recast
- Contains polygon mesh and connectivity data
- Loaded through AssetSystem as `NavMeshData`

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
}
```

**Fallback Behavior:**
- If no navmesh loaded: returns `std::nullopt`
- If navmesh loaded but no data: returns straight line path
- If pathfinding fails: returns `std::nullopt`

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
// Make enemy chase player
Vec2 playerPos = physics->getPosition(player);
aiSystem->setNavigationTarget(enemy, playerPos);

// Later: stop chasing
aiSystem->clearNavigationTarget(enemy);
```

**Note:** Navigation target is stored but not automatically applied. Combine with path following in your behavior tree or update logic.

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

**Defaults:**
- Max speed: 100.0 units/second
- Max acceleration: 500.0 units/second²

#### Patrol Behavior

Built-in horizontal patrol behavior (automatically updates physics velocity).

```cpp
struct PatrolBehavior {
    float startX = 0.0f;      // Center X position of patrol
    float range = 100.0f;     // Distance to patrol in each direction
    float speed = 50.0f;      // Movement speed
    bool movingRight = true;  // Current direction
};

void setPatrolBehavior(Entity entity, const PatrolBehavior& patrol);
void clearPatrolBehavior(Entity entity);
std::optional<PatrolBehavior> getPatrolBehavior(Entity entity) const;
```

**Example:**
```cpp
// Create patrolling enemy
Entity enemy = entities->createEntity();

// Create physics body (required for patrol)
physics->createBody(enemy, PhysicsBodyDef{
    .type = BodyType::Dynamic,
    .transform = {.x = 300.0f, .y = 100.0f}
});

// Setup patrol: 200-400 on X axis at 75 units/sec
aiSystem->setPatrolBehavior(enemy, PatrolBehavior{
    .startX = 300.0f,
    .range = 100.0f,   // Patrols from 200 to 400
    .speed = 75.0f,
    .movingRight = true
});

// AI system will automatically:
// - Set velocity each frame based on direction
// - Flip direction at range boundaries
// - Preserve Y velocity (for gravity, jumping)
```

**Behavior Details:**
- Automatically flips direction at `startX ± range`
- Updates X velocity, preserves Y velocity
- Requires physics body (gracefully skips entities without)
- Can check current direction via `getPatrolBehavior()`

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
- Empty vector if no entities found

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
- Searches within 2000 unit radius (hardcoded)
- Returns `std::nullopt` if no entities in range
- Collision mask currently not implemented (uses all layers)

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

## Creating Custom Behavior Trees

**Note:** BehaviorTree.CPP integration is planned but not yet implemented. This section describes the future API.

### Behavior Tree XML Format

```xml
<BehaviorTree>
    <Sequence>
        <Condition name="PlayerVisible"/>
        <Selector>
            <Sequence>
                <Condition name="InRange"/>
                <Action name="Attack"/>
            </Sequence>
            <Action name="MoveToPlayer"/>
        </Selector>
    </Sequence>
    <Action name="Patrol"/>
</BehaviorTree>
```

### Custom Action Nodes

```cpp
// Example: Custom attack action
class AttackAction : public BT::SyncActionNode {
public:
    AttackAction(const std::string& name, const BT::NodeConfiguration& config)
        : BT::SyncActionNode(name, config) {}

    static BT::PortsList providedPorts() {
        return { BT::InputPort<Entity>("target") };
    }

    BT::NodeStatus tick() override {
        Entity target;
        if (!getInput("target", target)) {
            return BT::NodeStatus::FAILURE;
        }

        // Perform attack
        damageSystem->dealDamage(target, 10);

        return BT::NodeStatus::SUCCESS;
    }
};
```

### Custom Condition Nodes

```cpp
// Example: Check if player is visible
class PlayerVisibleCondition : public BT::ConditionNode {
public:
    PlayerVisibleCondition(const std::string& name,
                           const BT::NodeConfiguration& config,
                           IAISystem* aiSystem,
                           IPhysicsSystem* physicsSystem,
                           Entity selfEntity)
        : BT::ConditionNode(name, config)
        , aiSystem_(aiSystem)
        , physicsSystem_(physicsSystem)
        , selfEntity_(selfEntity) {}

    static BT::PortsList providedPorts() {
        return { BT::OutputPort<bool>("visible") };
    }

    BT::NodeStatus tick() override {
        // Get player from blackboard
        Entity player = std::any_cast<Entity>(
            aiSystem_->getBehaviorTreeBlackboard(selfEntity_, "player"));

        Vec2 selfPos = physicsSystem_->getPosition(selfEntity_);
        Vec2 playerPos = physicsSystem_->getPosition(player);

        bool visible = aiSystem_->hasLineOfSight(selfPos, playerPos);

        setOutput("visible", visible);

        return visible ? BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
    }

private:
    IAISystem* aiSystem_;
    IPhysicsSystem* physicsSystem_;
    Entity selfEntity_;
};
```

### Registering Custom Nodes

```cpp
// In your game initialization
BT::BehaviorTreeFactory factory;

// Register custom actions
factory.registerNodeType<AttackAction>("Attack");
factory.registerNodeType<MoveAction>("MoveToPlayer");

// Register custom conditions
factory.registerBuilder<PlayerVisibleCondition>(
    "PlayerVisible",
    [&](const std::string& name, const BT::NodeConfiguration& config) {
        return std::make_unique<PlayerVisibleCondition>(
            name, config, aiSystem, physicsSystem, currentEntity);
    }
);

// Load and create tree
auto tree = factory.createTreeFromFile("enemy_ai.xml");
```

---

## Best Practices

### Behavior Tree Design Patterns

#### 1. Layered Priority with Selector

```xml
<Selector name="EnemyBehavior">
    <!-- Highest priority: React to danger -->
    <Sequence name="Flee">
        <Condition name="HealthLow"/>
        <Action name="RunAway"/>
    </Sequence>

    <!-- Medium priority: Engage player -->
    <Sequence name="Combat">
        <Condition name="PlayerVisible"/>
        <Selector>
            <Sequence name="Melee">
                <Condition name="InMeleeRange"/>
                <Action name="Attack"/>
            </Sequence>
            <Action name="Approach"/>
        </Selector>
    </Sequence>

    <!-- Lowest priority: Default behavior -->
    <Action name="Patrol"/>
</Selector>
```

**Why it works:**
- Selector tries each child until one succeeds
- Higher priority behaviors appear first
- Falls through to patrol when nothing else applies

#### 2. Sequence for Multi-Step Actions

```xml
<Sequence name="CollectItem">
    <Condition name="ItemNearby"/>
    <Action name="PathToItem"/>
    <Action name="PickUpItem"/>
    <Action name="ReturnToBase"/>
</Sequence>
```

**Why it works:**
- Sequence requires all steps to succeed
- Fails early if any step fails
- Ensures complete action flow

#### 3. Decorator for Repetition

```xml
<Repeat num_cycles="3">
    <Sequence name="PatrolCycle">
        <Action name="MoveToWaypoint"/>
        <Action name="Wait" duration="2.0"/>
    </Sequence>
</Repeat>
```

**Why it works:**
- Decorators wrap and modify child behavior
- Useful for loops, timers, inverters

### Performance Optimization

#### 1. Cache Spatial Queries

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

#### 2. LOD Behavior Trees

```cpp
// Distance-based AI detail levels
float distToPlayer = distance(enemyPos, playerPos);

if (distToPlayer < 300.0f) {
    // Full AI: complex behavior tree
    aiSystem->attachBehaviorTree(enemy, detailedAI);
} else if (distToPlayer < 1000.0f) {
    // Simple AI: patrol only
    aiSystem->attachBehaviorTree(enemy, simpleAI);
} else {
    // No AI: sleep/despawn
    aiSystem->detachBehaviorTree(enemy);
}
```

#### 3. Limit Pathfinding Frequency

```cpp
// Don't recalculate path every frame
struct ChaseState {
    std::vector<Vec2> currentPath;
    int waypointIndex = 0;
    float recalcTimer = 0.0f;
};

void updateChase(ChaseState& state, DeltaTime dt) {
    state.recalcTimer += dt;

    // Recalculate path every 1 second
    if (state.recalcTimer >= 1.0f) {
        auto path = aiSystem->findPath({
            .start = enemyPos,
            .end = playerPos,
            .agentRadius = 0.5f
        });
        if (path) {
            state.currentPath = path->waypoints;
            state.waypointIndex = 0;
        }
        state.recalcTimer = 0.0f;
    }

    // Follow current path
    if (state.waypointIndex < state.currentPath.size()) {
        Vec2 target = state.currentPath[state.waypointIndex];
        // Move toward target...
    }
}
```

### Debugging AI Behavior

#### 1. Blackboard Visualization

```cpp
// Log blackboard state for debugging
void debugBlackboard(Entity entity) {
    auto health = aiSystem->getBehaviorTreeBlackboard(entity, "health");
    auto state = aiSystem->getBehaviorTreeBlackboard(entity, "state");
    auto target = aiSystem->getBehaviorTreeBlackboard(entity, "target");

    if (health.has_value()) {
        fmt::print("Health: {}\n", std::any_cast<int>(health));
    }
    if (state.has_value()) {
        fmt::print("State: {}\n", std::any_cast<std::string>(state));
    }
    if (target.has_value()) {
        Vec2 t = std::any_cast<Vec2>(target);
        fmt::print("Target: ({}, {})\n", t.x, t.y);
    }
}
```

#### 2. Visual Debug Overlays

```cpp
// Draw AI debug info (in debug builds only)
#ifdef BESTOW_DEBUG
void drawAIDebug(Entity entity) {
    // Draw line of sight rays
    Vec2 enemyPos = physics->getPosition(entity);
    Vec2 playerPos = physics->getPosition(player);

    Color rayColor = aiSystem->hasLineOfSight(enemyPos, playerPos)
        ? Color::Green
        : Color::Red;
    graphics->drawLine(enemyPos, playerPos, rayColor);

    // Draw navigation path
    if (auto target = aiSystem->getNavigationTarget(entity)) {
        auto path = aiSystem->findPath({
            .start = enemyPos,
            .end = *target
        });
        if (path) {
            for (size_t i = 0; i < path->waypoints.size() - 1; ++i) {
                graphics->drawLine(path->waypoints[i],
                                   path->waypoints[i+1],
                                   Color::Yellow);
            }
        }
    }

    // Draw patrol range
    if (auto patrol = aiSystem->getPatrolBehavior(entity)) {
        Vec2 left{patrol->startX - patrol->range, enemyPos.y};
        Vec2 right{patrol->startX + patrol->range, enemyPos.y};
        graphics->drawLine(left, right, Color::Cyan);
    }
}
#endif
```

#### 3. State Logging

```cpp
// Log AI state transitions
class LoggingAI {
    std::string currentState_;

    void transitionTo(const std::string& newState) {
        if (newState != currentState_) {
            fmt::print("[AI] Entity {} transitioning: {} -> {}\n",
                       static_cast<uint32_t>(entity_),
                       currentState_,
                       newState);
            currentState_ = newState;

            aiSystem->setBehaviorTreeBlackboard(entity_, "state", newState);
        }
    }
};
```

### Combining with Physics for Steering

#### Basic Seek Behavior

```cpp
void seekTarget(Entity entity, Vec2 target, float dt) {
    Vec2 currentPos = physics->getPosition(entity);
    Vec2 currentVel = physics->getVelocity(entity);

    // Desired velocity
    Vec2 toTarget = {target.x - currentPos.x, target.y - currentPos.y};
    float distance = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y);

    if (distance < 0.001f) return; // At target

    // Get AI parameters
    float maxSpeed = 100.0f; // Or from aiSystem->getMaxSpeed()
    float maxAccel = 500.0f; // Or from aiSystem->getMaxAcceleration()

    // Normalize and scale to max speed
    Vec2 desiredVel = {
        (toTarget.x / distance) * maxSpeed,
        (toTarget.y / distance) * maxSpeed
    };

    // Calculate steering force
    Vec2 steering = {
        desiredVel.x - currentVel.x,
        desiredVel.y - currentVel.y
    };

    // Limit to max acceleration
    float steerMag = std::sqrt(steering.x * steering.x + steering.y * steering.y);
    if (steerMag > maxAccel * dt) {
        float scale = (maxAccel * dt) / steerMag;
        steering.x *= scale;
        steering.y *= scale;
    }

    // Apply to velocity
    Vec2 newVel = {
        currentVel.x + steering.x,
        currentVel.y + steering.y
    };
    physics->setVelocity(entity, newVel);
}
```

#### Arrive Behavior (Slow Down Near Target)

```cpp
void arriveAtTarget(Entity entity, Vec2 target, float slowRadius, float dt) {
    Vec2 currentPos = physics->getPosition(entity);
    Vec2 currentVel = physics->getVelocity(entity);

    Vec2 toTarget = {target.x - currentPos.x, target.y - currentPos.y};
    float distance = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y);

    if (distance < 0.001f) {
        physics->setVelocity(entity, {0.0f, 0.0f});
        return;
    }

    float maxSpeed = 100.0f;
    float targetSpeed = maxSpeed;

    // Slow down within slow radius
    if (distance < slowRadius) {
        targetSpeed = maxSpeed * (distance / slowRadius);
    }

    Vec2 desiredVel = {
        (toTarget.x / distance) * targetSpeed,
        (toTarget.y / distance) * targetSpeed
    };

    // Apply steering...
    // (Same as seek behavior)
}
```

#### Path Following

```cpp
void followPath(Entity entity, const NavigationPath& path, float dt) {
    static size_t currentWaypoint = 0;
    static const float WAYPOINT_RADIUS = 10.0f; // Distance to consider reached

    if (currentWaypoint >= path.waypoints.size()) {
        return; // Path complete
    }

    Vec2 target = path.waypoints[currentWaypoint];
    Vec2 currentPos = physics->getPosition(entity);

    // Check if reached current waypoint
    Vec2 toWaypoint = {target.x - currentPos.x, target.y - currentPos.y};
    float dist = std::sqrt(toWaypoint.x * toWaypoint.x + toWaypoint.y * toWaypoint.y);

    if (dist < WAYPOINT_RADIUS) {
        currentWaypoint++;
        if (currentWaypoint >= path.waypoints.size()) {
            // Reached end of path
            physics->setVelocity(entity, {0.0f, 0.0f});
            return;
        }
        target = path.waypoints[currentWaypoint];
    }

    // Use arrive for last waypoint, seek for others
    if (currentWaypoint == path.waypoints.size() - 1) {
        arriveAtTarget(entity, target, 50.0f, dt);
    } else {
        seekTarget(entity, target, dt);
    }
}
```

---

## Complete Examples

### Example 1: Simple Patrol Behavior

```cpp
// Create patrolling enemy that doesn't use behavior trees
void createPatrolEnemy(Vec2 position, float patrolRange) {
    Entity enemy = entities->createEntity();

    // Physics body
    physics->createBody(enemy, PhysicsBodyDef{
        .type = BodyType::Dynamic,
        .transform = {.x = position.x, .y = position.y},
        .fixedRotation = true
    });

    // Collision shape
    physics->attachBox(enemy, 16.0f, 16.0f);

    // Patrol behavior
    aiSystem->setPatrolBehavior(enemy, PatrolBehavior{
        .startX = position.x,
        .range = patrolRange,
        .speed = 50.0f,
        .movingRight = true
    });

    // AI system will automatically update velocity each frame
}

// In game update loop:
void update(DeltaTime dt) {
    aiSystem->update(dt); // Handles patrol velocity updates
    physics->update(dt);   // Applies velocity to position
}
```

### Example 2: Chase and Attack Behavior

```cpp
class ChaseAttackAI {
public:
    ChaseAttackAI(Entity self, Entity target,
                  IAISystem* ai, IPhysicsSystem* physics)
        : self_(self), target_(target), ai_(ai), physics_(physics) {}

    void update(DeltaTime dt) {
        Vec2 selfPos = physics_->getPosition(self_);
        Vec2 targetPos = physics_->getPosition(target_);

        // Calculate distance to target
        Vec2 toTarget = {targetPos.x - selfPos.x, targetPos.y - selfPos.y};
        float distance = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y);

        // State machine
        switch (state_) {
            case State::Patrol:
                updatePatrol(dt);

                // Transition to chase if player visible
                if (ai_->hasLineOfSight(selfPos, targetPos)) {
                    state_ = State::Chase;
                    recalcPathTimer_ = 0.0f; // Immediate path recalc
                }
                break;

            case State::Chase:
                updateChase(dt, selfPos, targetPos, distance);

                // Transition to attack if in range
                if (distance < ATTACK_RANGE) {
                    state_ = State::Attack;
                    attackCooldown_ = 0.0f;
                }
                // Transition back to patrol if lost sight
                else if (!ai_->hasLineOfSight(selfPos, targetPos)) {
                    lostSightTimer_ += dt;
                    if (lostSightTimer_ > 3.0f) {
                        state_ = State::Patrol;
                        ai_->setPatrolBehavior(self_, PatrolBehavior{
                            .startX = selfPos.x,
                            .range = 100.0f,
                            .speed = 50.0f
                        });
                    }
                } else {
                    lostSightTimer_ = 0.0f;
                }
                break;

            case State::Attack:
                updateAttack(dt, distance);

                // Transition back to chase if out of range
                if (distance > ATTACK_RANGE * 1.2f) {
                    state_ = State::Chase;
                }
                break;
        }
    }

private:
    enum class State { Patrol, Chase, Attack };

    static constexpr float ATTACK_RANGE = 30.0f;
    static constexpr float ATTACK_COOLDOWN = 1.0f;
    static constexpr float PATH_RECALC_INTERVAL = 0.5f;

    Entity self_;
    Entity target_;
    IAISystem* ai_;
    IPhysicsSystem* physics_;

    State state_ = State::Patrol;
    float attackCooldown_ = 0.0f;
    float lostSightTimer_ = 0.0f;
    float recalcPathTimer_ = 0.0f;
    std::vector<Vec2> currentPath_;
    size_t waypointIndex_ = 0;

    void updatePatrol(DeltaTime dt) {
        // Patrol behavior handled automatically by AI system
        // Just maintain patrol state in blackboard
        ai_->setBehaviorTreeBlackboard(self_, "state", std::string("patrol"));
    }

    void updateChase(DeltaTime dt, Vec2 selfPos, Vec2 targetPos, float distance) {
        ai_->setBehaviorTreeBlackboard(self_, "state", std::string("chase"));

        // Recalculate path periodically
        recalcPathTimer_ += dt;
        if (recalcPathTimer_ >= PATH_RECALC_INTERVAL) {
            if (ai_->hasNavMesh()) {
                auto path = ai_->findPath({
                    .start = selfPos,
                    .end = targetPos,
                    .agentRadius = 0.5f
                });
                if (path) {
                    currentPath_ = path->waypoints;
                    waypointIndex_ = 0;
                }
            }
            recalcPathTimer_ = 0.0f;
        }

        // Follow path or seek directly
        if (!currentPath_.empty() && waypointIndex_ < currentPath_.size()) {
            Vec2 waypoint = currentPath_[waypointIndex_];
            seekTarget(self_, waypoint, dt);

            // Check if reached waypoint
            Vec2 toWaypoint = {waypoint.x - selfPos.x, waypoint.y - selfPos.y};
            float wpDist = std::sqrt(toWaypoint.x * toWaypoint.x +
                                    toWaypoint.y * toWaypoint.y);
            if (wpDist < 10.0f) {
                waypointIndex_++;
            }
        } else {
            // No path - seek directly
            seekTarget(self_, targetPos, dt);
        }
    }

    void updateAttack(DeltaTime dt, float distance) {
        ai_->setBehaviorTreeBlackboard(self_, "state", std::string("attack"));

        // Stop moving
        physics_->setVelocity(self_, {0.0f, 0.0f});

        // Attack on cooldown
        attackCooldown_ += dt;
        if (attackCooldown_ >= ATTACK_COOLDOWN) {
            performAttack();
            attackCooldown_ = 0.0f;
        }
    }

    void seekTarget(Entity entity, Vec2 target, float dt) {
        Vec2 currentPos = physics_->getPosition(entity);
        Vec2 currentVel = physics_->getVelocity(entity);

        Vec2 toTarget = {target.x - currentPos.x, target.y - currentPos.y};
        float distance = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y);

        if (distance < 0.001f) return;

        float maxSpeed = 100.0f;
        Vec2 desiredVel = {
            (toTarget.x / distance) * maxSpeed,
            (toTarget.y / distance) * maxSpeed
        };

        // Instant velocity change for simplicity
        // (In production, use steering forces with acceleration)
        physics_->setVelocity(entity, desiredVel);
    }

    void performAttack() {
        // Deal damage, play animation, etc.
        ai_->setBehaviorTreeBlackboard(self_, "last_attack_time",
                                        std::chrono::steady_clock::now());
    }
};
```

### Example 3: Pathfinding to Target

```cpp
// Smart enemy that navigates around obstacles
class PathfindingEnemy {
public:
    PathfindingEnemy(Entity self, IAISystem* ai, IPhysicsSystem* physics)
        : self_(self), ai_(ai), physics_(physics) {}

    // Set a destination and begin pathfinding
    void setDestination(Vec2 destination) {
        destination_ = destination;
        recalculatePath();
    }

    void update(DeltaTime dt) {
        if (!destination_.has_value()) return;

        // Recalculate path periodically
        recalcTimer_ += dt;
        if (recalcTimer_ >= RECALC_INTERVAL) {
            recalculatePath();
            recalcTimer_ = 0.0f;
        }

        // Follow current path
        if (!path_.empty() && waypointIndex_ < path_.size()) {
            Vec2 currentWaypoint = path_[waypointIndex_];
            Vec2 selfPos = physics_->getPosition(self_);

            // Calculate distance to waypoint
            Vec2 toWaypoint = {
                currentWaypoint.x - selfPos.x,
                currentWaypoint.y - selfPos.y
            };
            float distance = std::sqrt(toWaypoint.x * toWaypoint.x +
                                      toWaypoint.y * toWaypoint.y);

            // Check if reached waypoint
            if (distance < WAYPOINT_RADIUS) {
                waypointIndex_++;

                // Check if reached destination
                if (waypointIndex_ >= path_.size()) {
                    onReachedDestination();
                    return;
                }

                currentWaypoint = path_[waypointIndex_];
            }

            // Move toward current waypoint
            bool isLastWaypoint = (waypointIndex_ == path_.size() - 1);
            if (isLastWaypoint) {
                arriveAt(currentWaypoint, dt);
            } else {
                seekTo(currentWaypoint, dt);
            }
        }
    }

private:
    static constexpr float RECALC_INTERVAL = 1.0f;  // Recalc path every second
    static constexpr float WAYPOINT_RADIUS = 15.0f; // Distance to consider reached
    static constexpr float SLOW_RADIUS = 50.0f;     // Start slowing down
    static constexpr float MAX_SPEED = 120.0f;
    static constexpr float MAX_ACCEL = 600.0f;

    Entity self_;
    IAISystem* ai_;
    IPhysicsSystem* physics_;

    std::optional<Vec2> destination_;
    std::vector<Vec2> path_;
    size_t waypointIndex_ = 0;
    float recalcTimer_ = 0.0f;

    void recalculatePath() {
        if (!destination_.has_value() || !ai_->hasNavMesh()) {
            return;
        }

        Vec2 selfPos = physics_->getPosition(self_);

        auto result = ai_->findPath({
            .start = selfPos,
            .end = *destination_,
            .agentRadius = 0.5f
        });

        if (result) {
            path_ = result->waypoints;
            waypointIndex_ = 0;

            // Update blackboard
            ai_->setBehaviorTreeBlackboard(self_, "path_length",
                                            result->totalLength);
            ai_->setBehaviorTreeBlackboard(self_, "path_complete",
                                            result->isComplete);
        } else {
            // No path found - clear path and stop
            path_.clear();
            waypointIndex_ = 0;
            physics_->setVelocity(self_, {0.0f, 0.0f});
        }
    }

    void seekTo(Vec2 target, DeltaTime dt) {
        Vec2 selfPos = physics_->getPosition(self_);
        Vec2 currentVel = physics_->getVelocity(self_);

        Vec2 toTarget = {target.x - selfPos.x, target.y - selfPos.y};
        float distance = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y);

        if (distance < 0.001f) return;

        // Desired velocity at max speed
        Vec2 desiredVel = {
            (toTarget.x / distance) * MAX_SPEED,
            (toTarget.y / distance) * MAX_SPEED
        };

        // Calculate steering force
        Vec2 steering = {
            desiredVel.x - currentVel.x,
            desiredVel.y - currentVel.y
        };

        // Apply acceleration limit
        float steerMag = std::sqrt(steering.x * steering.x +
                                   steering.y * steering.y);
        float maxSteer = MAX_ACCEL * dt;
        if (steerMag > maxSteer) {
            float scale = maxSteer / steerMag;
            steering.x *= scale;
            steering.y *= scale;
        }

        // Apply steering
        Vec2 newVel = {
            currentVel.x + steering.x,
            currentVel.y + steering.y
        };

        physics_->setVelocity(self_, newVel);
    }

    void arriveAt(Vec2 target, DeltaTime dt) {
        Vec2 selfPos = physics_->getPosition(self_);
        Vec2 currentVel = physics_->getVelocity(self_);

        Vec2 toTarget = {target.x - selfPos.x, target.y - selfPos.y};
        float distance = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y);

        if (distance < 0.001f) {
            physics_->setVelocity(self_, {0.0f, 0.0f});
            return;
        }

        // Calculate target speed (slow down near target)
        float targetSpeed = MAX_SPEED;
        if (distance < SLOW_RADIUS) {
            targetSpeed = MAX_SPEED * (distance / SLOW_RADIUS);
        }

        Vec2 desiredVel = {
            (toTarget.x / distance) * targetSpeed,
            (toTarget.y / distance) * targetSpeed
        };

        // Apply steering (same as seek)
        Vec2 steering = {
            desiredVel.x - currentVel.x,
            desiredVel.y - currentVel.y
        };

        float steerMag = std::sqrt(steering.x * steering.x +
                                   steering.y * steering.y);
        float maxSteer = MAX_ACCEL * dt;
        if (steerMag > maxSteer) {
            float scale = maxSteer / steerMag;
            steering.x *= scale;
            steering.y *= scale;
        }

        Vec2 newVel = {
            currentVel.x + steering.x,
            currentVel.y + steering.y
        };

        physics_->setVelocity(self_, newVel);
    }

    void onReachedDestination() {
        destination_.reset();
        path_.clear();
        waypointIndex_ = 0;
        physics_->setVelocity(self_, {0.0f, 0.0f});

        ai_->setBehaviorTreeBlackboard(self_, "reached_destination", true);
    }
};
```

---

## Integration Checklist

When integrating the AI system into your game:

- [ ] **Initialize dependencies first**: Physics and Asset systems must be initialized before AI system
- [ ] **Load navmesh early**: Load level navmesh during level load, before creating AI entities
- [ ] **Create physics bodies**: Entities need physics bodies for spatial queries and steering
- [ ] **Call update()**: Include `aiSystem->update(dt)` in your game loop
- [ ] **Set collision layers**: Configure physics collision masks for LOS and spatial queries
- [ ] **Cache expensive queries**: Don't pathfind or query every frame
- [ ] **Handle optional results**: Check `std::optional` returns before use
- [ ] **Use blackboard for state**: Store AI state in blackboard for debugging
- [ ] **Implement LOD**: Reduce AI complexity for distant entities

---

## Common Pitfalls

### 1. Missing Physics Body

```cpp
// WRONG: No physics body
Entity enemy = entities->createEntity();
aiSystem->setPatrolBehavior(enemy, patrol); // Will silently fail

// CORRECT: Create physics body first
Entity enemy = entities->createEntity();
physics->createBody(enemy, bodyDef);
aiSystem->setPatrolBehavior(enemy, patrol); // Now works
```

### 2. Not Checking Optional Returns

```cpp
// WRONG: Assuming path exists
auto path = aiSystem->findPath(query);
for (const Vec2& wp : path->waypoints) { ... } // CRASH if nullopt

// CORRECT: Check before use
if (auto path = aiSystem->findPath(query)) {
    for (const Vec2& wp : path->waypoints) { ... }
}
```

### 3. Pathfinding Every Frame

```cpp
// WRONG: Too expensive
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

### 4. Wrong Type Cast from Blackboard

```cpp
// WRONG: Wrong type
aiSystem->setBehaviorTreeBlackboard(enemy, "health", 100.0f); // float
int health = std::any_cast<int>(
    aiSystem->getBehaviorTreeBlackboard(enemy, "health")); // Throws!

// CORRECT: Matching types
aiSystem->setBehaviorTreeBlackboard(enemy, "health", 100.0f);
float health = std::any_cast<float>(
    aiSystem->getBehaviorTreeBlackboard(enemy, "health"));
```

### 5. Not Updating AI System

```cpp
// WRONG: Patrol doesn't work
void gameUpdate(DeltaTime dt) {
    physics->update(dt); // Only physics
}

// CORRECT: Update AI before physics
void gameUpdate(DeltaTime dt) {
    aiSystem->update(dt); // Updates velocities
    physics->update(dt);   // Applies velocities
}
```

---

## Further Reading

- **BehaviorTree.CPP Documentation**: https://www.behaviortree.dev/
- **Recast/Detour Manual**: https://recastnav.com/
- **Steering Behaviors**: "Steering Behaviors For Autonomous Characters" by Craig Reynolds
- **Game AI Pro**: Book series with advanced AI techniques

---

## Version History

- **v1.0** (Current): Initial AI System with patrol, navigation, spatial queries
- **Planned**: BehaviorTree.CPP integration, more steering behaviors, influence maps
