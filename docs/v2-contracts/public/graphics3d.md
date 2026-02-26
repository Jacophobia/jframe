# 3D Graphics System

> **Visibility:** Public (Lua + C++ API)
> **Tier:** 5
> **Dependencies:** Types, Entity, Assets, Shader, Animation, GraphicsContext
> **Lua Paths:** `bestow.graphics3d` (high-level), `bestow.graphics3d.core` (low-level)

## Purpose

The 3D Graphics System is the primary rendering interface for Bestow, responsible for mesh rendering, PBR and unlit materials, lighting (directional, point, spot, ambient), camera management, environment effects (skybox, fog), post-processing, shadow mapping, debug visualization, and 3D text. The high-level API provides path-based drawing with sensible defaults for rapid prototyping, while the low-level API exposes handle-based mesh and material management, instanced rendering, skinned mesh support, LOD control, and four focused sub-contracts for post-processing, shadows, debug rendering, and 3D text.

## High-Level API: `IGraphics3DSystem`

The simplified API for common 3D rendering tasks. Accepts file paths instead of handles, creates internal resources automatically, and exposes only the most common lighting and camera operations. No lifecycle methods -- the engine manages those internally.

### Rendering

| Method | Returns | Description |
|--------|---------|-------------|
| `drawModel(std::string_view modelPath, Vec3 position, Quat rotation = {}, Vec3 scale = {1,1,1})` | `Result<void>` | Draw a 3D model at the given position, rotation, and scale using a file path; handles are created and cached internally |
| `drawPrimitive(PrimitiveType type, Vec3 position, Vec3 scale = {1,1,1}, Color color = Color::white())` | `Result<void>` | Draw a built-in primitive shape (cube, sphere, etc.) at the given position, scale, and color |

### Lighting

| Method | Returns | Description |
|--------|---------|-------------|
| `setDirectionalLight(Vec3 direction, Color color = Color::white(), float intensity = 1.0f)` | `void` | Set the main directional light direction, color, and intensity |
| `setAmbientLight(Color color, float intensity = 0.3f)` | `void` | Set the ambient light color and intensity for indirect illumination |

### Camera

| Method | Returns | Description |
|--------|---------|-------------|
| `setCameraPosition(Vec3 position)` | `void` | Set the camera eye position in world space |
| `setCameraTarget(Vec3 target)` | `void` | Set the point the camera looks at in world space |
| `setCameraFOV(float degrees)` | `void` | Set the vertical field of view in degrees |

### Environment

| Method | Returns | Description |
|--------|---------|-------------|
| `setSkybox(std::string_view cubemapPath)` | `Result<void>` | Load and set a cubemap skybox from a file path |

### Window

| Method | Returns | Description |
|--------|---------|-------------|
| `getWindowSize()` | `Size` | Return the current window dimensions in pixels |
| `shouldClose()` | `bool` | Return true if the window close has been requested |
| `setClearColor(Color color)` | `void` | Set the color used to clear the framebuffer each frame |

## Low-Level API: `IGraphics3DCore`

Full control API inheriting from `IGraphicsContextCore`. Exposes handle-based resource management for meshes, materials, and textures, instanced and skinned rendering, full lighting control, camera matrices, environment settings, render state, and LOD configuration.

### Lifecycle

| Method | Returns | Description |
|--------|---------|-------------|
| `initialize(const Graphics3DConfig& config)` | `Result<void>` | Initialize the 3D graphics subsystem with the given configuration |
| `shutdown()` | `void` | Shut down the 3D graphics subsystem and release all GPU resources |
| `beginFrame()` | `void` | Begin a new rendering frame; must be called before any draw calls |
| `endFrame()` | `void` | End the current frame and present the result to the screen |

### Mesh Management

| Method | Returns | Description |
|--------|---------|-------------|
| `createMesh(const MeshDef& def)` | `Result<MeshHandle>` | Create a GPU mesh from vertex and index data and return its handle |
| `destroyMesh(MeshHandle handle)` | `Result<void>` | Destroy a mesh and free its GPU memory |
| `createPrimitiveMesh(PrimitiveType type, const PrimitiveParams& params = {})` | `Result<MeshHandle>` | Create a built-in primitive mesh (cube, sphere, etc.) with optional sizing parameters |

