# Assets

> **Visibility:** Protected (C++ peer systems only -- not in Lua API)
> **Tier:** 2
> **Dependencies:** Types, Events
> **Consumers:** Shader, Audio, Graphics2D, Graphics3D, Animation, AI, Scene, Config, UI, Blueprints -- every system that loads files

## Purpose

IAssetCore is the **sole gateway to the file system** in Bestow. No other system may directly open, read, or stat files. All asset registration, loading, caching, hot-reload monitoring, and path resolution flow through this single interface. This centralisation gives the engine unified caching, background-thread loading, hot-reload support via efsw file watching, and consistent path resolution for the `:library:/` and `:assets:/` virtual prefixes. The typed data accessors eliminate the V1 `std::any` pattern -- consumers retrieve concrete data pointers (`const TextureData*`, `const ShaderData*`, etc.) without casting.

## Contract: `IAssetCore`

### Lifecycle

| Method | Returns | Description |
|--------|---------|-------------|
| `update()` | `void` | Pump the asset system once per frame. Processes completed async loads, dispatches hot-reload notifications to subscribers, and finalises any pending unload requests. Must be called every frame during `EarlyUpdate`. |

### Registration and Loading

| Method | Returns | Description |
|--------|---------|-------------|
| `registerAsset(AssetType type, std::string_view path)` | `AssetHandle` | Register an asset by type and virtual path. Does **not** load the asset -- only creates the handle and internal metadata entry. The same path registered twice returns the same handle. |
| `unregisterAsset(AssetHandle handle)` | `void` | Remove the asset from the registry entirely. If the asset is loaded it is unloaded first. All subscriptions for this handle are silently removed. |
| `loadAsset(AssetHandle handle)` | `Result<void>` | Synchronously load the asset into memory. Blocks the calling thread until IO and parsing complete. Returns `IOError` if the file cannot be read, `ParseError` if the data is malformed. |
| `loadAssetAsync(AssetHandle handle, std::function<void(AssetHandle, AssetState)> callback)` | `void` | Queue the asset for background loading. The optional callback fires on the main thread during the next `update()` after loading completes, with the final `AssetState` (`Loaded` or `Failed`). |
| `unloadAsset(AssetHandle handle)` | `Result<void>` | Free the in-memory data for this asset. The handle remains valid and can be re-loaded later. Returns `InvalidHandle` if the handle is unknown. |

### State Queries

| Method | Returns | Description |
|--------|---------|-------------|
| `getState(AssetHandle handle) const` | `AssetState` | Return the current lifecycle state of the asset (`Unloaded`, `Loading`, `Loaded`, or `Failed`). |
| `isLoaded(AssetHandle handle) const` | `bool` | Convenience shorthand for `getState(handle) == AssetState::Loaded`. |

### Typed Data Access

All accessors return `nullptr` when the handle is unknown, unloaded, or of the wrong type. No `std::any` -- every accessor returns a concrete pointer.

| Method | Returns | Description |
|--------|---------|-------------|
| `getTextureData(AssetHandle h) const` | `const TextureData*` | Raw pixel data, dimensions, channel count, and format. |
| `getFontData(AssetHandle h) const` | `const FontData*` | Font atlas, glyph metrics, and kerning tables. |
| `getSoundData(AssetHandle h) const` | `const SoundData*` | PCM samples, sample rate, channel count, and duration. |
| `getShaderData(AssetHandle h) const` | `const ShaderData*` | GLSL or SPIR-V source/bytecode and stage metadata. |
| `getMeshData(AssetHandle h) const` | `const MeshData*` | Vertex positions, normals, UVs, tangents, and index buffer. |
| `getModelData(AssetHandle h) const` | `const ModelData*` | Full model hierarchy: meshes, materials, skeleton, and animations. |
| `getMaterialData(AssetHandle h) const` | `const MaterialData*` | Material properties: textures, uniform values, shader references. |
| `getCubemapData(AssetHandle h) const` | `const CubemapData*` | Six-face pixel data for skyboxes and environment maps. |
| `getNavMeshData(AssetHandle h) const` | `const NavMeshData*` | Navigation mesh geometry for AI pathfinding. |
| `getTextData(AssetHandle h) const` | `const std::string*` | Raw text content for Lua scripts, config files, and other text assets. |

