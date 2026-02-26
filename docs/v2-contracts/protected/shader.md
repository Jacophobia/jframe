# Shader

> **Visibility:** Protected (C++ peer systems only -- not in Lua API)
> **Tier:** 3
> **Dependencies:** Types, Assets
> **Consumers:** Graphics2D, Graphics3D

## Purpose

IShaderCore owns the entire shader compilation pipeline -- from GLSL source text to linked GPU programs. It compiles GLSL to backend-native representations (OpenGL shader objects or SPIR-V bytecode for Vulkan), links vertex/fragment/geometry/compute stages into programs, and provides reflection metadata so that material and rendering systems can discover uniform locations and vertex attribute layouts automatically. When hot reload is active, the Shader system recompiles individual stages in-place and re-links affected programs without tearing down the graphics pipeline. A built-in cache avoids redundant compilations of identical source.

## Contract: `IShaderCore`

### Compilation

| Method | Returns | Description |
|--------|---------|-------------|
| `compileGLSL(std::string_view source, ShaderStage stage)` | `Result<ShaderHandle>` | Compile GLSL source code for the given pipeline stage into a backend-native shader object (OpenGL) or intermediate representation. Returns `ParseError` if compilation fails, with the compiler error log in `SystemError::message`. |
| `compileToSPIRV(std::string_view glslSource, ShaderStage stage)` | `Result<ShaderHandle>` | Compile GLSL source code to SPIR-V bytecode via glslang/shaderc. The resulting handle references SPIR-V data suitable for Vulkan pipeline creation. Returns `ParseError` on compilation failure. |
| `loadSPIRV(std::span<const std::uint32_t> bytecode, ShaderStage stage)` | `Result<ShaderHandle>` | Load pre-compiled SPIR-V bytecode directly. No compilation step -- the bytecode is validated and stored. Returns `InvalidArgument` if the bytecode header is malformed. |

### Program Linking

| Method | Returns | Description |
|--------|---------|-------------|
| `createProgram(ShaderHandle vertex, ShaderHandle fragment)` | `Result<ShaderHandle>` | Link a vertex shader and a fragment shader into a complete graphics program. Returns `InvalidHandle` if either input handle is invalid or has the wrong stage. Returns `ParseError` if linking fails (e.g. mismatched varyings). |
| `createComputeProgram(ShaderHandle compute)` | `Result<ShaderHandle>` | Create a compute program from a single compute shader stage. Returns `InvalidHandle` if the handle is not a compute shader. |
| `destroyShader(ShaderHandle handle)` | `void` | Destroy a shader object or linked program and free its GPU resources. Safe to call with an invalid handle (no-op). Programs that reference a destroyed stage become invalid. |

### Reflection

| Method | Returns | Description |
|--------|---------|-------------|
| `getUniforms(ShaderHandle program) const` | `std::vector<UniformInfo>` | Query all active uniforms in a linked program. Returns an empty vector if the handle is invalid or not a linked program. Used by the material system for automatic uniform binding. |
| `getAttributes(ShaderHandle program) const` | `std::vector<AttributeInfo>` | Query all active vertex attributes in a linked program. Returns an empty vector if the handle is invalid or not a linked program. Used by the mesh rendering pipeline to match vertex buffer layouts. |

### Hot Reload Integration

| Method | Returns | Description |
|--------|---------|-------------|
| `recompile(ShaderHandle handle, std::string_view newSource)` | `Result<void>` | Recompile an existing shader stage with new source code, then re-link any programs that reference it. On success the handle remains valid and all dependent programs are automatically updated. On failure the original compiled shader is preserved (atomic swap). Returns `ParseError` if the new source fails to compile. |

### Cache Management

| Method | Returns | Description |
|--------|---------|-------------|
| `clearCache()` | `void` | Evict all entries from the shader compilation cache. Forces recompilation on the next use of any shader. Primarily useful during development when iterating on include files that affect multiple shaders. |
| `cacheSize() const` | `std::size_t` | Return the number of entries currently held in the compilation cache. Useful for diagnostics and dev overlay display. |

## Types

### ShaderStage

