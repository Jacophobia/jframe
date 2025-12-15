// games/game1/src/snake.game.cppm
// 3D Isometric Snake Game
//
// A simple snake game demonstrating the Bestow contract-based DI architecture.
// Uses 3D isometric view with camera following the snake head.
// Controls: ,AOE (Dvorak) or Arrow Keys for movement

module;

#include <kangaru/kangaru.hpp>
#include <GLFW/glfw3.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

export module snake.game;

import std;
import bestow.services;  // All contracts + service definitions
import bestow.types;
import bestow.graphics3d;

export namespace snake {

//==========================================================================
// Game Constants
//==========================================================================

constexpr int GRID_SIZE = 20;              // 20x20 grid
constexpr float CELL_SIZE = 1.0f;          // Each cell is 1 unit
constexpr float MOVE_INTERVAL = 0.15f;     // Move every 150ms
constexpr float CAMERA_DISTANCE = 25.0f;   // Camera distance from center
constexpr float CAMERA_HEIGHT = 20.0f;     // Camera height for isometric view

//==========================================================================
// Direction Enum
//==========================================================================

enum class Direction {
    Up,     // -Z
    Down,   // +Z
    Left,   // -X
    Right   // +X
};

//==========================================================================
// Grid Position
//==========================================================================

struct GridPos {
    int x = 0;
    int z = 0;

    bool operator==(const GridPos& other) const {
        return x == other.x && z == other.z;
    }

    GridPos operator+(const GridPos& other) const {
        return {x + other.x, z + other.z};
    }
};

GridPos directionToOffset(Direction dir) {
    switch (dir) {
        case Direction::Up:    return {0, -1};
        case Direction::Down:  return {0, 1};
        case Direction::Left:  return {-1, 0};
        case Direction::Right: return {1, 0};
    }
    return {0, 0};
}

//==========================================================================
// Snake Game Application
//
// This game extends IApplication and receives its systems via DI.
// The main.cpp registers the implementations and resolves this.
//==========================================================================

class SnakeGame : public bestow::IApplication {
public:
    // Constructor receives dependencies via Kangaru DI
    explicit SnakeGame(
        bestow::IGraphics3DSystem* graphics = nullptr,
        bestow::IInputSystem* input = nullptr,
        bestow::IAudioSystem* audio = nullptr)
        : graphics_(graphics)
        , input_(input)
        , audio_(audio) {}

    ~SnakeGame() override = default;

    //======================================================================
    // IApplication Interface
    //======================================================================

    void run() override {
        if (!initialize()) {
            return;
        }

        gameLoop();
        cleanup();
    }

    void shutdown() override {
        running_ = false;
    }

private:
    //======================================================================
    // Dependencies (injected via constructor)
    //======================================================================
    bestow::IGraphics3DSystem* graphics_ = nullptr;
    bestow::IInputSystem* input_ = nullptr;
    bestow::IAudioSystem* audio_ = nullptr;

    //======================================================================
    // Meshes
    //======================================================================
    bestow::MeshHandle cubeMesh_ = 0;
    bestow::MeshHandle groundMesh_ = 0;

    //======================================================================
    // Materials
    //======================================================================
    bestow::MaterialHandle snakeMaterial_ = 0;
    bestow::MaterialHandle groundMaterial_ = 0;
    bestow::MaterialHandle foodMaterial_ = 0;

    //======================================================================
    // Game State
    //======================================================================
    std::vector<GridPos> snake_;
    GridPos food_;
    Direction direction_ = Direction::Right;
    Direction nextDirection_ = Direction::Right;
    float moveTimer_ = 0.0f;
    int score_ = 0;
    bool gameOver_ = false;
    bool running_ = false;

    //======================================================================
    // Camera
    //======================================================================
    bestow::Vec3 cameraTarget_{0.0f};
    float cameraAngle_ = 45.0f;  // Degrees around Y axis

    //======================================================================
    // Initialization
    //======================================================================

