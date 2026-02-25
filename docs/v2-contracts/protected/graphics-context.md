# Graphics Context

> **Visibility:** Protected (C++ peer systems only -- not in Lua API)
> **Tier:** 4
> **Dependencies:** Types
> **Consumers:** UI, DevTools, UIRenderBackend

## Purpose

IGraphicsContextCore is the minimal base interface that every rendering backend (OpenGL, Vulkan, Metal, Null) implements. It provides the small set of capabilities that non-rendering systems need from the graphics layer: window dimensions, the native window handle for platform interop, frame state queries, access to the UI render backend, and backend identification for systems that need to branch on the active renderer. Critically, this interface contains **zero backend-specific types** -- no `VkDevice`, no `GLuint`, no `MTLDevice`. Systems that need vendor-specific access must `dynamic_cast` to the optional vendor extension interfaces (`IVulkanContext`, `IOpenGLContext`, etc.) which live in their respective backend libraries, not in `bestow-contract`.

## Contract: `IGraphicsContextCore`

### Window and Viewport

| Method | Returns | Description |
|--------|---------|-------------|
| `getWindowSize() const` | `Size` | Return the current window dimensions in pixels (width, height). This reflects the actual framebuffer size, accounting for DPI scaling on high-DPI displays. |
| `getNativeWindowHandle() const` | `void*` | Return the platform-native window handle. On macOS this is an `NSWindow*`, on Windows an `HWND`, on Linux an `X11 Window` or `wl_surface*`. Used by systems that need platform interop (e.g. input, clipboard, file dialogs). The caller must cast to the appropriate platform type. |

### Frame State

| Method | Returns | Description |
|--------|---------|-------------|
| `isInFrame() const` | `bool` | Return `true` if execution is currently between a `beginFrame()` and `endFrame()` call on the active graphics system. UI and debug rendering systems use this to guard against issuing draw commands outside a valid frame. |

### UI Integration

| Method | Returns | Description |
|--------|---------|-------------|
| `getUIRenderBackend()` | `IUIRenderBackend*` | Return a pointer to the UI render backend implemented by this graphics context. The UI system uses this to issue GPU draw commands (triangles, textures, scissor regions) without knowing which graphics API is active. Returns `nullptr` if the backend does not support UI rendering (e.g. the Null backend). |

### Backend Identification

| Method | Returns | Description |
|--------|---------|-------------|
| `getBackend() const` | `Backend` | Return which rendering API this context implements. Consumers can use this to select code paths, log diagnostics, or conditionally enable features that are only available on certain backends. |

## Types

### Backend

```cpp
enum class Backend : std::uint8_t {
    OpenGL,     // OpenGL 4.1+ (macOS) or 4.6+ (Windows/Linux)
    Vulkan,     // Vulkan 1.2+
    Metal,      // Metal 2+ (macOS/iOS)
    Null        // No-op backend for headless testing and CI
};
```

| Value | Description |
|-------|-------------|
| `OpenGL` | OpenGL rendering backend. Used for rapid prototyping and as a fallback renderer. |
| `Vulkan` | Vulkan rendering backend. The primary and most feature-complete renderer in Bestow. |
| `Metal` | Metal rendering backend. Native Apple GPU API for macOS and iOS. |
| `Null` | No-op backend that accepts all calls but produces no visual output. Used for headless testing, CI pipelines, and server-side game logic. |

### Size

```cpp
struct Size {
    int width = 0;
    int height = 0;
};
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `width` | `int` | `0` | Horizontal dimension in pixels. |
| `height` | `int` | `0` | Vertical dimension in pixels. |

## Vendor Extension Pattern

IGraphicsContextCore intentionally contains no backend-specific types. Systems that need direct GPU API access use the vendor extension pattern:

```cpp
// Vendor extensions live in their respective backend libraries,
// NOT in bestow-contract.

// bestow-vulkan/IVulkanContext.h
class IVulkanContext {
public:
    virtual VkInstance getInstance() const = 0;
    virtual VkDevice getDevice() const = 0;
    virtual VkPhysicalDevice getPhysicalDevice() const = 0;
    virtual VkCommandBuffer getCurrentCommandBuffer() const = 0;
    virtual VkQueue getGraphicsQueue() const = 0;
    virtual std::uint32_t getGraphicsQueueFamily() const = 0;
};

// bestow-opengl/IOpenGLContext.h
class IOpenGLContext {
public:
    virtual void* getGLProcAddress(const char* name) const = 0;
    virtual int getGLVersionMajor() const = 0;
    virtual int getGLVersionMinor() const = 0;
};
```

## Examples

### Querying window size for layout calculations

```cpp
void UISystem::updateLayout(IGraphicsContextCore& gfx) {
    Size windowSize = gfx.getWindowSize();

    // Calculate UI scaling based on window dimensions
    float scaleX = static_cast<float>(windowSize.width) / designWidth_;
    float scaleY = static_cast<float>(windowSize.height) / designHeight_;
    uiScale_ = std::min(scaleX, scaleY);
}
```

### Accessing the UI render backend

```cpp
void UISystem::render(IGraphicsContextCore& gfx) {
    IUIRenderBackend* uiBackend = gfx.getUIRenderBackend();
    if (!uiBackend) {
        // Null backend or backend without UI support -- skip rendering
        return;
    }

    uiBackend->beginUIPass();

    // Issue UI draw commands...
    for (const auto& batch : drawBatches_) {
        uiBackend->setScissor(batch.scissor.x, batch.scissor.y,
                              batch.scissor.w, batch.scissor.h);
        uiBackend->drawTriangles(batch.vertices, batch.indices, batch.texture);
    }

    uiBackend->endUIPass();
}
```

### Guarding draw calls with frame state

```cpp
void DevOverlay::render(IGraphicsContextCore& gfx) {
    if (!gfx.isInFrame()) {
        LOG_WARN("DevOverlay::render() called outside of active frame");
        return;
    }

    // Safe to issue draw commands
    drawFPSCounter(gfx);
    drawMemoryStats(gfx);
}
```

### Backend-specific branching

```cpp
void RenderFeatures::configure(IGraphicsContextCore& gfx) {
    switch (gfx.getBackend()) {
        case IGraphicsContextCore::Backend::Vulkan:
            enableBindlessTextures_ = true;
            enableMeshShaders_ = true;
            break;
        case IGraphicsContextCore::Backend::OpenGL:
            enableBindlessTextures_ = false;
            enableMeshShaders_ = false;
            break;
        case IGraphicsContextCore::Backend::Metal:
            enableBindlessTextures_ = true;
            enableMeshShaders_ = false;
            break;
        case IGraphicsContextCore::Backend::Null:
            // Disable all GPU features
            break;
    }
}
```

### Vendor extension access (outside bestow-contract)

```cpp
void VulkanParticleSystem::initialize(IGraphicsContextCore& gfx) {
    // Attempt to get Vulkan-specific access
    auto* vk = dynamic_cast<IVulkanContext*>(&gfx);
    if (!vk) {
        LOG_ERROR("VulkanParticleSystem requires Vulkan backend");
        return;
    }

    VkDevice device = vk->getDevice();
    VkQueue queue = vk->getGraphicsQueue();
    // ... create Vulkan compute pipelines for GPU particles
}
```
