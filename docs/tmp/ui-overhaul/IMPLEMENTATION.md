# Implementation Guide

This document provides step-by-step implementation instructions for the UI overhaul.

## Implementation Order

The implementation must proceed in this order due to dependencies:

```
Phase 1: Contracts (no dependencies)
    1.1 bestow.graphics.context.cppm
    1.2 bestow.uirender.cppm
    1.3 Update bestow.services.cppm

Phase 2: Graphics Contract Updates
    2.1 Modify bestow.graphics.cppm (2D)
    2.2 Modify bestow.graphics3d.cppm (3D)

Phase 3: UI Render Backend Implementations
    3.1 OpenGLUIRenderBackend (shared 2D/3D)
    3.2 VulkanUIRenderBackend

Phase 4: Graphics System Updates
    4.1 OpenGLGraphicsSystem (add getUIRenderBackend)
    4.2 OpenGLGraphics3DSystem (add getUIRenderBackend)
    4.3 VulkanGraphics3DSystem (add getUIRenderBackend)

Phase 5: UI System Refactor
    5.1 BestowRmlRenderInterface
    5.2 BestowRmlSystemInterface
    5.3 RmlUISystem refactor

Phase 6: Testing
    6.1 Snake game test (Vulkan 3D)
    6.2 2D example test (OpenGL 2D)
    6.3 Hot reload test
```

## Phase 1: Contracts

### 1.1 Create `bestow.graphics.context.cppm`

**File:** `bestow-contract/src/bestow.graphics.context.cppm`

See [CONTRACTS.md](./CONTRACTS.md) for full source.

**CMake:** Add to `bestow-contract/CMakeLists.txt`:
```cmake
target_sources(bestow-contract
    PUBLIC
        FILE_SET CXX_MODULES FILES
        src/bestow.graphics.context.cppm  # Add this line
        # ... existing files
)
```

### 1.2 Create `bestow.uirender.cppm`

**File:** `bestow-contract/src/bestow.uirender.cppm`

See [CONTRACTS.md](./CONTRACTS.md) for full source.

**CMake:** Add to `bestow-contract/CMakeLists.txt`:
```cmake
target_sources(bestow-contract
    PUBLIC
        FILE_SET CXX_MODULES FILES
        src/bestow.uirender.cppm  # Add this line
        # ... existing files
)
```

### 1.3 Update `bestow.services.cppm`

Add the new service definitions:

```cpp
// In bestow.services.cppm

// Add imports
import bestow.graphics.context;
import bestow.uirender;

// Add in the services section
struct IGraphicsContextService : kgr::abstract_service<IGraphicsContext> {};
struct IUIRenderBackendService : kgr::abstract_service<IUIRenderBackend> {};
```

## Phase 2: Graphics Contract Updates

### 2.1 Modify `bestow.graphics.cppm`

**Changes:**

1. Add import at top:
```cpp
import bestow.graphics.context;
```

2. Change class declaration:
```cpp
class IGraphicsSystem : public IGraphicsContext {
```

3. The following methods are inherited from `IGraphicsContext` and must be implemented by concrete classes:
- `getUIRenderBackend()`
- `getViewportSize()` - can delegate to existing `getWindowSize()`
- `getNativeWindowHandle()` - already exists
- `isInFrame()`
- `getRenderContext()`
- `getCurrentCommandBuffer()`

### 2.2 Modify `bestow.graphics3d.cppm`

**Changes:**

1. Add import at top:
```cpp
import bestow.graphics.context;
```

2. Change class declaration:
```cpp
class IGraphics3DSystem : public IGraphicsContext {
```

3. Same inherited methods as above.

## Phase 3: UI Render Backend Implementations

### 3.1 OpenGL UI Render Backend

**File:** `bestow-opengl/src/OpenGLUIRenderBackend.hpp`

