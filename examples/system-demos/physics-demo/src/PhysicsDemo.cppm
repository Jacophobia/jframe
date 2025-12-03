// PhysicsDemo.cppm
// Comprehensive demonstration of the JFrame Physics System API

module;

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <print>
#include <string>
#include <vector>

export module physics.demo;
import jframe.types;
import jframe.physics;

export namespace demo {

//==========================================================================
// PhysicsDemo Class
//==========================================================================

class PhysicsDemo {
public:
    explicit PhysicsDemo(jframe::IPhysicsSystem& physics)
        : physics_(physics) {}

    void run() {
        printHeader("JFRAME PHYSICS SYSTEM COMPREHENSIVE DEMO");

        demoWorldSettings();
        demoBodyLifecycle();
        demoBodyTypes();
        demoBodyProperties();
        demoVelocity();
        demoForces();
        demoCollisionFiltering();
        demoSpatialQueries();
        demoRaycasting();
        demoCollisionCallbacks();
        demoGroundDetection();
        demoComplexSimulation();

        printHeader("DEMO COMPLETE");
    }

private:
    jframe::IPhysicsSystem& physics_;
    std::vector<jframe::Entity> trackedEntities_;
    int nextEntityId_ = 1;

    //==========================================================================
    // Demo Sections
    //==========================================================================

    void demoWorldSettings() {
        printSection("World Settings");

        // getGravity()
        std::println("Default gravity:");
        auto defaultGravity = physics_.getGravity();
        std::println("  Gravity: ({}, {})", defaultGravity.x, defaultGravity.y);

        // setGravity()
        std::println("\nSetting custom gravity...");
        physics_.setGravity({0.0f, 980.0f});  // Standard Earth gravity in pixels/s²
        auto newGravity = physics_.getGravity();
        std::println("  New gravity: ({}, {})", newGravity.x, newGravity.y);

        std::println("\nTesting zero gravity...");
        physics_.setGravity({0.0f, 0.0f});
        auto zeroGravity = physics_.getGravity();
        std::println("  Zero gravity: ({}, {})", zeroGravity.x, zeroGravity.y);

        // Restore normal gravity
        physics_.setGravity({0.0f, 980.0f});
        std::println("\nGravity restored to: ({}, {})",
                     physics_.getGravity().x, physics_.getGravity().y);
    }

    void demoBodyLifecycle() {
        printSection("Body Lifecycle");

        // createBody()
        std::println("Creating physics bodies...");

        auto e1 = createMockEntity();
        jframe::PhysicsBodyDef def1{
            .type = jframe::BodyType::Dynamic,
            .transform = {.x = 100.0f, .y = 100.0f},
            .size = {32.0f, 32.0f}
        };
        physics_.createBody(e1, def1);
        std::println("  Created dynamic body for entity {}", entityId(e1));

        auto e2 = createMockEntity();
        jframe::PhysicsBodyDef def2{
            .type = jframe::BodyType::Static,
            .transform = {.x = 200.0f, .y = 400.0f},
            .size = {100.0f, 20.0f}
        };
        physics_.createBody(e2, def2);
        std::println("  Created static body for entity {}", entityId(e2));

        // hasBody()
        std::println("\nChecking body existence...");
        std::println("  Entity {} has body: {}", entityId(e1), physics_.hasBody(e1));
        std::println("  Entity {} has body: {}", entityId(e2), physics_.hasBody(e2));

        auto e3 = createMockEntity();
        std::println("  Entity {} (no body) has body: {}", entityId(e3), physics_.hasBody(e3));

        // destroyBody()
        std::println("\nDestroying body for entity {}...", entityId(e2));
        physics_.destroyBody(e2);
        std::println("  Entity {} has body after destroy: {}", entityId(e2), physics_.hasBody(e2));

        // Cleanup
        physics_.destroyBody(e1);
    }