### Convenience Loaders

Each convenience loader registers the asset (if not already registered) and performs a synchronous load in a single call. Returns the handle on success or a `SystemError` on failure.

| Method | Returns | Description |
|--------|---------|-------------|
| `loadTexture(std::string_view path)` | `Result<AssetHandle>` | Register + load an image file (PNG, JPG, TGA, BMP, HDR). |
| `loadFont(std::string_view path)` | `Result<AssetHandle>` | Register + load a font file (TTF, OTF). |
| `loadSound(std::string_view path)` | `Result<AssetHandle>` | Register + load a sound file (WAV, OGG, MP3). |
| `loadShader(std::string_view path)` | `Result<AssetHandle>` | Register + load a GLSL shader source file (.vert, .frag, .geom, .comp). |
| `loadShaderCompiled(std::string_view path)` | `Result<AssetHandle>` | Register + load a pre-compiled SPIR-V binary (.spv). |
| `loadMesh(std::string_view path)` | `Result<AssetHandle>` | Register + load a 3D mesh file (OBJ, glTF, FBX). |
| `loadModel(std::string_view path)` | `Result<AssetHandle>` | Register + load a full 3D model with materials and skeleton (glTF, FBX). |
| `loadCubemap(std::string_view path)` | `Result<AssetHandle>` | Register + load a single-file cubemap (HDR equirectangular, KTX). |
| `loadCubemap(std::string_view posX, std::string_view negX, std::string_view posY, std::string_view negY, std::string_view posZ, std::string_view negZ)` | `Result<AssetHandle>` | Register + load a cubemap from six individual face images. |
| `loadMaterial(std::string_view luaPath)` | `Result<AssetHandle>` | Register + load a Lua material definition file. |
| `loadNavMesh(std::string_view path)` | `Result<AssetHandle>` | Register + load a pre-baked navigation mesh. |
| `loadData(std::string_view path)` | `Result<AssetHandle>` | Register + load a generic text/data file as a raw string. |

### Bulk Operations

| Method | Returns | Description |
|--------|---------|-------------|
| `loadAll()` | `void` | Synchronously load every registered-but-unloaded asset. Useful for loading screens. |
| `unloadAll()` | `void` | Unload every loaded asset. Handles remain valid for future re-loading. |
| `getAssetsOfType(AssetType type) const` | `std::vector<AssetHandle>` | Return all registered handles that match the given type. |

### Hot Reload

| Method | Returns | Description |
|--------|---------|-------------|
| `enableHotReload(bool enable)` | `void` | Start or stop the efsw file watcher on all registered asset directories. Typically enabled only in debug/development builds. |
| `isHotReloadEnabled() const` | `bool` | Query whether hot reload monitoring is currently active. |
| `checkForReloads()` | `void` | Manually poll the file watcher queue. Called automatically by `update()`, but can be invoked explicitly if needed. |
| `reloadAsset(AssetHandle handle)` | `Result<void>` | Force-reload a specific asset from disk regardless of file-watcher state. Notifies all subscribers after reload completes. |

### Subscriptions

All subscriptions follow the V2 unified subscription pattern and return a `SubscriptionId` that can be passed to `unsubscribe()`.

| Method | Returns | Description |
|--------|---------|-------------|
| `subscribe(AssetHandle handle, std::function<void(AssetHandle, AssetType)> callback)` | `SubscriptionId` | Subscribe to changes on a **single** asset. The callback fires when the asset is hot-reloaded or explicitly reloaded. |
| `subscribeToType(AssetType type, std::function<void(AssetHandle, AssetType)> callback)` | `SubscriptionId` | Subscribe to changes on **all** assets of the given type. Useful for systems that need to know when any shader or any texture changes. |
| `unsubscribe(SubscriptionId id)` | `void` | Remove a subscription. Safe to call with an already-removed or invalid ID (no-op). |

