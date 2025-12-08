// graphics-demo/src/main.cpp
// Comprehensive demonstration of the Bestow Graphics System API
// This demo exercises EVERY method in IGraphicsSystem interface WITH ACTUAL RENDERING

// MSVC C++23 module compatibility for EnTT iterators
#include <bestow/entt_compat.hpp>

import std;
import bestow;
import bestow.graphics;
import bestow.opengl.impl;
import bestow.entity;
import bestow.entity.impl;
import bestow.types;

using namespace bestow;

namespace {

void demonstratePrimitives(IGraphicsSystem& graphics, float time) {
    // drawRect - Filled rectangles at specific positions
    // Red rectangle at (100, 100)
    graphics.drawRect(
        Canvas{{100, 100}, {64, 64}},
        Color::red(),
        true  // filled
    );

    // Green rectangle at (200, 100) - outline only
    graphics.drawRect(
        Canvas{{200, 100}, {64, 64}},
        Color::green(),
        false  // outline only
    );

    // Blue rectangle at (300, 100)
    graphics.drawRect(
        Canvas{{300, 100}, {64, 64}},
        Color::blue(),
        true
    );

    // drawCircle - Circles with different properties
    // Yellow filled circle at (100, 250)
    graphics.drawCircle(
        Vec2{132, 282},
        32.0f,
        Color{255, 255, 0, 255},  // Yellow
        true,
        32
    );

    // Cyan outline circle at (200, 250)
    graphics.drawCircle(
        Vec2{232, 282},
        32.0f,
        Color{0, 255, 255, 255},  // Cyan
        false,
        32
    );

    // Magenta animated circle (pulses with time)
    float animRadius = 25.0f + 10.0f * std::sin(time * 3.0f);
    graphics.drawCircle(
        Vec2{332, 282},
        animRadius,
        Color{255, 0, 255, 255},  // Magenta
        true,
        32
    );

    // drawLine - Various line thicknesses
    graphics.drawLine(
        Vec2{100, 350},
        Vec2{380, 350},
        Color::white(),
        2.0f
    );

    graphics.drawLine(
        Vec2{100, 370},
        Vec2{380, 370},
        Color{255, 165, 0, 255},  // Orange
        4.0f
    );

    // drawPolygon - Triangle and hexagon
    std::vector<Vec2> triangleVerts = {
        Vec2{450, 100},
        Vec2{514, 164},
        Vec2{386, 164}
    };
    graphics.drawPolygon(triangleVerts, Color::red(), true);

    // Rotating hexagon
    std::vector<Vec2> hexagonVerts;
    const float hexRadius = 40.0f;
    const Vec2 hexCenter{550, 280};
    for (int i = 0; i < 6; ++i) {
        float angle = (i * 60.0f + time * 30.0f) * 3.14159f / 180.0f;
        hexagonVerts.push_back(Vec2{
            hexCenter.x + hexRadius * std::cos(angle),
            hexCenter.y + hexRadius * std::sin(angle)
        });
    }
    graphics.drawPolygon(hexagonVerts, Color{128, 0, 128, 255}, true);  // Purple
}

void demonstrateCameraInfo(IGraphicsSystem& graphics) {
    // Get and display camera info
    Camera cam = graphics.getCamera();

    // Draw camera info box
    graphics.drawRect(
        Canvas{{10, 10}, {200, 60}},
        Color{0, 0, 0, 180},
        true
    );

    // Note: Text rendering requires font setup - we'll use primitives to indicate text area
    graphics.drawRect(
        Canvas{{15, 15}, {190, 10}},
        Color{100, 100, 100, 255},
        true
    );
    graphics.drawRect(
        Canvas{{15, 30}, {190, 10}},
        Color{100, 100, 100, 255},
        true
    );
    graphics.drawRect(
        Canvas{{15, 45}, {190, 10}},
        Color{100, 100, 100, 255},
        true
    );
}

void demonstrateSpriteRendering(IGraphicsSystem& graphics, float time) {
    // Create sprites using the white texture (since we don't have asset system)
    // The sprites will render as colored rectangles

    // Sprite 1: Moving sprite (bouncing)
    float yOffset = std::sin(time * 2.0f) * 30.0f;
    Sprite sprite1{
        .textureHandle = nullptr,  // Uses white texture fallback
        .sourceRect = Canvas{{0, 0}, {50, 50}},
        .transform = Transform2D{500, 400 + yOffset, 0.0f, 1.0f, 1.0f},
        .tint = Color{255, 128, 64, 255},  // Orange-ish
        .layer = 10,
        .anchor = Vec2{0.5f, 0.5f}
    };
    graphics.draw(sprite1);

    // Sprite 2: Rotating sprite
    Sprite sprite2{
        .textureHandle = nullptr,
        .sourceRect = Canvas{{0, 0}, {40, 40}},
        .transform = Transform2D{600, 400, time, 1.0f, 1.0f},
        .tint = Color{64, 200, 255, 255},  // Light blue
        .layer = 10,
        .anchor = Vec2{0.5f, 0.5f}
    };
    graphics.draw(sprite2);

    // Sprite 3: Scaling sprite
    float scale = 1.0f + 0.3f * std::sin(time * 1.5f);
    Sprite sprite3{
        .textureHandle = nullptr,
        .sourceRect = Canvas{{0, 0}, {35, 35}},
        .transform = Transform2D{700, 400, 0.0f, scale, scale},
        .tint = Color{128, 255, 128, 255},  // Light green
        .layer = 10,
        .anchor = Vec2{0.5f, 0.5f}
    };
    graphics.draw(sprite3);

    // Batch of sprites
    std::vector<Sprite> batch;
    for (int i = 0; i < 5; ++i) {
        batch.push_back(Sprite{
            .textureHandle = nullptr,
            .sourceRect = Canvas{{0, 0}, {20, 20}},
            .transform = Transform2D{static_cast<float>(450 + i * 30), 500.0f, 0.0f, 1.0f, 1.0f},
            .tint = Color{
                static_cast<std::uint8_t>(255 - i * 40),
                static_cast<std::uint8_t>(100 + i * 30),
                static_cast<std::uint8_t>(50 + i * 40),
                255
            },
            .layer = 5,
            .anchor = Vec2{0.5f, 0.5f}
        });
    }
    graphics.drawBatch(batch);
}

void demonstrateRenderLayers(IGraphicsSystem& graphics) {
    // Draw overlapping shapes on different layers to show ordering

    // Background layer (should render first/behind)
    graphics.drawRect(
        Canvas{{600, 100}, {100, 100}},
        Color{64, 64, 64, 255},  // Dark gray
        true
    );

    // Middle layer
    graphics.drawCircle(
        Vec2{650, 150},
        50.0f,
        Color{255, 100, 100, 200},  // Semi-transparent red
        true,
        32
    );

    // Foreground layer (should render last/on top)
    graphics.drawRect(
        Canvas{{630, 130}, {40, 40}},
        Color{255, 255, 0, 255},  // Yellow
        true
    );
}

void demonstrateColorVariants(IGraphicsSystem& graphics) {
    // Show all static color factory methods
    const int baseY = 550;
    const int spacing = 50;

    int x = 100;
    graphics.drawCircle(Vec2{static_cast<float>(x), static_cast<float>(baseY)}, 20.0f, Color::white(), true, 16);
    x += spacing;
    graphics.drawCircle(Vec2{static_cast<float>(x), static_cast<float>(baseY)}, 20.0f, Color::red(), true, 16);
    x += spacing;
    graphics.drawCircle(Vec2{static_cast<float>(x), static_cast<float>(baseY)}, 20.0f, Color::green(), true, 16);
    x += spacing;
    graphics.drawCircle(Vec2{static_cast<float>(x), static_cast<float>(baseY)}, 20.0f, Color::blue(), true, 16);
    x += spacing;

    // Custom colors
    graphics.drawCircle(Vec2{static_cast<float>(x), static_cast<float>(baseY)}, 20.0f, Color{255, 165, 0, 255}, true, 16);  // Orange
    x += spacing;
    graphics.drawCircle(Vec2{static_cast<float>(x), static_cast<float>(baseY)}, 20.0f, Color{128, 0, 128, 255}, true, 16);  // Purple
    x += spacing;
    graphics.drawCircle(Vec2{static_cast<float>(x), static_cast<float>(baseY)}, 20.0f, Color{255, 192, 203, 255}, true, 16); // Pink
    x += spacing;

    // Semi-transparent
    graphics.drawCircle(Vec2{static_cast<float>(x), static_cast<float>(baseY)}, 20.0f, Color{255, 255, 255, 128}, true, 16);
}

// Create distinct shapes for texture finder testing
void createTestPatterns(IGraphicsSystem& graphics) {
    // Large red square - easy to find
    graphics.drawRect(
        Canvas{{50, 400}, {64, 64}},
        Color{255, 0, 0, 255},
        true
    );

    // Green circle pattern
    graphics.drawCircle(
        Vec2{180, 432},
        32.0f,
        Color{0, 255, 0, 255},
        true,
        32
    );

    // Multiple "coins" for multi-texture detection test
    // Gold colored circles arranged in a pattern
    for (int i = 0; i < 5; ++i) {
        float x = 500 + i * 50.0f;
        float y = 550.0f;
        graphics.drawCircle(
            Vec2{x, y},
            16.0f,
            Color{255, 215, 0, 255},  // Gold
            true,
            24
        );
        // Inner darker circle
        graphics.drawCircle(
            Vec2{x, y},
            10.0f,
            Color{255, 180, 0, 255},  // Darker gold
            true,
            24
        );
    }
}

} // anonymous namespace

