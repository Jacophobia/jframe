// bestow-opengl/src/OpenGLUIRenderBackend.cpp
// OpenGL implementation of IUIRenderBackend for 2D UI rendering

module;

// Include OpenGL headers in global module fragment
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stb_image.h>

#include <spdlog/spdlog.h>

module bestow.opengl.impl;

import std;
import bestow.services;

namespace bestow {

//==========================================================================
// Shader Sources
//==========================================================================

static constexpr const char* UI_VERTEX_SHADER = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTexCoord;

out vec4 vColor;
out vec2 vTexCoord;

uniform mat4 uProjection;
uniform vec2 uTranslation;

void main() {
    vec2 pos = aPos + uTranslation;
    gl_Position = uProjection * vec4(pos, 0.0, 1.0);
    vColor = aColor;
    vTexCoord = aTexCoord;
}
)";

static constexpr const char* UI_FRAGMENT_SHADER = R"(
#version 330 core
in vec4 vColor;
in vec2 vTexCoord;

out vec4 FragColor;

uniform sampler2D uTexture;
uniform bool uHasTexture;

void main() {
    vec4 texColor = uHasTexture ? texture(uTexture, vTexCoord) : vec4(1.0);
    // Convert from straight alpha to premultiplied alpha
    vec4 color = vColor * texColor;
    color.rgb *= color.a;
    FragColor = color;
}
)";

//==========================================================================
// OpenGLUIRenderBackend Implementation
//==========================================================================

OpenGLUIRenderBackend::OpenGLUIRenderBackend() = default;

OpenGLUIRenderBackend::~OpenGLUIRenderBackend() {
    shutdown();
}

bool OpenGLUIRenderBackend::initialize() {
    if (initialized_) {
        return true;
    }

    // Compile shaders
    shaderProgram_ = createShaderProgram(UI_VERTEX_SHADER, UI_FRAGMENT_SHADER);
    if (shaderProgram_ == 0) {
        spdlog::error("OpenGLUIRenderBackend: Failed to create shader program");
        return false;
    }

    // Get uniform locations
    projectionLoc_ = glGetUniformLocation(shaderProgram_, "uProjection");
    translationLoc_ = glGetUniformLocation(shaderProgram_, "uTranslation");
    hasTextureLoc_ = glGetUniformLocation(shaderProgram_, "uHasTexture");
    textureLoc_ = glGetUniformLocation(shaderProgram_, "uTexture");

    // Create white texture for solid color rendering
    createWhiteTexture();

    initialized_ = true;
    spdlog::info("OpenGLUIRenderBackend: Initialized successfully");
    return true;
}

void OpenGLUIRenderBackend::shutdown() {
    if (!initialized_) {
        return;
    }

    // Release all geometry
    for (auto& [handle, geom] : geometryCache_) {
        glDeleteVertexArrays(1, &geom.vao);
        glDeleteBuffers(1, &geom.vbo);
        glDeleteBuffers(1, &geom.ebo);
    }
    geometryCache_.clear();

    // Release all textures
    for (auto& [handle, texId] : textureCache_) {
        glDeleteTextures(1, &texId);
    }
    textureCache_.clear();

    // Delete white texture
    if (whiteTexture_ != 0) {
        glDeleteTextures(1, &whiteTexture_);
        whiteTexture_ = 0;
    }

    // Delete shader program
    if (shaderProgram_ != 0) {
        glDeleteProgram(shaderProgram_);
        shaderProgram_ = 0;
    }

    initialized_ = false;
    spdlog::info("OpenGLUIRenderBackend: Shutdown complete");
}

bool OpenGLUIRenderBackend::isInitialized() const {
    return initialized_;
}

//==========================================================================
// Geometry Management
//==========================================================================

