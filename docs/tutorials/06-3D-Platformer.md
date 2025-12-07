# Tutorial 6: 3D Platformer

This tutorial covers Bestow's 3D systems: Graphics3D and Physics3D. You'll learn how to create 3D scenes, work with meshes and materials, set up physics bodies, implement character controllers, handle 3D input, and create a first-person platformer game.

## 3D Systems Overview

Bestow provides dedicated 3D systems built on modern graphics and physics libraries:

- **Graphics3D** - OpenGL-based 3D rendering with PBR materials, lighting, and debug visualization
- **Physics3D** - Jolt Physics for 3D rigid body simulation and character controllers
- **Input** - Same input system as 2D, but used for first-person camera control

## Prerequisites

Before starting, make sure you understand:

- Basic Bestow concepts (Tutorial 1)
- Input handling (Tutorial 4)
- Physics concepts (Tutorial 3 - 2D physics)

## Scene Setup

### Step 1: Creating a 3D Project

Create a new directory for your 3D platformer:

```bash
cd examples
mkdir my-3d-platformer
cd my-3d-platformer
```

Create the directory structure:

```
my-3d-platformer/
├── CMakeLists.txt
├── src/
│   └── main.cpp
└── data/
    └── textures/
```

### Step 2: CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.28)
project(my-3d-platformer CXX)

add_executable(my-3d-platformer
    src/main.cpp
)

target_link_libraries(my-3d-platformer PRIVATE
    bestow-contract
    bestow-graphics3d
    bestow-physics3d
    bestow-input
)

target_compile_features(my-3d-platformer PRIVATE cxx_std_23)

# Copy data directory to build output
add_custom_command(TARGET my-3d-platformer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/data
    $<TARGET_FILE_DIR:my-3d-platformer>/data
)
```

## Graphics3D Fundamentals

### Creating Meshes

Bestow provides built-in primitive mesh generators:

```cpp
import bestow.graphics3d;
import bestow.graphics3d.impl;

auto graphics = createGraphics3DSystem();

// Create primitive meshes
auto cubeResult = graphics->createCubeMesh(1.0f);
if (cubeResult) {
    MeshHandle cubeMesh = *cubeResult;
}

auto sphereResult = graphics->createSphereMesh(
    0.5f,    // radius
    32,      // segments
    16       // rings
);

auto capsuleResult = graphics->createCapsuleMesh(
    0.3f,    // radius
    0.8f,    // height
    32,      // segments
    8        // rings
);

auto cylinderResult = graphics->createCylinderMesh(
    0.5f,    // radius
    1.0f,    // height
    32       // segments
);

auto planeResult = graphics->createPlaneMesh(
    10.0f,   // width
    10.0f,   // height
    10,      // width segments
    10       // height segments
);
```

### Creating Materials

Materials define how surfaces look. Bestow supports PBR (Physically Based Rendering):

```cpp
// PBR Material (realistic lighting)
PBRMaterial mat;
mat.baseColorFactor = Vec4{0.2f, 0.4f, 0.9f, 1.0f};  // Blue color
mat.roughnessFactor = 0.3f;  // 0.0 = mirror, 1.0 = rough
mat.metallicFactor = 0.5f;   // 0.0 = dielectric, 1.0 = metal

auto matResult = graphics->createMaterial(mat);
if (matResult) {
    MaterialHandle material = *matResult;
}

// Unlit Material (no lighting, just color/texture)
UnlitMaterial unlitMat;
unlitMat.color = Vec4{1.0f, 1.0f, 1.0f, 1.0f};
// unlitMat.texture = textureHandle;  // Optional texture

auto unlitResult = graphics->createUnlitMaterial(unlitMat);
```

### Setting Up Lighting

Lights make your 3D scene visible:

```cpp
// Directional Light (sunlight)
DirectionalLight sunLight;
sunLight.direction = glm::normalize(Vec3{-0.5f, -1.0f, -0.3f});
sunLight.color = Vec3{1.0f, 0.95f, 0.9f};  // Warm white
sunLight.intensity = 1.2f;
sunLight.castShadows = true;
graphics->setDirectionalLight(sunLight);

// Ambient Light (global illumination)
graphics->setAmbientLight(
    Vec3{0.4f, 0.45f, 0.5f},  // Cool blue tint
    0.3f                       // Intensity
);

