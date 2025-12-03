// AIDemo.cppm
// Comprehensive demonstration of the JFrame AI System API

module;

#include <entt/entt.hpp>
#include <any>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <print>
#include <string>
#include <utility>
#include <vector>

export module ai.demo;
import jframe.types;
import jframe.ai;
import jframe.entity;
import jframe.physics;
import jframe.assets;

export namespace demo {

//==========================================================================
// Sample Components for AI Demo
//==========================================================================

struct Position {
    float x = 0.0f;
    float y = 0.0f;

    Position() = default;
    Position(float x, float y) : x(x), y(y) {}
};

struct Name {
    std::string value;

    Name() = default;
    Name(std::string name) : value(std::move(name)) {}
};

// Tag components for entity types
struct AIAgentTag {
    bool dummy = true;
};

struct EnemyTag {
    bool dummy = true;
};

struct NPCTag {
    bool dummy = true;
};

struct PatrollerTag {
    bool dummy = true;
};

//==========================================================================
// AIDemo Class
//==========================================================================

class AIDemo {
public:
    AIDemo(jframe::IAISystem& aiSystem,
           jframe::IEntitySystem& entitySystem,
           jframe::IPhysicsSystem& physicsSystem,
           jframe::IAssetSystem& assetSystem)
        : ai_(aiSystem)
        , entities_(entitySystem)
        , physics_(physicsSystem)
        , assets_(assetSystem) {}

    void run() {
        printHeader("JFRAME AI SYSTEM COMPREHENSIVE DEMO");

        demoLifecycle();
        demoBehaviorTrees();
        demoBlackboard();
        demoNavigation();
        demoPathfinding();
        demoSteering();
        demoMovementParameters();
        demoPatrolBehavior();
        demoSpatialQueries();
        demoComplexAIScenario();

        printHeader("DEMO COMPLETE");
    }

private:
    jframe::IAISystem& ai_;
    jframe::IEntitySystem& entities_;
    jframe::IPhysicsSystem& physics_;
    jframe::IAssetSystem& assets_;

    // Test entities created during demo
    std::vector<jframe::Entity> testEntities_;

    //==========================================================================
    // Demo Sections
    //==========================================================================

    void demoLifecycle() {
        printSection("AI System Lifecycle");

        std::println("Testing update(DeltaTime dt)...");

        // Create some AI entities for update to process
        auto agent1 = createAIAgent("UpdateTestAgent1", 100.0f, 100.0f);
        auto agent2 = createAIAgent("UpdateTestAgent2", 200.0f, 100.0f);

        // Set navigation targets
        ai_.setNavigationTarget(agent1, jframe::Vec2{150.0f, 150.0f});
        ai_.setNavigationTarget(agent2, jframe::Vec2{250.0f, 150.0f});

        std::println("  Created 2 AI agents with navigation targets");

        // Run several update cycles
        float deltaTime = 0.016f; // 60 FPS
        std::println("\nRunning 5 update cycles at 60 FPS...");
        for (int i = 0; i < 5; ++i) {
            ai_.update(deltaTime);
            std::println("  Update cycle {} completed (dt = {:.3f}s)", i + 1, deltaTime);
        }

        std::println("\nAI system update cycles completed successfully");
    }

    void demoBehaviorTrees() {
        printSection("Behavior Trees");

        // Register mock behavior tree assets
        auto btHandle1 = assets_.registerAsset(jframe::AssetType::BehaviorTree,
                                                "behaviors/patrol.bt");
        auto btHandle2 = assets_.registerAsset(jframe::AssetType::BehaviorTree,
                                                "behaviors/chase.bt");

        std::println("Registered behavior tree assets:");
        std::println("  Patrol BT: UUID {}", btHandle1.uuid);
        std::println("  Chase BT: UUID {}", btHandle2.uuid);

        auto agent = createAIAgent("BehaviorTreeAgent", 300.0f, 100.0f);

        // attachBehaviorTree
        std::println("\nTesting attachBehaviorTree()...");
        std::println("  Attaching patrol behavior tree to agent...");
        ai_.attachBehaviorTree(agent, btHandle1);

        // hasBehaviorTree
        std::println("\nTesting hasBehaviorTree()...");
        bool hasTree = ai_.hasBehaviorTree(agent);
        std::println("  Agent has behavior tree: {}", hasTree);

        // Switch to different behavior tree
        std::println("\nSwitching to chase behavior tree...");
        ai_.attachBehaviorTree(agent, btHandle2);
        std::println("  Agent now has chase behavior tree attached");

        // detachBehaviorTree
        std::println("\nTesting detachBehaviorTree()...");
        ai_.detachBehaviorTree(agent);
        hasTree = ai_.hasBehaviorTree(agent);
        std::println("  Behavior tree detached. Has tree: {}", hasTree);
    }