```cpp
#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

import bestow.uirender;
import bestow.types;

namespace bestow {

class OpenGLUIRenderBackend : public IUIRenderBackend {
public:
    OpenGLUIRenderBackend() = default;
    ~OpenGLUIRenderBackend() override;

    // IUIRenderBackend interface
    bool initialize() override;
    void shutdown() override;
    bool isInitialized() const override { return initialized_; }

    UIGeometryHandle compileGeometry(
        std::span<const UIVertex> vertices,
        std::span<const std::uint32_t> indices) override;
    void releaseGeometry(UIGeometryHandle geometry) override;

    void beginUIPass() override;
    void renderGeometry(UIGeometryHandle geometry, Vec2 translation,
                        UITextureHandle texture) override;
    void endUIPass() override;

    UITextureHandle loadTexture(const std::filesystem::path& path,
                                 int& outWidth, int& outHeight) override;
    UITextureHandle createTexture(std::span<const std::uint8_t> data,
                                   int width, int height) override;
    void releaseTexture(UITextureHandle texture) override;

    void enableScissor(bool enable) override;
    void setScissorRegion(const UIScissorRect& region) override;

    void setViewportSize(int width, int height) override;
    Size getViewportSize() const override;

    std::uint32_t getDrawCallCount() const override { return drawCalls_; }
    std::uint32_t getTriangleCount() const override { return triangles_; }

private:
    bool initialized_ = false;

    // Shader
    GLuint shaderProgram_ = 0;
    GLint projectionLoc_ = -1;
    GLint translationLoc_ = -1;
    GLint textureLoc_ = -1;

    // White texture for solid colors
    GLuint whiteTexture_ = 0;

    // Viewport
    int viewportWidth_ = 800;
    int viewportHeight_ = 600;
    glm::mat4 projection_;

    // Scissor state
    bool scissorEnabled_ = false;

    // Previous GL state (for restore)
    GLint prevBlendSrc_, prevBlendDst_;
    GLboolean prevDepthTest_, prevBlend_, prevScissor_;

    // Stats
    std::uint32_t drawCalls_ = 0;
    std::uint32_t triangles_ = 0;

    // Compiled geometry
    struct CompiledGeometry {
        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint ebo = 0;
        GLsizei indexCount = 0;
    };
    std::unordered_map<UIGeometryHandle, CompiledGeometry> geometries_;
    UIGeometryHandle nextGeometryHandle_ = 1;

    // Textures
    std::unordered_map<UITextureHandle, GLuint> textures_;
    UITextureHandle nextTextureHandle_ = 1;

    // Helpers
    bool createShader();
    void createWhiteTexture();
};

}  // namespace bestow
```

**File:** `bestow-opengl/src/OpenGLUIRenderBackend.cpp`

