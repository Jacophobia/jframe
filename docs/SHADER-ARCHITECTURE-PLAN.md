# Shader Architecture Refactor Plan

## Goal

Unify shader loading and compilation under AssetSystem with:
- Event-driven hot reload (no circular dependencies)
- Minimal public API (compilation is internal detail)
- Single flow for ALL graphics backends (OpenGL 2D, OpenGL 3D, Vulkan 3D)
- GLSL-only interface (SPIR-V is internal implementation detail)
- Hash-based caching for compiled shaders

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              EventSystem                                     │
│                                                                             │
│  Events::FileChanged      → AssetSystem subscribes                          │
│  Events::AssetReloaded    → GraphicsSystems subscribe                       │
└─────────────────────────────────────────────────────────────────────────────┘
         ▲                              ▲                           ▲
         │ emit                         │ emit                      │ subscribe
         │                              │                           │
┌────────┴────────┐            ┌────────┴────────┐         ┌────────┴────────┐
│ FileWatcherSys  │            │   AssetSystem   │         │ Graphics System │
│                 │            │                 │         │ (any backend)   │
│ - watches dirs  │            │ - loads GLSL    │         │                 │
│ - detects mods  │            │ - compiles      │         │ - subscribes to │
│                 │            │   internally    │         │   AssetReloaded │
│                 │            │ - caches SPIRV  │         │ - fetches data  │
│                 │            │ - emits events  │         │ - uploads to GPU│
└─────────────────┘            └─────────────────┘         └─────────────────┘
                                        ▲
                                        │ loadShader() / loadShaderCompiled()
                                        │ getShaderData()
                                        │
                               ┌────────┴────────┐
                               │ Graphics System │
                               │ (one-way dep)   │
                               └─────────────────┘
```

### Key Principle: No Circular Dependencies

- **GraphicsSystem → AssetSystem**: Direct dependency (fetching shaders)
- **GraphicsSystem → EventSystem**: Direct dependency (subscribing)
- **AssetSystem → EventSystem**: Direct dependency (emitting)
- **AssetSystem → FileWatcherSystem**: Via EventSystem only
- **NO**: AssetSystem → GraphicsSystem (broken by events)

---

## Minimal Public API

### IAssetSystem Contract Changes

```cpp
class IAssetSystem {
public:
    // ... existing methods ...

    //======================================================================
    // Shader Loading (MINIMAL API)
    //======================================================================

    /// Load a GLSL shader (source only - for OpenGL)
    /// Stage inferred from extension: .vert, .frag, .geom, .comp, .tesc, .tese
    /// WARNING: .spv files are NOT supported - provide GLSL source
    virtual AssetHandle loadShader(const std::filesystem::path& glslPath) = 0;

    /// Load a GLSL shader with compilation (source + SPIR-V - for Vulkan)
    /// Compilation is cached - only recompiles if source hash changes
    /// WARNING: .spv files are NOT supported - provide GLSL source
    virtual AssetHandle loadShaderCompiled(const std::filesystem::path& glslPath) = 0;

    /// Get shader data (source always present, SPIR-V only if loadShaderCompiled was used)
    virtual const ShaderData* getShaderData(AssetHandle handle) const = 0;

    // REMOVED: compileShaderAsync() - no public compilation API
    // REMOVED: isShaderCompilationSupported() - internal detail
};
```

### What's NOT Exposed

- `compileShaderAsync()` - removed from public API
- `compileGlslToSpirv()` - internal function only
- `isShaderCompilationSupported()` - internal detail
- Any SPIR-V loading methods - we only accept GLSL

---

## ShaderData Structure

```cpp
struct ShaderData {
    std::string glslSource;                    // Always populated (original GLSL)
    std::vector<std::uint32_t> spirvBytecode;  // Only populated if loadShaderCompiled() was used
    std::string path;                          // Source file path
    std::string entryPoint = "main";

    enum class Stage { Vertex, Fragment, Geometry, Compute, TessControl, TessEval };
    Stage stage = Stage::Vertex;

    // Internal status (not for client use)
    bool compiled = false;
    std::string compileError;    // Error message if compilation failed
    std::uint64_t sourceHash;    // For cache invalidation
};
```

---

## Cache System

### Cache Location
```
<project>/
├── assets/
│   └── shaders/
│       ├── pbr.vert              # User's GLSL source
│       └── pbr.frag
└── .shader_cache/                # Auto-created by AssetSystem
    ├── pbr.vert.spv              # Compiled SPIR-V
    ├── pbr.vert.meta             # Metadata JSON
    ├── pbr.frag.spv
    └── pbr.frag.meta
```

### Cache Metadata (.meta file)
```json
{
    "sourceHash": "a1b2c3d4e5f6...",
    "sourcePath": "assets/shaders/pbr.vert",
    "compileTime": "2024-01-15T10:30:00Z",
    "shadercVersion": "2024.0"
}
```

### Cache Invalidation Logic

```cpp
AssetHandle loadShaderCompiled(path) {
    1. Read GLSL source
    2. Compute hash = std::hash<std::string>{}(source)
    3. Check cache:
       - If .meta exists AND meta.sourceHash == hash:
         → Load cached .spv (fast path)
       - Else:
         → Compile with shaderc
         → Write .spv and .meta to cache
    4. Return handle
}
```

---

## Event Flow

### Initialization Sequence

```
1. GraphicsSystem::init()
   │
   ├─▶ pIEventSystem_->subscribe(Events::AssetReloaded, [this](const EventData& e) {
   │       auto& data = std::get<AssetEventData>(e);
   │       if (data.type == AssetType::Shader) {
   │           onShaderReloaded(data.handle);
   │       }
   │   });
   │
   └─▶ vertHandle_ = pIAssetSystem_->loadShaderCompiled("shaders/basic.vert");
       fragHandle_ = pIAssetSystem_->loadShaderCompiled("shaders/basic.frag");

       // Get data and upload to GPU
       auto* vertData = pIAssetSystem_->getShaderData(vertHandle_);
       auto* fragData = pIAssetSystem_->getShaderData(fragHandle_);
       uploadShadersToGPU(vertData, fragData);