    void demoBlackboard() {
        printSection("Blackboard (AI State Storage)");

        auto agent = createAIAgent("BlackboardAgent", 400.0f, 100.0f);

        std::println("Testing setBehaviorTreeBlackboard()...");

        // Store various data types in blackboard
        ai_.setBehaviorTreeBlackboard(agent, "alert_level", std::any(3));
        ai_.setBehaviorTreeBlackboard(agent, "target_seen", std::any(true));
        ai_.setBehaviorTreeBlackboard(agent, "patrol_index", std::any(5));
        ai_.setBehaviorTreeBlackboard(agent, "chase_speed", std::any(150.0f));
        ai_.setBehaviorTreeBlackboard(agent, "state", std::any(std::string("PATROL")));

        std::println("  Stored values in blackboard:");
        std::println("    alert_level: 3");
        std::println("    target_seen: true");
        std::println("    patrol_index: 5");
        std::println("    chase_speed: 150.0");
        std::println("    state: PATROL");

        std::println("\nTesting getBehaviorTreeBlackboard()...");

        // Retrieve and verify values
        auto alertLevel = std::any_cast<int>(
            ai_.getBehaviorTreeBlackboard(agent, "alert_level"));
        auto targetSeen = std::any_cast<bool>(
            ai_.getBehaviorTreeBlackboard(agent, "target_seen"));
        auto patrolIndex = std::any_cast<int>(
            ai_.getBehaviorTreeBlackboard(agent, "patrol_index"));
        auto chaseSpeed = std::any_cast<float>(
            ai_.getBehaviorTreeBlackboard(agent, "chase_speed"));
        auto state = std::any_cast<std::string>(
            ai_.getBehaviorTreeBlackboard(agent, "state"));

        std::println("  Retrieved values from blackboard:");
        std::println("    alert_level: {}", alertLevel);
        std::println("    target_seen: {}", targetSeen);
        std::println("    patrol_index: {}", patrolIndex);
        std::println("    chase_speed: {}", chaseSpeed);
        std::println("    state: {}", state);

        // Update values
        std::println("\nUpdating blackboard values...");
        ai_.setBehaviorTreeBlackboard(agent, "alert_level", std::any(5));
        ai_.setBehaviorTreeBlackboard(agent, "state", std::any(std::string("CHASE")));

        alertLevel = std::any_cast<int>(
            ai_.getBehaviorTreeBlackboard(agent, "alert_level"));
        state = std::any_cast<std::string>(
            ai_.getBehaviorTreeBlackboard(agent, "state"));

        std::println("  Updated values:");
        std::println("    alert_level: {}", alertLevel);
        std::println("    state: {}", state);
    }

    void demoNavigation() {
        printSection("Navigation Mesh");

        std::println("Testing navigation mesh lifecycle...\n");

        // hasNavMesh (before loading)
        std::println("Testing hasNavMesh() before loading...");
        bool hasNav = ai_.hasNavMesh();
        std::println("  Has NavMesh: {}", hasNav);

        // loadNavMesh
        std::println("\nTesting loadNavMesh()...");
        auto navMeshHandle = assets_.registerAsset(jframe::AssetType::NavMesh,
                                                    "navmesh/level1.navmesh");
        std::println("  Registered NavMesh asset: UUID {}", navMeshHandle.uuid);

        ai_.loadNavMesh(navMeshHandle);
        std::println("  NavMesh loaded successfully");

        // hasNavMesh (after loading)
        hasNav = ai_.hasNavMesh();
        std::println("\nTesting hasNavMesh() after loading...");
        std::println("  Has NavMesh: {}", hasNav);

        // unloadNavMesh
        std::println("\nTesting unloadNavMesh()...");
        ai_.unloadNavMesh();
        hasNav = ai_.hasNavMesh();
        std::println("  NavMesh unloaded. Has NavMesh: {}", hasNav);

        // Reload for subsequent tests
        std::println("\nReloading NavMesh for pathfinding tests...");
        ai_.loadNavMesh(navMeshHandle);
        std::println("  NavMesh reloaded");
    }