```cpp
#include "OpenGLUIRenderBackend.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace bestow {

// Shader source
static const char* kVertexShader = R"(
#version 330 core
layout (location = 0) in vec2 aPosition;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTexCoord;

out vec4 vColor;
out vec2 vTexCoord;

uniform mat4 uProjection;
uniform vec2 uTranslation;

void main() {
    vec2 pos = aPosition + uTranslation;
    gl_Position = uProjection * vec4(pos, 0.0, 1.0);
    vColor = aColor;
    vTexCoord = aTexCoord;
}
)";

static const char* kFragmentShader = R"(
#version 330 core
in vec4 vColor;
in vec2 vTexCoord;

out vec4 fragColor;

uniform sampler2D uTexture;

void main() {
    vec4 texColor = texture(uTexture, vTexCoord);
    // vColor is already premultiplied by RmlUi
    fragColor = vColor * texColor;
}
)";

OpenGLUIRenderBackend::~OpenGLUIRenderBackend() {
    shutdown();
}

bool OpenGLUIRenderBackend::initialize() {
    if (initialized_) return true;

    if (!createShader()) {
        return false;
    }

    createWhiteTexture();

    // Get uniform locations
    projectionLoc_ = glGetUniformLocation(shaderProgram_, "uProjection");
    translationLoc_ = glGetUniformLocation(shaderProgram_, "uTranslation");
    textureLoc_ = glGetUniformLocation(shaderProgram_, "uTexture");

    // Initialize projection (will be updated by setViewportSize)
    projection_ = glm::ortho(0.0f, (float)viewportWidth_,
                             (float)viewportHeight_, 0.0f, -1.0f, 1.0f);

    initialized_ = true;
    return true;
}

void OpenGLUIRenderBackend::shutdown() {
    if (!initialized_) return;

    // Release all geometries
    for (auto& [handle, geo] : geometries_) {
        glDeleteVertexArrays(1, &geo.vao);
        glDeleteBuffers(1, &geo.vbo);
        glDeleteBuffers(1, &geo.ebo);
    }
    geometries_.clear();

    // Release all textures
    for (auto& [handle, texId] : textures_) {
        glDeleteTextures(1, &texId);
    }
    textures_.clear();

    if (whiteTexture_) {
        glDeleteTextures(1, &whiteTexture_);
        whiteTexture_ = 0;
    }

    if (shaderProgram_) {
        glDeleteProgram(shaderProgram_);
        shaderProgram_ = 0;
    }

    initialized_ = false;
}

UIGeometryHandle OpenGLUIRenderBackend::compileGeometry(
    std::span<const UIVertex> vertices,
    std::span<const std::uint32_t> indices)
{
    CompiledGeometry geo;

    glGenVertexArrays(1, &geo.vao);
    glGenBuffers(1, &geo.vbo);
    glGenBuffers(1, &geo.ebo);

    glBindVertexArray(geo.vao);

    // Vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, geo.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(UIVertex),
                 vertices.data(), GL_STATIC_DRAW);

    // Index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geo.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(std::uint32_t),
                 indices.data(), GL_STATIC_DRAW);

    // Position (vec2)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(UIVertex),
                          (void*)offsetof(UIVertex, position));
    glEnableVertexAttribArray(0);

    // Color (4 bytes normalized to float)
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(UIVertex),
                          (void*)offsetof(UIVertex, color));
    glEnableVertexAttribArray(1);

    // TexCoord (vec2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(UIVertex),
                          (void*)offsetof(UIVertex, texCoord));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    geo.indexCount = static_cast<GLsizei>(indices.size());

    UIGeometryHandle handle = nextGeometryHandle_++;
    geometries_[handle] = geo;

    return handle;
}

void OpenGLUIRenderBackend::releaseGeometry(UIGeometryHandle geometry) {
    auto it = geometries_.find(geometry);
    if (it == geometries_.end()) return;

    glDeleteVertexArrays(1, &it->second.vao);
    glDeleteBuffers(1, &it->second.vbo);
    glDeleteBuffers(1, &it->second.ebo);
    geometries_.erase(it);
}

void OpenGLUIRenderBackend::beginUIPass() {
    // Save current state
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &prevBlendSrc_);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &prevBlendDst_);
    prevDepthTest_ = glIsEnabled(GL_DEPTH_TEST);
    prevBlend_ = glIsEnabled(GL_BLEND);
    prevScissor_ = glIsEnabled(GL_SCISSOR_TEST);

    // Setup UI rendering state
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);  // Premultiplied alpha

    // Bind shader and set projection
    glUseProgram(shaderProgram_);
    glUniformMatrix4fv(projectionLoc_, 1, GL_FALSE, glm::value_ptr(projection_));
    glUniform1i(textureLoc_, 0);

    // Reset stats
    drawCalls_ = 0;
    triangles_ = 0;

    // Default scissor off
    glDisable(GL_SCISSOR_TEST);
    scissorEnabled_ = false;
}

void OpenGLUIRenderBackend::renderGeometry(
    UIGeometryHandle geometry,
    Vec2 translation,
    UITextureHandle texture)
{
    auto it = geometries_.find(geometry);
    if (it == geometries_.end()) return;

    const auto& geo = it->second;

    // Set translation
    glUniform2f(translationLoc_, translation.x, translation.y);

    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    if (texture != InvalidUITexture) {
        auto texIt = textures_.find(texture);
        glBindTexture(GL_TEXTURE_2D,
                      texIt != textures_.end() ? texIt->second : whiteTexture_);
    } else {
        glBindTexture(GL_TEXTURE_2D, whiteTexture_);
    }

    // Draw
    glBindVertexArray(geo.vao);
    glDrawElements(GL_TRIANGLES, geo.indexCount, GL_UNSIGNED_INT, nullptr);

    // Stats
    drawCalls_++;
    triangles_ += geo.indexCount / 3;
}

void OpenGLUIRenderBackend::endUIPass() {
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Restore previous state
    if (prevDepthTest_) glEnable(GL_DEPTH_TEST);
    else glDisable(GL_DEPTH_TEST);

    if (prevBlend_) {
        glEnable(GL_BLEND);
        glBlendFunc(prevBlendSrc_, prevBlendDst_);
    } else {
        glDisable(GL_BLEND);
    }

    if (prevScissor_) glEnable(GL_SCISSOR_TEST);
    else glDisable(GL_SCISSOR_TEST);
}

UITextureHandle OpenGLUIRenderBackend::loadTexture(
    const std::filesystem::path& path,
    int& outWidth, int& outHeight)
{
    // TODO: Load via AssetSystem
    // For now, return invalid
    outWidth = 0;
    outHeight = 0;
    return InvalidUITexture;
}

UITextureHandle OpenGLUIRenderBackend::createTexture(
    std::span<const std::uint8_t> data,
    int width, int height)
{
    GLuint texId;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, data.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);

    UITextureHandle handle = nextTextureHandle_++;
    textures_[handle] = texId;

    return handle;
}

void OpenGLUIRenderBackend::releaseTexture(UITextureHandle texture) {
    auto it = textures_.find(texture);
    if (it == textures_.end()) return;

    glDeleteTextures(1, &it->second);
    textures_.erase(it);
}

void OpenGLUIRenderBackend::enableScissor(bool enable) {
    scissorEnabled_ = enable;
    if (enable) {
        glEnable(GL_SCISSOR_TEST);
    } else {
        glDisable(GL_SCISSOR_TEST);
    }
}

void OpenGLUIRenderBackend::setScissorRegion(const UIScissorRect& region) {
    // OpenGL scissor Y is from bottom, UI is from top
    int y = viewportHeight_ - region.y - region.height;
    glScissor(region.x, y, region.width, region.height);
}

void OpenGLUIRenderBackend::setViewportSize(int width, int height) {
    viewportWidth_ = width;
    viewportHeight_ = height;
    projection_ = glm::ortho(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f);
}

Size OpenGLUIRenderBackend::getViewportSize() const {
    return Size{viewportWidth_, viewportHeight_};
}

bool OpenGLUIRenderBackend::createShader() {
    GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertShader, 1, &kVertexShader, nullptr);
    glCompileShader(vertShader);

    GLint success;
    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glDeleteShader(vertShader);
        return false;
    }

    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &kFragmentShader, nullptr);
    glCompileShader(fragShader);

    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glDeleteShader(vertShader);
        glDeleteShader(fragShader);
        return false;
    }

    shaderProgram_ = glCreateProgram();
    glAttachShader(shaderProgram_, vertShader);
    glAttachShader(shaderProgram_, fragShader);
    glLinkProgram(shaderProgram_);

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    glGetProgramiv(shaderProgram_, GL_LINK_STATUS, &success);
    if (!success) {
        glDeleteProgram(shaderProgram_);
        shaderProgram_ = 0;
        return false;
    }

    return true;
}

void OpenGLUIRenderBackend::createWhiteTexture() {
    std::uint8_t white[] = {255, 255, 255, 255};

    glGenTextures(1, &whiteTexture_);
    glBindTexture(GL_TEXTURE_2D, whiteTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
}

}  // namespace bestow
```

