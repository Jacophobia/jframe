# Bestow Graphics3D System

## Overview

The Graphics3D System is Bestow's 3D rendering subsystem, providing modern PBR (Physically Based Rendering) capabilities, skeletal animation, instanced rendering, debug visualization, and advanced features like frustum culling and LOD (Level of Detail). Built on OpenGL 4.1 Core, it offers a complete 3D rendering pipeline optimized for game development.

### Key Features

- **PBR Rendering** - Physically Based Rendering with metallic-roughness workflow
- **Mesh Management** - Dynamic and static mesh creation with primitive generators
- **Material System** - PBR and unlit materials with texture support
- **Skeletal Animation** - Bone-based animation with blending and sampling
- **Instanced Rendering** - Efficient rendering of multiple instances
- **Lighting** - Directional, point, and spot lights with shadow support
- **Debug Drawing** - Comprehensive 3D debug primitives (lines, boxes, spheres, etc.)
- **Frustum Culling** - Automatic visibility culling for performance
- **LOD System** - Level-of-detail mesh switching based on distance
- **3D Text Rendering** - Billboard and world-space text rendering
- **Post-Processing** - Tone mapping, bloom, SSAO
- **Entity Integration** - Automatic ECS entity rendering

### Architecture

The Graphics3D System follows the Bestow interface pattern:

```
IGraphics3DSystem (interface) <- OpenGLGraphics3DSystem (implementation)
```

The implementation uses:
- **OpenGL 4.1 Core Profile** for rendering
- **GLFW 3.3+** for window management
- **GLM** for 3D math
- **Embedded PBR shaders** for material rendering
- **Frustum culling** for visibility determination

---

## Getting Started

### Initialization

The Graphics3D System is typically used alongside the 2D Graphics System:

```cpp
import bestow;
import bestow.graphics3d;
import bestow.graphics3d.impl;

// Create 3D graphics system (shares context with 2D graphics)
auto graphics3d = bestow::createGraphics3DSystem();
graphics3d->initialize(graphics2d->getNativeWindowHandle());

// Basic setup
graphics3d->setClearColor(Color{135, 206, 235, 255});  // Sky blue
graphics3d->setCamera(camera);
graphics3d->setDirectionalLight(sunLight);
```

### Basic Rendering Loop

```cpp
while (!graphics3d->shouldClose()) {
    graphics3d->beginFrame();

    // Draw meshes
    graphics3d->drawMesh(cubeMesh, material, transform);

    // Or render entities automatically
    graphics3d->renderEntities(entities);

    graphics3d->endFrame();
}
```

---

## Frame Lifecycle

Every frame must be wrapped in `beginFrame()` and `endFrame()` calls:

```cpp
graphics3d->beginFrame();  // Clears screen, resets state

// All rendering calls go here
graphics3d->drawMesh(mesh, material, transform);
graphics3d->debugDrawLine(start, end, Color::red());

graphics3d->endFrame();    // Flushes queue, swaps buffers
```

### What Happens During beginFrame()

1. Clears color and depth buffers
2. Resets render statistics
3. Updates persistent debug lines
4. Prepares OpenGL state for rendering

### What Happens During endFrame()

1. Flushes the render queue (sorts and batches)
2. Renders debug primitives
3. Swaps front/back buffers
4. Polls window events

---

## Camera System

### Camera3D Structure

```cpp
struct Camera3D {
    Transform3D transform;    // Position and rotation
    float fov = 60.0f;       // Field of view (degrees)
    float nearPlane = 0.1f;  // Near clipping plane
    float farPlane = 1000.0f; // Far clipping plane
    Size viewportSize;       // Screen dimensions
};
```

### Setting Up a Camera

```cpp
Camera3D camera;
camera.transform.position = {0.0f, 5.0f, 10.0f};
camera.transform.rotation = glm::quatLookAt(
    glm::normalize(glm::vec3{0.0f, -0.5f, -1.0f}),  // Forward
    glm::vec3{0.0f, 1.0f, 0.0f}                     // Up
);
camera.fov = 60.0f;
camera.nearPlane = 0.1f;
camera.farPlane = 500.0f;
camera.viewportSize = graphics3d->getWindowSize();

graphics3d->setCamera(camera);
```

### Camera Movement

