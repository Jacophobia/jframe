// bestow-contract/src/bestow.shader.cppm
// Dynamic shader system interface with hot reload and Lua material support

module;

#include <string>
#include <functional>
#include <optional>
#include <span>
#include <variant>

export module bestow.shader;

import bestow.types;
import bestow.assets;

export namespace bestow {

//==========================================================================
// Shader Handle Types
//==========================================================================

using ShaderHandle = std::uint64_t;
using ShaderProgramHandle = std::uint64_t;
// Note: MaterialHandle is already defined in bestow.types

namespace ShaderHandles {
    inline constexpr ShaderHandle Invalid = 0;
}

//==========================================================================
// Uniform Types
//==========================================================================

/// Uniform value variant - supports all common uniform types
using UniformValue = std::variant<
    bool,
    int,
    float,
    Vec2,
    Vec3,
    Vec4,
    Mat3,
    Mat4,
    std::vector<float>,     // Float array
    std::vector<Vec2>,      // Vec2 array
    std::vector<Vec3>,      // Vec3 array
    std::vector<Vec4>,      // Vec4 array
    std::vector<Mat4>       // Mat4 array (for bone transforms)
>;

/// Uniform descriptor for introspection
struct UniformInfo {
    std::string name;
    std::uint32_t location;
    std::uint32_t type;        // GL type enum
    std::uint32_t count;       // Array size (1 for non-arrays)
};

//==========================================================================
// Shader Definition Types
//==========================================================================

enum class ShaderStage : std::uint8_t {
    Vertex,
    Fragment,
    Geometry,
    TessControl,
    TessEvaluation,
    Compute
};

struct ShaderSourceDef {
    ShaderStage stage;
    std::string source;             // Inline source
    std::string filePath;           // Or file path (mutually exclusive)

    bool isFromFile() const { return !filePath.empty(); }
};

struct ShaderProgramDef {
    std::string name;               // For debugging
    std::vector<ShaderSourceDef> stages;
    bool enableHotReload = true;    // Watch files for changes
};

//==========================================================================
// Material Types
//==========================================================================

/// Texture slot assignment
struct TextureBinding {
    std::uint32_t slot;             // Texture unit
    std::string samplerName;        // Uniform sampler name
    AssetHandle texture;            // Texture asset
};

/// Material definition
struct ShaderMaterialDef {
    std::string name;
    ShaderProgramHandle shader;
    std::unordered_map<std::string, UniformValue> uniforms;
    std::vector<TextureBinding> textures;

    BlendMode blendMode = BlendMode::Opaque;
    CullMode cullMode = CullMode::Back;
    bool depthWrite = true;
    bool depthTest = true;
};

/// Lua-loaded material definition
struct LuaMaterialDef {
    std::string vertexShaderPath;
    std::string fragmentShaderPath;
    std::unordered_map<std::string, UniformValue> uniforms;
    std::unordered_map<std::string, std::string> texturePaths;  // slot name -> texture path

    BlendMode blendMode = BlendMode::Opaque;
    CullMode cullMode = CullMode::Back;
    bool depthWrite = true;
    bool depthTest = true;
    bool hotReload = true;
};

//==========================================================================
// Error Types
//==========================================================================

enum class ShaderError {
    Success,
    FileNotFound,
    CompilationFailed,
    LinkingFailed,
    InvalidHandle,
    InvalidUniform,
    InvalidTexture,
    LuaParseError,
    InternalError
};

struct ShaderCompileError {
    ShaderError error;
    std::string message;
    std::string filePath;
    std::uint32_t line = 0;
};

//==========================================================================
// Hot Reload Callback
//==========================================================================

using ShaderReloadCallback = std::function<void(ShaderProgramHandle shader, bool success, const std::string& error)>;
using MaterialReloadCallback = std::function<void(MaterialHandle material, bool success, const std::string& error)>;

//==========================================================================
// IShaderSystem Interface
//==========================================================================

class IShaderSystem {
public:
    virtual ~IShaderSystem() = default;

    //======================================================================
    // Shader Program Management
    //======================================================================

    /// Compile shader from source strings
    virtual Result<ShaderProgramHandle, ShaderCompileError> createShaderFromSource(
        std::string_view vertexSource,
        std::string_view fragmentSource,
        std::string_view name = "") = 0;

    /// Compile shader from file paths (enables hot reload)
    virtual Result<ShaderProgramHandle, ShaderCompileError> loadShader(
        std::string_view vertexPath,
        std::string_view fragmentPath,
        bool enableHotReload = true) = 0;

    /// Create shader from full definition
    virtual Result<ShaderProgramHandle, ShaderCompileError> createShader(const ShaderProgramDef& def) = 0;

    /// Destroy shader program
    virtual void destroyShader(ShaderProgramHandle handle) = 0;

    /// Check if shader exists
    virtual bool hasShader(ShaderProgramHandle handle) const = 0;

    /// Get shader name (for debugging)
    virtual std::string getShaderName(ShaderProgramHandle handle) const = 0;

    //======================================================================
    // Shader Binding
    //======================================================================

    /// Bind shader for rendering
    virtual void bindShader(ShaderProgramHandle handle) = 0;

    /// Unbind current shader
    virtual void unbindShader() = 0;