// Point Light (local light source)
PointLight torchLight;
torchLight.color = Vec3{1.0f, 0.8f, 0.5f};  // Orange glow
torchLight.intensity = 2.0f;
torchLight.range = 10.0f;
std::uint32_t lightId = graphics->addPointLight(torchLight, Vec3{5.0f, 2.0f, 0.0f});

// Spot Light (flashlight, spotlight)
SpotLight flashlight;
flashlight.direction = Vec3{0.0f, -1.0f, 0.0f};
flashlight.color = Vec3{1.0f, 1.0f, 1.0f};
flashlight.intensity = 3.0f;
flashlight.range = 20.0f;
flashlight.innerConeAngle = 0.4f;  // Radians (22.9 degrees)
flashlight.outerConeAngle = 0.5f;  // Radians (28.6 degrees)
std::uint32_t spotId = graphics->addSpotLight(flashlight, Vec3{0.0f, 5.0f, 0.0f});
```

### Camera Setup

The camera determines what the player sees:

```cpp
Camera3D camera;

// Camera position and orientation
camera.transform.position = Vec3{0.0f, 2.0f, 5.0f};  // 5 units back, 2 units up

// Camera rotation using quaternions
// For first-person: combine yaw (horizontal) and pitch (vertical) rotation
float yaw = 0.0f;    // Horizontal angle
float pitch = 0.0f;  // Vertical angle (looking up/down)

glm::quat yawQuat = glm::angleAxis(yaw, glm::vec3(0.0f, 1.0f, 0.0f));
glm::quat pitchQuat = glm::angleAxis(pitch, glm::vec3(1.0f, 0.0f, 0.0f));
camera.transform.rotation = yawQuat * pitchQuat;

// Projection settings
camera.projection = ProjectionType::Perspective;
camera.fovY = 70.0f;  // Field of view (degrees)
camera.aspectRatio = 1280.0f / 720.0f;
camera.nearPlane = 0.1f;
camera.farPlane = 500.0f;

graphics->setCamera(camera);
```

### Drawing Meshes

```cpp
// Draw a mesh with a transform
Transform3D transform;
transform.position = Vec3{0.0f, 1.0f, 0.0f};
transform.rotation = Quat{1.0f, 0.0f, 0.0f, 0.0f};  // Identity (no rotation)
transform.scale = Vec3{1.0f, 1.0f, 1.0f};

graphics->drawMesh(meshHandle, materialHandle, transform);

// Draw with scaling
transform.scale = Vec3{2.0f, 1.0f, 2.0f};  // Wider in X and Z
graphics->drawMesh(meshHandle, materialHandle, transform);
```

### Frame Rendering

```cpp
// In your game loop
graphics->beginFrame();

// Set camera
graphics->setCamera(camera);

// Draw all your meshes
graphics->drawMesh(platformMesh, platformMat, platformTransform);
graphics->drawMesh(playerMesh, playerMat, playerTransform);

// Debug visualization
graphics->debugDrawAxes(Transform3D{}, 2.0f);

graphics->endFrame();
```

## Physics3D Fundamentals

### Creating Physics Bodies

Physics bodies make objects interact with physics simulation:

```cpp
import bestow.physics3d;
import bestow.physics3d.impl;

auto physics = createPhysics3DSystem();

// Set world gravity
physics->setGravity(Vec3{0.0f, -20.0f, 0.0f});  // 20 m/s^2 downward

// Dynamic Body (player, boxes - affected by gravity)
PhysicsBodyDef3D playerDef;
playerDef.type = BodyType3D::Dynamic;
playerDef.transform.position = Vec3{0.0f, 2.0f, 0.0f};
playerDef.shapeType = ShapeType3D::Capsule;
playerDef.shapeRadius = 0.3f;
playerDef.shapeHalfHeight = 0.8f;
playerDef.density = 1000.0f;  // kg/m^3
playerDef.friction = 0.5f;
playerDef.restitution = 0.0f;  // No bounce
playerDef.layer = CollisionLayers3D::Character;
playerDef.mask = CollisionLayers3D::World | CollisionLayers3D::Platform;

physics->createBody(playerEntity, playerDef);

// Static Body (ground, platforms - never moves)
PhysicsBodyDef3D groundDef;
groundDef.type = BodyType3D::Static;
groundDef.transform.position = Vec3{0.0f, -2.0f, 0.0f};
groundDef.shapeType = ShapeType3D::Box;
groundDef.shapeHalfExtents = Vec3{25.0f, 0.5f, 25.0f};  // Large floor
groundDef.friction = 0.8f;
groundDef.layer = CollisionLayers3D::World;

