// bestow-opengl-helpers/src/OpenGLHelpers.cpp
// Implementation of shared OpenGL utilities

module;

#include <cstdio>
#include <stdio.h>  // For stderr in global module fragment
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

module bestow.opengl.helpers;

import std;

namespace bestow::gl {

//==========================================================================
// Shader Compilation & Linking
//==========================================================================

ShaderCompileResult compileShader(GLenum shaderType, std::string_view source) {
    ShaderCompileResult result;

    result.shader = glCreateShader(shaderType);
    if (result.shader == 0) {
        result.errorMessage = "Failed to create shader object";
        return result;
    }

    const char* sourcePtr = source.data();
    GLint sourceLength = static_cast<GLint>(source.length());
    glShaderSource(result.shader, 1, &sourcePtr, &sourceLength);
    glCompileShader(result.shader);

    GLint success;
    glGetShaderiv(result.shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        char infoLog[1024];
        glGetShaderInfoLog(result.shader, sizeof(infoLog), nullptr, infoLog);

        const char* shaderTypeName = (shaderType == GL_VERTEX_SHADER) ? "Vertex" :
                                     (shaderType == GL_FRAGMENT_SHADER) ? "Fragment" :
                                     (shaderType == GL_GEOMETRY_SHADER) ? "Geometry" :
                                     (shaderType == GL_COMPUTE_SHADER) ? "Compute" : "Unknown";

        result.errorMessage = std::format("{} shader compilation failed: {}", shaderTypeName, infoLog);
        glDeleteShader(result.shader);
        result.shader = 0;
        return result;
    }

    result.success = true;
    return result;
}

ProgramLinkResult linkProgram(GLuint vertexShader, GLuint fragmentShader) {
    ProgramLinkResult result;

    result.program = glCreateProgram();
    if (result.program == 0) {
        result.errorMessage = "Failed to create program object";
        return result;
    }

    glAttachShader(result.program, vertexShader);
    glAttachShader(result.program, fragmentShader);
    glLinkProgram(result.program);

    GLint success;
    glGetProgramiv(result.program, GL_LINK_STATUS, &success);

    if (!success) {
        char infoLog[1024];
        glGetProgramInfoLog(result.program, sizeof(infoLog), nullptr, infoLog);
        result.errorMessage = std::format("Program linking failed: {}", infoLog);
        glDeleteProgram(result.program);
        result.program = 0;
        return result;
    }

    result.success = true;
    return result;
}

ProgramLinkResult createShaderProgram(std::string_view vertexSource,
                                     std::string_view fragmentSource) {
    ProgramLinkResult result;

    // Compile vertex shader
    auto vertResult = compileShader(GL_VERTEX_SHADER, vertexSource);
    if (!vertResult.success) {
        result.errorMessage = vertResult.errorMessage;
        return result;
    }

    // Compile fragment shader
    auto fragResult = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (!fragResult.success) {
        glDeleteShader(vertResult.shader);
        result.errorMessage = fragResult.errorMessage;
        return result;
    }

    // Link program
    result = linkProgram(vertResult.shader, fragResult.shader);

    // Clean up shaders (attached to program, no longer needed)
    glDeleteShader(vertResult.shader);
    glDeleteShader(fragResult.shader);

    return result;
}

//==========================================================================
// Uniform Management
//==========================================================================

GLint UniformCache::getLocation(GLuint program, std::string_view name) {
    std::string nameStr(name);
    auto it = cache_.find(nameStr);
    if (it != cache_.end()) {
        return it->second;
    }
    GLint location = glGetUniformLocation(program, nameStr.c_str());
    cache_[nameStr] = location;
    return location;
}

void UniformCache::clear() {
    cache_.clear();
}

void setUniformMat4(GLint location, const glm::mat4& value) {
    if (location != -1) {
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
    }
}

void setUniformMat3(GLint location, const glm::mat3& value) {
    if (location != -1) {
        glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
    }
}

void setUniformVec4(GLint location, const glm::vec4& value) {
    if (location != -1) {
        glUniform4fv(location, 1, glm::value_ptr(value));
    }
}

void setUniformVec3(GLint location, const glm::vec3& value) {
    if (location != -1) {
        glUniform3fv(location, 1, glm::value_ptr(value));
    }
}

void setUniformVec2(GLint location, const glm::vec2& value) {
    if (location != -1) {
        glUniform2fv(location, 1, glm::value_ptr(value));
    }
}

void setUniformFloat(GLint location, float value) {
    if (location != -1) {
        glUniform1f(location, value);
    }
}

void setUniformInt(GLint location, int value) {
    if (location != -1) {
        glUniform1i(location, value);
    }
}

void setUniformBool(GLint location, bool value) {
    if (location != -1) {
        glUniform1i(location, value ? 1 : 0);
    }
}

void setUniformFloatArray(GLint location, std::span<const float> values) {
    if (location != -1 && !values.empty()) {
        glUniform1fv(location, static_cast<GLsizei>(values.size()), values.data());
    }
}

void setUniformVec3Array(GLint location, std::span<const glm::vec3> values) {
    if (location != -1 && !values.empty()) {
        glUniform3fv(location, static_cast<GLsizei>(values.size()), glm::value_ptr(values[0]));
    }
}

void setUniformMat4Array(GLint location, std::span<const glm::mat4> values) {
    if (location != -1 && !values.empty()) {
        glUniformMatrix4fv(location, static_cast<GLsizei>(values.size()), GL_FALSE, glm::value_ptr(values[0]));
    }
}

//==========================================================================
// Texture Utilities
//==========================================================================

TextureFormat getTextureFormat(int channels) {
    TextureFormat format;
    switch (channels) {
        case 1:
            format.internalFormat = GL_R8;
            format.format = GL_RED;
            break;
        case 2:
            format.internalFormat = GL_RG8;
            format.format = GL_RG;
            break;
        case 3:
            format.internalFormat = GL_RGB8;
            format.format = GL_RGB;
            break;
        case 4:
        default:
            format.internalFormat = GL_RGBA8;
            format.format = GL_RGBA;
            break;
    }
    return format;
}

GLuint createTexture2D(int width, int height, int channels,
                       const void* pixels,
                       const TextureParams& params) {
    GLuint textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    auto format = getTextureFormat(channels);

    glTexImage2D(GL_TEXTURE_2D, 0, format.internalFormat,
                 width, height, 0,
                 format.format, GL_UNSIGNED_BYTE, pixels);

    if (params.generateMipmaps && pixels != nullptr) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, params.minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, params.magFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, params.wrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, params.wrapT);

