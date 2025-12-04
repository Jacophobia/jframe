// CameraDemo.cppm
// Comprehensive demonstration of the Bestow Camera System API

module;

// Use compatibility header for MSVC C++23 module support
#include <bestow/entt_compat.hpp>

export module camera.demo;

import std;
import bestow.types;
import bestow.camera;
import bestow.entity;

export namespace demo {

//==========================================================================
// CameraDemo Class
//==========================================================================

class CameraDemo {
public:
    explicit CameraDemo(bestow::ICameraSystem& cameraSystem, bestow::IEntitySystem& entitySystem)
        : camera_(cameraSystem), entities_(entitySystem) {}

    void run() {
        printHeader("BESTOW CAMERA SYSTEM COMPREHENSIVE DEMO");

        demoBasicCameraState();
        demoTargetFollowing();
        demoFollowSmoothing();
        demoOffsetAndLookAhead();
        demoDeadzone();
        demoWorldBounds();
        demoCameraShake();
        demoZoom();
        demoCoordinateConversion();
        demoUpdateLoop();
        demoComplexScenario();

        printHeader("DEMO COMPLETE");
    }

private:
    bestow::ICameraSystem& camera_;
    bestow::IEntitySystem& entities_;

    //==========================================================================
    // Demo Sections
    //==========================================================================

    void demoBasicCameraState() {
        printSection("Basic Camera State");

        // getCamera() - Get current camera state
        std::println("Querying initial camera state...");
        auto cam = camera_.getCamera();
        std::println("  Position: ({}, {})", cam.transform.x, cam.transform.y);
        std::println("  Rotation: {}", cam.transform.rotation);
        std::println("  Zoom: {}", cam.zoom);
        std::println("  Viewport: {}x{}", cam.viewportSize.width, cam.viewportSize.height);

        // getPosition() - Get camera position as Vec2
        std::println("\nCamera position as Vec2:");
        auto pos = camera_.getPosition();
        std::println("  Position: ({}, {})", pos.x, pos.y);

        // getZoom()
        std::println("\nInitial zoom level:");
        std::println("  Zoom: {}", camera_.getZoom());
    }

    void demoTargetFollowing() {
        printSection("Target Following");

        // Create a target entity
        std::println("Creating target entity...");
        auto target = entities_.createEntity();
        entities_.emplace<bestow::Transform2D>(target, 100.0f, 200.0f);
        std::println("  Target entity created with ID: {}", static_cast<std::uint32_t>(target));

        // setTarget()
        std::println("\nSetting camera target...");
        camera_.setTarget(target);
        std::println("  Camera target set to entity {}", static_cast<std::uint32_t>(target));

        // getTarget()
        std::println("\nVerifying camera target...");
        auto currentTarget = camera_.getTarget();
        if (currentTarget != entt::null) {
            std::println("  Current target: {}", static_cast<std::uint32_t>(currentTarget));
            std::println("  Target matches: {}", currentTarget == target);
        } else {
            std::println("  No target set (unexpected!)");
        }

        // Update camera to follow target
        std::println("\nUpdating camera to follow target position...");
        auto& targetTransform = entities_.get<bestow::Transform2D>(target);
        camera_.update(0.016f, targetTransform.position());
        auto camPos = camera_.getPosition();
        std::println("  Target at: ({}, {})", targetTransform.x, targetTransform.y);
        std::println("  Camera at: ({}, {})", camPos.x, camPos.y);

        // clearTarget()
        std::println("\nClearing camera target...");
        camera_.clearTarget();
        auto clearedTarget = camera_.getTarget();
        std::println("  Target after clear: {}",
                     clearedTarget == entt::null ? "null" : std::format("{}", static_cast<std::uint32_t>(clearedTarget)));

        // Cleanup
        entities_.destroyEntity(target);
    }

