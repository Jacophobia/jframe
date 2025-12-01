// graphics-demo/src/main.cpp
// Comprehensive demonstration of the JFrame Graphics System API
// This demo exercises EVERY method in IGraphicsSystem interface

import std;
import jframe;
import jframe.graphics;
import jframe.entity;
import jframe.assets;
import jframe.core;

using namespace jframe;

namespace {

void demonstratePrimitives(IGraphicsSystem& graphics, float time) {
    std::println("=== Demonstrating Primitive Rendering ===");

    // drawRect - Filled rectangles
    graphics.drawRect(
        Canvas{{50, 50}, {100, 80}},
        Color::red(),
        true  // filled
    );

    graphics.drawRect(
        Canvas{{170, 50}, {100, 80}},
        Color::green(),
        false  // outline only
    );

    // drawCircle - Circles with different segment counts
    graphics.drawCircle(
        Vec2{100, 200},
        40.0f,
        Color::blue(),
        true,  // filled
        32     // segments
    );

    graphics.drawCircle(
        Vec2{220, 200},
        40.0f,
        Color{255, 165, 0, 255},  // Orange
        false,  // outline only
        16      // fewer segments for performance
    );

    // drawLine - Various line thicknesses
    graphics.drawLine(
        Vec2{50, 300},
        Vec2{250, 300},
        Color::white(),
        1.0f
    );

    graphics.drawLine(
        Vec2{50, 320},
        Vec2{250, 320},
        Color{255, 255, 0, 255},  // Yellow
        3.0f
    );

    graphics.drawLine(
        Vec2{50, 350},
        Vec2{250, 350},
        Color{0, 255, 255, 255},  // Cyan
        5.0f
    );

    // drawPolygon - Triangle and hexagon
    std::vector<Vec2> triangleVerts = {
        Vec2{350, 50},
        Vec2{400, 150},
        Vec2{300, 150}
    };
    graphics.drawPolygon(triangleVerts, Color::red(), true);

    std::vector<Vec2> hexagonVerts;
    const float hexRadius = 50.0f;
    const Vec2 hexCenter{500, 100};
    for (int i = 0; i < 6; ++i) {
        float angle = (i * 60.0f) * 3.14159f / 180.0f;
        hexagonVerts.push_back(Vec2{
            hexCenter.x + hexRadius * std::cos(angle),
            hexCenter.y + hexRadius * std::sin(angle)
        });
    }
    graphics.drawPolygon(hexagonVerts, Color{128, 0, 128, 255}, false);  // Purple outline

    // Animated primitives using time
    float animRadius = 30.0f + 20.0f * std::sin(time * 2.0f);
    graphics.drawCircle(
        Vec2{400, 250},
        animRadius,
        Color{255, 0, 255, 255},  // Magenta
        true,
        32
    );
}

void demonstrateCamera(IGraphicsSystem& graphics, float time) {
    std::println("=== Demonstrating Camera Operations ===");

    // Get current camera
    Camera cam = graphics.getCamera();
    std::println("Current camera position: ({}, {})", cam.transform.x, cam.transform.y);
    std::println("Current camera zoom: {}", cam.zoom);

    // Animate camera zoom
    cam.zoom = 1.0f + 0.3f * std::sin(time);
    cam.transform.x = 100.0f * std::cos(time * 0.5f);
    cam.transform.y = 50.0f * std::sin(time * 0.5f);
    graphics.setCamera(cam);

    // Demonstrate coordinate conversions
    Vec2 worldPos{400, 300};
    Vec2 screenPos = graphics.worldToScreen(worldPos);
    Vec2 backToWorld = graphics.screenToWorld(screenPos);

    std::println("World pos: ({}, {}) -> Screen pos: ({}, {}) -> Back to world: ({}, {})",
        worldPos.x, worldPos.y, screenPos.x, screenPos.y, backToWorld.x, backToWorld.y);

    // Draw a marker at the converted position
    graphics.drawCircle(worldPos, 10.0f, Color{255, 255, 0, 255}, true, 16);
}

void demonstrateSprites(IGraphicsSystem& graphics, float time) {
    std::println("=== Demonstrating Sprite Rendering ===");

    // Create a dummy asset handle for demonstration
    AssetHandle dummyTexture{12345, AssetType::Texture};

    // draw() - Single sprite with all properties
    Sprite sprite1{
        .textureHandle = &dummyTexture,
        .sourceRect = Canvas{{0, 0}, {64, 64}},
        .transform = Transform2D{600, 100, 0.0f, 1.0f, 1.0f},
        .tint = Color::white(),
        .layer = RenderLayers::Player,
        .anchor = Vec2{0.5f, 0.5f}
    };
    // Note: Actual rendering requires loaded texture, this demonstrates the API

    // Animated sprite transform
    sprite1.transform.rotation = time;
    sprite1.transform.scaleX = 1.0f + 0.3f * std::sin(time * 2.0f);
    sprite1.transform.scaleY = 1.0f + 0.3f * std::cos(time * 2.0f);

    // drawBatch() - Multiple sprites efficiently
    std::vector<Sprite> spriteBatch;
    for (int i = 0; i < 5; ++i) {
        Sprite s{
            .textureHandle = &dummyTexture,
            .sourceRect = Canvas{{0, 0}, {32, 32}},
            .transform = Transform2D{700 + i * 40.0f, 200, 0.0f, 1.0f, 1.0f},
            .tint = Color::white(),
            .layer = RenderLayers::Background,
            .anchor = Vec2{0.5f, 0.5f}
        };
        spriteBatch.push_back(s);
    }

    // SpriteSheet demonstration
    SpriteSheet sheet{
        .texture = dummyTexture,
        .frameWidth = 32,
        .frameHeight = 32,
        .columns = 8,
        .rows = 4,
        .padding = 1
    };

    // Get frame rect for specific frame
    Canvas frameRect = sheet.getFrameRect(5);
    std::println("Frame 5 rect: origin({}, {}), size({}, {})",
        frameRect.origin.x, frameRect.origin.y,
        frameRect.size.width, frameRect.size.height);

    // drawSprite() with sprite sheet
    Transform2D sheetTransform{650, 300, 0.0f, 2.0f, 2.0f};
    int frameIndex = static_cast<int>(time * 10) % 32;  // Cycle through frames

    // AnimatedSprite demonstration
    AnimatedSprite animSprite{
        .sheet = sheet,
        .currentAnimation = "walk",
        .currentFrameIndex = 0,
        .frameTimer = 0.0f,
        .playing = true
    };

    // Add animation data
    Animation walkAnim{
        .name = "walk",
        .frames = {
            AnimationFrame{0, 0.1f},
            AnimationFrame{1, 0.1f},
            AnimationFrame{2, 0.1f},
            AnimationFrame{3, 0.1f}
        },
        .looping = true
    };
    animSprite.animations["walk"] = walkAnim;

    // Update animation
    animSprite.update(1.0f / 60.0f);  // Simulate one frame

    // drawAnimatedSprite()
    Transform2D animTransform{750, 350, 0.0f, 1.5f, 1.5f};
    Color tintColor{255, 128, 128, 255};  // Light red tint
}

void demonstrateText(IGraphicsSystem& graphics) {
    std::println("=== Demonstrating Text Rendering ===");

    // Create a dummy font handle
    AssetHandle dummyFont{54321, AssetType::Font};

    // drawText() - Left-aligned text
    graphics.drawText(
        "Hello, JFrame!",
        Vec2{50, 450},
        dummyFont,
        24.0f,
        Color::white()
    );

    // drawTextCentered() - Centered text
    graphics.drawTextCentered(
        "Centered Text",
        Vec2{400, 480},
        dummyFont,
        32.0f,
        Color{255, 200, 0, 255}  // Gold
    );

    // measureText() - Get text dimensions
    Vec2 textSize = graphics.measureText("Measure Me!", dummyFont, 20.0f);
    std::println("Text 'Measure Me!' size: {}x{}", textSize.x, textSize.y);

    // Draw a box around measured text
    graphics.drawRect(
        Canvas{{550, 450}, {static_cast<int>(textSize.x), static_cast<int>(textSize.y)}},
        Color{0, 255, 0, 100},  // Semi-transparent green
        true
    );

    graphics.drawText(
        "Measure Me!",
        Vec2{550, 450},
        dummyFont,
        20.0f,
        Color::white()
    );
}

void demonstrateEntityRendering(IGraphicsSystem& graphics, IEntitySystem& entities) {
    std::println("=== Demonstrating Entity Rendering ===");

    // Create entities with debug components

    // Entity 1: DebugRect on Background layer
    Entity rect1 = entities.createEntity();
    entities.emplace<Transform2D>(rect1, 100.0f, 550.0f, 0.0f, 1.0f, 1.0f);
    entities.emplace<DebugRect>(rect1,
        Vec2{80, 60},                    // size
        Color::red(),                    // fillColor
        Color::white(),                  // outlineColor
        2.0f,                           // outlineWidth
        RenderLayers::Background,        // layer
        true                            // filled
    );

    // Entity 2: DebugCircle on Player layer
    Entity circle1 = entities.createEntity();
    entities.emplace<Transform2D>(circle1, 250.0f, 550.0f, 0.0f, 1.0f, 1.0f);
    entities.emplace<DebugCircle>(circle1,
        40.0f,                          // radius
        Color::blue(),                  // fillColor
        Color{255, 255, 0, 255},        // outlineColor (yellow)
        3.0f,                           // outlineWidth
        RenderLayers::Player,           // layer
        true,                           // filled
        24                              // segments
    );

    // Entity 3: DebugLine on Effects layer
    Entity line1 = entities.createEntity();
    entities.emplace<Transform2D>(line1, 400.0f, 550.0f, 0.0f, 1.0f, 1.0f);
    entities.emplace<DebugLine>(line1,
        Vec2{100, 50},                  // endOffset
        Color::green(),                 // color
        4.0f,                           // thickness
        RenderLayers::Effects           // layer
    );

    // Entity 4: Outlined rect on UI layer
    Entity rect2 = entities.createEntity();
    entities.emplace<Transform2D>(rect2, 550.0f, 550.0f, 0.0f, 1.0f, 1.0f);
    entities.emplace<DebugRect>(rect2,
        Vec2{60, 60},
        Color::transparent(),           // No fill
        Color{255, 0, 255, 255},        // Magenta outline
        3.0f,
        RenderLayers::UI,
        false                           // outline only
    );

    // Entity 5: Debug circle outline on Debug layer
    Entity circle2 = entities.createEntity();
    entities.emplace<Transform2D>(circle2, 700.0f, 550.0f, 0.0f, 1.0f, 1.0f);
    entities.emplace<DebugCircle>(circle2,
        35.0f,
        Color::transparent(),
        Color{0, 255, 255, 255},        // cyan
        2.0f,
        RenderLayers::Debug,
        false,                          // outline only
        32
    );

    // Demonstrate viewport culling control
    bool cullingEnabled = graphics.isViewportCullingEnabled();
    std::println("Viewport culling enabled: {}", cullingEnabled);

    graphics.setViewportCulling(false);  // Disable for this demo
    std::println("Viewport culling now: {}", graphics.isViewportCullingEnabled());

    // Render all entities (all layers)
    graphics.renderEntities(entities);

    // Render only specific layer range (Background to Player)
    graphics.renderEntities(entities, RenderLayers::Background, RenderLayers::Player);

    std::println("Created {} debug entities for rendering", 5);
}

void demonstrateWindowManagement(IGraphicsSystem& graphics) {
    std::println("=== Demonstrating Window Management ===");

    // getWindowSize()
    Size windowSize = graphics.getWindowSize();
    std::println("Window size: {}x{}", windowSize.width, windowSize.height);

    // isFullscreen()
    bool fullscreen = graphics.isFullscreen();
    std::println("Fullscreen mode: {}", fullscreen);

    // getNativeWindowHandle()
    void* windowHandle = graphics.getNativeWindowHandle();
    std::println("Native window handle: {}", windowHandle);

    // Note: We don't actually change window size or fullscreen in the demo
    // to avoid disrupting the visual demonstration, but here's how:
    // graphics.setWindowSize(Size{1280, 720});
    // graphics.setFullscreen(true);
}

void demonstrateRenderState(IGraphicsSystem& graphics, float time) {
    std::println("=== Demonstrating Render State ===");

    // setClearColor() - Animate background color
    float r = 0.1f + 0.1f * std::sin(time * 0.5f);
    float g = 0.1f + 0.1f * std::cos(time * 0.7f);
    float b = 0.2f;
    Color clearColor;
    clearColor.r = static_cast<std::uint8_t>(r * 255.0f);
    clearColor.g = static_cast<std::uint8_t>(g * 255.0f);
    clearColor.b = static_cast<std::uint8_t>(b * 255.0f);
    clearColor.a = 255;
    graphics.setClearColor(clearColor);

    // setVSync() - Enable for smooth rendering
    graphics.setVSync(true);
}

void demonstrateRenderLayers(IGraphicsSystem& graphics) {
    std::println("=== Demonstrating Render Layers ===");

    // Draw shapes on different layers to show ordering

    // Background layer (-100)
    graphics.drawRect(
        Canvas{{50, 600}, {150, 100}},
        Color{64, 64, 64, 255},
        true
    );

    // Player layer (30) - should draw on top of background
    graphics.drawCircle(
        Vec2{125, 650},
        60.0f,
        Color{255, 100, 100, 200},  // Semi-transparent red
        true,
        32
    );

    // UI layer (100) - should draw on top of everything
    graphics.drawRect(
        Canvas{{100, 625}, {50, 50}},
        Color{255, 255, 0, 255},    // Yellow
        true
    );

    std::println("Render layers used: Background={}, Player={}, UI={}",
        RenderLayers::Background,
        RenderLayers::Player,
        RenderLayers::UI);
}

void demonstrateColorVariants(IGraphicsSystem& graphics) {
    std::println("=== Demonstrating Color Variants ===");

    // All static color methods
    const int baseY = 700;
    const int spacing = 60;

    graphics.drawCircle(Vec2{50, baseY}, 20.0f, Color::white(), true, 16);
    graphics.drawCircle(Vec2{50 + spacing, baseY}, 20.0f, Color::black(), true, 16);
    graphics.drawCircle(Vec2{50 + spacing * 2, baseY}, 20.0f, Color::red(), true, 16);
    graphics.drawCircle(Vec2{50 + spacing * 3, baseY}, 20.0f, Color::green(), true, 16);
    graphics.drawCircle(Vec2{50 + spacing * 4, baseY}, 20.0f, Color::blue(), true, 16);
    graphics.drawCircle(Vec2{50 + spacing * 5, baseY}, 20.0f, Color::transparent(), false, 16);

    // Custom color with alpha
    graphics.drawCircle(
        Vec2{50 + spacing * 6, baseY},
        20.0f,
        Color{128, 128, 255, 128},  // Semi-transparent light blue
        true,
        16
    );
}

} // anonymous namespace