```cpp
// First-person camera movement
void updateCamera(DeltaTime dt) {
    Camera3D cam = graphics3d->getCamera();

    // Get camera forward/right vectors
    Vec3 forward = cam.transform.forward();
    Vec3 right = cam.transform.right();

    float speed = 10.0f * dt;

    if (input->isKeyPressed(GLFW_KEY_W))
        cam.transform.position += forward * speed;
    if (input->isKeyPressed(GLFW_KEY_S))
        cam.transform.position -= forward * speed;
    if (input->isKeyPressed(GLFW_KEY_A))
        cam.transform.position -= right * speed;
    if (input->isKeyPressed(GLFW_KEY_D))
        cam.transform.position += right * speed;

    graphics3d->setCamera(cam);
}
```

### Mouse Look

```cpp
// Rotate camera with mouse
void handleMouseLook(Vec2 mouseDelta) {
    Camera3D cam = graphics3d->getCamera();

    float sensitivity = 0.002f;

    // Yaw (horizontal rotation around Y axis)
    Quat yaw = glm::angleAxis(-mouseDelta.x * sensitivity, Vec3{0.0f, 1.0f, 0.0f});

    // Pitch (vertical rotation around local right axis)
    Vec3 right = cam.transform.right();
    Quat pitch = glm::angleAxis(-mouseDelta.y * sensitivity, right);

    // Apply rotations
    cam.transform.rotation = glm::normalize(yaw * cam.transform.rotation * pitch);

    graphics3d->setCamera(cam);
}
```

### Coordinate Conversion

```cpp
// Convert screen position to world ray (for mouse picking)
Vec2 mousePos = input->getMousePosition();
Ray3D ray = graphics3d->screenToWorldRay(mousePos);

// Cast ray into scene
auto hit = physics->raycast(ray.origin, ray.direction, 1000.0f);
if (hit) {
    // Mouse is pointing at an object
}

// Convert world position to screen position
Vec3 worldPos = entity.transform.position;
auto screenPos = graphics3d->worldToScreen(worldPos);
if (screenPos) {
    // Draw HUD marker at screen position
    graphics2d->drawCircle(*screenPos, 5.0f, Color::red());
}
```

---

## Mesh Management

### Creating Meshes from Data

```cpp
// Define vertices and indices
std::vector<Vertex3D> vertices = {
    {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {255, 255, 255, 255}},
    {{ 0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {255, 255, 255, 255}},
    {{ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {255, 255, 255, 255}},
    {{-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {255, 255, 255, 255}}
};

std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0};

MeshDef meshDef{
    .vertices = vertices,
    .indices = indices,
    .bounds = AABB3D{{-0.5f, -0.5f, 0.0f}, {0.5f, 0.5f, 0.0f}},
    .isDynamic = false
};

auto meshResult = graphics3d->createMesh(meshDef);
if (meshResult) {
    MeshHandle mesh = *meshResult;
    // Use mesh for rendering
}
```

### Primitive Mesh Generation

```cpp
// Create a cube (1x1x1, centered at origin)
auto cubeResult = graphics3d->createCubeMesh(1.0f);
MeshHandle cube = cubeResult.value();

// Create a sphere (radius 0.5, 32 segments, 16 rings)
auto sphereResult = graphics3d->createSphereMesh(0.5f, 32, 16);
MeshHandle sphere = sphereResult.value();

// Create a cylinder (radius 0.5, height 2.0, 32 segments)
auto cylinderResult = graphics3d->createCylinderMesh(0.5f, 2.0f, 32);
MeshHandle cylinder = cylinderResult.value();

// Create a capsule (radius 0.3, height 1.0)
auto capsuleResult = graphics3d->createCapsuleMesh(0.3f, 1.0f, 32, 8);
MeshHandle capsule = capsuleResult.value();

// Create a plane (width 10, height 10, subdivided 10x10)
auto planeResult = graphics3d->createPlaneMesh(10.0f, 10.0f, 10, 10);
MeshHandle ground = planeResult.value();
```

### Dynamic Mesh Updates

```cpp
// Create dynamic mesh
MeshDef dynamicMeshDef{
    .vertices = vertices,
    .indices = indices,
    .isDynamic = true  // Allow vertex updates
};

auto meshResult = graphics3d->createMesh(dynamicMeshDef);
MeshHandle dynamicMesh = meshResult.value();

// Later, update vertices
std::vector<Vertex3D> newVertices = generateDeformedVertices();
graphics3d->updateMeshVertices(dynamicMesh, newVertices);
```

### Mesh Bounds

```cpp
// Get mesh bounding box
AABB3D bounds = graphics3d->getMeshBounds(mesh);

// Use for culling
Vec3 center = (bounds.min + bounds.max) * 0.5f;
Vec3 extents = (bounds.max - bounds.min) * 0.5f;
```