    void demoBodyTypes() {
        printSection("Body Types");

        auto entity = createMockEntity();

        // Create as Dynamic
        std::println("Creating Dynamic body...");
        jframe::PhysicsBodyDef def{
            .type = jframe::BodyType::Dynamic,
            .transform = {.x = 100.0f, .y = 100.0f},
            .size = {32.0f, 32.0f}
        };
        physics_.createBody(entity, def);

        // getBodyType()
        auto currentType = physics_.getBodyType(entity);
        std::println("  Current type: {}", bodyTypeToString(currentType));

        // setBodyType() - Change to Static
        std::println("\nChanging to Static...");
        physics_.setBodyType(entity, jframe::BodyType::Static);
        std::println("  New type: {}", bodyTypeToString(physics_.getBodyType(entity)));

        // Change to Kinematic
        std::println("\nChanging to Kinematic...");
        physics_.setBodyType(entity, jframe::BodyType::Kinematic);
        std::println("  New type: {}", bodyTypeToString(physics_.getBodyType(entity)));

        // Change back to Dynamic
        std::println("\nChanging back to Dynamic...");
        physics_.setBodyType(entity, jframe::BodyType::Dynamic);
        std::println("  Final type: {}", bodyTypeToString(physics_.getBodyType(entity)));

        physics_.destroyBody(entity);
    }

    void demoBodyProperties() {
        printSection("Body Properties");

        auto entity = createMockEntity();
        jframe::PhysicsBodyDef def{
            .type = jframe::BodyType::Dynamic,
            .transform = {.x = 100.0f, .y = 100.0f, .rotation = 0.0f},
            .size = {32.0f, 48.0f}
        };
        physics_.createBody(entity, def);

        // getPosition()
        std::println("Position management:");
        auto pos = physics_.getPosition(entity);
        std::println("  Initial position: ({}, {})", pos.x, pos.y);

        // setPosition()
        std::println("\nSetting new position...");
        physics_.setPosition(entity, {200.0f, 150.0f});
        auto newPos = physics_.getPosition(entity);
        std::println("  New position: ({}, {})", newPos.x, newPos.y);

        // getRotation()
        std::println("\nRotation management:");
        auto rot = physics_.getRotation(entity);
        std::println("  Initial rotation: {} radians", rot);

        // setRotation()
        std::println("\nSetting rotation to 45 degrees...");
        float deg45 = 3.14159f / 4.0f;  // 45 degrees in radians
        physics_.setRotation(entity, deg45);
        auto newRot = physics_.getRotation(entity);
        std::println("  New rotation: {} radians ({} degrees)",
                     newRot, newRot * 180.0f / 3.14159f);

        // getBodySize()
        std::println("\nBody size:");
        auto size = physics_.getBodySize(entity);
        std::println("  Size: ({}, {})", size.x, size.y);

        physics_.destroyBody(entity);
    }

    void demoVelocity() {
        printSection("Velocity");

        auto entity = createMockEntity();
        jframe::PhysicsBodyDef def{
            .type = jframe::BodyType::Dynamic,
            .transform = {.x = 100.0f, .y = 100.0f},
            .size = {32.0f, 32.0f}
        };
        physics_.createBody(entity, def);

        // getVelocity()
        std::println("Linear velocity:");
        auto vel = physics_.getVelocity(entity);
        std::println("  Initial velocity: ({}, {})", vel.x, vel.y);

        // setVelocity()
        std::println("\nSetting linear velocity...");
        physics_.setVelocity(entity, {100.0f, -50.0f});
        auto newVel = physics_.getVelocity(entity);
        std::println("  New velocity: ({}, {})", newVel.x, newVel.y);

        // getAngularVelocity()
        std::println("\nAngular velocity:");
        auto angVel = physics_.getAngularVelocity(entity);
        std::println("  Initial angular velocity: {} rad/s", angVel);

        // setAngularVelocity()
        std::println("\nSetting angular velocity...");
        physics_.setAngularVelocity(entity, 3.14159f);  // π rad/s (180°/s)
        auto newAngVel = physics_.getAngularVelocity(entity);
        std::println("  New angular velocity: {} rad/s ({} deg/s)",
                     newAngVel, newAngVel * 180.0f / 3.14159f);

        physics_.destroyBody(entity);
    }

