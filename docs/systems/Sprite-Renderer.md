# Bestow Sprite Renderer System

## Overview

The Sprite Renderer System provides automatic batch rendering for entities with visual components, eliminating repetitive rendering loops in game code. Instead of manually iterating entity collections and calling draw functions, games can rely on the engine to render all entities with `Sprite`, `AnimatedSprite`, or debug primitive components.

**Module:** `bestow.graphics` (extended interface)
**Implementation:** `bestow-graphics/`
**Status:** Planned Enhancement

## Problem Statement

Currently, games must write extensive rendering code:

```cpp
// Current approach - 580+ lines of rendering code in ability-demo
void Game::render(float alpha) {
    // Render platforms
    for (const auto& platform : platforms_) {
        auto* size = sys.entities->tryGet<Size2D>(platform);
        if (size && sys.physics->hasBody(platform)) {
            Vec2 pos = sys.physics->getPosition(platform);
            float halfWidth = size->width / 2.0f;
            float halfHeight = size->height / 2.0f;
            sys.graphics->drawRect(
                Canvas{{int(pos.x - halfWidth), int(pos.y - halfHeight)},
                       {int(size->width), int(size->height)}},
                platformColor, true);
        }
    }

    // Render enemies (same pattern)
    for (const auto& enemy : enemies_) {
        // ... 20 lines of position/size calculation
    }

    // Render collectables (same pattern)
    for (const auto& collectable : collectables_) {
        // ... 20 lines
    }

    // ... 10 more entity types with identical patterns
}
```

This leads to:
- 500-600 lines of nearly identical rendering code
- Manual render order management
- No automatic culling
- Duplicated camera transformation logic

## Proposed Solution

### Automatic Entity Rendering

Entities with visual components are rendered automatically:

```cpp
// New approach - automatic rendering
void Game::render(float alpha) {
    // All entities with Sprite/AnimatedSprite/DebugRect rendered automatically
    sys.graphics->renderEntities(*sys.entities);

    // Only need to render custom/procedural visuals
    renderUI();
}
```

### Visual Components

#### DebugRect (New Component)

For entities without textures (prototyping, debug visualization):

```cpp
struct DebugRect {
    Vec2 size;           // Width and height
    Color fillColor;     // Fill color (alpha for transparency)
    Color outlineColor;  // Outline color (optional)
    float outlineWidth;  // Outline thickness (0 = no outline)
    RenderLayer layer;   // Render order
};

// Usage
entities->emplace<DebugRect>(platform, DebugRect{
    .size = {200.0f, 20.0f},
    .fillColor = {128, 128, 128, 255},
    .outlineColor = {64, 64, 64, 255},
    .outlineWidth = 2.0f,
    .layer = RenderLayer::Background
});
```

#### DebugCircle (New Component)

```cpp
struct DebugCircle {
    float radius;
    Color fillColor;
    Color outlineColor;
    float outlineWidth;
    RenderLayer layer;
};
```

#### Existing Components Enhanced

```cpp
// Sprite - already exists, add auto-render support
struct Sprite {
    AssetHandle textureHandle;
    Canvas sourceRect;
    Color tint;
    RenderLayer layer;
    Vec2 anchor;        // Origin point (0.5, 0.5 = center)
    bool flipX, flipY;  // Sprite flipping
};

// AnimatedSprite - already exists
struct AnimatedSprite {
    SpriteSheet sheet;
    std::unordered_map<std::string, Animation> animations;
    std::string currentAnimation;
    // ...
};
```

### Render Layers

```cpp
enum class RenderLayer : int {
    Background = -100,
    BackgroundDecor = -50,
    Platforms = 0,
    Items = 10,
    Enemies = 20,
    Player = 30,
    Effects = 40,
    Foreground = 50,
    UI = 100
};
```

### Interface Extension

```cpp
class IGraphicsSystem {
    // ... existing methods ...

    // Automatic entity rendering
    void renderEntities(const IEntitySystem& entities);

    // Render specific layer range
    void renderEntities(const IEntitySystem& entities,
                        RenderLayer minLayer, RenderLayer maxLayer);

    // Enable/disable auto-culling
    void setViewportCulling(bool enabled);

    // Debug visualization
    void setDebugRenderEnabled(bool enabled);
    void renderDebugInfo(const IEntitySystem& entities);
};
```

