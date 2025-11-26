// jframe-graphics/src/jframe.graphics.impl.cppm
// Graphics system implementation

module;

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

export module jframe.graphics.impl;

import std;
import jframe.graphics;
import jframe.types;

export namespace jframe {

// Font glyph metrics
struct GlyphInfo {
    float advanceX;      // Horizontal advance
    float bearingX;      // Horizontal bearing (offset from cursor)
    float bearingY;      // Vertical bearing (offset from baseline)
    float width;         // Glyph width
    float height;        // Glyph height
    float texCoordX;     // Texture coordinate X (normalized)
    float texCoordY;     // Texture coordinate Y (normalized)
    float texCoordW;     // Texture coordinate width (normalized)
    float texCoordH;     // Texture coordinate height (normalized)
};

// Font atlas data
struct FontAtlas {
    GLuint textureID = 0;
    int atlasWidth = 0;
    int atlasHeight = 0;
    float fontSize = 0.0f;
    GlyphInfo glyphs[256] = {};  // ASCII characters
    float lineHeight = 0.0f;
    AssetHandle fontHandle;
};

class GraphicsSystem : public IGraphicsSystem {
public:
    GraphicsSystem() = default;
    ~GraphicsSystem() override;

    bool initialize(int width, int height, const std::string& title);

    //======================================================================
    // Frame Lifecycle
    //======================================================================

    void beginFrame() override;
    void endFrame() override;

    //======================================================================
    // Sprite Rendering
    //======================================================================

    void draw(const Sprite& sprite) override;
    void drawBatch(std::span<const Sprite> sprites) override;

    //======================================================================
    // Primitive Rendering
    //======================================================================

    void drawRect(const Canvas& rect, const Color& color, bool filled = true) override;
    void drawLine(Vec2 from, Vec2 to, const Color& color, float thickness = 1.0f) override;
    void drawCircle(Vec2 center, float radius, const Color& color,
                    bool filled = true, int segments = 32) override;
    void drawPolygon(std::span<const Vec2> vertices, const Color& color,
                     bool filled = true) override;

    //======================================================================
    // Text Rendering
    //======================================================================

    void drawText(const std::string& text, Vec2 position,
                  AssetHandle fontHandle, float size,
                  const Color& color = Color::white()) override;
    Vec2 measureText(const std::string& text, AssetHandle fontHandle,
                     float size) const override;

    //======================================================================
    // Camera
    //======================================================================

    void setCamera(const Camera& camera) override;
    Camera getCamera() const override;

    Vec2 worldToScreen(Vec2 worldPos) const override;
    Vec2 screenToWorld(Vec2 screenPos) const override;

    //======================================================================
    // Window Management
    //======================================================================

    Size getWindowSize() const override;
    void setWindowSize(Size size) override;
    bool isFullscreen() const override;
    void setFullscreen(bool fullscreen) override;
    bool shouldClose() const override;
    void* getNativeWindowHandle() const override;

    //======================================================================
    // Render State
    //======================================================================

    void setClearColor(const Color& color) override;
    void setVSync(bool enabled) override;

private:
    GLFWwindow* window_ = nullptr;
    Camera camera_;
    Color clearColor_ = Color::black();
    bool isFullscreen_ = false;
    int windowedWidth_ = 0;
    int windowedHeight_ = 0;
    int windowedX_ = 0;
    int windowedY_ = 0;
    std::vector<Sprite> spriteBatch_;

    // OpenGL rendering resources
    GLuint spriteShaderProgram_ = 0;
    GLuint spriteVAO_ = 0;
    GLuint spriteVBO_ = 0;
    GLuint whiteTexture_ = 0;

    // Debug primitive rendering resources
    GLuint primitiveShaderProgram_ = 0;
    GLuint primitiveVAO_ = 0;
    GLuint primitiveVBO_ = 0;

    // Text rendering resources
    GLuint textShaderProgram_ = 0;
    GLuint textVAO_ = 0;
    GLuint textVBO_ = 0;
    std::unordered_map<AssetHandle, FontAtlas, AssetHandleHash> fontAtlases_;
    FontAtlas defaultFontAtlas_;

    // Shader compilation helpers
    bool compileShader(GLuint shader, const char* source);
    bool linkProgram(GLuint program);
    GLuint createShaderProgram(const char* vertSource, const char* fragSource);
    void createWhiteTexture();
    void createPrimitiveResources();

    // Text rendering helpers
    void createDefaultFont();
    void createTextResources();
    const FontAtlas& getFontAtlas(AssetHandle fontHandle, float size) const;
};

// Factory function (exported via namespace)
inline std::unique_ptr<IGraphicsSystem> createGraphicsSystem() {
    return std::make_unique<GraphicsSystem>();
}

}  // namespace jframe