physics->createBody(groundEntity, groundDef);

// Kinematic Body (moving platform - moves programmatically)
PhysicsBodyDef3D platformDef;
platformDef.type = BodyType3D::Kinematic;
platformDef.transform.position = Vec3{5.0f, 1.0f, 0.0f};
platformDef.shapeType = ShapeType3D::Box;
platformDef.shapeHalfExtents = Vec3{2.0f, 0.25f, 2.0f};
platformDef.layer = CollisionLayers3D::Platform;

physics->createBody(platformEntity, platformDef);
```

### Shape Types

Physics3D supports multiple collision shapes:

```cpp
// Box Shape
PhysicsBodyDef3D boxDef;
boxDef.shapeType = ShapeType3D::Box;
boxDef.shapeHalfExtents = Vec3{0.5f, 0.5f, 0.5f};  // 1m cube

// Sphere Shape
PhysicsBodyDef3D sphereDef;
sphereDef.shapeType = ShapeType3D::Sphere;
sphereDef.shapeRadius = 0.5f;  // 0.5m radius

// Capsule Shape (best for characters)
PhysicsBodyDef3D capsuleDef;
capsuleDef.shapeType = ShapeType3D::Capsule;
capsuleDef.shapeRadius = 0.3f;
capsuleDef.shapeHalfHeight = 0.8f;  // Total height = 2 * (halfHeight + radius)

// Cylinder Shape
PhysicsBodyDef3D cylinderDef;
cylinderDef.shapeType = ShapeType3D::Cylinder;
cylinderDef.shapeRadius = 0.5f;
cylinderDef.shapeHalfHeight = 1.0f;
```

### Character Controller

For player movement, use a character controller instead of a regular dynamic body:

```cpp
// Create character controller
CharacterControllerDef charDef;
charDef.radius = 0.3f;           // Capsule radius
charDef.height = 1.6f;           // Total height (not including caps)
charDef.stepHeight = 0.35f;      // Max step height to auto-climb
charDef.maxSlopeAngle = 45.0f;   // Max walkable slope (degrees)
charDef.mass = 80.0f;            // Player weight (kg)
charDef.layer = CollisionLayers3D::Character;
charDef.mask = CollisionLayers3D::World | CollisionLayers3D::Platform;

auto result = physics->createCharacter(playerEntity, charDef);
if (!result) {
    std::println("Failed to create character controller");
}

// Set initial position
physics->setCharacterPosition(playerEntity, Vec3{0.0f, 2.0f, 0.0f});
```

### Moving the Character

```cpp
// In your update loop
void updatePlayer(DeltaTime dt) {
    Vec3 velocity{0.0f};

    // Apply movement input
    if (movingForward) velocity += forward * moveSpeed;
    if (movingBackward) velocity -= forward * moveSpeed;
    if (movingLeft) velocity -= right * moveSpeed;
    if (movingRight) velocity += right * moveSpeed;

    // Check if grounded
    auto groundInfo = physics->getCharacterGroundInfo(playerEntity);
    bool grounded = false;
    if (groundInfo) {
        grounded = (groundInfo->state == CharacterGroundState::OnGround);
    }

    // Get current velocity
    auto velResult = physics->getCharacterVelocity(playerEntity);
    if (velResult) {
        Vec3 currentVel = *velResult;
        velocity.y = currentVel.y;  // Preserve vertical velocity
    }

    // Jump
    if (jumpPressed && grounded) {
        velocity.y = jumpForce;
    }

    // Apply gravity
    if (!grounded) {
        velocity.y += gravity * dt;
    } else if (velocity.y < 0.0f) {
        velocity.y = 0.0f;  // Stop falling when grounded
    }

    // Move character (handles collisions and sliding)
    physics->moveCharacter(playerEntity, velocity, dt);

    // Get updated position
    auto posResult = physics->getCharacterPosition(playerEntity);
    if (posResult) {
        playerPosition = *posResult;
    }
}
```

### Raycasting

Detect what the player is looking at:

```cpp
// Cast a ray from camera
Vec3 rayOrigin = camera.transform.position;
Vec3 rayDirection = camera.transform.rotation * Vec3{0.0f, 0.0f, -1.0f};
float maxDistance = 100.0f;

