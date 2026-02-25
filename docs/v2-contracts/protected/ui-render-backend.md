# UI Render Backend

> **Visibility:** Protected (C++ peer systems only -- not in Lua API)
> **Tier:** 5
> **Dependencies:** Types, GraphicsContext
> **Consumers:** UI System only

## Purpose

IUIRenderBackend is the thin GPU abstraction that the UI system uses to draw 2D overlay elements without knowing which graphics API is active. Each rendering backend (OpenGL, Vulkan, Metal) provides its own implementation. The interface covers exactly the primitives required for UI rendering: texture management, scissor-based clipping, and indexed triangle submission with a fixed vertex format (`UIVertex`). The UI system obtains its render backend via `IGraphicsContextCore::getUIRenderBackend()` and issues all draw commands between matched `beginUIPass()` / `endUIPass()` calls. The V2 contract streamlines the V1 interface by using strong typed handles (`TextureHandle`) and adding the direct `drawTriangles()` call alongside the compiled-geometry path.

## Contract: `IUIRenderBackend`

### Render Pass

| Method | Returns | Description |
|--------|---------|-------------|
| `beginUIPass()` | `void` | Begin a UI rendering pass. Configures GPU state for 2D overlay rendering: disables depth testing, enables premultiplied alpha blending, and sets an orthographic projection matrix matching the current viewport size. Must be called before any `drawTriangles()` calls. Nesting is not supported -- calling `beginUIPass()` while already in a pass is undefined behaviour. |
| `endUIPass()` | `void` | End the current UI rendering pass. Flushes any batched draw commands and restores the previous GPU render state (depth testing, blending mode, projection). Must be paired with a preceding `beginUIPass()`. |

### Texture Management

| Method | Returns | Description |
|--------|---------|-------------|
| `createTexture(int width, int height, int channels, const unsigned char* data)` | `TextureHandle` | Create a GPU texture from raw pixel data. The `data` pointer must contain `width * height * channels` bytes in row-major order. `channels` is typically 1 (greyscale font atlas), 3 (RGB), or 4 (RGBA). Returns a valid `TextureHandle` on success. Returns a handle with `id == 0` if texture creation fails (e.g. out of GPU memory, dimensions exceed maximum). Ownership of the GPU resource is held by the backend until `destroyTexture()` is called. |
| `destroyTexture(TextureHandle handle)` | `void` | Destroy a previously created texture and free its GPU memory. Safe to call with an invalid handle (no-op). Must not be called for a texture that is currently bound in an active draw call. |

### Scissor (Clipping)

| Method | Returns | Description |
|--------|---------|-------------|
| `setScissor(int x, int y, int w, int h)` | `void` | Enable scissor testing and set the clip rectangle. All subsequent `drawTriangles()` calls will be clipped to the rectangle defined by the top-left corner (`x`, `y`) and dimensions (`w`, `h`), in screen-space pixels with the origin at the top-left of the viewport. Coordinates are clamped to the viewport bounds. |
| `clearScissor()` | `void` | Disable scissor testing. Subsequent draw calls will render to the full viewport without clipping. |

### Drawing

| Method | Returns | Description |
|--------|---------|-------------|
| `drawTriangles(std::span<const UIVertex> vertices, std::span<const std::uint32_t> indices, TextureHandle texture)` | `void` | Submit indexed triangles for immediate rendering. `vertices` contains the vertex data (position, colour, UV). `indices` contains triangle indices (three per triangle, counter-clockwise winding). `texture` is the texture to sample; pass a handle with `id == 0` for untextured (solid colour) rendering. The vertex and index data is consumed immediately -- the caller may reuse or free the memory after this call returns. |

### Viewport

| Method | Returns | Description |
|--------|---------|-------------|
| `setViewportSize(int width, int height)` | `void` | Update the viewport dimensions. Called when the window resizes. Updates the internal orthographic projection matrix used by `beginUIPass()`. Must be called before `beginUIPass()` when the window size changes. |

## Types

### UIVertex

```cpp
struct UIVertex {
    Vec2 position;      // Screen-space position in pixels
    Color color;        // RGBA colour (0-255 per component)
    Vec2 texCoord;      // Texture UV coordinates (0.0 - 1.0)
};
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `position` | `Vec2` | `{0, 0}` | The vertex position in screen-space pixels. The origin is at the top-left corner of the viewport. X increases rightward, Y increases downward. |
| `color` | `Color` | `{255, 255, 255, 255}` | Per-vertex RGBA colour with components in the 0-255 range. The backend converts to premultiplied alpha internally before blending. When a texture is bound, the vertex colour is multiplied with the texture sample (modulation). |
| `texCoord` | `Vec2` | `{0, 0}` | Texture UV coordinates in the 0.0 to 1.0 range. Used to sample the bound texture. Ignored when rendering untextured geometry (texture handle is null). |

### TextureHandle

The V2 strong typed handle:

```cpp
using TextureHandle = Handle<struct TextureTag>;
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `id` | `std::uint64_t` | `0` | Unique identifier for the GPU texture. Zero means invalid/null (no texture bound). |