```cpp
enum class ShaderStage : std::uint8_t {
    Vertex,         // Vertex processing stage
    Fragment,       // Fragment/pixel processing stage
    Geometry,       // Geometry processing stage (optional)
    Compute,        // Compute dispatch stage
    TessControl,    // Tessellation control stage (optional)
    TessEval        // Tessellation evaluation stage (optional)
};
```

| Value | Description |
|-------|-------------|
| `Vertex` | Processes each vertex. Required for all graphics programs. |
| `Fragment` | Processes each fragment/pixel. Required for all graphics programs. |
| `Geometry` | Optional stage that processes entire primitives (points, lines, triangles) and can emit new geometry. |
| `Compute` | Standalone compute dispatches. Not part of the graphics pipeline. Used with `createComputeProgram()`. |
| `TessControl` | Controls how much tessellation is applied. Used with `TessEval` for hardware tessellation. |
| `TessEval` | Evaluates the position of each tessellated vertex. Paired with `TessControl`. |

### ShaderHandle

A strong typed handle from the V2 handle system:

```cpp
using ShaderHandle = Handle<struct ShaderTag>;
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `id` | `std::uint64_t` | `0` | Unique identifier. Zero means invalid/null. Assigned by `IShaderCore` during compilation or program creation. |

### UniformInfo

```cpp
struct UniformInfo {
    std::string name;
    std::uint32_t location;
    std::uint32_t binding;
    Type type;
};
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `name` | `std::string` | -- | The uniform name as declared in the GLSL source (e.g. `"u_modelMatrix"`, `"u_albedoMap"`). |
| `location` | `std::uint32_t` | `0` | OpenGL uniform location or Vulkan push-constant offset. |
| `binding` | `std::uint32_t` | `0` | Descriptor set binding point (Vulkan) or texture unit (OpenGL). Meaningful for sampler uniforms. |
| `type` | `UniformInfo::Type` | -- | The data type of the uniform. See `UniformInfo::Type` below. |

#### UniformInfo::Type

```cpp
enum class Type {
    Float,          // Single float (GLSL: float)
    Int,            // Single int (GLSL: int)
    Vec2,           // 2-component float vector (GLSL: vec2)
    Vec3,           // 3-component float vector (GLSL: vec3)
    Vec4,           // 4-component float vector (GLSL: vec4)
    Mat3,           // 3x3 float matrix (GLSL: mat3)
    Mat4,           // 4x4 float matrix (GLSL: mat4)
    Sampler2D,      // 2D texture sampler (GLSL: sampler2D)
    SamplerCube     // Cube texture sampler (GLSL: samplerCube)
};
```

| Value | Description |
|-------|-------------|
| `Float` | A single 32-bit floating point uniform. |
| `Int` | A single 32-bit integer uniform. |
| `Vec2` | A 2-component float vector. Maps to `bestow::Vec2`. |
| `Vec3` | A 3-component float vector. Maps to `bestow::Vec3` or `bestow::Color` (RGB). |
| `Vec4` | A 4-component float vector. Maps to `bestow::Vec4` or `bestow::Color` (RGBA). |
| `Mat3` | A 3x3 float matrix. Maps to `bestow::Mat3`. Commonly used for normal matrices. |
| `Mat4` | A 4x4 float matrix. Maps to `bestow::Mat4`. Model, view, and projection matrices. |
| `Sampler2D` | A 2D texture sampler. The `binding` field indicates the texture unit or descriptor binding. |
| `SamplerCube` | A cubemap texture sampler. Used for skyboxes, environment maps, and reflection probes. |

### AttributeInfo