### Destroying Meshes

```cpp
graphics3d->destroyMesh(mesh);
```

---

## Material System

### PBR Materials

```cpp
PBRMaterial pbrMat;
pbrMat.baseColorFactor = {1.0f, 0.8f, 0.6f, 1.0f};  // Slight orange tint
pbrMat.metallicFactor = 0.0f;     // Non-metallic
pbrMat.roughnessFactor = 0.8f;    // Fairly rough
pbrMat.emissiveFactor = {0.0f, 0.0f, 0.0f};
pbrMat.blendMode = BlendMode::Opaque;
pbrMat.cullMode = CullMode::Back;
pbrMat.doubleSided = false;
pbrMat.receiveShadows = true;
pbrMat.castShadows = true;

auto matResult = graphics3d->createMaterial(pbrMat);
MaterialHandle material = matResult.value();
```

### PBR with Textures

```cpp
// Load textures via asset system
AssetHandle baseColorTex = assets->registerAsset(AssetType::Texture, "textures/brick_color.png");
AssetHandle normalTex = assets->registerAsset(AssetType::Texture, "textures/brick_normal.png");
AssetHandle metalRoughTex = assets->registerAsset(AssetType::Texture, "textures/brick_metalrough.png");

assets->loadAssetSync(baseColorTex);
assets->loadAssetSync(normalTex);
assets->loadAssetSync(metalRoughTex);

// Create material with textures
PBRMaterial texMat;
texMat.baseColorTexture = baseColorTex;
texMat.normalTexture = normalTex;
texMat.metallicRoughnessTexture = metalRoughTex;
texMat.normalScale = 1.0f;

auto matResult = graphics3d->createMaterial(texMat);
```

### Unlit Materials

```cpp
UnlitMaterial unlitMat;
unlitMat.color = {1.0f, 1.0f, 1.0f, 1.0f};
unlitMat.texture = skyTexture;
unlitMat.blendMode = BlendMode::Opaque;
unlitMat.cullMode = CullMode::Back;

auto matResult = graphics3d->createUnlitMaterial(unlitMat);
```

### Default Materials

```cpp
// Get pre-created materials
MaterialHandle defaultPBR = graphics3d->getDefaultPBRMaterial();
MaterialHandle defaultUnlit = graphics3d->getDefaultUnlitMaterial();
MaterialHandle error = graphics3d->getErrorMaterial();  // Checkerboard pattern
```

### Updating Material Properties

```cpp
// Change base color
graphics3d->setMaterialBaseColor(material, {1.0f, 0.0f, 0.0f, 1.0f});

// Change metallic and roughness
graphics3d->setMaterialMetallicRoughness(material, 0.9f, 0.1f);  // Very metallic, very smooth

// Make material glow
graphics3d->setMaterialEmissive(material, {1.0f, 0.5f, 0.0f});  // Orange glow
```

### Destroying Materials

```cpp
graphics3d->destroyMaterial(material);
```

---

## Rendering

### Immediate Mode Rendering

```cpp
// Draw mesh with matrix
Mat4 worldMatrix = glm::translate(glm::mat4(1.0f), Vec3{5.0f, 0.0f, 0.0f});
graphics3d->drawMesh(mesh, material, worldMatrix);

// Draw mesh with transform
Transform3D transform;
transform.position = {10.0f, 2.0f, 0.0f};
transform.rotation = glm::angleAxis(glm::radians(45.0f), Vec3{0.0f, 1.0f, 0.0f});
transform.scale = {2.0f, 2.0f, 2.0f};

graphics3d->drawMesh(mesh, material, transform);
```

### Batch Rendering

```cpp
// Queue multiple render items
std::vector<RenderItem> items;

for (int i = 0; i < 100; i++) {
    RenderItem item;
    item.mesh = cubeMesh;
    item.material = material;
    item.worldMatrix = glm::translate(glm::mat4(1.0f), Vec3{i * 2.0f, 0.0f, 0.0f});
    item.layer = 0;
    item.castShadow = true;
    item.receiveShadow = true;
    items.push_back(item);
}

graphics3d->queueRenderItems(items);

// Flush at end of frame (or manually)
graphics3d->flushRenderQueue();
```

### Entity Rendering (ECS)

```cpp
// Render all entities with Mesh3D and Transform3D components
graphics3d->renderEntities(entities);

// Render with frustum culling
Frustum frustum = calculateViewFrustum(camera);
graphics3d->renderEntities(entities, frustum);

// Render specific layer range
graphics3d->renderEntities(entities, 0, 10);  // Layers 0-10 only
```

