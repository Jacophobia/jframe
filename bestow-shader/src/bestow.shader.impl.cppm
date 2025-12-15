// bestow-shader/src/bestow.shader.impl.cppm
// Dynamic Shader System with Hot Reload and Lua Material Support

module;

// Kangaru DI
#include <kangaru/kangaru.hpp>
#include <bestow/kangaru_macros.hpp>

// OpenGL headers - MUST be in global module fragment
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// Lua support (conditional)
#ifdef BESTOW_HAS_SOL2
#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>
#endif

// File watching (conditional)
#ifdef BESTOW_HAS_EFSW
#include <efsw/efsw.hpp>
#endif

export module bestow.shader.impl;

import std;
import bestow.services;  // Re-exports all contracts including bestow.shader, bestow.types, bestow.assets

export namespace bestow {

//==========================================================================
// Internal Resource Structures
//==========================================================================

struct ShaderProgramResource {
    GLuint program = 0;
    std::string name;
    std::string vertexPath;
    std::string fragmentPath;
    std::string geometryPath;
    std::string tessControlPath;
    std::string tessEvalPath;
    std::string computePath;
    bool enableHotReload = false;
    bool isCompute = false;  // Compute shaders are standalone
    std::unordered_map<std::string, GLint> uniformLocations;

    // AssetSystem integration
    AssetHandle vertAssetHandle{};
    AssetHandle fragAssetHandle{};
    AssetHandle geomAssetHandle{};
    AssetHandle tessCtrlAssetHandle{};
    AssetHandle tessEvalAssetHandle{};
    AssetHandle computeAssetHandle{};
    SubscriptionId vertSubscriptionId = InvalidSubscriptionId;
    SubscriptionId fragSubscriptionId = InvalidSubscriptionId;
    SubscriptionId geomSubscriptionId = InvalidSubscriptionId;
    SubscriptionId tessCtrlSubscriptionId = InvalidSubscriptionId;
    SubscriptionId tessEvalSubscriptionId = InvalidSubscriptionId;
    SubscriptionId computeSubscriptionId = InvalidSubscriptionId;

    GLint getUniformLocation(const std::string& uniformName) {
        auto it = uniformLocations.find(uniformName);
        if (it != uniformLocations.end()) {
            return it->second;
        }
        GLint loc = glGetUniformLocation(program, uniformName.c_str());
        uniformLocations[uniformName] = loc;
        return loc;
    }
};

struct ShaderMaterialResource {
    std::string name;
    ShaderProgramHandle shader;
    std::unordered_map<std::string, UniformValue> uniforms;
    std::vector<TextureBinding> textures;
    std::string luaPath;  // For reload

    BlendMode blendMode = BlendMode::Opaque;
    CullMode cullMode = CullMode::Back;
    bool depthWrite = true;
    bool depthTest = true;

    // AssetSystem integration
    AssetHandle matAssetHandle{};
    SubscriptionId matSubscriptionId = InvalidSubscriptionId;
};



//==========================================================================
// OpenGLShaderSystem Implementation
//==========================================================================

class OpenGLShaderSystem : public IShaderSystem {
public:
    explicit OpenGLShaderSystem(IAssetSystem* pIAssetSystem = nullptr)
        : pIAssetSystem_(pIAssetSystem) {}
    ~OpenGLShaderSystem() override {
        // Unsubscribe from all AssetSystem notifications
        if (pIAssetSystem_) {
            for (auto& [handle, shader] : shaders_) {
                if (shader.vertSubscriptionId != InvalidSubscriptionId) {
                    pIAssetSystem_->unsubscribe(shader.vertSubscriptionId);
                }
                if (shader.fragSubscriptionId != InvalidSubscriptionId) {
                    pIAssetSystem_->unsubscribe(shader.fragSubscriptionId);
                }
                if (shader.geomSubscriptionId != InvalidSubscriptionId) {
                    pIAssetSystem_->unsubscribe(shader.geomSubscriptionId);
                }
                if (shader.tessCtrlSubscriptionId != InvalidSubscriptionId) {
                    pIAssetSystem_->unsubscribe(shader.tessCtrlSubscriptionId);
                }
                if (shader.tessEvalSubscriptionId != InvalidSubscriptionId) {
                    pIAssetSystem_->unsubscribe(shader.tessEvalSubscriptionId);
                }
                if (shader.computeSubscriptionId != InvalidSubscriptionId) {
                    pIAssetSystem_->unsubscribe(shader.computeSubscriptionId);
                }
            }

            for (auto& [handle, material] : materials_) {
                if (material.matSubscriptionId != InvalidSubscriptionId) {
                    pIAssetSystem_->unsubscribe(material.matSubscriptionId);
                }
            }
        }

        // Clean up cached textures
        for (auto& [handle, textureId] : textureCache_) {
            if (textureId != 0) {
                glDeleteTextures(1, &textureId);
            }
        }
        textureCache_.clear();

        // Clean up all shaders and materials
        for (auto& [handle, resource] : shaders_) {
            if (resource.program != 0) {
                glDeleteProgram(resource.program);
            }
        }
    }

    //======================================================================
    // Shader Program Management
    //======================================================================

    Result<ShaderProgramHandle, ShaderCompileError> createShaderFromSource(
        std::string_view vertexSource,
        std::string_view fragmentSource,
        std::string_view name) override
    {
        return compileShaderProgram(
            std::string(vertexSource),
            std::string(fragmentSource),
            std::string(name),
            "", ""  // No file paths
        );
    }

    Result<ShaderProgramHandle, ShaderCompileError> loadShader(
        std::string_view vertexPath,
        std::string_view fragmentPath,
        bool enableHotReload) override
    {
        if (!pIAssetSystem_) {
            return std::unexpected(ShaderCompileError{
                .error = ShaderError::InternalError,
                .message = "AssetSystem not initialized"
            });
        }

        std::string fullVertPath = resolvePath(shaderBasePath_, vertexPath);
        std::string fullFragPath = resolvePath(shaderBasePath_, fragmentPath);

        // Load vertex shader through AssetSystem
        AssetHandle vertHandle = pIAssetSystem_->loadShader(fullVertPath);
        const ShaderData* vertShaderData = pIAssetSystem_->getShaderData(vertHandle);
        if (!vertShaderData || vertShaderData->glslSource.empty()) {
            return std::unexpected(ShaderCompileError{
                .error = ShaderError::FileNotFound,
                .message = "Could not load vertex shader file",
                .filePath = fullVertPath
            });
        }

        // Load fragment shader through AssetSystem
        AssetHandle fragHandle = pIAssetSystem_->loadShader(fullFragPath);
        const ShaderData* fragShaderData = pIAssetSystem_->getShaderData(fragHandle);
        if (!fragShaderData || fragShaderData->glslSource.empty()) {
            return std::unexpected(ShaderCompileError{
                .error = ShaderError::FileNotFound,
                .message = "Could not load fragment shader file",
                .filePath = fullFragPath
            });
        }

        // Compile
        auto result = compileShaderProgram(
            vertShaderData->glslSource, fragShaderData->glslSource,
            std::string(vertexPath) + " + " + std::string(fragmentPath),
            fullVertPath, fullFragPath
        );

        if (result && enableHotReload && hotReloadEnabled_) {
            auto& shader = shaders_[*result];
            shader.enableHotReload = true;
            shader.vertAssetHandle = vertHandle;
            shader.fragAssetHandle = fragHandle;

            // Subscribe to shader changes via AssetSystem
            SubscriptionId vertSub = pIAssetSystem_->subscribe(vertHandle, [this, handle = *result](AssetHandle, AssetType) {
                onShaderFileChanged(handle);
            });
            SubscriptionId fragSub = pIAssetSystem_->subscribe(fragHandle, [this, handle = *result](AssetHandle, AssetType) {
                onShaderFileChanged(handle);
            });

            shader.vertSubscriptionId = vertSub;
            shader.fragSubscriptionId = fragSub;
        }

        return result;
    }