    void demoFollowSmoothing() {
        printSection("Follow Smoothing");

        // Create target
        auto target = entities_.createEntity();
        entities_.emplace<bestow::Transform2D>(target, 0.0f, 0.0f);
        camera_.setTarget(target);

        std::println("Testing different smoothing values...\n");

        // Test 1: No smoothing (instant follow)
        std::println("Test 1: No Smoothing (0.0)");
        camera_.setFollowSmoothing(0.0f);
        auto& transform = entities_.get<bestow::Transform2D>(target);
        transform.x = 100.0f;
        transform.y = 50.0f;
        camera_.update(0.016f, transform.position());
        auto pos1 = camera_.getPosition();
        std::println("  Target moved to: ({}, {})", transform.x, transform.y);
        std::println("  Camera position: ({}, {})", pos1.x, pos1.y);
        std::println("  (Should be at target instantly)");

        // Test 2: Medium smoothing
        std::println("\nTest 2: Medium Smoothing (0.5)");
        camera_.setFollowSmoothing(0.5f);
        transform.x = 300.0f;
        transform.y = 200.0f;
        // Simulate a few frames
        for (int i = 0; i < 5; ++i) {
            camera_.update(0.016f, transform.position());
        }
        auto pos2 = camera_.getPosition();
        std::println("  Target at: ({}, {})", transform.x, transform.y);
        std::println("  Camera position after 5 frames: ({}, {})", pos2.x, pos2.y);
        std::println("  (Should be smoothly approaching target)");

        // Test 3: High smoothing (slow follow)
        std::println("\nTest 3: High Smoothing (0.95)");
        camera_.setFollowSmoothing(0.95f);
        transform.x = 500.0f;
        transform.y = 400.0f;
        camera_.update(0.016f, transform.position());
        auto pos3 = camera_.getPosition();
        std::println("  Target at: ({}, {})", transform.x, transform.y);
        std::println("  Camera position after 1 frame: ({}, {})", pos3.x, pos3.y);
        std::println("  (Should lag behind target significantly)");

        // Reset smoothing
        camera_.setFollowSmoothing(0.1f);
        entities_.destroyEntity(target);
    }

    void demoOffsetAndLookAhead() {
        printSection("Camera Offset & Look-Ahead");

        // Create target
        auto target = entities_.createEntity();
        entities_.emplace<bestow::Transform2D>(target, 200.0f, 150.0f);
        camera_.setTarget(target);
        camera_.setFollowSmoothing(0.0f); // Instant for clearer demonstration

        std::println("Testing camera offset for look-ahead effect...\n");

        // No offset
        std::println("Test 1: No Offset");
        camera_.setOffset(bestow::Vec2{0.0f, 0.0f});
        auto& transform = entities_.get<bestow::Transform2D>(target);
        camera_.update(0.016f, transform.position());
        auto pos1 = camera_.getPosition();
        std::println("  Offset: (0, 0)");
        std::println("  Target: ({}, {})", transform.x, transform.y);
        std::println("  Camera: ({}, {})", pos1.x, pos1.y);

        // Right offset (look-ahead to the right)
        std::println("\nTest 2: Right Look-Ahead");
        camera_.setOffset(bestow::Vec2{50.0f, 0.0f});
        camera_.update(0.016f, transform.position());
        auto pos2 = camera_.getPosition();
        std::println("  Offset: (50, 0)");
        std::println("  Target: ({}, {})", transform.x, transform.y);
        std::println("  Camera: ({}, {})", pos2.x, pos2.y);
        std::println("  (Camera should be 50 pixels right of target)");

        // Up-right diagonal offset
        std::println("\nTest 3: Diagonal Look-Ahead");
        camera_.setOffset(bestow::Vec2{30.0f, -40.0f});
        camera_.update(0.016f, transform.position());
        auto pos3 = camera_.getPosition();
        std::println("  Offset: (30, -40)");
        std::println("  Target: ({}, {})", transform.x, transform.y);
        std::println("  Camera: ({}, {})", pos3.x, pos3.y);
        std::println("  (Camera offset by +30 right, -40 up)");

        // Reset offset
        camera_.setOffset(bestow::Vec2{0.0f, 0.0f});
        entities_.destroyEntity(target);
    }

