// examples/3d-platformer/src/main.cpp
// 3D Platformer Demo - Demonstrates Physics3D, Graphics3D, and Input systems
//
// Controls (Dvorak layout):
//   ,AOE - Move (equivalent to WASD on QWERTY)
//   Space - Jump
//   Mouse - Look around
//   Escape - Exit

// Third-party headers MUST come BEFORE 'import std;' for C++23 module compatibility
#define SDL_MAIN_HANDLED
#define GLM_ENABLE_EXPERIMENTAL

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

import std;
import bestow.types;
import bestow.input;
import bestow.input.impl;
import bestow.graphics3d;
import bestow.graphics3d.impl;
import bestow.physics3d;
import bestow.physics3d.impl;
import bestow.shader;
import bestow.shader.impl;

using namespace bestow;

// Game configuration
constexpr float MOVE_SPEED = 5.0f;
constexpr float JUMP_FORCE = 8.0f;
constexpr float MOUSE_SENSITIVITY = 0.002f;
constexpr float GRAVITY = -20.0f;
constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;

// Third-person camera configuration
constexpr float CAMERA_DISTANCE_MIN = 1.0f;      // Minimum distance (when against wall)
constexpr float CAMERA_DISTANCE_MAX = 5.0f;      // Ideal/maximum distance behind player
constexpr float CAMERA_HEIGHT_OFFSET = 1.5f;     // Height above player pivot
constexpr float CAMERA_LOOK_OFFSET = 0.8f;       // Look target height offset
constexpr float CAMERA_COLLISION_RADIUS = 0.3f;  // Sphere radius for camera collision
constexpr float CAMERA_SMOOTH_SPEED = 12.0f;     // How fast camera adjusts distance

// Camera state for smooth third-person following
struct CameraState {
    float currentDistance = CAMERA_DISTANCE_MAX;  // Current smoothed distance
    float targetDistance = CAMERA_DISTANCE_MAX;   // Target distance (after collision)
};

// Player state
struct Player {
    Entity entity;
    Vec3 position{0.0f, 2.0f, 0.0f};
    Vec3 velocity{0.0f};
    float yaw = 0.0f;    // Horizontal rotation
    float pitch = 0.0f;  // Vertical rotation
    bool grounded = false;
    MeshHandle mesh;
    MaterialHandle material;
    std::string luaMaterial;  // For cel-shading support
};

// Global camera state
CameraState g_cameraState;

// Platform definition
struct Platform {
    Entity entity;
    Vec3 position;
    Vec3 size;
    Vec3 color;  // Per-object color for cel-shading
    MeshHandle mesh;
    MaterialHandle material;
    std::string luaMaterial;  // Optional Lua material path for special effects
    bool useShaderMaterial = false;
};

// GLFW callbacks for input
GLFWwindow* g_window = nullptr;
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
    g_mouseDeltaY = g_lastMouseY - ypos;  // Inverted for natural camera movement

    g_lastMouseX = xpos;
    g_lastMouseY = ypos;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        if (g_cursorCaptured) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            g_cursorCaptured = false;
        } else {
            glfwSetWindowShouldClose(window, true);
        }
    }

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        if (!g_cursorCaptured) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            g_cursorCaptured = true;
            g_firstMouse = true;
        }
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && !g_cursorCaptured) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        g_cursorCaptured = true;
        g_firstMouse = true;
    }
}

// Create the game window
GLFWwindow* createWindow() {
    if (!glfwInit()) {
        std::println("Failed to initialize GLFW");
        return nullptr;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
                                           "Bestow 3D Platformer Demo", nullptr, nullptr);
    if (!window) {
        std::println("Failed to create GLFW window");
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    return window;
}

// Setup input mappings
void setupInput(IInputSystem* input) {
    // Movement - ,AOE (Dvorak equivalent of WASD)
    input->registerMapping(InputMapping{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = GLFW_KEY_COMMA},
        .action = "MoveForward"
    });
    input->registerMapping(InputMapping{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = GLFW_KEY_O},
        .action = "MoveBackward"
    });
    input->registerMapping(InputMapping{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = GLFW_KEY_A},
        .action = "MoveLeft"
    });
    input->registerMapping(InputMapping{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = GLFW_KEY_E},
        .action = "MoveRight"
    });
    input->registerMapping(InputMapping{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = GLFW_KEY_SPACE},
        .action = "Jump"
    });
}