    /// Get currently bound shader
    virtual ShaderProgramHandle getCurrentShader() const = 0;

    //======================================================================
    // Uniform Management
    //======================================================================

    /// Set uniform value on currently bound shader
    virtual void setUniform(std::string_view name, const UniformValue& value) = 0;

    /// Set uniform value by location (faster if you cache locations)
    virtual void setUniform(std::uint32_t location, const UniformValue& value) = 0;

    /// Get uniform location (returns -1 if not found)
    virtual std::int32_t getUniformLocation(ShaderProgramHandle shader, std::string_view name) const = 0;

    /// Get all uniform info for a shader (for introspection/debugging)
    virtual std::vector<UniformInfo> getUniformInfo(ShaderProgramHandle shader) const = 0;

    //======================================================================
    // Material Management
    //======================================================================

    /// Create material from definition
    virtual Result<MaterialHandle, ShaderError> createMaterial(const ShaderMaterialDef& def) = 0;

    /// Load material from Lua file
    virtual Result<MaterialHandle, ShaderCompileError> loadMaterial(std::string_view luaPath) = 0;

    /// Create material from shader (bare material, no preset uniforms)
    virtual Result<MaterialHandle, ShaderError> createMaterial(ShaderProgramHandle shader, std::string_view name = "") = 0;

    /// Clone existing material
    virtual Result<MaterialHandle, ShaderError> cloneMaterial(MaterialHandle source, std::string_view newName = "") = 0;

    /// Destroy material
    virtual void destroyMaterial(MaterialHandle handle) = 0;

    /// Check if material exists
    virtual bool hasMaterial(MaterialHandle handle) const = 0;

    /// Get material name
    virtual std::string getMaterialName(MaterialHandle handle) const = 0;

    //======================================================================
    // Material Property Setting
    //======================================================================

    /// Set material uniform (stored until material is bound)
    virtual void setMaterialUniform(MaterialHandle handle, std::string_view name, const UniformValue& value) = 0;

    /// Set material texture
    virtual void setMaterialTexture(MaterialHandle handle, std::string_view samplerName, AssetHandle texture) = 0;

    /// Set material texture by slot
    virtual void setMaterialTexture(MaterialHandle handle, std::uint32_t slot, AssetHandle texture) = 0;

    /// Set blend mode
    virtual void setMaterialBlendMode(MaterialHandle handle, BlendMode mode) = 0;

    /// Set cull mode
    virtual void setMaterialCullMode(MaterialHandle handle, CullMode mode) = 0;

    /// Set depth settings
    virtual void setMaterialDepthSettings(MaterialHandle handle, bool depthWrite, bool depthTest) = 0;

    //======================================================================
    // Material Binding
    //======================================================================

    /// Bind material (binds shader, sets uniforms, binds textures)
    virtual void bindMaterial(MaterialHandle handle) = 0;

    /// Get currently bound material
    virtual MaterialHandle getCurrentMaterial() const = 0;

    //======================================================================
    // Built-in Shaders
    //======================================================================

    /// Get default PBR shader
    virtual ShaderProgramHandle getDefaultPBRShader() const = 0;

    /// Get default unlit shader
    virtual ShaderProgramHandle getDefaultUnlitShader() const = 0;

    /// Get debug/wireframe shader
    virtual ShaderProgramHandle getDebugShader() const = 0;

    /// Get skybox shader
    virtual ShaderProgramHandle getSkyboxShader() const = 0;

    //======================================================================
    // Hot Reload
    //======================================================================

    /// Enable/disable hot reload globally
    virtual void setHotReloadEnabled(bool enabled) = 0;

    /// Check if hot reload is enabled
    virtual bool isHotReloadEnabled() const = 0;

    /// Check for file changes and reload modified shaders
    virtual void update() = 0;

    /// Force reload a specific shader
    virtual Result<void, ShaderCompileError> reloadShader(ShaderProgramHandle handle) = 0;

    /// Force reload a specific material (reloads Lua and shaders)
    virtual Result<void, ShaderCompileError> reloadMaterial(MaterialHandle handle) = 0;

    /// Register callback for shader reloads
    virtual void setShaderReloadCallback(ShaderReloadCallback callback) = 0;

    /// Register callback for material reloads
    virtual void setMaterialReloadCallback(MaterialReloadCallback callback) = 0;

    //======================================================================
    // Asset Integration
    //======================================================================

    /// Set asset system for texture loading
    virtual void setAssetSystem(IAssetSystem* assets) = 0;

    /// Set base path for shader files (default: "assets/shaders/")
    virtual void setShaderBasePath(std::string_view path) = 0;

    /// Set base path for material files (default: "assets/materials/")
    virtual void setMaterialBasePath(std::string_view path) = 0;

    //======================================================================
    // Statistics
    //======================================================================

    struct ShaderStats {
        std::uint32_t shaderCount = 0;
        std::uint32_t materialCount = 0;
        std::uint32_t shaderBinds = 0;      // Per frame
        std::uint32_t materialBinds = 0;    // Per frame
        std::uint32_t uniformSets = 0;      // Per frame
        std::uint32_t hotReloads = 0;       // Total reloads
    };

    virtual ShaderStats getStats() const = 0;
    virtual void resetFrameStats() = 0;
};

}  // namespace bestow