    void demoDeadzone() {
        printSection("Camera Deadzone");

        // Create target
        auto target = entities_.createEntity();
        entities_.emplace<bestow::Transform2D>(target, 200.0f, 150.0f);
        camera_.setTarget(target);
        camera_.setFollowSmoothing(0.0f);

        // Initial camera position
        auto& transform = entities_.get<bestow::Transform2D>(target);
        camera_.update(0.016f, transform.position());
        auto initialPos = camera_.getPosition();

        std::println("Demonstrating deadzone (platformer-style camera)...\n");
        std::println("Initial state:");
        std::println("  Target: ({}, {})", transform.x, transform.y);
        std::println("  Camera: ({}, {})", initialPos.x, initialPos.y);

        // Set deadzone
        std::println("\nSetting deadzone to 50x50...");
        camera_.setDeadzone(bestow::Vec2{50.0f, 50.0f});

        // Small movement within deadzone
        std::println("\nTest 1: Small movement (within deadzone)");
        transform.x += 20.0f;
        transform.y += 15.0f;
        camera_.update(0.016f, transform.position());
        auto pos1 = camera_.getPosition();
        std::println("  Target moved to: ({}, {})", transform.x, transform.y);
        std::println("  Camera at: ({}, {})", pos1.x, pos1.y);
        std::println("  Camera moved: {}", pos1.x != initialPos.x || pos1.y != initialPos.y ? "Yes" : "No (expected)");

        // Large movement outside deadzone
        std::println("\nTest 2: Large movement (outside deadzone)");
        transform.x += 100.0f;
        camera_.update(0.016f, transform.position());
        auto pos2 = camera_.getPosition();
        std::println("  Target moved to: ({}, {})", transform.x, transform.y);
        std::println("  Camera at: ({}, {})", pos2.x, pos2.y);
        std::println("  Camera moved: {}", pos2.x != pos1.x ? "Yes (expected)" : "No");

        // Clear deadzone
        std::println("\nClearing deadzone...");
        camera_.setDeadzone(bestow::Vec2{0.0f, 0.0f});
        std::println("  Deadzone cleared (set to 0x0)");

        entities_.destroyEntity(target);
    }

    void demoWorldBounds() {
        printSection("World Bounds");

        // Create target
        auto target = entities_.createEntity();
        entities_.emplace<bestow::Transform2D>(target, 0.0f, 0.0f);
        camera_.setTarget(target);
        camera_.setFollowSmoothing(0.0f);

        std::println("Testing camera bounds to keep camera within world...\n");

        // setBounds()
        std::println("Setting world bounds: X[0, 800], Y[0, 600]");
        camera_.setBounds(0.0f, 800.0f, 0.0f, 600.0f);

        // Test 1: Target at world center (within bounds)
        std::println("\nTest 1: Target at world center");
        auto& transform = entities_.get<bestow::Transform2D>(target);
        transform.x = 400.0f;
        transform.y = 300.0f;
        camera_.update(0.016f, transform.position());
        auto pos1 = camera_.getPosition();
        std::println("  Target: ({}, {})", transform.x, transform.y);
        std::println("  Camera: ({}, {})", pos1.x, pos1.y);

        // Test 2: Target beyond left bound
        std::println("\nTest 2: Target beyond left bound");
        transform.x = -100.0f;
        transform.y = 300.0f;
        camera_.update(0.016f, transform.position());
        auto pos2 = camera_.getPosition();
        std::println("  Target: ({}, {})", transform.x, transform.y);
        std::println("  Camera: ({}, {})", pos2.x, pos2.y);
        std::println("  (Camera clamped to bounds)");

        // Test 3: Target beyond right bound
        std::println("\nTest 3: Target beyond right bound");
        transform.x = 1000.0f;
        transform.y = 300.0f;
        camera_.update(0.016f, transform.position());
        auto pos3 = camera_.getPosition();
        std::println("  Target: ({}, {})", transform.x, transform.y);
        std::println("  Camera: ({}, {})", pos3.x, pos3.y);
        std::println("  (Camera clamped to bounds)");

        // Test 4: Target beyond top bound
        std::println("\nTest 4: Target beyond top bound");
        transform.x = 400.0f;
        transform.y = -50.0f;
        camera_.update(0.016f, transform.position());
        auto pos4 = camera_.getPosition();
        std::println("  Target: ({}, {})", transform.x, transform.y);
        std::println("  Camera: ({}, {})", pos4.x, pos4.y);
        std::println("  (Camera clamped to bounds)");

        // clearBounds()
        std::println("\nClearing camera bounds...");
        camera_.clearBounds();
        transform.x = -100.0f;
        camera_.update(0.016f, transform.position());
        auto pos5 = camera_.getPosition();
        std::println("  Target: ({}, {})", transform.x, transform.y);
        std::println("  Camera: ({}, {})", pos5.x, pos5.y);
        std::println("  (Camera can now follow beyond previous bounds)");

        entities_.destroyEntity(target);
    }