    void demoForces() {
        printSection("Forces and Impulses");

        auto entity = createMockEntity();
        jframe::PhysicsBodyDef def{
            .type = jframe::BodyType::Dynamic,
            .transform = {.x = 100.0f, .y = 100.0f},
            .size = {32.0f, 32.0f}
        };
        physics_.createBody(entity, def);

        // applyForce()
        std::println("Applying continuous force...");
        auto initialVel = physics_.getVelocity(entity);
        std::println("  Velocity before force: ({}, {})", initialVel.x, initialVel.y);

        physics_.applyForce(entity, {1000.0f, 0.0f});
        std::println("  Applied force: (1000, 0)");

        // Need to step the simulation to see effect
        physics_.update(0.016f);
        auto velAfterForce = physics_.getVelocity(entity);
        std::println("  Velocity after 1 step: ({}, {})", velAfterForce.x, velAfterForce.y);

        // applyImpulse()
        std::println("\nApplying instant impulse...");
        physics_.setVelocity(entity, {0.0f, 0.0f});  // Reset velocity
        auto velBefore = physics_.getVelocity(entity);
        std::println("  Velocity before impulse: ({}, {})", velBefore.x, velBefore.y);

        physics_.applyImpulse(entity, {50.0f, -100.0f});
        std::println("  Applied impulse: (50, -100)");

        auto velAfterImpulse = physics_.getVelocity(entity);
        std::println("  Velocity after impulse: ({}, {})",
                     velAfterImpulse.x, velAfterImpulse.y);

        // applyTorque()
        std::println("\nApplying torque...");
        physics_.setAngularVelocity(entity, 0.0f);  // Reset rotation
        auto angVelBefore = physics_.getAngularVelocity(entity);
        std::println("  Angular velocity before torque: {} rad/s", angVelBefore);

        physics_.applyTorque(entity, 5000.0f);
        std::println("  Applied torque: 5000");

        physics_.update(0.016f);
        auto angVelAfter = physics_.getAngularVelocity(entity);
        std::println("  Angular velocity after 1 step: {} rad/s", angVelAfter);

        // applyForce with offset point
        std::println("\nApplying force at offset point (creates torque)...");
        physics_.setVelocity(entity, {0.0f, 0.0f});
        physics_.setAngularVelocity(entity, 0.0f);

        physics_.applyForce(entity, {1000.0f, 0.0f}, {0.0f, 16.0f});  // Force at top
        std::println("  Applied force (1000, 0) at offset (0, 16)");

        physics_.update(0.016f);
        auto finalVel = physics_.getVelocity(entity);
        auto finalAngVel = physics_.getAngularVelocity(entity);
        std::println("  Linear velocity: ({}, {})", finalVel.x, finalVel.y);
        std::println("  Angular velocity: {} rad/s", finalAngVel);

        physics_.destroyBody(entity);
    }

