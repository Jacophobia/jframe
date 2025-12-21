# UI System Architecture

## Overview

This document describes the detailed architecture for the UI system overhaul, focusing on how contracts interact and how renderer-agnosticism is achieved.

## Dependency Graph

```
                              ┌─────────────┐
                              │ Application │
                              └──────┬──────┘
                                     │ uses
                    ┌────────────────┼────────────────┐
                    ▼                ▼                ▼
           ┌────────────────┐ ┌─────────────┐ ┌──────────────┐
           │  IUISystem     │ │IGraphics-   │ │IGraphics3D-  │
           │                │ │   System    │ │   System     │
           └───────┬────────┘ └──────┬──────┘ └──────┬───────┘
                   │                 │               │
                   │                 └───────┬───────┘
                   │                         │
                   │ depends on              │ extends
                   ▼                         ▼
           ┌────────────────────────────────────────────────┐
           │              IGraphicsContext                   │
           │                                                 │
           │  + getUIRenderBackend(): IUIRenderBackend*     │
           │  + getViewportSize(): Size                      │
           │  + getNativeWindowHandle(): void*               │
           │  + isInFrame(): bool                            │
           └────────────────────────────────────────────────┘
                              │
                              │ provides
                              ▼
           ┌────────────────────────────────────────────────┐
           │              IUIRenderBackend                   │
           │                                                 │
           │  + compileGeometry(vertices, indices)          │
           │  + releaseGeometry(handle)                      │
           │  + beginUIPass()                                │
           │  + renderGeometry(geometry, translation, tex)  │
           │  + endUIPass()                                  │
           │  + loadTexture(path) / createTexture(data)     │
           │  + releaseTexture(handle)                       │
           │  + enableScissor(bool) / setScissorRegion()    │
           └────────────────────────────────────────────────┘
```

## Contract Hierarchy

### Layer 0: Base Graphics Context

```cpp
// bestow.graphics.context.cppm
class IGraphicsContext {
    virtual IUIRenderBackend* getUIRenderBackend() = 0;
    virtual Size getViewportSize() const = 0;
    virtual void* getNativeWindowHandle() const = 0;
    virtual bool isInFrame() const = 0;
};
```

This is the minimal interface that all graphics systems must implement. It provides:
- Access to the UI render backend (created by the graphics impl)
- Viewport information for UI layout
- Native window handle for input integration
- Frame state for synchronization

### Layer 1: Specialized Graphics Contracts

```cpp
// bestow.graphics.cppm (2D)
class IGraphicsSystem : public IGraphicsContext {
    // 2D-specific: sprites, primitives, 2D camera
    virtual void draw(const Sprite&) = 0;
    virtual void drawRect(...) = 0;
    virtual void drawText(...) = 0;
    // ...
};

// bestow.graphics3d.cppm (3D)
class IGraphics3DSystem : public IGraphicsContext {
    // 3D-specific: meshes, materials, 3D camera, lighting
    virtual void drawMesh(...) = 0;
    virtual void setCamera(const Camera3D&) = 0;
    virtual void debugDrawLine(...) = 0;
    // ...
};
```

Both inherit from `IGraphicsContext`, ensuring UI support is available regardless of which graphics system is used.

### Layer 2: UI Render Backend Contract

```cpp
// bestow.uirender.cppm
class IUIRenderBackend {
    // Geometry lifecycle
    virtual UIGeometryHandle compileGeometry(
        std::span<const UIVertex> vertices,
        std::span<const std::uint32_t> indices) = 0;
    virtual void releaseGeometry(UIGeometryHandle) = 0;

    // Rendering
    virtual void beginUIPass() = 0;
    virtual void renderGeometry(
        UIGeometryHandle geometry,
        Vec2 translation,
        UITextureHandle texture) = 0;
    virtual void endUIPass() = 0;

    // Textures
    virtual UITextureHandle loadTexture(const path&, int& w, int& h) = 0;
    virtual UITextureHandle createTexture(span<byte> data, int w, int h) = 0;
    virtual void releaseTexture(UITextureHandle) = 0;

    // Scissor (clipping)
    virtual void enableScissor(bool) = 0;
    virtual void setScissorRegion(const UIScissorRect&) = 0;
};
```

This is what RmlUi needs, abstracted from any specific renderer.

### Layer 3: UI System Contract

```cpp
// bestow.ui.cppm (existing, unchanged interface)
class IUISystem {
    virtual Result<void, UIError> initialize(const UIConfig&) = 0;
    virtual Result<UIDocumentHandle, UIError> loadDocument(const path&) = 0;
    virtual void render() = 0;
    // ... full DOM-like API
};
```

## Implementation Structure

### Graphics Implementations