### 3.2 Vulkan UI Render Backend

Similar structure to OpenGL, but using Vulkan APIs. Key differences:

- Uses VkPipeline instead of shader program
- Uses VkBuffer + VkDeviceMemory for vertex/index data
- Uses VkDescriptorSet for texture binding
- Records into provided VkCommandBuffer
- Needs VkRenderPass compatibility

**File:** `bestow-vulkan/src/VulkanUIRenderBackend.hpp` and `.cpp`

(Implementation similar to OpenGL but with Vulkan equivalents)

## Phase 4: Graphics System Updates

### 4.1-4.3 Add `getUIRenderBackend()` to Graphics Systems

For each graphics system, add:

```cpp
// In class declaration
private:
    std::unique_ptr<OpenGLUIRenderBackend> uiRenderBackend_;  // or VulkanUIRenderBackend
    bool inFrame_ = false;

public:
    // IGraphicsContext implementation
    IUIRenderBackend* getUIRenderBackend() override {
        if (!uiRenderBackend_) {
            uiRenderBackend_ = std::make_unique<OpenGLUIRenderBackend>();
            uiRenderBackend_->initialize();
            uiRenderBackend_->setViewportSize(windowWidth_, windowHeight_);
        }
        return uiRenderBackend_.get();
    }

    Size getViewportSize() const override {
        return getWindowSize();  // Delegate to existing method
    }

    bool isInFrame() const override {
        return inFrame_;
    }

    void* getRenderContext() const override {
        return nullptr;  // OpenGL doesn't need this
        // For Vulkan: return &vulkanContext_;
    }

    void* getCurrentCommandBuffer() const override {
        return nullptr;  // OpenGL doesn't need this
        // For Vulkan: return currentCommandBuffer_;
    }

    // Update beginFrame/endFrame
    void beginFrame() override {
        inFrame_ = true;
        // ... existing code ...
    }

    void endFrame() override {
        // ... existing code ...
        inFrame_ = false;
    }
```