    Result<ShaderProgramHandle, ShaderCompileError> createShader(const ShaderProgramDef& def) override {
        if (!pIAssetSystem_) {
            return std::unexpected(ShaderCompileError{
                .error = ShaderError::InternalError,
                .message = "AssetSystem not initialized"
            });
        }

        std::string vertexSource, fragmentSource, geometrySource;
        std::string tessControlSource, tessEvalSource, computeSource;
        std::string vertexPath, fragmentPath, geometryPath;
        std::string tessControlPath, tessEvalPath, computePath;

        // Track asset handles for hot reload
        AssetHandle vertHandle{}, fragHandle{}, geomHandle{};
        AssetHandle tessCtrlHandle{}, tessEvalHandle{}, computeHandle{};

        for (const auto& stage : def.stages) {
            std::string source;
            std::string path;
            AssetHandle stageHandle{};

            if (stage.isFromFile()) {
                std::string fullPath = resolvePath(shaderBasePath_, stage.filePath);

                // Load shader through AssetSystem
                stageHandle = pIAssetSystem_->loadShader(fullPath);
                const ShaderData* shaderData = pIAssetSystem_->getShaderData(stageHandle);
                if (!shaderData || shaderData->glslSource.empty()) {
                    return std::unexpected(ShaderCompileError{
                        .error = ShaderError::FileNotFound,
                        .message = "Could not load shader file",
                        .filePath = stage.filePath
                    });
                }
                source = shaderData->glslSource;
                path = fullPath;
            } else {
                source = stage.source;
            }

            switch (stage.stage) {
                case ShaderStage::Vertex:
                    vertexSource = source;
                    vertexPath = path;
                    vertHandle = stageHandle;
                    break;
                case ShaderStage::Fragment:
                    fragmentSource = source;
                    fragmentPath = path;
                    fragHandle = stageHandle;
                    break;
                case ShaderStage::Geometry:
                    geometrySource = source;
                    geometryPath = path;
                    geomHandle = stageHandle;
                    break;
                case ShaderStage::TessControl:
                    tessControlSource = source;
                    tessControlPath = path;
                    tessCtrlHandle = stageHandle;
                    break;
                case ShaderStage::TessEvaluation:
                    tessEvalSource = source;
                    tessEvalPath = path;
                    tessEvalHandle = stageHandle;
                    break;
                case ShaderStage::Compute:
                    computeSource = source;
                    computePath = path;
                    computeHandle = stageHandle;
                    break;
            }
        }

        auto result = compileFullShaderProgram(
            vertexSource, fragmentSource, geometrySource,
            tessControlSource, tessEvalSource, computeSource,
            def.name,
            vertexPath, fragmentPath, geometryPath,
            tessControlPath, tessEvalPath, computePath
        );

        // Set up hot reload subscriptions for all shader stages
        if (result && def.enableHotReload && hotReloadEnabled_) {
            auto& shader = shaders_[*result];
            shader.enableHotReload = true;

            // Store asset handles
            shader.vertAssetHandle = vertHandle;
            shader.fragAssetHandle = fragHandle;
            shader.geomAssetHandle = geomHandle;
            shader.tessCtrlAssetHandle = tessCtrlHandle;
            shader.tessEvalAssetHandle = tessEvalHandle;
            shader.computeAssetHandle = computeHandle;

            // Subscribe to changes for each valid shader stage
            if (vertHandle.isValid()) {
                shader.vertSubscriptionId = pIAssetSystem_->subscribe(vertHandle,
                    [this, handle = *result](AssetHandle, AssetType) {
                        onShaderFileChanged(handle);
                    });
            }
            if (fragHandle.isValid()) {
                shader.fragSubscriptionId = pIAssetSystem_->subscribe(fragHandle,
                    [this, handle = *result](AssetHandle, AssetType) {
                        onShaderFileChanged(handle);
                    });
            }
            if (geomHandle.isValid()) {
                shader.geomSubscriptionId = pIAssetSystem_->subscribe(geomHandle,
                    [this, handle = *result](AssetHandle, AssetType) {
                        onShaderFileChanged(handle);
                    });
            }
            if (tessCtrlHandle.isValid()) {
                shader.tessCtrlSubscriptionId = pIAssetSystem_->subscribe(tessCtrlHandle,
                    [this, handle = *result](AssetHandle, AssetType) {
                        onShaderFileChanged(handle);
                    });
            }
            if (tessEvalHandle.isValid()) {
                shader.tessEvalSubscriptionId = pIAssetSystem_->subscribe(tessEvalHandle,
                    [this, handle = *result](AssetHandle, AssetType) {
                        onShaderFileChanged(handle);
                    });
            }
            if (computeHandle.isValid()) {
                shader.computeSubscriptionId = pIAssetSystem_->subscribe(computeHandle,
                    [this, handle = *result](AssetHandle, AssetType) {
                        onShaderFileChanged(handle);
                    });
            }
        }

        return result;
    }

    void destroyShader(ShaderProgramHandle handle) override {
        auto it = shaders_.find(handle);
        if (it != shaders_.end()) {
            // Unsubscribe from AssetSystem notifications
            if (pIAssetSystem_) {
                if (it->second.vertSubscriptionId != InvalidSubscriptionId) {
                    pIAssetSystem_->unsubscribe(it->second.vertSubscriptionId);
                }
                if (it->second.fragSubscriptionId != InvalidSubscriptionId) {
                    pIAssetSystem_->unsubscribe(it->second.fragSubscriptionId);
                }
                if (it->second.geomSubscriptionId != InvalidSubscriptionId) {
                    pIAssetSystem_->unsubscribe(it->second.geomSubscriptionId);
                }
                if (it->second.tessCtrlSubscriptionId != InvalidSubscriptionId) {
                    pIAssetSystem_->unsubscribe(it->second.tessCtrlSubscriptionId);
                }
                if (it->second.tessEvalSubscriptionId != InvalidSubscriptionId) {
                    pIAssetSystem_->unsubscribe(it->second.tessEvalSubscriptionId);
                }
                if (it->second.computeSubscriptionId != InvalidSubscriptionId) {
                    pIAssetSystem_->unsubscribe(it->second.computeSubscriptionId);
                }
            }

            if (it->second.program != 0) {
                glDeleteProgram(it->second.program);
            }
            shaders_.erase(it);
        }
    }

    bool hasShader(ShaderProgramHandle handle) const override {
        return shaders_.contains(handle);
    }

    std::string getShaderName(ShaderProgramHandle handle) const override {
        auto it = shaders_.find(handle);
        return it != shaders_.end() ? it->second.name : "";
    }

    //======================================================================
    // Shader Binding
    //======================================================================