    void demoPathfinding() {
        printSection("Pathfinding");

        std::println("Testing pathfinding queries...\n");

        // isPointOnNavMesh
        std::println("Testing isPointOnNavMesh()...");
        jframe::Vec2 validPoint{100.0f, 100.0f};
        jframe::Vec2 invalidPoint{-999.0f, -999.0f};

        bool onMesh1 = ai_.isPointOnNavMesh(validPoint);
        bool onMesh2 = ai_.isPointOnNavMesh(invalidPoint);

        std::println("  Point ({}, {}) on NavMesh: {}",
                     validPoint.x, validPoint.y, onMesh1);
        std::println("  Point ({}, {}) on NavMesh: {}",
                     invalidPoint.x, invalidPoint.y, onMesh2);

        // getClosestPointOnNavMesh
        std::println("\nTesting getClosestPointOnNavMesh()...");
        jframe::Vec2 queryPoint{105.5f, 99.3f};

        auto closestPoint = ai_.getClosestPointOnNavMesh(queryPoint);
        if (closestPoint) {
            std::println("  Query point: ({}, {})", queryPoint.x, queryPoint.y);
            std::println("  Closest point on NavMesh: ({}, {})",
                         closestPoint->x, closestPoint->y);
        } else {
            std::println("  No closest point found (NavMesh may be empty)");
        }

        // findPath
        std::println("\nTesting findPath()...");

        jframe::NavMeshQuery query1{
            .start = {50.0f, 50.0f},
            .end = {200.0f, 150.0f},
            .agentRadius = 0.5f
        };

        std::println("  Query 1:");
        std::println("    Start: ({}, {})", query1.start.x, query1.start.y);
        std::println("    End: ({}, {})", query1.end.x, query1.end.y);
        std::println("    Agent radius: {}", query1.agentRadius);

        auto path1 = ai_.findPath(query1);
        if (path1) {
            std::println("    Path found:");
            std::println("      Waypoints: {}", path1->waypoints.size());
            std::println("      Total length: {:.2f}", path1->totalLength);
            std::println("      Is complete: {}", path1->isComplete);

            if (!path1->waypoints.empty()) {
                std::println("      First waypoint: ({}, {})",
                             path1->waypoints[0].x, path1->waypoints[0].y);
                std::println("      Last waypoint: ({}, {})",
                             path1->waypoints.back().x, path1->waypoints.back().y);
            }
        } else {
            std::println("    No path found");
        }

        // Test with different agent radius
        jframe::NavMeshQuery query2{
            .start = {100.0f, 100.0f},
            .end = {300.0f, 200.0f},
            .agentRadius = 1.0f
        };

        std::println("\n  Query 2 (larger agent):");
        std::println("    Start: ({}, {})", query2.start.x, query2.start.y);
        std::println("    End: ({}, {})", query2.end.x, query2.end.y);
        std::println("    Agent radius: {}", query2.agentRadius);

        auto path2 = ai_.findPath(query2);
        if (path2) {
            std::println("    Path found:");
            std::println("      Waypoints: {}", path2->waypoints.size());
            std::println("      Total length: {:.2f}", path2->totalLength);
            std::println("      Is complete: {}", path2->isComplete);
        } else {
            std::println("    No path found");
        }
    }

    void demoSteering() {
        printSection("Steering Behaviors");

        auto agent = createAIAgent("SteeringAgent", 500.0f, 100.0f);

        std::println("Testing steering target management...\n");

        // setNavigationTarget
        std::println("Testing setNavigationTarget()...");
        jframe::Vec2 target1{600.0f, 200.0f};
        ai_.setNavigationTarget(agent, target1);
        std::println("  Set navigation target to ({}, {})", target1.x, target1.y);

        // getNavigationTarget
        std::println("\nTesting getNavigationTarget()...");
        auto retrievedTarget = ai_.getNavigationTarget(agent);
        if (retrievedTarget) {
            std::println("  Current target: ({}, {})",
                         retrievedTarget->x, retrievedTarget->y);
        } else {
            std::println("  No navigation target set");
        }

        // Update to new target
        std::println("\nChanging navigation target...");
        jframe::Vec2 target2{700.0f, 300.0f};
        ai_.setNavigationTarget(agent, target2);
        retrievedTarget = ai_.getNavigationTarget(agent);
        if (retrievedTarget) {
            std::println("  New target: ({}, {})",
                         retrievedTarget->x, retrievedTarget->y);
        }

        // clearNavigationTarget
        std::println("\nTesting clearNavigationTarget()...");
        ai_.clearNavigationTarget(agent);
        retrievedTarget = ai_.getNavigationTarget(agent);
        std::println("  Target cleared. Has target: {}", retrievedTarget.has_value());
    }