    void demoCollisionFiltering() {
        printSection("Collision Filtering");

        auto entity1 = createMockEntity();
        auto entity2 = createMockEntity();

        jframe::PhysicsBodyDef def{
            .type = jframe::BodyType::Dynamic,
            .transform = {.x = 100.0f, .y = 100.0f},
            .size = {32.0f, 32.0f}
        };
        physics_.createBody(entity1, def);

        def.transform.x = 150.0f;
        physics_.createBody(entity2, def);

        // setCollisionLayer()
        std::println("Setting collision layers...");
        physics_.setCollisionLayer(entity1, jframe::CollisionLayers::Player);
        physics_.setCollisionLayer(entity2, jframe::CollisionLayers::Enemy);
        std::println("  Entity {} set to Player layer (0x{:04X})",
                     entityId(entity1), jframe::CollisionLayers::Player);
        std::println("  Entity {} set to Enemy layer (0x{:04X})",
                     entityId(entity2), jframe::CollisionLayers::Enemy);

        // getCollisionLayer()
        std::println("\nRetrieving collision layers...");
        auto layer1 = physics_.getCollisionLayer(entity1);
        auto layer2 = physics_.getCollisionLayer(entity2);
        std::println("  Entity {} layer: 0x{:04X}", entityId(entity1), layer1);
        std::println("  Entity {} layer: 0x{:04X}", entityId(entity2), layer2);

        // setCollisionMask()
        std::println("\nSetting collision masks...");
        // Player collides with everything except Projectile
        jframe::CollisionMask playerMask = 0xFFFF & ~jframe::CollisionLayers::Projectile;
        physics_.setCollisionMask(entity1, playerMask);
        std::println("  Player mask: 0x{:04X} (collides with most things)", playerMask);

        // Enemy only collides with Player and Terrain
        jframe::CollisionMask enemyMask =
            jframe::CollisionLayers::Player | jframe::CollisionLayers::Terrain;
        physics_.setCollisionMask(entity2, enemyMask);
        std::println("  Enemy mask: 0x{:04X} (only Player and Terrain)", enemyMask);

        // setSensor()
        std::println("\nCreating sensor bodies...");
        auto trigger = createMockEntity();
        jframe::PhysicsBodyDef triggerDef{
            .type = jframe::BodyType::Static,
            .transform = {.x = 200.0f, .y = 100.0f},
            .size = {50.0f, 50.0f},
            .isSensor = true
        };
        physics_.createBody(trigger, triggerDef);
        physics_.setCollisionLayer(trigger, jframe::CollisionLayers::Trigger);
        std::println("  Created sensor trigger at (200, 100)");

        // Change sensor status
        std::println("\nToggling sensor status...");
        physics_.setSensor(entity1, true);
        std::println("  Entity {} is now a sensor", entityId(entity1));

        physics_.setSensor(entity1, false);
        std::println("  Entity {} is now a normal body", entityId(entity1));

        // Demonstrate layer usage
        std::println("\nCommon collision layer patterns:");
        std::println("  Player:      0x{:04X}", jframe::CollisionLayers::Player);
        std::println("  Enemy:       0x{:04X}", jframe::CollisionLayers::Enemy);
        std::println("  Projectile:  0x{:04X}", jframe::CollisionLayers::Projectile);
        std::println("  Terrain:     0x{:04X}", jframe::CollisionLayers::Terrain);
        std::println("  Trigger:     0x{:04X}", jframe::CollisionLayers::Trigger);
        std::println("  Collectible: 0x{:04X}", jframe::CollisionLayers::Collectible);
        std::println("  Ground:      0x{:04X}", jframe::CollisionLayers::Ground);

        physics_.destroyBody(entity1);
        physics_.destroyBody(entity2);
        physics_.destroyBody(trigger);
    }

    void demoSpatialQueries() {
        printSection("Spatial Queries");

        // Create test bodies in various positions
        std::println("Setting up test bodies...");

        auto e1 = createMockEntity();
        jframe::PhysicsBodyDef def{
            .type = jframe::BodyType::Static,
            .transform = {.x = 100.0f, .y = 100.0f},
            .size = {20.0f, 20.0f}
        };
        physics_.createBody(e1, def);
        std::println("  Entity {} at (100, 100)", entityId(e1));

        auto e2 = createMockEntity();
        def.transform = {.x = 150.0f, .y = 120.0f};
        physics_.createBody(e2, def);
        std::println("  Entity {} at (150, 120)", entityId(e2));

        auto e3 = createMockEntity();
        def.transform = {.x = 200.0f, .y = 100.0f};
        physics_.createBody(e3, def);
        std::println("  Entity {} at (200, 100)", entityId(e3));

        auto e4 = createMockEntity();
        def.transform = {.x = 300.0f, .y = 300.0f};
        physics_.createBody(e4, def);
        std::println("  Entity {} at (300, 300)", entityId(e4));

        // queryAABB()
        std::println("\nQuery AABB (50,50) to (180,150)...");
        auto aabbResults = physics_.queryAABB({50.0f, 50.0f}, {180.0f, 150.0f});
        std::println("  Found {} entities in AABB:", aabbResults.size());
        for (auto entity : aabbResults) {
            auto pos = physics_.getPosition(entity);
            std::println("    Entity {} at ({}, {})", entityId(entity), pos.x, pos.y);
        }

        // queryCircle()
        std::println("\nQuery circle at (150, 110) with radius 60...");
        auto circleResults = physics_.queryCircle({150.0f, 110.0f}, 60.0f);
        std::println("  Found {} entities in circle:", circleResults.size());
        for (auto entity : circleResults) {
            auto pos = physics_.getPosition(entity);
            float dx = pos.x - 150.0f;
            float dy = pos.y - 110.0f;
            float dist = std::sqrt(dx * dx + dy * dy);
            std::println("    Entity {} at ({}, {}) - distance: {}",
                         entityId(entity), pos.x, pos.y, dist);
        }

        physics_.destroyBody(e1);
        physics_.destroyBody(e2);
        physics_.destroyBody(e3);
        physics_.destroyBody(e4);
    }