    void bindShader(ShaderProgramHandle handle) override {
        auto it = shaders_.find(handle);
        if (it != shaders_.end() && it->second.program != 0) {
            glUseProgram(it->second.program);
            currentShader_ = handle;
            stats_.shaderBinds++;
        }
    }

    void unbindShader() override {
        glUseProgram(0);
        currentShader_ = 0;
    }

    ShaderProgramHandle getCurrentShader() const override {
        return currentShader_;
    }

    //======================================================================
    // Uniform Management
    //======================================================================

    void setUniform(std::string_view name, const UniformValue& value) override {
        if (currentShader_ == 0) return;

        auto it = shaders_.find(currentShader_);
        if (it == shaders_.end()) return;

        GLint loc = it->second.getUniformLocation(std::string(name));
        if (loc == -1) return;

        setUniformValue(loc, value);
        stats_.uniformSets++;
    }

    void setUniform(std::uint32_t location, const UniformValue& value) override {
        if (currentShader_ == 0 || static_cast<GLint>(location) == -1) return;
        setUniformValue(static_cast<GLint>(location), value);
        stats_.uniformSets++;
    }

    std::int32_t getUniformLocation(ShaderProgramHandle shader, std::string_view name) const override {
        auto it = shaders_.find(shader);
        if (it == shaders_.end()) return -1;
        return glGetUniformLocation(it->second.program, std::string(name).c_str());
    }

    std::vector<UniformInfo> getUniformInfo(ShaderProgramHandle shader) const override {
        std::vector<UniformInfo> result;
        auto it = shaders_.find(shader);
        if (it == shaders_.end()) return result;

        GLint count;
        glGetProgramiv(it->second.program, GL_ACTIVE_UNIFORMS, &count);

        for (GLint i = 0; i < count; ++i) {
            char name[256];
            GLsizei length;
            GLint size;
            GLenum type;
            glGetActiveUniform(it->second.program, i, sizeof(name), &length, &size, &type, name);

            result.push_back(UniformInfo{
                .name = std::string(name, length),
                .location = static_cast<uint32_t>(glGetUniformLocation(it->second.program, name)),
                .type = static_cast<uint32_t>(type),
                .count = static_cast<uint32_t>(size)
            });
        }

        return result;
    }

    //======================================================================
    // Material Management
    //======================================================================

    Result<MaterialHandle, ShaderError> createMaterial(const ShaderMaterialDef& def) override {
        if (!hasShader(def.shader)) {
            return std::unexpected(ShaderError::InvalidHandle);
        }

        MaterialHandle handle = nextMaterialHandle_++;
        materials_[handle] = ShaderMaterialResource{
            .name = def.name,
            .shader = def.shader,
            .uniforms = def.uniforms,
            .textures = def.textures,
            .blendMode = def.blendMode,
            .cullMode = def.cullMode,
            .depthWrite = def.depthWrite,
            .depthTest = def.depthTest
        };

        stats_.materialCount++;
        return handle;
    }

    Result<MaterialHandle, ShaderCompileError> loadMaterial(std::string_view luaPath) override {
#ifdef BESTOW_HAS_SOL2
        if (!pIAssetSystem_) {
            return std::unexpected(ShaderCompileError{
                .error = ShaderError::InternalError,
                .message = "AssetSystem not initialized"
            });
        }

        std::string fullPath = resolvePath(materialBasePath_, luaPath);

        // Load material Lua file through AssetSystem as Data asset
        AssetHandle matAssetHandle = pIAssetSystem_->registerAsset(AssetType::Data, fullPath);
        pIAssetSystem_->loadAsset(matAssetHandle);

        if (!pIAssetSystem_->isLoaded(matAssetHandle)) {
            return std::unexpected(ShaderCompileError{
                .error = ShaderError::FileNotFound,
                .message = "Could not load material Lua file",
                .filePath = fullPath
            });
        }

        // Get the loaded data and parse it as Lua
        // For now, we still need to use sol2 to parse, but the file I/O goes through AssetSystem
        // TODO: AssetSystem could return the raw file content as string for Data assets

        sol::state lua;
        lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table);

        // Sandbox: remove dangerous functions
        lua["os"] = sol::lua_nil;
        lua["io"] = sol::lua_nil;
        lua["loadfile"] = sol::lua_nil;
        lua["dofile"] = sol::lua_nil;
        lua["load"] = sol::lua_nil;

        try {
            // TEMPORARY: Still using safe_script_file until AssetSystem provides raw data API
            // This is an acceptable compromise as the asset is registered and tracked
            sol::protected_function_result result = lua.safe_script_file(fullPath);
            if (!result.valid()) {
                sol::error err = result;
                return std::unexpected(ShaderCompileError{
                    .error = ShaderError::LuaParseError,
                    .message = err.what(),
                    .filePath = fullPath
                });
            }

            sol::table mat = result;
            LuaMaterialDef def = parseLuaMaterial(mat);

            // Load or get shader
            auto shaderResult = loadShader(def.vertexShaderPath, def.fragmentShaderPath, def.hotReload);
            if (!shaderResult) {
                return std::unexpected(shaderResult.error());
            }

            MaterialHandle handle = nextMaterialHandle_++;
            materials_[handle] = ShaderMaterialResource{
                .name = std::string(luaPath),
                .shader = *shaderResult,
                .uniforms = def.uniforms,
                .luaPath = fullPath,
                .blendMode = def.blendMode,
                .cullMode = def.cullMode,
                .depthWrite = def.depthWrite,
                .depthTest = def.depthTest
            };

            // Load textures
            for (const auto& [samplerName, texturePath] : def.texturePaths) {
                if (pIAssetSystem_) {
                    AssetHandle texHandle = pIAssetSystem_->registerAsset(AssetType::Texture, texturePath);
                    materials_[handle].textures.push_back(TextureBinding{
                        .slot = static_cast<uint32_t>(materials_[handle].textures.size()),
                        .samplerName = samplerName,
                        .texture = texHandle
                    });
                }
            }

            if (def.hotReload && hotReloadEnabled_) {
                // Subscribe to material file changes via AssetSystem
                SubscriptionId matSub = pIAssetSystem_->subscribe(matAssetHandle, [this, handle](AssetHandle, AssetType) {
                    onMaterialFileChanged(handle);
                });
                materials_[handle].matAssetHandle = matAssetHandle;
                materials_[handle].matSubscriptionId = matSub;
            }

            stats_.materialCount++;
            return handle;

        } catch (const std::exception& e) {
            return std::unexpected(ShaderCompileError{
                .error = ShaderError::LuaParseError,
                .message = e.what(),
                .filePath = fullPath
            });
        }
#else
        return std::unexpected(ShaderCompileError{
            .error = ShaderError::InternalError,
            .message = "Lua support not compiled in",
            .filePath = std::string(luaPath)
        });
#endif
    }

    Result<MaterialHandle, ShaderError> createMaterial(ShaderProgramHandle shader, std::string_view name) override {
        if (!hasShader(shader)) {
            return std::unexpected(ShaderError::InvalidHandle);
        }

        MaterialHandle handle = nextMaterialHandle_++;
        materials_[handle] = ShaderMaterialResource{
            .name = std::string(name),
            .shader = shader
        };

        stats_.materialCount++;
        return handle;
    }