Each graphics implementation provides its own UI render backend:

```
bestow-opengl/
├── src/
│   ├── bestow.opengl.impl.cppm      # OpenGLGraphicsSystem, OpenGLGraphics3DSystem
│   ├── OpenGLUIRenderBackend.cpp    # Shared between 2D and 3D
│   └── OpenGLUIRenderBackend.hpp

bestow-vulkan/
├── src/
│   ├── bestow.vulkan.impl.cppm      # VulkanGraphics3DSystem
│   ├── VulkanUIRenderBackend.cpp    # Vulkan-specific UI backend
│   └── VulkanUIRenderBackend.hpp
```

### UI System Implementation

```
bestow-ui/
├── src/
│   ├── bestow.ui.impl.cppm          # RmlUISystem (uses IGraphicsContext)
│   ├── BestowRmlRenderInterface.cpp # Adapts IUIRenderBackend → Rml::RenderInterface
│   └── BestowRmlSystemInterface.cpp # Implements Rml::SystemInterface
```

## Data Flow

### Initialization Flow

```
1. Application creates Engine
2. Engine registers graphics system (e.g., VulkanGraphics3DSystem)
   - Graphics impl is created
   - Graphics impl creates its VulkanUIRenderBackend internally (lazy)

3. Engine registers UI system (RmlUISystem)
   - RmlUISystem receives IGraphicsContext& via DI

4. RmlUISystem::initialize() called
   - Calls graphics_->getUIRenderBackend() to get the backend
   - Creates BestowRmlRenderInterface wrapping the backend
   - Initializes RmlUi with our interfaces
```

### Render Flow

```
1. Application game loop:
   graphics->beginFrame();

   // Game renders its 3D/2D content
   graphics->drawMesh(...);
   graphics->debugDrawLine(...);

   // UI renders on top
   uiSystem->render();

   graphics->endFrame();

2. Inside uiSystem->render():
   renderBackend_->beginUIPass();    // Prepare for 2D overlay
   rmlContext_->Render();             // RmlUi issues draw calls
   renderBackend_->endUIPass();       // Flush UI rendering

3. Inside RmlUi render (via BestowRmlRenderInterface):
   - CompileGeometry() → backend_->compileGeometry()
   - RenderGeometry() → backend_->renderGeometry()
   - EnableScissorRegion() → backend_->enableScissor()
   - etc.
```

## RmlUi Integration

### Adapter Pattern

RmlUi requires implementing `Rml::RenderInterface` and `Rml::SystemInterface`. We create adapters:

```cpp
// BestowRmlRenderInterface.cpp
class BestowRmlRenderInterface : public Rml::RenderInterface {
public:
    explicit BestowRmlRenderInterface(IUIRenderBackend& backend)
        : backend_(&backend) {}

    // Rml::RenderInterface methods delegate to IUIRenderBackend
    Rml::CompiledGeometryHandle CompileGeometry(
        Rml::Span<const Rml::Vertex> vertices,
        Rml::Span<const int> indices) override
    {
        // Convert Rml::Vertex to UIVertex
        std::vector<UIVertex> uiVerts = convertVertices(vertices);
        std::vector<uint32_t> uiIndices(indices.begin(), indices.end());

        return static_cast<Rml::CompiledGeometryHandle>(
            backend_->compileGeometry(uiVerts, uiIndices));
    }

    void RenderGeometry(
        Rml::CompiledGeometryHandle geometry,
        Rml::Vector2f translation,
        Rml::TextureHandle texture) override
    {
        backend_->renderGeometry(
            static_cast<UIGeometryHandle>(geometry),
            Vec2{translation.x, translation.y},
            static_cast<UITextureHandle>(texture));
    }

    // ... other methods

private:
    IUIRenderBackend* backend_;
};
```

### Vertex Format Conversion

RmlUi vertex format:
```cpp
struct Rml::Vertex {
    Vector2f position;              // 2D position
    ColourPremultiplied colour;     // RGBA premultiplied alpha
    Vector2f tex_coord;             // UV coordinates
};
```

Bestow UI vertex format:
```cpp
struct UIVertex {
    Vec2 position;     // 2D position
    Color color;       // RGBA (will be converted to premultiplied)
    Vec2 texCoord;     // UV coordinates
};
```

Conversion is straightforward, just need to handle premultiplied alpha correctly.

## OpenGL UI Render Backend

### Key Implementation Details