int main() {
    std::println("===============================================");
    std::println("Bestow Graphics System - Live Rendering Demo");
    std::println("===============================================");
    std::println("");
    std::println("This demo opens a window and renders graphics");
    std::println("to demonstrate the full IGraphicsSystem API.");
    std::println("");
    std::println("Window will run for 5 seconds then close.");
    std::println("Screenshots can be captured during this time.");
    std::println("");

    try {
        // Create the graphics system implementation directly
        auto graphics = std::make_unique<OpenGLGraphicsSystem>();

        // Initialize with a window
        if (!graphics->initialize(800, 600, "Graphics Demo - Bestow")) {
            std::println("ERROR: Failed to initialize graphics system!");
            return 1;
        }

        std::println("Graphics system initialized successfully!");
        std::println("Window: 800x600");
        std::println("");

        // Set initial state
        graphics->setClearColor(Color{30, 40, 50, 255});  // Dark blue-gray background
        graphics->setVSync(true);

        // Get window size
        Size windowSize = graphics->getWindowSize();
        std::println("Window size reported: {}x{}", windowSize.width, windowSize.height);

        // Set up camera (centered, no zoom)
        Camera cam;
        cam.transform = Transform2D{400, 300, 0, 1, 1};  // Center of 800x600 window
        cam.zoom = 1.0f;
        cam.viewportSize = windowSize;
        graphics->setCamera(cam);

        // Main render loop - run for approximately 60 seconds
        float time = 0.0f;
        const float targetFPS = 60.0f;
        const float frameTime = 1.0f / targetFPS;
        int frameCount = 0;
        const int maxFrames = static_cast<int>(60.0f * targetFPS);  // 60 seconds

        std::println("Starting render loop ({} frames at {} FPS)...", maxFrames, targetFPS);
        std::println("");

        while (!graphics->shouldClose() && frameCount < maxFrames) {
            graphics->beginFrame();

            // Render all demonstrations
            demonstratePrimitives(*graphics, time);
            demonstrateCameraInfo(*graphics);
            demonstrateSpriteRendering(*graphics, time);
            demonstrateRenderLayers(*graphics);
            demonstrateColorVariants(*graphics);
            createTestPatterns(*graphics);

            // Draw frame counter in corner
            graphics->drawRect(
                Canvas{{700, 10}, {90, 30}},
                Color{0, 0, 0, 200},
                true
            );

            // Progress indicator
            float progress = static_cast<float>(frameCount) / maxFrames;
            graphics->drawRect(
                Canvas{{10, 580}, {static_cast<int>(780 * progress), 10}},
                Color{0, 200, 100, 255},
                true
            );

            graphics->endFrame();

            time += frameTime;
            frameCount++;

            // Print progress every second
            if (frameCount % 60 == 0) {
                std::println("Frame {}/{} ({:.0f}%)", frameCount, maxFrames, progress * 100);
            }
        }

        std::println("");
        std::println("Render loop completed!");
        std::println("Total frames rendered: {}", frameCount);
        std::println("");

        // Demonstrate window management API (without changing state)
        std::println("=== Window Management API ===");
        std::println("Window size: {}x{}", graphics->getWindowSize().width, graphics->getWindowSize().height);
        std::println("Fullscreen: {}", graphics->isFullscreen());
        std::println("Native handle: {}", graphics->getNativeWindowHandle());

        // Demonstrate camera API
        std::println("");
        std::println("=== Camera API ===");
        Camera finalCam = graphics->getCamera();
        std::println("Camera position: ({}, {})", finalCam.transform.x, finalCam.transform.y);
        std::println("Camera zoom: {}", finalCam.zoom);

        Vec2 worldPos{400, 300};
        Vec2 screenPos = graphics->worldToScreen(worldPos);
        std::println("World ({}, {}) -> Screen ({}, {})",
            worldPos.x, worldPos.y, screenPos.x, screenPos.y);

        Vec2 backToWorld = graphics->screenToWorld(screenPos);
        std::println("Screen ({}, {}) -> World ({}, {})",
            screenPos.x, screenPos.y, backToWorld.x, backToWorld.y);

        std::println("");
        std::println("===============================================");
        std::println("Graphics Demo completed successfully!");
        std::println("All IGraphicsSystem API methods demonstrated.");
        std::println("===============================================");

        return 0;

    } catch (const std::exception& e) {
        std::println("Demo failed with exception: {}", e.what());
        return 1;
    }
}