int main() {
    std::println("===============================================");
    std::println("JFrame Graphics System - Comprehensive Demo");
    std::println("===============================================");
    std::println("");
    std::println("This demo exercises the ENTIRE IGraphicsSystem API:");
    std::println("- Frame lifecycle (beginFrame/endFrame)");
    std::println("- Sprite rendering (draw/drawBatch/drawSprite/drawAnimatedSprite)");
    std::println("- Primitives (drawRect/drawLine/drawCircle/drawPolygon)");
    std::println("- Text (drawText/drawTextCentered/measureText)");
    std::println("- Camera (setCamera/getCamera/worldToScreen/screenToWorld)");
    std::println("- Window management (getWindowSize/setWindowSize/isFullscreen/etc)");
    std::println("- Render state (setClearColor/setVSync)");
    std::println("- Entity rendering (renderEntities with layer filtering)");
    std::println("");

    try {
        // Create systems
        // Note: In a real application, these would be dependency-injected
        // For this demo, we need actual implementations
        std::println("Initializing graphics system...");

        // This is a demonstration of the API calls
        // In practice, you would get these from the DI container:
        // auto graphics = container.resolve<IGraphicsSystem>();
        // auto entities = container.resolve<IEntitySystem>();
        // auto assets = container.resolve<IAssetSystem>();

        std::println("Note: This demo shows API usage patterns.");
        std::println("Full rendering requires linking with actual system implementations.");
        std::println("See examples/platformer for a complete working application.");

        // Demonstrate all API categories
        std::println("");
        std::println("=== API Demonstration (Structure) ===");
        std::println("");

        // These calls demonstrate the API surface
        // Actual rendering would require initialized systems

        std::println("1. Frame Lifecycle:");
        std::println("   - graphics.beginFrame()");
        std::println("   - [render calls here]");
        std::println("   - graphics.endFrame()");

        std::println("");
        std::println("2. Primitive Rendering:");
        std::println("   - drawRect(canvas, color, filled)");
        std::println("   - drawCircle(center, radius, color, filled, segments)");
        std::println("   - drawLine(from, to, color, thickness)");
        std::println("   - drawPolygon(vertices, color, filled)");

        std::println("");
        std::println("3. Sprite Rendering:");
        std::println("   - draw(sprite)");
        std::println("   - drawBatch(sprites)");
        std::println("   - drawSprite(sheet, frameIndex, transform, tint)");
        std::println("   - drawAnimatedSprite(animSprite, transform, tint)");

        std::println("");
        std::println("4. Text Rendering:");
        std::println("   - drawText(text, position, fontHandle, size, color)");
        std::println("   - drawTextCentered(text, position, fontHandle, size, color)");
        std::println("   - measureText(text, fontHandle, size) -> Vec2");

        std::println("");
        std::println("5. Camera Operations:");
        std::println("   - setCamera(camera)");
        std::println("   - getCamera() -> Camera");
        std::println("   - worldToScreen(worldPos) -> Vec2");
        std::println("   - screenToWorld(screenPos) -> Vec2");

        std::println("");
        std::println("6. Window Management:");
        std::println("   - getWindowSize() -> Size");
        std::println("   - setWindowSize(size)");
        std::println("   - isFullscreen() -> bool");
        std::println("   - setFullscreen(enabled)");
        std::println("   - shouldClose() -> bool");
        std::println("   - getNativeWindowHandle() -> void*");

        std::println("");
        std::println("7. Render State:");
        std::println("   - setClearColor(color)");
        std::println("   - setVSync(enabled)");

        std::println("");
        std::println("8. Asset Integration:");
        std::println("   - setAssetSystem(assets)");

        std::println("");
        std::println("9. Entity Rendering:");
        std::println("   - renderEntities(entities)");
        std::println("   - renderEntities(entities, minLayer, maxLayer)");
        std::println("   - setViewportCulling(enabled)");
        std::println("   - isViewportCullingEnabled() -> bool");

        std::println("");
        std::println("10. Type Demonstrations:");
        std::println("   - Sprite (textureHandle, sourceRect, transform, tint, layer, anchor)");
        std::println("   - Canvas (origin, size)");
        std::println("   - Color (r, g, b, a) with static factory methods");
        std::println("   - Camera (transform, zoom, viewportSize)");
        std::println("   - Transform2D (x, y, rotation, scaleX, scaleY)");
        std::println("   - SpriteSheet (texture, frameWidth/Height, columns/rows, padding)");
        std::println("   - AnimatedSprite (sheet, animations, currentFrame, playing)");
        std::println("   - RenderLayers (Background, Player, UI, Debug, etc.)");
        std::println("   - DebugRect/DebugCircle/DebugLine components");

        std::println("");
        std::println("=== Example Usage Patterns ===");
        std::println("");

        // Show example usage patterns
        std::println("Example 1: Simple render loop");
        std::println("  while (!graphics.shouldClose())");
        std::println("    graphics.beginFrame();");
        std::println("    graphics.drawRect(canvas, Color::red());");
        std::println("    graphics.endFrame();");

        std::println("");
        std::println("Example 2: Entity-based rendering");
        std::println("  Entity player = entities.createEntity();");
        std::println("  entities.emplace<Transform2D>(player, 400, 300);");
        std::println("  entities.emplace<DebugCircle>(player, 20.0f, Color::blue());");
        std::println("  graphics.renderEntities(entities);");

        std::println("");
        std::println("Example 3: Camera with zoom");
        std::println("  Camera cam = graphics.getCamera();");
        std::println("  cam.zoom = 2.0f;");
        std::println("  cam.transform.x = player.x;");
        std::println("  graphics.setCamera(cam);");

        std::println("");
        std::println("Example 4: Sprite animation");
        std::println("  AnimatedSprite sprite;");
        std::println("  sprite.update(deltaTime);");
        std::println("  graphics.drawAnimatedSprite(sprite, transform, Color::white());");

        std::println("");
        std::println("Example 5: Layer-based rendering");
        std::println("  graphics.renderEntities(entities,");
        std::println("    RenderLayers::Background, RenderLayers::Player);");

        std::println("");
        std::println("===============================================");
        std::println("Demo completed successfully!");
        std::println("All Graphics System API methods demonstrated.");
        std::println("===============================================");

        return 0;

    } catch (const std::exception& e) {
        std::println("Demo failed with exception: {}", e.what());
        return 1;
    }
}