### Material Management

| Method | Returns | Description |
|--------|---------|-------------|
| `createPBRMaterial(const PBRMaterialDef& def)` | `Result<MaterialHandle>` | Create a PBR material from the given definition and return its handle |
| `createUnlitMaterial(const UnlitMaterialDef& def)` | `Result<MaterialHandle>` | Create an unlit material with a flat color and optional texture |
| `destroyMaterial(MaterialHandle handle)` | `Result<void>` | Destroy a material and free its resources |
| `setMaterialUniform(MaterialHandle handle, std::string_view name, const UniformValue& value)` | `Result<void>` | Set a named uniform value on a material (float, Vec3, Mat4, TextureHandle, etc.) |
| `setMaterialTexture(MaterialHandle handle, std::string_view slot, TextureHandle texture)` | `Result<void>` | Bind a texture to a named slot on a material (e.g., "albedo", "normal") |
| `getDefaultMaterial()` | `MaterialHandle` | Return the engine's built-in default PBR material handle |
| `getErrorMaterial()` | `MaterialHandle` | Return the magenta error material used for missing or broken assets |

### Texture Management

| Method | Returns | Description |
|--------|---------|-------------|
| `createTexture(const TextureData& data)` | `Result<TextureHandle>` | Create a GPU texture from raw pixel data |
| `createTextureFromAsset(AssetHandle asset)` | `Result<TextureHandle>` | Create a GPU texture from a previously loaded asset |
| `destroyTexture(TextureHandle handle)` | `Result<void>` | Destroy a texture and free its GPU memory |

### Rendering

| Method | Returns | Description |
|--------|---------|-------------|
| `drawMesh(MeshHandle mesh, MaterialHandle material, const Mat4& transform, bool castShadow = true)` | `void` | Submit a mesh for rendering with the given material and world transform |
| `drawMeshInstanced(MeshHandle mesh, MaterialHandle material, std::span<const Mat4> transforms)` | `void` | Submit a mesh for instanced rendering with an array of world transforms |
| `drawSkinnedMesh(MeshHandle mesh, MaterialHandle material, const Mat4& transform, std::span<const Mat4> boneMatrices)` | `void` | Submit a skinned mesh with bone matrices for skeletal animation rendering |
| `renderEntities(IEntityCore& entities)` | `void` | Automatically render all entities that have mesh and transform components |

### Lighting

| Method | Returns | Description |
|--------|---------|-------------|
| `setDirectionalLight(const DirectionalLight& light)` | `void` | Set the main directional light using a full DirectionalLight struct |
| `addPointLight(const PointLight& light)` | `Result<std::uint32_t>` | Add a point light to the scene and return its unique light ID |
| `addSpotLight(const SpotLight& light)` | `Result<std::uint32_t>` | Add a spot light to the scene and return its unique light ID |
| `removeLight(std::uint32_t lightId)` | `Result<void>` | Remove a previously added point or spot light by its ID |
| `clearLights()` | `void` | Remove all dynamic lights from the scene |
| `setAmbientLight(Vec3 color, float intensity = 1.0f)` | `void` | Set the ambient light color and intensity |

### Camera

| Method | Returns | Description |
|--------|---------|-------------|
| `setCamera(const Camera3D& camera)` | `void` | Set the active 3D camera using a full Camera3D struct |
| `getCamera()` | `Camera3D` | Return the current active camera state |
| `screenToWorldRay(Vec2 screenPos)` | `Vec3` | Convert a screen-space coordinate to a world-space ray direction |
| `worldToScreen(Vec3 worldPos)` | `Vec2` | Project a world-space position to screen-space coordinates |

### Environment

| Method | Returns | Description |
|--------|---------|-------------|
| `setSkybox(TextureHandle cubemap)` | `Result<void>` | Set the skybox from a cubemap texture handle |
| `clearSkybox()` | `void` | Remove the current skybox |
| `setFog(const FogDef& fog)` | `void` | Enable and configure distance fog |
| `clearFog()` | `void` | Disable fog rendering |