    void demoMovementParameters() {
        printSection("Movement Parameters");

        auto agent = createAIAgent("MovementAgent", 600.0f, 100.0f);

        std::println("Testing movement parameter configuration...\n");

        // setMaxSpeed
        std::println("Testing setMaxSpeed()...");
        ai_.setMaxSpeed(agent, 150.0f);
        std::println("  Set max speed to 150.0 units/sec");

        ai_.setMaxSpeed(agent, 75.0f);
        std::println("  Updated max speed to 75.0 units/sec");

        // setMaxAcceleration
        std::println("\nTesting setMaxAcceleration()...");
        ai_.setMaxAcceleration(agent, 600.0f);
        std::println("  Set max acceleration to 600.0 units/sec²");

        ai_.setMaxAcceleration(agent, 300.0f);
        std::println("  Updated max acceleration to 300.0 units/sec²");

        std::println("\nMovement parameters configured:");
        std::println("  Final max speed: 75.0 units/sec");
        std::println("  Final max acceleration: 300.0 units/sec²");
    }

    void demoPatrolBehavior() {
        printSection("Patrol Behavior");

        std::println("Testing patrol behavior configuration...\n");

        // Create patrol entities
        auto patroller1 = createAIAgent("Patroller1", 100.0f, 300.0f);
        auto patroller2 = createAIAgent("Patroller2", 300.0f, 300.0f);

        entities_.emplace<PatrollerTag>(patroller1);
        entities_.emplace<PatrollerTag>(patroller2);

        // setPatrolBehavior
        std::println("Testing setPatrolBehavior()...");

        jframe::PatrolBehavior patrol1{
            .startX = 100.0f,
            .range = 150.0f,
            .speed = 60.0f,
            .movingRight = true
        };

        ai_.setPatrolBehavior(patroller1, patrol1);
        std::println("  Patroller1 patrol config:");
        std::println("    Start X: {}", patrol1.startX);
        std::println("    Range: {}", patrol1.range);
        std::println("    Speed: {}", patrol1.speed);
        std::println("    Moving right: {}", patrol1.movingRight);

        jframe::PatrolBehavior patrol2{
            .startX = 300.0f,
            .range = 200.0f,
            .speed = 80.0f,
            .movingRight = false
        };

        ai_.setPatrolBehavior(patroller2, patrol2);
        std::println("\n  Patroller2 patrol config:");
        std::println("    Start X: {}", patrol2.startX);
        std::println("    Range: {}", patrol2.range);
        std::println("    Speed: {}", patrol2.speed);
        std::println("    Moving right: {}", patrol2.movingRight);

        // getPatrolBehavior
        std::println("\nTesting getPatrolBehavior()...");

        auto retrievedPatrol1 = ai_.getPatrolBehavior(patroller1);
        if (retrievedPatrol1) {
            std::println("  Patroller1 patrol retrieved:");
            std::println("    Start X: {}", retrievedPatrol1->startX);
            std::println("    Range: {}", retrievedPatrol1->range);
            std::println("    Speed: {}", retrievedPatrol1->speed);
            std::println("    Moving right: {}", retrievedPatrol1->movingRight);
        } else {
            std::println("  No patrol behavior found for Patroller1");
        }

        // Update patrol behavior
        std::println("\nUpdating Patroller1 patrol behavior...");
        patrol1.speed = 100.0f;
        patrol1.movingRight = false;
        ai_.setPatrolBehavior(patroller1, patrol1);

        retrievedPatrol1 = ai_.getPatrolBehavior(patroller1);
        if (retrievedPatrol1) {
            std::println("  Updated patrol:");
            std::println("    Speed: {}", retrievedPatrol1->speed);
            std::println("    Moving right: {}", retrievedPatrol1->movingRight);
        }

        // clearPatrolBehavior
        std::println("\nTesting clearPatrolBehavior()...");
        ai_.clearPatrolBehavior(patroller1);
        retrievedPatrol1 = ai_.getPatrolBehavior(patroller1);
        std::println("  Patrol cleared. Has patrol: {}", retrievedPatrol1.has_value());
    }