    void demoCameraShake() {
        printSection("Camera Shake Effects");

        std::println("Demonstrating camera shake for impact feedback...\n");

        // Get initial position
        auto initialPos = camera_.getPosition();
        std::println("Initial camera position: ({}, {})", initialPos.x, initialPos.y);

        // shake()
        std::println("\nTriggering camera shake...");
        std::println("  Intensity: 10.0");
        std::println("  Duration: 0.5 seconds");
        camera_.shake(10.0f, 0.5f);

        // Simulate shake over several frames
        std::println("\nSimulating shake effect:");
        for (int frame = 0; frame < 10; ++frame) {
            camera_.update(0.016f, initialPos);
            auto shakePos = camera_.getPosition();
            float offsetX = shakePos.x - initialPos.x;
            float offsetY = shakePos.y - initialPos.y;
            std::println("  Frame {}: offset ({:+.2f}, {:+.2f})",
                         frame, offsetX, offsetY);
        }

        // Continue until shake finishes
        std::println("\nContinuing simulation until shake ends...");
        for (int i = 0; i < 40; ++i) {
            camera_.update(0.016f, initialPos);
        }
        auto finalPos = camera_.getPosition();
        std::println("  Final position: ({}, {})", finalPos.x, finalPos.y);
        std::println("  Shake finished: {}",
                     (std::abs(finalPos.x - initialPos.x) < 0.1f &&
                      std::abs(finalPos.y - initialPos.y) < 0.1f) ? "Yes" : "No");

        // stopShake()
        std::println("\nTesting manual shake stop...");
        camera_.shake(15.0f, 2.0f); // Long shake
        camera_.update(0.016f, initialPos);
        auto shakingPos = camera_.getPosition();
        std::println("  Shake active, position: ({}, {})", shakingPos.x, shakingPos.y);

        std::println("\nStopping shake immediately...");
        camera_.stopShake();
        camera_.update(0.016f, initialPos);
        auto stoppedPos = camera_.getPosition();
        std::println("  After stop, position: ({}, {})", stoppedPos.x, stoppedPos.y);
        std::println("  Shake stopped: {}",
                     (std::abs(stoppedPos.x - initialPos.x) < 0.1f &&
                      std::abs(stoppedPos.y - initialPos.y) < 0.1f) ? "Yes" : "No");
    }

    void demoZoom() {
        printSection("Camera Zoom");

        std::println("Demonstrating camera zoom functionality...\n");

        // Get initial zoom
        auto initialZoom = camera_.getZoom();
        std::println("Initial zoom: {}", initialZoom);

        // setZoom() - Zoom in
        std::println("\nZooming in (2x)...");
        camera_.setZoom(2.0f);
        auto zoom1 = camera_.getZoom();
        std::println("  Zoom level: {}", zoom1);

        // Zoom out
        std::println("\nZooming out (0.5x)...");
        camera_.setZoom(0.5f);
        auto zoom2 = camera_.getZoom();
        std::println("  Zoom level: {}", zoom2);

        // Normal zoom
        std::println("\nResetting to normal zoom (1.0x)...");
        camera_.setZoom(1.0f);
        auto zoom3 = camera_.getZoom();
        std::println("  Zoom level: {}", zoom3);

        // Verify zoom affects camera
        std::println("\nVerifying zoom affects camera state:");
        auto cam = camera_.getCamera();
        std::println("  Camera zoom property: {}", cam.zoom);
    }