    glBindTexture(GL_TEXTURE_2D, 0);

    return textureId;
}

//==========================================================================
// Render State Management
//==========================================================================

void setBlendMode(BlendMode mode) {
    switch (mode) {
        case BlendMode::Opaque:
            glDisable(GL_BLEND);
            break;
        case BlendMode::AlphaBlend:
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case BlendMode::Additive:
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            break;
        case BlendMode::Multiply:
            glEnable(GL_BLEND);
            glBlendFunc(GL_DST_COLOR, GL_ZERO);
            break;
        case BlendMode::AlphaTest:
            glDisable(GL_BLEND);
            // Alpha test handled in shader
            break;
    }
}

void setCullMode(CullMode mode) {
    switch (mode) {
        case CullMode::None:
            glDisable(GL_CULL_FACE);
            break;
        case CullMode::Front:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
            break;
        case CullMode::Back:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            break;
    }
}

void setDepthSettings(const DepthSettings& settings) {
    if (settings.depthTest) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    glDepthMask(settings.depthWrite ? GL_TRUE : GL_FALSE);
}

//==========================================================================
// GL State Backup/Restore
//==========================================================================

GLState captureGLState() {
    GLState state;
    glGetIntegerv(GL_CURRENT_PROGRAM, &state.program);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &state.texture);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &state.arrayBuffer);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &state.vertexArray);
    glGetIntegerv(GL_VIEWPORT, state.viewport);
    glGetIntegerv(GL_SCISSOR_BOX, state.scissorBox);
    glGetBooleanv(GL_BLEND, &state.blendEnabled);
    glGetBooleanv(GL_CULL_FACE, &state.cullFaceEnabled);
    glGetBooleanv(GL_DEPTH_TEST, &state.depthTestEnabled);
    glGetBooleanv(GL_SCISSOR_TEST, &state.scissorTestEnabled);
    glGetIntegerv(GL_BLEND_SRC_RGB, reinterpret_cast<GLint*>(&state.blendSrcRgb));
    glGetIntegerv(GL_BLEND_DST_RGB, reinterpret_cast<GLint*>(&state.blendDstRgb));
    glGetIntegerv(GL_BLEND_SRC_ALPHA, reinterpret_cast<GLint*>(&state.blendSrcAlpha));
    glGetIntegerv(GL_BLEND_DST_ALPHA, reinterpret_cast<GLint*>(&state.blendDstAlpha));
    return state;
}

void restoreGLState(const GLState& state) {
    glUseProgram(state.program);
    glBindTexture(GL_TEXTURE_2D, state.texture);
    glBindBuffer(GL_ARRAY_BUFFER, state.arrayBuffer);
    glBindVertexArray(state.vertexArray);
    glViewport(state.viewport[0], state.viewport[1], state.viewport[2], state.viewport[3]);
    glScissor(state.scissorBox[0], state.scissorBox[1], state.scissorBox[2], state.scissorBox[3]);

    if (state.blendEnabled) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (state.cullFaceEnabled) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (state.depthTestEnabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (state.scissorTestEnabled) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);

    glBlendFuncSeparate(state.blendSrcRgb, state.blendDstRgb, state.blendSrcAlpha, state.blendDstAlpha);
}

//==========================================================================
// Error Checking
//==========================================================================

bool checkGLError(std::string_view context) {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        const char* errorStr =
            (error == GL_INVALID_ENUM) ? "GL_INVALID_ENUM" :
            (error == GL_INVALID_VALUE) ? "GL_INVALID_VALUE" :
            (error == GL_INVALID_OPERATION) ? "GL_INVALID_OPERATION" :
            (error == GL_OUT_OF_MEMORY) ? "GL_OUT_OF_MEMORY" :
            (error == GL_INVALID_FRAMEBUFFER_OPERATION) ? "GL_INVALID_FRAMEBUFFER_OPERATION" :
            "Unknown";

        if (!context.empty()) {
            std::fprintf(stderr, "[OpenGL Error] %s: %s (0x%X)\n",
                        std::string(context).c_str(), errorStr, error);
        } else {
            std::fprintf(stderr, "[OpenGL Error] %s (0x%X)\n", errorStr, error);
        }
        return true;
    }
    return false;
}

void clearGLErrors() {
    while (glGetError() != GL_NO_ERROR) {
        // Drain error queue
    }
}

} // namespace bestow::gl