    void demoSpatialQueries() {
        printSection("Spatial Queries");

        std::println("Setting up test scenario with multiple entities...\n");

        // Create entities at various positions with physics bodies
        auto entity1 = createPhysicsEntity("Entity1", 100.0f, 100.0f, 0x0001);
        auto entity2 = createPhysicsEntity("Entity2", 150.0f, 120.0f, 0x0001);
        auto entity3 = createPhysicsEntity("Entity3", 200.0f, 100.0f, 0x0002);
        auto entity4 = createPhysicsEntity("Entity4", 500.0f, 100.0f, 0x0001);
        auto entity5 = createPhysicsEntity("Entity5", 120.0f, 140.0f, 0x0004);

        std::println("Created 5 entities:");
        std::println("  Entity1 at (100, 100) - Mask: 0x0001");
        std::println("  Entity2 at (150, 120) - Mask: 0x0001");
        std::println("  Entity3 at (200, 100) - Mask: 0x0002");
        std::println("  Entity4 at (500, 100) - Mask: 0x0001");
        std::println("  Entity5 at (120, 140) - Mask: 0x0004");

        // findEntitiesInRadius
        std::println("\nTesting findEntitiesInRadius()...");

        jframe::Vec2 center1{125.0f, 110.0f};
        float radius1 = 50.0f;

        std::println("  Query 1: Center ({}, {}), Radius {}",
                     center1.x, center1.y, radius1);
        auto nearbyAll = ai_.findEntitiesInRadius(center1, radius1);
        std::println("    Found {} entities (all masks)", nearbyAll.size());

        for (auto entity : nearbyAll) {
            if (auto* name = entities_.tryGet<Name>(entity)) {
                auto* pos = entities_.tryGet<Position>(entity);
                std::println("      {} at ({}, {})", name->value,
                             pos ? pos->x : 0.0f, pos ? pos->y : 0.0f);
            }
        }

        // With collision mask filtering
        std::println("\n  Query 2: Same location, Mask filter 0x0001");
        auto nearby0001 = ai_.findEntitiesInRadius(center1, radius1, 0x0001);
        std::println("    Found {} entities with mask 0x0001", nearby0001.size());

        std::println("\n  Query 3: Same location, Mask filter 0x0002");
        auto nearby0002 = ai_.findEntitiesInRadius(center1, radius1, 0x0002);
        std::println("    Found {} entities with mask 0x0002", nearby0002.size());

        // Larger radius
        float radius2 = 100.0f;
        std::println("\n  Query 4: Center ({}, {}), Radius {}",
                     center1.x, center1.y, radius2);
        auto nearbyLarge = ai_.findEntitiesInRadius(center1, radius2);
        std::println("    Found {} entities", nearbyLarge.size());

        // findClosestEntity
        std::println("\nTesting findClosestEntity()...");

        jframe::Vec2 queryPos{140.0f, 115.0f};
        std::println("  Query position: ({}, {})", queryPos.x, queryPos.y);

        auto closestAll = ai_.findClosestEntity(queryPos);
        if (closestAll) {
            if (auto* name = entities_.tryGet<Name>(*closestAll)) {
                auto* pos = entities_.tryGet<Position>(*closestAll);
                std::println("    Closest entity (all masks): {} at ({}, {})",
                             name->value,
                             pos ? pos->x : 0.0f, pos ? pos->y : 0.0f);
            }
        } else {
            std::println("    No entity found");
        }

        // With mask filter
        auto closest0002 = ai_.findClosestEntity(queryPos, 0x0002);
        if (closest0002) {
            if (auto* name = entities_.tryGet<Name>(*closest0002)) {
                auto* pos = entities_.tryGet<Position>(*closest0002);
                std::println("    Closest entity (mask 0x0002): {} at ({}, {})",
                             name->value,
                             pos ? pos->x : 0.0f, pos ? pos->y : 0.0f);
            }
        } else {
            std::println("    No entity found with mask 0x0002");
        }

        // hasLineOfSight
        std::println("\nTesting hasLineOfSight()...");

        jframe::Vec2 from{100.0f, 100.0f};
        jframe::Vec2 to1{150.0f, 120.0f};
        jframe::Vec2 to2{500.0f, 100.0f};

        std::println("  From ({}, {}) to ({}, {}):",
                     from.x, from.y, to1.x, to1.y);
        bool los1 = ai_.hasLineOfSight(from, to1);
        std::println("    Line of sight: {}", los1);

        std::println("  From ({}, {}) to ({}, {}):",
                     from.x, from.y, to2.x, to2.y);
        bool los2 = ai_.hasLineOfSight(from, to2);
        std::println("    Line of sight: {}", los2);

        // With obstacle mask
        std::println("  From ({}, {}) to ({}, {}) with obstacle mask 0x0002:",
                     from.x, from.y, to2.x, to2.y);
        bool los3 = ai_.hasLineOfSight(from, to2, 0x0002);
        std::println("    Line of sight: {}", los3);
    }