auto hit = physics->raycast(rayOrigin, rayDirection, maxDistance);
if (hit) {
    Entity hitEntity = hit->entity;
    Vec3 hitPoint = hit->point;
    Vec3 hitNormal = hit->normal;
    float distance = hit->distance;

    std::println("Hit entity at distance: {}", distance);

    // Draw debug visualization
    graphics->debugDrawLine(rayOrigin, hitPoint, Color::green());
    graphics->debugDrawSphere(hitPoint, 0.1f, Color::red());
}
```

## Input for 3D First-Person Control

### Dvorak Keyboard Layout

The 3D platformer example uses Dvorak keyboard layout for movement:

- **,** (comma) - Move Forward (QWERTY W)
- **O** - Move Backward (QWERTY S)
- **A** - Move Left (QWERTY A)
- **E** - Move Right (QWERTY D)
- **Space** - Jump
- **Mouse** - Look around

### Setting Up Input

```cpp
import bestow.input;
import bestow.input.impl;

auto input = createInputSystem();
auto* inputImpl = dynamic_cast<InputSystem*>(input.get());
if (!inputImpl || !inputImpl->initialize(window)) {
    std::println("Failed to initialize input system");
    return 1;
}

// Register Dvorak movement keys
input->registerMapping(InputMapping{
    .binding = {
        .deviceType = InputDeviceType::Keyboard,
        .deviceIndex = 0,
        .keyCode = GLFW_KEY_COMMA  // Dvorak: ,
    },
    .action = "MoveForward"
});

input->registerMapping(InputMapping{
    .binding = {
        .deviceType = InputDeviceType::Keyboard,
        .deviceIndex = 0,
        .keyCode = GLFW_KEY_O  // Dvorak: O
    },
    .action = "MoveBackward"
});

input->registerMapping(InputMapping{
    .binding = {
        .deviceType = InputDeviceType::Keyboard,
        .deviceIndex = 0,
        .keyCode = GLFW_KEY_A  // Dvorak: A
    },
    .action = "MoveLeft"
});

input->registerMapping(InputMapping{
    .binding = {
        .deviceType = InputDeviceType::Keyboard,
        .deviceIndex = 0,
        .keyCode = GLFW_KEY_E  // Dvorak: E
    },
    .action = "MoveRight"
});

input->registerMapping(InputMapping{
    .binding = {
        .deviceType = InputDeviceType::Keyboard,
        .deviceIndex = 0,
        .keyCode = GLFW_KEY_SPACE
    },
    .action = "Jump"
});
```

### Mouse Look

For first-person camera control, capture mouse movement:

```cpp
// GLFW mouse callback
double g_lastMouseX = 0.0;
double g_lastMouseY = 0.0;
double g_mouseDeltaX = 0.0;
double g_mouseDeltaY = 0.0;
bool g_firstMouse = true;
bool g_cursorCaptured = true;

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (g_firstMouse) {
        g_lastMouseX = xpos;
        g_lastMouseY = ypos;
        g_firstMouse = false;
    }

    g_mouseDeltaX = xpos - g_lastMouseX;
    g_mouseDeltaY = g_lastMouseY - ypos;  // Inverted Y for natural camera

    g_lastMouseX = xpos;
    g_lastMouseY = ypos;
}

// In your window setup
glfwSetCursorPosCallback(window, mouseCallback);
glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);  // Capture cursor
```

### Camera Rotation from Mouse

```cpp
constexpr float MOUSE_SENSITIVITY = 0.002f;

void updateCameraRotation(float& yaw, float& pitch) {
    if (g_cursorCaptured) {
        yaw -= static_cast<float>(g_mouseDeltaX) * MOUSE_SENSITIVITY;
        pitch += static_cast<float>(g_mouseDeltaY) * MOUSE_SENSITIVITY;

        // Clamp pitch to prevent camera flipping
        pitch = std::clamp(pitch, -1.5f, 1.5f);  // ~86 degrees up/down
    }

    // Reset mouse delta
    g_mouseDeltaX = 0.0;
    g_mouseDeltaY = 0.0;
}
```

### Movement Direction from Camera

```cpp
void updateMovement(IInputSystem* input, float yaw, Vec3& velocity) {
    // Calculate forward and right vectors from camera yaw
    glm::quat yawQuat = glm::angleAxis(yaw, glm::vec3(0.0f, 1.0f, 0.0f));
    Vec3 forward = yawQuat * glm::vec3(0.0f, 0.0f, -1.0f);  // Camera looks -Z
    Vec3 right = yawQuat * glm::vec3(1.0f, 0.0f, 0.0f);     // Right is +X

    // Build movement direction
    Vec3 moveDir{0.0f};
    if (input->isActionActive("MoveForward")) moveDir += forward;
    if (input->isActionActive("MoveBackward")) moveDir -= forward;
    if (input->isActionActive("MoveLeft")) moveDir -= right;
    if (input->isActionActive("MoveRight")) moveDir += right;

    // Normalize for consistent diagonal movement
    if (glm::length(moveDir) > 0.01f) {
        moveDir = glm::normalize(moveDir);
    }

    // Apply move speed
    constexpr float MOVE_SPEED = 5.0f;
    velocity.x = moveDir.x * MOVE_SPEED;
    velocity.z = moveDir.z * MOVE_SPEED;
}
```

## Complete Example: 3D Platformer

Here's a complete 3D platformer game with platforms, jumping, and first-person camera:

```cpp
// src/main.cpp
#define SDL_MAIN_HANDLED
#define GLM_ENABLE_EXPERIMENTAL

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