## Usage Examples

### Basic Setup

```cpp
bool Game::initialize(Engine& engine) {
    auto& sys = engine.systems();

    // Create player with sprite
    player_ = sys.entities->createEntity();
    sys.entities->emplace<Transform2D>(player_, Transform2D{.x = 100, .y = 200});
    sys.entities->emplace<Sprite>(player_, Sprite{
        .textureHandle = playerTexture_,
        .layer = RenderLayer::Player
    });

    // Create platform with debug rect (no texture needed)
    Entity platform = sys.entities->createEntity();
    sys.entities->emplace<Transform2D>(platform, Transform2D{.x = 400, .y = 500});
    sys.entities->emplace<DebugRect>(platform, DebugRect{
        .size = {200.0f, 20.0f},
        .fillColor = {100, 100, 100, 255},
        .layer = RenderLayer::Platforms
    });

    return true;
}

void Game::render(float alpha) {
    auto& sys = engine_->systems();

    // Render all entities automatically (sorted by layer)
    sys.graphics->renderEntities(*sys.entities);

    // Render UI on top
    renderUI();
}
```

### Layer-Based Rendering

```cpp
void Game::render(float alpha) {
    auto& sys = engine_->systems();

    // Render background layers
    sys.graphics->renderEntities(*sys.entities,
        RenderLayer::Background, RenderLayer::BackgroundDecor);

    // Render custom parallax effect
    renderParallax();

    // Render gameplay layers
    sys.graphics->renderEntities(*sys.entities,
        RenderLayer::Platforms, RenderLayer::Effects);

    // Render foreground and UI
    sys.graphics->renderEntities(*sys.entities,
        RenderLayer::Foreground, RenderLayer::UI);
}
```

### Debug Visualization

```cpp
void Game::render(float alpha) {
    auto& sys = engine_->systems();

    sys.graphics->renderEntities(*sys.entities);

    #if defined(BESTOW_DEV_TOOLS)
    if (showDebugInfo_) {
        sys.graphics->setDebugRenderEnabled(true);
        sys.graphics->renderDebugInfo(*sys.entities);
        // Shows: collision boxes, entity IDs, component counts
    }
    #endif
}
```

### Entity Creation Helpers

```cpp
// Helper functions reduce boilerplate further
Entity Game::createPlatform(float x, float y, float width, float height) {
    auto& sys = engine_->systems();

    Entity platform = sys.entities->createEntity();
    sys.entities->emplace<Transform2D>(platform, Transform2D{.x = x, .y = y});
    sys.entities->emplace<DebugRect>(platform, DebugRect{
        .size = {width, height},
        .fillColor = platformColor_,
        .layer = RenderLayer::Platforms
    });

    // Physics body
    sys.physics->createBody(platform, PhysicsBodyDef{
        .type = BodyType::Static,
        .transform = {.x = x, .y = y},
        .size = {width, height}
    });

    return platform;
}

Entity Game::createEnemy(float x, float y, const std::string& type) {
    auto& sys = engine_->systems();

    Entity enemy = sys.entities->createEntity();
    sys.entities->emplace<Transform2D>(enemy, Transform2D{.x = x, .y = y});
    sys.entities->emplace<AnimatedSprite>(enemy, loadEnemySprite(type));
    sys.entities->emplace<EnemyTag>(enemy, EnemyTag{.type = type});

    return enemy;
}
```

## Implementation Plan

### Phase 1: Debug Primitive Components

**Files to create/modify:**
- `bestow-contract/src/bestow.types.cppm` - Add DebugRect, DebugCircle
- `bestow-graphics/src/GraphicsSystem.cpp` - Render debug primitives

```cpp
// In bestow.types.cppm
export struct DebugRect {
    Vec2 size{32.0f, 32.0f};
    Color fillColor{255, 255, 255, 255};
    Color outlineColor{0, 0, 0, 0};
    float outlineWidth{0.0f};
    RenderLayer layer{RenderLayer::Default};
};

export struct DebugCircle {
    float radius{16.0f};
    Color fillColor{255, 255, 255, 255};
    Color outlineColor{0, 0, 0, 0};
    float outlineWidth{0.0f};
    RenderLayer layer{RenderLayer::Default};
};
```