    void demoComplexAIScenario() {
        printSection("Complex AI Scenario: Guard AI System");

        std::println("Setting up a complex guard AI scenario...\n");

        // Create player entity
        auto player = createPhysicsEntity("Player", 50.0f, 50.0f, 0x0001);
        std::println("Created player at (50, 50)");

        // Create guard entities with full AI setup
        std::vector<jframe::Entity> guards;

        for (int i = 0; i < 3; ++i) {
            float x = 200.0f + i * 150.0f;
            float y = 100.0f;

            auto guard = createPhysicsEntity(
                std::format("Guard{}", i + 1), x, y, 0x0002);
            entities_.emplace<EnemyTag>(guard);
            guards.push_back(guard);

            // Attach behavior tree
            auto btHandle = assets_.registerAsset(
                jframe::AssetType::BehaviorTree,
                std::format("behaviors/guard{}.bt", i + 1));
            ai_.attachBehaviorTree(guard, btHandle);

            // Configure patrol
            jframe::PatrolBehavior patrol{
                .startX = x,
                .range = 100.0f,
                .speed = 50.0f,
                .movingRight = (i % 2 == 0)
            };
            ai_.setPatrolBehavior(guard, patrol);

            // Set movement params
            ai_.setMaxSpeed(guard, 100.0f);
            ai_.setMaxAcceleration(guard, 400.0f);

            // Initialize blackboard
            ai_.setBehaviorTreeBlackboard(guard, "state", std::any(std::string("PATROL")));
            ai_.setBehaviorTreeBlackboard(guard, "alert_level", std::any(0));
            ai_.setBehaviorTreeBlackboard(guard, "detection_radius", std::any(100.0f));

            std::println("  Guard{} created at ({}, {}) - Patrol range: {}",
                         i + 1, x, y, patrol.range);
        }

        // Simulate AI decision making
        std::println("\n--- AI Update Cycle 1: Patrol State ---");
        ai_.update(0.016f);

        for (size_t i = 0; i < guards.size(); ++i) {
            auto state = std::any_cast<std::string>(
                ai_.getBehaviorTreeBlackboard(guards[i], "state"));
            std::println("  Guard{} state: {}", i + 1, state);
        }

        // Player enters detection range
        std::println("\n--- Player moves to (220, 105) ---");
        if (auto* playerPos = entities_.tryGet<Position>(player)) {
            playerPos->x = 220.0f;
            playerPos->y = 105.0f;
        }

        // Check which guards detect player
        std::println("\nChecking guard detection...");
        for (size_t i = 0; i < guards.size(); ++i) {
            auto* guardPos = entities_.tryGet<Position>(guards[i]);
            auto* playerPos = entities_.tryGet<Position>(player);

            if (guardPos && playerPos) {
                float dx = playerPos->x - guardPos->x;
                float dy = playerPos->y - guardPos->y;
                float distance = std::sqrt(dx * dx + dy * dy);

                auto detectionRadius = std::any_cast<float>(
                    ai_.getBehaviorTreeBlackboard(guards[i], "detection_radius"));

                if (distance <= detectionRadius) {
                    std::println("  Guard{} detected player! Distance: {:.1f}",
                                 i + 1, distance);

                    // Update guard state
                    ai_.setBehaviorTreeBlackboard(guards[i], "state",
                                                  std::any(std::string("ALERT")));
                    ai_.setBehaviorTreeBlackboard(guards[i], "alert_level",
                                                  std::any(3));
                    ai_.clearPatrolBehavior(guards[i]);
                    ai_.setNavigationTarget(guards[i],
                                           jframe::Vec2{playerPos->x, playerPos->y});
                } else {
                    std::println("  Guard{} still patrolling. Distance: {:.1f}",
                                 i + 1, distance);
                }
            }
        }

        // Update AI again
        std::println("\n--- AI Update Cycle 2: Alert State ---");
        ai_.update(0.016f);

        for (size_t i = 0; i < guards.size(); ++i) {
            auto state = std::any_cast<std::string>(
                ai_.getBehaviorTreeBlackboard(guards[i], "state"));
            auto alertLevel = std::any_cast<int>(
                ai_.getBehaviorTreeBlackboard(guards[i], "alert_level"));

            std::println("  Guard{} - State: {}, Alert: {}",
                         i + 1, state, alertLevel);

            if (auto target = ai_.getNavigationTarget(guards[i])) {
                std::println("    Navigating to: ({:.1f}, {:.1f})",
                             target->x, target->y);
            }
        }

        // Check line of sight between guards and player
        std::println("\nLine of sight checks:");
        auto* playerPos = entities_.tryGet<Position>(player);
        for (size_t i = 0; i < guards.size(); ++i) {
            auto* guardPos = entities_.tryGet<Position>(guards[i]);
            if (guardPos && playerPos) {
                bool los = ai_.hasLineOfSight(
                    jframe::Vec2{guardPos->x, guardPos->y},
                    jframe::Vec2{playerPos->x, playerPos->y});
                std::println("  Guard{} -> Player: {}", i + 1,
                             los ? "Clear" : "Blocked");
            }
        }

        // Find all enemies near player
        std::println("\nSpatial query - enemies near player:");
        if (playerPos) {
            auto nearbyEnemies = ai_.findEntitiesInRadius(
                jframe::Vec2{playerPos->x, playerPos->y}, 150.0f, 0x0002);
            std::println("  Found {} enemies within 150 units", nearbyEnemies.size());

            // Find closest enemy
            auto closestEnemy = ai_.findClosestEntity(
                jframe::Vec2{playerPos->x, playerPos->y}, 0x0002);
            if (closestEnemy) {
                if (auto* name = entities_.tryGet<Name>(*closestEnemy)) {
                    std::println("  Closest enemy: {}", name->value);
                }
            }
        }

        std::println("\nComplex AI scenario completed successfully!");
    }