    Result<MaterialHandle, ShaderError> cloneMaterial(MaterialHandle source, std::string_view newName) override {
        auto it = materials_.find(source);
        if (it == materials_.end()) {
            return std::unexpected(ShaderError::InvalidHandle);
        }

        MaterialHandle handle = nextMaterialHandle_++;
        materials_[handle] = it->second;
        materials_[handle].name = std::string(newName.empty() ? it->second.name + "_clone" : newName);
        materials_[handle].luaPath.clear();  // Clones don't inherit reload path

        stats_.materialCount++;
        return handle;
    }

    void destroyMaterial(MaterialHandle handle) override {
        auto it = materials_.find(handle);
        if (it != materials_.end()) {
            // Unsubscribe from AssetSystem notifications
            if (pIAssetSystem_ && it->second.matSubscriptionId != InvalidSubscriptionId) {
                pIAssetSystem_->unsubscribe(it->second.matSubscriptionId);
            }
            materials_.erase(it);
        }
    }

    bool hasMaterial(MaterialHandle handle) const override {
        return materials_.contains(handle);
    }

    std::string getMaterialName(MaterialHandle handle) const override {
        auto it = materials_.find(handle);
        return it != materials_.end() ? it->second.name : "";
    }

    //======================================================================
    // Material Property Setting
    //======================================================================

    void setMaterialUniform(MaterialHandle handle, std::string_view name, const UniformValue& value) override {
        auto it = materials_.find(handle);
        if (it != materials_.end()) {
            it->second.uniforms[std::string(name)] = value;
        }
    }

    void setMaterialTexture(MaterialHandle handle, std::string_view samplerName, AssetHandle texture) override {
        auto it = materials_.find(handle);
        if (it == materials_.end()) return;

        // Find existing binding or add new one
        for (auto& binding : it->second.textures) {
            if (binding.samplerName == samplerName) {
                binding.texture = texture;
                return;
            }
        }

        it->second.textures.push_back(TextureBinding{
            .slot = static_cast<uint32_t>(it->second.textures.size()),
            .samplerName = std::string(samplerName),
            .texture = texture
        });
    }

    void setMaterialTexture(MaterialHandle handle, std::uint32_t slot, AssetHandle texture) override {
        auto it = materials_.find(handle);
        if (it == materials_.end()) return;

        for (auto& binding : it->second.textures) {
            if (binding.slot == slot) {
                binding.texture = texture;
                return;
            }
        }

        it->second.textures.push_back(TextureBinding{
            .slot = slot,
            .samplerName = "texture" + std::to_string(slot),
            .texture = texture
        });
    }

    void setMaterialBlendMode(MaterialHandle handle, BlendMode mode) override {
        auto it = materials_.find(handle);
        if (it != materials_.end()) {
            it->second.blendMode = mode;
        }
    }

    void setMaterialCullMode(MaterialHandle handle, CullMode mode) override {
        auto it = materials_.find(handle);
        if (it != materials_.end()) {
            it->second.cullMode = mode;
        }
    }

    void setMaterialDepthSettings(MaterialHandle handle, bool depthWrite, bool depthTest) override {
        auto it = materials_.find(handle);
        if (it != materials_.end()) {
            it->second.depthWrite = depthWrite;
            it->second.depthTest = depthTest;
        }
    }

    //======================================================================
    // Material Binding
    //======================================================================