### Instanced Rendering

```cpp
// Create instance buffer
auto bufferResult = graphics3d->createInstanceBuffer(1000, true);
InstanceBufferHandle instanceBuffer = bufferResult.value();

// Prepare instance data
std::vector<InstanceData> instances;
for (int i = 0; i < 1000; i++) {
    InstanceData inst;
    inst.worldMatrix = glm::translate(glm::mat4(1.0f), randomPosition());
    inst.customData = {randomColor(), 0.0f};  // Pack color in custom data
    instances.push_back(inst);
}

// Update buffer
graphics3d->updateInstanceBuffer(instanceBuffer, instances);

// Draw instanced
InstancedRenderItem instItem;
instItem.mesh = cubeMesh;
instItem.material = material;
instItem.instances = instanceBuffer;
instItem.instanceCount = instances.size();

graphics3d->drawInstanced(instItem);
```

---

## Lighting

### Directional Light (Sun)

```cpp
DirectionalLight sun;
sun.direction = glm::normalize(Vec3{0.3f, -1.0f, 0.5f});
sun.color = {1.0f, 0.95f, 0.8f};  // Warm sunlight
sun.intensity = 1.2f;
sun.castShadows = true;
sun.shadowMapResolution = 2048;

graphics3d->setDirectionalLight(sun);

// Remove directional light
graphics3d->clearDirectionalLight();
```

### Point Lights

```cpp
PointLight pointLight;
pointLight.color = {1.0f, 0.8f, 0.5f};  // Warm light
pointLight.intensity = 2.0f;
pointLight.range = 15.0f;
pointLight.castShadows = false;

uint32_t lightId = graphics3d->addPointLight(pointLight, Vec3{5.0f, 3.0f, 0.0f});

// Move light
graphics3d->setLightPosition(lightId, Vec3{10.0f, 3.0f, 5.0f});

// Remove light
graphics3d->removeLight(lightId);
```

### Spot Lights

```cpp
SpotLight spotlight;
spotlight.direction = {0.0f, -1.0f, 0.0f};
spotlight.color = {1.0f, 1.0f, 1.0f};
spotlight.intensity = 5.0f;
spotlight.range = 20.0f;
spotlight.innerConeAngle = glm::radians(15.0f);  // Bright center
spotlight.outerConeAngle = glm::radians(30.0f);  // Falloff edge
spotlight.castShadows = true;

uint32_t spotId = graphics3d->addSpotLight(spotlight, Vec3{0.0f, 10.0f, 0.0f});
```

### Ambient Light

```cpp
// Set global ambient lighting
graphics3d->setAmbientLight(Vec3{0.1f, 0.12f, 0.15f}, 1.0f);  // Cool ambient
```

### Entity Light Integration

```cpp
// Automatically update lights from entities with light components
graphics3d->updateEntityLights(entities);
```

---

## Shadows

```cpp
// Enable shadows
graphics3d->setShadowsEnabled(true);

// Set directional shadow resolution (higher = sharper shadows, lower performance)
graphics3d->setDirectionalShadowResolution(4096);

// Set shadow distance (objects beyond this don't cast shadows)
graphics3d->setShadowDistance(100.0f);

// Check if shadows are enabled
bool enabled = graphics3d->areShadowsEnabled();
```

---

## Debug Drawing

### Lines

```cpp
// Draw a line
graphics3d->debugDrawLine(
    Vec3{0.0f, 0.0f, 0.0f},      // Start
    Vec3{10.0f, 0.0f, 0.0f},     // End
    Color::red(),                // Color
    0.0f,                        // Duration (0 = one frame)
    true                         // Depth test
);

// Persistent line (stays for 5 seconds)
graphics3d->debugDrawLine(
    start, end, Color::green(),
    5.0f,   // 5 seconds
    true
);

// Line that draws on top of everything (no depth test)
graphics3d->debugDrawLine(
    start, end, Color::blue(),
    0.0f,
    false  // No depth test
);
```

### Boxes

```cpp
// Draw an axis-aligned box
graphics3d->debugDrawBox(
    Vec3{0.0f, 1.0f, 0.0f},      // Center
    Vec3{0.5f, 0.5f, 0.5f},      // Half extents
    Quat{1.0f, 0.0f, 0.0f, 0.0f}, // No rotation
    Color::green()
);

// Draw a rotated box
Quat rotation = glm::angleAxis(glm::radians(45.0f), Vec3{0.0f, 1.0f, 0.0f});
graphics3d->debugDrawBox(
    center, halfExtents, rotation,
    Color::yellow()
);
```