    //==========================================================================
    // Helper Functions
    //==========================================================================

    jframe::Entity createAIAgent(const std::string& name, float x, float y) {
        auto entity = entities_.createEntity();
        entities_.emplace<Position>(entity, x, y);
        entities_.emplace<Name>(entity, name);
        entities_.emplace<AIAgentTag>(entity);
        testEntities_.push_back(entity);
        return entity;
    }

    jframe::Entity createPhysicsEntity(const std::string& name,
                                       float x, float y,
                                       jframe::CollisionMask mask) {
        auto entity = entities_.createEntity();
        entities_.emplace<Position>(entity, x, y);
        entities_.emplace<Name>(entity, name);

        // Create physics body
        jframe::PhysicsBodyDef bodyDef{
            .type = jframe::BodyType::Dynamic,
            .transform = {.x = x, .y = y},
            .size = {32.0f, 32.0f},
            .fixedRotation = true
        };
        physics_.createBody(entity, bodyDef);

        // Set collision mask (using layer as mask for demo)
        physics_.setCollisionLayer(entity, static_cast<jframe::CollisionLayer>(mask));

        testEntities_.push_back(entity);
        return entity;
    }

    void printHeader(const std::string& title) {
        std::println("\n{:=^70}", "");
        std::println("{:^70}", title);
        std::println("{:=^70}\n", "");
    }

    void printSection(const std::string& title) {
        std::println("\n{:-^70}", "");
        std::println("{:^70}", title);
        std::println("{:-^70}\n", "");
    }
};

} // namespace demo