UIGeometryHandle OpenGLUIRenderBackend::compileGeometry(
    std::span<const UIVertex> vertices,
    std::span<const std::uint32_t> indices) {

    if (!initialized_ || vertices.empty() || indices.empty()) {
        return InvalidUIGeometry;
    }

    GeometryResource geom{};
    geom.indexCount = static_cast<std::uint32_t>(indices.size());

    // Create VAO
    glGenVertexArrays(1, &geom.vao);
    glBindVertexArray(geom.vao);

    // Create VBO for vertices
    glGenBuffers(1, &geom.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, geom.vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(UIVertex),
                 vertices.data(),
                 GL_STATIC_DRAW);

    // Create EBO for indices
    glGenBuffers(1, &geom.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geom.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(std::uint32_t),
                 indices.data(),
                 GL_STATIC_DRAW);

    // Set up vertex attributes
    // Position (Vec2)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(UIVertex),
                          reinterpret_cast<void*>(offsetof(UIVertex, position)));
    glEnableVertexAttribArray(0);

    // Color (4 bytes as normalized ubyte)
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(UIVertex),
                          reinterpret_cast<void*>(offsetof(UIVertex, color)));
    glEnableVertexAttribArray(1);

    // TexCoord (Vec2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(UIVertex),
                          reinterpret_cast<void*>(offsetof(UIVertex, texCoord)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    // Store and return handle
    UIGeometryHandle handle = nextGeometryHandle_++;
    geometryCache_[handle] = geom;
    return handle;
}

void OpenGLUIRenderBackend::releaseGeometry(UIGeometryHandle geometry) {
    auto it = geometryCache_.find(geometry);
    if (it != geometryCache_.end()) {
        glDeleteVertexArrays(1, &it->second.vao);
        glDeleteBuffers(1, &it->second.vbo);
        glDeleteBuffers(1, &it->second.ebo);
        geometryCache_.erase(it);
    }
}

//==========================================================================
// Rendering
//==========================================================================

void OpenGLUIRenderBackend::beginUIPass() {
    if (!initialized_) {
        return;
    }

    // Save current OpenGL state
    glGetIntegerv(GL_BLEND_SRC_RGB, &savedBlendSrcRGB_);
    glGetIntegerv(GL_BLEND_DST_RGB, &savedBlendDstRGB_);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &savedBlendSrcAlpha_);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &savedBlendDstAlpha_);
    savedBlendEnabled_ = glIsEnabled(GL_BLEND);
    savedDepthTestEnabled_ = glIsEnabled(GL_DEPTH_TEST);
    savedScissorEnabled_ = glIsEnabled(GL_SCISSOR_TEST);
    savedCullFaceEnabled_ = glIsEnabled(GL_CULL_FACE);

    // Set up UI rendering state
    glEnable(GL_BLEND);
    // Premultiplied alpha blending
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // Use UI shader
    glUseProgram(shaderProgram_);

    // Set orthographic projection
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(viewportWidth_),
                                       static_cast<float>(viewportHeight_), 0.0f,
                                       -1.0f, 1.0f);
    glUniformMatrix4fv(projectionLoc_, 1, GL_FALSE, glm::value_ptr(projection));

    // Reset statistics
    drawCallCount_ = 0;
    triangleCount_ = 0;

    inUIPass_ = true;
}

void OpenGLUIRenderBackend::renderGeometry(
    UIGeometryHandle geometry,
    Vec2 translation,
    UITextureHandle texture) {

    if (!initialized_ || !inUIPass_) {
        return;
    }

    auto it = geometryCache_.find(geometry);
    if (it == geometryCache_.end()) {
        return;
    }

    const auto& geom = it->second;

    // Set translation uniform
    glUniform2f(translationLoc_, translation.x, translation.y);

    // Bind texture
    GLuint texId = whiteTexture_;
    bool hasTexture = false;
    if (texture != InvalidUITexture) {
        auto texIt = textureCache_.find(texture);
        if (texIt != textureCache_.end()) {
            texId = texIt->second;
            hasTexture = true;
        }
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texId);
    glUniform1i(textureLoc_, 0);
    glUniform1i(hasTextureLoc_, hasTexture ? 1 : 0);

    // Draw
    glBindVertexArray(geom.vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(geom.indexCount), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    // Update statistics
    drawCallCount_++;
    triangleCount_ += geom.indexCount / 3;
}

void OpenGLUIRenderBackend::endUIPass() {
    if (!inUIPass_) {
        return;
    }

    // Restore OpenGL state
    if (savedBlendEnabled_) {
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }
    glBlendFuncSeparate(savedBlendSrcRGB_, savedBlendDstRGB_,
                         savedBlendSrcAlpha_, savedBlendDstAlpha_);

    if (savedDepthTestEnabled_) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }

    if (savedScissorEnabled_) {
        glEnable(GL_SCISSOR_TEST);
    } else {
        glDisable(GL_SCISSOR_TEST);
    }

    if (savedCullFaceEnabled_) {
        glEnable(GL_CULL_FACE);
    } else {
        glDisable(GL_CULL_FACE);
    }

    glUseProgram(0);
    inUIPass_ = false;
}