```

### Hot Reload Sequence

```
1. User edits basic.vert in their editor

2. FileWatcherSystem detects modification
   └─▶ pIEventSystem_->publish(Events::FileChanged, FileChangeEventData{
           .path = "shaders/basic.vert",
           .fileType = "shader"
       });

3. AssetSystem::onFileChanged() (subscribed to Events::FileChanged)
   │
   ├─▶ Find asset handle by path
   ├─▶ Re-read GLSL source
   ├─▶ Recompile with shaderc (if was compiled)
   ├─▶ Update cache
   └─▶ pIEventSystem_->publish(Events::AssetReloaded, AssetEventData{
           .handle = handle,
           .type = AssetType::Shader,
           .state = AssetState::Loaded
       });

4. GraphicsSystem::onShaderReloaded() (subscribed to Events::AssetReloaded)
   │
   ├─▶ auto* data = pIAssetSystem_->getShaderData(handle);
   └─▶ reuploadShaderToGPU(handle, data);
```

---

## .spv Warning

When a user tries to load a `.spv` file directly:

```cpp
AssetHandle AssetSystem::loadShader(const std::filesystem::path& path) {
    if (path.extension() == ".spv") {
        spdlog::warn(
            "Direct SPIR-V loading is not supported: '{}'. "
            "Please provide GLSL source (.vert, .frag, .geom, .comp). "
            "The engine compiles to SPIR-V internally when needed.",
            path.string()
        );
        return AssetHandle::invalid();
    }
    // ... normal loading
}

AssetHandle AssetSystem::loadShaderCompiled(const std::filesystem::path& path) {
    // Same warning check
}
```

---

## Files to Modify

### Contracts (bestow-contract)

| File | Change |
|------|--------|
| `bestow.assets.cppm` | Add `loadShaderCompiled()`, remove `compileShaderAsync()`, `isShaderCompilationSupported()` |
| `bestow.shader.cppm` | **DELETE** - IShaderSystem absorbed into AssetSystem |
| `bestow.events.cppm` | Already has `Events::AssetReloaded`, `Events::FileChanged` - no changes needed |
| `bestow.cppm` | Remove `export import bestow.shader;` |

### AssetSystem (bestow-assets)

| File | Change |
|------|--------|
| `AssetSystem.cpp` | Add `loadShaderCompiled()`, caching logic, .spv warning, file watcher subscription |
| `CMakeLists.txt` | Ensure shaderc dependency (already present) |

### Vulkan (bestow-vulkan)

| File | Change |
|------|--------|
| `CMakeLists.txt` | **REMOVE** glslc/glslangValidator subprocess compilation |
| `VulkanGraphics3DSystem.cpp` | Use `loadShaderCompiled()` + event subscription |

### OpenGL 3D (bestow-graphics3d)

| File | Change |
|------|--------|
| Implementation | Use `loadShader()` (source only) + event subscription |

### OpenGL 2D (bestow-shader)

| File | Change |
|------|--------|
| Repurpose | Keep GPU upload logic, remove file I/O (use AssetSystem) |

---

## Backend-Specific Behavior

| Backend | loadShader() | loadShaderCompiled() | GPU Upload |
|---------|--------------|----------------------|------------|
| **OpenGL 2D** | ✅ Use this | ❌ Unnecessary | `glShaderSource` + `glCompileShader` |
| **OpenGL 3D** | ✅ Use this | ❌ Unnecessary | `glShaderSource` + `glCompileShader` |
| **Vulkan 3D** | ❌ No SPIR-V | ✅ Use this | `vkCreateShaderModule` with SPIR-V |

OpenGL compiles GLSL on the GPU driver, so it only needs source.
Vulkan requires pre-compiled SPIR-V, so it uses `loadShaderCompiled()`.

---

## Migration Checklist

### Phase 1: Contract Updates
- [ ] Remove `compileShaderAsync()` from IAssetSystem
- [ ] Remove `isShaderCompilationSupported()` from IAssetSystem
- [ ] Add `loadShaderCompiled()` to IAssetSystem
- [ ] Delete `bestow.shader.cppm` contract
- [ ] Update `bestow.cppm` to remove shader module export

### Phase 2: AssetSystem Implementation
- [ ] Implement `loadShaderCompiled()` with caching
- [ ] Add .spv file warning
- [ ] Subscribe to `Events::FileChanged` for hot reload
- [ ] Emit `Events::AssetReloaded` on shader reload
- [ ] Implement hash-based cache invalidation

### Phase 3: Graphics System Updates
- [ ] VulkanGraphics3DSystem: Use `loadShaderCompiled()` + event subscription
- [ ] OpenGL 3D: Use `loadShader()` + event subscription
- [ ] OpenGL 2D (bestow-shader): Refactor to use AssetSystem for file I/O

### Phase 4: Cleanup
- [ ] Remove Vulkan CMake subprocess shader compilation
- [ ] Remove IShaderSystem dependency from VulkanGraphics3D
- [ ] Delete bestow-shader system if fully absorbed

---

## Questions Resolved

1. **Where does compilation logic live?** → AssetSystem (internal)
2. **What does GraphicsSystem receive?** → ShaderData (source + optional SPIR-V)
3. **Who owns the shader compilation strategy?** → AssetSystem decides based on which method was called
4. **How to avoid circular deps?** → EventSystem as intermediary
5. **How to handle cache invalidation?** → Hash GLSL source, compare to cached hash