    void demoRaycasting() {
        printSection("Raycasting");

        // Create obstacles
        std::println("Setting up obstacles for raycasting...");

        auto wall1 = createMockEntity();
        jframe::PhysicsBodyDef def{
            .type = jframe::BodyType::Static,
            .transform = {.x = 200.0f, .y = 100.0f},
            .size = {20.0f, 100.0f}
        };
        physics_.createBody(wall1, def);
        physics_.setCollisionLayer(wall1, jframe::CollisionLayers::Terrain);
        std::println("  Wall 1 at (200, 100)");

        auto wall2 = createMockEntity();
        def.transform = {.x = 300.0f, .y = 100.0f};
        physics_.createBody(wall2, def);
        physics_.setCollisionLayer(wall2, jframe::CollisionLayers::Terrain);
        std::println("  Wall 2 at (300, 100)");

        auto wall3 = createMockEntity();
        def.transform = {.x = 400.0f, .y = 100.0f};
        physics_.createBody(wall3, def);
        physics_.setCollisionLayer(wall3, jframe::CollisionLayers::Enemy);
        std::println("  Wall 3 at (400, 100) [Enemy layer]");

        // raycast() - single hit
        std::println("\nRaycast from (50, 100) rightward, max 500...");
        auto hit = physics_.raycast({50.0f, 100.0f}, {1.0f, 0.0f}, 500.0f);
        if (hit) {
            std::println("  HIT!");
            std::println("    Entity: {}", entityId(hit->entity));
            std::println("    Point: ({}, {})", hit->point.x, hit->point.y);
            std::println("    Normal: ({}, {})", hit->normal.x, hit->normal.y);
            std::println("    Distance: {}", hit->distance);
        } else {
            std::println("  No hit");
        }

        // raycast() with mask filter
        std::println("\nRaycast with Enemy layer mask only...");
        auto hitEnemy = physics_.raycast(
            {50.0f, 100.0f}, {1.0f, 0.0f}, 500.0f,
            jframe::CollisionLayers::Enemy
        );
        if (hitEnemy) {
            std::println("  HIT Enemy at distance {}", hitEnemy->distance);
        } else {
            std::println("  No enemy hit (filtered out Terrain)");
        }

        // raycastAll() - multiple hits
        std::println("\nRaycastAll from (50, 100) rightward, max 500...");
        auto hits = physics_.raycastAll({50.0f, 100.0f}, {1.0f, 0.0f}, 500.0f);
        std::println("  Found {} hits:", hits.size());
        for (std::size_t i = 0; i < hits.size(); ++i) {
            const auto& h = hits[i];
            std::println("    Hit {}: Entity {} at ({}, {}) - distance {}",
                         i + 1, entityId(h.entity), h.point.x, h.point.y, h.distance);
        }

        // Diagonal raycast
        std::println("\nDiagonal raycast from (50, 50) toward (300, 150)...");
        jframe::Vec2 dir = {250.0f, 100.0f};  // Direction vector
        float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        dir.x /= len;  // Normalize
        dir.y /= len;

        auto diagHit = physics_.raycast({50.0f, 50.0f}, dir, 400.0f);
        if (diagHit) {
            std::println("  HIT at ({}, {}) - distance {}",
                         diagHit->point.x, diagHit->point.y, diagHit->distance);
        } else {
            std::println("  No hit");
        }

        physics_.destroyBody(wall1);
        physics_.destroyBody(wall2);
        physics_.destroyBody(wall3);
    }