### Window

| Method | Returns | Description |
|--------|---------|-------------|
| `setWindowSize(Size size)` | `void` | Resize the window to the given dimensions |
| `getWindowMode()` | `WindowMode` | Return the current window mode (windowed, fullscreen, borderless) |
| `setWindowMode(WindowMode mode)` | `void` | Change the window mode |
| `shouldClose()` | `bool` | Return true if the window close has been requested |
| `setWindowTitle(std::string_view title)` | `void` | Set the window title bar text |
| `setClearColor(const Color& color)` | `void` | Set the framebuffer clear color |
| `setVSync(bool enabled)` | `void` | Enable or disable vertical synchronization |

### Render State

| Method | Returns | Description |
|--------|---------|-------------|
| `setWireframe(bool enabled)` | `void` | Toggle wireframe rendering mode |
| `setFaceCulling(CullMode mode)` | `void` | Set the face culling mode (None, Front, Back, FrontAndBack) |

### LOD (Level of Detail)

| Method | Returns | Description |
|--------|---------|-------------|
| `setMeshLODs(MeshHandle baseMesh, std::span<const MeshLOD> lods)` | `Result<void>` | Associate LOD meshes with a base mesh for automatic distance-based switching |
| `setLODBias(float bias)` | `void` | Set a global LOD distance bias; positive values prefer higher-detail LODs |

## Sub-Contract: `IPostProcessingCore`

Accessed via `bestow.graphics3d.postfx` in Lua. Controls screen-space post-processing effects applied after the main render pass.

### Effects

| Method | Returns | Description |
|--------|---------|-------------|
| `setToneMapping(bool enabled)` | `void` | Enable or disable HDR tone mapping |
| `setToneMappingOperator(ToneMappingOp op)` | `void` | Set the tone mapping algorithm (Reinhard, ACES, Filmic, Uncharted2, Linear) |
| `setExposure(float exposure)` | `void` | Set the exposure value for tone mapping |
| `setBloom(bool enabled, float threshold = 1.0f, float intensity = 1.0f)` | `void` | Enable bloom with a brightness threshold and intensity |
| `setSSAO(bool enabled, float radius = 0.5f, float intensity = 1.0f, int samples = 16)` | `void` | Enable screen-space ambient occlusion with radius, intensity, and sample count |
| `setDepthOfField(bool enabled, float focusDist = 10.0f, float aperture = 5.6f)` | `void` | Enable depth-of-field blur with a focus distance and aperture |
| `setMotionBlur(bool enabled, float scale = 1.0f)` | `void` | Enable per-object motion blur with the given scale |
| `setAntiAliasing(AntiAliasingMode mode)` | `void` | Set the anti-aliasing mode (None, FXAA, MSAA2x, MSAA4x, MSAA8x, TAA) |
| `setColorGrading(const ColorGradingParams& params)` | `void` | Apply color grading with contrast, saturation, brightness, color balance, and gamma |
| `setVignette(bool enabled, float intensity = 0.5f)` | `void` | Enable a screen-edge vignette darkening effect |
| `setChromaticAberration(bool enabled, float intensity = 0.5f)` | `void` | Enable chromatic aberration (color fringing at screen edges) |

## Sub-Contract: `IShadowCore`

Accessed via `bestow.graphics3d.shadows` in Lua. Controls shadow mapping configuration.

### Shadow Configuration

| Method | Returns | Description |
|--------|---------|-------------|
| `setShadowsEnabled(bool enabled)` | `void` | Globally enable or disable shadow rendering |
| `areShadowsEnabled()` | `bool` | Return whether shadows are currently enabled |
| `setDirectionalShadowResolution(int resolution)` | `void` | Set the shadow map resolution for the directional light (e.g., 1024, 2048, 4096) |
| `setShadowDistance(float distance)` | `void` | Set the maximum distance from the camera at which shadows are rendered |
| `setShadowCascadeCount(int count)` | `void` | Set the number of cascaded shadow map splits (typically 2-4) |
| `setShadowBias(float depthBias, float normalBias)` | `void` | Set depth and normal bias to reduce shadow acne artifacts |
| `setPointShadowResolution(int resolution)` | `void` | Set the shadow map resolution for point light cube maps |
| `setSoftShadows(bool enabled)` | `void` | Enable or disable PCF soft shadow filtering |