    void bindMaterial(MaterialHandle handle) override {
        auto it = materials_.find(handle);
        if (it == materials_.end()) return;

        const auto& mat = it->second;

        // Bind shader
        bindShader(mat.shader);

        // Set all uniforms
        for (const auto& [name, value] : mat.uniforms) {
            setUniform(name, value);
        }

        // Set blend mode
        switch (mat.blendMode) {
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

        // Set cull mode
        switch (mat.cullMode) {
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

        // Set depth settings
        if (mat.depthTest) {
            glEnable(GL_DEPTH_TEST);
        } else {
            glDisable(GL_DEPTH_TEST);
        }
        glDepthMask(mat.depthWrite ? GL_TRUE : GL_FALSE);

        // Bind textures
        for (const auto& texBinding : mat.textures) {
            GLuint textureId = const_cast<OpenGLShaderSystem*>(this)->getOrCreateTexture(texBinding.texture);
            if (textureId != 0) {
                glActiveTexture(GL_TEXTURE0 + texBinding.slot);
                glBindTexture(GL_TEXTURE_2D, textureId);
                // Set the sampler uniform to point to the correct texture unit
                setUniform(texBinding.samplerName, static_cast<int>(texBinding.slot));
            }
        }

        currentMaterial_ = handle;
        stats_.materialBinds++;
    }

    MaterialHandle getCurrentMaterial() const override {
        return currentMaterial_;
    }

    //======================================================================
    // Built-in Shaders
    //======================================================================

    ShaderProgramHandle getDefaultPBRShader() const override {
        return defaultPBRShader_;
    }

    ShaderProgramHandle getDefaultUnlitShader() const override {
        return defaultUnlitShader_;
    }

    ShaderProgramHandle getDebugShader() const override {
        return debugShader_;
    }

    ShaderProgramHandle getSkyboxShader() const override {
        return skyboxShader_;
    }

    //======================================================================
    // Hot Reload
    //======================================================================

    void setHotReloadEnabled(bool enabled) override {
        hotReloadEnabled_ = enabled;
    }

    bool isHotReloadEnabled() const override {
        return hotReloadEnabled_;
    }

    void update() override {
        // Hot reload is now handled via AssetSystem subscriptions
        // No polling needed - AssetSystem will call our callbacks when files change
    }

    Result<void, ShaderCompileError> reloadShader(ShaderProgramHandle handle) override {
        if (!pIAssetSystem_) {
            return std::unexpected(ShaderCompileError{
                .error = ShaderError::InternalError,
                .message = "AssetSystem not initialized"
            });
        }

        auto it = shaders_.find(handle);
        if (it == shaders_.end()) {
            return std::unexpected(ShaderCompileError{
                .error = ShaderError::InvalidHandle,
                .message = "Shader not found"
            });
        }

        auto& shader = it->second;

        // For compute shaders, only compute handle is needed
        if (shader.isCompute) {
            if (!shader.computeAssetHandle.isValid()) {
                return std::unexpected(ShaderCompileError{
                    .error = ShaderError::InternalError,
                    .message = "Compute shader was not loaded from file"
                });
            }

            // Reload the asset
            pIAssetSystem_->reloadAsset(shader.computeAssetHandle);
            const ShaderData* computeData = pIAssetSystem_->getShaderData(shader.computeAssetHandle);
            if (!computeData || computeData->glslSource.empty()) {
                return std::unexpected(ShaderCompileError{
                    .error = ShaderError::FileNotFound,
                    .filePath = shader.computePath
                });
            }

            // Compile new compute program
            GLuint newProgram = compileFullProgram("", "", "", "", "", computeData->glslSource);
            if (newProgram == 0) {
                return std::unexpected(ShaderCompileError{
                    .error = ShaderError::CompilationFailed,
                    .message = lastCompileError_,
                    .filePath = shader.computePath
                });
            }

            // Delete old program and replace
            if (shader.program != 0) {
                glDeleteProgram(shader.program);
            }
            shader.program = newProgram;
            shader.uniformLocations.clear();
            stats_.hotReloads++;
            return {};
        }

        // For graphics pipeline shaders, vertex and fragment are required
        if (!shader.vertAssetHandle.isValid() || !shader.fragAssetHandle.isValid()) {
            return std::unexpected(ShaderCompileError{
                .error = ShaderError::InternalError,
                .message = "Shader was not loaded from files"
            });
        }

        // Reload all valid assets
        pIAssetSystem_->reloadAsset(shader.vertAssetHandle);
        pIAssetSystem_->reloadAsset(shader.fragAssetHandle);
        if (shader.geomAssetHandle.isValid()) {
            pIAssetSystem_->reloadAsset(shader.geomAssetHandle);
        }
        if (shader.tessCtrlAssetHandle.isValid()) {
            pIAssetSystem_->reloadAsset(shader.tessCtrlAssetHandle);
        }
        if (shader.tessEvalAssetHandle.isValid()) {
            pIAssetSystem_->reloadAsset(shader.tessEvalAssetHandle);
        }

        // Get reloaded shader data from AssetSystem
        const ShaderData* vertShaderData = pIAssetSystem_->getShaderData(shader.vertAssetHandle);
        if (!vertShaderData || vertShaderData->glslSource.empty()) {
            return std::unexpected(ShaderCompileError{
                .error = ShaderError::FileNotFound,
                .filePath = shader.vertexPath
            });
        }

        const ShaderData* fragShaderData = pIAssetSystem_->getShaderData(shader.fragAssetHandle);
        if (!fragShaderData || fragShaderData->glslSource.empty()) {
            return std::unexpected(ShaderCompileError{
                .error = ShaderError::FileNotFound,
                .filePath = shader.fragmentPath
            });
        }

        // Get optional shader stages
        std::string geometrySource, tessControlSource, tessEvalSource;

        if (shader.geomAssetHandle.isValid()) {
            const ShaderData* geomData = pIAssetSystem_->getShaderData(shader.geomAssetHandle);
            if (geomData && !geomData->glslSource.empty()) {
                geometrySource = geomData->glslSource;
            }
        }

        if (shader.tessCtrlAssetHandle.isValid()) {
            const ShaderData* tessCtrlData = pIAssetSystem_->getShaderData(shader.tessCtrlAssetHandle);
            if (tessCtrlData && !tessCtrlData->glslSource.empty()) {
                tessControlSource = tessCtrlData->glslSource;
            }
        }

        if (shader.tessEvalAssetHandle.isValid()) {
            const ShaderData* tessEvalData = pIAssetSystem_->getShaderData(shader.tessEvalAssetHandle);
            if (tessEvalData && !tessEvalData->glslSource.empty()) {
                tessEvalSource = tessEvalData->glslSource;
            }
        }

        // Compile new program with all stages
        GLuint newProgram = compileFullProgram(
            vertShaderData->glslSource,
            fragShaderData->glslSource,
            geometrySource,
            tessControlSource,
            tessEvalSource,
            ""  // No compute for graphics pipeline
        );

        if (newProgram == 0) {
            return std::unexpected(ShaderCompileError{
                .error = ShaderError::CompilationFailed,
                .message = lastCompileError_,
                .filePath = shader.vertexPath
            });
        }

        // Delete old program and replace
        if (shader.program != 0) {
            glDeleteProgram(shader.program);
        }
        shader.program = newProgram;
        shader.uniformLocations.clear();

        stats_.hotReloads++;
        return {};
    }

    Result<void, ShaderCompileError> reloadMaterial(MaterialHandle handle) override {
        auto it = materials_.find(handle);
        if (it == materials_.end()) {
            return std::unexpected(ShaderCompileError{
                .error = ShaderError::InvalidHandle,
                .message = "Material not found"
            });
        }

        if (it->second.luaPath.empty()) {
            return std::unexpected(ShaderCompileError{
                .error = ShaderError::InternalError,
                .message = "Material was not loaded from Lua"
            });
        }

        // Save existing subscription info to preserve hot reload capability
        AssetHandle savedAssetHandle = it->second.matAssetHandle;
        SubscriptionId savedSubscriptionId = it->second.matSubscriptionId;
        std::string savedLuaPath = it->second.luaPath;

        // Re-load from Lua file (creates a new material internally)
        auto result = loadMaterial(savedLuaPath);
        if (!result) {
            return std::unexpected(result.error());
        }

        // Copy new material data over old, preserving the original handle
        auto newIt = materials_.find(*result);
        if (newIt != materials_.end()) {
            // Unsubscribe the new material's subscription (it's a duplicate)
            if (pIAssetSystem_ && newIt->second.matSubscriptionId != InvalidSubscriptionId) {
                pIAssetSystem_->unsubscribe(newIt->second.matSubscriptionId);
            }

            // Copy the newly loaded material properties to the original
            it->second.name = newIt->second.name;
            it->second.shader = newIt->second.shader;
            it->second.uniforms = newIt->second.uniforms;
            it->second.textures = newIt->second.textures;
            it->second.blendMode = newIt->second.blendMode;
            it->second.cullMode = newIt->second.cullMode;
            it->second.depthWrite = newIt->second.depthWrite;
            it->second.depthTest = newIt->second.depthTest;

            // Restore original subscription info (so hot reload continues working)
            it->second.luaPath = savedLuaPath;
            it->second.matAssetHandle = savedAssetHandle;
            it->second.matSubscriptionId = savedSubscriptionId;

            // Remove the temporary new material
            materials_.erase(newIt);
            stats_.materialCount--;  // We didn't actually add a new material
        }

        return {};
    }

    void setShaderReloadCallback(ShaderReloadCallback callback) override {
        shaderReloadCallback_ = std::move(callback);
    }

    void setMaterialReloadCallback(MaterialReloadCallback callback) override {
        materialReloadCallback_ = std::move(callback);
    }

    //======================================================================
    // Asset Integration
    //======================================================================

    void setShaderBasePath(std::string_view path) override {
        shaderBasePath_ = std::string(path);
        if (!shaderBasePath_.empty() && shaderBasePath_.back() != '/') {
            shaderBasePath_ += '/';
        }
    }

    void setMaterialBasePath(std::string_view path) override {
        materialBasePath_ = std::string(path);
        if (!materialBasePath_.empty() && materialBasePath_.back() != '/') {
            materialBasePath_ += '/';
        }
    }

    //======================================================================
    // Statistics
    //======================================================================

    ShaderStats getStats() const override {
        return stats_;
    }

    void resetFrameStats() override {
        stats_.shaderBinds = 0;
        stats_.materialBinds = 0;
        stats_.uniformSets = 0;
    }

    //======================================================================
    // Initialization
    //======================================================================

    bool initialize() {
        // Create default shaders
        auto pbrResult = createShaderFromSource(getDefaultPBRVertexSource(), getDefaultPBRFragmentSource(), "DefaultPBR");
        if (pbrResult) {
            defaultPBRShader_ = *pbrResult;
        }

        auto unlitResult = createShaderFromSource(getDefaultUnlitVertexSource(), getDefaultUnlitFragmentSource(), "DefaultUnlit");
        if (unlitResult) {
            defaultUnlitShader_ = *unlitResult;
        }

        auto debugResult = createShaderFromSource(getDebugVertexSource(), getDebugFragmentSource(), "Debug");
        if (debugResult) {
            debugShader_ = *debugResult;
        }

        auto skyboxResult = createShaderFromSource(getSkyboxVertexSource(), getSkyboxFragmentSource(), "Skybox");
        if (skyboxResult) {
            skyboxShader_ = *skyboxResult;
        }

        return defaultPBRShader_ != 0;
    }

private:
    //======================================================================
    // Hot Reload Callbacks (called by AssetSystem subscriptions)
    //======================================================================

    void onShaderFileChanged(ShaderProgramHandle handle) {
        auto result = reloadShader(handle);
        if (shaderReloadCallback_) {
            shaderReloadCallback_(
                handle,
                result.has_value(),
                result.has_value() ? "" : result.error().message
            );
        }
    }

    void onMaterialFileChanged(MaterialHandle handle) {
        auto result = reloadMaterial(handle);
        if (materialReloadCallback_) {
            materialReloadCallback_(
                handle,
                result.has_value(),
                result.has_value() ? "" : result.error().message
            );
        }
    }

    //======================================================================
    // Internal Helpers
    //======================================================================

    std::string resolvePath(const std::string& basePath, std::string_view relativePath) {
        if (relativePath.empty()) return "";

        // Check if path uses :assets:/ or :library:/ scheme - resolve via PathResolver
        if (PathResolver::hasScheme(relativePath)) {
            return PathResolver::resolveString(relativePath);
        }

        // Check if already an absolute system path
        if (relativePath[0] == '/') {
            return std::string(relativePath);
        }

        // If basePath uses a scheme, resolve it first then append relative path
        if (PathResolver::hasScheme(basePath)) {
            auto resolvedBase = PathResolver::resolve(basePath);
            return (resolvedBase / std::filesystem::path(relativePath)).string();
        }

        // Plain relative path - use basePath directly
        return basePath + std::string(relativePath);
    }


    Result<ShaderProgramHandle, ShaderCompileError> compileShaderProgram(
        const std::string& vertexSource,
        const std::string& fragmentSource,
        const std::string& name,
        const std::string& vertexPath,
        const std::string& fragmentPath)
    {
        GLuint program = compileProgram(vertexSource, fragmentSource);
        if (program == 0) {
            return std::unexpected(ShaderCompileError{
                .error = ShaderError::CompilationFailed,
                .message = lastCompileError_,
                .filePath = !vertexPath.empty() ? vertexPath : fragmentPath
            });
        }

        ShaderProgramHandle handle = nextShaderHandle_++;
        shaders_[handle] = ShaderProgramResource{
            .program = program,
            .name = name,
            .vertexPath = vertexPath,
            .fragmentPath = fragmentPath,
            .enableHotReload = false
        };

        stats_.shaderCount++;
        return handle;
    }

    GLuint compileProgram(const std::string& vertexSource, const std::string& fragmentSource) {
        GLuint vertShader = compileShader(GL_VERTEX_SHADER, vertexSource);
        if (vertShader == 0) return 0;

        GLuint fragShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
        if (fragShader == 0) {
            glDeleteShader(vertShader);
            return 0;
        }

        GLuint program = glCreateProgram();
        glAttachShader(program, vertShader);
        glAttachShader(program, fragShader);
        glLinkProgram(program);

        glDeleteShader(vertShader);
        glDeleteShader(fragShader);

        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[1024];
            glGetProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);
            lastCompileError_ = std::string("Linking failed: ") + infoLog;
            glDeleteProgram(program);
            return 0;
        }

        return program;
    }