    bool initialize() {
        if (!graphics_) {
            std::cerr << "Graphics system not available\n";
            return false;
        }

        // Initialize graphics
        bestow::Graphics3DConfig gfxConfig{
            .windowWidth = 1280,
            .windowHeight = 720,
            .windowTitle = "Snake 3D - Bestow Demo",
            .vsync = true,
            .fullscreen = false
        };

        if (!graphics_->initialize(gfxConfig)) {
            std::cerr << "Failed to initialize graphics\n";
            return false;
        }

        graphics_->setClearColor(bestow::Color{20, 25, 35, 255});

        // Initialize input
        if (input_) {
            input_->initialize(graphics_->getNativeWindowHandle());
        }

        // Create meshes
        auto cubeResult = graphics_->createCubeMesh(CELL_SIZE * 0.9f);
        if (cubeResult) {
            cubeMesh_ = *cubeResult;
        }

        auto planeResult = graphics_->createPlaneMesh(
            GRID_SIZE * CELL_SIZE,
            GRID_SIZE * CELL_SIZE,
            GRID_SIZE,
            GRID_SIZE
        );
        if (planeResult) {
            groundMesh_ = *planeResult;
        }

        // Create materials
        snakeMaterial_ = graphics_->getDefaultPBRMaterial();
        groundMaterial_ = graphics_->getDefaultUnlitMaterial();
        foodMaterial_ = graphics_->getDefaultPBRMaterial();

        // Initialize snake
        snake_.clear();
        snake_.push_back({GRID_SIZE / 2, GRID_SIZE / 2});     // Head
        snake_.push_back({GRID_SIZE / 2 - 1, GRID_SIZE / 2}); // Body
        snake_.push_back({GRID_SIZE / 2 - 2, GRID_SIZE / 2}); // Tail

        direction_ = Direction::Right;
        nextDirection_ = Direction::Right;

        // Spawn initial food
        spawnFood();

        // Setup camera (isometric view)
        setupCamera();

        // Setup lighting
        bestow::DirectionalLight light{
            .direction = {0.5f, -1.0f, 0.3f},
            .color = {1.0f, 0.95f, 0.9f},
            .intensity = 1.0f,
            .castShadows = true
        };
        graphics_->setDirectionalLight(light);
        graphics_->setAmbientLight({0.2f, 0.25f, 0.3f}, 0.4f);

        return true;
    }

    //======================================================================
    // Game Loop
    //======================================================================

    void gameLoop() {
        constexpr bestow::DeltaTime fixedDt = 1.0f / 60.0f;
        auto previousTime = std::chrono::high_resolution_clock::now();
        bestow::DeltaTime accumulator = 0.0f;

        running_ = true;

        while (running_ && !graphics_->shouldClose()) {
            // Calculate delta time
            auto currentTime = std::chrono::high_resolution_clock::now();
            bestow::DeltaTime frameTime =
                std::chrono::duration<float>(currentTime - previousTime).count();
            previousTime = currentTime;

            // Clamp to prevent spiral of death
            if (frameTime > 0.25f) {
                frameTime = 0.25f;
            }
            accumulator += frameTime;

            // Update input
            if (input_) {
                input_->update();
            }

            // Update audio
            if (audio_) {
                audio_->update(frameTime);
            }

            // Fixed timestep updates
            while (accumulator >= fixedDt) {
                handleInput();
                moveTimer_ += fixedDt;
                if (moveTimer_ >= MOVE_INTERVAL) {
                    moveTimer_ = 0.0f;
                    moveSnake();
                }
                updateCamera(fixedDt);
                accumulator -= fixedDt;
            }

            // Render
            graphics_->beginFrame();
            drawGround();
            drawSnake();
            drawFood();
            drawGridBorder();
            graphics_->endFrame();
        }
    }

    void cleanup() {
        if (input_) {
            input_->shutdown();
        }
        if (graphics_) {
            graphics_->shutdown();
        }
    }

    //======================================================================
    // Input Handling
    //======================================================================