## Sub-Contract: `IDebugRendererCore`

Accessed via `bestow.graphics3d.debug` in Lua. Provides immediate-mode debug drawing primitives rendered as overlays. All draw calls accept an optional duration for persistent display.

### Debug Drawing

| Method | Returns | Description |
|--------|---------|-------------|
| `drawLine(Vec3 from, Vec3 to, Color color = Color::white(), float duration = 0)` | `void` | Draw a debug line between two world-space points |
| `drawBox(Vec3 center, Vec3 halfExtents, Quat rotation = {}, Color color = Color::white(), float duration = 0)` | `void` | Draw a wireframe box at the given center with half-extents and optional rotation |
| `drawSphere(Vec3 center, float radius, Color color = Color::white(), int segments = 16, float duration = 0)` | `void` | Draw a wireframe sphere at the given center with the specified radius |
| `drawCapsule(Vec3 center, float halfHeight, float radius, Quat rotation = {}, Color color = Color::white(), float duration = 0)` | `void` | Draw a wireframe capsule shape |
| `drawFrustum(const Mat4& viewProjection, Color color = Color::white(), float duration = 0)` | `void` | Draw a wireframe camera frustum from a view-projection matrix |
| `drawRay(Vec3 origin, Vec3 direction, float length = 10, Color color = Color::white(), float duration = 0)` | `void` | Draw a ray from an origin point along a direction |
| `drawAxes(Vec3 origin, Quat rotation = {}, float size = 1.0f, float duration = 0)` | `void` | Draw RGB coordinate axes (X=red, Y=green, Z=blue) at a position |
| `drawAABB(Vec3 min, Vec3 max, Color color = Color::white(), float duration = 0)` | `void` | Draw a wireframe axis-aligned bounding box from min/max corners |
| `drawGrid(Vec3 origin, int gridSize = 10, float cellSize = 1.0f, Color color = {0.5f,0.5f,0.5f,0.5f})` | `void` | Draw a reference grid on the XZ plane |
| `drawText(Vec3 position, std::string_view text, Color color = Color::white(), float duration = 0)` | `void` | Draw billboard debug text at a world-space position |
| `setEnabled(bool enabled)` | `void` | Globally enable or disable debug rendering |
| `isEnabled()` | `bool` | Return whether debug rendering is currently enabled |
| `clear()` | `void` | Remove all persistent debug draw commands |

## Sub-Contract: `IText3DCore`

Provides 3D world-space text rendering with font management. Used for in-game labels, floating damage numbers, and world-space UI.

### Text Rendering

| Method | Returns | Description |
|--------|---------|-------------|
| `loadFont(AssetHandle fontAsset)` | `Result<FontHandle>` | Load a font from a previously loaded asset and return its handle |
| `destroyFont(FontHandle handle)` | `void` | Destroy a loaded font and free its resources |
| `drawText(Vec3 position, std::string_view text, FontHandle font, float size, Color color = Color::white(), bool billboard = true)` | `void` | Draw text at a world-space position with optional billboarding to face the camera |
| `measureText(std::string_view text, FontHandle font, float size)` | `AABB3D` | Measure the 3D bounding box that the given text would occupy |

## Types

### MeshDef

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `vertices` | `std::span<const Vertex3D>` | -- | Vertex data for the mesh |
| `indices` | `std::span<const std::uint32_t>` | -- | Index data for triangle faces |
| `subMeshes` | `std::vector<SubMesh>` | `{}` | Optional sub-mesh definitions for multi-material meshes |
| `bounds` | `AABB3D` | `{}` | Axis-aligned bounding box of the mesh |
| `isDynamic` | `bool` | `false` | Whether vertices can be updated after creation |