//==========================================================================
// Texture Management
//==========================================================================

UITextureHandle OpenGLUIRenderBackend::loadTexture(
    const std::filesystem::path& path,
    int& outWidth,
    int& outHeight) {

    if (!initialized_) {
        return InvalidUITexture;
    }

    // Load image using stb_image
    int width, height, channels;
    stbi_set_flip_vertically_on_load(false);  // UI textures typically don't need flipping
    unsigned char* data = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
    if (!data) {
        spdlog::error("OpenGLUIRenderBackend: Failed to load texture: {}", path.string());
        return InvalidUITexture;
    }

    outWidth = width;
    outHeight = height;

    // Create OpenGL texture
    GLuint texId;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);

    // Store and return handle
    UITextureHandle handle = nextTextureHandle_++;
    textureCache_[handle] = texId;
    textureDimensions_[handle] = {width, height};
    return handle;
}

UITextureHandle OpenGLUIRenderBackend::createTexture(
    std::span<const std::uint8_t> data,
    int width,
    int height) {

    if (!initialized_ || data.empty() || width <= 0 || height <= 0) {
        return InvalidUITexture;
    }

    // Validate data size
    if (data.size() < static_cast<size_t>(width * height * 4)) {
        spdlog::error("OpenGLUIRenderBackend: Texture data size mismatch");
        return InvalidUITexture;
    }

    // Create OpenGL texture
    GLuint texId;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Store and return handle
    UITextureHandle handle = nextTextureHandle_++;
    textureCache_[handle] = texId;
    textureDimensions_[handle] = {width, height};
    return handle;
}

void OpenGLUIRenderBackend::releaseTexture(UITextureHandle texture) {
    auto it = textureCache_.find(texture);
    if (it != textureCache_.end()) {
        glDeleteTextures(1, &it->second);
        textureCache_.erase(it);
        textureDimensions_.erase(texture);
    }
}

//==========================================================================
// Scissor (Clipping)
//==========================================================================

void OpenGLUIRenderBackend::enableScissor(bool enable) {
    if (enable) {
        glEnable(GL_SCISSOR_TEST);
    } else {
        glDisable(GL_SCISSOR_TEST);
    }
}

void OpenGLUIRenderBackend::setScissorRegion(const UIScissorRect& region) {
    // OpenGL scissor origin is bottom-left, but UI is top-left
    // Convert Y coordinate
    int flippedY = viewportHeight_ - (region.y + region.height);
    glScissor(region.x, flippedY, region.width, region.height);
}

//==========================================================================
// Viewport
//==========================================================================

void OpenGLUIRenderBackend::setViewportSize(int width, int height) {
    viewportWidth_ = width;
    viewportHeight_ = height;
}

Size OpenGLUIRenderBackend::getViewportSize() const {
    return Size{viewportWidth_, viewportHeight_};
}

//==========================================================================
// Statistics
//==========================================================================

std::uint32_t OpenGLUIRenderBackend::getDrawCallCount() const {
    return drawCallCount_;
}

std::uint32_t OpenGLUIRenderBackend::getTriangleCount() const {
    return triangleCount_;
}

//==========================================================================
// Private Helpers
//==========================================================================

GLuint OpenGLUIRenderBackend::createShaderProgram(const char* vertSource, const char* fragSource) {
    // Compile vertex shader
    GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertShader, 1, &vertSource, nullptr);
    glCompileShader(vertShader);

    GLint success;
    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertShader, 512, nullptr, infoLog);
        spdlog::error("UI Vertex shader compilation failed: {}", infoLog);
        glDeleteShader(vertShader);
        return 0;
    }

    // Compile fragment shader
    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &fragSource, nullptr);
    glCompileShader(fragShader);

    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragShader, 512, nullptr, infoLog);
        spdlog::error("UI Fragment shader compilation failed: {}", infoLog);
        glDeleteShader(vertShader);
        glDeleteShader(fragShader);
        return 0;
    }

    // Link program
    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        spdlog::error("UI Shader program linking failed: {}", infoLog);
        glDeleteShader(vertShader);
        glDeleteShader(fragShader);
        glDeleteProgram(program);
        return 0;
    }

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    return program;
}

void OpenGLUIRenderBackend::createWhiteTexture() {
    std::uint8_t whitePixel[4] = {255, 255, 255, 255};
    glGenTextures(1, &whiteTexture_);
    glBindTexture(GL_TEXTURE_2D, whiteTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
}

}  // namespace bestow