```cpp
struct AttributeInfo {
    std::string name;
    std::uint32_t location;
    std::uint32_t components;
};
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `name` | `std::string` | -- | The attribute name as declared in the GLSL source (e.g. `"a_position"`, `"a_normal"`, `"a_texCoord"`). |
| `location` | `std::uint32_t` | `0` | The vertex attribute location (`layout(location = N)`). Used to match vertex buffer bindings. |
| `components` | `std::uint32_t` | `0` | Number of components (1-4). A `vec3` attribute has 3 components, a `float` has 1. |

## Examples

### Compiling and linking a shader program

```cpp
void RenderPipeline::createPBRProgram(IShaderCore& shaders, IAssetCore& assets) {
    // Load shader source through AssetSystem
    auto vertAsset = assets.loadShader(":library:/shaders/pbr.vert");
    auto fragAsset = assets.loadShader(":library:/shaders/pbr.frag");

    if (!vertAsset || !fragAsset) {
        LOG_ERROR("Failed to load PBR shader sources");
        return;
    }

    const ShaderData* vertSrc = assets.getShaderData(*vertAsset);
    const ShaderData* fragSrc = assets.getShaderData(*fragAsset);

    // Compile individual stages
    auto vertShader = shaders.compileGLSL(vertSrc->source, ShaderStage::Vertex);
    if (!vertShader) {
        LOG_ERROR("Vertex compilation failed: {}", vertShader.error().message);
        return;
    }

    auto fragShader = shaders.compileGLSL(fragSrc->source, ShaderStage::Fragment);
    if (!fragShader) {
        LOG_ERROR("Fragment compilation failed: {}", fragShader.error().message);
        return;
    }

    // Link into a program
    auto program = shaders.createProgram(*vertShader, *fragShader);
    if (!program) {
        LOG_ERROR("Program linking failed: {}", program.error().message);
        return;
    }

    pbrProgram_ = *program;

    // Discover uniforms for automatic material binding
    auto uniforms = shaders.getUniforms(pbrProgram_);
    for (const auto& u : uniforms) {
        uniformLocations_[u.name] = u.location;
    }
}
```

### SPIR-V compilation for Vulkan

```cpp
void VulkanPipeline::createShaderModules(IShaderCore& shaders, IAssetCore& assets) {
    const ShaderData* vertSrc = assets.getShaderData(vertAssetHandle_);

    // Compile GLSL to SPIR-V
    auto vertSpirv = shaders.compileToSPIRV(vertSrc->source, ShaderStage::Vertex);
    if (!vertSpirv) {
        LOG_ERROR("SPIR-V compilation failed: {}", vertSpirv.error().message);
        return;
    }

    // Or load pre-compiled SPIR-V directly
    auto precompiled = assets.loadShaderCompiled(":assets:/shaders/pbr.frag.spv");
    if (precompiled) {
        const ShaderData* spvData = assets.getShaderData(*precompiled);
        auto fragSpirv = shaders.loadSPIRV(
            std::span{spvData->spirv}, ShaderStage::Fragment);
        // ... create Vulkan pipeline with both modules
    }
}
```

### Hot reload recompilation

```cpp
void ShaderManager::onShaderFileChanged(
    IShaderCore& shaders, IAssetCore& assets,
    AssetHandle changedAsset
) {
    const ShaderData* newSource = assets.getShaderData(changedAsset);
    if (!newSource) return;

    // Find which compiled shader corresponds to this asset
    auto it = assetToShaderMap_.find(changedAsset.uuid);
    if (it == assetToShaderMap_.end()) return;

    ShaderHandle shaderHandle = it->second;

    // Recompile in-place -- on failure, the old shader is preserved
    auto result = shaders.recompile(shaderHandle, newSource->source);
    if (!result) {
        LOG_WARN("Hot reload recompilation failed: {}", result.error().message);
    } else {
        LOG_INFO("Shader hot-reloaded successfully");
    }
}
```

### Querying reflection data for material binding

```cpp
void MaterialSystem::bindMaterial(
    IShaderCore& shaders, ShaderHandle program,
    const std::unordered_map<std::string, UniformValue>& uniforms
) {
    auto programUniforms = shaders.getUniforms(program);

    for (const auto& info : programUniforms) {
        auto it = uniforms.find(info.name);
        if (it == uniforms.end()) continue;

        // Bind based on reflected type
        std::visit([&](const auto& value) {
            setUniform(info.location, value);
        }, it->second);
    }

    auto attributes = shaders.getAttributes(program);
    for (const auto& attr : attributes) {
        configureVertexAttribute(attr.location, attr.components);
    }
}
```