### PBRMaterialDef

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `baseColorFactor` | `Vec4` | `{1,1,1,1}` | Base color multiplier (RGBA) |
| `baseColorTexture` | `AssetHandle` | `0` | Optional albedo texture |
| `metallicFactor` | `float` | `0.0f` | Metallic value (0 = dielectric, 1 = metal) |
| `roughnessFactor` | `float` | `1.0f` | Roughness value (0 = mirror, 1 = rough) |
| `metallicRoughnessTexture` | `AssetHandle` | `0` | Combined metallic-roughness texture |
| `normalTexture` | `AssetHandle` | `0` | Normal map texture |
| `normalScale` | `float` | `1.0f` | Normal map intensity multiplier |
| `occlusionTexture` | `AssetHandle` | `0` | Ambient occlusion texture |
| `occlusionStrength` | `float` | `1.0f` | AO intensity multiplier |
| `emissiveFactor` | `Vec3` | `{0,0,0}` | Emissive color multiplier |
| `emissiveTexture` | `AssetHandle` | `0` | Emissive texture |
| `blendMode` | `BlendMode` | `Opaque` | Blend mode (Opaque, AlphaBlend, Additive) |
| `cullMode` | `CullMode` | `Back` | Face culling mode |
| `alphaCutoff` | `float` | `0.5f` | Alpha test threshold for masked materials |
| `doubleSided` | `bool` | `false` | Whether to render both faces |
| `receiveShadows` | `bool` | `true` | Whether the material receives shadows |
| `castShadows` | `bool` | `true` | Whether the material casts shadows |

### UnlitMaterialDef

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `color` | `Vec4` | `{1,1,1,1}` | Flat color multiplier (RGBA) |
| `texture` | `AssetHandle` | `0` | Optional texture |
| `blendMode` | `BlendMode` | `Opaque` | Blend mode |
| `cullMode` | `CullMode` | `Back` | Face culling mode |

### PrimitiveType

| Value | Description |
|-------|-------------|
| `Cube` | Unit cube centered at origin |
| `Sphere` | UV sphere |
| `Cylinder` | Cylinder along the Y axis |
| `Capsule` | Capsule shape (cylinder with hemisphere caps) |
| `Plane` | Flat quad on the XZ plane |
| `Cone` | Cone along the Y axis |
| `Torus` | Torus (donut shape) |
| `Quad` | Single-sided quad facing +Z |

### PrimitiveParams

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `size` | `float` | `1.0f` | Overall scale of the primitive |
| `segments` | `int` | `32` | Number of horizontal segments (longitude) |
| `rings` | `int` | `16` | Number of vertical rings (latitude) |

### DirectionalLight

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `direction` | `Vec3` | `{0,-1,0}` | Light direction vector (normalized) |
| `color` | `Vec3` | `{1,1,1}` | Light color (RGB) |
| `intensity` | `float` | `1.0f` | Light intensity multiplier |
| `castShadows` | `bool` | `true` | Whether this light casts shadows |
| `shadowMapResolution` | `int` | `2048` | Shadow map texture resolution |

### PointLight

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `color` | `Vec3` | `{1,1,1}` | Light color (RGB) |
| `intensity` | `float` | `1.0f` | Light intensity multiplier |
| `range` | `float` | `10.0f` | Maximum light attenuation range |
| `castShadows` | `bool` | `false` | Whether this light casts shadows |

### SpotLight

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `direction` | `Vec3` | `{0,-1,0}` | Light direction vector |
| `color` | `Vec3` | `{1,1,1}` | Light color (RGB) |
| `intensity` | `float` | `1.0f` | Light intensity multiplier |
| `range` | `float` | `10.0f` | Maximum attenuation range |
| `innerConeAngle` | `float` | `0.4f` | Inner cone half-angle in radians (full intensity) |
| `outerConeAngle` | `float` | `0.5f` | Outer cone half-angle in radians (falloff edge) |
| `castShadows` | `bool` | `false` | Whether this light casts shadows |