```cpp
class OpenGLUIRenderBackend : public IUIRenderBackend {
public:
    OpenGLUIRenderBackend() {
        // Create shader for UI rendering
        // Position + Color + TexCoord → Textured colored quads
        createShaderProgram();
    }

    UIGeometryHandle compileGeometry(
        std::span<const UIVertex> vertices,
        std::span<const uint32_t> indices) override
    {
        // Create VAO/VBO/EBO for this geometry
        GLuint vao, vbo, ebo;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(UIVertex),
                     vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t),
                     indices.data(), GL_STATIC_DRAW);

        // Setup vertex attributes
        // Position: location 0
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(UIVertex),
                              (void*)offsetof(UIVertex, position));
        glEnableVertexAttribArray(0);

        // Color: location 1
        glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(UIVertex),
                              (void*)offsetof(UIVertex, color));
        glEnableVertexAttribArray(1);

        // TexCoord: location 2
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(UIVertex),
                              (void*)offsetof(UIVertex, texCoord));
        glEnableVertexAttribArray(2);

        // Store metadata
        CompiledGeometry geo{vao, vbo, ebo, static_cast<GLsizei>(indices.size())};
        UIGeometryHandle handle = nextHandle_++;
        geometries_[handle] = geo;

        return handle;
    }

    void beginUIPass() override {
        // Setup state for 2D UI rendering
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);  // Premultiplied alpha

        // Set orthographic projection
        glUseProgram(uiShader_);
        glUniformMatrix4fv(projectionLoc_, 1, GL_FALSE,
                          glm::value_ptr(orthoProjection_));
    }

    void renderGeometry(UIGeometryHandle geometry, Vec2 translation,
                        UITextureHandle texture) override
    {
        auto it = geometries_.find(geometry);
        if (it == geometries_.end()) return;

        // Set translation uniform
        glUniform2f(translationLoc_, translation.x, translation.y);

        // Bind texture (or white texture if none)
        glActiveTexture(GL_TEXTURE0);
        if (texture != 0) {
            auto texIt = textures_.find(texture);
            glBindTexture(GL_TEXTURE_2D, texIt != textures_.end() ? texIt->second : whiteTexture_);
        } else {
            glBindTexture(GL_TEXTURE_2D, whiteTexture_);
        }

        // Draw
        glBindVertexArray(it->second.vao);
        glDrawElements(GL_TRIANGLES, it->second.indexCount, GL_UNSIGNED_INT, nullptr);
    }

    void endUIPass() override {
        // Restore state
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
    }

    void enableScissor(bool enable) override {
        if (enable) {
            glEnable(GL_SCISSOR_TEST);
        } else {
            glDisable(GL_SCISSOR_TEST);
        }
    }

    void setScissorRegion(const UIScissorRect& region) override {
        // Note: OpenGL scissor Y is from bottom, RmlUi is from top
        int flippedY = viewportHeight_ - region.y - region.height;
        glScissor(region.x, flippedY, region.width, region.height);
    }

private:
    GLuint uiShader_;
    GLuint whiteTexture_;
    GLint projectionLoc_;
    GLint translationLoc_;
    glm::mat4 orthoProjection_;
    int viewportWidth_, viewportHeight_;

    struct CompiledGeometry {
        GLuint vao, vbo, ebo;
        GLsizei indexCount;
    };
    std::unordered_map<UIGeometryHandle, CompiledGeometry> geometries_;
    std::unordered_map<UITextureHandle, GLuint> textures_;
    UIGeometryHandle nextHandle_ = 1;
    UITextureHandle nextTexHandle_ = 1;
};
```

## Vulkan UI Render Backend

### Key Differences from OpenGL

1. **Command buffer integration**: Must record into the current frame's command buffer
2. **Render pass**: UI should be rendered in a compatible render pass (or separate)
3. **Descriptor sets**: Textures bound via descriptor sets
4. **Synchronization**: Must handle frame-in-flight properly

### Implementation Sketch

