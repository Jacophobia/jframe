// jframe-graphics/src/GraphicsSystem.cpp
// Graphics system implementation

module;

#include <algorithm>
#include <span>
#include <string>
#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>

module jframe.graphics.impl;

namespace jframe {

GraphicsSystem::~GraphicsSystem() {
    // Clean up OpenGL resources
    if (spriteVBO_) glDeleteBuffers(1, &spriteVBO_);
    if (spriteVAO_) glDeleteVertexArrays(1, &spriteVAO_);
    if (spriteShaderProgram_) glDeleteProgram(spriteShaderProgram_);
    if (whiteTexture_) glDeleteTextures(1, &whiteTexture_);

    if (window_) {
        glfwDestroyWindow(window_);
    }
    glfwTerminate();
}

bool GraphicsSystem::initialize(int width, int height, const std::string& title) {
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

    return true;
}

void GraphicsSystem::beginFrame() {
    glClearColor(
        clearColor_.r / 255.0f,
        clearColor_.g / 255.0f,
        clearColor_.b / 255.0f,
        clearColor_.a / 255.0f
    );
    glClear(GL_COLOR_BUFFER_BIT);
    spriteBatch_.clear();
}

void GraphicsSystem::endFrame() {
    if (spriteBatch_.empty()) {
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
        camPos.y - halfHeight,
        camPos.y + halfHeight,
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

        // Bind texture (use white texture as default for now)
        glBindTexture(GL_TEXTURE_2D, whiteTexture_);

        // Draw quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    glBindVertexArray(0);
    glUseProgram(0);

    glfwSwapBuffers(window_);
    glfwPollEvents();
}

void GraphicsSystem::draw(const Sprite& sprite) {
    spriteBatch_.push_back(sprite);
}

void GraphicsSystem::drawBatch(std::span<const Sprite> sprites) {
    spriteBatch_.insert(spriteBatch_.end(), sprites.begin(), sprites.end());
}

void GraphicsSystem::drawRect(const Canvas& rect, const Color& color, bool filled) {
    // OpenGL primitive rendering
}

void GraphicsSystem::drawLine(Vec2 from, Vec2 to, const Color& color, float thickness) {
    // OpenGL line rendering
}

void GraphicsSystem::drawCircle(Vec2 center, float radius, const Color& color,
                                 bool filled, int segments) {
    // OpenGL circle rendering
}

void GraphicsSystem::drawPolygon(std::span<const Vec2> vertices,
                                  const Color& color, bool filled) {
    // OpenGL polygon rendering
}

void GraphicsSystem::drawText(const std::string& text, Vec2 position,
                               AssetHandle fontHandle, float size,
                               const Color& color) {
    // Text rendering with MSDF fonts
}

Vec2 GraphicsSystem::measureText(const std::string& text, AssetHandle fontHandle,
                                  float size) const {
    // Text measurement
    return Vec2{0.0f, 0.0f};
}

void GraphicsSystem::setCamera(const Camera& camera) {
    camera_ = camera;
}

Camera GraphicsSystem::getCamera() const {
    return camera_;
}

Vec2 GraphicsSystem::worldToScreen(Vec2 worldPos) const {
    // Transform world coordinates to screen coordinates
    Vec2 camPos = camera_.transform.position();
    Vec2 relative = worldPos - camPos;
    relative *= camera_.zoom;
    return Vec2{
        relative.x + camera_.viewportSize.width / 2.0f,
        camera_.viewportSize.height / 2.0f - relative.y
    };
}

Vec2 GraphicsSystem::screenToWorld(Vec2 screenPos) const {
    // Transform screen coordinates to world coordinates
    Vec2 centered{
        screenPos.x - camera_.viewportSize.width / 2.0f,
        camera_.viewportSize.height / 2.0f - screenPos.y
    };
    centered /= camera_.zoom;
    return centered + camera_.transform.position();
}

Size GraphicsSystem::getWindowSize() const {
    int w, h;
    glfwGetWindowSize(window_, &w, &h);
    return {w, h};
}

void GraphicsSystem::setWindowSize(Size size) {
    glfwSetWindowSize(window_, size.width, size.height);
}

bool GraphicsSystem::isFullscreen() const {
    return isFullscreen_;
}

void GraphicsSystem::setFullscreen(bool fullscreen) {
    if (fullscreen == isFullscreen_) return;

    if (fullscreen) {
        glfwGetWindowPos(window_, &windowedX_, &windowedY_);
        glfwGetWindowSize(window_, &windowedWidth_, &windowedHeight_);

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(window_, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    } else {
        glfwSetWindowMonitor(window_, nullptr, windowedX_, windowedY_,
                             windowedWidth_, windowedHeight_, 0);
    }

    isFullscreen_ = fullscreen;
}

bool GraphicsSystem::shouldClose() const {
    return glfwWindowShouldClose(window_);
}

void* GraphicsSystem::getNativeWindowHandle() const {
    return window_;
}

void GraphicsSystem::setClearColor(const Color& color) {
    clearColor_ = color;
}

void GraphicsSystem::setVSync(bool enabled) {
    glfwSwapInterval(enabled ? 1 : 0);
}

bool GraphicsSystem::compileShader(GLuint shader, const char* source) {
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

bool GraphicsSystem::linkProgram(GLuint program) {
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

GLuint GraphicsSystem::createShaderProgram(const char* vertSource, const char* fragSource) {
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

void GraphicsSystem::createWhiteTexture() {
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

}  // namespace jframe