    void demoCollisionCallbacks() {
        printSection("Collision Callbacks");

        std::println("Setting up collision callback...");

        int collisionCount = 0;
        physics_.setCollisionCallback([&](const jframe::CollisionEvent& event) {
            ++collisionCount;
            std::println("  COLLISION #{}", collisionCount);
            std::println("    Entity A: {}", entityId(event.entityA));
            std::println("    Entity B: {}", entityId(event.entityB));
            std::println("    Contact point: ({}, {})",
                         event.contactPoint.x, event.contactPoint.y);
            std::println("    Normal: ({}, {})", event.normal.x, event.normal.y);
            std::println("    Impulse: {}", event.impulse);
        });

        // Create two bodies that will collide
        std::println("\nCreating colliding bodies...");

        auto floor = createMockEntity();
        jframe::PhysicsBodyDef floorDef{
            .type = jframe::BodyType::Static,
            .transform = {.x = 200.0f, .y = 400.0f},
            .size = {200.0f, 20.0f}
        };
        physics_.createBody(floor, floorDef);
        std::println("  Floor created at (200, 400)");

        auto ball = createMockEntity();
        jframe::PhysicsBodyDef ballDef{
            .type = jframe::BodyType::Dynamic,
            .transform = {.x = 200.0f, .y = 100.0f},
            .size = {20.0f, 20.0f},
            .restitution = 0.8f  // Bouncy
        };
        physics_.createBody(ball, ballDef);
        std::println("  Ball created at (200, 100)");

        // Simulate to trigger collision
        std::println("\nSimulating physics (ball falls and hits floor)...");
        for (int i = 0; i < 60; ++i) {  // ~1 second at 60fps
            physics_.update(0.016f);
        }

        std::println("\nTotal collisions detected: {}", collisionCount);

        // Clear callback
        physics_.setCollisionCallback(nullptr);
        std::println("Collision callback cleared");

        physics_.destroyBody(floor);
        physics_.destroyBody(ball);
    }