### Path Resolution

| Method | Returns | Description |
|--------|---------|-------------|
| `resolveLibraryPath(std::string_view relativePath) const` | `std::optional<std::filesystem::path>` | Resolve a `:library:/`-prefixed path to an absolute filesystem path. Returns `std::nullopt` if the file does not exist. |
| `resolveAssetPath(std::string_view relativePath) const` | `std::optional<std::filesystem::path>` | Resolve a `:assets:/`-prefixed path to an absolute filesystem path. Returns `std::nullopt` if the file does not exist. |
| `assetExists(std::string_view path) const` | `bool` | Check whether a file exists at the given virtual path without registering or loading it. Handles both `:library:/` and `:assets:/` prefixes. |

### Library Discovery

| Method | Returns | Description |
|--------|---------|-------------|
| `listLibraryCategories() const` | `std::vector<std::string>` | Return the top-level subdirectory names inside the engine's built-in library path (e.g. `"shaders"`, `"fonts"`, `"textures"`). |
| `listLibraryAssets(std::string_view category) const` | `std::vector<std::string>` | Return the file names within a specific library category. Useful for editor tooling and dev overlays. |

## Types

### AssetType

```cpp
enum class AssetType : std::uint8_t {
    Texture,        // PNG, JPG, TGA, BMP, HDR
    Sound,          // WAV, OGG, MP3
    Music,          // Streaming audio
    Font,           // TTF, OTF
    Scene,          // Lua scene definition
    Data,           // Generic text/binary data
    Shader,         // GLSL source (.vert, .frag, .geom, .comp)
    NavMesh,        // Pre-baked navigation mesh
    BehaviorTree,   // AI behavior tree definition
    Mesh,           // 3D mesh (OBJ, glTF, FBX)
    Model,          // 3D model with materials (glTF, FBX)
    Material,       // Material definition (Lua, glTF embedded)
    Cubemap         // Skybox / environment map
};
```

| Value | Description |
|-------|-------------|
| `Texture` | 2D image data. Loaded via stb_image. Supports PNG, JPG, TGA, BMP, HDR. |
| `Sound` | Short audio clip loaded entirely into memory. WAV, OGG, MP3. |
| `Music` | Streaming audio. Only a small buffer is held in memory at a time. |
| `Font` | TrueType or OpenType font. Rasterised into an atlas by FreeType/msdf-atlas-gen. |
| `Scene` | Lua file describing a scene's entity layout and initial state. |
| `Data` | Generic text or binary blob. Returned as `std::string` via `getTextData()`. |
| `Shader` | GLSL shader source file. Individual stages (.vert, .frag, .geom, .comp). |
| `NavMesh` | Pre-baked Recast/Detour navigation mesh for AI pathfinding. |
| `BehaviorTree` | AI behavior tree definition (XML or Lua). |
| `Mesh` | Raw 3D mesh geometry without materials or skeleton. |
| `Model` | Full 3D model: meshes + materials + skeleton + animations. |
| `Material` | Material property definition (shader refs, texture slots, uniforms). |
| `Cubemap` | Six-face or equirectangular environment map. |

### AssetState

```cpp
enum class AssetState : std::uint8_t {
    Unloaded,   // Registered but not loaded
    Loading,    // Async load in progress
    Loaded,     // Ready to use
    Failed      // Load attempted and failed
};
```

| Value | Description |
|-------|-------------|
| `Unloaded` | The asset is registered in the system but no data has been loaded. This is the initial state after `registerAsset()`. |
| `Loading` | An asynchronous load is in progress on a background thread. The handle is valid but data accessors return `nullptr`. |
| `Loaded` | The asset data is in memory and ready for consumption. Typed data accessors return valid pointers. |
| `Failed` | The most recent load attempt failed (file not found, parse error, etc.). Call `getState()` or check the `Result` from `loadAsset()` for details. |