### Spheres

```cpp
// Draw a sphere
graphics3d->debugDrawSphere(
    Vec3{0.0f, 2.0f, 0.0f},  // Center
    1.0f,                    // Radius
    Color::cyan()
);
```

### Capsules

```cpp
// Draw a capsule (useful for character collision visualization)
graphics3d->debugDrawCapsule(
    Vec3{0.0f, 0.5f, 0.0f},  // Bottom center
    Vec3{0.0f, 2.0f, 0.0f},  // Top center
    0.3f,                    // Radius
    Color::magenta()
);
```

### Frustums

```cpp
// Draw camera frustum
Frustum frustum = calculateViewFrustum(camera);
graphics3d->debugDrawFrustum(frustum, Color::white());
```

### Rays

```cpp
// Draw a ray (useful for raycasts)
graphics3d->debugDrawRay(
    Vec3{0.0f, 1.0f, 0.0f},          // Origin
    glm::normalize(Vec3{1.0f, 0.0f, 1.0f}),  // Direction
    10.0f,                           // Length
    Color::red()
);
```

### Coordinate Axes

```cpp
// Draw transform axes (red=X, green=Y, blue=Z)
Transform3D transform;
transform.position = {5.0f, 0.0f, 0.0f};
graphics3d->debugDrawAxes(transform, 1.0f);
```

### AABBs

```cpp
// Draw bounding box
AABB3D bounds = graphics3d->getMeshBounds(mesh);
graphics3d->debugDrawAABB(bounds, Color::yellow());
```

### Debug Rendering Control

```cpp
// Enable/disable all debug rendering
graphics3d->setDebugRenderingEnabled(true);
graphics3d->setDebugRenderingEnabled(false);

// Clear all persistent debug lines
graphics3d->debugClear();

// Check if enabled
bool enabled = graphics3d->isDebugRenderingEnabled();
```

---

## Frustum Culling

```cpp
// Enable frustum culling (automatically cull objects outside view)
graphics3d->setFrustumCulling(true);

// Disable frustum culling (render all objects)
graphics3d->setFrustumCulling(false);

// Check if enabled
bool enabled = graphics3d->isFrustumCullingEnabled();
```

The frustum culling system automatically:
- Calculates the view frustum from the camera
- Tests mesh bounds against the frustum
- Skips rendering for objects outside the view
- Updates render statistics with culled object counts

---

## Level of Detail (LOD)

### Setting LOD Distances

```cpp
// Define LOD switch distances
std::vector<float> lodDistances = {20.0f, 50.0f, 100.0f};
graphics3d->setLODDistances(lodDistances);
// LOD0 (highest detail): 0-20 units
// LOD1: 20-50 units
// LOD2: 50-100 units
// LOD3 (lowest detail): 100+ units
```

### Registering LOD Meshes

```cpp
// Create LOD meshes (progressively simpler)
auto lod0 = graphics3d->createSphereMesh(1.0f, 64, 32);  // High detail
auto lod1 = graphics3d->createSphereMesh(1.0f, 32, 16);  // Medium detail
auto lod2 = graphics3d->createSphereMesh(1.0f, 16, 8);   // Low detail
auto lod3 = graphics3d->createSphereMesh(1.0f, 8, 4);    // Very low detail

std::vector<MeshHandle> lodMeshes = {lod1.value(), lod2.value(), lod3.value()};
graphics3d->registerLODMeshes(lod0.value(), lodMeshes);
```

### LOD Bias

```cpp
// Positive bias = use higher detail LODs
// Negative bias = use lower detail LODs
graphics3d->setLODBias(0.5f);  // Prefer higher quality
graphics3d->setLODBias(-0.5f); // Prefer performance
```

---

## Skeletal Animation

### Creating Skeletons

```cpp
// Load model with skeleton
AssetHandle modelAsset = assets->registerAsset(AssetType::Model, "models/character.gltf");
assets->loadAssetSync(modelAsset);

ModelData* modelData = assets->getModelData(modelAsset);

// Create skeleton from model
auto skelResult = graphics3d->createSkeleton(*modelData);
SkeletonHandle skeleton = skelResult.value();
```

### Loading Animation Clips

```cpp
// Create animation clip from model data
auto clipResult = graphics3d->createAnimationClip(
    skeleton,
    "run",      // Clip name
    *modelData
);
AnimationClipHandle runClip = clipResult.value();

// Get clip info
AnimationClip info = graphics3d->getAnimationClipInfo(runClip);
std::println("Clip: {}, Duration: {}s", info.name, info.duration);
```