## Phase 5: UI System Refactor

### 5.1 BestowRmlRenderInterface

**File:** `bestow-ui/src/BestowRmlRenderInterface.cpp`

```cpp
#include <RmlUi/Core.h>
#include <vector>

import bestow.uirender;
import bestow.types;

namespace bestow {

class BestowRmlRenderInterface : public Rml::RenderInterface {
public:
    explicit BestowRmlRenderInterface(IUIRenderBackend& backend)
        : backend_(&backend) {}

    Rml::CompiledGeometryHandle CompileGeometry(
        Rml::Span<const Rml::Vertex> vertices,
        Rml::Span<const int> indices) override
    {
        // Convert vertices
        std::vector<UIVertex> uiVerts;
        uiVerts.reserve(vertices.size());
        for (const auto& v : vertices) {
            uiVerts.push_back(UIVertex{
                .position = {v.position.x, v.position.y},
                .color = Color(v.colour.red, v.colour.green,
                              v.colour.blue, v.colour.alpha),
                .texCoord = {v.tex_coord.x, v.tex_coord.y}
            });
        }

        // Convert indices
        std::vector<std::uint32_t> uiIndices(indices.begin(), indices.end());

        UIGeometryHandle handle = backend_->compileGeometry(uiVerts, uiIndices);
        return static_cast<Rml::CompiledGeometryHandle>(handle);
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

    void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override {
        backend_->releaseGeometry(static_cast<UIGeometryHandle>(geometry));
    }

    Rml::TextureHandle LoadTexture(
        Rml::Vector2i& texture_dimensions,
        const Rml::String& source) override
    {
        int width, height;
        UITextureHandle handle = backend_->loadTexture(source, width, height);
        texture_dimensions.x = width;
        texture_dimensions.y = height;
        return static_cast<Rml::TextureHandle>(handle);
    }

    Rml::TextureHandle GenerateTexture(
        Rml::Span<const Rml::byte> source,
        Rml::Vector2i dimensions) override
    {
        std::span<const std::uint8_t> data(source.data(), source.size());
        UITextureHandle handle = backend_->createTexture(
            data, dimensions.x, dimensions.y);
        return static_cast<Rml::TextureHandle>(handle);
    }

    void ReleaseTexture(Rml::TextureHandle texture) override {
        backend_->releaseTexture(static_cast<UITextureHandle>(texture));
    }

    void EnableScissorRegion(bool enable) override {
        backend_->enableScissor(enable);
    }

    void SetScissorRegion(Rml::Rectanglei region) override {
        backend_->setScissorRegion(UIScissorRect{
            region.Left(), region.Top(),
            region.Width(), region.Height()
        });
    }

private:
    IUIRenderBackend* backend_;
};

}  // namespace bestow
```

### 5.2 BestowRmlSystemInterface

**File:** `bestow-ui/src/BestowRmlSystemInterface.cpp`

```cpp
#include <RmlUi/Core.h>
#include <chrono>

namespace bestow {

class BestowRmlSystemInterface : public Rml::SystemInterface {
public:
    double GetElapsedTime() override {
        using namespace std::chrono;
        static auto start = steady_clock::now();
        auto now = steady_clock::now();
        return duration<double>(now - start).count();
    }

    bool LogMessage(Rml::Log::Type type, const Rml::String& message) override {
        // TODO: Integrate with spdlog
        return true;
    }
};

}  // namespace bestow
```