### Phase 2: renderEntities Implementation

```cpp
void GraphicsSystem::renderEntities(const IEntitySystem& entities) {
    // Collect all renderable entities
    std::vector<RenderItem> items;

    // Sprites
    for (auto [entity, transform, sprite] :
         entities.view<Transform2D, Sprite>().each()) {
        items.push_back({
            .entity = entity,
            .layer = sprite.layer,
            .type = RenderType::Sprite,
            .transform = transform
        });
    }

    // Animated sprites
    for (auto [entity, transform, anim] :
         entities.view<Transform2D, AnimatedSprite>().each()) {
        items.push_back({
            .entity = entity,
            .layer = anim.sheet.layer,
            .type = RenderType::AnimatedSprite,
            .transform = transform
        });
    }

    // Debug rects
    for (auto [entity, transform, rect] :
         entities.view<Transform2D, DebugRect>().each()) {
        items.push_back({
            .entity = entity,
            .layer = rect.layer,
            .type = RenderType::DebugRect,
            .transform = transform
        });
    }

    // Sort by layer
    std::sort(items.begin(), items.end(),
        [](const RenderItem& a, const RenderItem& b) {
            return a.layer < b.layer;
        });

    // Render in order
    for (const RenderItem& item : items) {
        renderItem(entities, item);
    }
}
```

### Phase 3: Viewport Culling

```cpp
void GraphicsSystem::renderEntities(const IEntitySystem& entities) {
    if (!viewportCullingEnabled_) {
        renderAllEntities(entities);
        return;
    }

    // Calculate visible bounds
    float halfWidth = camera_.viewportSize.width / (2.0f * camera_.zoom);
    float halfHeight = camera_.viewportSize.height / (2.0f * camera_.zoom);

    Rect visibleBounds{
        camera_.transform.x - halfWidth,
        camera_.transform.y - halfHeight,
        halfWidth * 2.0f,
        halfHeight * 2.0f
    };

    // Only render entities within bounds
    for (auto [entity, transform, sprite] :
         entities.view<Transform2D, Sprite>().each()) {
        if (isVisible(transform, sprite, visibleBounds)) {
            renderSprite(transform, sprite);
        }
    }
}
```

### Phase 4: Tests

**File:** `tests/unit/SpriteRendererTests.cpp`

```cpp
TEST(SpriteRendererTest, RenderLayerOrdering) {
    auto entities = createEntitySystem();
    auto graphics = createGraphicsSystem();

    // Create entities at different layers
    Entity background = entities->createEntity();
    entities->emplace<Transform2D>(background, Transform2D{.x = 0, .y = 0});
    entities->emplace<DebugRect>(background, DebugRect{
        .layer = RenderLayer::Background
    });

    Entity player = entities->createEntity();
    entities->emplace<Transform2D>(player, Transform2D{.x = 0, .y = 0});
    entities->emplace<DebugRect>(player, DebugRect{
        .layer = RenderLayer::Player
    });

    // Verify render order
    auto renderOrder = graphics->getRenderOrder(*entities);
    EXPECT_EQ(renderOrder[0], background);
    EXPECT_EQ(renderOrder[1], player);
}

TEST(SpriteRendererTest, ViewportCulling) {
    auto entities = createEntitySystem();
    auto graphics = createGraphicsSystem();

    graphics->setCamera(Camera{
        .transform = {.x = 0, .y = 0},
        .zoom = 1.0f,
        .viewportSize = {800, 600}
    });
    graphics->setViewportCulling(true);

    // Entity outside viewport
    Entity offscreen = entities->createEntity();
    entities->emplace<Transform2D>(offscreen, Transform2D{.x = 2000, .y = 2000});
    entities->emplace<DebugRect>(offscreen, DebugRect{.size = {32, 32}});

    // Entity inside viewport
    Entity onscreen = entities->createEntity();
    entities->emplace<Transform2D>(onscreen, Transform2D{.x = 100, .y = 100});
    entities->emplace<DebugRect>(onscreen, DebugRect{.size = {32, 32}});

    auto visible = graphics->getVisibleEntities(*entities);
    EXPECT_EQ(visible.size(), 1);
    EXPECT_EQ(visible[0], onscreen);
}

TEST(SpriteRendererTest, DebugRectRendering) {
    auto entities = createEntitySystem();
    auto graphics = createGraphicsSystem();

    Entity rect = entities->createEntity();
    entities->emplace<Transform2D>(rect, Transform2D{.x = 100, .y = 100});
    entities->emplace<DebugRect>(rect, DebugRect{
        .size = {50.0f, 30.0f},
        .fillColor = {255, 0, 0, 255},
        .outlineColor = {0, 0, 0, 255},
        .outlineWidth = 2.0f
    });

    // Should render without crashing
    graphics->renderEntities(*entities);
}
```