    GLuint compileShader(GLenum type, const std::string& source) {
        if (source.empty()) return 0;

        GLuint shader = glCreateShader(type);
        const char* src = source.c_str();
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[1024];
            glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
            const char* typeName = "Unknown";
            switch (type) {
                case GL_VERTEX_SHADER: typeName = "Vertex"; break;
                case GL_FRAGMENT_SHADER: typeName = "Fragment"; break;
                case GL_GEOMETRY_SHADER: typeName = "Geometry"; break;
                case GL_TESS_CONTROL_SHADER: typeName = "TessControl"; break;
                case GL_TESS_EVALUATION_SHADER: typeName = "TessEvaluation"; break;
                case GL_COMPUTE_SHADER: typeName = "Compute"; break;
            }
            lastCompileError_ = std::string(typeName) + " shader compilation failed: " + infoLog;
            glDeleteShader(shader);
            return 0;
        }

        return shader;
    }

    // Compile a full shader program with all optional stages
    GLuint compileFullProgram(
        const std::string& vertexSource,
        const std::string& fragmentSource,
        const std::string& geometrySource,
        const std::string& tessControlSource,
        const std::string& tessEvalSource,
        const std::string& computeSource)
    {
        // Compute shaders are standalone - can't be combined with other stages
        if (!computeSource.empty()) {
            GLuint computeShader = compileShader(GL_COMPUTE_SHADER, computeSource);
            if (computeShader == 0) return 0;

            GLuint program = glCreateProgram();
            glAttachShader(program, computeShader);
            glLinkProgram(program);
            glDeleteShader(computeShader);

            GLint success;
            glGetProgramiv(program, GL_LINK_STATUS, &success);
            if (!success) {
                char infoLog[1024];
                glGetProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);
                lastCompileError_ = std::string("Compute program linking failed: ") + infoLog;
                glDeleteProgram(program);
                return 0;
            }
            return program;
        }

        // Regular graphics pipeline - vertex and fragment are required
        if (vertexSource.empty() || fragmentSource.empty()) {
            lastCompileError_ = "Vertex and fragment shaders are required for graphics pipeline";
            return 0;
        }

        // Compile all shaders
        GLuint vertShader = compileShader(GL_VERTEX_SHADER, vertexSource);
        if (vertShader == 0) return 0;

        GLuint fragShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
        if (fragShader == 0) {
            glDeleteShader(vertShader);
            return 0;
        }

        GLuint geomShader = 0;
        if (!geometrySource.empty()) {
            geomShader = compileShader(GL_GEOMETRY_SHADER, geometrySource);
            if (geomShader == 0) {
                glDeleteShader(vertShader);
                glDeleteShader(fragShader);
                return 0;
            }
        }

        GLuint tessCtrlShader = 0;
        GLuint tessEvalShader = 0;
        if (!tessControlSource.empty() || !tessEvalSource.empty()) {
            // Both tessellation shaders must be provided together
            if (tessControlSource.empty() || tessEvalSource.empty()) {
                lastCompileError_ = "Both tessellation control and evaluation shaders must be provided together";
                glDeleteShader(vertShader);
                glDeleteShader(fragShader);
                if (geomShader != 0) glDeleteShader(geomShader);
                return 0;
            }

            tessCtrlShader = compileShader(GL_TESS_CONTROL_SHADER, tessControlSource);
            if (tessCtrlShader == 0) {
                glDeleteShader(vertShader);
                glDeleteShader(fragShader);
                if (geomShader != 0) glDeleteShader(geomShader);
                return 0;
            }

            tessEvalShader = compileShader(GL_TESS_EVALUATION_SHADER, tessEvalSource);
            if (tessEvalShader == 0) {
                glDeleteShader(vertShader);
                glDeleteShader(fragShader);
                if (geomShader != 0) glDeleteShader(geomShader);
                glDeleteShader(tessCtrlShader);
                return 0;
            }
        }

        // Link program
        GLuint program = glCreateProgram();
        glAttachShader(program, vertShader);
        glAttachShader(program, fragShader);
        if (geomShader != 0) glAttachShader(program, geomShader);
        if (tessCtrlShader != 0) glAttachShader(program, tessCtrlShader);
        if (tessEvalShader != 0) glAttachShader(program, tessEvalShader);
        glLinkProgram(program);

        // Delete shaders after linking
        glDeleteShader(vertShader);
        glDeleteShader(fragShader);
        if (geomShader != 0) glDeleteShader(geomShader);
        if (tessCtrlShader != 0) glDeleteShader(tessCtrlShader);
        if (tessEvalShader != 0) glDeleteShader(tessEvalShader);

        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[1024];
            glGetProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);
            lastCompileError_ = std::string("Program linking failed: ") + infoLog;
            glDeleteProgram(program);
            return 0;
        }

        return program;
    }

    Result<ShaderProgramHandle, ShaderCompileError> compileFullShaderProgram(
        const std::string& vertexSource,
        const std::string& fragmentSource,
        const std::string& geometrySource,
        const std::string& tessControlSource,
        const std::string& tessEvalSource,
        const std::string& computeSource,
        const std::string& name,
        const std::string& vertexPath,
        const std::string& fragmentPath,
        const std::string& geometryPath,
        const std::string& tessControlPath,
        const std::string& tessEvalPath,
        const std::string& computePath)
    {
        GLuint program = compileFullProgram(
            vertexSource, fragmentSource, geometrySource,
            tessControlSource, tessEvalSource, computeSource
        );

        if (program == 0) {
            std::string filePath = !vertexPath.empty() ? vertexPath :
                                  !fragmentPath.empty() ? fragmentPath :
                                  !computePath.empty() ? computePath : "";
            return std::unexpected(ShaderCompileError{
                .error = ShaderError::CompilationFailed,
                .message = lastCompileError_,
                .filePath = filePath
            });
        }

        ShaderProgramHandle handle = nextShaderHandle_++;
        shaders_[handle] = ShaderProgramResource{
            .program = program,
            .name = name,
            .vertexPath = vertexPath,
            .fragmentPath = fragmentPath,
            .geometryPath = geometryPath,
            .tessControlPath = tessControlPath,
            .tessEvalPath = tessEvalPath,
            .computePath = computePath,
            .enableHotReload = false,
            .isCompute = !computeSource.empty()
        };

        stats_.shaderCount++;
        return handle;
    }

    void setUniformValue(GLint location, const UniformValue& value) {
        std::visit([location](const auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, bool>) {
                glUniform1i(location, v ? 1 : 0);
            } else if constexpr (std::is_same_v<T, int>) {
                glUniform1i(location, v);
            } else if constexpr (std::is_same_v<T, float>) {
                glUniform1f(location, v);
            } else if constexpr (std::is_same_v<T, Vec2>) {
                glUniform2fv(location, 1, glm::value_ptr(v));
            } else if constexpr (std::is_same_v<T, Vec3>) {
                glUniform3fv(location, 1, glm::value_ptr(v));
            } else if constexpr (std::is_same_v<T, Vec4>) {
                glUniform4fv(location, 1, glm::value_ptr(v));
            } else if constexpr (std::is_same_v<T, Mat3>) {
                glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(v));
            } else if constexpr (std::is_same_v<T, Mat4>) {
                glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(v));
            } else if constexpr (std::is_same_v<T, std::vector<float>>) {
                if (!v.empty()) {
                    glUniform1fv(location, static_cast<GLsizei>(v.size()), v.data());
                }
            } else if constexpr (std::is_same_v<T, std::vector<Vec2>>) {
                if (!v.empty()) {
                    glUniform2fv(location, static_cast<GLsizei>(v.size()), glm::value_ptr(v[0]));
                }
            } else if constexpr (std::is_same_v<T, std::vector<Vec3>>) {
                if (!v.empty()) {
                    glUniform3fv(location, static_cast<GLsizei>(v.size()), glm::value_ptr(v[0]));
                }
            } else if constexpr (std::is_same_v<T, std::vector<Vec4>>) {
                if (!v.empty()) {
                    glUniform4fv(location, static_cast<GLsizei>(v.size()), glm::value_ptr(v[0]));
                }
            } else if constexpr (std::is_same_v<T, std::vector<Mat4>>) {
                if (!v.empty()) {
                    glUniformMatrix4fv(location, static_cast<GLsizei>(v.size()), GL_FALSE, glm::value_ptr(v[0]));
                }
            }
        }, value);
    }