    void demoGroundDetection() {
        printSection("Ground Detection");

        // Create ground
        auto ground = createMockEntity();
        jframe::PhysicsBodyDef groundDef{
            .type = jframe::BodyType::Static,
            .transform = {.x = 200.0f, .y = 400.0f},
            .size = {400.0f, 20.0f}
        };
        physics_.createBody(ground, groundDef);
        physics_.setCollisionLayer(ground, jframe::CollisionLayers::Ground);
        std::println("Ground platform created at (200, 400)");

        // Create sloped ground
        auto slope = createMockEntity();
        jframe::PhysicsBodyDef slopeDef{
            .type = jframe::BodyType::Static,
            .transform = {.x = 500.0f, .y = 380.0f, .rotation = 0.5f},  // ~30 degrees
            .size = {100.0f, 20.0f}
        };
        physics_.createBody(slope, slopeDef);
        physics_.setCollisionLayer(slope, jframe::CollisionLayers::Ground);
        std::println("Sloped platform created at (500, 380)");

        // Create player above ground
        auto player = createMockEntity();
        jframe::PhysicsBodyDef playerDef{
            .type = jframe::BodyType::Dynamic,
            .transform = {.x = 200.0f, .y = 300.0f},
            .size = {32.0f, 48.0f},
            .fixedRotation = true
        };
        physics_.createBody(player, playerDef);
        physics_.setCollisionLayer(player, jframe::CollisionLayers::Player);
        std::println("Player created at (200, 300)");

        // Test ground check while in air
        std::println("\nGround check while in air:");
        jframe::GroundCheckParams params{
            .rayDistance = 5.0f,
            .slopeToleranceDeg = 60.0f,
            .groundMask = jframe::CollisionLayers::Ground
        };
        auto result1 = physics_.checkGrounded(player, params);
        std::println("  Grounded: {}", result1.grounded);
        std::println("  Distance to ground: ~90 pixels");

        // Simulate falling
        std::println("\nSimulating fall...");
        for (int i = 0; i < 100; ++i) {
            physics_.update(0.016f);
        }

        // Test ground check while on ground
        std::println("\nGround check after landing:");
        auto result2 = physics_.checkGrounded(player, params);
        std::println("  Grounded: {}", result2.grounded);
        if (result2.grounded) {
            std::println("  Ground entity: {}", entityId(result2.groundEntity));
            std::println("  Contact point: ({}, {})",
                         result2.contactPoint.x, result2.contactPoint.y);
            std::println("  Surface normal: ({}, {})",
                         result2.surfaceNormal.x, result2.surfaceNormal.y);
            std::println("  Slope angle: {} degrees", result2.slopeAngle);
        }

        // Test with different ray distance
        std::println("\nGround check with larger ray distance (20 pixels)...");
        params.rayDistance = 20.0f;
        auto result3 = physics_.checkGrounded(player, params);
        std::println("  Grounded: {}", result3.grounded);

        // Test slope detection
        std::println("\nMoving player to slope...");
        physics_.setPosition(player, {500.0f, 300.0f});

        for (int i = 0; i < 100; ++i) {
            physics_.update(0.016f);
        }

        std::println("\nGround check on slope:");
        auto result4 = physics_.checkGrounded(player, params);
        std::println("  Grounded: {}", result4.grounded);
        if (result4.grounded) {
            std::println("  Slope angle: {} degrees", result4.slopeAngle);
            std::println("  Within tolerance: {}",
                         result4.slopeAngle <= params.slopeToleranceDeg);
        }

        // Test with strict slope tolerance
        std::println("\nGround check with strict slope tolerance (20 degrees)...");
        params.slopeToleranceDeg = 20.0f;
        auto result5 = physics_.checkGrounded(player, params);
        std::println("  Grounded (strict): {}", result5.grounded);
        if (result5.grounded) {
            std::println("  Slope angle: {} degrees", result5.slopeAngle);
        }

        physics_.destroyBody(ground);
        physics_.destroyBody(slope);
        physics_.destroyBody(player);
    }

