# Migration Plan

This document describes how to migrate existing code to the new UI architecture.

## Breaking Changes

### 1. RmlUISystem Constructor Change

**Before:**
```cpp
RmlUISystem()  // No dependencies
```

**After:**
```cpp
RmlUISystem(IGraphicsContext& graphics, IAssetSystem& assets)
```

**Migration:** Update DI registration and any manual construction.

### 2. Window Handle Removed

**Before:**
```cpp
uiSystem->setWindow(glfwWindow);  // Custom method
```

**After:**
```cpp
// No longer needed - UI gets window info from IGraphicsContext
```

**Migration:** Remove `setWindow()` calls.

### 3. IGraphicsSystem/IGraphics3DSystem Changes

**Before:**
```cpp
class IGraphicsSystem {
    // No IGraphicsContext base
};
```

**After:**
```cpp
class IGraphicsSystem : public IGraphicsContext {
    // New methods from IGraphicsContext
    virtual IUIRenderBackend* getUIRenderBackend() = 0;
    virtual bool isInFrame() const = 0;
    virtual void* getRenderContext() const = 0;
    virtual void* getCurrentCommandBuffer() const = 0;
};
```

**Migration:** All graphics implementations must implement these new methods.

### 4. Render Loop Change

**Before:**
```cpp
graphics->beginFrame();
// game rendering
graphics->endFrame();
// UI rendered... somewhere? (was broken)
```

**After:**
```cpp
graphics->beginFrame();
// game rendering
uiSystem->render();  // Explicitly render UI
graphics->endFrame();
```

**Migration:** Add explicit `uiSystem->render()` call in game loop.

## Migration Steps

### Step 1: Update Dependencies

Ensure your application depends on:
- `bestow-contract` (includes new contracts)
- `bestow-ui` (updated implementation)
- Your graphics backend (`bestow-opengl` or `bestow-vulkan`)

### Step 2: Update DI Registration

**Before:**
```cpp
// No specific UI registration pattern
```

**After:**
```cpp
// For 2D OpenGL
container.service<IGraphicsSystem, OpenGLGraphicsSystem>();
container.service<IGraphicsContext, OpenGLGraphicsSystem>();  // Same instance
container.service<IUISystem, RmlUISystem>();

// For 3D Vulkan
container.service<IGraphics3DSystem, VulkanGraphics3DSystem>();
container.service<IGraphicsContext, VulkanGraphics3DSystem>();  // Same instance
container.service<IUISystem, RmlUISystem>();
```

### Step 3: Remove GLFW Window Handling

**Before:**
```cpp
auto* window = static_cast<GLFWwindow*>(graphics->getNativeWindowHandle());
uiSystem->setWindow(window);
```

**After:**
```cpp
// Remove these lines - no longer needed
```

### Step 4: Update Game Loop

**Before:**
```cpp
void gameLoop() {
    while (!graphics->shouldClose()) {
        float dt = timer.getDelta();

        update(dt);

        graphics->beginFrame();
        render();
        graphics->endFrame();

        // UI was not being rendered (broken)
    }
}
```

**After:**
```cpp
void gameLoop() {
    while (!graphics->shouldClose()) {
        float dt = timer.getDelta();

        update(dt);
        uiSystem->update(dt);  // Update UI animations

        graphics->beginFrame();
        render();
        uiSystem->render();    // Render UI on top
        graphics->endFrame();
    }
}
```

### Step 5: Update Input Handling

**Before:**
```cpp
// Input not properly forwarded to UI
```

**After:**
```cpp
void onMouseMove(double x, double y) {
    UIInputEvent event{
        .type = UIInputType::MouseMove,
        .x = static_cast<int>(x),
        .y = static_cast<int>(y)
    };

    if (uiSystem->processInput(event)) {
        return;  // UI consumed the input
    }

    // Handle game input
}
```

### Step 6: Update Viewport Handling

**Before:**
```cpp
// Manual viewport sync (or missing)
```