    void demoCoordinateConversion() {
        printSection("Coordinate Conversion");

        std::println("Demonstrating screen-to-world and world-to-screen conversion...\n");

        // Set known camera state
        auto target = entities_.createEntity();
        entities_.emplace<bestow::Transform2D>(target, 400.0f, 300.0f);
        camera_.setTarget(target);
        camera_.setFollowSmoothing(0.0f);
        camera_.setZoom(1.0f);
        auto& transform = entities_.get<bestow::Transform2D>(target);
        camera_.update(0.016f, transform.position());

        auto camPos = camera_.getPosition();
        std::println("Camera setup:");
        std::println("  Position: ({}, {})", camPos.x, camPos.y);
        std::println("  Zoom: {}", camera_.getZoom());

        // screenToWorld()
        std::println("\nConverting screen coordinates to world coordinates:");

        bestow::Vec2 screenCenter{400.0f, 300.0f};
        auto worldCenter = camera_.screenToWorld(screenCenter);
        std::println("  Screen (400, 300) -> World ({}, {})", worldCenter.x, worldCenter.y);

        bestow::Vec2 screenTopLeft{0.0f, 0.0f};
        auto worldTopLeft = camera_.screenToWorld(screenTopLeft);
        std::println("  Screen (0, 0) -> World ({}, {})", worldTopLeft.x, worldTopLeft.y);

        bestow::Vec2 screenBottomRight{800.0f, 600.0f};
        auto worldBottomRight = camera_.screenToWorld(screenBottomRight);
        std::println("  Screen (800, 600) -> World ({}, {})", worldBottomRight.x, worldBottomRight.y);

        // worldToScreen()
        std::println("\nConverting world coordinates to screen coordinates:");

        bestow::Vec2 worldOrigin{0.0f, 0.0f};
        auto screenOrigin = camera_.worldToScreen(worldOrigin);
        std::println("  World (0, 0) -> Screen ({}, {})", screenOrigin.x, screenOrigin.y);

        bestow::Vec2 worldTarget{400.0f, 300.0f};
        auto screenTarget = camera_.worldToScreen(worldTarget);
        std::println("  World (400, 300) -> Screen ({}, {})", screenTarget.x, screenTarget.y);

        // Round-trip test
        std::println("\nRound-trip conversion test:");
        bestow::Vec2 originalScreen{123.0f, 456.0f};
        auto world = camera_.screenToWorld(originalScreen);
        auto backToScreen = camera_.worldToScreen(world);
        std::println("  Original screen: ({}, {})", originalScreen.x, originalScreen.y);
        std::println("  -> World: ({}, {})", world.x, world.y);
        std::println("  -> Back to screen: ({}, {})", backToScreen.x, backToScreen.y);
        std::println("  Match: {}",
                     (std::abs(backToScreen.x - originalScreen.x) < 0.1f &&
                      std::abs(backToScreen.y - originalScreen.y) < 0.1f) ? "Yes" : "No");

        entities_.destroyEntity(target);
    }

    void demoUpdateLoop() {
        printSection("Update Loop Simulation");

        std::println("Simulating a typical game update loop with camera...\n");

        // Create moving target
        auto target = entities_.createEntity();
        entities_.emplace<bestow::Transform2D>(target, 0.0f, 0.0f);
        camera_.setTarget(target);
        camera_.setFollowSmoothing(0.3f);

        std::println("Simulating 10 frames of a moving target:");
        std::println("  (Target moving right at 50 pixels/second)\n");

        auto& transform = entities_.get<bestow::Transform2D>(target);
        constexpr float velocity = 50.0f; // pixels per second
        constexpr float dt = 0.016f; // 60 FPS

        for (int frame = 0; frame < 10; ++frame) {
            // Update target position (gameplay logic)
            transform.x += velocity * dt;

            // Update camera (camera system)
            camera_.update(dt, transform.position());

            // Get current state
            auto camPos = camera_.getPosition();

            std::println("Frame {}: Target ({:.1f}, {:.1f}) | Camera ({:.1f}, {:.1f})",
                         frame, transform.x, transform.y, camPos.x, camPos.y);
        }

        std::println("\nCamera smoothly follows target movement");

        entities_.destroyEntity(target);
    }