    void demoComplexSimulation() {
        printSection("Complex Simulation: Physics Sandbox");

        std::println("Creating physics sandbox with multiple objects...\n");

        // Create ground
        auto ground = createMockEntity();
        jframe::PhysicsBodyDef groundDef{
            .type = jframe::BodyType::Static,
            .transform = {.x = 300.0f, .y = 500.0f},
            .size = {600.0f, 40.0f}
        };
        physics_.createBody(ground, groundDef);
        physics_.setCollisionLayer(ground, jframe::CollisionLayers::Ground);
        std::println("Ground created");

        // Create platforms
        auto platform1 = createMockEntity();
        jframe::PhysicsBodyDef platDef{
            .type = jframe::BodyType::Static,
            .transform = {.x = 150.0f, .y = 350.0f},
            .size = {100.0f, 20.0f}
        };
        physics_.createBody(platform1, platDef);
        physics_.setCollisionLayer(platform1, jframe::CollisionLayers::Terrain);
        std::println("Platform 1 created at (150, 350)");

        auto platform2 = createMockEntity();
        platDef.transform = {.x = 450.0f, .y = 350.0f};
        physics_.createBody(platform2, platDef);
        physics_.setCollisionLayer(platform2, jframe::CollisionLayers::Terrain);
        std::println("Platform 2 created at (450, 350)");

        // Create dynamic boxes
        std::vector<jframe::Entity> boxes;
        std::println("\nCreating stack of boxes...");
        for (int i = 0; i < 5; ++i) {
            auto box = createMockEntity();
            jframe::PhysicsBodyDef boxDef{
                .type = jframe::BodyType::Dynamic,
                .transform = {.x = 300.0f, .y = 100.0f + i * 35.0f},
                .size = {30.0f, 30.0f},
                .density = 1.0f,
                .friction = 0.5f,
                .restitution = 0.1f
            };
            physics_.createBody(box, boxDef);
            physics_.setCollisionLayer(box, jframe::CollisionLayers::Enemy);
            boxes.push_back(box);
        }
        std::println("  Created {} boxes", boxes.size());

        // Create projectile
        auto projectile = createMockEntity();
        jframe::PhysicsBodyDef projDef{
            .type = jframe::BodyType::Dynamic,
            .transform = {.x = 50.0f, .y = 250.0f},
            .size = {15.0f, 15.0f},
            .density = 2.0f,
            .restitution = 0.6f
        };
        physics_.createBody(projectile, projDef);
        physics_.setCollisionLayer(projectile, jframe::CollisionLayers::Projectile);
        physics_.setVelocity(projectile, {300.0f, -50.0f});
        std::println("\nProjectile launched at (50, 250) with velocity (300, -50)");

        // Set up collision tracking
        int collisionCount = 0;
        physics_.setCollisionCallback([&](const jframe::CollisionEvent&) {
            ++collisionCount;
        });

        // Simulate
        std::println("\nSimulating physics for 3 seconds...");
        int steps = 0;
        for (int frame = 0; frame < 180; ++frame) {  // 3 seconds at 60fps
            physics_.update(0.016f);
            ++steps;

            // Print status every 60 frames (1 second)
            if ((frame + 1) % 60 == 0) {
                std::println("\n  After {} second(s):", (frame + 1) / 60);

                // Check box positions
                int fallenBoxes = 0;
                for (auto box : boxes) {
                    auto pos = physics_.getPosition(box);
                    if (pos.y > 450.0f) {  // Near ground
                        ++fallenBoxes;
                    }
                }
                std::println("    Boxes on ground: {}/{}", fallenBoxes, boxes.size());

                // Check projectile
                auto projPos = physics_.getPosition(projectile);
                auto projVel = physics_.getVelocity(projectile);
                std::println("    Projectile at ({:.1f}, {:.1f}) vel ({:.1f}, {:.1f})",
                             projPos.x, projPos.y, projVel.x, projVel.y);

                std::println("    Total collisions so far: {}", collisionCount);
            }
        }

        std::println("\nFinal simulation statistics:");
        std::println("  Total physics steps: {}", steps);
        std::println("  Total collisions: {}", collisionCount);
        std::println("  Average collisions per second: {}",
                     collisionCount / 3.0f);

        // Test spatial query on final state
        std::println("\nQuerying entities in bottom half of scene...");
        auto bottomEntities = physics_.queryAABB({0.0f, 350.0f}, {600.0f, 550.0f});
        std::println("  Found {} entities in bottom region", bottomEntities.size());

        // Test raycast through the scene
        std::println("\nRaycasting from top to bottom...");
        auto verticalHits = physics_.raycastAll({300.0f, 0.0f}, {0.0f, 1.0f}, 600.0f);
        std::println("  Ray passed through {} objects", verticalHits.size());

        // Cleanup
        physics_.setCollisionCallback(nullptr);
        physics_.destroyBody(ground);
        physics_.destroyBody(platform1);
        physics_.destroyBody(platform2);
        for (auto box : boxes) {
            physics_.destroyBody(box);
        }
        physics_.destroyBody(projectile);

        std::println("\nSimulation complete!");
    }

    //==========================================================================
    // Helper Functions
    //==========================================================================

    jframe::Entity createMockEntity() {
        // In a real demo with entity system, you'd use entities_.createEntity()
        // For this standalone demo, we'll create mock entity IDs
        auto entity = static_cast<jframe::Entity>(nextEntityId_++);
        trackedEntities_.push_back(entity);
        return entity;
    }

    std::uint32_t entityId(jframe::Entity entity) const {
        return static_cast<std::uint32_t>(entity);
    }

    std::string bodyTypeToString(jframe::BodyType type) const {
        switch (type) {
            case jframe::BodyType::Static: return "Static";
            case jframe::BodyType::Kinematic: return "Kinematic";
            case jframe::BodyType::Dynamic: return "Dynamic";
            default: return "Unknown";
        }
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