import std;
import bestow.types;
import bestow.input;
import bestow.input.impl;
import bestow.graphics3d;
import bestow.graphics3d.impl;
import bestow.physics3d;
import bestow.physics3d.impl;

using namespace bestow;

// Game constants
constexpr float MOVE_SPEED = 5.0f;
constexpr float JUMP_FORCE = 8.0f;
constexpr float MOUSE_SENSITIVITY = 0.002f;
constexpr float GRAVITY = -20.0f;
constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;

// Player state
struct Player {
    Entity entity;
    Vec3 position{0.0f, 2.0f, 0.0f};
    Vec3 velocity{0.0f};
    float yaw = 0.0f;
    float pitch = 0.0f;
    bool grounded = false;
    MeshHandle mesh;
    MaterialHandle material;
};

// Platform
struct Platform {
    Entity entity;
    Vec3 position;
    Vec3 size;
    MeshHandle mesh;
    MaterialHandle material;
};

// Mouse state
double g_mouseDeltaX = 0.0;
double g_mouseDeltaY = 0.0;
double g_lastMouseX = 0.0;
double g_lastMouseY = 0.0;
bool g_firstMouse = true;
bool g_cursorCaptured = true;

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (g_firstMouse) {
        g_lastMouseX = xpos;
        g_lastMouseY = ypos;
        g_firstMouse = false;
    }

    g_mouseDeltaX = xpos - g_lastMouseX;
    g_mouseDeltaY = g_lastMouseY - ypos;

    g_lastMouseX = xpos;
    g_lastMouseY = ypos;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}

GLFWwindow* createWindow() {
    if (!glfwInit()) return nullptr;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(
        WINDOW_WIDTH, WINDOW_HEIGHT,
        "My 3D Platformer", nullptr, nullptr
    );

    if (!window) {
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    return window;
}

void setupInput(IInputSystem* input) {
    input->registerMapping(InputMapping{
        .binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = GLFW_KEY_COMMA},
        .action = "MoveForward"
    });
    input->registerMapping(InputMapping{
        .binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = GLFW_KEY_O},
        .action = "MoveBackward"
    });
    input->registerMapping(InputMapping{
        .binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = GLFW_KEY_A},
        .action = "MoveLeft"
    });
    input->registerMapping(InputMapping{
        .binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = GLFW_KEY_E},
        .action = "MoveRight"
    });
    input->registerMapping(InputMapping{
        .binding = {.deviceType = InputDeviceType::Keyboard, .keyCode = GLFW_KEY_SPACE},
        .action = "Jump"
    });
}