**After:**
```cpp
void onWindowResize(int width, int height) {
    graphics->setWindowSize({width, height});
    uiSystem->setViewportSize(width, height);
}
```

## Backward Compatibility

### Deprecated Methods

The following methods are deprecated and will be removed:

| Method | Replacement |
|--------|-------------|
| `RmlUISystem::setWindow()` | Removed - use IGraphicsContext |
| `RmlUISystem::setAssetSystem()` | Constructor injection |

### Compile-Time Detection

To help catch migration issues, you can define:

```cpp
#define BESTOW_UI_V2  // Enable new API
```

This will:
- Hide deprecated methods
- Enable stricter type checking
- Warn about old patterns

## Example: Full Migration

### Before (Broken Code)

```cpp
// main.cpp
#include <GLFW/glfw3.h>

int main() {
    // Create systems manually
    auto graphics = std::make_unique<VulkanGraphics3DSystem>();
    graphics->initialize({1280, 720, "My Game"});

    auto ui = std::make_unique<RmlUISystem>();
    ui->setWindow(static_cast<GLFWwindow*>(graphics->getNativeWindowHandle()));
    ui->initialize({});

    // Game loop
    while (!graphics->shouldClose()) {
        graphics->beginFrame();
        // ... game render ...
        graphics->endFrame();
        // UI never rendered!
    }
}
```

### After (Working Code)

```cpp
// main.cpp
import bestow;

int main() {
    // Use DI container
    kgr::container container;

    // Register systems
    container.service<IGraphics3DSystem, VulkanGraphics3DSystem>();
    container.service<IGraphicsContext, VulkanGraphics3DSystem>();
    container.service<IAssetSystem, AssetSystem>();
    container.service<IUISystem, RmlUISystem>();

    // Get systems
    auto& graphics = container.service<IGraphics3DSystem>();
    auto& ui = container.service<IUISystem>();

    // Initialize
    graphics.initialize({1280, 720, "My Game"});
    ui.initialize({});

    // Load UI
    auto hud = ui.loadDocument(":assets:/ui/hud.rml");
    if (hud) ui.showDocument(*hud);

    // Game loop
    while (!graphics.shouldClose()) {
        graphics.beginFrame();
        // ... game render ...
        ui.render();  // UI rendered properly!
        graphics.endFrame();
    }
}
```

## Checklist

Use this checklist when migrating:

- [ ] Updated to latest `bestow-contract`
- [ ] Updated to latest `bestow-ui`
- [ ] Updated to latest graphics backend (`bestow-opengl` or `bestow-vulkan`)
- [ ] Updated DI registration to include `IGraphicsContext`
- [ ] Removed `setWindow()` calls
- [ ] Added `uiSystem->render()` to game loop
- [ ] Added `uiSystem->update(dt)` to game loop
- [ ] Updated input handling to use `processInput()`
- [ ] Updated resize handling to call `setViewportSize()`
- [ ] Tested UI rendering works
- [ ] Tested input handling works
- [ ] Tested hot reload works (if used)

## Troubleshooting

### UI Not Rendering

1. Ensure `uiSystem->render()` is called between `beginFrame()` and `endFrame()`
2. Check that `getUIRenderBackend()` returns non-null
3. Verify the document is loaded and shown: `isDocumentVisible(handle)`

### Textures Missing

1. RmlUi uses its own texture loading via `GenerateTexture()`
2. Font atlases are created automatically
3. External images need proper path resolution

### Scissor Not Working

1. Verify `enableScissor(true)` is called
2. Check scissor region coordinates (Y-axis may be flipped)
3. Ensure nested elements have proper overflow settings

### Performance Issues

1. Check `getDrawCallCount()` - should be low with batching
2. Ensure geometry is being compiled once, not every frame
3. Use profiler to check texture switches

## Support

If you encounter issues during migration:

1. Check this document for common patterns
2. Review the [IMPLEMENTATION.md](./IMPLEMENTATION.md) for detailed code
3. Check the test cases for working examples
4. File an issue with reproduction steps