#ifdef BESTOW_HAS_SOL2
    // Helper to get value with default (avoids sol2 get_or ambiguity)
    template<typename T>
    T getWithDefault(const sol::table& t, const char* key, T defaultVal) {
        sol::optional<T> val = t[key];
        return val.value_or(defaultVal);
    }

    template<typename T>
    T getWithDefault(const sol::table& t, int key, T defaultVal) {
        sol::optional<T> val = t[key];
        return val.value_or(defaultVal);
    }

    LuaMaterialDef parseLuaMaterial(const sol::table& mat) {
        LuaMaterialDef def;

        // Shader paths
        if (mat["shader"].valid()) {
            sol::table shader = mat["shader"];
            def.vertexShaderPath = getWithDefault<std::string>(shader, "vertex", "shaders/pbr.vert");
            def.fragmentShaderPath = getWithDefault<std::string>(shader, "fragment", "shaders/pbr.frag");
        }

        // Uniforms
        if (mat["uniforms"].valid()) {
            sol::table uniforms = mat["uniforms"];
            for (auto& [key, val] : uniforms) {
                std::string name = key.as<std::string>();
                if (val.is<double>()) {
                    def.uniforms[name] = static_cast<float>(val.as<double>());
                } else if (val.is<bool>()) {
                    def.uniforms[name] = val.as<bool>();
                } else if (val.is<int>()) {
                    def.uniforms[name] = val.as<int>();
                } else if (val.is<sol::table>()) {
                    sol::table t = val.as<sol::table>();
                    size_t size = t.size();
                    if (size == 2) {
                        def.uniforms[name] = Vec2{
                            getWithDefault<float>(t, 1, 0.0f),
                            getWithDefault<float>(t, 2, 0.0f)
                        };
                    } else if (size == 3) {
                        def.uniforms[name] = Vec3{
                            getWithDefault<float>(t, 1, 0.0f),
                            getWithDefault<float>(t, 2, 0.0f),
                            getWithDefault<float>(t, 3, 0.0f)
                        };
                    } else if (size == 4) {
                        def.uniforms[name] = Vec4{
                            getWithDefault<float>(t, 1, 0.0f),
                            getWithDefault<float>(t, 2, 0.0f),
                            getWithDefault<float>(t, 3, 0.0f),
                            getWithDefault<float>(t, 4, 1.0f)
                        };
                    }
                }
            }
        }

        // Textures
        if (mat["textures"].valid()) {
            sol::table textures = mat["textures"];
            for (auto& [key, val] : textures) {
                def.texturePaths[key.as<std::string>()] = val.as<std::string>();
            }
        }

        // Render state
        def.blendMode = parseBlendMode(getWithDefault<std::string>(mat, "blendMode", "opaque"));
        def.cullMode = parseCullMode(getWithDefault<std::string>(mat, "cullMode", "back"));
        def.depthWrite = getWithDefault<bool>(mat, "depthWrite", true);
        def.depthTest = getWithDefault<bool>(mat, "depthTest", true);
        def.hotReload = getWithDefault<bool>(mat, "hotReload", true);

        return def;
    }

    BlendMode parseBlendMode(const std::string& mode) {
        if (mode == "alphaBlend" || mode == "alpha") return BlendMode::AlphaBlend;
        if (mode == "additive") return BlendMode::Additive;
        if (mode == "multiply") return BlendMode::Multiply;
        if (mode == "alphaTest") return BlendMode::AlphaTest;
        return BlendMode::Opaque;
    }

    CullMode parseCullMode(const std::string& mode) {
        if (mode == "none" || mode == "off") return CullMode::None;
        if (mode == "front") return CullMode::Front;
        return CullMode::Back;
    }