### Sampling Animations

```cpp
// Sample animation at specific time
float time = 2.5f;
std::vector<Mat4> boneTransforms = graphics3d->sampleAnimation(
    runClip,
    time,
    true  // Loop
);

// Draw skinned mesh
graphics3d->drawSkinnedMesh(
    characterMesh,
    characterMaterial,
    worldMatrix,
    boneTransforms
);
```

### Animation Blending

```cpp
// Blend multiple animations
BlendedAnimation blend;

AnimationState idle;
idle.clip = idleClip;
idle.time = idleTime;
idle.weight = 0.3f;
idle.playing = true;

AnimationState run;
run.clip = runClip;
run.time = runTime;
run.weight = 0.7f;
run.playing = true;

blend.layers = {idle, run};
blend.crossfadeDuration = 0.25f;

std::vector<Mat4> blendedTransforms = graphics3d->blendAnimations(blend);
```

---

## 3D Text Rendering

### Loading Fonts

```cpp
// Load font for 3D text
AssetHandle fontAsset = assets->registerAsset(AssetType::Font, "fonts/Roboto-Regular.ttf");
assets->loadAssetSync(fontAsset);

auto fontResult = graphics3d->loadFont3D(fontAsset);
Font3DHandle font3d = fontResult.value();
```

### Drawing 3D Text

```cpp
// Simple 3D text
graphics3d->drawText3D(
    "Hello World",
    Vec3{0.0f, 2.0f, 0.0f},  // Position
    font3d,
    1.0f,                     // Font size
    Color::white()
);

// Advanced 3D text with styling
Text3DStyle style;
style.font = font3d;
style.fontSize = 0.5f;
style.color = Color::yellow();
style.alignment = TextAlignment3D::Center;
style.verticalAlign = TextVerticalAlign3D::Middle;
style.billboard = true;  // Always face camera
style.outlineWidth = 0.05f;
style.outlineColor = Color::black();

Text3DItem textItem;
textItem.text = "Score: 1000";
textItem.style = style;
textItem.transform.position = {0.0f, 5.0f, 0.0f};

graphics3d->drawText3D(textItem);
```

### Measuring Text

```cpp
// Get text bounds
AABB3D bounds = graphics3d->measureText3D("Player Name", font3d, 1.0f);
Vec3 size = bounds.max - bounds.min;
```

---

## Environment

### Skybox

```cpp
// Load cubemap textures
AssetHandle skyboxCubemap = assets->registerAsset(
    AssetType::Cubemap,
    "textures/skybox"
);
assets->loadAssetSync(skyboxCubemap);

Skybox skybox;
skybox.cubemapTexture = skyboxCubemap;
skybox.rotation = 0.0f;
skybox.exposure = 1.2f;

graphics3d->setSkybox(skybox);

// Remove skybox
graphics3d->clearSkybox();
```

### Environment Maps (IBL)

```cpp
EnvironmentMap envMap;
envMap.irradianceMap = irradianceAsset;
envMap.prefilteredMap = prefilteredAsset;
envMap.brdfLUT = brdfAsset;
envMap.intensity = 1.5f;

graphics3d->setEnvironmentMap(envMap);

// Remove environment map
graphics3d->clearEnvironmentMap();
```

### Fog

```cpp
Fog fog;
fog.enabled = true;
fog.color = {0.7f, 0.8f, 0.9f};  // Bluish fog
fog.density = 0.02f;
fog.start = 10.0f;
fog.end = 100.0f;

graphics3d->setFog(fog);
```

---

## Post-Processing

### Tone Mapping

```cpp
// Enable tone mapping (for HDR rendering)
graphics3d->setToneMapping(true);
graphics3d->setExposure(1.5f);  // Brighten scene
```

### Bloom

```cpp
// Enable bloom effect
graphics3d->setBloom(
    true,     // Enable
    1.0f,     // Threshold (bright areas above this value)
    0.8f      // Intensity
);
```

### SSAO (Screen Space Ambient Occlusion)

```cpp
// Enable SSAO
graphics3d->setSSAO(
    true,     // Enable
    0.5f,     // Sample radius
    1.2f      // Intensity
);
```

---

## Render Statistics