### Camera3D

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `transform` | `Transform3D` | `{}` | Camera position, rotation, and scale in world space |
| `projection` | `ProjectionType` | `Perspective` | Projection type (Perspective or Orthographic) |
| `fovY` | `float` | `60.0f` | Vertical field of view in degrees |
| `aspectRatio` | `float` | `16.0f/9.0f` | Width-to-height ratio |
| `orthoWidth` | `float` | `10.0f` | Orthographic view width (when projection is Orthographic) |
| `orthoHeight` | `float` | `10.0f` | Orthographic view height |
| `nearPlane` | `float` | `0.1f` | Near clipping plane distance |
| `farPlane` | `float` | `1000.0f` | Far clipping plane distance |

### FogDef

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `enabled` | `bool` | `false` | Whether fog is active |
| `color` | `Vec3` | `{0.5,0.5,0.6}` | Fog color (RGB) |
| `density` | `float` | `0.01f` | Fog density for exponential modes |
| `startDistance` | `float` | `10.0f` | Distance at which fog begins (linear mode) |
| `endDistance` | `float` | `100.0f` | Distance at which fog reaches full opacity (linear mode) |

### MeshLOD

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `mesh` | `MeshHandle` | -- | Handle to the LOD mesh variant |
| `screenPercentage` | `float` | -- | Switch to this LOD when the mesh covers less than this percentage of the screen |

### Graphics3DConfig

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `windowWidth` | `int` | `1280` | Initial window width in pixels |
| `windowHeight` | `int` | `720` | Initial window height in pixels |
| `windowTitle` | `std::string` | `"Bestow"` | Window title bar text |
| `vsync` | `bool` | `true` | Enable vertical sync |
| `msaaSamples` | `int` | `4` | MSAA sample count (1 = disabled) |
| `maxPointLights` | `int` | `64` | Maximum number of simultaneous point lights |
| `maxSpotLights` | `int` | `32` | Maximum number of simultaneous spot lights |
| `shadowMapResolution` | `int` | `2048` | Default shadow map resolution |

### ToneMappingOp

| Value | Description |
|-------|-------------|
| `Reinhard` | Simple Reinhard tone mapping |
| `ACES` | Academy Color Encoding System (cinematic) |
| `Filmic` | Filmic curve with shoulder and toe |
| `Uncharted2` | Uncharted 2 filmic tone mapping |
| `Linear` | No tone mapping (pass-through) |

### AntiAliasingMode

| Value | Description |
|-------|-------------|
| `None` | No anti-aliasing |
| `FXAA` | Fast approximate anti-aliasing (post-process) |
| `MSAA2x` | 2x multisample anti-aliasing |
| `MSAA4x` | 4x multisample anti-aliasing |
| `MSAA8x` | 8x multisample anti-aliasing |
| `TAA` | Temporal anti-aliasing |

### ColorGradingParams

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `contrast` | `float` | `1.0f` | Contrast multiplier (1.0 = neutral) |
| `saturation` | `float` | `1.0f` | Saturation multiplier (0.0 = grayscale) |
| `brightness` | `float` | `0.0f` | Brightness offset |
| `colorBalance` | `Vec3` | `{1,1,1}` | Per-channel RGB color balance multipliers |
| `gamma` | `float` | `2.2f` | Display gamma value |

## Lua Examples