std::vector<Platform> createPlatforms(IGraphics3DSystem* graphics, IPhysics3DSystem* physics) {
    std::vector<Platform> platforms;

    struct PlatformDef {
        Vec3 pos;
        Vec3 size;
        Vec3 color;
    };

    std::vector<PlatformDef> defs = {
        // Ground
        {{0.0f, -2.0f, 0.0f}, {50.0f, 1.0f, 50.0f}, {0.3f, 0.5f, 0.3f}},
        // Starting platform
        {{0.0f, 0.0f, 0.0f}, {4.0f, 0.5f, 4.0f}, {0.5f, 0.5f, 0.6f}},
        // Jump platforms
        {{5.0f, 1.0f, 0.0f}, {3.0f, 0.5f, 3.0f}, {0.6f, 0.4f, 0.4f}},
        {{10.0f, 2.5f, 2.0f}, {3.0f, 0.5f, 3.0f}, {0.4f, 0.6f, 0.4f}},
        {{8.0f, 4.0f, 6.0f}, {3.0f, 0.5f, 3.0f}, {0.4f, 0.4f, 0.6f}},
    };

    for (const auto& def : defs) {
        Platform platform;
        platform.position = def.pos;
        platform.size = def.size;
        platform.entity = static_cast<Entity>(platforms.size() + 100);

        // Create mesh
        auto meshResult = graphics->createCubeMesh(1.0f);
        if (meshResult) platform.mesh = *meshResult;

        // Create material
        PBRMaterial mat;
        mat.baseColorFactor = Vec4{def.color.x, def.color.y, def.color.z, 1.0f};
        mat.roughnessFactor = 0.7f;
        mat.metallicFactor = 0.1f;
        auto matResult = graphics->createMaterial(mat);
        if (matResult) platform.material = *matResult;

        // Create physics body
        PhysicsBodyDef3D bodyDef;
        bodyDef.type = BodyType3D::Static;
        bodyDef.transform.position = def.pos;
        bodyDef.shapeType = ShapeType3D::Box;
        bodyDef.shapeHalfExtents = def.size * 0.5f;
        physics->createBody(platform.entity, bodyDef);

        platforms.push_back(platform);
    }

    return platforms;
}

Player createPlayer(IGraphics3DSystem* graphics, IPhysics3DSystem* physics) {
    Player player;
    player.entity = static_cast<Entity>(1);
    player.position = Vec3{0.0f, 2.0f, 0.0f};

    // Create capsule mesh
    auto meshResult = graphics->createCapsuleMesh(0.3f, 0.8f);
    if (meshResult) player.mesh = *meshResult;

    // Create material
    PBRMaterial mat;
    mat.baseColorFactor = Vec4{0.2f, 0.4f, 0.9f, 1.0f};
    mat.roughnessFactor = 0.3f;
    mat.metallicFactor = 0.5f;
    auto matResult = graphics->createMaterial(mat);
    if (matResult) player.material = *matResult;

    // Create character controller
    CharacterControllerDef charDef;
    charDef.radius = 0.3f;
    charDef.height = 1.6f;
    charDef.stepHeight = 0.35f;
    charDef.maxSlopeAngle = 50.0f;
    charDef.mass = 80.0f;
    physics->createCharacter(player.entity, charDef);
    physics->setCharacterPosition(player.entity, player.position);

    return player;
}

void updatePlayer(Player& player, IInputSystem* input, IPhysics3DSystem* physics, float dt) {
    // Mouse look
    if (g_cursorCaptured) {
        player.yaw -= static_cast<float>(g_mouseDeltaX) * MOUSE_SENSITIVITY;
        player.pitch += static_cast<float>(g_mouseDeltaY) * MOUSE_SENSITIVITY;
        player.pitch = std::clamp(player.pitch, -1.5f, 1.5f);
    }
    g_mouseDeltaX = 0.0;
    g_mouseDeltaY = 0.0;

    // Calculate movement direction
    glm::quat yawQuat = glm::angleAxis(player.yaw, glm::vec3(0.0f, 1.0f, 0.0f));
    Vec3 forward = yawQuat * glm::vec3(0.0f, 0.0f, -1.0f);
    Vec3 right = yawQuat * glm::vec3(1.0f, 0.0f, 0.0f);

    Vec3 moveDir{0.0f};
    if (input->isActionActive("MoveForward")) moveDir += forward;
    if (input->isActionActive("MoveBackward")) moveDir -= forward;
    if (input->isActionActive("MoveLeft")) moveDir -= right;
    if (input->isActionActive("MoveRight")) moveDir += right;

    if (glm::length(moveDir) > 0.01f) {
        moveDir = glm::normalize(moveDir);
    }

    // Check grounded
    auto groundResult = physics->getCharacterGroundInfo(player.entity);
    if (groundResult) {
        player.grounded = (groundResult->state == CharacterGroundState::OnGround);
    }

    // Get current velocity
    auto velResult = physics->getCharacterVelocity(player.entity);
    if (velResult) player.velocity = *velResult;

    // Apply movement
    player.velocity.x = moveDir.x * MOVE_SPEED;
    player.velocity.z = moveDir.z * MOVE_SPEED;

    // Jump
    if (input->wasActionJustPressed("Jump") && player.grounded) {
        player.velocity.y = JUMP_FORCE;
    }

    // Apply gravity
    if (!player.grounded) {
        player.velocity.y += GRAVITY * dt;
    } else if (player.velocity.y < 0) {
        player.velocity.y = 0.0f;
    }

    // Move character
    physics->moveCharacter(player.entity, player.velocity, dt);

    // Get updated position
    auto posResult = physics->getCharacterPosition(player.entity);
    if (posResult) player.position = *posResult;

    // Respawn if fallen
    if (player.position.y < -10.0f) {
        player.position = Vec3{0.0f, 5.0f, 0.0f};
        player.velocity = Vec3{0.0f};
        physics->setCharacterPosition(player.entity, player.position);
    }
}