### AssetHandle

```cpp
struct AssetHandle {
    UUID uuid = 0;
    AssetType type = AssetType::Data;

    bool isValid() const noexcept { return uuid != 0; }
    explicit operator bool() const noexcept { return uuid != 0; }
    auto operator<=>(const AssetHandle&) const = default;
};
```

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `uuid` | `UUID` (`std::uint64_t`) | `0` | Unique identifier assigned by the asset system during registration. Zero means invalid/null. |
| `type` | `AssetType` | `AssetType::Data` | The asset category. Set at registration time and immutable thereafter. |

### TextureData

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `pixels` | `std::vector<std::uint8_t>` | -- | Raw pixel data in row-major order. |
| `width` | `int` | `0` | Image width in pixels. |
| `height` | `int` | `0` | Image height in pixels. |
| `channels` | `int` | `4` | Number of colour channels (1=grey, 3=RGB, 4=RGBA). |
| `isHDR` | `bool` | `false` | Whether the source was an HDR format (pixel data is float). |

### FontData

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `atlasPixels` | `std::vector<std::uint8_t>` | -- | Rasterised font atlas bitmap. |
| `atlasWidth` | `int` | `0` | Atlas width in pixels. |
| `atlasHeight` | `int` | `0` | Atlas height in pixels. |
| `glyphs` | `std::vector<GlyphInfo>` | -- | Per-glyph metrics and atlas UV coordinates. |
| `lineHeight` | `float` | `0` | Recommended vertical distance between baselines. |
| `ascender` | `float` | `0` | Distance from baseline to top of tallest glyph. |
| `descender` | `float` | `0` | Distance from baseline to bottom of lowest glyph (typically negative). |

### SoundData

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `samples` | `std::vector<float>` | -- | PCM audio samples, interleaved for multi-channel. |
| `sampleRate` | `int` | `44100` | Samples per second. |
| `channelCount` | `int` | `2` | Number of audio channels (1=mono, 2=stereo). |
| `duration` | `float` | `0` | Total duration in seconds. |

### ShaderData

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `source` | `std::string` | -- | GLSL source code (empty if loaded as SPIR-V). |
| `spirv` | `std::vector<std::uint32_t>` | -- | SPIR-V bytecode (empty if loaded as GLSL source). |
| `stage` | `ShaderStage` | `ShaderStage::Vertex` | Which pipeline stage this shader targets. |
| `path` | `std::string` | -- | Original file path for diagnostics and hot reload tracking. |

### MeshData

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `positions` | `std::vector<Vec3>` | -- | Vertex positions in model space. |
| `normals` | `std::vector<Vec3>` | -- | Per-vertex normals. |
| `uvs` | `std::vector<Vec2>` | -- | Texture coordinates (first UV set). |
| `tangents` | `std::vector<Vec4>` | -- | Per-vertex tangent vectors (xyz=tangent, w=handedness). |
| `indices` | `std::vector<std::uint32_t>` | -- | Triangle index buffer. |
| `boneIndices` | `std::vector<std::array<int, 4>>` | -- | Per-vertex bone indices for skeletal animation (up to 4 bones per vertex). |
| `boneWeights` | `std::vector<Vec4>` | -- | Per-vertex bone weights corresponding to `boneIndices`. |
| `aabb` | `AABB3D` | -- | Axis-aligned bounding box computed from `positions`. |

### ModelData

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `meshes` | `std::vector<MeshData>` | -- | All meshes in the model. |
| `materials` | `std::vector<MaterialData>` | -- | Embedded material definitions. |
| `meshMaterialIndices` | `std::vector<int>` | -- | Maps each mesh to its material index. |
| `skeleton` | `std::optional<SkeletonData>` | -- | Bone hierarchy (absent for static models). |
| `animations` | `std::vector<Animation>` | -- | Embedded animation clips. |