```lua
-- High-level: Draw a model and set up basic lighting
bestow.graphics3d.setClearColor(0.1, 0.1, 0.15, 1.0)
bestow.graphics3d.setCameraPosition(0, 5, 10)
bestow.graphics3d.setCameraTarget(0, 0, 0)
bestow.graphics3d.setCameraFOV(60)
bestow.graphics3d.setDirectionalLight({0, -1, -0.5}, {1, 0.95, 0.9}, 1.2)
bestow.graphics3d.setAmbientLight({0.2, 0.25, 0.3}, 0.4)

local _, err = bestow.graphics3d.drawModel("models/character.glb", {0, 0, 0})
if err then print("Draw failed: " .. err.message) end

bestow.graphics3d.drawPrimitive("Cube", {3, 0.5, 0}, {1, 1, 1}, {0.8, 0.2, 0.2, 1})

-- Set a skybox
bestow.graphics3d.setSkybox("textures/sky_cubemap.hdr")

-- Low-level: Mesh and material management
local mesh, err = bestow.graphics3d.core.createPrimitiveMesh("Sphere", {size = 2, segments = 64})
if err then return end

local mat, err = bestow.graphics3d.core.createPBRMaterial({
    baseColorFactor = {0.8, 0.1, 0.1, 1.0},
    metallicFactor = 0.9,
    roughnessFactor = 0.2
})
if err then return end

bestow.graphics3d.core.drawMesh(mesh, mat, transform_matrix)

-- Post-processing
bestow.graphics3d.postfx.setToneMapping(true)
bestow.graphics3d.postfx.setToneMappingOperator("ACES")
bestow.graphics3d.postfx.setBloom(true, 0.8, 1.5)
bestow.graphics3d.postfx.setSSAO(true, 0.5, 1.2, 32)

-- Shadows
bestow.graphics3d.shadows.setShadowsEnabled(true)
bestow.graphics3d.shadows.setShadowCascadeCount(4)
bestow.graphics3d.shadows.setSoftShadows(true)

-- Debug rendering
bestow.graphics3d.debug.setEnabled(true)
bestow.graphics3d.debug.drawLine({0,0,0}, {5,5,0}, {1,0,0,1}, 5.0)
bestow.graphics3d.debug.drawSphere({2, 1, 0}, 0.5, {0, 1, 0, 1})
bestow.graphics3d.debug.drawGrid({0, 0, 0}, 20, 1.0)
```

## C++ Examples

```cpp
// High-level: Quick scene setup
graphics3D->setCameraPosition({0, 5, 10});
graphics3D->setCameraTarget({0, 0, 0});
graphics3D->setCameraFOV(60.0f);
graphics3D->setDirectionalLight({0, -1, -0.5f}, Color::white(), 1.2f);
graphics3D->setAmbientLight({0.2f, 0.25f, 0.3f}, 0.4f);

auto result = graphics3D->drawModel("models/tree.glb", {5, 0, 3});
if (!result) {
    spdlog::error("Draw failed: {}", result.error().message);
}

// Low-level: Full mesh/material pipeline
auto meshResult = graphics3DCore->createMesh(MeshDef{
    .vertices = vertexData,
    .indices  = indexData,
    .bounds   = computedBounds
});
if (!meshResult) return;
MeshHandle mesh = *meshResult;

auto matResult = graphics3DCore->createPBRMaterial(PBRMaterialDef{
    .baseColorFactor = {0.8f, 0.1f, 0.1f, 1.0f},
    .metallicFactor  = 0.9f,
    .roughnessFactor = 0.2f,
    .castShadows     = true
});
MaterialHandle mat = *matResult;

// Submit for rendering
graphics3DCore->drawMesh(mesh, mat, worldTransform, true);

// Instanced rendering (e.g., forest of trees)
std::vector<Mat4> treeTransforms = generateForest(500);
graphics3DCore->drawMeshInstanced(treeMesh, treeMat, treeTransforms);

// Skinned mesh with animation
auto boneMatrices = animationCore->samplePose(animator);
graphics3DCore->drawSkinnedMesh(characterMesh, characterMat, worldMatrix, boneMatrices);

// Point light
auto lightId = graphics3DCore->addPointLight(PointLight{
    .color = {1.0f, 0.8f, 0.5f},
    .intensity = 2.0f,
    .range = 15.0f,
    .castShadows = true
});

// Post-processing
postProcessing->setToneMapping(true);
postProcessing->setToneMappingOperator(ToneMappingOp::ACES);
postProcessing->setBloom(true, 0.8f, 1.5f);

// Shadows
shadows->setShadowsEnabled(true);
shadows->setShadowCascadeCount(4);
shadows->setSoftShadows(true);

// Debug drawing
debugRenderer->drawSphere({0, 1, 0}, 0.5f, Color::green(), 16, 5.0f);
debugRenderer->drawAxes({0, 0, 0}, {}, 2.0f);

// LOD setup
graphics3DCore->setMeshLODs(baseMesh, {
    {.mesh = lodMesh1, .screenPercentage = 0.5f},
    {.mesh = lodMesh2, .screenPercentage = 0.2f},
    {.mesh = lodMesh3, .screenPercentage = 0.05f}
});
```