Camera3D getPlayerCamera(const Player& player) {
    Camera3D camera;

    // First-person camera at head height
    Vec3 cameraOffset{0.0f, 0.8f, 0.0f};
    camera.transform.position = player.position + cameraOffset;

    // Camera rotation
    glm::quat yawQuat = glm::angleAxis(player.yaw, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::quat pitchQuat = glm::angleAxis(player.pitch, glm::vec3(1.0f, 0.0f, 0.0f));
    camera.transform.rotation = yawQuat * pitchQuat;

    camera.projection = ProjectionType::Perspective;
    camera.fovY = 70.0f;
    camera.aspectRatio = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT);
    camera.nearPlane = 0.1f;
    camera.farPlane = 500.0f;

    return camera;
}

int main() {
    std::println("My 3D Platformer");
    std::println("Controls: ,AOE to move, Space to jump, Mouse to look");

    // Create window
    GLFWwindow* window = createWindow();
    if (!window) return 1;

    // Initialize systems
    auto graphics = createGraphics3DSystem();
    auto graphicsImpl = dynamic_cast<OpenGLGraphics3DSystem*>(graphics.get());
    if (!graphicsImpl || !graphicsImpl->initialize(window)) {
        std::println("Failed to initialize graphics");
        return 1;
    }

    auto physics = createPhysics3DSystem();
    if (!physics) {
        std::println("Failed to initialize physics");
        return 1;
    }

    auto input = createInputSystem();
    auto* inputImpl = dynamic_cast<InputSystem*>(input.get());
    if (!inputImpl || !inputImpl->initialize(window)) {
        std::println("Failed to initialize input");
        return 1;
    }

    // Setup
    setupInput(input.get());
    physics->setGravity(Vec3{0.0f, GRAVITY, 0.0f});

    // Create game objects
    auto platforms = createPlatforms(graphics.get(), physics.get());
    auto player = createPlayer(graphics.get(), physics.get());

    // Setup lighting
    DirectionalLight sunLight;
    sunLight.direction = glm::normalize(Vec3{-0.5f, -1.0f, -0.3f});
    sunLight.color = Vec3{1.0f, 0.95f, 0.9f};
    sunLight.intensity = 1.2f;
    graphics->setDirectionalLight(sunLight);
    graphics->setAmbientLight(Vec3{0.4f, 0.45f, 0.5f}, 0.3f);
    graphics->setClearColor(Color{128, 179, 230, 255});

    // Game loop
    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float dt = static_cast<float>(currentTime - lastTime);
        lastTime = currentTime;
        dt = std::min(dt, 0.05f);

        glfwPollEvents();
        input->update();
        physics->update(dt, 4);
        updatePlayer(player, input.get(), physics.get(), dt);

        graphics->beginFrame();
        graphics->setCamera(getPlayerCamera(player));

        // Render platforms
        for (const auto& platform : platforms) {
            Transform3D transform;
            transform.position = platform.position;
            transform.scale = platform.size;
            graphics->drawMesh(platform.mesh, platform.material, transform);
        }

        // Render player
        Transform3D playerTransform;
        playerTransform.position = player.position;
        graphics->drawMesh(player.mesh, player.material, playerTransform);

        graphics->debugDrawAxes(Transform3D{}, 2.0f);
        graphics->endFrame();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
```

## Debug Visualization

Graphics3D provides debug drawing for development:

```cpp
// Draw coordinate axes at origin
graphics->debugDrawAxes(Transform3D{}, 2.0f);

// Draw a line
graphics->debugDrawLine(
    Vec3{0.0f, 0.0f, 0.0f},
    Vec3{5.0f, 5.0f, 5.0f},
    Color::green()
);

// Draw a box (for showing collision bounds)
graphics->debugDrawBox(
    Vec3{0.0f, 1.0f, 0.0f},  // center
    Vec3{0.5f, 0.5f, 0.5f},  // half extents
    Quat{1.0f, 0.0f, 0.0f, 0.0f},  // rotation
    Color::yellow()
);

// Draw a sphere
graphics->debugDrawSphere(
    Vec3{3.0f, 1.0f, 0.0f},  // center
    0.5f,                     // radius
    Color::cyan()
);

// Draw a capsule
graphics->debugDrawCapsule(
    Vec3{0.0f, 0.0f, 0.0f},  // bottom
    Vec3{0.0f, 2.0f, 0.0f},  // top
    0.3f,                     // radius
    Color::magenta()
);

// Draw a ray
graphics->debugDrawRay(
    Vec3{0.0f, 2.0f, 0.0f},  // origin
    Vec3{0.0f, 0.0f, -1.0f}, // direction
    10.0f,                    // length
    Color::red()
);

// Enable/disable debug rendering
graphics->setDebugRenderingEnabled(true);

// Clear all debug primitives
graphics->debugClear();
```

## Best Practices

### 1. Use Character Controllers for Players

Don't use regular dynamic bodies for player characters:

```cpp
// Bad: Regular dynamic body
PhysicsBodyDef3D playerDef;
playerDef.type = BodyType3D::Dynamic;  // Will tumble and fall over

// Good: Character controller
CharacterControllerDef charDef;
charDef.radius = 0.3f;
charDef.height = 1.6f;
physics->createCharacter(playerEntity, charDef);
```

### 2. Normalize Movement Vectors

Prevent faster diagonal movement:

```cpp
// Always normalize before applying speed
if (glm::length(moveDir) > 0.01f) {
    moveDir = glm::normalize(moveDir);
}
moveDir *= MOVE_SPEED;
```

### 3. Clamp Camera Pitch

Prevent camera from flipping:

```cpp
// Limit vertical look to ~86 degrees
pitch = std::clamp(pitch, -1.5f, 1.5f);
```

### 4. Use Proper Collision Layers

Organize your physics objects with layers:

```cpp
// Define layers
namespace CollisionLayers3D {
    constexpr CollisionLayer3D World = 0x0001;
    constexpr CollisionLayer3D Character = 0x0002;
    constexpr CollisionLayer3D Platform = 0x0004;
    constexpr CollisionLayer3D Projectile = 0x0008;
}

// Character only collides with world and platforms
charDef.layer = CollisionLayers3D::Character;
charDef.mask = CollisionLayers3D::World | CollisionLayers3D::Platform;
```

### 5. Cap Delta Time

Prevent physics explosions during lag spikes:

```cpp
dt = std::min(dt, 0.05f);  // Max 50ms per frame
```

## Troubleshooting

**Player falls through platforms**
- Check collision layers and masks match
- Ensure platforms are created before first physics update
- Verify platform has `BodyType3D::Static`

**Camera movement feels sluggish or choppy**
- Adjust `MOUSE_SENSITIVITY` constant
- Check that `g_mouseDelta` is being reset each frame
- Ensure cursor is captured: `glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED)`

**Character gets stuck on edges**
- Increase `stepHeight` in `CharacterControllerDef`
- Check collision shape size matches visual mesh
- Reduce `maxSlopeAngle` if climbing unwanted surfaces

**Meshes not visible**
- Check camera position and direction
- Verify lighting is set up (at least ambient light)
- Ensure materials have non-zero alpha
- Check mesh bounds with debug visualization

**Lighting too dark or too bright**
- Adjust directional light intensity
- Increase ambient light for indoor scenes
- Use `graphics->setExposure()` for brightness control

## Next Steps

You now understand 3D graphics and physics in Bestow! Explore:

- **Advanced Materials** - Normal maps, roughness maps, PBR textures
- **Skeletal Animation** - Animated 3D models with bones
- **Instanced Rendering** - Efficiently render thousands of objects
- **Post-Processing** - Bloom, tone mapping, SSAO
- **Constraints** - Hinges, sliders, ragdolls
- **Vehicles** - Car physics with wheels and suspension

## Further Reading

- **3D Platformer Example**: `/Users/jaaaacob/Documents/GameDev/jframe/examples/3d-platformer/src/main.cpp`
- **Graphics3D API**: `/Users/jaaaacob/Documents/GameDev/jframe/bestow-contract/src/bestow.graphics3d.cppm`
- **Physics3D API**: `/Users/jaaaacob/Documents/GameDev/jframe/bestow-contract/src/bestow.physics3d.cppm`
- **Jolt Physics Documentation**: https://jrouwe.github.io/JoltPhysics/
- **PBR Theory**: https://learnopengl.com/PBR/Theory