### MaterialData

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `name` | `std::string` | -- | Human-readable material name. |
| `shaderPath` | `std::string` | -- | Path to the shader program (if external). |
| `textures` | `std::unordered_map<std::string, std::string>` | -- | Named texture slots mapped to file paths (`"albedo"`, `"normal"`, `"metallic"`, etc.). |
| `uniforms` | `std::unordered_map<std::string, UniformValue>` | -- | Named uniform values (colours, floats, vectors). |
| `doubleSided` | `bool` | `false` | Whether back-face culling should be disabled. |
| `alphaMode` | `std::string` | `"opaque"` | Alpha blending mode (`"opaque"`, `"blend"`, `"mask"`). |
| `alphaCutoff` | `float` | `0.5f` | Alpha test threshold when `alphaMode` is `"mask"`. |

### CubemapData

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `faces` | `std::array<TextureData, 6>` | -- | Pixel data for each face: +X, -X, +Y, -Y, +Z, -Z. |

### NavMeshData

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `vertices` | `std::vector<Vec3>` | -- | Navigation mesh vertex positions. |
| `triangles` | `std::vector<std::array<int, 3>>` | -- | Triangle indices into the vertex array. |
| `detourData` | `std::vector<std::uint8_t>` | -- | Serialised Detour navmesh binary (for direct Detour loading). |

## Examples

### Basic synchronous asset workflow

```cpp
// In system initialisation -- assets injected via DI
void ShaderSystem::initialize(IAssetCore& assets) {
    // Register and load a shader pair
    auto vertResult = assets.loadShader(":library:/shaders/pbr.vert");
    if (!vertResult) {
        LOG_ERROR("Failed to load vertex shader: {}", vertResult.error().message);
        return;
    }
    vertHandle_ = *vertResult;

    auto fragResult = assets.loadShader(":library:/shaders/pbr.frag");
    if (!fragResult) {
        LOG_ERROR("Failed to load fragment shader: {}", fragResult.error().message);
        return;
    }
    fragHandle_ = *fragResult;

    // Read the loaded data
    const ShaderData* vertData = assets.getShaderData(vertHandle_);
    const ShaderData* fragData = assets.getShaderData(fragHandle_);
    // ... compile shaders from data
}
```

### Asynchronous loading with callback

```cpp
void GraphicsSystem::loadTexture(IAssetCore& assets, std::string_view path) {
    AssetHandle handle = assets.registerAsset(AssetType::Texture, path);

    assets.loadAssetAsync(handle, [this, &assets](AssetHandle h, AssetState state) {
        if (state == AssetState::Loaded) {
            const TextureData* data = assets.getTextureData(h);
            uploadToGPU(h, *data);
        } else {
            LOG_WARN("Texture load failed for handle {}", h.uuid);
        }
    });
}
```

### Hot reload subscription

```cpp
void ShaderSystem::setupHotReload(IAssetCore& assets) {
    assets.enableHotReload(true);

    // Subscribe to ALL shader changes
    shaderSubId_ = assets.subscribeToType(AssetType::Shader,
        [this, &assets](AssetHandle h, AssetType type) {
            const ShaderData* data = assets.getShaderData(h);
            if (data) {
                recompileShader(h, *data);
            }
        });
}

void ShaderSystem::shutdown(IAssetCore& assets) {
    assets.unsubscribe(shaderSubId_);
}
```

### Path resolution and existence checks

```cpp
void ConfigSystem::loadConfig(IAssetCore& assets, std::string_view name) {
    std::string path = std::format(":assets:/config/{}.lua", name);

    if (!assets.assetExists(path)) {
        // Fall back to library default
        path = std::format(":library:/config/{}.lua", name);
    }

    auto result = assets.loadData(path);
    if (result) {
        const std::string* text = assets.getTextData(*result);
        if (text) {
            parseLuaConfig(*text);
        }
    }
}
```
