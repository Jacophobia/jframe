// bestow-opengl/src/GraphicsSystem.cpp
// Graphics system implementation

module;

#include <algorithm>
#include <any>
#include <cmath>
#include <span>
#include <string>
#include <vector>
#include <cstring>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

// MSVC C++23 module compatibility for EnTT iterators used in renderEntities()
#include <bestow/entt_compat.hpp>

module bestow.opengl.impl;

import bestow.services;  // Re-exports all contracts
import bestow.utils;

namespace bestow {

OpenGLGraphicsSystem::~OpenGLGraphicsSystem() {
    // Clean up texture cache
    for (auto& [handle, textureId] : textureCache_) {
        if (textureId) glDeleteTextures(1, &textureId);
    }
    textureCache_.clear();

    // Clean up OpenGL resources
    if (spriteVBO_) glDeleteBuffers(1, &spriteVBO_);
    if (spriteVAO_) glDeleteVertexArrays(1, &spriteVAO_);
    if (spriteShaderProgram_) glDeleteProgram(spriteShaderProgram_);
    if (whiteTexture_) glDeleteTextures(1, &whiteTexture_);

    // Clean up debug primitive resources
    if (primitiveVBO_) glDeleteBuffers(1, &primitiveVBO_);
    if (primitiveVAO_) glDeleteVertexArrays(1, &primitiveVAO_);
    if (primitiveShaderProgram_) glDeleteProgram(primitiveShaderProgram_);

    // Clean up text rendering resources
    if (textVBO_) glDeleteBuffers(1, &textVBO_);
    if (textVAO_) glDeleteVertexArrays(1, &textVAO_);
    if (textShaderProgram_) glDeleteProgram(textShaderProgram_);
    if (defaultFontAtlas_.textureID) glDeleteTextures(1, &defaultFontAtlas_.textureID);
    for (auto& [handle, atlas] : fontAtlases_) {
        if (atlas.textureID) glDeleteTextures(1, &atlas.textureID);
    }

    if (window_) {
        glfwDestroyWindow(window_);
    }
    glfwTerminate();
}

bool OpenGLGraphicsSystem::initialize(int width, int height, const std::string& title) {
    if (!glfwInit()) {
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    window_ = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!window_) {
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);

    // Initialize glad
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        glfwDestroyWindow(window_);
        glfwTerminate();
        return false;
    }

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    windowedWidth_ = width;
    windowedHeight_ = height;

    camera_.viewportSize = {width, height};

    // Initialize rendering resources
    createWhiteTexture();

    // Create sprite shader
    const char* vertexShaderSource = R"(
        #version 410 core
        layout (location = 0) in vec2 aPosition;
        layout (location = 1) in vec2 aTexCoord;
        out vec2 vTexCoord;
        uniform mat4 uModel;
        uniform mat4 uViewProj;
        void main() {
            gl_Position = uViewProj * uModel * vec4(aPosition, 0.0, 1.0);
            vTexCoord = aTexCoord;
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 410 core
        in vec2 vTexCoord;
        out vec4 FragColor;
        uniform sampler2D uTexture;
        uniform vec4 uTint;
        void main() {
            vec4 texColor = texture(uTexture, vTexCoord);
            FragColor = texColor * uTint;
        }
    )";

    spriteShaderProgram_ = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    if (spriteShaderProgram_ == 0) {
        return false;
    }

    // Create sprite quad VAO/VBO
    // Vertex format: position (2 floats) + texcoord (2 floats)
    float vertices[] = {
        // positions   // texcoords
        0.0f, 1.0f,    0.0f, 1.0f,  // top-left
        1.0f, 1.0f,    1.0f, 1.0f,  // top-right
        0.0f, 0.0f,    0.0f, 0.0f,  // bottom-left

        1.0f, 1.0f,    1.0f, 1.0f,  // top-right
        1.0f, 0.0f,    1.0f, 0.0f,  // bottom-right
        0.0f, 0.0f,    0.0f, 0.0f   // bottom-left
    };

    glGenVertexArrays(1, &spriteVAO_);
    glGenBuffers(1, &spriteVBO_);