    void handleInput() {
        if (!input_) return;

        if (gameOver_) {
            // R to restart
            if (input_->wasKeyJustPressed(GLFW_KEY_R)) {
                restartGame();
            }
            return;
        }

        // Dvorak-friendly controls: ,AOE (equivalent to WASD positions on Dvorak)
        // Also support arrow keys for QWERTY users

        // Up: Comma (Dvorak W) or Up Arrow
        if ((input_->wasKeyJustPressed(GLFW_KEY_COMMA) ||
             input_->wasKeyJustPressed(GLFW_KEY_UP)) &&
            direction_ != Direction::Down) {
            nextDirection_ = Direction::Up;
        }
        // Down: O (Dvorak S) or Down Arrow
        if ((input_->wasKeyJustPressed(GLFW_KEY_O) ||
             input_->wasKeyJustPressed(GLFW_KEY_DOWN)) &&
            direction_ != Direction::Up) {
            nextDirection_ = Direction::Down;
        }
        // Left: A or Left Arrow
        if ((input_->wasKeyJustPressed(GLFW_KEY_A) ||
             input_->wasKeyJustPressed(GLFW_KEY_LEFT)) &&
            direction_ != Direction::Right) {
            nextDirection_ = Direction::Left;
        }
        // Right: E (Dvorak D) or Right Arrow
        if ((input_->wasKeyJustPressed(GLFW_KEY_E) ||
             input_->wasKeyJustPressed(GLFW_KEY_RIGHT)) &&
            direction_ != Direction::Left) {
            nextDirection_ = Direction::Right;
        }

        // ESC to quit
        if (input_->wasKeyJustPressed(GLFW_KEY_ESCAPE)) {
            running_ = false;
        }
    }

    //======================================================================
    // Game Logic
    //======================================================================

    void moveSnake() {
        if (gameOver_) return;

        direction_ = nextDirection_;
        GridPos offset = directionToOffset(direction_);
        GridPos newHead = snake_[0] + offset;

        // Wrap around
        if (newHead.x < 0) newHead.x = GRID_SIZE - 1;
        if (newHead.x >= GRID_SIZE) newHead.x = 0;
        if (newHead.z < 0) newHead.z = GRID_SIZE - 1;
        if (newHead.z >= GRID_SIZE) newHead.z = 0;

        // Check self-collision
        for (const auto& segment : snake_) {
            if (newHead == segment) {
                gameOver_ = true;
                return;
            }
        }

        // Insert new head
        snake_.insert(snake_.begin(), newHead);

        // Check food
        if (newHead == food_) {
            score_++;
            spawnFood();
            // Don't remove tail - snake grows
        } else {
            // Remove tail
            snake_.pop_back();
        }
    }

    void spawnFood() {
        // Find a random position not occupied by snake
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, GRID_SIZE - 1);

        do {
            food_ = {dist(gen), dist(gen)};
        } while (isSnakeAt(food_));
    }

    bool isSnakeAt(const GridPos& pos) const {
        for (const auto& segment : snake_) {
            if (segment == pos) return true;
        }
        return false;
    }

    void restartGame() {
        snake_.clear();
        snake_.push_back({GRID_SIZE / 2, GRID_SIZE / 2});
        snake_.push_back({GRID_SIZE / 2 - 1, GRID_SIZE / 2});
        snake_.push_back({GRID_SIZE / 2 - 2, GRID_SIZE / 2});

        direction_ = Direction::Right;
        nextDirection_ = Direction::Right;
        score_ = 0;
        gameOver_ = false;

        spawnFood();
    }

    //======================================================================
    // Camera
    //======================================================================

    void setupCamera() {
        bestow::Camera3D cam;
        cam.fovY = 45.0f;
        cam.nearPlane = 0.1f;
        cam.farPlane = 100.0f;

        updateCameraTransform(cam);
        graphics_->setCamera(cam);
    }

    void updateCamera(float dt) {
        // Smoothly follow snake head
        if (!snake_.empty()) {
            bestow::Vec3 headWorldPos = gridToWorld(snake_[0]);
            cameraTarget_ = cameraTarget_ + (headWorldPos - cameraTarget_) * (5.0f * dt);
        }

        bestow::Camera3D cam = graphics_->getCamera();
        updateCameraTransform(cam);
        graphics_->setCamera(cam);
    }