### 5.3 RmlUISystem Refactor

**File:** `bestow-ui/src/bestow.ui.impl.cppm`

Key changes:
1. Remove GLFW includes
2. Remove GLFWwindow* member
3. Add IGraphicsContext* member
4. Get IUIRenderBackend from graphics context
5. Create Rml interfaces wrapping the backend

```cpp
class RmlUISystem : public IUISystem {
public:
    // Constructor injection - receives IGraphicsContext
    explicit RmlUISystem(IGraphicsContext& graphics, IAssetSystem& assets)
        : graphics_(&graphics), assetSystem_(&assets) {}

    Result<void, UIError> initialize(const UIConfig& config) override {
        // Get render backend from graphics
        renderBackend_ = graphics_->getUIRenderBackend();
        if (!renderBackend_) {
            return std::unexpected(UIError::InternalError);
        }

        // Create RmlUi interfaces
        rmlRenderInterface_ = std::make_unique<BestowRmlRenderInterface>(*renderBackend_);
        rmlSystemInterface_ = std::make_unique<BestowRmlSystemInterface>();

        // Initialize RmlUi
        Rml::SetRenderInterface(rmlRenderInterface_.get());
        Rml::SetSystemInterface(rmlSystemInterface_.get());
        Rml::Initialise();

        // Create context
        auto viewportSize = graphics_->getViewportSize();
        context_ = Rml::CreateContext("main",
            Rml::Vector2i(viewportSize.width, viewportSize.height));

        if (!context_) {
            return std::unexpected(UIError::InternalError);
        }

        // ... rest of initialization
        return {};
    }

    void render() override {
        if (!context_ || !renderBackend_) return;

        renderBackend_->beginUIPass();
        context_->Render();
        renderBackend_->endUIPass();
    }

    void setViewportSize(int width, int height) override {
        if (context_) {
            context_->SetDimensions(Rml::Vector2i(width, height));
        }
        if (renderBackend_) {
            renderBackend_->setViewportSize(width, height);
        }
    }

private:
    IGraphicsContext* graphics_ = nullptr;
    IAssetSystem* assetSystem_ = nullptr;
    IUIRenderBackend* renderBackend_ = nullptr;

    std::unique_ptr<BestowRmlRenderInterface> rmlRenderInterface_;
    std::unique_ptr<BestowRmlSystemInterface> rmlSystemInterface_;
    Rml::Context* context_ = nullptr;

    // ... rest of existing members (documents_, elements_, etc.)
};
```

## Phase 6: Testing

### 6.1 Snake Game Test

Update snake game to use UI system:

```cpp
// In snake initialization
uiSystem_->initialize({});

// Load HUD document
auto hudResult = uiSystem_->loadDocument(":assets:/ui/hud.rml");
if (hudResult) {
    uiSystem_->showDocument(*hudResult);
}

// In render loop
graphics_->beginFrame();
// ... game rendering ...
uiSystem_->render();  // UI on top
graphics_->endFrame();
```

### 6.2 Create Simple HUD Document

**File:** `assets/ui/hud.rml`

```xml
<rml>
<head>
    <style>
        body {
            font-family: LatoLatin;
            font-size: 16px;
            color: white;
        }
        #score {
            position: absolute;
            top: 10px;
            left: 10px;
        }
        #level {
            position: absolute;
            top: 10px;
            right: 10px;
        }
    </style>
</head>
<body>
    <div id="score">Score: <span data-value="score">0</span></div>
    <div id="level">Level: <span data-value="level">1</span></div>
</body>
</rml>
```

### 6.3 Verification Checklist

- [ ] RmlUISystem compiles with no GLFW/OpenGL/Vulkan includes
- [ ] Snake game renders UI correctly
- [ ] Score/level display updates via data binding
- [ ] Scissor clipping works (nested elements)
- [ ] Font rendering works
- [ ] 2D OpenGL example still works
- [ ] Hot reload of .rml files works