### UIScissorRect (V1 Compatibility Reference)

The V2 `setScissor()` method takes individual `int` parameters instead of a struct. For reference, the V1 struct form:

```cpp
struct UIScissorRect {
    int x = 0;         // Left edge in screen pixels
    int y = 0;         // Top edge in screen pixels
    int width = 0;     // Rectangle width in pixels
    int height = 0;    // Rectangle height in pixels
};
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `x` | `int` | `0` | X coordinate of the scissor rectangle's left edge, in screen pixels from the viewport origin. |
| `y` | `int` | `0` | Y coordinate of the scissor rectangle's top edge, in screen pixels from the viewport origin. |
| `width` | `int` | `0` | Width of the scissor rectangle in pixels. |
| `height` | `int` | `0` | Height of the scissor rectangle in pixels. |

## Blending Behaviour

The UI render backend always uses **premultiplied alpha blending** during `beginUIPass()` / `endUIPass()`:

```
outputColor.rgb = srcColor.rgb + dstColor.rgb * (1 - srcColor.a)
outputColor.a   = srcColor.a   + dstColor.a   * (1 - srcColor.a)
```

The backend converts `UIVertex::color` from straight alpha (0-255 components) to premultiplied alpha internally. This allows correct rendering of semi-transparent text, icons, and overlapping UI elements without artefacts.

## Examples

### Basic UI rendering pass

```cpp
void UISystem::render(IUIRenderBackend& backend) {
    backend.beginUIPass();

    for (const auto& batch : renderBatches_) {
        if (batch.hasScissor) {
            backend.setScissor(
                batch.scissor.x, batch.scissor.y,
                batch.scissor.width, batch.scissor.height);
        } else {
            backend.clearScissor();
        }

        backend.drawTriangles(
            batch.vertices,
            batch.indices,
            batch.texture);
    }

    backend.clearScissor();
    backend.endUIPass();
}
```

### Creating a font atlas texture

```cpp
TextureHandle UISystem::uploadFontAtlas(
    IUIRenderBackend& backend,
    const FontData& fontData
) {
    // Font atlas is single-channel greyscale -- expand to RGBA for GPU
    std::vector<unsigned char> rgba(
        fontData.atlasWidth * fontData.atlasHeight * 4);

    for (int i = 0; i < fontData.atlasWidth * fontData.atlasHeight; ++i) {
        rgba[i * 4 + 0] = 255;                      // R
        rgba[i * 4 + 1] = 255;                      // G
        rgba[i * 4 + 2] = 255;                      // B
        rgba[i * 4 + 3] = fontData.atlasPixels[i];  // A from greyscale
    }

    TextureHandle handle = backend.createTexture(
        fontData.atlasWidth, fontData.atlasHeight, 4, rgba.data());

    if (!handle.isValid()) {
        LOG_ERROR("Failed to create font atlas texture ({}x{})",
                  fontData.atlasWidth, fontData.atlasHeight);
    }

    return handle;
}
```

### Drawing a solid-colour rectangle (no texture)

```cpp
void UISystem::drawRect(
    IUIRenderBackend& backend,
    float x, float y, float w, float h,
    Color color
) {
    std::array<UIVertex, 4> vertices = {{
        {{x,     y},     color, {0, 0}},   // top-left
        {{x + w, y},     color, {1, 0}},   // top-right
        {{x + w, y + h}, color, {1, 1}},   // bottom-right
        {{x,     y + h}, color, {0, 1}},   // bottom-left
    }};

    std::array<std::uint32_t, 6> indices = {0, 1, 2, 0, 2, 3};

    // TextureHandle with id=0 means no texture (solid colour)
    TextureHandle noTexture{};
    backend.drawTriangles(vertices, indices, noTexture);
}
```

### Handling window resize

```cpp
void UISystem::onWindowResize(IUIRenderBackend& backend, int newWidth, int newHeight) {
    backend.setViewportSize(newWidth, newHeight);

    // Recalculate UI layout for new dimensions
    rootElement_.layout(static_cast<float>(newWidth), static_cast<float>(newHeight));
}
```

### Cleanup on shutdown

```cpp
void UISystem::shutdown(IUIRenderBackend& backend) {
    // Destroy all cached textures
    for (auto& [name, handle] : textureCache_) {
        backend.destroyTexture(handle);
    }
    textureCache_.clear();
}
```
