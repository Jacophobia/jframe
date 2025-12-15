// games/game1/src/snake.game.cppm
// 3D Isometric Snake Game
//
// A simple snake game demonstrating the bestow::Game API.
// Uses 3D isometric view with camera following the snake head.
// Controls: ,AOE (Dvorak) or Arrow Keys for movement

module;

#include <GLFW/glfw3.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

export module snake.game;

import std;
import bestow.runtime;
import bestow.types;
import bestow.graphics3d;  // For DirectionalLight

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
// Snake Game Class
//==========================================================================

class SnakeGame : public bestow::Game {
public:
    void onStart() override {
        // Create meshes
        auto cubeResult = graphics3d()->createCubeMesh(CELL_SIZE * 0.9f);
        if (cubeResult) {
            cubeMesh_ = *cubeResult;
        }

        auto planeResult = graphics3d()->createPlaneMesh(
            GRID_SIZE * CELL_SIZE,
            GRID_SIZE * CELL_SIZE,
            GRID_SIZE,
            GRID_SIZE
        );
        if (planeResult) {
            groundMesh_ = *planeResult;
        }

        // Create materials
        snakeMaterial_ = graphics3d()->getDefaultPBRMaterial();
        groundMaterial_ = graphics3d()->getDefaultUnlitMaterial();
        foodMaterial_ = graphics3d()->getDefaultPBRMaterial();

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
        graphics3d()->setDirectionalLight(light);
        graphics3d()->setAmbientLight({0.2f, 0.25f, 0.3f}, 0.4f);
    }

    void onUpdate(bestow::DeltaTime dt) override {
        handleInput();

        moveTimer_ += dt;
        if (moveTimer_ >= MOVE_INTERVAL) {
            moveTimer_ = 0.0f;
            moveSnake();
        }

        updateCamera(dt);
    }

    void onRender() override {
        // Draw ground
        drawGround();

        // Draw snake
        drawSnake();

        // Draw food
        drawFood();

        // Draw grid border (debug lines)
        drawGridBorder();
    }

private:
    // Meshes
    bestow::MeshHandle cubeMesh_ = 0;
    bestow::MeshHandle groundMesh_ = 0;

    // Materials
    bestow::MaterialHandle snakeMaterial_ = 0;
    bestow::MaterialHandle groundMaterial_ = 0;
    bestow::MaterialHandle foodMaterial_ = 0;

    // Game state
    std::vector<GridPos> snake_;
    GridPos food_;
    Direction direction_ = Direction::Right;
    Direction nextDirection_ = Direction::Right;
    float moveTimer_ = 0.0f;
    int score_ = 0;
    bool gameOver_ = false;

    // Camera
    bestow::Vec3 cameraTarget_{0.0f};
    float cameraAngle_ = 45.0f;  // Degrees around Y axis

    void handleInput() {
        if (gameOver_) {
            // R to restart
            if (input()->wasKeyJustPressed(GLFW_KEY_R)) {
                restartGame();
            }
            return;
        }

        // Dvorak-friendly controls: ,AOE (equivalent to WASD positions on Dvorak)
        // Also support arrow keys for QWERTY users

        // Up: Comma (Dvorak W) or Up Arrow
        if ((input()->wasKeyJustPressed(GLFW_KEY_COMMA) ||
             input()->wasKeyJustPressed(GLFW_KEY_UP)) &&
            direction_ != Direction::Down) {
            nextDirection_ = Direction::Up;
        }
        // Down: O (Dvorak S) or Down Arrow
        if ((input()->wasKeyJustPressed(GLFW_KEY_O) ||
             input()->wasKeyJustPressed(GLFW_KEY_DOWN)) &&
            direction_ != Direction::Up) {
            nextDirection_ = Direction::Down;
        }
        // Left: A or Left Arrow
        if ((input()->wasKeyJustPressed(GLFW_KEY_A) ||
             input()->wasKeyJustPressed(GLFW_KEY_LEFT)) &&
            direction_ != Direction::Right) {
            nextDirection_ = Direction::Left;
        }
        // Right: E (Dvorak D) or Right Arrow
        if ((input()->wasKeyJustPressed(GLFW_KEY_E) ||
             input()->wasKeyJustPressed(GLFW_KEY_RIGHT)) &&
            direction_ != Direction::Left) {
            nextDirection_ = Direction::Right;
        }

        // ESC to quit
        if (input()->wasKeyJustPressed(GLFW_KEY_ESCAPE)) {
            quit();
        }
    }

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

