// bestow-opengl-helpers/src/bestow.opengl.helpers.cppm
// Shared OpenGL utility functions for Bestow game engine

module;

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <unordered_map>

export module bestow.opengl.helpers;

import std;

export namespace bestow::gl {

//==========================================================================
// Shader Compilation & Linking
//==========================================================================

struct ShaderCompileResult {
    GLuint shader = 0;
    bool success = false;
    std::string errorMessage;
};

struct ProgramLinkResult {
    GLuint program = 0;
    bool success = false;
    std::string errorMessage;
};

/// Compile a shader from source with detailed error reporting
ShaderCompileResult compileShader(GLenum shaderType, std::string_view source);

/// Link vertex and fragment shaders into a program
ProgramLinkResult linkProgram(GLuint vertexShader, GLuint fragmentShader);

/// Compile and link a complete shader program from source
ProgramLinkResult createShaderProgram(std::string_view vertexSource,
                                     std::string_view fragmentSource);

//==========================================================================
// Uniform Management
//==========================================================================

/// Cache for uniform locations to avoid repeated glGetUniformLocation calls
class UniformCache {
public:
    GLint getLocation(GLuint program, std::string_view name);
    void clear();
private:
    std::unordered_map<std::string, GLint> cache_;
};

// Type-safe uniform setters
void setUniformMat4(GLint location, const glm::mat4& value);
void setUniformMat3(GLint location, const glm::mat3& value);
void setUniformVec4(GLint location, const glm::vec4& value);
void setUniformVec3(GLint location, const glm::vec3& value);
void setUniformVec2(GLint location, const glm::vec2& value);
void setUniformFloat(GLint location, float value);
void setUniformInt(GLint location, int value);
void setUniformBool(GLint location, bool value);

// Array uniform setters
void setUniformFloatArray(GLint location, std::span<const float> values);
void setUniformVec3Array(GLint location, std::span<const glm::vec3> values);
void setUniformMat4Array(GLint location, std::span<const glm::mat4> values);

//==========================================================================
// Texture Utilities
//==========================================================================

struct TextureFormat {
    GLenum internalFormat;
    GLenum format;
};

/// Get OpenGL texture format from channel count (1-4)
TextureFormat getTextureFormat(int channels);

struct TextureParams {
    GLenum minFilter = GL_LINEAR;
    GLenum magFilter = GL_LINEAR;
    GLenum wrapS = GL_REPEAT;
    GLenum wrapT = GL_REPEAT;
    bool generateMipmaps = true;
};

/// Create a 2D texture from pixel data
GLuint createTexture2D(int width, int height, int channels,
                       const void* pixels,
                       const TextureParams& params = {});

//==========================================================================
// Render State Management
//==========================================================================

enum class BlendMode {
    Opaque,
    AlphaBlend,
    Additive,
    Multiply,
    AlphaTest
};

enum class CullMode {
    None,
    Front,
    Back
};

struct DepthSettings {
    bool depthTest = true;
    bool depthWrite = true;
};

void setBlendMode(BlendMode mode);
void setCullMode(CullMode mode);
void setDepthSettings(const DepthSettings& settings);

//==========================================================================
// GL State Backup/Restore (for ImGui-like overlays)
//==========================================================================

struct GLState {
    GLint program;
    GLint texture;
    GLint arrayBuffer;
    GLint vertexArray;
    GLint viewport[4];
    GLint scissorBox[4];
    GLboolean blendEnabled;
    GLboolean cullFaceEnabled;
    GLboolean depthTestEnabled;
    GLboolean scissorTestEnabled;
    GLenum blendSrcRgb;
    GLenum blendDstRgb;
    GLenum blendSrcAlpha;
    GLenum blendDstAlpha;
};

GLState captureGLState();
void restoreGLState(const GLState& state);

//==========================================================================
// Error Checking (Debug)
//==========================================================================

/// Check for OpenGL errors and log them (returns true if error occurred)
bool checkGLError(std::string_view context = "");

/// Clear any pending GL errors
void clearGLErrors();

} // namespace bestow::gl