```cpp
// Get render statistics
RenderStats stats = graphics3d->getStats();

std::println("Draw calls: {}", stats.drawCalls);
std::println("Triangles: {}", stats.triangles);
std::println("Vertices: {}", stats.vertices);
std::println("Meshes: {}", stats.meshes);
std::println("Materials: {}", stats.materials);
std::println("Lights: {}", stats.lights);
std::println("Visible objects: {}", stats.visibleObjects);
std::println("Culled objects: {}", stats.culledObjects);
std::println("Frame time: {}ms", stats.frameTimeMs);
std::println("GPU time: {}ms", stats.gpuTimeMs);
```

---

## Window Management

```cpp
// Get window size
Size size = graphics3d->getWindowSize();

// Set window size
graphics3d->setWindowSize(Size{1920, 1080});

// Fullscreen toggle
graphics3d->setFullscreen(true);
bool isFullscreen = graphics3d->isFullscreen();

// Check if window should close
if (graphics3d->shouldClose()) {
    // Exit game
}

// Get native window handle (GLFW)
void* handle = graphics3d->getNativeWindowHandle();
GLFWwindow* window = static_cast<GLFWwindow*>(handle);
```

---

## Render State

```cpp
// Set clear color
graphics3d->setClearColor(Color{50, 100, 150, 255});

// Enable/disable VSync
graphics3d->setVSync(true);

// Set render scale (for resolution scaling)
graphics3d->setRenderScale(0.75f);  // Render at 75% resolution
float scale = graphics3d->getRenderScale();
```

---

## Asset System Integration

```cpp
// Set asset system
graphics3d->setAssetSystem(assets);

// Create mesh from asset data
MeshData* meshData = assets->getMeshData(meshAsset);
auto meshResult = graphics3d->createMeshFromData(*meshData);

// Create material from asset data
MaterialData* matData = assets->getMaterialData(matAsset);
auto matResult = graphics3d->createMaterialFromData(*matData);

// Create multiple materials from model
ModelData* modelData = assets->getModelData(modelAsset);
auto materialsResult = graphics3d->createMaterialsFromModel(*modelData);
std::vector<MaterialHandle> materials = materialsResult.value();

// Create skybox from cubemap data
CubemapData* cubemapData = assets->getCubemapData(cubemapAsset);
graphics3d->createSkyboxFromData(*cubemapData);
```

---

## Complete Example

Here's a complete 3D rendering example:

```cpp
import bestow;
import bestow.graphics3d;
import bestow.graphics3d.impl;

class Game3D {
public:
    bool initialize() {
        // Initialize systems
        graphics3d_ = bestow::createGraphics3DSystem();
        graphics3d_->initialize();
        graphics3d_->setAssetSystem(assets_);

        // Setup camera
        camera_.transform.position = {0.0f, 5.0f, 10.0f};
        camera_.fov = 60.0f;
        camera_.nearPlane = 0.1f;
        camera_.farPlane = 500.0f;
        camera_.viewportSize = graphics3d_->getWindowSize();
        graphics3d_->setCamera(camera_);

        // Setup lighting
        DirectionalLight sun;
        sun.direction = glm::normalize(Vec3{0.3f, -1.0f, 0.5f});
        sun.color = {1.0f, 0.95f, 0.8f};
        sun.intensity = 1.2f;
        graphics3d_->setDirectionalLight(sun);
        graphics3d_->setAmbientLight(Vec3{0.1f, 0.12f, 0.15f}, 1.0f);

        // Create meshes
        auto cubeResult = graphics3d_->createCubeMesh(1.0f);
        cubeMesh_ = cubeResult.value();

        auto groundResult = graphics3d_->createPlaneMesh(50.0f, 50.0f, 10, 10);
        groundMesh_ = groundResult.value();

        // Create materials
        PBRMaterial cubeMat;
        cubeMat.baseColorFactor = {0.8f, 0.3f, 0.2f, 1.0f};
        cubeMat.metallicFactor = 0.1f;
        cubeMat.roughnessFactor = 0.7f;
        auto cubeMatResult = graphics3d_->createMaterial(cubeMat);
        cubeMaterial_ = cubeMatResult.value();

        PBRMaterial groundMat;
        groundMat.baseColorFactor = {0.3f, 0.6f, 0.3f, 1.0f};
        groundMat.roughnessFactor = 1.0f;
        auto groundMatResult = graphics3d_->createMaterial(groundMat);
        groundMaterial_ = groundMatResult.value();

        return true;
    }

    void update(DeltaTime dt) {
        // Update camera
        updateCamera(dt);

        // Rotate cube
        cubeRotation_ += dt;
    }

    void render() {
        graphics3d_->beginFrame();

        // Draw ground
        Transform3D groundTransform;
        groundTransform.position = {0.0f, 0.0f, 0.0f};
        graphics3d_->drawMesh(groundMesh_, groundMaterial_, groundTransform);

        // Draw rotating cube
        Transform3D cubeTransform;
        cubeTransform.position = {0.0f, 1.0f, 0.0f};
        cubeTransform.rotation = glm::angleAxis(
            cubeRotation_,
            glm::normalize(Vec3{0.0f, 1.0f, 0.0f})
        );
        graphics3d_->drawMesh(cubeMesh_, cubeMaterial_, cubeTransform);

        // Debug visualization
        graphics3d_->debugDrawAxes(cubeTransform, 1.5f);
        graphics3d_->debugDrawSphere(cubeTransform.position, 0.5f, Color::yellow());

        graphics3d_->endFrame();
    }

    void updateCamera(DeltaTime dt) {
        // Simple orbit camera
        float speed = 1.0f * dt;
        cameraAngle_ += speed;

        float radius = 10.0f;
        camera_.transform.position = {
            std::cos(cameraAngle_) * radius,
            5.0f,
            std::sin(cameraAngle_) * radius
        };

        // Look at origin
        Vec3 forward = glm::normalize(Vec3{0.0f} - camera_.transform.position);
        camera_.transform.rotation = glm::quatLookAt(forward, Vec3{0.0f, 1.0f, 0.0f});

        graphics3d_->setCamera(camera_);
    }

private:
    std::unique_ptr<IGraphics3DSystem> graphics3d_;
    Camera3D camera_;
    MeshHandle cubeMesh_;
    MeshHandle groundMesh_;
    MaterialHandle cubeMaterial_;
    MaterialHandle groundMaterial_;
    float cubeRotation_ = 0.0f;
    float cameraAngle_ = 0.0f;
};
```