    void setupCamera() {
        bestow::Camera3D cam;
        cam.fovY = 45.0f;
        cam.nearPlane = 0.1f;
        cam.farPlane = 100.0f;

        updateCameraTransform(cam);
        graphics3d()->setCamera(cam);
    }

    void updateCamera(float dt) {
        // Smoothly follow snake head
        if (!snake_.empty()) {
            bestow::Vec3 headWorldPos = gridToWorld(snake_[0]);
            cameraTarget_ = cameraTarget_ + (headWorldPos - cameraTarget_) * (5.0f * dt);
        }

        bestow::Camera3D cam = graphics3d()->getCamera();
        updateCameraTransform(cam);
        graphics3d()->setCamera(cam);
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
        // lookAt direction: target - position
        glm::vec3 lookDir = glm::normalize(glm::vec3(
            cameraTarget_.x - cameraPos.x,
            cameraTarget_.y - cameraPos.y,
            cameraTarget_.z - cameraPos.z
        ));

        // Convert look direction to quaternion
        // Using glm::quatLookAt which creates a rotation from -Z axis to lookDir
        glm::vec3 up(0.0f, 1.0f, 0.0f);
        glm::quat rotation = glm::quatLookAt(lookDir, up);
        cam.transform.rotation = {rotation.w, rotation.x, rotation.y, rotation.z};
    }

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
        // Ground plane is at Y=0, already centered at origin
        graphics3d()->drawMesh(groundMesh_, groundMaterial_, groundMatrix, false, true);
    }

    void drawSnake() {
        if (!cubeMesh_) return;

        for (std::size_t i = 0; i < snake_.size(); ++i) {
            bestow::Vec3 worldPos = gridToWorld(snake_[i]);

            // Head is brighter
            bestow::Vec4 color = (i == 0)
                ? bestow::Vec4{0.2f, 0.8f, 0.3f, 1.0f}   // Bright green head
                : bestow::Vec4{0.1f, 0.6f, 0.2f, 1.0f}; // Darker green body

            bestow::Mat4 transform = glm::translate(glm::identity<glm::mat4>(),
                glm::vec3(worldPos.x, worldPos.y, worldPos.z));

            graphics3d()->drawMesh(cubeMesh_, snakeMaterial_, transform, true, true);
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

        graphics3d()->drawMesh(cubeMesh_, foodMaterial_, transform, true, true);
    }

    void drawGridBorder() {
        float halfGrid = GRID_SIZE * CELL_SIZE * 0.5f;
        bestow::Color borderColor = bestow::Color{100, 100, 100, 255};

        // Draw border lines
        graphics3d()->debugDrawLine(
            {-halfGrid, 0.01f, -halfGrid},
            {halfGrid, 0.01f, -halfGrid},
            borderColor, 0.0f, false
        );
        graphics3d()->debugDrawLine(
            {halfGrid, 0.01f, -halfGrid},
            {halfGrid, 0.01f, halfGrid},
            borderColor, 0.0f, false
        );
        graphics3d()->debugDrawLine(
            {halfGrid, 0.01f, halfGrid},
            {-halfGrid, 0.01f, halfGrid},
            borderColor, 0.0f, false
        );
        graphics3d()->debugDrawLine(
            {-halfGrid, 0.01f, halfGrid},
            {-halfGrid, 0.01f, -halfGrid},
            borderColor, 0.0f, false
        );

        // Game over message (via debug text would be nice, but we don't have that)
        // For now, the screen just stops updating
    }
};

}  // namespace snake