#endif

    //======================================================================
    // Default Shader Sources
    //======================================================================

    static const char* getDefaultPBRVertexSource() {
        return R"(
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec4 aColor;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vTexCoord;
out vec4 vColor;

void main() {
    vec4 worldPos = uModel * vec4(aPosition, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal = normalize(uNormalMatrix * aNormal);
    vTexCoord = aTexCoord;
    vColor = aColor;
    gl_Position = uProjection * uView * worldPos;
}
)";
    }

    static const char* getDefaultPBRFragmentSource() {
        return R"(
#version 330 core
out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform vec4 uBaseColor;
uniform float uMetallic;
uniform float uRoughness;
uniform vec3 uEmissive;
uniform vec3 uCameraPos;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbientColor;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    vec3 albedo = uBaseColor.rgb * vColor.rgb;
    float metallic = uMetallic;
    float roughness = max(uRoughness, 0.04);

    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 L = normalize(-uLightDir);
    vec3 H = normalize(V + L);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    float NdotL = max(dot(N, L), 0.0);
    vec3 Lo = (kD * albedo / PI + specular) * uLightColor * NdotL;

    vec3 ambient = uAmbientColor * albedo;
    vec3 color = ambient + Lo + uEmissive;

    // Tone mapping (Reinhard)
    color = color / (color + vec3(1.0));
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, uBaseColor.a * vColor.a);
}
)";
    }

    static const char* getDefaultUnlitVertexSource() {
        return R"(
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec4 aColor;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec2 vTexCoord;
out vec4 vColor;

void main() {
    vTexCoord = aTexCoord;
    vColor = aColor;
    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
}
)";
    }

    static const char* getDefaultUnlitFragmentSource() {
        return R"(
#version 330 core
out vec4 FragColor;

in vec2 vTexCoord;
in vec4 vColor;

uniform vec4 uColor;
uniform sampler2D uTexture;
uniform bool uHasTexture;

void main() {
    vec4 color = uColor * vColor;
    if (uHasTexture) {
        color *= texture(uTexture, vTexCoord);
    }
    FragColor = color;
}
)";
    }

    static const char* getDebugVertexSource() {
        return R"(
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;

uniform mat4 uViewProjection;

out vec4 vColor;

void main() {
    vColor = aColor;
    gl_Position = uViewProjection * vec4(aPosition, 1.0);
}
)";
    }

    static const char* getDebugFragmentSource() {
        return R"(
#version 330 core
out vec4 FragColor;

in vec4 vColor;

void main() {
    FragColor = vColor;
}
)";
    }

    static const char* getSkyboxVertexSource() {
        return R"(
#version 330 core
layout(location = 0) in vec3 aPosition;

uniform mat4 uViewProjection;

out vec3 vTexCoord;

void main() {
    vTexCoord = aPosition;
    vec4 pos = uViewProjection * vec4(aPosition, 1.0);
    gl_Position = pos.xyww;  // Z = W for maximum depth
}
)";
    }

    static const char* getSkyboxFragmentSource() {
        return R"(
#version 330 core
out vec4 FragColor;

in vec3 vTexCoord;

uniform samplerCube uSkybox;
uniform float uExposure;

void main() {
    vec3 color = texture(uSkybox, vTexCoord).rgb;
    color *= uExposure;
    // Tone mapping
    color = color / (color + vec3(1.0));
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));
    FragColor = vec4(color, 1.0);
}
)";
    }

    //======================================================================
    // Texture Caching Helper
    //======================================================================

    GLuint getOrCreateTexture(AssetHandle handle) {
        // Check cache first
        auto it = textureCache_.find(handle);
        if (it != textureCache_.end()) {
            return it->second;
        }

        // Load texture from asset system
        if (!pIAssetSystem_) return 0;

        // Ensure asset is loaded
        if (pIAssetSystem_->getAssetState(handle) != AssetState::Loaded) {
            pIAssetSystem_->loadAsset(handle);
        }

        if (pIAssetSystem_->getAssetState(handle) != AssetState::Loaded) {
            return 0;  // Failed to load
        }

        // Get texture data
        const TextureData* texData = pIAssetSystem_->getAsset<TextureData>(handle);
        if (!texData || texData->pixels.empty()) {
            return 0;
        }

        // Create OpenGL texture
        GLuint textureId;
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // Determine format based on channels
        GLenum format = GL_RGBA;
        GLenum internalFormat = GL_RGBA;
        switch (texData->channels) {
            case 1:
                format = GL_RED;
                internalFormat = GL_R8;
                break;
            case 2:
                format = GL_RG;
                internalFormat = GL_RG8;
                break;
            case 3:
                format = GL_RGB;
                internalFormat = GL_RGB8;
                break;
            case 4:
            default:
                format = GL_RGBA;
                internalFormat = GL_RGBA8;
                break;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat,
                     texData->width, texData->height, 0,
                     format, GL_UNSIGNED_BYTE, texData->pixels.data());

        // Generate mipmaps
        glGenerateMipmap(GL_TEXTURE_2D);

        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glBindTexture(GL_TEXTURE_2D, 0);

        // Cache the texture
        textureCache_[handle] = textureId;

        return textureId;
    }

    //======================================================================
    // Data Members
    //======================================================================

    std::unordered_map<ShaderProgramHandle, ShaderProgramResource> shaders_;
    std::unordered_map<MaterialHandle, ShaderMaterialResource> materials_;
    std::unordered_map<AssetHandle, GLuint, AssetHandleHash> textureCache_;  // Maps asset handles to OpenGL texture IDs

    std::string shaderBasePath_ = "assets/shaders/";
    std::string materialBasePath_ = "assets/materials/";

    ShaderProgramHandle nextShaderHandle_ = 1;
    MaterialHandle nextMaterialHandle_ = 1;

    ShaderProgramHandle currentShader_ = 0;
    MaterialHandle currentMaterial_ = 0;

    ShaderProgramHandle defaultPBRShader_ = 0;
    ShaderProgramHandle defaultUnlitShader_ = 0;
    ShaderProgramHandle debugShader_ = 0;
    ShaderProgramHandle skyboxShader_ = 0;

    bool hotReloadEnabled_ = true;
    std::string lastCompileError_;

    ShaderReloadCallback shaderReloadCallback_;
    MaterialReloadCallback materialReloadCallback_;

    mutable ShaderStats stats_;

    // Injected dependencies
    IAssetSystem* pIAssetSystem_ = nullptr;
};

// Service definition - must be after class is complete
BESTOW_SERVICE(OpenGLShaderSystem, ShaderSystem, AssetSystem);

}  // namespace bestow