    glBindVertexArray(spriteVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, spriteVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    // Texcoord attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Initialize debug primitive resources
    createPrimitiveResources();

    // Initialize text rendering resources
    createTextResources();
    createDefaultFont();

    return true;
}

void OpenGLGraphicsSystem::beginFrame() {
    inFrame_ = true;
    glClearColor(
        clearColor_.r / 255.0f,
        clearColor_.g / 255.0f,
        clearColor_.b / 255.0f,
        clearColor_.a / 255.0f
    );
    glClear(GL_COLOR_BUFFER_BIT);
    spriteBatch_.clear();
}

void OpenGLGraphicsSystem::endFrame() {
    if (spriteBatch_.empty()) {
        glFlush();  // Ensure clear command completes before swap
        inFrame_ = false;
        glfwSwapBuffers(window_);
        glfwPollEvents();
        return;
    }

    // Sort sprite batch by layer
    std::sort(spriteBatch_.begin(), spriteBatch_.end(),
        [](const Sprite& a, const Sprite& b) {
            return a.layer < b.layer;
        });

    // Calculate view-projection matrix
    Vec2 camPos = camera_.transform.position();
    float halfWidth = camera_.viewportSize.width / (2.0f * camera_.zoom);
    float halfHeight = camera_.viewportSize.height / (2.0f * camera_.zoom);

    glm::mat4 projection = glm::ortho(
        camPos.x - halfWidth,
        camPos.x + halfWidth,
        camPos.y + halfHeight,  // bottom (Y-down screen coords)
        camPos.y - halfHeight,  // top
        -1.0f,
        1.0f
    );

    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 viewProj = projection * view;

    // Use sprite shader
    glUseProgram(spriteShaderProgram_);
    glBindVertexArray(spriteVAO_);

    // Set view-projection uniform
    GLint viewProjLoc = glGetUniformLocation(spriteShaderProgram_, "uViewProj");
    glUniformMatrix4fv(viewProjLoc, 1, GL_FALSE, &viewProj[0][0]);

    // Get uniform locations
    GLint modelLoc = glGetUniformLocation(spriteShaderProgram_, "uModel");
    GLint tintLoc = glGetUniformLocation(spriteShaderProgram_, "uTint");

    // Render each sprite
    for (const Sprite& sprite : spriteBatch_) {
        // Extract sprite dimensions from sourceRect (or use transform scale if sourceRect is empty)
        float width = sprite.sourceRect.size.width > 0 ? sprite.sourceRect.size.width : 100.0f;
        float height = sprite.sourceRect.size.height > 0 ? sprite.sourceRect.size.height : 100.0f;

        // Apply transform scale
        width *= sprite.transform.scaleX;
        height *= sprite.transform.scaleY;

        // Build model matrix
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(sprite.transform.x, sprite.transform.y, 0.0f));
        model = glm::rotate(model, sprite.transform.rotation, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(width, height, 1.0f));

        // Apply anchor offset (sprites anchor at center by default in the shader's unit quad)
        model = glm::translate(model, glm::vec3(-sprite.anchor.x, -sprite.anchor.y, 0.0f));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);

        // Set tint color
        glm::vec4 tint(
            sprite.tint.r / 255.0f,
            sprite.tint.g / 255.0f,
            sprite.tint.b / 255.0f,
            sprite.tint.a / 255.0f
        );
        glUniform4fv(tintLoc, 1, &tint[0]);

        // Bind texture - use actual texture if available, otherwise fallback to white
        GLuint textureId = whiteTexture_;
        if (sprite.textureHandle) {
            textureId = getOrUploadTexture(*sprite.textureHandle);
        }
        glBindTexture(GL_TEXTURE_2D, textureId);

        // Draw quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    glBindVertexArray(0);
    glUseProgram(0);

    inFrame_ = false;
    glfwSwapBuffers(window_);
    glfwPollEvents();
}

void OpenGLGraphicsSystem::draw(const Sprite& sprite) {
    spriteBatch_.push_back(sprite);
}

void OpenGLGraphicsSystem::drawBatch(std::span<const Sprite> sprites) {
    spriteBatch_.insert(spriteBatch_.end(), sprites.begin(), sprites.end());
}

void OpenGLGraphicsSystem::drawSprite(const SpriteSheet& sheet, int frameIndex,
                                 const Transform2D& transform, Color tint) {
    // Get the frame rectangle from the spritesheet
    Canvas frameRect = sheet.getFrameRect(frameIndex);

    // Create a Sprite object with the calculated frame rectangle
    Sprite sprite;
    sprite.textureHandle = const_cast<AssetHandle*>(&sheet.texture);
    sprite.sourceRect = frameRect;
    sprite.transform = transform;
    sprite.tint = tint;
    sprite.layer = 0;
    sprite.anchor = {0.5f, 0.5f};

    // Add to the batch
    spriteBatch_.push_back(sprite);
}

void OpenGLGraphicsSystem::drawAnimatedSprite(AnimatedSprite& sprite,
                                        const Transform2D& transform, Color tint) {
    // Get the current frame index from the animated sprite
    int frameIndex = sprite.getCurrentFrame();

    // Draw using the spritesheet
    drawSprite(sprite.sheet, frameIndex, transform, tint);
}