```cpp
class VulkanUIRenderBackend : public IUIRenderBackend {
public:
    VulkanUIRenderBackend(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkQueue graphicsQueue,
        uint32_t graphicsQueueFamily,
        VkRenderPass renderPass,
        uint32_t subpass = 0)
        : device_(device),
          physicalDevice_(physicalDevice),
          graphicsQueue_(graphicsQueue),
          renderPass_(renderPass)
    {
        createPipeline();
        createDescriptorPool();
        createWhiteTexture();
    }

    void beginUIPass() override {
        // Record commands into the current command buffer
        // (obtained from graphics system via callback or stored reference)
        vkCmdBindPipeline(currentCmdBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, uiPipeline_);

        // Set viewport/scissor to full screen by default
        VkViewport viewport{0, 0, (float)viewportWidth_, (float)viewportHeight_, 0, 1};
        vkCmdSetViewport(currentCmdBuffer_, 0, 1, &viewport);
    }

    void renderGeometry(UIGeometryHandle geometry, Vec2 translation,
                        UITextureHandle texture) override
    {
        auto& geo = geometries_[geometry];

        // Push constants for translation
        struct PushConstants {
            float translationX, translationY;
            float scaleX, scaleY;
        } push = {translation.x, translation.y,
                  2.0f / viewportWidth_, -2.0f / viewportHeight_};
        vkCmdPushConstants(currentCmdBuffer_, pipelineLayout_,
                          VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push), &push);

        // Bind texture descriptor
        VkDescriptorSet texDescriptor = getTextureDescriptor(texture);
        vkCmdBindDescriptorSets(currentCmdBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                               pipelineLayout_, 0, 1, &texDescriptor, 0, nullptr);

        // Bind vertex/index buffers
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(currentCmdBuffer_, 0, 1, &geo.vertexBuffer, &offset);
        vkCmdBindIndexBuffer(currentCmdBuffer_, geo.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        // Draw
        vkCmdDrawIndexed(currentCmdBuffer_, geo.indexCount, 1, 0, 0, 0);
    }

    void enableScissor(bool enable) override {
        scissorEnabled_ = enable;
        if (!enable) {
            // Reset to full viewport
            VkRect2D fullScissor{{0, 0}, {(uint32_t)viewportWidth_, (uint32_t)viewportHeight_}};
            vkCmdSetScissor(currentCmdBuffer_, 0, 1, &fullScissor);
        }
    }

    void setScissorRegion(const UIScissorRect& region) override {
        if (scissorEnabled_) {
            VkRect2D scissor{
                {region.x, region.y},
                {(uint32_t)region.width, (uint32_t)region.height}
            };
            vkCmdSetScissor(currentCmdBuffer_, 0, 1, &scissor);
        }
    }

private:
    VkDevice device_;
    VkPhysicalDevice physicalDevice_;
    VkQueue graphicsQueue_;
    VkRenderPass renderPass_;

    VkPipeline uiPipeline_;
    VkPipelineLayout pipelineLayout_;
    VkDescriptorPool descriptorPool_;
    VkDescriptorSetLayout descriptorSetLayout_;

    VkCommandBuffer currentCmdBuffer_;  // Set by graphics system each frame
    bool scissorEnabled_ = false;
    int viewportWidth_, viewportHeight_;

    struct CompiledGeometry {
        VkBuffer vertexBuffer;
        VkBuffer indexBuffer;
        VkDeviceMemory memory;
        uint32_t indexCount;
    };
    std::unordered_map<UIGeometryHandle, CompiledGeometry> geometries_;
};
```

## Synchronization Considerations

### Frame Timing

The UI render backend needs to know when to render. This is handled by:

1. Graphics system calls `beginFrame()` / `endFrame()`
2. Game calls `uiSystem->render()` between those
3. `render()` calls `renderBackend_->beginUIPass()` and `endUIPass()`

For Vulkan, the UI backend needs access to the current command buffer:

```cpp
// Option 1: Callback
class VulkanGraphics3DSystem {
    void beginFrame() override {
        // ...acquire swapchain, begin command buffer...
        if (uiBackend_) {
            uiBackend_->setCommandBuffer(currentCmdBuffer_);
        }
    }
};

// Option 2: Query method
class VulkanUIRenderBackend {
    void beginUIPass() override {
        currentCmdBuffer_ = static_cast<VkCommandBuffer>(
            graphics_->getCurrentCommandBuffer());
    }
};
```

### Render Pass Compatibility

The UI should render in the same render pass as the main content (for efficiency) or in a separate overlay pass. This is configured during backend creation.

## Error Handling

All operations use Bestow's `Result<T, E>` pattern:

```cpp
Result<UIGeometryHandle, UIRenderError> compileGeometry(...);
Result<UITextureHandle, UIRenderError> createTexture(...);

enum class UIRenderError {
    Success,
    OutOfMemory,
    InvalidGeometry,
    InvalidTexture,
    ShaderCompilationFailed,
    PipelineCreationFailed,
    NotInitialized
};
```

## Performance Considerations

### Geometry Caching

RmlUi compiles geometry once and reuses it. Our backend should:
- Keep compiled VAO/VBO (OpenGL) or buffers (Vulkan) alive
- Only recompile when document structure changes
- Use dynamic buffers for frequently-changing content

### Texture Atlas

RmlUi generates texture atlases for fonts. We should:
- Support `createTexture()` for runtime-generated textures
- Keep textures in GPU memory
- Implement proper cleanup on `releaseTexture()`

### Batching

RmlUi batches draw calls internally. Our backend just needs to:
- Efficiently handle many small `renderGeometry()` calls
- Minimize state changes (sort by texture if possible)