    void demoComplexScenario() {
        printSection("Complex Scenario: Platformer Camera");

        std::println("Demonstrating a complete platformer camera setup...\n");

        // Create player entity
        auto player = entities_.createEntity();
        entities_.emplace<bestow::Transform2D>(player, 100.0f, 400.0f);

        std::println("Setting up platformer camera:");
        std::println("  - Smooth following");
        std::println("  - Look-ahead offset");
        std::println("  - Vertical deadzone");
        std::println("  - World bounds");
        std::println();

        // Configure camera for platformer
        camera_.setTarget(player);
        camera_.setFollowSmoothing(0.2f);
        camera_.setOffset(bestow::Vec2{80.0f, 0.0f}); // Look ahead to the right
        camera_.setDeadzone(bestow::Vec2{40.0f, 60.0f}); // Wider horizontal deadzone
        camera_.setBounds(0.0f, 2000.0f, 0.0f, 1200.0f);
        camera_.setZoom(1.0f);

        std::println("Camera configured:");
        std::println("  Smoothing: 0.2");
        std::println("  Offset: (80, 0) - look ahead right");
        std::println("  Deadzone: (40, 60) - platformer style");
        std::println("  Bounds: [0, 2000] x [0, 1200]");
        std::println();

        auto& playerTransform = entities_.get<bestow::Transform2D>(player);

        // Simulate player movement
        std::println("Simulating player movement:\n");

        // Initial update
        camera_.update(0.016f, playerTransform.position());
        std::println("Frame 0: Player at ({:.0f}, {:.0f}), Camera at ({:.1f}, {:.1f})",
                     playerTransform.x, playerTransform.y,
                     camera_.getPosition().x, camera_.getPosition().y);

        // Move right
        std::println("\nPlayer moving right...");
        for (int i = 0; i < 5; ++i) {
            playerTransform.x += 100.0f * 0.016f;
            camera_.update(0.016f, playerTransform.position());
        }
        std::println("  After 5 frames: Player at ({:.0f}, {:.0f}), Camera at ({:.1f}, {:.1f})",
                     playerTransform.x, playerTransform.y,
                     camera_.getPosition().x, camera_.getPosition().y);

        // Jump (vertical movement)
        std::println("\nPlayer jumping...");
        playerTransform.y -= 100.0f;
        camera_.update(0.016f, playerTransform.position());
        std::println("  Player at ({:.0f}, {:.0f}), Camera at ({:.1f}, {:.1f})",
                     playerTransform.x, playerTransform.y,
                     camera_.getPosition().x, camera_.getPosition().y);
        std::println("  (Small movement - within deadzone)");

        // Land
        std::println("\nPlayer landing...");
        playerTransform.y += 100.0f;
        camera_.update(0.016f, playerTransform.position());
        std::println("  Player at ({:.0f}, {:.0f}), Camera at ({:.1f}, {:.1f})",
                     playerTransform.x, playerTransform.y,
                     camera_.getPosition().x, camera_.getPosition().y);

        // Trigger screen shake (e.g., from explosion)
        std::println("\nExplosion causes camera shake!");
        camera_.shake(8.0f, 0.3f);
        for (int i = 0; i < 5; ++i) {
            camera_.update(0.016f, playerTransform.position());
            auto shakePos = camera_.getPosition();
            std::println("  Shake frame {}: Camera at ({:.1f}, {:.1f})",
                         i, shakePos.x, shakePos.y);
        }

        // Move to world boundary
        std::println("\nPlayer moving to world edge...");
        playerTransform.x = 1950.0f; // Near right boundary
        for (int i = 0; i < 10; ++i) {
            camera_.update(0.016f, playerTransform.position());
        }
        auto boundedPos = camera_.getPosition();
        std::println("  Player at ({:.0f}, {:.0f}), Camera at ({:.1f}, {:.1f})",
                     playerTransform.x, playerTransform.y, boundedPos.x, boundedPos.y);
        std::println("  (Camera clamped by world bounds)");

        // Coordinate conversion example
        std::println("\nCoordinate conversion for UI cursor:");
        bestow::Vec2 cursorScreen{400.0f, 300.0f};
        auto cursorWorld = camera_.screenToWorld(cursorScreen);
        std::println("  Cursor at screen ({}, {}) -> world ({:.1f}, {:.1f})",
                     cursorScreen.x, cursorScreen.y, cursorWorld.x, cursorWorld.y);

        std::println("\nPlatformer camera scenario complete!");

        entities_.destroyEntity(player);
    }

    //==========================================================================
    // Helper Functions
    //==========================================================================

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
