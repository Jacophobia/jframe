# Asset System Guide

## Table of Contents

1. [Overview](#overview)
2. [Core Concepts](#core-concepts)
3. [API Reference](#api-reference)
4. [Hot Reload](#hot-reload)
5. [Asset Types](#asset-types)
6. [Shader System](#shader-system)
7. [3D Asset Loading](#3d-asset-loading)
8. [Best Practices](#best-practices)
9. [Code Examples](#code-examples)

---

## Overview

The **Asset System** is the **sole gateway to the file system** in Bestow. This is a fundamental architectural principle:

**ALL file I/O MUST go through the Asset System.**

This centralized approach provides:

- **Unified caching** - Assets loaded once, reused everywhere
- **Hot reload support** - Automatic file watching and reloading during development
- **Lifecycle management** - Proper loading, unloading, and error handling
- **Path resolution** - Consistent handling of `:library:/` and `:assets:/` path prefixes
- **Async loading** - Non-blocking asset loads on background threads
- **Type safety** - Strongly-typed asset handles and data structures

### Why Centralize File I/O?

Other systems (Graphics, Audio, Shader, Level, etc.) **must NOT** perform direct file operations. Instead, they interact with assets through the Asset System:

```cpp
// WRONG - Direct file I/O bypasses the asset system
std::ifstream file(path);
lua.safe_script_file(path);
std::filesystem::exists(path);

// CORRECT - Through AssetSystem
AssetHandle handle = assets->registerAsset(AssetType::Shader, path);
assets->loadAsset(handle);
const ShaderData* data = assets->getAsset<ShaderData>(handle);
```

**Exception:** The SaveSystem is intentionally exempt from this rule because save files are user-generated data (not game assets) and require write operations.

---

## Core Concepts

### AssetHandle

An **opaque reference** to an asset. Handles are lightweight, copyable, and used to identify assets throughout their lifetime.

```cpp
struct AssetHandle {
    UUID uuid;        // Unique identifier
    AssetType type;   // Asset type discriminator

    bool isValid() const { return uuid != 0; }
    static AssetHandle invalid() { return {0, AssetType::Data}; }
};
```

Handles use the UUID to look up assets internally and include the type for validation and type-safe access.

### AssetType

Enum defining supported asset categories:

```cpp
enum class AssetType {
    Texture,        // PNG, JPG, BMP images
    Sound,          // WAV, OGG, MP3 audio (short sounds)
    Music,          // WAV, OGG, MP3 audio (streamed music)
    Font,           // TTF, OTF font files
    Shader,         // GLSL shader source (.vert, .frag, .geom, .comp)
    Data,           // JSON, Lua, or text configuration files
    Level,          // Lua level definitions
    Mesh,           // OBJ, glTF mesh geometry
    Model,          // glTF, FBX models with materials
    Material,       // Lua material definitions
    Cubemap,        // Cubemap textures (6 faces or equirectangular)
    NavMesh,        // Navigation mesh binary data
    BehaviorTree    // AI behavior tree definitions (JSON)
};
```

### AssetState

Represents the current loading state of an asset:

```cpp
enum class AssetState {
    Unloaded,   // Not yet loaded or explicitly unloaded
    Loading,    // Currently being loaded (async only)
    Loaded,     // Successfully loaded and ready to use
    Failed      // Load failed (see AssetMetadata.errorMessage)
};
```

### Path Schemes

The Asset System supports special path prefixes:

- **`:assets:/`** - Resolves to the game's asset directory (e.g., `game1/data/`)
- **`:library:/`** - Resolves to the engine's asset library (e.g., `bestow-assets/library/`)
- **Relative/absolute paths** - Used as-is

```cpp
// Load from game assets
AssetHandle playerTexture = assets->registerAsset(
    AssetType::Texture,
    ":assets:/textures/player.png"
);

// Load from engine library
AssetHandle toonShader = assets->registerAsset(
    AssetType::Shader,
    ":library:/shaders/toon.frag"
);
```

### AssetMetadata

Provides information about a registered asset:

```cpp
struct AssetMetadata {
    AssetHandle handle;                          // Asset handle
    std::filesystem::path sourcePath;            // Original file path
    AssetState state;                            // Current state
    std::size_t sizeBytes;                       // Size in memory
    std::optional<std::string> errorMessage;     // Error details if Failed
};
```

---

## API Reference

### Lifecycle

#### `void update()`

Call this every frame in your main loop to:
- Process completed async loads and invoke callbacks
- Handle file change events from the hot reload watcher

```cpp
void gameLoop() {
    while (running) {
        assets->update();  // Must be called every frame
        // ... rest of game loop
    }
}
```

### Registration

#### `AssetHandle registerAsset(AssetType type, const std::filesystem::path& path)`

Registers an asset for tracking. Does **not** load the asset - it only creates a handle and associates it with a file path.

```cpp
AssetHandle texture = assets->registerAsset(AssetType::Texture, "textures/player.png");
ASSERT(texture.isValid());
ASSERT(texture.type == AssetType::Texture);
```

**Returns:** Valid `AssetHandle` with a unique UUID

**Note:** The same path can be registered multiple times, creating separate handles.

#### `void unregisterAsset(AssetHandle handle)`

Removes an asset from tracking. If the asset is loaded, it will be unloaded first.

```cpp
assets->unregisterAsset(handle);
```

### Loading

#### `void loadAsset(AssetHandle handle)`

**Synchronously** loads an asset. Blocks until loading completes (success or failure).

```cpp
AssetHandle texture = assets->registerAsset(AssetType::Texture, "textures/enemy.png");
assets->loadAsset(texture);  // Blocks until loaded

if (assets->isLoaded(texture)) {
    const TextureData* data = assets->getAsset<TextureData>(texture);
    // Use texture data...
}
```

**Use case:** Preloading critical assets during initialization or level load screens.

#### `void loadAssetAsync(AssetHandle handle, AssetLoadCallback callback = nullptr)`

**Asynchronously** loads an asset on a background thread. Non-blocking.

```cpp
using AssetLoadCallback = std::function<void(AssetHandle, AssetState)>;
```

The callback is invoked on the **main thread** (during `update()`) when loading completes.

```cpp
AssetHandle sound = assets->registerAsset(AssetType::Sound, "sounds/jump.wav");

assets->loadAssetAsync(sound, [this](AssetHandle h, AssetState state) {
    if (state == AssetState::Loaded) {
        const SoundData* data = assets->getAsset<SoundData>(h);
        audioSystem->createSound(h, data);
    } else {
        // Handle error
        AssetMetadata meta = assets->getAssetMetadata(h);
        if (meta.errorMessage) {
            logError("Failed to load asset: {}", *meta.errorMessage);
        }
    }
});

// Continue execution immediately, callback invoked later during update()
```

**Use case:** Loading non-critical assets without freezing gameplay.

#### `void unloadAsset(AssetHandle handle)`

Unloads an asset, freeing its memory. The handle remains valid and can be reloaded later.

```cpp
assets->unloadAsset(handle);
ASSERT(assets->getAssetState(handle) == AssetState::Unloaded);
```

### State Queries

#### `AssetState getAssetState(AssetHandle handle) const`

Returns the current loading state.

```cpp
AssetState state = assets->getAssetState(handle);

switch (state) {
    case AssetState::Unloaded:
        // Asset not loaded yet
        break;
    case AssetState::Loading:
        // Async load in progress
        break;
    case AssetState::Loaded:
        // Ready to use
        break;
    case AssetState::Failed:
        // Load failed - check errorMessage
        break;
}
```

#### `bool isLoaded(AssetHandle handle) const`

Convenience method - returns `true` if state is `Loaded`.

```cpp
if (assets->isLoaded(handle)) {
    // Safe to access asset data
}
```

#### `AssetMetadata getAssetMetadata(AssetHandle handle) const`

Returns full metadata including error messages.

```cpp
AssetMetadata meta = assets->getAssetMetadata(handle);
logInfo("Asset: {} - State: {} - Size: {} bytes",
    meta.sourcePath.string(),
    static_cast<int>(meta.state),
    meta.sizeBytes
);

if (meta.state == AssetState::Failed && meta.errorMessage) {
    logError("Error: {}", *meta.errorMessage);
}
```

### Data Access

#### `T* getAsset<T>(AssetHandle handle)`

Type-safe access to loaded asset data. Returns `nullptr` if asset is not loaded or types don't match.

```cpp
// Access texture data
const TextureData* texture = assets->getAsset<TextureData>(handle);
if (texture) {
    int width = texture->width;
    int height = texture->height;
    const unsigned char* pixels = texture->pixels.data();
}

// Access shader data
const ShaderData* shader = assets->getAsset<ShaderData>(shaderHandle);
if (shader) {
    std::string source = shader->glslSource;
    bool compiled = shader->compiled;
}
```

**Available types:**
- `TextureData` - For `AssetType::Texture`
- `SoundData` - For `AssetType::Sound`, `AssetType::Music`
- `FontData` - For `AssetType::Font`
- `ShaderData` - For `AssetType::Shader`
- `DataAsset` - For `AssetType::Data`, `AssetType::Level`
- `MeshData` - For `AssetType::Mesh`
- `ModelData` - For `AssetType::Model`
- `MaterialData` - For `AssetType::Material`
- `CubemapData` - For `AssetType::Cubemap`
- `NavMeshData` - For `AssetType::NavMesh`

#### `void* getRawAsset(AssetHandle handle)`

Untyped access to asset data. Prefer `getAsset<T>()` for type safety.

### Bulk Operations

#### `void loadAll()`

Synchronously loads **all** registered assets. Useful for preloading at startup.

```cpp
// Register all needed assets
assets->registerAsset(AssetType::Texture, "textures/player.png");
assets->registerAsset(AssetType::Texture, "textures/enemy.png");
assets->registerAsset(AssetType::Sound, "sounds/jump.wav");

// Load everything at once
assets->loadAll();
```

**Warning:** This blocks until all assets are loaded. Use with caution.

#### `void unloadAll()`

Unloads all assets, freeing memory. Handles remain valid.

```cpp
assets->unloadAll();
```

#### `std::vector<AssetHandle> getAssetsOfType(AssetType type) const`

Returns all registered assets of a specific type.

```cpp
// Get all textures
std::vector<AssetHandle> textures = assets->getAssetsOfType(AssetType::Texture);

for (AssetHandle h : textures) {
    if (assets->isLoaded(h)) {
        const TextureData* tex = assets->getAsset<TextureData>(h);
        // Process texture...
    }
}
```

---

## Hot Reload

Hot reload enables **automatic asset reloading** when files change on disk. Essential for rapid iteration during development.

### How It Works

1. **File watching** - Uses `efsw` library to monitor directories in real-time
2. **Event queueing** - File changes are queued thread-safely by background watcher
3. **Main thread processing** - `update()` processes queued events and reloads assets
4. **Notification** - Subscribers are notified via callbacks

### Enabling Hot Reload

#### `void enableHotReload(bool enable)`

Enables or disables hot reload. When enabled, the Asset System automatically watches directories of registered assets.

```cpp
#if defined(BESTOW_DEV_TOOLS)
    // Enable hot reload in development builds
    assets->enableHotReload(true);
#endif
```

**Important:** Enable hot reload **before** registering assets, or call it after registration to start watching.

#### `void checkForReloads()`

Manually triggers a check for file changes. Normally you don't need to call this - `update()` handles it automatically.

#### `void reloadAsset(AssetHandle handle)`

Manually forces an asset to reload from disk.

```cpp
assets->reloadAsset(shaderHandle);
```

### Subscriptions

Subscribe to asset change notifications to react when assets are reloaded.

#### `SubscriptionId subscribe(AssetHandle handle, AssetChangeCallback callback)`

Subscribes to changes for a **specific asset**.

```cpp
using AssetChangeCallback = std::function<void(AssetHandle, AssetType)>;

AssetHandle shader = assets->registerAsset(AssetType::Shader, ":library:/shaders/toon.frag");
assets->loadAsset(shader);

SubscriptionId subId = assets->subscribe(shader, [this](AssetHandle h, AssetType t) {
    logInfo("Shader reloaded: {}", h.uuid);

    // Recompile and re-upload to GPU
    shaderSystem->recompileShader(h);
});
```

**Returns:** `SubscriptionId` - use this to unsubscribe later

#### `SubscriptionId subscribeToType(AssetType type, AssetChangeCallback callback)`

Subscribes to changes for **all assets of a type**.

```cpp
// React to ANY texture change
SubscriptionId texSubId = assets->subscribeToType(
    AssetType::Texture,
    [this](AssetHandle h, AssetType t) {
        logInfo("Texture {} reloaded", h.uuid);
        graphicsSystem->reloadTexture(h);
    }
);
```

**Use cases:**
- **Shader system** - Recompile all shaders when any shader file changes
- **Graphics system** - Re-upload textures to GPU when modified
- **Audio system** - Reload sounds into FMOD
- **Level system** - Reload current level when level file changes

#### `void unsubscribe(SubscriptionId id)`

Removes a subscription. Always unsubscribe in your system's destructor or shutdown method.

```cpp
class MySystem {
public:
    ~MySystem() {
        if (subscriptionId_ != InvalidSubscriptionId) {
            assets_->unsubscribe(subscriptionId_);
        }
    }

private:
    SubscriptionId subscriptionId_ = InvalidSubscriptionId;
};
```

---

## Asset Types

### TextureData

Image pixel data loaded via `stb_image`.

```cpp
struct TextureData {
    std::vector<unsigned char> pixels;  // Raw RGBA pixel data
    int width;                          // Image width in pixels
    int height;                         // Image height in pixels
    int channels;                       // Color channels (3=RGB, 4=RGBA)
};
```

**Supported formats:** PNG, JPG, BMP, TGA, PSD, GIF, HDR, PIC

**Usage:**
```cpp
AssetHandle tex = assets->registerAsset(AssetType::Texture, "textures/player.png");
assets->loadAsset(tex);

const TextureData* data = assets->getAsset<TextureData>(tex);
if (data) {
    // Upload to GPU via GraphicsSystem
    graphicsSystem->createTexture(tex, data->pixels.data(),
                                  data->width, data->height, data->channels);
}
```

### SoundData

Raw audio file bytes for FMOD or other audio engines.

```cpp
struct SoundData {
    std::vector<unsigned char> fileData;  // Raw audio bytes (WAV/OGG/MP3)
    std::string path;                     // Original file path
    std::size_t fileSize;                 // File size in bytes
};
```

**Supported formats:** WAV, OGG, MP3, FLAC

**Usage:**
```cpp
AssetHandle sound = assets->registerAsset(AssetType::Sound, "sounds/jump.wav");
assets->loadAsset(sound);

const SoundData* data = assets->getAsset<SoundData>(sound);
if (data) {
    // Pass to AudioSystem to create FMOD sound
    audioSystem->createSoundFromMemory(sound, data->fileData.data(), data->fileSize);
}
```

**Note:** Use `AssetType::Sound` for short effects and `AssetType::Music` for long tracks (streamed).

### FontData

Raw font file bytes for FreeType or other font rasterizers.

```cpp
struct FontData {
    std::vector<unsigned char> fileData;  // Raw TTF/OTF bytes
    std::string path;                     // Original file path
    std::size_t fileSize;                 // File size in bytes
};
```

**Supported formats:** TTF, OTF

**Usage:**
```cpp
AssetHandle font = assets->registerAsset(AssetType::Font, "fonts/arial.ttf");
assets->loadAsset(font);

const FontData* data = assets->getAsset<FontData>(font);
if (data) {
    // Pass to UISystem or GraphicsSystem for rendering
    uiSystem->loadFont("arial", data->fileData.data(), data->fileSize);
}
```

### ShaderData

GLSL shader source and optional compiled SPIR-V bytecode.

```cpp
struct ShaderData {
    std::string glslSource;                   // Original GLSL code
    std::vector<std::uint32_t> spirvBytecode; // Compiled SPIR-V (if compiled)
    std::string path;                         // Original file path
    std::string entryPoint;                   // Entry point ("main")

    enum class Stage {
        Vertex, Fragment, Geometry, Compute, TessControl, TessEval
    } stage;

    bool compiled;                            // Is SPIR-V bytecode valid?
    std::string compileError;                 // Error message if compilation failed
};
```

**Supported formats:** `.vert`, `.frag`, `.geom`, `.comp`, `.tesc`, `.tese`

**Usage:**
```cpp
// Load GLSL source
AssetHandle shader = assets->registerAsset(AssetType::Shader, ":library:/shaders/toon.frag");
assets->loadAsset(shader);

const ShaderData* data = assets->getAsset<ShaderData>(shader);
if (data) {
    logInfo("Loaded shader: {} (stage: {})",
        data->path, static_cast<int>(data->stage));

    // Use GLSL source for OpenGL or Vulkan
    graphicsSystem->compileShader(data->glslSource);
}
```

**Shader stage inference:** Determined by file extension automatically.

### DataAsset

JSON or Lua configuration files.

```cpp
struct DataAsset {
    std::any jsonData;       // Parsed JSON (nlohmann::json) if isJson=true
    std::string rawText;     // Raw file contents (always available)
    bool isJson;             // True if successfully parsed as JSON
};
```

**Supported formats:** JSON, Lua, TXT, or any text file

**Usage:**
```cpp
AssetHandle config = assets->registerAsset(AssetType::Data, "config/game.json");
assets->loadAsset(config);

const DataAsset* data = assets->getAsset<DataAsset>(config);
if (data && data->isJson) {
    // Access parsed JSON
    auto& json = std::any_cast<const nlohmann::json&>(data->jsonData);
    int maxHealth = json["player"]["maxHealth"];
} else if (data) {
    // Use raw text (for Lua files, etc.)
    lua.safe_script(data->rawText);
}
```

### MeshData

3D mesh geometry with vertices, indices, and submeshes.

```cpp
struct MeshData {
    std::vector<Vertex3DData> vertices;      // Vertex data
    std::vector<std::uint32_t> indices;      // Index buffer
    std::vector<SubMeshData> subMeshes;      // Submesh ranges
    float boundsMin[3], boundsMax[3];        // Bounding box
    std::string name;
    bool hasTangents;                        // Tangent space available?
    bool hasBoneData;                        // Skeletal animation data?
};
```

**Supported formats:** OBJ, glTF

**Usage:**
```cpp
AssetHandle mesh = assets->registerAsset(AssetType::Mesh, "models/character.obj");
assets->loadAsset(mesh);

const MeshData* data = assets->getAsset<MeshData>(mesh);
if (data) {
    // Upload to GPU
    graphicsSystem->createMeshBuffer(mesh, data->vertices, data->indices);
}
```

### ModelData

Complete 3D models with meshes, materials, hierarchy, and animations.

```cpp
struct ModelData {
    std::vector<MeshData> meshes;
    std::vector<MaterialData> materials;
    std::string name;

    // Scene hierarchy
    std::vector<Node> nodes;
    int rootNodeIndex;

    // Skeletal animation
    std::vector<Bone> bones;
    std::vector<Animation> animations;
};
```

**Supported formats:** glTF, FBX

**Usage:**
```cpp
AssetHandle model = assets->registerAsset(AssetType::Model, "models/character.gltf");
assets->loadAsset(model);

const ModelData* data = assets->getAsset<ModelData>(model);
if (data) {
    // Load all meshes and materials
    for (const MeshData& mesh : data->meshes) {
        graphicsSystem->uploadMesh(mesh);
    }

    // Play first animation
    if (!data->animations.empty()) {
        animationSystem->playAnimation(data->animations[0]);
    }
}
```

### MaterialData

PBR material properties and texture references.

```cpp
struct MaterialData {
    std::string name;

    // PBR properties
    float baseColorFactor[4];
    float metallicFactor;
    float roughnessFactor;
    float normalScale;
    float occlusionStrength;
    float emissiveFactor[3];
    float alphaCutoff;

    // Texture references
    MaterialTextureRef baseColorTexture;
    MaterialTextureRef metallicRoughnessTexture;
    MaterialTextureRef normalTexture;
    MaterialTextureRef occlusionTexture;
    MaterialTextureRef emissiveTexture;

    // Render state
    bool doubleSided;
    bool transparent;
    bool unlit;
};
```

### CubemapData

Cubemap textures for skyboxes and environment mapping.

```cpp
struct CubemapData {
    std::vector<std::vector<unsigned char>> facePixels;  // 6 faces: +X,-X,+Y,-Y,+Z,-Z
    int faceWidth, faceHeight;
    int channels;
    std::string name;
};
```

**Usage:**
```cpp
// Load cubemap (single file or 6-face configuration)
AssetHandle cubemap = assets->registerAsset(AssetType::Cubemap, "skybox/environment.hdr");
assets->loadAsset(cubemap);

const CubemapData* data = assets->getAsset<CubemapData>(cubemap);
if (data) {
    // Upload to GPU for skybox rendering
    graphicsSystem->createCubemap(cubemap, data);
}
```

### NavMeshData

Navigation mesh binary data for AI pathfinding.

```cpp
struct NavMeshData {
    std::vector<unsigned char> fileData;  // Raw navmesh binary (Recast format)
    std::string path;
    std::size_t fileSize;
};
```

**Usage:**
```cpp
AssetHandle navmesh = assets->registerAsset(AssetType::NavMesh, "levels/navmesh.bin");
assets->loadAsset(navmesh);

const NavMeshData* data = assets->getAsset<NavMeshData>(navmesh);
if (data) {
    // Pass to AI system for pathfinding
    aiSystem->loadNavMesh(data->fileData.data(), data->fileSize);
}
```

---

## Shader System

The Asset System provides specialized support for shader loading and compilation.

### Loading Shaders

#### `AssetHandle loadShader(const std::filesystem::path& path)`

Convenience method that combines `registerAsset()` + `loadAsset()` for shaders.

```cpp
// Register and load in one call
AssetHandle shader = assets->loadShader(":library:/shaders/pbr.frag");

const ShaderData* data = assets->getAsset<ShaderData>(shader);
if (data) {
    logInfo("Loaded shader: {}", data->path);
    logInfo("GLSL source length: {}", data->glslSource.length());
}
```

**Stage inference:** The shader stage is automatically inferred from the file extension:
- `.vert` → `ShaderData::Stage::Vertex`
- `.frag` → `ShaderData::Stage::Fragment`
- `.geom` → `ShaderData::Stage::Geometry`
- `.comp` → `ShaderData::Stage::Compute`
- `.tesc` → `ShaderData::Stage::TessControl`
- `.tese` → `ShaderData::Stage::TessEval`

### Accessing Shader Data

#### `const ShaderData* getShaderData(AssetHandle handle) const`

Type-safe accessor for shader data (equivalent to `getAsset<ShaderData>(handle)`).

```cpp
const ShaderData* shader = assets->getShaderData(handle);
if (shader) {
    // Access GLSL source
    std::string_view source = shader->glslSource;

    // Check if compiled to SPIR-V
    if (shader->compiled) {
        const auto& bytecode = shader->spirvBytecode;
        logInfo("SPIR-V bytecode size: {} bytes", bytecode.size() * 4);
    } else if (!shader->compileError.empty()) {
        logError("Shader compilation failed: {}", shader->compileError);
    }
}
```

### SPIR-V Compilation

#### `void compileShaderAsync(AssetHandle handle, AssetLoadCallback callback = nullptr)`

Compiles a loaded GLSL shader to SPIR-V bytecode asynchronously. Requires `shaderc` library.

```cpp
AssetHandle shader = assets->loadShader("shaders/custom.frag");

assets->compileShaderAsync(shader, [this](AssetHandle h, AssetState state) {
    const ShaderData* data = assets->getShaderData(h);

    if (state == AssetState::Loaded && data->compiled) {
        logInfo("Shader compiled successfully!");

        // Use SPIR-V bytecode for Vulkan
        vulkanRenderer->createShaderModule(h, data->spirvBytecode);
    } else if (data && !data->compileError.empty()) {
        logError("Shader compilation failed:\n{}", data->compileError);
    }
});
```

#### `bool isShaderCompilationSupported() const`

Returns `true` if SPIR-V compilation is available (shaderc library linked).

```cpp
if (assets->isShaderCompilationSupported()) {
    // Can use compileShaderAsync()
    assets->compileShaderAsync(shader);
} else {
    logWarn("SPIR-V compilation not supported, using GLSL source only");
}
```

---

## 3D Asset Loading

The Asset System provides convenient methods for loading 3D assets.

### Loading Meshes

#### `AssetHandle loadMesh(const std::filesystem::path& path)`

Convenience method that combines `registerAsset()` + `loadAsset()` for meshes.

```cpp
AssetHandle mesh = assets->loadMesh("models/statue.obj");

const MeshData* data = assets->getMeshData(mesh);
if (data) {
    logInfo("Loaded mesh: {} vertices, {} indices",
        data->vertices.size(), data->indices.size());
}
```

#### `const MeshData* getMeshData(AssetHandle handle) const`

Type-safe accessor for mesh data.

```cpp
const MeshData* mesh = assets->getMeshData(handle);
if (mesh) {
    for (const SubMeshData& submesh : mesh->subMeshes) {
        logInfo("Submesh: {} ({} indices)", submesh.name, submesh.indexCount);
    }
}
```

### Loading Models

#### `AssetHandle loadModel(const std::filesystem::path& path)`

Loads a complete model with meshes, materials, and hierarchy.

```cpp
AssetHandle model = assets->loadModel("models/character.gltf");

const ModelData* data = assets->getModelData(model);
if (data) {
    logInfo("Loaded model: {} with {} meshes, {} materials",
        data->name, data->meshes.size(), data->materials.size());

    // Access scene hierarchy
    for (const ModelData::Node& node : data->nodes) {
        logInfo("Node: {} (mesh index: {})", node.name, node.meshIndex);
    }
}
```

#### `const ModelData* getModelData(AssetHandle handle) const`

Type-safe accessor for model data.

### Loading Cubemaps

#### `AssetHandle loadCubemap(const std::filesystem::path& path)`

Loads a cubemap from a single equirectangular HDR image.

```cpp
AssetHandle skybox = assets->loadCubemap("skybox/sunset.hdr");

const CubemapData* data = assets->getCubemapData(skybox);
if (data) {
    logInfo("Loaded cubemap: {}x{} per face", data->faceWidth, data->faceHeight);
}
```

#### `AssetHandle loadCubemap(posX, negX, posY, negY, posZ, negZ)`

Loads a cubemap from 6 separate face images.

```cpp
AssetHandle skybox = assets->loadCubemap(
    "skybox/right.png",  // +X
    "skybox/left.png",   // -X
    "skybox/top.png",    // +Y
    "skybox/bottom.png", // -Y
    "skybox/front.png",  // +Z
    "skybox/back.png"    // -Z
);
```

#### `const CubemapData* getCubemapData(AssetHandle handle) const`

Type-safe accessor for cubemap data.

```cpp
const CubemapData* cubemap = assets->getCubemapData(handle);
if (cubemap) {
    // Access individual faces
    for (size_t i = 0; i < 6; ++i) {
        const auto& facePixels = cubemap->facePixels[i];
        logInfo("Face {} size: {} bytes", i, facePixels.size());
    }
}
```

### Loading Materials

#### `const MaterialData* getMaterialData(AssetHandle handle) const`

Type-safe accessor for material data.

```cpp
AssetHandle material = assets->registerAsset(AssetType::Material, "materials/metal.lua");
assets->loadAsset(material);

const MaterialData* data = assets->getMaterialData(material);
if (data) {
    logInfo("Material: {}", data->name);
    logInfo("Metallic: {}, Roughness: {}", data->metallicFactor, data->roughnessFactor);

    // Access texture references
    if (!data->baseColorTexture.path.empty()) {
        AssetHandle colorTex = data->baseColorTexture.handle;
        // Load and use the texture
    }
}
```

---

## Best Practices

### 1. Preload Critical Assets

Load essential assets synchronously during initialization to avoid hitches during gameplay.

```cpp
void Game::initialize() {
    // Preload player and UI assets synchronously
    AssetHandle playerTexture = assets->registerAsset(
        AssetType::Texture, ":assets:/textures/player.png"
    );
    AssetHandle font = assets->registerAsset(
        AssetType::Font, ":assets:/fonts/main.ttf"
    );

    assets->loadAsset(playerTexture);
    assets->loadAsset(font);

    // Now start loading other assets asynchronously
    loadLevelAssetsAsync();
}
```

### 2. Use Async Loading for Non-Critical Assets

Avoid blocking the game loop with `loadAssetAsync()`:

```cpp
void Game::loadLevelAssetsAsync() {
    for (const std::string& texturePath : levelTextures) {
        AssetHandle h = assets->registerAsset(AssetType::Texture, texturePath);
        assets->loadAssetAsync(h, [this](AssetHandle handle, AssetState state) {
            if (state == AssetState::Loaded) {
                onTextureLoaded(handle);
            }
        });
    }
}
```

### 3. Organize Assets with Path Schemes

Use `:assets:/` for game-specific content and `:library:/` for engine-provided assets:

```cpp
// Game assets
":assets:/textures/player.png"
":assets:/sounds/jump.wav"
":assets:/levels/level1.lua"

// Engine library assets
":library:/shaders/pbr.frag"
":library:/fonts/debug.ttf"
":library:/materials/standard.lua"
```

### 4. Always Call `update()` Every Frame

The Asset System requires regular updates to process async loads and hot reload events:

```cpp
void Game::gameLoop() {
    while (running) {
        deltaTime = calculateDeltaTime();

        // CRITICAL: Must be called every frame
        assets->update();

        input->update();
        physics->update(deltaTime);
        graphics->render();
    }
}
```

### 5. Handle Load Failures Gracefully

Always check asset state and handle failures:

```cpp
assets->loadAssetAsync(handle, [this](AssetHandle h, AssetState state) {
    if (state == AssetState::Loaded) {
        // Success path
        useAsset(h);
    } else {
        // Failure path
        AssetMetadata meta = assets->getAssetMetadata(h);
        logError("Failed to load asset: {}",
            meta.errorMessage.value_or("Unknown error"));

        // Load fallback asset
        useFallbackAsset();
    }
});
```

### 6. Unload Unused Assets to Conserve Memory

Unload assets when transitioning between levels or states:

```cpp
void Game::onLevelTransition() {
    // Unload previous level's assets
    for (AssetHandle h : currentLevelAssets) {
        assets->unloadAsset(h);
    }

    currentLevelAssets.clear();

    // Load next level's assets
    loadLevelAssets(nextLevelId);
}
```

### 7. Use Hot Reload During Development

Enable hot reload for rapid iteration:

```cpp
void Game::initialize() {
    #if defined(BESTOW_DEV_TOOLS)
        assets->enableHotReload(true);

        // Subscribe to shader changes for automatic recompilation
        shaderSubId = assets->subscribeToType(
            AssetType::Shader,
            [this](AssetHandle h, AssetType t) {
                shaderSystem->recompileShader(h);
            }
        );
    #endif
}
```

### 8. Batch Register Assets, Then Load

Register all assets first, then load them in priority order:

```cpp
void Game::loadLevel(const std::string& levelName) {
    // Register everything first
    std::vector<AssetHandle> criticalAssets;
    std::vector<AssetHandle> optionalAssets;

    criticalAssets.push_back(assets->registerAsset(
        AssetType::Texture, ":assets:/textures/terrain.png"
    ));
    optionalAssets.push_back(assets->registerAsset(
        AssetType::Sound, ":assets:/sounds/ambient.ogg"
    ));

    // Load critical assets synchronously
    for (AssetHandle h : criticalAssets) {
        assets->loadAsset(h);
    }

    // Load optional assets asynchronously
    for (AssetHandle h : optionalAssets) {
        assets->loadAssetAsync(h);
    }
}
```

### 9. Store Handles, Not Pointers

Always store `AssetHandle` instead of raw pointers to asset data. Handles remain valid across reloads, pointers do not.

```cpp
class Player {
public:
    void initialize(IAssetSystem* assets) {
        // Store handle, not pointer
        textureHandle_ = assets->registerAsset(
            AssetType::Texture, ":assets:/textures/player.png"
        );
        assets->loadAsset(textureHandle_);
    }

    void render(IAssetSystem* assets, IGraphicsSystem* graphics) {
        // Fetch data when needed
        const TextureData* texture = assets->getAsset<TextureData>(textureHandle_);
        if (texture) {
            graphics->drawSprite(textureHandle_, position);
        }
    }

private:
    AssetHandle textureHandle_;  // Safe across hot reloads
};
```

### 10. Clean Up Subscriptions

Always unsubscribe when your system shuts down:

```cpp
class MySystem {
public:
    void initialize(IAssetSystem* assets) {
        subId_ = assets->subscribeToType(
            AssetType::Texture,
            [this](AssetHandle h, AssetType t) { onTextureChanged(h); }
        );
    }

    void shutdown(IAssetSystem* assets) {
        if (subId_ != InvalidSubscriptionId) {
            assets->unsubscribe(subId_);
            subId_ = InvalidSubscriptionId;
        }
    }

private:
    SubscriptionId subId_ = InvalidSubscriptionId;
};
```

---

## Code Examples

### Example 1: Loading a Texture

```cpp
void Game::loadPlayerTexture(IAssetSystem* assets, IGraphicsSystem* graphics) {
    // 1. Register the asset
    AssetHandle textureHandle = assets->registerAsset(
        AssetType::Texture,
        ":assets:/textures/player.png"
    );

    // 2. Load synchronously
    assets->loadAsset(textureHandle);

    // 3. Check if loaded successfully
    if (assets->isLoaded(textureHandle)) {
        // 4. Access the texture data
        const TextureData* texture = assets->getAsset<TextureData>(textureHandle);

        if (texture) {
            logInfo("Loaded texture: {}x{} with {} channels",
                texture->width, texture->height, texture->channels);

            // 5. Upload to GPU via Graphics System
            graphics->createTexture(
                textureHandle,
                texture->pixels.data(),
                texture->width,
                texture->height,
                texture->channels
            );
        }
    } else {
        // Handle failure
        AssetMetadata meta = assets->getAssetMetadata(textureHandle);
        logError("Failed to load texture: {}",
            meta.errorMessage.value_or("Unknown error"));
    }
}
```

### Example 2: Async Loading with Progress Tracking

```cpp
class LevelLoader {
public:
    void loadLevelAssets(IAssetSystem* assets, const std::vector<std::string>& paths) {
        totalAssets_ = paths.size();
        loadedAssets_ = 0;

        for (const std::string& path : paths) {
            AssetHandle h = assets->registerAsset(AssetType::Texture, path);

            assets->loadAssetAsync(h, [this](AssetHandle handle, AssetState state) {
                loadedAssets_++;

                if (state == AssetState::Loaded) {
                    logInfo("Loaded asset {} of {}", loadedAssets_, totalAssets_);
                } else {
                    logWarn("Failed to load asset {} of {}", loadedAssets_, totalAssets_);
                }

                if (loadedAssets_ == totalAssets_) {
                    onAllAssetsLoaded();
                }
            });
        }
    }

    float getProgress() const {
        return static_cast<float>(loadedAssets_) / totalAssets_;
    }

private:
    void onAllAssetsLoaded() {
        logInfo("All level assets loaded!");
        // Transition to gameplay
    }

    int totalAssets_ = 0;
    std::atomic<int> loadedAssets_{0};
};
```

### Example 3: Hot Reload Setup

```cpp
class ShaderSystem {
public:
    void initialize(IAssetSystem* assets) {
        assets_ = assets;

        #if defined(BESTOW_DEV_TOOLS)
            // Enable hot reload
            assets_->enableHotReload(true);

            // Subscribe to ALL shader changes
            shaderSubId_ = assets_->subscribeToType(
                AssetType::Shader,
                [this](AssetHandle h, AssetType t) {
                    logInfo("Shader {} changed, recompiling...", h.uuid);
                    recompileShader(h);
                }
            );

            logInfo("Shader hot reload enabled");
        #endif
    }

    void shutdown() {
        #if defined(BESTOW_DEV_TOOLS)
            if (shaderSubId_ != InvalidSubscriptionId) {
                assets_->unsubscribe(shaderSubId_);
            }
        #endif
    }

    void recompileShader(AssetHandle handle) {
        // Get updated shader source
        const ShaderData* shader = assets_->getAsset<ShaderData>(handle);
        if (!shader) return;

        // Re-upload to GPU
        uploadToGPU(handle, shader);
    }

private:
    void uploadToGPU(AssetHandle handle, const ShaderData* shaderData) {
        // Vulkan/OpenGL shader upload logic
    }

    IAssetSystem* assets_ = nullptr;
    SubscriptionId shaderSubId_ = InvalidSubscriptionId;
};
```

### Example 4: Loading a 3D Model

```cpp
void Game::loadCharacterModel(IAssetSystem* assets) {
    // Load model with meshes and materials
    AssetHandle model = assets->loadModel(":assets:/models/character.gltf");

    const ModelData* data = assets->getModelData(model);
    if (!data) {
        logError("Failed to load character model");
        return;
    }

    logInfo("Loaded model: {} with {} meshes", data->name, data->meshes.size());

    // Upload all meshes to GPU
    for (const MeshData& mesh : data->meshes) {
        graphics->uploadMesh(mesh);
    }

    // Load material textures
    for (const MaterialData& material : data->materials) {
        if (!material.baseColorTexture.path.empty()) {
            AssetHandle texHandle = assets->registerAsset(
                AssetType::Texture,
                material.baseColorTexture.path
            );
            assets->loadAssetAsync(texHandle);
        }
    }

    // Setup animations
    if (!data->animations.empty()) {
        logInfo("Found {} animations", data->animations.size());
        for (const ModelData::Animation& anim : data->animations) {
            logInfo("Animation: {} ({:.2f}s)", anim.name, anim.duration);
        }
    }
}
```

### Example 5: Loading and Playing Sounds

```cpp
class AudioSystem {
public:
    void loadSound(IAssetSystem* assets, const std::string& name, const std::string& path) {
        AssetHandle handle = assets->registerAsset(AssetType::Sound, path);

        assets->loadAssetAsync(handle, [this, name](AssetHandle h, AssetState state) {
            if (state == AssetState::Loaded) {
                const SoundData* data = assets_->getAsset<SoundData>(h);

                if (data) {
                    // Create FMOD sound from memory
                    FMOD::Sound* fmodSound = nullptr;
                    FMOD_CREATESOUNDEXINFO exinfo = {};
                    exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
                    exinfo.length = data->fileSize;

                    FMOD_RESULT result = fmodSystem_->createSound(
                        reinterpret_cast<const char*>(data->fileData.data()),
                        FMOD_OPENMEMORY | FMOD_DEFAULT,
                        &exinfo,
                        &fmodSound
                    );

                    if (result == FMOD_OK) {
                        sounds_[name] = {h, fmodSound};
                        logInfo("Loaded sound: {}", name);
                    } else {
                        logError("Failed to create FMOD sound: {}", name);
                    }
                }
            }
        });
    }

    void playSound(const std::string& name) {
        auto it = sounds_.find(name);
        if (it != sounds_.end()) {
            FMOD::Channel* channel = nullptr;
            fmodSystem_->playSound(it->second.fmodSound, nullptr, false, &channel);
        }
    }

private:
    struct SoundEntry {
        AssetHandle handle;
        FMOD::Sound* fmodSound;
    };

    IAssetSystem* assets_;
    FMOD::System* fmodSystem_;
    std::unordered_map<std::string, SoundEntry> sounds_;
};
```

### Example 6: Shader Compilation with Error Handling

```cpp
void GraphicsSystem::loadAndCompileShader(IAssetSystem* assets, const std::string& path) {
    // Load shader source
    AssetHandle shader = assets->loadShader(path);

    if (!assets->isShaderCompilationSupported()) {
        logWarn("SPIR-V compilation not supported, using GLSL only");

        // Use GLSL source directly
        const ShaderData* data = assets->getShaderData(shader);
        if (data) {
            compileGLSL(data->glslSource);
        }
        return;
    }

    // Compile to SPIR-V asynchronously
    assets->compileShaderAsync(shader, [this, path](AssetHandle h, AssetState state) {
        const ShaderData* data = assets_->getShaderData(h);
        if (!data) return;

        if (state == AssetState::Loaded && data->compiled) {
            logInfo("Shader compiled: {}", path);
            logInfo("SPIR-V size: {} bytes", data->spirvBytecode.size() * 4);

            // Create Vulkan shader module
            createVulkanShaderModule(h, data->spirvBytecode);
        } else {
            logError("Shader compilation failed: {}", path);
            if (!data->compileError.empty()) {
                logError("Compilation errors:\n{}", data->compileError);
            }

            // Fall back to GLSL for OpenGL
            compileGLSL(data->glslSource);
        }
    });
}
```

---

## Summary

The Asset System is the **centralized file I/O gateway** for Bestow. Key takeaways:

1. **ALL file operations MUST go through the Asset System** - no direct `std::ifstream`, `lua.safe_script_file()`, etc.
2. **Call `update()` every frame** - required for async loads and hot reload
3. **Use async loading** for non-critical assets to avoid blocking gameplay
4. **Enable hot reload during development** for rapid iteration
5. **Subscribe to asset changes** to react to hot reloads
6. **Store handles, not pointers** - handles survive reloads, pointers do not
7. **Always check for load failures** and handle gracefully
8. **Use specialized methods** - `loadShader()`, `loadMesh()`, `loadModel()`, `loadCubemap()` for convenience
9. **Compile shaders to SPIR-V** asynchronously when targeting Vulkan

For more information, see:
- `/bestow-contract/src/bestow.assets.cppm` - Interface definition
- `/bestow-assets/src/AssetSystem.cpp` - Implementation details
- `/tests/unit/AssetSystemTests.cpp` - Usage examples and test cases