---

## Best Practices

### Performance

1. **Use Instanced Rendering** - For rendering many identical objects
2. **Enable Frustum Culling** - Automatically skip off-screen objects
3. **Use LOD System** - Switch to simpler meshes at distance
4. **Batch Render Items** - Use `queueRenderItems()` instead of individual `drawMesh()` calls
5. **Limit Shadow Casters** - Not all objects need to cast shadows
6. **Optimize Mesh Complexity** - Use lower poly counts where possible
7. **Reuse Materials** - Share materials across multiple meshes

### Organization

1. **Define Material Constants** - Create named materials for common uses
2. **Cache Handles** - Don't create/destroy resources every frame
3. **Use Entity Rendering** - Let the system handle ECS rendering automatically
4. **Separate Debug Code** - Wrap debug drawing in `#if defined(BESTOW_DEV_TOOLS)`

### Lighting

1. **Limit Dynamic Lights** - Too many lights hurt performance
2. **Use Baked Lighting** - For static scenes, bake lighting into textures
3. **Shadow Distance** - Set reasonable shadow distance, not infinite
4. **Directional Light** - Use only one directional light (the sun)

---

## Troubleshooting

### Meshes Not Appearing

- Check that mesh handle is valid: `graphics3d->hasMesh(mesh)`
- Verify camera is set and positioned correctly
- Check material is valid: `graphics3d->hasMaterial(material)`
- Ensure frustum culling isn't culling the object incorrectly
- Verify mesh is within camera view frustum

### Performance Issues

- Check render statistics: `graphics3d->getStats()`
- Enable frustum culling: `graphics3d->setFrustumCulling(true)`
- Use LOD system for distant objects
- Reduce shadow resolution: `graphics3d->setDirectionalShadowResolution(1024)`
- Disable expensive post-processing (SSAO, bloom)
- Use instanced rendering for repeated objects

### Dark Scene

- Check directional light is set
- Verify ambient light is set: `graphics3d->setAmbientLight(...)`
- Ensure material isn't fully black
- Check camera isn't inside geometry

### Debug Lines Not Appearing

- Verify debug rendering is enabled: `graphics3d->setDebugRenderingEnabled(true)`
- Check debug lines are being called every frame (if duration = 0)
- Ensure camera can see the debug geometry

---

## See Also

- [Graphics System (2D)](Graphics-System.md) - 2D rendering companion
- [Camera System](Camera-System.md) - 2D camera controls
- [Entity System](Entity-System.md) - ECS for managing 3D entities
- [Asset System](Assets-System.md) - Loading 3D models and textures
- [Physics System](Physics-System.md) - 3D physics simulation
- Technical Design: `docs/bestow-technical-design.md`