// Create platforms in the scene
std::vector<Platform> createPlatforms(IGraphics3DSystem* graphics, IPhysics3DSystem* physics) {
    std::vector<Platform> platforms;

    // Platform data: position, size
    struct PlatformDef {
        Vec3 pos;
        Vec3 size;
        Vec3 color;
        std::string luaMaterial;  // Empty = use PBR, otherwise use Lua material
    };

    // All platforms use cel-shading for a cartoon look!
    // Material paths are relative to the material base path (assets/materials/)
    std::vector<PlatformDef> defs = {
        // Ground floor - cel-shaded grass green
        {{0.0f, -2.0f, 0.0f}, {50.0f, 1.0f, 50.0f}, {0.35f, 0.6f, 0.25f}, "toon.lua"},

        // Starting platform - cel-shaded stone gray
        {{0.0f, 0.0f, 0.0f}, {4.0f, 0.5f, 4.0f}, {0.6f, 0.55f, 0.5f}, "toon.lua"},

        // Jumping platforms - colorful cel-shaded blocks
        {{5.0f, 1.0f, 0.0f}, {3.0f, 0.5f, 3.0f}, {0.85f, 0.4f, 0.35f}, "toon.lua"},   // Red
        {{10.0f, 2.5f, 2.0f}, {3.0f, 0.5f, 3.0f}, {0.4f, 0.75f, 0.4f}, "toon.lua"},   // Green
        {{8.0f, 4.0f, 6.0f}, {3.0f, 0.5f, 3.0f}, {0.4f, 0.5f, 0.85f}, "toon.lua"},    // Blue
        {{3.0f, 5.5f, 8.0f}, {3.0f, 0.5f, 3.0f}, {0.9f, 0.8f, 0.3f}, "toon.lua"},     // Yellow
        {{-2.0f, 7.0f, 6.0f}, {3.0f, 0.5f, 3.0f}, {0.7f, 0.4f, 0.8f}, "toon.lua"},    // Purple
        {{-6.0f, 8.5f, 3.0f}, {3.0f, 0.5f, 3.0f}, {0.9f, 0.6f, 0.3f}, "toon.lua"},    // Orange

        // Goal platform - glowing gold with special effect!
        {{-8.0f, 10.0f, -2.0f}, {4.0f, 0.5f, 4.0f}, {1.0f, 0.85f, 0.2f}, "glow.lua"},

        // Side platforms - cel-shaded pastels
        {{-5.0f, 2.0f, -3.0f}, {2.5f, 0.5f, 2.5f}, {0.7f, 0.5f, 0.7f}, "toon.lua"},   // Lavender
        {{-8.0f, 3.5f, 0.0f}, {2.5f, 0.5f, 2.5f}, {0.5f, 0.7f, 0.7f}, "toon.lua"},    // Teal
    };

    for (const auto& def : defs) {
        Platform platform;
        platform.position = def.pos;
        platform.size = def.size;
        platform.color = def.color;  // Store for per-object shader color
        platform.entity = static_cast<Entity>(platforms.size() + 100);

        // Create mesh (scaled cube)
        auto meshResult = graphics->createCubeMesh(1.0f);
        if (meshResult) {
            platform.mesh = *meshResult;
        }

        // Check if this platform uses a Lua shader material
        if (!def.luaMaterial.empty()) {
            platform.luaMaterial = def.luaMaterial;
            platform.useShaderMaterial = true;
        }

        // Create fallback PBR material with color (used when shader system not available)
        PBRMaterial mat;
        mat.baseColorFactor = Vec4{def.color.x, def.color.y, def.color.z, 1.0f};
        mat.roughnessFactor = 0.7f;
        mat.metallicFactor = 0.1f;
        auto matResult = graphics->createMaterial(mat);
        if (matResult) {
            platform.material = *matResult;
        }

        // Create physics body (static)
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

// Create the player
Player createPlayer(IGraphics3DSystem* graphics, IPhysics3DSystem* physics) {
    Player player;
    player.entity = static_cast<Entity>(1);
    player.position = Vec3{0.0f, 2.0f, 0.0f};
    player.luaMaterial = "toon.lua";  // Cel-shaded player!

    // Create capsule mesh for player
    auto meshResult = graphics->createCapsuleMesh(0.3f, 0.8f);
    if (meshResult) {
        player.mesh = *meshResult;
    } else {
        // Fallback to sphere if capsule not implemented
        auto sphereResult = graphics->createSphereMesh(0.4f);
        if (sphereResult) {
            player.mesh = *sphereResult;
        }
    }

    // Player material - bright cartoon blue (fallback and for shader base color)
    PBRMaterial mat;
    mat.baseColorFactor = Vec4{0.3f, 0.5f, 0.95f, 1.0f};  // Bright cartoon blue
    mat.roughnessFactor = 0.3f;
    mat.metallicFactor = 0.5f;
    auto matResult = graphics->createMaterial(mat);
    if (matResult) {
        player.material = *matResult;
    }

    // Create character controller (proper character physics with ground detection)
    CharacterControllerDef charDef;
    charDef.radius = 0.3f;
    charDef.height = 1.6f;
    charDef.stepHeight = 0.35f;
    charDef.maxSlopeAngle = 50.0f;
    charDef.mass = 80.0f;

    auto result = physics->createCharacter(player.entity, charDef);
    if (!result) {
        std::println("Warning: Failed to create character controller");
    }

    // Set initial position
    physics->setCharacterPosition(player.entity, player.position);

    return player;
}

// Update player movement
void updatePlayer(Player& player, IInputSystem* input, IPhysics3DSystem* physics, float dt) {
    // Mouse look (only when cursor is captured)
    if (g_cursorCaptured) {
        player.yaw -= static_cast<float>(g_mouseDeltaX) * MOUSE_SENSITIVITY;
        player.pitch += static_cast<float>(g_mouseDeltaY) * MOUSE_SENSITIVITY;

        // Clamp pitch to prevent camera flipping
        player.pitch = std::clamp(player.pitch, -1.5f, 1.5f);
    }

    // Reset mouse delta
    g_mouseDeltaX = 0.0;
    g_mouseDeltaY = 0.0;

    // Calculate forward and right vectors using the same yaw quaternion as the camera
    // This ensures movement direction matches where the camera is looking
    glm::quat yawQuat = glm::angleAxis(player.yaw, glm::vec3(0.0f, 1.0f, 0.0f));
    Vec3 forward = yawQuat * glm::vec3(0.0f, 0.0f, -1.0f);  // Camera looks along -Z
    Vec3 right = yawQuat * glm::vec3(1.0f, 0.0f, 0.0f);     // Right is +X

    // Movement input
    Vec3 moveDir{0.0f};
    if (input->isActionActive("MoveForward")) moveDir += forward;
    if (input->isActionActive("MoveBackward")) moveDir -= forward;
    if (input->isActionActive("MoveLeft")) moveDir -= right;
    if (input->isActionActive("MoveRight")) moveDir += right;

    // Normalize if moving diagonally
    if (glm::length(moveDir) > 0.01f) {
        moveDir = glm::normalize(moveDir);
    }

    // Get ground state from character controller
    auto groundResult = physics->getCharacterGroundInfo(player.entity);
    if (groundResult) {
        player.grounded = (groundResult->state == CharacterGroundState::OnGround ||
                          groundResult->state == CharacterGroundState::OnSteepGround);
    }

    // Fallback: use raycast for ground detection if character controller reports InAir
    if (!player.grounded) {
        Vec3 rayStart = player.position + Vec3{0.0f, 0.1f, 0.0f};
        Vec3 rayDir{0.0f, -1.0f, 0.0f};
        auto rayResult = physics->raycast(rayStart, rayDir, 0.3f);
        if (rayResult.has_value()) {
            player.grounded = true;
        }
    }

    // Apply horizontal movement directly - physics system handles wall collision projection
    player.velocity.x = moveDir.x * MOVE_SPEED;
    player.velocity.z = moveDir.z * MOVE_SPEED;

    // Jump
    if (input->wasActionJustPressed("Jump") && player.grounded) {
        player.velocity.y = JUMP_FORCE;
    }

    // Apply gravity manually for character controller
    if (!player.grounded) {
        player.velocity.y += GRAVITY * dt;
    } else if (player.velocity.y < 0) {
        // Reset downward velocity when grounded
        player.velocity.y = 0.0f;
    }

    // Move character controller (handles collisions and sliding)
    physics->moveCharacter(player.entity, player.velocity, dt);

    // Get position from character controller
    auto posResult = physics->getCharacterPosition(player.entity);
    if (posResult) {
        player.position = *posResult;
    }

    // Respawn if fallen too far
    if (player.position.y < -10.0f) {
        player.position = Vec3{0.0f, 5.0f, 0.0f};
        player.velocity = Vec3{0.0f};
        physics->setCharacterPosition(player.entity, player.position);
    }
}

// Update camera with wall collision detection (telescoping)
void updateCameraCollision(const Player& player, IPhysics3DSystem* physics, float dt) {
    // Calculate the look target (where the camera looks at)
    Vec3 lookTarget = player.position + Vec3{0.0f, CAMERA_LOOK_OFFSET, 0.0f};

    // Calculate ideal camera direction (behind player based on yaw/pitch)
    glm::quat yawQuat = glm::angleAxis(player.yaw, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::quat pitchQuat = glm::angleAxis(player.pitch, glm::vec3(1.0f, 0.0f, 0.0f));
    glm::quat cameraRot = yawQuat * pitchQuat;

    // Camera looks forward (-Z), so "behind" is +Z in camera space
    Vec3 behindDir = cameraRot * glm::vec3(0.0f, 0.0f, 1.0f);

    // Add some height to the camera offset
    Vec3 cameraOffsetDir = glm::normalize(behindDir + Vec3{0.0f, CAMERA_HEIGHT_OFFSET / CAMERA_DISTANCE_MAX, 0.0f});

    // Start position for raycast (at player's head)
    Vec3 rayStart = lookTarget;

    // Raycast from player toward ideal camera position
    float targetDist = CAMERA_DISTANCE_MAX;

    auto hitResult = physics->raycast(rayStart, cameraOffsetDir, CAMERA_DISTANCE_MAX + CAMERA_COLLISION_RADIUS);
    if (hitResult.has_value()) {
        // Hit something - pull camera closer
        float hitDist = hitResult->distance - CAMERA_COLLISION_RADIUS;
        targetDist = std::max(CAMERA_DISTANCE_MIN, hitDist);
    }

    g_cameraState.targetDistance = targetDist;

    // Smooth camera distance - fast when pulling in (hitting wall), slower when extending out
    float smoothSpeed = CAMERA_SMOOTH_SPEED;
    if (g_cameraState.currentDistance > g_cameraState.targetDistance) {
        // Pulling in quickly (wall hit)
        smoothSpeed *= 3.0f;
    }

    float diff = g_cameraState.targetDistance - g_cameraState.currentDistance;
    g_cameraState.currentDistance += diff * std::min(1.0f, smoothSpeed * dt);
}

// Get camera transform from player (third-person with collision)
Camera3D getPlayerCamera(const Player& player) {
    Camera3D camera;

    // Look target: slightly above player pivot
    Vec3 lookTarget = player.position + Vec3{0.0f, CAMERA_LOOK_OFFSET, 0.0f};

    // Calculate camera orientation from yaw and pitch
    glm::quat yawQuat = glm::angleAxis(player.yaw, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::quat pitchQuat = glm::angleAxis(player.pitch, glm::vec3(1.0f, 0.0f, 0.0f));
    glm::quat cameraRot = yawQuat * pitchQuat;

    // Camera is positioned behind the player
    // In camera space, forward is -Z, so behind is +Z
    Vec3 behindDir = cameraRot * glm::vec3(0.0f, 0.0f, 1.0f);

    // Add height offset to camera position
    Vec3 cameraOffset = behindDir * g_cameraState.currentDistance;
    cameraOffset.y += CAMERA_HEIGHT_OFFSET * (g_cameraState.currentDistance / CAMERA_DISTANCE_MAX);

    // Final camera position
    camera.transform.position = lookTarget + cameraOffset;

    // Camera looks at the look target
    Vec3 lookDir = glm::normalize(lookTarget - camera.transform.position);

    // Calculate rotation to look at target
    // Create a rotation that points -Z toward lookDir
    Vec3 forward = -lookDir;  // Camera forward is -Z
    Vec3 right = glm::normalize(glm::cross(Vec3{0.0f, 1.0f, 0.0f}, forward));
    Vec3 up = glm::cross(forward, right);

    glm::mat3 rotMatrix(right, up, forward);
    camera.transform.rotation = glm::quat_cast(rotMatrix);

    camera.projection = ProjectionType::Perspective;
    camera.fovY = 60.0f;  // Slightly narrower FOV for third-person
    camera.aspectRatio = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT);
    camera.nearPlane = 0.1f;
    camera.farPlane = 500.0f;

    return camera;
}

int main() {
    std::println("Bestow 3D Platformer Demo");
    std::println("==========================");
    std::println("Controls (Dvorak layout):");
    std::println("  ,AOE  - Move");
    std::println("  Space - Jump");
    std::println("  Mouse - Look around");
    std::println("  Tab   - Recapture mouse");
    std::println("  Esc   - Release mouse / Exit");
    std::println("");

    // Create window
    g_window = createWindow();
    if (!g_window) {
        return 1;
    }

    // Initialize systems
    auto graphics = createGraphics3DSystem();
    auto graphicsImpl = dynamic_cast<OpenGLGraphics3DSystem*>(graphics.get());
    if (!graphicsImpl || !graphicsImpl->initialize(g_window)) {
        std::println("Failed to initialize graphics system");
        glfwTerminate();
        return 1;
    }

    auto physics = createPhysics3DSystem();
    if (!physics) {
        std::println("Failed to initialize physics system");
        glfwTerminate();
        return 1;
    }

    // Create shader system and connect to graphics
    auto shaderSystem = createShaderSystem();
    graphics->setShaderSystem(shaderSystem.get());
    shaderSystem->setShaderBasePath("assets/shaders/");
    shaderSystem->setMaterialBasePath("assets/materials/");
    std::println("Shader system initialized with hot reload support");

    auto inputPtr = createInputSystem();
    auto* inputImpl = dynamic_cast<InputSystem*>(inputPtr.get());
    if (!inputImpl || !inputImpl->initialize(g_window)) {
        std::println("Failed to initialize input system");
        glfwTerminate();
        return 1;
    }
    auto& input = inputPtr;  // Keep using input variable name for consistency

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

    // Sky color (Color uses 0-255)
    graphics->setClearColor(Color{128, 179, 230, 255});

    std::println("Starting game loop...");

    // Game loop
    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(g_window)) {
        // Calculate delta time
        double currentTime = glfwGetTime();
        float dt = static_cast<float>(currentTime - lastTime);
        lastTime = currentTime;

        // Cap delta time to prevent physics issues
        dt = std::min(dt, 0.05f);

        // Poll events
        glfwPollEvents();

        // Update input
        input->update();

        // Update physics
        physics->update(dt, 4);

        // Update player
        updatePlayer(player, input.get(), physics.get(), dt);

        // Update third-person camera collision (telescoping behind walls)
        updateCameraCollision(player, physics.get(), dt);

        // Begin frame
        graphics->beginFrame();

        // Set camera (third-person with wall avoidance)
        Camera3D camera = getPlayerCamera(player);
        graphics->setCamera(camera);

        // Check for shader hot reload
        graphics->updateShaders();

        // Render platforms
        for (const auto& platform : platforms) {
            Transform3D transform;
            transform.position = platform.position;
            transform.scale = platform.size;

            // Use Lua shader material if available, otherwise use PBR material
            if (platform.useShaderMaterial && !platform.luaMaterial.empty()) {
                // Calculate world matrix for shader
                Mat4 worldMatrix = glm::translate(Mat4(1.0f), platform.position);
                worldMatrix = glm::scale(worldMatrix, platform.size);

                // Pass per-object color to the shader
                Vec4 colorOverride{platform.color.x, platform.color.y, platform.color.z, 1.0f};
                auto result = graphics->drawMeshWithLuaMaterial(
                    platform.mesh, platform.luaMaterial, worldMatrix, colorOverride);

                if (!result) {
                    // Fallback to regular PBR if shader fails
                    graphics->drawMesh(platform.mesh, platform.material, transform);
                }
            } else {
                graphics->drawMesh(platform.mesh, platform.material, transform);
            }
        }

        // Render player (visible in third person, or for shadow) - cel-shaded!
        {
            Transform3D transform;
            transform.position = player.position;

            // Use cel-shader for player with bright cartoon blue
            if (!player.luaMaterial.empty()) {
                Mat4 worldMatrix = glm::translate(Mat4(1.0f), player.position);
                Vec4 playerColor{0.3f, 0.5f, 0.95f, 1.0f};  // Bright cartoon blue
                auto result = graphics->drawMeshWithLuaMaterial(
                    player.mesh, player.luaMaterial, worldMatrix, playerColor);

                if (!result) {
                    // Fallback to PBR if shader fails
                    graphics->drawMesh(player.mesh, player.material, transform);
                }
            } else {
                graphics->drawMesh(player.mesh, player.material, transform);
            }
        }

        // Debug: draw some coordinate axes at origin (disabled - causes rendering artifacts)
        // graphics->debugDrawAxes(Transform3D{}, 2.0f);

        // End frame
        graphics->endFrame();
    }

    std::println("Shutting down...");

    // Cleanup
    glfwDestroyWindow(g_window);
    glfwTerminate();

    std::println("Goodbye!");
    return 0;
}