## Migration Guide

### Before (Manual Rendering)

```cpp
void Game::render(float alpha) {
    // 580 lines of rendering code...
    for (const auto& platform : platforms_) {
        auto* size = sys.entities->tryGet<Size2D>(platform);
        if (size && sys.physics->hasBody(platform)) {
            Vec2 pos = sys.physics->getPosition(platform);
            sys.graphics->drawRect(
                Canvas{{int(pos.x - size->width/2), int(pos.y - size->height/2)},
                       {int(size->width), int(size->height)}},
                platformColor, true);
        }
    }
    // ... repeat for 11 more entity types
}
```

### After (Automatic Rendering)

```cpp
void Game::initialize() {
    // Add DebugRect to entities at creation time
    Entity platform = entities->createEntity();
    entities->emplace<Transform2D>(platform, Transform2D{.x = x, .y = y});
    entities->emplace<DebugRect>(platform, DebugRect{
        .size = {width, height},
        .fillColor = platformColor_,
        .layer = RenderLayer::Platforms
    });
}

void Game::render(float alpha) {
    // One line replaces 580
    sys.graphics->renderEntities(*sys.entities);

    // Only custom rendering remains
    renderUI();
}
```

## Performance Considerations

### Batching
- Sprites with same texture are batched automatically
- Debug primitives are batched by type
- Reduces draw calls significantly

### Culling
- Viewport culling skips off-screen entities
- Enable with `setViewportCulling(true)`
- Spatial partitioning for large entity counts (future)

### Memory
- No additional memory per entity
- Render list is temporary per frame
- Layer sorting is O(n log n)

### Best Practices
1. Use consistent RenderLayer values
2. Enable viewport culling for large levels
3. Use DebugRect for prototyping, switch to Sprite for production
4. Batch similar entities on same layer

## Related Documentation

- [Graphics System](Graphics-System.md) - Core graphics API
- [Entity System](Entity-System.md) - Entity management
- [Camera System](Camera-System.md) - Camera integration
- [Assets System](Assets-System.md) - Texture loading

## Status

| Task | Status |
|------|--------|
| Interface design | ✅ Complete |
| DebugRect component | ✅ Complete |
| DebugCircle component | ✅ Complete |
| DebugLine component | ✅ Complete |
| renderEntities impl | ✅ Complete |
| Viewport culling | ✅ Complete |
| Batching | 🔲 Future |
| Tests | 🔲 Planned |
| Documentation | ✅ Complete |

## Implementation Notes

The Sprite Renderer was implemented across several files:

**Components** (`bestow-contract/src/bestow.types.cppm`):
- `DebugRect` - Rectangle with fill/outline colors and layer
- `DebugCircle` - Circle with fill/outline colors, segments, and layer
- `DebugLine` - Line with color, thickness, and layer
- `RenderLayers` namespace with preset layer values (Background, Player, UI, etc.)

**Interface** (`bestow-contract/src/bestow.graphics.cppm`):
- `renderEntities(IEntitySystem&)` - Render all entities with visual components
- `renderEntities(IEntitySystem&, minLayer, maxLayer)` - Render within layer range
- `setViewportCulling(bool)` / `isViewportCullingEnabled()` - Culling control

**Implementation** (`bestow-graphics/src/GraphicsSystem.cpp`):
- Collects all entities with Transform2D + visual components
- Sorts by render layer
- Optionally culls off-screen entities
- Renders each type appropriately (drawRect, drawCircle, drawLine)