    void updateCameraTransform(bestow::Camera3D& cam) {
        // Isometric-style camera position
        float angleRad = glm::radians(cameraAngle_);
        bestow::Vec3 cameraPos = {
            cameraTarget_.x + CAMERA_DISTANCE * std::sin(angleRad),
            cameraTarget_.y + CAMERA_HEIGHT,
            cameraTarget_.z + CAMERA_DISTANCE * std::cos(angleRad)
        };

        cam.transform.position = cameraPos;

        // Calculate rotation to look at target
        glm::vec3 lookDir = glm::normalize(glm::vec3(
            cameraTarget_.x - cameraPos.x,
            cameraTarget_.y - cameraPos.y,
            cameraTarget_.z - cameraPos.z
        ));

        glm::vec3 up(0.0f, 1.0f, 0.0f);
        glm::quat rotation = glm::quatLookAt(lookDir, up);
        cam.transform.rotation = {rotation.w, rotation.x, rotation.y, rotation.z};
    }

    //======================================================================
    // Rendering
    //======================================================================

    bestow::Vec3 gridToWorld(const GridPos& pos) const {
        float halfGrid = GRID_SIZE * CELL_SIZE * 0.5f;
        return {
            pos.x * CELL_SIZE - halfGrid + CELL_SIZE * 0.5f,
            CELL_SIZE * 0.5f,  // Half cell above ground
            pos.z * CELL_SIZE - halfGrid + CELL_SIZE * 0.5f
        };
    }

    void drawGround() {
        if (!groundMesh_) return;

        bestow::Mat4 groundMatrix = glm::identity<glm::mat4>();
        graphics_->drawMesh(groundMesh_, groundMaterial_, groundMatrix, false, true);
    }

    void drawSnake() {
        if (!cubeMesh_) return;

        for (std::size_t i = 0; i < snake_.size(); ++i) {
            bestow::Vec3 worldPos = gridToWorld(snake_[i]);

            bestow::Mat4 transform = glm::translate(glm::identity<glm::mat4>(),
                glm::vec3(worldPos.x, worldPos.y, worldPos.z));

            graphics_->drawMesh(cubeMesh_, snakeMaterial_, transform, true, true);
        }
    }

    void drawFood() {
        if (!cubeMesh_) return;

        bestow::Vec3 worldPos = gridToWorld(food_);

        // Pulsing effect
        float pulse = 0.9f + 0.1f * std::sin(moveTimer_ * 10.0f);

        bestow::Mat4 transform = glm::translate(glm::identity<glm::mat4>(),
            glm::vec3(worldPos.x, worldPos.y, worldPos.z));
        transform = glm::scale(transform, glm::vec3(pulse));

        graphics_->drawMesh(cubeMesh_, foodMaterial_, transform, true, true);
    }

    void drawGridBorder() {
        float halfGrid = GRID_SIZE * CELL_SIZE * 0.5f;
        bestow::Color borderColor = bestow::Color{100, 100, 100, 255};

        // Draw border lines
        graphics_->debugDrawLine(
            {-halfGrid, 0.01f, -halfGrid},
            {halfGrid, 0.01f, -halfGrid},
            borderColor, 0.0f, false
        );
        graphics_->debugDrawLine(
            {halfGrid, 0.01f, -halfGrid},
            {halfGrid, 0.01f, halfGrid},
            borderColor, 0.0f, false
        );
        graphics_->debugDrawLine(
            {halfGrid, 0.01f, halfGrid},
            {-halfGrid, 0.01f, halfGrid},
            borderColor, 0.0f, false
        );
        graphics_->debugDrawLine(
            {-halfGrid, 0.01f, halfGrid},
            {-halfGrid, 0.01f, -halfGrid},
            borderColor, 0.0f, false
        );
    }

public:
    //======================================================================
    // Kangaru Service Definition (nested inside class for Engine::run<>)
    //======================================================================
    struct Service : kgr::single_service<SnakeGame> {
        static auto construct(
            kgr::inject_t<bestow::IGraphics3DSystemService> graphics,
            kgr::inject_t<bestow::IInputSystemService> input,
            kgr::inject_t<bestow::IAudioSystemService> audio)
            -> kgr::inject_result<
                bestow::IGraphics3DSystem*,
                bestow::IInputSystem*,
                bestow::IAudioSystem*>
        {
            return kgr::inject(
                &graphics.service(),
                &input.service(),
                &audio.service()
            );
        }
    };
};

}  // namespace snake
