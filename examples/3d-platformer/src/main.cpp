// examples/3d-platformer/src/main.cpp
// 3D Platformer Demo - Demonstrates Physics3D, Graphics3D, and Input systems
//
// Controls:
//   WASD - Move
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

// Game configuration
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
    float yaw = 0.0f;    // Horizontal rotation
    float pitch = 0.0f;  // Vertical rotation
    bool grounded = false;
    MeshHandle mesh;
    MaterialHandle material;
};

// Platform definition
struct Platform {
    Entity entity;
    Vec3 position;
    Vec3 size;
    MeshHandle mesh;
    MaterialHandle material;
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
    // Movement - WASD
    input->registerMapping(InputMapping{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = GLFW_KEY_W},
        .action = "MoveForward"
    });
    input->registerMapping(InputMapping{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = GLFW_KEY_S},
        .action = "MoveBackward"
    });
    input->registerMapping(InputMapping{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = GLFW_KEY_A},
        .action = "MoveLeft"
    });
    input->registerMapping(InputMapping{
        .binding = {.deviceType = InputDeviceType::Keyboard, .deviceIndex = 0, .keyCode = GLFW_KEY_D},
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
    };

    std::vector<PlatformDef> defs = {
        // Ground floor (endless floor)
        {{0.0f, -2.0f, 0.0f}, {50.0f, 1.0f, 50.0f}, {0.3f, 0.5f, 0.3f}},

        // Starting platform
        {{0.0f, 0.0f, 0.0f}, {4.0f, 0.5f, 4.0f}, {0.5f, 0.5f, 0.6f}},

        // Jumping platforms
        {{5.0f, 1.0f, 0.0f}, {3.0f, 0.5f, 3.0f}, {0.6f, 0.4f, 0.4f}},
        {{10.0f, 2.5f, 2.0f}, {3.0f, 0.5f, 3.0f}, {0.4f, 0.6f, 0.4f}},
        {{8.0f, 4.0f, 6.0f}, {3.0f, 0.5f, 3.0f}, {0.4f, 0.4f, 0.6f}},
        {{3.0f, 5.5f, 8.0f}, {3.0f, 0.5f, 3.0f}, {0.6f, 0.6f, 0.4f}},
        {{-2.0f, 7.0f, 6.0f}, {3.0f, 0.5f, 3.0f}, {0.5f, 0.4f, 0.6f}},
        {{-6.0f, 8.5f, 3.0f}, {3.0f, 0.5f, 3.0f}, {0.6f, 0.5f, 0.4f}},
        {{-8.0f, 10.0f, -2.0f}, {4.0f, 0.5f, 4.0f}, {0.8f, 0.7f, 0.3f}},  // Goal platform

        // Some side platforms
        {{-5.0f, 2.0f, -3.0f}, {2.5f, 0.5f, 2.5f}, {0.5f, 0.3f, 0.5f}},
        {{-8.0f, 3.5f, 0.0f}, {2.5f, 0.5f, 2.5f}, {0.3f, 0.5f, 0.5f}},
    };

    for (const auto& def : defs) {
        Platform platform;
        platform.position = def.pos;
        platform.size = def.size;
        platform.entity = static_cast<Entity>(platforms.size() + 100);

        // Create mesh (scaled cube)
        auto meshResult = graphics->createCubeMesh(1.0f);
        if (meshResult) {
            platform.mesh = *meshResult;
        }

        // Create material with color
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

    // Player material (bright blue)
    PBRMaterial mat;
    mat.baseColorFactor = Vec4{0.2f, 0.4f, 0.9f, 1.0f};
    mat.roughnessFactor = 0.3f;
    mat.metallicFactor = 0.5f;
    auto matResult = graphics->createMaterial(mat);
    if (matResult) {
        player.material = *matResult;
    }

    // Create physics body (dynamic capsule)
    PhysicsBodyDef3D bodyDef;
    bodyDef.type = BodyType3D::Dynamic;
    bodyDef.transform.position = player.position;
    bodyDef.shapeType = ShapeType3D::Capsule;
    bodyDef.shapeRadius = 0.3f;
    bodyDef.shapeHalfHeight = 0.4f;
    bodyDef.density = 1000.0f;
    bodyDef.friction = 0.5f;
    bodyDef.linearDamping = 0.1f;
    physics->createBody(player.entity, bodyDef);

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

    // Calculate forward and right vectors based on yaw only (for movement)
    Vec3 forward{
        std::sin(player.yaw),
        0.0f,
        -std::cos(player.yaw)
    };
    Vec3 right{
        std::cos(player.yaw),
        0.0f,
        std::sin(player.yaw)
    };

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

    // Get current velocity from physics
    auto velResult = physics->getLinearVelocity(player.entity);
    if (velResult) {
        player.velocity = *velResult;
    }

    // Apply horizontal movement
    Vec3 targetVel = moveDir * MOVE_SPEED;
    player.velocity.x = targetVel.x;
    player.velocity.z = targetVel.z;

    // Check if grounded (simple check: velocity.y is near zero and we're not going up)
    player.grounded = std::abs(player.velocity.y) < 0.5f && player.velocity.y <= 0.1f;

    // Jump
    if (input->wasActionJustPressed("Jump") && player.grounded) {
        player.velocity.y = JUMP_FORCE;
    }

    // Apply gravity (physics system handles this, but we might need to tweak)
    // The physics system should have gravity set

    // Set velocity back to physics
    physics->setLinearVelocity(player.entity, player.velocity);

    // Get position from physics
    auto posResult = physics->getPosition(player.entity);
    if (posResult) {
        player.position = *posResult;
    }

    // Respawn if fallen too far
    if (player.position.y < -10.0f) {
        player.position = Vec3{0.0f, 5.0f, 0.0f};
        player.velocity = Vec3{0.0f};
        physics->setPosition(player.entity, player.position);
        physics->setLinearVelocity(player.entity, player.velocity);
    }
}

// Get camera transform from player
Camera3D getPlayerCamera(const Player& player) {
    Camera3D camera;

    // Camera position: slightly behind and above player
    Vec3 cameraOffset{0.0f, 0.8f, 0.0f};  // First person: at head height
    camera.transform.position = player.position + cameraOffset;

    // Camera rotation from yaw and pitch
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
    std::println("Bestow 3D Platformer Demo");
    std::println("==========================");
    std::println("Controls:");
    std::println("  WASD  - Move");
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

    auto input = createInputSystem();
    if (!input) {
        std::println("Failed to initialize input system");
        glfwTerminate();
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

        // Sync physics to get updated positions
        // (The physics system updates body positions internally)

        // Begin frame
        graphics->beginFrame();

        // Set camera
        Camera3D camera = getPlayerCamera(player);
        graphics->setCamera(camera);

        // Render platforms
        for (const auto& platform : platforms) {
            Transform3D transform;
            transform.position = platform.position;
            transform.scale = platform.size;
            graphics->drawMesh(platform.mesh, platform.material, transform);
        }

        // Render player (visible in third person, or for shadow)
        {
            Transform3D transform;
            transform.position = player.position;
            graphics->drawMesh(player.mesh, player.material, transform);
        }

        // Debug: draw some coordinate axes at origin
        graphics->debugDrawAxes(Transform3D{}, 2.0f);

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