void OpenGLGraphicsSystem::drawRect(const Canvas& rect, const Color& color, bool filled) {
    // Calculate view-projection matrix
    Vec2 camPos = camera_.transform.position();
    float halfWidth = camera_.viewportSize.width / (2.0f * camera_.zoom);
    float halfHeight = camera_.viewportSize.height / (2.0f * camera_.zoom);

    glm::mat4 projection = glm::ortho(
        camPos.x - halfWidth,
        camPos.x + halfWidth,
        camPos.y + halfHeight,  // bottom (Y-down screen coords)
        camPos.y - halfHeight,  // top
        -1.0f,
        1.0f
    );

    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 viewProj = projection * view;

    // Set up primitive shader
    glUseProgram(primitiveShaderProgram_);
    GLint viewProjLoc = glGetUniformLocation(primitiveShaderProgram_, "uViewProj");
    glUniformMatrix4fv(viewProjLoc, 1, GL_FALSE, &viewProj[0][0]);

    GLint colorLoc = glGetUniformLocation(primitiveShaderProgram_, "uColor");
    glm::vec4 glColor(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
    glUniform4fv(colorLoc, 1, &glColor[0]);

    // Create rectangle vertices
    float x = static_cast<float>(rect.origin.x);
    float y = static_cast<float>(rect.origin.y);
    float w = static_cast<float>(rect.size.width);
    float h = static_cast<float>(rect.size.height);

    if (filled) {
        // Two triangles for filled rect
        float vertices[] = {
            x, y,           // bottom-left
            x + w, y,       // bottom-right
            x, y + h,       // top-left
            x + w, y,       // bottom-right
            x + w, y + h,   // top-right
            x, y + h        // top-left
        };

        glBindVertexArray(primitiveVAO_);
        glBindBuffer(GL_ARRAY_BUFFER, primitiveVBO_);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    } else {
        // Line loop for outline
        float vertices[] = {
            x, y,           // bottom-left
            x + w, y,       // bottom-right
            x + w, y + h,   // top-right
            x, y + h        // top-left
        };

        glBindVertexArray(primitiveVAO_);
        glBindBuffer(GL_ARRAY_BUFFER, primitiveVBO_);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
        glDrawArrays(GL_LINE_LOOP, 0, 4);
    }

    glBindVertexArray(0);
    glUseProgram(0);
}

void OpenGLGraphicsSystem::drawLine(Vec2 from, Vec2 to, const Color& color, float thickness) {
    // Calculate view-projection matrix
    Vec2 camPos = camera_.transform.position();
    float halfWidth = camera_.viewportSize.width / (2.0f * camera_.zoom);
    float halfHeight = camera_.viewportSize.height / (2.0f * camera_.zoom);

    glm::mat4 projection = glm::ortho(
        camPos.x - halfWidth,
        camPos.x + halfWidth,
        camPos.y + halfHeight,  // bottom (Y-down screen coords)
        camPos.y - halfHeight,  // top
        -1.0f,
        1.0f
    );

    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 viewProj = projection * view;

    // Set up primitive shader
    glUseProgram(primitiveShaderProgram_);
    GLint viewProjLoc = glGetUniformLocation(primitiveShaderProgram_, "uViewProj");
    glUniformMatrix4fv(viewProjLoc, 1, GL_FALSE, &viewProj[0][0]);

    GLint colorLoc = glGetUniformLocation(primitiveShaderProgram_, "uColor");
    glm::vec4 glColor(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
    glUniform4fv(colorLoc, 1, &glColor[0]);

    // Create line vertices
    float vertices[] = {
        from.x, from.y,
        to.x, to.y
    };

    glBindVertexArray(primitiveVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, primitiveVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    // Set line width if thickness is specified (note: thick lines may not be supported on all platforms)
    if (thickness > 1.0f) {
        glLineWidth(thickness);
    }

    glDrawArrays(GL_LINES, 0, 2);

    // Reset line width
    if (thickness > 1.0f) {
        glLineWidth(1.0f);
    }

    glBindVertexArray(0);
    glUseProgram(0);
}

void OpenGLGraphicsSystem::drawCircle(Vec2 center, float radius, const Color& color,
                                 bool filled, int segments) {
    // Calculate view-projection matrix
    Vec2 camPos = camera_.transform.position();
    float halfWidth = camera_.viewportSize.width / (2.0f * camera_.zoom);
    float halfHeight = camera_.viewportSize.height / (2.0f * camera_.zoom);

    glm::mat4 projection = glm::ortho(
        camPos.x - halfWidth,
        camPos.x + halfWidth,
        camPos.y + halfHeight,  // bottom (Y-down screen coords)
        camPos.y - halfHeight,  // top
        -1.0f,
        1.0f
    );

    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 viewProj = projection * view;

    // Set up primitive shader
    glUseProgram(primitiveShaderProgram_);
    GLint viewProjLoc = glGetUniformLocation(primitiveShaderProgram_, "uViewProj");
    glUniformMatrix4fv(viewProjLoc, 1, GL_FALSE, &viewProj[0][0]);

    GLint colorLoc = glGetUniformLocation(primitiveShaderProgram_, "uColor");
    glm::vec4 glColor(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
    glUniform4fv(colorLoc, 1, &glColor[0]);

    // Generate circle vertices
    std::vector<float> vertices;
    vertices.reserve(segments * 2);

    for (int i = 0; i < segments; ++i) {
        float angle = (core::Math::TWO_PI * i) / segments;
        vertices.push_back(center.x + radius * std::cos(angle));
        vertices.push_back(center.y + radius * std::sin(angle));
    }

    glBindVertexArray(primitiveVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, primitiveVBO_);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

    if (filled) {
        glDrawArrays(GL_TRIANGLE_FAN, 0, segments);
    } else {
        glDrawArrays(GL_LINE_LOOP, 0, segments);
    }

    glBindVertexArray(0);
    glUseProgram(0);
}

void OpenGLGraphicsSystem::drawPolygon(std::span<const Vec2> vertices,
                                  const Color& color, bool filled) {
    if (vertices.empty()) return;

    // Calculate view-projection matrix
    Vec2 camPos = camera_.transform.position();
    float halfWidth = camera_.viewportSize.width / (2.0f * camera_.zoom);
    float halfHeight = camera_.viewportSize.height / (2.0f * camera_.zoom);

    glm::mat4 projection = glm::ortho(
        camPos.x - halfWidth,
        camPos.x + halfWidth,
        camPos.y + halfHeight,  // bottom (Y-down screen coords)
        camPos.y - halfHeight,  // top
        -1.0f,
        1.0f
    );

    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 viewProj = projection * view;

    // Set up primitive shader
    glUseProgram(primitiveShaderProgram_);
    GLint viewProjLoc = glGetUniformLocation(primitiveShaderProgram_, "uViewProj");
    glUniformMatrix4fv(viewProjLoc, 1, GL_FALSE, &viewProj[0][0]);

    GLint colorLoc = glGetUniformLocation(primitiveShaderProgram_, "uColor");
    glm::vec4 glColor(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
    glUniform4fv(colorLoc, 1, &glColor[0]);

    // Convert Vec2 span to float array
    std::vector<float> vertexData;
    vertexData.reserve(vertices.size() * 2);
    for (const Vec2& v : vertices) {
        vertexData.push_back(v.x);
        vertexData.push_back(v.y);
    }

    glBindVertexArray(primitiveVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, primitiveVBO_);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_DYNAMIC_DRAW);

    if (filled) {
        glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<GLsizei>(vertices.size()));
    } else {
        glDrawArrays(GL_LINE_LOOP, 0, static_cast<GLsizei>(vertices.size()));
    }

    glBindVertexArray(0);
    glUseProgram(0);
}

void OpenGLGraphicsSystem::drawText(const std::string& text, Vec2 position,
                               AssetHandle fontHandle, float size,
                               const Color& color) {
    if (text.empty()) return;

    // Get font atlas (default if handle is invalid)
    const FontAtlas& atlas = getFontAtlas(fontHandle, size);
    if (atlas.textureID == 0) return;

    // Calculate scale factor based on desired size vs atlas size
    float scale = size / atlas.fontSize;

    // Calculate view-projection matrix
    Vec2 camPos = camera_.transform.position();
    float halfWidth = camera_.viewportSize.width / (2.0f * camera_.zoom);
    float halfHeight = camera_.viewportSize.height / (2.0f * camera_.zoom);

    glm::mat4 projection = glm::ortho(
        camPos.x - halfWidth,
        camPos.x + halfWidth,
        camPos.y + halfHeight,  // bottom (Y-down screen coords)
        camPos.y - halfHeight,  // top
        -1.0f,
        1.0f
    );

    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 viewProj = projection * view;

    // Use text shader
    glUseProgram(textShaderProgram_);
    glBindVertexArray(textVAO_);
    glBindTexture(GL_TEXTURE_2D, atlas.textureID);

    // Set uniforms
    GLint viewProjLoc = glGetUniformLocation(textShaderProgram_, "uViewProj");
    glUniformMatrix4fv(viewProjLoc, 1, GL_FALSE, &viewProj[0][0]);

    GLint colorLoc = glGetUniformLocation(textShaderProgram_, "uColor");
    glm::vec4 glColor(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
    glUniform4fv(colorLoc, 1, &glColor[0]);

    // Render each character
    float x = position.x;
    float y = position.y;

    for (char c : text) {
        unsigned char uc = static_cast<unsigned char>(c);

        const GlyphInfo& glyph = atlas.glyphs[uc];

        float xpos = x + glyph.bearingX * scale;
        float ypos = y - (glyph.height - glyph.bearingY) * scale;
        float w = glyph.width * scale;
        float h = glyph.height * scale;

        // Create quad vertices (position + texcoord)
        // Texture Y coords are flipped: bottom of quad gets top of texture
        float vertices[6][4] = {
            { xpos,     ypos + h,   glyph.texCoordX,                           glyph.texCoordY + glyph.texCoordH },
            { xpos,     ypos,       glyph.texCoordX,                           glyph.texCoordY },
            { xpos + w, ypos,       glyph.texCoordX + glyph.texCoordW,         glyph.texCoordY },

            { xpos,     ypos + h,   glyph.texCoordX,                           glyph.texCoordY + glyph.texCoordH },
            { xpos + w, ypos,       glyph.texCoordX + glyph.texCoordW,         glyph.texCoordY },
            { xpos + w, ypos + h,   glyph.texCoordX + glyph.texCoordW,         glyph.texCoordY + glyph.texCoordH },
        };

        // Update VBO and render
        glBindBuffer(GL_ARRAY_BUFFER, textVBO_);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Advance cursor
        x += glyph.advanceX * scale;
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

void OpenGLGraphicsSystem::drawTextCentered(const std::string& text, Vec2 position,
                                       AssetHandle fontHandle, float size,
                                       const Color& color) {
    if (text.empty()) return;

    // Measure the text to find its width and height
    Vec2 textSize = measureText(text, fontHandle, size);

    // Calculate centered position
    Vec2 centeredPos{
        position.x - textSize.x / 2.0f,
        position.y - textSize.y / 2.0f
    };

    // Draw at the centered position
    drawText(text, centeredPos, fontHandle, size, color);
}

Vec2 OpenGLGraphicsSystem::measureText(const std::string& text, AssetHandle fontHandle,
                                  float size) const {
    if (text.empty()) return Vec2{0.0f, 0.0f};

    // Get font atlas (default if handle is invalid)
    const FontAtlas& atlas = getFontAtlas(fontHandle, size);

    // Calculate scale factor
    float scale = size / atlas.fontSize;

    float width = 0.0f;
    float maxHeight = size;

    for (char c : text) {
        unsigned char uc = static_cast<unsigned char>(c);
        const GlyphInfo& glyph = atlas.glyphs[uc];
        width += glyph.advanceX * scale;

        float glyphHeight = glyph.height * scale;
        if (glyphHeight > maxHeight) {
            maxHeight = glyphHeight;
        }
    }

    return Vec2{width, maxHeight};
}

void OpenGLGraphicsSystem::setCamera(const Camera& camera) {
    camera_ = camera;
}

Camera OpenGLGraphicsSystem::getCamera() const {
    return camera_;
}

Vec2 OpenGLGraphicsSystem::worldToScreen(Vec2 worldPos) const {
    // Transform world coordinates to screen coordinates
    Vec2 camPos = camera_.transform.position();
    Vec2 relative = worldPos - camPos;
    relative *= camera_.zoom;
    return Vec2{
        relative.x + camera_.viewportSize.width / 2.0f,
        camera_.viewportSize.height / 2.0f - relative.y
    };
}

Vec2 OpenGLGraphicsSystem::screenToWorld(Vec2 screenPos) const {
    // Transform screen coordinates to world coordinates
    Vec2 centered{
        screenPos.x - camera_.viewportSize.width / 2.0f,
        camera_.viewportSize.height / 2.0f - screenPos.y
    };
    centered /= camera_.zoom;
    return centered + camera_.transform.position();
}

Size OpenGLGraphicsSystem::getWindowSize() const {
    int w, h;
    glfwGetWindowSize(window_, &w, &h);
    return {w, h};
}

void OpenGLGraphicsSystem::setWindowSize(Size size) {
    glfwSetWindowSize(window_, size.width, size.height);
}

WindowMode OpenGLGraphicsSystem::getWindowMode() const {
    return windowMode_;
}

void OpenGLGraphicsSystem::setWindowMode(WindowMode mode) {
    if (mode == windowMode_) return;

    // Save windowed state before leaving windowed mode
    if (windowMode_ == WindowMode::Windowed) {
        glfwGetWindowPos(window_, &windowedX_, &windowedY_);
        glfwGetWindowSize(window_, &windowedWidth_, &windowedHeight_);
    }

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* vidmode = glfwGetVideoMode(monitor);

    switch (mode) {
        case WindowMode::Fullscreen:
            glfwSetWindowMonitor(window_, monitor, 0, 0,
                                 vidmode->width, vidmode->height, vidmode->refreshRate);
            break;
        case WindowMode::BorderlessFullscreen:
            glfwSetWindowAttrib(window_, GLFW_DECORATED, GLFW_FALSE);
            glfwSetWindowMonitor(window_, nullptr, 0, 0,
                                 vidmode->width, vidmode->height, 0);
            break;
        case WindowMode::Windowed:
            glfwSetWindowAttrib(window_, GLFW_DECORATED, GLFW_TRUE);
            glfwSetWindowMonitor(window_, nullptr, windowedX_, windowedY_,
                                 windowedWidth_, windowedHeight_, 0);
            break;
    }

    windowMode_ = mode;
}

bool OpenGLGraphicsSystem::shouldClose() const {
    return glfwWindowShouldClose(window_);
}

void* OpenGLGraphicsSystem::getNativeWindowHandle() const {
    return window_;
}

void OpenGLGraphicsSystem::setClearColor(const Color& color) {
    clearColor_ = color;
}

void OpenGLGraphicsSystem::setVSync(bool enabled) {
    glfwSwapInterval(enabled ? 1 : 0);
}

bool OpenGLGraphicsSystem::compileShader(GLuint shader, const char* source) {
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        // In a real implementation, use logging system
        return false;
    }
    return true;
}

bool OpenGLGraphicsSystem::linkProgram(GLuint program) {
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        // In a real implementation, use logging system
        return false;
    }
    return true;
}

GLuint OpenGLGraphicsSystem::createShaderProgram(const char* vertSource, const char* fragSource) {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    if (!compileShader(vertexShader, vertSource)) {
        glDeleteShader(vertexShader);
        return 0;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    if (!compileShader(fragmentShader, fragSource)) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    if (!linkProgram(program)) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(program);
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

void OpenGLGraphicsSystem::createWhiteTexture() {
    // Create a 1x1 white texture for sprites without textures
    unsigned char white[4] = {255, 255, 255, 255};

    glGenTextures(1, &whiteTexture_);
    glBindTexture(GL_TEXTURE_2D, whiteTexture_);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void OpenGLGraphicsSystem::createPrimitiveResources() {
    // Create primitive shader (simple colored vertices, no texture)
    const char* primitiveVertexShader = R"(
        #version 410 core
        layout (location = 0) in vec2 aPosition;
        uniform mat4 uViewProj;
        void main() {
            gl_Position = uViewProj * vec4(aPosition, 0.0, 1.0);
        }
    )";

    const char* primitiveFragmentShader = R"(
        #version 410 core
        out vec4 FragColor;
        uniform vec4 uColor;
        void main() {
            FragColor = uColor;
        }
    )";

    primitiveShaderProgram_ = createShaderProgram(primitiveVertexShader, primitiveFragmentShader);

    // Create VAO/VBO for dynamic primitive data
    glGenVertexArrays(1, &primitiveVAO_);
    glGenBuffers(1, &primitiveVBO_);

    glBindVertexArray(primitiveVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, primitiveVBO_);

    // Position attribute (2 floats per vertex)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void OpenGLGraphicsSystem::createTextResources() {
    // Create text shader
    const char* textVertexShader = R"(
        #version 410 core
        layout (location = 0) in vec2 aPosition;
        layout (location = 1) in vec2 aTexCoord;
        out vec2 vTexCoord;
        uniform mat4 uViewProj;
        void main() {
            gl_Position = uViewProj * vec4(aPosition, 0.0, 1.0);
            vTexCoord = aTexCoord;
        }
    )";

    const char* textFragmentShader = R"(
        #version 410 core
        in vec2 vTexCoord;
        out vec4 FragColor;
        uniform sampler2D uTexture;
        uniform vec4 uColor;
        void main() {
            float alpha = texture(uTexture, vTexCoord).r;
            FragColor = vec4(uColor.rgb, uColor.a * alpha);
        }
    )";

    textShaderProgram_ = createShaderProgram(textVertexShader, textFragmentShader);

    // Create VAO/VBO for dynamic text data
    glGenVertexArrays(1, &textVAO_);
    glGenBuffers(1, &textVBO_);

    glBindVertexArray(textVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);

    // Position attribute (2 floats)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    // Texcoord attribute (2 floats)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void OpenGLGraphicsSystem::createDefaultFont() {
    // Create a simple bitmap font using stb_truetype
    // For now, we'll create a placeholder monospaced font atlas

    const int ATLAS_SIZE = 512;
    const float FONT_SIZE = 32.0f;

    // Allocate bitmap
    std::vector<unsigned char> bitmap(ATLAS_SIZE * ATLAS_SIZE);

    // Initialize atlas metadata
    defaultFontAtlas_.atlasWidth = ATLAS_SIZE;
    defaultFontAtlas_.atlasHeight = ATLAS_SIZE;
    defaultFontAtlas_.fontSize = FONT_SIZE;
    defaultFontAtlas_.lineHeight = FONT_SIZE;
    defaultFontAtlas_.fontHandle = AssetHandle{};

    // Create simple monospaced placeholder glyphs
    const float charWidth = 16.0f;
    const float charHeight = 24.0f;
    const int charsPerRow = 16;

    for (int i = 0; i < 256; ++i) {
        int col = i % charsPerRow;
        int row = i / charsPerRow;

        GlyphInfo& glyph = defaultFontAtlas_.glyphs[i];
        glyph.advanceX = charWidth;
        glyph.bearingX = 0.0f;
        glyph.bearingY = charHeight;
        glyph.width = charWidth;
        glyph.height = charHeight;

        // Texture coordinates (normalized)
        glyph.texCoordX = static_cast<float>(col * 32) / ATLAS_SIZE;
        glyph.texCoordY = static_cast<float>(row * 32) / ATLAS_SIZE;
        glyph.texCoordW = 32.0f / ATLAS_SIZE;
        glyph.texCoordH = 32.0f / ATLAS_SIZE;

        // Draw a simple rectangle in the bitmap for each glyph
        int x = col * 32;
        int y = row * 32;
        for (int dy = 2; dy < 30; ++dy) {
            for (int dx = 2; dx < 30; ++dx) {
                bitmap[(y + dy) * ATLAS_SIZE + (x + dx)] = 255;
            }
        }
    }

    // Create OpenGL texture
    glGenTextures(1, &defaultFontAtlas_.textureID);
    glBindTexture(GL_TEXTURE_2D, defaultFontAtlas_.textureID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, ATLAS_SIZE, ATLAS_SIZE, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);
}

const FontAtlas& OpenGLGraphicsSystem::getFontAtlas(AssetHandle fontHandle, float size) const {
    // If no handle or invalid, return default font
    if (!fontHandle.isValid() || !pIAssetSystem_) {
        return defaultFontAtlas_;
    }

    // Check if already cached
    auto it = fontAtlases_.find(fontHandle);
    if (it != fontAtlases_.end()) {
        return it->second;
    }

    // Check if asset is loaded
    if (!pIAssetSystem_->isLoaded(fontHandle)) {
        return defaultFontAtlas_;
    }

    // Get font data from asset system
    void* rawData = pIAssetSystem_->getRawAsset(fontHandle);
    if (!rawData) {
        return defaultFontAtlas_;
    }

    // Cast to std::any and get FontData
    std::any* anyData = static_cast<std::any*>(rawData);
    if (!anyData->has_value()) {
        return defaultFontAtlas_;
    }

    // FontData is defined in bestow.assets.impl
    FontData* fontData = nullptr;
    try {
        fontData = std::any_cast<FontData>(anyData);
    } catch (const std::bad_any_cast&) {
        return defaultFontAtlas_;
    }

    if (!fontData || fontData->fileData.empty()) {
        return defaultFontAtlas_;
    }

    // Create font atlas using stb_truetype
    const int ATLAS_SIZE = 512;
    const float FONT_SIZE = size > 0 ? size : 32.0f;

    std::vector<unsigned char> bitmap(ATLAS_SIZE * ATLAS_SIZE);

    // Initialize stb_truetype font info
    stbtt_fontinfo fontInfo;
    if (!stbtt_InitFont(&fontInfo, fontData->fileData.data(), 0)) {
        return defaultFontAtlas_;
    }

    // Calculate font scale
    float scale = stbtt_ScaleForPixelHeight(&fontInfo, FONT_SIZE);

    // Get font metrics
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);

    // Create new atlas
    FontAtlas atlas;
    atlas.atlasWidth = ATLAS_SIZE;
    atlas.atlasHeight = ATLAS_SIZE;
    atlas.fontSize = FONT_SIZE;
    atlas.lineHeight = (ascent - descent + lineGap) * scale;
    atlas.fontHandle = fontHandle;

    // Pack glyphs into atlas
    int x = 1, y = 1;
    int rowHeight = 0;

    for (int c = 32; c < 127; ++c) {  // ASCII printable characters
        int w, h, xoff, yoff;
        unsigned char* glyphBitmap = stbtt_GetCodepointBitmap(&fontInfo, scale, scale,
                                                               c, &w, &h, &xoff, &yoff);

        // Check if glyph fits in current row
        if (x + w + 1 >= ATLAS_SIZE) {
            x = 1;
            y += rowHeight + 1;
            rowHeight = 0;
        }

        // Check if glyph fits in atlas
        if (y + h + 1 >= ATLAS_SIZE) {
            stbtt_FreeBitmap(glyphBitmap, nullptr);
            break;
        }

        // Copy glyph to atlas bitmap
        for (int gy = 0; gy < h; ++gy) {
            for (int gx = 0; gx < w; ++gx) {
                bitmap[(y + gy) * ATLAS_SIZE + (x + gx)] = glyphBitmap[gy * w + gx];
            }
        }

        // Get advance width
        int advanceWidth, leftSideBearing;
        stbtt_GetCodepointHMetrics(&fontInfo, c, &advanceWidth, &leftSideBearing);

        // Store glyph info
        GlyphInfo& glyph = atlas.glyphs[c];
        glyph.width = static_cast<float>(w);
        glyph.height = static_cast<float>(h);
        glyph.bearingX = static_cast<float>(xoff);
        glyph.bearingY = static_cast<float>(-yoff);  // stbtt uses top-down, we need bottom-up
        glyph.advanceX = advanceWidth * scale;
        glyph.texCoordX = static_cast<float>(x) / ATLAS_SIZE;
        glyph.texCoordY = static_cast<float>(y) / ATLAS_SIZE;
        glyph.texCoordW = static_cast<float>(w) / ATLAS_SIZE;
        glyph.texCoordH = static_cast<float>(h) / ATLAS_SIZE;

        x += w + 1;
        if (h > rowHeight) rowHeight = h;

        stbtt_FreeBitmap(glyphBitmap, nullptr);
    }

    // Set space character metrics (usually not rendered)
    int spaceAdvance, spaceLeftBearing;
    stbtt_GetCodepointHMetrics(&fontInfo, ' ', &spaceAdvance, &spaceLeftBearing);
    atlas.glyphs[' '].advanceX = spaceAdvance * scale;

    // Create OpenGL texture
    glGenTextures(1, &atlas.textureID);
    glBindTexture(GL_TEXTURE_2D, atlas.textureID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, ATLAS_SIZE, ATLAS_SIZE, 0,
                 GL_RED, GL_UNSIGNED_BYTE, bitmap.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);

    // Cache and return
    auto [inserted, success] = fontAtlases_.emplace(fontHandle, std::move(atlas));
    return inserted->second;
}

GLuint OpenGLGraphicsSystem::getOrUploadTexture(AssetHandle handle) {
    // Check cache first
    auto it = textureCache_.find(handle);
    if (it != textureCache_.end()) {
        return it->second;
    }

    // Not in cache - need to upload from asset system
    if (!pIAssetSystem_ || !pIAssetSystem_->isLoaded(handle)) {
        return whiteTexture_;  // Fallback if asset not available
    }

    // Get texture data from asset system
    void* rawData = pIAssetSystem_->getRawAsset(handle);
    if (!rawData) {
        return whiteTexture_;
    }

    // Cast the std::any to TextureData (asset system stores data as std::any)
    std::any* anyData = static_cast<std::any*>(rawData);
    if (!anyData->has_value()) {
        return whiteTexture_;
    }

    TextureData* textureData = nullptr;
    try {
        textureData = std::any_cast<TextureData>(anyData);
    } catch (const std::bad_any_cast&) {
        return whiteTexture_;
    }

    if (!textureData || textureData->pixels.empty()) {
        return whiteTexture_;
    }

    // Create OpenGL texture
    GLuint textureId = 0;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    // Determine format based on channels
    GLenum format = GL_RGBA;
    GLenum internalFormat = GL_RGBA;
    if (textureData->channels == 1) {
        format = GL_RED;
        internalFormat = GL_RED;
    } else if (textureData->channels == 3) {
        format = GL_RGB;
        internalFormat = GL_RGB;
    } else if (textureData->channels == 4) {
        format = GL_RGBA;
        internalFormat = GL_RGBA;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat,
                 textureData->width, textureData->height, 0,
                 format, GL_UNSIGNED_BYTE, textureData->pixels.data());

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);

    // Cache the texture
    textureCache_[handle] = textureId;
    return textureId;
}

//==============================================================================
// Automatic Entity Rendering
//==============================================================================

void OpenGLGraphicsSystem::setViewportCulling(bool enabled) {
    viewportCullingEnabled_ = enabled;
}

bool OpenGLGraphicsSystem::isViewportCullingEnabled() const {
    return viewportCullingEnabled_;
}

void OpenGLGraphicsSystem::renderEntities(IEntitySystem& entities) {
    // Render all layers
    renderEntities(entities, std::numeric_limits<RenderLayer>::min(),
                   std::numeric_limits<RenderLayer>::max());
}

void OpenGLGraphicsSystem::renderEntities(IEntitySystem& entities,
                                    RenderLayer minLayer, RenderLayer maxLayer) {
    // Structure to hold renderable items for sorting
    struct RenderItem {
        Entity entity;
        RenderLayer layer;
        enum class Type { Sprite, DebugRect, DebugCircle, DebugLine } type;
    };

    std::vector<RenderItem> items;

    // Calculate visible bounds for culling
    float viewHalfW = 0, viewHalfH = 0;
    Vec2 camPos = camera_.transform.position();
    if (viewportCullingEnabled_) {
        viewHalfW = camera_.viewportSize.width / (2.0f * camera_.zoom);
        viewHalfH = camera_.viewportSize.height / (2.0f * camera_.zoom);
    }

    auto isVisible = [&](const Transform2D& transform, float width, float height) -> bool {
        if (!viewportCullingEnabled_) return true;

        // Simple AABB check against camera viewport
        float entityLeft = transform.x - width / 2.0f;
        float entityRight = transform.x + width / 2.0f;
        float entityTop = transform.y - height / 2.0f;
        float entityBottom = transform.y + height / 2.0f;

        float viewLeft = camPos.x - viewHalfW;
        float viewRight = camPos.x + viewHalfW;
        float viewTop = camPos.y - viewHalfH;
        float viewBottom = camPos.y + viewHalfH;

        return !(entityRight < viewLeft || entityLeft > viewRight ||
                 entityBottom < viewTop || entityTop > viewBottom);
    };

    // Collect Sprites
    for (auto entity : entities.view<Transform2D, Sprite>()) {
        const auto& sprite = entities.get<Sprite>(entity);
        if (sprite.layer < minLayer || sprite.layer > maxLayer) continue;

        const auto& transform = entities.get<Transform2D>(entity);
        float width = static_cast<float>(sprite.sourceRect.size.width) * transform.scaleX;
        float height = static_cast<float>(sprite.sourceRect.size.height) * transform.scaleY;

        if (isVisible(transform, width, height)) {
            items.push_back({entity, sprite.layer, RenderItem::Type::Sprite});
        }
    }

    // Collect DebugRects
    for (auto entity : entities.view<Transform2D, DebugRect>()) {
        const auto& rect = entities.get<DebugRect>(entity);
        if (rect.layer < minLayer || rect.layer > maxLayer) continue;

        const auto& transform = entities.get<Transform2D>(entity);
        if (isVisible(transform, rect.size.x, rect.size.y)) {
            items.push_back({entity, rect.layer, RenderItem::Type::DebugRect});
        }
    }

    // Collect DebugCircles
    for (auto entity : entities.view<Transform2D, DebugCircle>()) {
        const auto& circle = entities.get<DebugCircle>(entity);
        if (circle.layer < minLayer || circle.layer > maxLayer) continue;

        const auto& transform = entities.get<Transform2D>(entity);
        if (isVisible(transform, circle.radius * 2, circle.radius * 2)) {
            items.push_back({entity, circle.layer, RenderItem::Type::DebugCircle});
        }
    }

    // Collect DebugLines
    for (auto entity : entities.view<Transform2D, DebugLine>()) {
        const auto& line = entities.get<DebugLine>(entity);
        if (line.layer < minLayer || line.layer > maxLayer) continue;

        items.push_back({entity, line.layer, RenderItem::Type::DebugLine});
    }

    // Sort by layer
    std::sort(items.begin(), items.end(),
              [](const RenderItem& a, const RenderItem& b) {
                  return a.layer < b.layer;
              });

    // Render in order
    for (const auto& item : items) {
        switch (item.type) {
            case RenderItem::Type::Sprite: {
                const auto& sprite = entities.get<Sprite>(item.entity);
                draw(sprite);
                break;
            }
            case RenderItem::Type::DebugRect: {
                const auto& transform = entities.get<Transform2D>(item.entity);
                const auto& rect = entities.get<DebugRect>(item.entity);

                // Calculate rectangle bounds centered on transform
                float halfW = rect.size.x / 2.0f;
                float halfH = rect.size.y / 2.0f;

                Canvas canvas{
                    .origin = {static_cast<int>(transform.x - halfW),
                               static_cast<int>(transform.y - halfH)},
                    .size = {static_cast<int>(rect.size.x),
                             static_cast<int>(rect.size.y)}
                };

                // Draw filled rect
                if (rect.filled && rect.fillColor.a > 0) {
                    drawRect(canvas, rect.fillColor, true);
                }

                // Draw outline
                if (rect.outlineWidth > 0.0f && rect.outlineColor.a > 0) {
                    drawRect(canvas, rect.outlineColor, false);
                }
                break;
            }
            case RenderItem::Type::DebugCircle: {
                const auto& transform = entities.get<Transform2D>(item.entity);
                const auto& circle = entities.get<DebugCircle>(item.entity);

                Vec2 center = {transform.x, transform.y};

                // Draw filled circle
                if (circle.filled && circle.fillColor.a > 0) {
                    drawCircle(center, circle.radius, circle.fillColor, true, circle.segments);
                }

                // Draw outline
                if (circle.outlineWidth > 0.0f && circle.outlineColor.a > 0) {
                    drawCircle(center, circle.radius, circle.outlineColor, false, circle.segments);
                }
                break;
            }
            case RenderItem::Type::DebugLine: {
                const auto& transform = entities.get<Transform2D>(item.entity);
                const auto& line = entities.get<DebugLine>(item.entity);

                Vec2 from = {transform.x, transform.y};
                Vec2 to = {transform.x + line.endOffset.x, transform.y + line.endOffset.y};

                drawLine(from, to, line.color, line.thickness);
                break;
            }
        }
    }
}

//==========================================================================
// IGraphicsContext Implementation
//==========================================================================

IUIRenderBackend* OpenGLGraphicsSystem::getUIRenderBackend() {
    // Create lazily on first call
    if (!uiRenderBackend_) {
        uiRenderBackend_ = std::make_unique<OpenGLUIRenderBackend>();
        if (!uiRenderBackend_->initialize()) {
            spdlog::error("Failed to initialize OpenGL UI render backend");
            uiRenderBackend_.reset();
            return nullptr;
        }
        // Set viewport size
        auto size = getWindowSize();
        uiRenderBackend_->setViewportSize(size.width, size.height);
    }
    return uiRenderBackend_.get();
}

bool OpenGLGraphicsSystem::isInFrame() const {
    return inFrame_;
}

void* OpenGLGraphicsSystem::getRenderContext() const {
    // OpenGL context is thread-bound, no explicit context object
    return nullptr;
}

void* OpenGLGraphicsSystem::getCurrentCommandBuffer() const {
    // OpenGL doesn't have command buffers
    return nullptr;
}

}  // namespace bestow
