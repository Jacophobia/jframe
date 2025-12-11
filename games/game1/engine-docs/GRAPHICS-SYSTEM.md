# Bestow Graphics System Guide

> **Comprehensive guide for 2D and 3D rendering in Bestow Engine**

## Table of Contents

1. [Overview](#overview)
2. [Renderer Selection](#renderer-selection)
3. [2D Graphics API](#2d-graphics-api)
4. [3D Graphics API](#3d-graphics-api)
5. [Shader System](#shader-system)
6. [Material System](#material-system)
7. [Best Practices](#best-practices)
8. [Complete Examples](#complete-examples)

---

## Overview

Bestow provides two comprehensive graphics systems:

- **IGraphicsSystem** - 2D rendering for sprites, primitives, and text
- **IGraphics3DSystem** - 3D rendering for meshes, materials, lighting, and effects

Both systems support **two rendering backends**:

| Backend | Status | Use Case |
|---------|--------|----------|
| **Vulkan** | **Primary** | Production, best performance, advanced features |
| **OpenGL** | Fallback | Rapid prototyping, debugging, older hardware |

### Key Features

**2D Rendering:**
- Sprite batching for optimal performance
- Sprite sheets and animations
- Debug shapes (rectangles, circles, lines, polygons)
- MSDF text rendering with font atlases
- Camera system with zoom and viewport culling

**3D Rendering:**
- PBR and unlit materials
- Directional, point, and spot lights
- Shadows, fog, and post-processing
- Mesh generation (cube, sphere, cylinder, capsule, plane)
- Skeletal animation and LOD support
- Debug visualization (lines, boxes, spheres, axes)
- Hot-reloadable Lua materials

---

## Renderer Selection

### Vulkan (Primary)

**Vulkan is the recommended renderer for production games.** It provides:

- Better performance on modern hardware
- Advanced features (compute shaders, descriptor sets)
- GLSL hot reload with runtime SPIR-V compilation
- Comprehensive validation layers for debugging

```cpp
import bestow;

int main() {
    Engine engine;

    // Register Vulkan renderer (primary choice)
    engine.registerSystem<IGraphics3DSystem, VulkanGraphics3DSystem>();

    engine.run<MyGame>();
}
```

### OpenGL (Fallback)

OpenGL is available for **rapid prototyping** and **older hardware support**:

```cpp
import bestow;

int main() {
    Engine engine;

    // Register OpenGL renderer (fallback)
    engine.registerSystem<IGraphics3DSystem, OpenGLGraphics3DSystem>();

    engine.run<MyGame>();
}
```

### Runtime Configuration

Both renderers support hot-reloadable configuration via `config/graphics3d.lua`:

```lua
return {
    -- Unified settings (apply to all backends)
    gammaCorrection = true,
    msaaSamples = 4,
    vsync = "FIFO",  -- "Immediate", "FIFO", "Mailbox"

    -- Lighting (in LINEAR space when gammaCorrection=true)
    lightDirection = {0.5, -1.0, 0.3},
    lightColor = {1.0, 0.89, 0.79},  -- Warm sunlight
    ambientColor = {0.13, 0.17, 0.26},  -- Cool sky
    ambientIntensity = 0.3,

    clearColor = {61, 158, 212, 255},  -- Sky blue

    -- Debug
    debugWireframe = false,
    debugShowNormals = false,
    debugShowFps = true,
    hotReload = true
}
```

---

## 2D Graphics API

### Frame Management

Every frame must begin and end properly:

```cpp
void MyGame::render() {
    graphics->beginFrame();

    // All rendering calls go here

    graphics->endFrame();
}
```

### Sprite Rendering

#### Simple Sprite

```cpp
// Load texture through AssetSystem
AssetHandle playerTexture = assets->loadTexture("textures/player.png");

// Draw sprite
Sprite sprite{
    .textureHandle = &playerTexture,
    .sourceRect = Canvas{{0, 0}, {32, 32}},  // Source region
    .transform = Transform2D{100, 200, 0.0f, 1.0f, 1.0f},
    .tint = Color::white(),
    .layer = RenderLayers::Player,
    .anchor = {0.5f, 0.5f}  // Center pivot
};

graphics->draw(sprite);
```

#### Batch Rendering

Batch sprites for better performance:

```cpp
std::vector<Sprite> enemies;
for (const auto& enemy : enemyList) {
    enemies.push_back(Sprite{
        .textureHandle = &enemyTexture,
        .transform = enemy.transform,
        .tint = enemy.color,
        .layer = RenderLayers::Enemies
    });
}

graphics->drawBatch(enemies);  // Single draw call
```

#### Sprite Sheets

```cpp
// Define sprite sheet layout
SpriteSheet coinSheet{
    .texture = coinTexture,
    .frameWidth = 16,
    .frameHeight = 16,
    .columns = 8,
    .rows = 1,
    .padding = 0
};

// Draw specific frame
graphics->drawSprite(coinSheet, frameIndex, transform, Color::white());
```

#### Animated Sprites

```cpp
// Define animation
AnimatedSprite player;
player.sheet = playerSheet;
player.animations["idle"] = Animation{
    .name = "idle",
    .frames = {
        {0, 0.1f}, {1, 0.1f}, {2, 0.1f}, {3, 0.1f}
    },
    .looping = true
};

player.play("idle");

// Update in game loop
void update(float dt) {
    player.update(dt);
}

// Render
void render() {
    graphics->drawAnimatedSprite(player, transform);
}
```

### Debug Primitives

Perfect for prototyping and physics visualization:

```cpp
// Rectangle
graphics->drawRect(
    Canvas{{100, 100}, {50, 50}},  // x, y, width, height
    Color::red(),
    true  // filled
);

// Circle
graphics->drawCircle(
    Vec2{200, 200},  // center
    25.0f,           // radius
    Color::green(),
    true,            // filled
    32               // segments
);

// Line
graphics->drawLine(
    Vec2{0, 0},      // start
    Vec2{100, 100},  // end
    Color::blue(),
    2.0f             // thickness
);

// Polygon
std::vector<Vec2> triangle = {
    {100, 100}, {150, 50}, {200, 100}
};
graphics->drawPolygon(triangle, Color::yellow(), true);
```

### Text Rendering

Bestow uses **MSDF (Multi-channel Signed Distance Field)** fonts for crisp text at any size:

```cpp
// Load font
AssetHandle font = assets->loadFont("fonts/PressStart2P.ttf");

// Draw text
graphics->drawText(
    "Score: 100",
    Vec2{10, 10},    // position
    font,
    24.0f,           // size
    Color::white()
);

// Draw centered text
graphics->drawTextCentered(
    "GAME OVER",
    Vec2{screenWidth/2, screenHeight/2},
    font,
    48.0f,
    Color::red()
);

// Measure text for layout
Vec2 textSize = graphics->measureText("Hello", font, 24.0f);
```

### Camera System

```cpp
// Set camera
Camera camera{
    .transform = Transform2D{0, 0, 0.0f, 1.0f, 1.0f},
    .zoom = 1.0f,
    .viewportSize = graphics->getWindowSize()
};
graphics->setCamera(camera);

// Screen to world conversion
Vec2 worldPos = graphics->screenToWorld(mousePos);

// World to screen conversion
Vec2 screenPos = graphics->worldToScreen(entityPos);
```

### Entity Rendering (ECS)

Automatically render all entities with visual components:

```cpp
// Render all entities
graphics->renderEntities(entities);

// Render specific layers
graphics->renderEntities(
    entities,
    RenderLayers::Background,
    RenderLayers::Foreground
);

// Enable viewport culling (only render visible entities)
graphics->setViewportCulling(true);
```

**Supported visual components:**
- `Sprite` + `Transform2D`
- `DebugRect` + `Transform2D`
- `DebugCircle` + `Transform2D`
- `DebugLine` + `Transform2D`

---

## 3D Graphics API

### Initialization

```cpp
bool MyGame::init() {
    Graphics3DConfig config{
        .windowWidth = 1280,
        .windowHeight = 720,
        .windowTitle = "My 3D Game",
        .vsync = true,
        .fullscreen = false,
        .enableValidation = true  // Enable for debugging
    };

    if (!graphics3D->initialize(config)) {
        return false;
    }

    // Set asset and shader systems
    graphics3D->setAssetSystem(assets);
    graphics3D->setShaderSystem(shaders);

    return true;
}
```

### Mesh Creation

#### Primitive Meshes

```cpp
// Cube
auto cubeResult = graphics3D->createCubeMesh(1.0f);
MeshHandle cube = *cubeResult;

// Sphere
auto sphereResult = graphics3D->createSphereMesh(
    0.5f,   // radius
    32,     // segments
    16      // rings
);

// Cylinder
auto cylinderResult = graphics3D->createCylinderMesh(
    0.5f,   // radius
    2.0f,   // height
    32      // segments
);

// Capsule
auto capsuleResult = graphics3D->createCapsuleMesh(
    0.5f,   // radius
    2.0f,   // height
    32,     // segments
    8       // rings
);

// Plane
auto planeResult = graphics3D->createPlaneMesh(
    10.0f,  // width
    10.0f,  // height
    10,     // width segments
    10      // height segments
);
```

#### Custom Meshes

```cpp
// Define vertices
std::vector<Vertex3D> vertices = {
    {{-0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{ 0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{ 0.0f, 1.0f,  0.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 1.0f}}
};

// Define indices
std::vector<std::uint32_t> indices = {0, 1, 2};

// Create mesh
MeshDef meshDef{
    .vertices = vertices,
    .indices = indices,
    .bounds = AABB3D{{-0.5f, 0.0f, -0.5f}, {0.5f, 1.0f, 0.5f}},
    .isDynamic = false
};

auto meshResult = graphics3D->createMesh(meshDef);
MeshHandle mesh = *meshResult;
```

#### Loading Meshes from Assets

```cpp
// Load model file (glTF, OBJ, FBX)
AssetHandle modelAsset = assets->loadModel("models/character.gltf");
const ModelData* modelData = assets->getModelData(modelAsset);

// Create GPU mesh from asset data
auto meshResult = graphics3D->createMeshFromData(modelData->meshes[0]);
MeshHandle mesh = *meshResult;
```

### Material Creation

#### PBR Materials

```cpp
// Create PBR material
PBRMaterial pbrMat{
    .baseColorFactor = Vec4{1.0f, 0.8f, 0.6f, 1.0f},
    .baseColorTexture = assets->loadTexture("textures/albedo.png"),
    .metallicFactor = 0.5f,
    .roughnessFactor = 0.3f,
    .normalTexture = assets->loadTexture("textures/normal.png"),
    .normalScale = 1.0f,
    .emissiveFactor = Vec3{0.0f},
    .blendMode = BlendMode::Opaque,
    .cullMode = CullMode::Back
};

auto matResult = graphics3D->createMaterial(pbrMat);
MaterialHandle material = *matResult;
```

#### Unlit Materials

```cpp
UnlitMaterial unlitMat{
    .color = Vec4{1.0f, 1.0f, 1.0f, 1.0f},
    .texture = assets->loadTexture("textures/sprite.png"),
    .blendMode = BlendMode::AlphaBlend,
    .cullMode = CullMode::None
};

auto matResult = graphics3D->createUnlitMaterial(unlitMat);
```

#### Default Materials

```cpp
// Get built-in materials
MaterialHandle defaultPBR = graphics3D->getDefaultPBRMaterial();
MaterialHandle defaultUnlit = graphics3D->getDefaultUnlitMaterial();
MaterialHandle errorMat = graphics3D->getErrorMaterial();  // Checkerboard
```

### Rendering Meshes

#### Immediate Mode

```cpp
// Draw mesh with transform
Transform3D transform{
    .position = Vec3{0.0f, 0.0f, 0.0f},
    .rotation = Quat{1.0f, 0.0f, 0.0f, 0.0f},
    .scale = Vec3{1.0f, 1.0f, 1.0f}
};

graphics3D->drawMesh(mesh, material, transform);

// Draw with matrix
Mat4 worldMatrix = glm::translate(Mat4{1.0f}, Vec3{0, 0, 0});
graphics3D->drawMesh(mesh, material, worldMatrix);
```

#### Batch Rendering

For better performance when rendering many objects:

```cpp
// Queue render items
for (const auto& obj : objects) {
    RenderItem item{
        .mesh = obj.mesh,
        .material = obj.material,
        .worldMatrix = obj.getWorldMatrix(),
        .layer = RenderLayers::Default,
        .castShadow = true,
        .receiveShadow = true
    };
    graphics3D->queueRenderItem(item);
}

// Flush queue (in endFrame or manually)
graphics3D->flushRenderQueue();
```

#### Entity Rendering (ECS)

```cpp
// Render all entities with Mesh3DComponent + Transform3D
graphics3D->renderEntities(entities);

// Frustum culling (only render visible objects)
Frustum frustum = camera.getFrustum();
graphics3D->renderEntities(entities, frustum);

// Render specific layers
graphics3D->renderEntities(
    entities,
    RenderLayers::Background,
    RenderLayers::Foreground
);
```

**Entity components:**
```cpp
Entity player = entities->createEntity();
entities->emplace<Transform3D>(player, Transform3D{
    .position = Vec3{0, 1, 0},
    .rotation = Quat{1, 0, 0, 0},
    .scale = Vec3{1, 1, 1}
});
entities->emplace<Mesh3DComponent>(player, Mesh3DComponent{
    .mesh = playerMesh,
    .material = playerMaterial,
    .layer = RenderLayers::Player,
    .visible = true,
    .castShadow = true,
    .receiveShadow = true
});
```

### Camera 3D

```cpp
// Create 3D camera
Camera3D camera{
    .transform = Transform3D{
        .position = Vec3{0, 2, 5},
        .rotation = Quat{1, 0, 0, 0},
        .scale = Vec3{1, 1, 1}
    },
    .projection = ProjectionType::Perspective,
    .fovY = 60.0f,
    .aspectRatio = 16.0f / 9.0f,
    .nearPlane = 0.1f,
    .farPlane = 1000.0f
};

graphics3D->setCamera(camera);

// Screen to ray for picking
Ray3D ray = graphics3D->screenToWorldRay(mousePos);

// World to screen (returns nullopt if behind camera)
std::optional<Vec2> screenPos = graphics3D->worldToScreen(worldPos);
```

### Lighting

#### Directional Light

```cpp
DirectionalLight sun{
    .direction = Vec3{0.5f, -1.0f, 0.3f},
    .color = Vec3{1.0f, 0.89f, 0.79f},  // Warm sunlight
    .intensity = 1.0f,
    .castShadows = true,
    .shadowMapResolution = 2048
};

graphics3D->setDirectionalLight(sun);
```

#### Point Lights

```cpp
PointLight torch{
    .color = Vec3{1.0f, 0.6f, 0.3f},  // Orange fire
    .intensity = 2.0f,
    .range = 10.0f,
    .castShadows = false
};

std::uint32_t lightId = graphics3D->addPointLight(torch, Vec3{0, 2, 0});

// Update position
graphics3D->setLightPosition(lightId, Vec3{5, 2, 0});

// Remove
graphics3D->removeLight(lightId);
```

#### Spot Lights

```cpp
SpotLight flashlight{
    .direction = Vec3{0.0f, -1.0f, 0.0f},
    .color = Vec3{1.0f, 1.0f, 1.0f},
    .intensity = 3.0f,
    .range = 20.0f,
    .innerConeAngle = 0.3f,  // Radians
    .outerConeAngle = 0.5f,
    .castShadows = true
};

std::uint32_t spotId = graphics3D->addSpotLight(flashlight, Vec3{0, 5, 0});
```

#### Ambient Light

```cpp
graphics3D->setAmbientLight(
    Vec3{0.2f, 0.3f, 0.4f},  // Cool blue ambient
    0.5f                      // intensity
);
```

#### Entity Lights (ECS)

```cpp
// Attach light to entity
Entity lamp = entities->createEntity();
entities->emplace<Transform3D>(lamp, Transform3D{.position = Vec3{0, 3, 0}});
entities->emplace<Light3DComponent>(lamp, Light3DComponent{
    .light = Light3D{
        .type = LightType::Point,
        .color = Vec3{1, 1, 0.8f},
        .intensity = 2.0f,
        .range = 15.0f
    },
    .enabled = true
});

// Update lights from entities each frame
graphics3D->updateEntityLights(entities);
```

### Environment

#### Skybox

```cpp
Skybox sky{
    .cubemapTexture = assets->loadCubemap("textures/sky.hdr"),
    .rotation = 0.0f,
    .exposure = 1.0f
};

graphics3D->setSkybox(sky);
```

#### Fog

```cpp
Fog fog{
    .enabled = true,
    .color = Vec3{0.5f, 0.6f, 0.7f},
    .density = 0.02f,
    .startDistance = 10.0f,
    .endDistance = 100.0f
};

graphics3D->setFog(fog);
```

### Debug Rendering

Perfect for physics visualization and debugging:

```cpp
// Line
graphics3D->debugDrawLine(
    Vec3{0, 0, 0},      // start
    Vec3{1, 0, 0},      // end
    Color::red(),
    0.0f,               // duration (0 = one frame)
    true                // depth test
);

// Box
graphics3D->debugDrawBox(
    Vec3{0, 0, 0},      // center
    Vec3{0.5f, 0.5f, 0.5f},  // half extents
    Quat{1, 0, 0, 0},   // rotation
    Color::green(),
    5.0f                // duration (5 seconds)
);

// Sphere
graphics3D->debugDrawSphere(
    Vec3{0, 1, 0},      // center
    0.5f,               // radius
    Color::blue()
);

// AABB
graphics3D->debugDrawAABB(
    AABB3D{Vec3{-1, 0, -1}, Vec3{1, 2, 1}},
    Color::yellow()
);

// Axes (X=red, Y=green, Z=blue)
graphics3D->debugDrawAxes(
    transform,
    1.0f                // size
);

// Clear all debug shapes
graphics3D->debugClear();
```

---

## Shader System

Bestow includes a **dynamic shader system** with **hot reload** and **Lua material support**.

### Loading Shaders

#### From Files

```cpp
// Load vertex + fragment shader
auto shaderResult = shaders->loadShader(
    "shaders/pbr.vert",
    "shaders/pbr.frag",
    true  // enable hot reload
);

if (shaderResult) {
    ShaderProgramHandle shader = *shaderResult;
}
```

#### From Source

```cpp
std::string vertSource = R"(
    #version 410 core
    layout(location = 0) in vec3 aPos;
    void main() {
        gl_Position = vec4(aPos, 1.0);
    }
)";

std::string fragSource = R"(
    #version 410 core
    out vec4 FragColor;
    void main() {
        FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    }
)";

auto shaderResult = shaders->createShaderFromSource(
    vertSource,
    fragSource,
    "RedShader"
);
```

### Uniform Management

```cpp
// Bind shader
shaders->bindShader(shader);

// Set uniforms by name
shaders->setUniform("uColor", UniformValue{Vec4{1, 0, 0, 1}});
shaders->setUniform("uTime", UniformValue{currentTime});
shaders->setUniform("uModelMatrix", UniformValue{worldMatrix});

// Cache location for performance
std::int32_t colorLoc = shaders->getUniformLocation(shader, "uColor");
shaders->setUniform(colorLoc, UniformValue{Vec4{1, 0, 0, 1}});
```

### Hot Reload

Shaders automatically reload when files change:

```cpp
// Enable hot reload globally
shaders->setHotReloadEnabled(true);

// Register callback
shaders->setShaderReloadCallback([](ShaderProgramHandle shader, bool success, const std::string& error) {
    if (success) {
        std::cout << "Shader reloaded successfully\n";
    } else {
        std::cerr << "Shader reload failed: " << error << "\n";
    }
});

// Update each frame to check for changes
void update() {
    shaders->update();
}
```

---

## Material System

### Creating Materials

#### From Shader Program

```cpp
// Create bare material
auto matResult = shaders->createMaterial(shaderProgram, "MyMaterial");
MaterialHandle material = *matResult;

// Set uniforms
shaders->setMaterialUniform(material, "uColor", UniformValue{Vec4{1, 0, 0, 1}});
shaders->setMaterialUniform(material, "uMetallic", UniformValue{0.5f});

// Set textures
shaders->setMaterialTexture(material, "uAlbedo", albedoTexture);
shaders->setMaterialTexture(material, "uNormal", normalTexture);

// Set blend/cull modes
shaders->setMaterialBlendMode(material, BlendMode::AlphaBlend);
shaders->setMaterialCullMode(material, CullMode::Back);
```

#### From Material Definition

```cpp
ShaderMaterialDef matDef{
    .name = "GlowMaterial",
    .shader = shaderProgram,
    .uniforms = {
        {"uGlowColor", UniformValue{Vec3{0, 1, 1}}},
        {"uGlowIntensity", UniformValue{2.0f}}
    },
    .textures = {
        TextureBinding{0, "uAlbedo", albedoTexture},
        TextureBinding{1, "uEmissive", emissiveTexture}
    },
    .blendMode = BlendMode::Additive,
    .cullMode = CullMode::Back
};

auto matResult = shaders->createMaterial(matDef);
```

### Lua Materials

**Lua materials are the primary way to define materials in Bestow.** They support hot reload and are easy to iterate on.

#### Lua Material Structure

Create `assets/materials/my_material.lua`:

```lua
return {
    shader = {
        vertex = "shaders/util/basic.vert",
        fragment = "shaders/effects/hologram.frag"
    },

    uniforms = {
        uBaseColor = {1.0, 1.0, 1.0, 0.5},
        uHologramColor = {0.0, 1.0, 1.0},
        uScanlineSpeed = 2.0,
        uGlitchIntensity = 0.1,
        uTime = 0.0
    },

    textures = {
        uAlbedo = "textures/hologram_pattern.png",
        uNoise = "textures/noise.png"
    },

    blendMode = "alphaBlend",  -- "opaque", "alphaTest", "alphaBlend", "additive"
    cullMode = "none",         -- "none", "front", "back"
    depthWrite = false,
    depthTest = true,
    hotReload = true
}
```

#### Loading Lua Materials

```cpp
// Load material
auto matResult = shaders->loadMaterial("materials/hologram.lua");
if (matResult) {
    MaterialHandle material = *matResult;
}

// Use with 3D graphics
graphics3D->drawMeshWithLuaMaterial(
    mesh,
    "materials/hologram.lua",
    worldMatrix
);

// With color override
graphics3D->drawMeshWithLuaMaterial(
    mesh,
    "materials/toon.lua",
    worldMatrix,
    Vec4{1.0f, 0.5f, 0.5f, 1.0f}  // Tint red
);
```

#### Material Hot Reload

Materials automatically reload when their Lua files or shader files change:

```cpp
// Register callback
shaders->setMaterialReloadCallback([](MaterialHandle material, bool success, const std::string& error) {
    if (success) {
        std::cout << "Material reloaded\n";
    }
});

// Update each frame
void update() {
    shaders->update();
    // OR
    graphics3D->updateShaders();  // Convenience wrapper
}
```

### Example Materials

**Toon/Cel Shading:**
```lua
return {
    shader = {
        vertex = "shaders/cel/toon.vert",
        fragment = "shaders/cel/toon.frag"
    },
    uniforms = {
        uBaseColor = {1.0, 1.0, 1.0, 1.0},
        uBands = 3,              -- Discrete light bands
        uRimPower = 3.0,
        uRimColor = {1.0, 1.0, 1.0}
    },
    blendMode = "opaque",
    cullMode = "back"
}
```

**Hologram Effect:**
```lua
return {
    shader = {
        vertex = "shaders/util/basic.vert",
        fragment = "shaders/effects/hologram.frag"
    },
    uniforms = {
        uHologramColor = {0.0, 1.0, 1.0},
        uScanlineSpeed = 2.0,
        uFlickerSpeed = 5.0,
        uGlitchIntensity = 0.1
    },
    blendMode = "alphaBlend",
    cullMode = "none",
    depthWrite = false
}
```

**Lava/Animated:**
```lua
return {
    shader = {
        vertex = "shaders/util/basic.vert",
        fragment = "shaders/materials/lava.frag"
    },
    uniforms = {
        uLavaColor1 = {1.0, 0.3, 0.0},
        uLavaColor2 = {1.0, 0.8, 0.0},
        uFlowSpeed = 1.0,
        uDistortionAmount = 0.1
    },
    textures = {
        uNoiseTexture = "textures/noise.png"
    },
    blendMode = "opaque"
}
```

---

## Best Practices

### Draw Order and Layers

**Use RenderLayer for proper draw order:**

```cpp
namespace RenderLayers {
    inline constexpr RenderLayer Background = -100;
    inline constexpr RenderLayer BackgroundDecor = -50;
    inline constexpr RenderLayer Platforms = 0;
    inline constexpr RenderLayer Items = 10;
    inline constexpr RenderLayer Enemies = 20;
    inline constexpr RenderLayer Player = 30;
    inline constexpr RenderLayer Effects = 40;
    inline constexpr RenderLayer Foreground = 50;
    inline constexpr RenderLayer UI = 100;
    inline constexpr RenderLayer Debug = 1000;
}
```

**Lower numbers render first (background), higher numbers render last (foreground).**

### Texture Management

**Always use AssetSystem for texture loading:**

```cpp
// CORRECT - AssetSystem handles caching and lifecycle
AssetHandle texture = assets->loadTexture("textures/player.png");

// WRONG - Never load textures directly
// std::ifstream file("player.png");  // DON'T DO THIS
```

**Benefits:**
- Automatic caching (same texture loaded once)
- Hot reload support
- Proper GPU resource management
- Async loading

### Performance Optimization

#### Sprite Batching

```cpp
// BAD - Many draw calls
for (const auto& enemy : enemies) {
    graphics->draw(enemy.sprite);  // 100 draw calls
}

// GOOD - Single draw call
std::vector<Sprite> sprites;
for (const auto& enemy : enemies) {
    sprites.push_back(enemy.sprite);
}
graphics->drawBatch(sprites);  // 1 draw call
```

#### Mesh Instancing

For rendering many identical meshes:

```cpp
// Create instance buffer
auto bufferResult = graphics3D->createInstanceBuffer(1000);
InstanceBufferHandle instanceBuffer = *bufferResult;

// Fill instance data
std::vector<InstanceData> instances;
for (const auto& tree : trees) {
    instances.push_back(InstanceData{
        .worldMatrix = tree.getWorldMatrix(),
        .customData = Vec4{tree.color, tree.id}
    });
}

graphics3D->updateInstanceBuffer(instanceBuffer, instances);

// Draw all instances in one call
InstancedRenderItem item{
    .mesh = treeMesh,
    .material = treeMaterial,
    .instances = instanceBuffer,
    .instanceCount = static_cast<std::uint32_t>(instances.size())
};

graphics3D->drawInstanced(item);  // 1 draw call for 1000 trees
```

#### Frustum Culling

Enable culling to only render visible objects:

```cpp
// 2D
graphics->setViewportCulling(true);

// 3D
graphics3D->setFrustumCulling(true);

// Manual culling
Frustum frustum = camera.getFrustum();
graphics3D->renderEntities(entities, frustum);
```

#### LOD (Level of Detail)

For distant objects:

```cpp
// Set LOD distances (meters)
std::vector<float> distances = {10.0f, 50.0f, 100.0f};
graphics3D->setLODDistances(distances);

// Register LOD meshes
graphics3D->registerLODMeshes(
    highDetailMesh,
    {mediumDetailMesh, lowDetailMesh, impostorMesh}
);

// Adjust LOD bias (< 1.0 = lower detail, > 1.0 = higher detail)
graphics3D->setLODBias(0.8f);
```

### Shader Best Practices

1. **Use Lua materials for game content** - Easy to iterate and hot reload
2. **Keep uniforms minimal** - Cache locations for frequently updated uniforms
3. **Batch by material** - Minimize material switches
4. **Use uniform buffers** - For per-frame data (camera, lights)
5. **Enable hot reload in development** - Disable in production builds

### Memory Management

```cpp
// ALWAYS destroy resources when done
graphics3D->destroyMesh(mesh);
graphics3D->destroyMaterial(material);
shaders->destroyShader(shader);
shaders->destroyMaterial(material);
graphics3D->destroyInstanceBuffer(instanceBuffer);
```

### Debug Rendering Workflow

```cpp
#ifdef BESTOW_DEBUG
    // Enable debug rendering
    graphics3D->setDebugRenderingEnabled(true);

    // Draw physics shapes
    for (const auto& body : physicsBodies) {
        graphics3D->debugDrawBox(body.position, body.halfExtents, body.rotation, Color::green());
    }

    // Draw AI paths
    for (size_t i = 0; i < path.size() - 1; ++i) {
        graphics3D->debugDrawLine(path[i], path[i+1], Color::yellow());
    }
#endif
```

---

## Complete Examples

### Example 1: 2D Platformer Renderer

```cpp
class PlatformerRenderer {
public:
    PlatformerRenderer(IGraphicsSystem* graphics, IAssetSystem* assets, IEntitySystem* entities)
        : graphics_(graphics), assets_(assets), entities_(entities)
    {
        // Load assets
        playerTexture_ = assets->loadTexture("textures/player.png");
        tileTexture_ = assets->loadTexture("textures/tiles.png");
        coinTexture_ = assets->loadTexture("textures/coin.png");
        font_ = assets->loadFont("fonts/PressStart2P.ttf");

        // Setup camera
        camera_.viewportSize = graphics->getWindowSize();
        camera_.zoom = 1.0f;

        // Enable culling
        graphics->setViewportCulling(true);
    }

    void render(float dt) {
        graphics_->beginFrame();
        graphics_->setClearColor(Color::fromFloat(0.53f, 0.81f, 0.92f));  // Sky blue

        // Update camera to follow player
        updateCamera();
        graphics_->setCamera(camera_);

        // Render layers in order
        renderBackground();
        renderEntities(RenderLayers::Platforms, RenderLayers::Items);
        renderEntities(RenderLayers::Enemies, RenderLayers::Player);
        renderEntities(RenderLayers::Effects, RenderLayers::Foreground);
        renderUI();

        graphics_->endFrame();
    }

private:
    void updateCamera() {
        // Get player position
        auto playerView = entities_->view<Transform2D, PlayerTag>();
        for (auto entity : playerView) {
            const auto& transform = playerView.get<Transform2D>(entity);
            camera_.transform.x = transform.x;
            camera_.transform.y = transform.y;
        }
    }

    void renderBackground() {
        // Draw parallax background layers
        Canvas bgRect{{0, 0}, graphics_->getWindowSize()};
        graphics_->drawRect(bgRect, Color::fromFloat(0.3f, 0.5f, 0.7f), true);
    }

    void renderEntities(RenderLayer minLayer, RenderLayer maxLayer) {
        graphics_->renderEntities(*entities_, minLayer, maxLayer);
    }

    void renderUI() {
        // Draw score
        graphics_->drawText(
            "Score: " + std::to_string(score_),
            Vec2{10, 10},
            font_,
            16.0f,
            Color::white()
        );

        // Draw health bar
        for (int i = 0; i < health_; ++i) {
            Canvas heart{{10 + i * 20, 40}, {16, 16}};
            graphics_->drawRect(heart, Color::red(), true);
        }
    }

    IGraphicsSystem* graphics_;
    IAssetSystem* assets_;
    IEntitySystem* entities_;

    Camera camera_;
    AssetHandle playerTexture_;
    AssetHandle tileTexture_;
    AssetHandle coinTexture_;
    AssetHandle font_;

    int score_ = 0;
    int health_ = 3;
};
```

### Example 2: 3D Scene Renderer

```cpp
class SceneRenderer {
public:
    SceneRenderer(IGraphics3DSystem* graphics, IShaderSystem* shaders, IAssetSystem* assets)
        : graphics_(graphics), shaders_(shaders), assets_(assets)
    {
        // Initialize graphics
        Graphics3DConfig config{
            .windowWidth = 1920,
            .windowHeight = 1080,
            .windowTitle = "3D Scene",
            .vsync = true,
            .enableValidation = true
        };
        graphics->initialize(config);
        graphics->setAssetSystem(assets);
        graphics->setShaderSystem(shaders);

        // Load runtime config
        graphics->loadRuntimeConfig("config/graphics3d.lua");

        // Setup camera
        camera_.transform.position = Vec3{0, 5, 10};
        camera_.fovY = 60.0f;
        camera_.aspectRatio = 16.0f / 9.0f;
        camera_.nearPlane = 0.1f;
        camera_.farPlane = 1000.0f;
        graphics->setCamera(camera_);

        // Setup lighting
        setupLighting();

        // Load scene assets
        loadScene();
    }

    void render(float dt) {
        time_ += dt;

        graphics_->beginFrame();

        // Update camera
        updateCamera(dt);
        graphics_->setCamera(camera_);

        // Render scene
        renderTerrain();
        renderObjects();
        renderCharacters();
        renderEffects();
        renderDebugInfo();

        graphics_->endFrame();
    }

private:
    void setupLighting() {
        // Directional light (sun)
        DirectionalLight sun{
            .direction = Vec3{0.5f, -1.0f, 0.3f},
            .color = Vec3{1.0f, 0.89f, 0.79f},
            .intensity = 1.0f,
            .castShadows = true,
            .shadowMapResolution = 2048
        };
        graphics_->setDirectionalLight(sun);

        // Ambient light
        graphics_->setAmbientLight(Vec3{0.2f, 0.3f, 0.4f}, 0.3f);

        // Fog
        Fog fog{
            .enabled = true,
            .color = Vec3{0.5f, 0.6f, 0.7f},
            .density = 0.01f,
            .startDistance = 50.0f,
            .endDistance = 200.0f
        };
        graphics_->setFog(fog);
    }

    void loadScene() {
        // Create terrain
        terrainMesh_ = *graphics_->createPlaneMesh(100.0f, 100.0f, 50, 50);

        PBRMaterial terrainMat{
            .baseColorFactor = Vec4{0.5f, 0.7f, 0.3f, 1.0f},
            .roughnessFactor = 0.8f,
            .metallicFactor = 0.0f
        };
        terrainMaterial_ = *graphics_->createMaterial(terrainMat);

        // Load tree model
        AssetHandle treeAsset = assets_->loadModel("models/tree.gltf");
        const ModelData* treeData = assets_->getModelData(treeAsset);
        treeMesh_ = *graphics_->createMeshFromData(treeData->meshes[0]);

        // Load character with Lua material
        AssetHandle characterAsset = assets_->loadModel("models/character.gltf");
        const ModelData* charData = assets_->getModelData(characterAsset);
        characterMesh_ = *graphics_->createMeshFromData(charData->meshes[0]);
    }

    void renderTerrain() {
        Transform3D transform{
            .position = Vec3{0, 0, 0},
            .rotation = Quat{1, 0, 0, 0},
            .scale = Vec3{1, 1, 1}
        };

        graphics_->drawMesh(terrainMesh_, terrainMaterial_, transform);
    }

    void renderObjects() {
        // Render trees with instancing
        std::vector<InstanceData> treeInstances;
        for (const auto& pos : treePositions_) {
            Mat4 worldMatrix = glm::translate(Mat4{1.0f}, pos);
            treeInstances.push_back(InstanceData{
                .worldMatrix = worldMatrix,
                .customData = Vec4{1.0f}
            });
        }

        if (!treeInstanceBuffer_) {
            treeInstanceBuffer_ = *graphics_->createInstanceBuffer(treeInstances.size());
        }
        graphics_->updateInstanceBuffer(treeInstanceBuffer_, treeInstances);

        InstancedRenderItem trees{
            .mesh = treeMesh_,
            .material = graphics_->getDefaultPBRMaterial(),
            .instances = treeInstanceBuffer_,
            .instanceCount = static_cast<std::uint32_t>(treeInstances.size())
        };
        graphics_->drawInstanced(trees);
    }

    void renderCharacters() {
        // Render character with toon shader
        Mat4 worldMatrix = glm::translate(Mat4{1.0f}, Vec3{0, 1, 0});

        graphics_->drawMeshWithLuaMaterial(
            characterMesh_,
            "materials/toon.lua",
            worldMatrix,
            Vec4{1.0f, 0.8f, 0.6f, 1.0f}  // Skin tone
        );
    }

    void renderEffects() {
        // Render hologram effect
        Mat4 holoMatrix = glm::translate(Mat4{1.0f}, Vec3{5, 2, 0});
        holoMatrix = glm::rotate(holoMatrix, time_, Vec3{0, 1, 0});

        auto cubeResult = graphics_->createCubeMesh(1.0f);
        graphics_->drawMeshWithLuaMaterial(
            *cubeResult,
            "materials/hologram.lua",
            holoMatrix
        );
    }

    void renderDebugInfo() {
        if (!debugEnabled_) return;

        // Draw camera frustum
        Frustum frustum = getFrustum(camera_);
        graphics_->debugDrawFrustum(frustum, Color::yellow(), 0.0f);

        // Draw light directions
        graphics_->debugDrawLine(
            Vec3{0, 0, 0},
            Vec3{0, 0, 0} + Vec3{0.5f, -1.0f, 0.3f} * 5.0f,
            Color::fromFloat(1.0f, 0.89f, 0.79f)
        );

        // Draw object bounds
        for (const auto& pos : treePositions_) {
            AABB3D bounds = graphics_->getMeshBounds(treeMesh_);
            bounds.min += pos;
            bounds.max += pos;
            graphics_->debugDrawAABB(bounds, Color::green());
        }
    }

    void updateCamera(float dt) {
        // Simple orbit camera
        float radius = 10.0f;
        float angle = time_ * 0.5f;
        camera_.transform.position = Vec3{
            std::cos(angle) * radius,
            5.0f,
            std::sin(angle) * radius
        };

        // Look at center
        // camera_.transform.rotation = lookAt(camera_.transform.position, Vec3{0, 0, 0});
    }

    IGraphics3DSystem* graphics_;
    IShaderSystem* shaders_;
    IAssetSystem* assets_;

    Camera3D camera_;
    float time_ = 0.0f;
    bool debugEnabled_ = true;

    MeshHandle terrainMesh_;
    MaterialHandle terrainMaterial_;
    MeshHandle treeMesh_;
    MeshHandle characterMesh_;
    InstanceBufferHandle treeInstanceBuffer_ = 0;

    std::vector<Vec3> treePositions_ = {
        {-10, 0, -10}, {10, 0, -10}, {-10, 0, 10}, {10, 0, 10},
        {0, 0, -15}, {0, 0, 15}, {-15, 0, 0}, {15, 0, 0}
    };
};
```

### Example 3: Material Hot Reload System

```cpp
class MaterialManager {
public:
    MaterialManager(IShaderSystem* shaders, IAssetSystem* assets)
        : shaders_(shaders), assets_(assets)
    {
        shaders->setHotReloadEnabled(true);

        // Register reload callbacks
        shaders->setMaterialReloadCallback([this](MaterialHandle mat, bool success, const std::string& error) {
            if (success) {
                std::cout << "[MaterialManager] Material reloaded: " << shaders_->getMaterialName(mat) << "\n";
                onMaterialReloaded(mat);
            } else {
                std::cerr << "[MaterialManager] Material reload failed: " << error << "\n";
            }
        });
    }

    MaterialHandle loadMaterial(const std::string& path) {
        // Check cache
        auto it = materialCache_.find(path);
        if (it != materialCache_.end()) {
            return it->second;
        }

        // Load new material
        auto result = shaders_->loadMaterial(path);
        if (result) {
            MaterialHandle handle = *result;
            materialCache_[path] = handle;
            materialPaths_[handle] = path;
            return handle;
        }

        return 0;  // Invalid
    }

    void update() {
        // Check for shader changes
        shaders_->update();
    }

    void reloadAll() {
        for (const auto& [path, handle] : materialCache_) {
            shaders_->reloadMaterial(handle);
        }
    }

private:
    void onMaterialReloaded(MaterialHandle handle) {
        // Notify systems that use this material
        auto it = materialPaths_.find(handle);
        if (it != materialPaths_.end()) {
            std::cout << "  Path: " << it->second << "\n";
        }
    }

    IShaderSystem* shaders_;
    IAssetSystem* assets_;

    std::unordered_map<std::string, MaterialHandle> materialCache_;
    std::unordered_map<MaterialHandle, std::string> materialPaths_;
};
```

---

## Summary

The Bestow Graphics System provides:

1. **Dual rendering backends** - Vulkan (primary) and OpenGL (fallback)
2. **Complete 2D system** - Sprites, text, primitives, camera
3. **Complete 3D system** - Meshes, materials, lighting, shadows, post-processing
4. **Dynamic shader system** - Hot reload, Lua materials, uniform management
5. **Entity-based rendering** - Automatic rendering from ECS components
6. **Performance features** - Batching, instancing, culling, LOD
7. **Debug tools** - Comprehensive debug drawing API

**Key Principles:**
- **Vulkan first** - Use Vulkan for production
- **Lua materials** - Define materials in Lua for easy iteration
- **Hot reload everything** - Shaders, materials, and configs reload on save
- **Use AssetSystem** - Never load files directly
- **Batch rendering** - Minimize draw calls
- **Layer organization** - Use RenderLayers for proper draw order

For more information, see:
- `/bestow-contract/src/bestow.graphics.cppm` - 2D interface
- `/bestow-contract/src/bestow.graphics3d.cppm` - 3D interface
- `/bestow-contract/src/bestow.shader.cppm` - Shader interface
- `/bestow-shader/materials/` - Example Lua materials
- `/bestow-shader/shaders/` - Example GLSL shaders
