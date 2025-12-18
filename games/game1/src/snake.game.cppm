// games/game1/src/snake.game.cppm
// 3D Isometric Snake Game
//
// A colorful snake game where each segment inherits the color of the food eaten.
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
import bestow.services;   // All contract interfaces
import bestow.types;
import bestow.graphics3d;

export namespace snake {

//==========================================================================
// Game Constants
//==========================================================================

constexpr int INITIAL_GRID_SIZE = 10;     // Start small (10x10)
constexpr float CELL_SIZE = 1.0f;          // Each cell is 1 unit
constexpr float INITIAL_MOVE_INTERVAL = 0.12f;  // Starting speed
constexpr float MIN_MOVE_INTERVAL = 0.04f;      // Maximum speed (minimum interval)
constexpr float SPEED_INCREASE = 0.003f;        // Speed up per food eaten
constexpr float BASE_CAMERA_DISTANCE = 12.0f;   // Base camera distance
constexpr float BASE_CAMERA_HEIGHT = 10.0f;     // Base camera height
constexpr float CAMERA_SCALE = 0.3f;            // Camera scales with grid size (very gentle zoom)
constexpr float ANIMATION_SMOOTH = 3.0f;        // Animation smoothing factor
constexpr float FOOD_SPIN_SPEED = 2.0f;         // Food rotation speed (radians/sec)
constexpr float OBSTACLE_SPAWN_CHANCE = 0.4f;   // 40% chance to spawn obstacle when eating
constexpr float EXPANSION_WAIT_TIME = 1.0f;     // Wait 1 second before merging borders

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
// Snake Segment - stores position and color
//==========================================================================

struct SnakeSegment {
    GridPos pos;
    bestow::Vec4 color{0.2f, 0.8f, 0.3f, 1.0f};  // Default green
};

//==========================================================================
// Snake Game Application
//==========================================================================

class SnakeGame : public bestow::Application<SnakeGame,
    bestow::IGraphics3DSystem,
    bestow::IInputSystem,
    bestow::IAudioSystem>
{
public:
    SnakeGame(bestow::IGraphics3DSystem& graphics,
              bestow::IInputSystem& input,
              bestow::IAudioSystem& audio)
        : graphics_(&graphics)
        , input_(&input)
        , audio_(&audio) {}

    ~SnakeGame() override = default;

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
    // Dependencies
    //======================================================================
    bestow::IGraphics3DSystem* graphics_ = nullptr;
    bestow::IInputSystem* input_ = nullptr;
    bestow::IAudioSystem* audio_ = nullptr;

    //======================================================================
    // Meshes & Materials
    //======================================================================
    bestow::MeshHandle cubeMesh_ = 0;
    bestow::MeshHandle groundMesh_ = 0;
    bestow::MaterialHandle groundMaterial_ = 0;

    //======================================================================
    // Game State
    //======================================================================
    std::vector<SnakeSegment> snake_;
    std::vector<GridPos> obstacles_;  // Obstacles as grid positions (move with grid)
    GridPos foodPos_;
    bestow::Vec4 foodColor_{1.0f, 0.0f, 0.0f, 1.0f};
    Direction direction_ = Direction::Right;
    Direction nextDirection_ = Direction::Right;
    Direction inputDirection_ = Direction::Right;  // Buffered input
    bool hasBufferedInput_ = false;
    float moveTimer_ = 0.0f;
    float moveInterval_ = INITIAL_MOVE_INTERVAL;  // Current speed (decreases as you eat)
    int score_ = 0;
    int gridSize_ = INITIAL_GRID_SIZE;  // Dynamic grid size - grows when eating!
    bool gameOver_ = false;
    bool running_ = false;

    // Animation state - for smooth transitions
    float visualGridSize_ = static_cast<float>(INITIAL_GRID_SIZE);  // Smoothly animated
    float currentCameraDistance_ = BASE_CAMERA_DISTANCE;
    float currentCameraHeight_ = BASE_CAMERA_HEIGHT;
    float gameTime_ = 0.0f;  // For spinning food

    // Expansion animation state
    bool isExpanding_ = false;           // Are we in expansion animation?
    float expansionTimer_ = 0.0f;        // Timer for expansion animation
    int targetGridSize_ = INITIAL_GRID_SIZE;  // Grid size we're expanding to

    // Random number generator
    std::mt19937 rng_{std::random_device{}()};

    //======================================================================
    // Camera
    //======================================================================
    bestow::Vec3 cameraTarget_{0.0f};
    float cameraAngle_ = 45.0f;

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

        // Green background!
        graphics_->setClearColor(bestow::Color{30, 120, 50, 255});

        // Initialize input
        if (input_) {
            input_->initialize(graphics_->getNativeWindowHandle());
        }

        // Create cube mesh
        auto cubeResult = graphics_->createCubeMesh(CELL_SIZE * 0.85f);
        if (cubeResult) {
            cubeMesh_ = *cubeResult;
        }

        // Create ground mesh for initial size
        rebuildGroundMesh();

        // Create ground material (dark green)
        bestow::PBRMaterial groundMat;
        groundMat.baseColorFactor = {0.15f, 0.4f, 0.2f, 1.0f};
        groundMat.roughnessFactor = 0.9f;
        groundMat.metallicFactor = 0.0f;
        auto groundResult = graphics_->createMaterial(groundMat);
        if (groundResult) {
            groundMaterial_ = *groundResult;
        }

        // Initialize snake with gradient green colors
        snake_.clear();
        snake_.push_back({{gridSize_ / 2, gridSize_ / 2}, {0.2f, 0.9f, 0.3f, 1.0f}});
        snake_.push_back({{gridSize_ / 2 - 1, gridSize_ / 2}, {0.25f, 0.85f, 0.35f, 1.0f}});
        snake_.push_back({{gridSize_ / 2 - 2, gridSize_ / 2}, {0.3f, 0.8f, 0.4f, 1.0f}});

        direction_ = Direction::Right;
        nextDirection_ = Direction::Right;

        // Spawn initial food
        spawnFood();

        // Setup camera
        setupCamera();

        // Setup lighting
        bestow::DirectionalLight light{
            .direction = {0.5f, -1.0f, 0.3f},
            .color = {1.0f, 0.98f, 0.95f},
            .intensity = 1.2f,
            .castShadows = true
        };
        graphics_->setDirectionalLight(light);
        graphics_->setAmbientLight({0.3f, 0.35f, 0.3f}, 0.5f);

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
            auto currentTime = std::chrono::high_resolution_clock::now();
            bestow::DeltaTime frameTime =
                std::chrono::duration<float>(currentTime - previousTime).count();
            previousTime = currentTime;

            if (frameTime > 0.25f) {
                frameTime = 0.25f;
            }
            accumulator += frameTime;

            // Update input ONCE per frame
            if (input_) {
                input_->update();
            }

            // Handle input ONCE per frame (before fixed timestep loop)
            handleInput();

            // Update audio
            if (audio_) {
                audio_->update(frameTime);
            }

            // Fixed timestep updates
            while (accumulator >= fixedDt) {
                gameTime_ += fixedDt;
                moveTimer_ += fixedDt;
                if (moveTimer_ >= moveInterval_) {
                    moveTimer_ = 0.0f;
                    applyBufferedInput();
                    moveSnake();
                }
                updateAnimations(fixedDt);
                updateCamera(fixedDt);
                accumulator -= fixedDt;
            }

            // Render
            graphics_->beginFrame();
            drawGround();
            drawObstacles();
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
    // Ground Mesh Management
    //======================================================================

    void rebuildGroundMesh() {
        // Build mesh at max expected size for smooth scaling
        int meshSize = gridSize_ + 10;  // Extra room for smooth growth
        auto planeResult = graphics_->createPlaneMesh(
            meshSize * CELL_SIZE,
            meshSize * CELL_SIZE,
            meshSize,
            meshSize
        );
        if (planeResult) {
            groundMesh_ = *planeResult;
        }
    }

    //======================================================================
    // Animation Updates
    //======================================================================

    void startExpansion() {
        isExpanding_ = true;
        expansionTimer_ = 0.0f;
        targetGridSize_ = gridSize_ + 2;  // Expand by 2 (one cell on each side)
        // New border snaps into existence immediately
    }

    void finalizeExpansion() {
        // Shift all existing positions by +1 to center the expansion
        // This makes the grid expand in ALL directions (not just positive side)
        for (auto& segment : snake_) {
            segment.pos.x += 1;
            segment.pos.z += 1;
        }
        for (auto& obstacle : obstacles_) {
            obstacle.x += 1;
            obstacle.z += 1;
        }
        foodPos_.x += 1;
        foodPos_.z += 1;

        gridSize_ = targetGridSize_;
        rebuildGroundMesh();
    }

    void updateAnimations(float dt) {
        // Handle expansion animation
        if (isExpanding_) {
            expansionTimer_ += dt;

            // After wait time, INSTANTLY merge (no smooth transition)
            if (expansionTimer_ >= EXPANSION_WAIT_TIME) {
                isExpanding_ = false;
                finalizeExpansion();
                // Camera will now start zooming out (see below)
            }
            // During wait: the NEW border is drawn separately in drawGridBorder
        }

        // Camera smoothly zooms - but only targets NEW size AFTER expansion completes
        int cameraTargetSize = isExpanding_ ? gridSize_ : gridSize_;
        float targetDistance = BASE_CAMERA_DISTANCE + (cameraTargetSize - INITIAL_GRID_SIZE) * CAMERA_SCALE;
        float targetHeight = BASE_CAMERA_HEIGHT + (cameraTargetSize - INITIAL_GRID_SIZE) * CAMERA_SCALE * 0.8f;

        currentCameraDistance_ += (targetDistance - currentCameraDistance_) * ANIMATION_SMOOTH * dt;
        currentCameraHeight_ += (targetHeight - currentCameraHeight_) * ANIMATION_SMOOTH * dt;
    }

    //======================================================================
    // Input Handling - now buffered and responsive
    //======================================================================

    void handleInput() {
        if (!input_) return;

        if (gameOver_) {
            if (input_->wasKeyJustPressed(GLFW_KEY_R)) {
                restartGame();
            }
            if (input_->wasKeyJustPressed(GLFW_KEY_ESCAPE)) {
                running_ = false;
            }
            return;
        }

        // Buffer direction changes - only accept valid turns
        // Up: Comma (Dvorak W) or Up Arrow
        if (input_->wasKeyJustPressed(GLFW_KEY_COMMA) ||
            input_->wasKeyJustPressed(GLFW_KEY_UP)) {
            if (direction_ != Direction::Down) {
                inputDirection_ = Direction::Up;
                hasBufferedInput_ = true;
            }
        }
        // Down: O (Dvorak S) or Down Arrow
        if (input_->wasKeyJustPressed(GLFW_KEY_O) ||
            input_->wasKeyJustPressed(GLFW_KEY_DOWN)) {
            if (direction_ != Direction::Up) {
                inputDirection_ = Direction::Down;
                hasBufferedInput_ = true;
            }
        }
        // Left: A or Left Arrow
        if (input_->wasKeyJustPressed(GLFW_KEY_A) ||
            input_->wasKeyJustPressed(GLFW_KEY_LEFT)) {
            if (direction_ != Direction::Right) {
                inputDirection_ = Direction::Left;
                hasBufferedInput_ = true;
            }
        }
        // Right: E (Dvorak D) or Right Arrow
        if (input_->wasKeyJustPressed(GLFW_KEY_E) ||
            input_->wasKeyJustPressed(GLFW_KEY_RIGHT)) {
            if (direction_ != Direction::Left) {
                inputDirection_ = Direction::Right;
                hasBufferedInput_ = true;
            }
        }

        // ESC to quit
        if (input_->wasKeyJustPressed(GLFW_KEY_ESCAPE)) {
            running_ = false;
        }
    }

    void applyBufferedInput() {
        if (hasBufferedInput_) {
            nextDirection_ = inputDirection_;
            hasBufferedInput_ = false;
        }
    }

    //======================================================================
    // Game Logic
    //======================================================================

    void moveSnake() {
        if (gameOver_) return;

        direction_ = nextDirection_;
        GridPos offset = directionToOffset(direction_);
        GridPos newHead = snake_[0].pos + offset;

        // Wrap around (using dynamic grid size)
        if (newHead.x < 0) newHead.x = gridSize_ - 1;
        if (newHead.x >= gridSize_) newHead.x = 0;
        if (newHead.z < 0) newHead.z = gridSize_ - 1;
        if (newHead.z >= gridSize_) newHead.z = 0;

        // Check self-collision
        for (const auto& segment : snake_) {
            if (newHead == segment.pos) {
                gameOver_ = true;
                return;
            }
        }

        // Check obstacle collision
        if (isObstacleAt(newHead)) {
            gameOver_ = true;
            return;
        }

        // Check food
        bool ateFood = (newHead == foodPos_);

        // Insert new head with appropriate color
        SnakeSegment newSegment;
        newSegment.pos = newHead;
        if (ateFood) {
            // New head gets the food's color!
            newSegment.color = foodColor_;
            score_++;

            // Start expansion animation instead of immediate expansion
            startExpansion();

            // Maybe spawn an obstacle for extra challenge
            std::uniform_real_distribution<float> chanceDist(0.0f, 1.0f);
            if (chanceDist(rng_) < OBSTACLE_SPAWN_CHANCE) {
                spawnObstacle();
            }

            spawnFood();
            // Speed up! (decrease interval, but not below minimum)
            moveInterval_ = std::max(MIN_MOVE_INTERVAL, moveInterval_ - SPEED_INCREASE);
        } else {
            // Inherit color from previous head (slight fade)
            newSegment.color = snake_[0].color;
        }
        snake_.insert(snake_.begin(), newSegment);

        if (!ateFood) {
            snake_.pop_back();
        }
    }

    void spawnFood() {
        std::uniform_int_distribution<> posDist(0, gridSize_ - 1);

        do {
            foodPos_ = {posDist(rng_), posDist(rng_)};
        } while (isSnakeAt(foodPos_) || isObstacleAt(foodPos_));

        // Generate vibrant random color for food
        foodColor_ = generateRandomColor();
    }

    bestow::Vec4 generateRandomColor() {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        // Generate vibrant colors by ensuring at least one channel is high
        float r = dist(rng_);
        float g = dist(rng_);
        float b = dist(rng_);

        // Boost saturation - make colors more vivid
        float maxVal = std::max({r, g, b});
        if (maxVal > 0.01f) {
            float boost = 1.0f / maxVal;
            r = std::min(1.0f, r * boost * 0.8f + 0.2f);
            g = std::min(1.0f, g * boost * 0.8f + 0.2f);
            b = std::min(1.0f, b * boost * 0.8f + 0.2f);
        }

        return {r, g, b, 1.0f};
    }

    bool isSnakeAt(const GridPos& pos) const {
        for (const auto& segment : snake_) {
            if (segment.pos == pos) return true;
        }
        return false;
    }

    bool isObstacleAt(const GridPos& pos) const {
        for (const auto& obstacle : obstacles_) {
            if (obstacle == pos) return true;
        }
        return false;
    }

    void spawnObstacle() {
        // Only spawn on outer rim of the grid
        std::uniform_int_distribution<> edgeDist(0, gridSize_ - 1);
        std::uniform_int_distribution<> sideDist(0, 3);  // Which edge: 0=top, 1=bottom, 2=left, 3=right

        GridPos gridPos;
        int attempts = 0;
        do {
            int side = sideDist(rng_);
            int pos = edgeDist(rng_);

            switch (side) {
                case 0: gridPos = {pos, 0}; break;              // Top edge
                case 1: gridPos = {pos, gridSize_ - 1}; break;  // Bottom edge
                case 2: gridPos = {0, pos}; break;              // Left edge
                case 3: gridPos = {gridSize_ - 1, pos}; break;  // Right edge
            }

            attempts++;
            // Don't spawn on snake, other obstacles, or food
        } while (attempts < 50 && (isSnakeAt(gridPos) || isObstacleAt(gridPos) ||
                 gridPos == foodPos_));

        if (attempts < 50) {
            obstacles_.push_back(gridPos);
        }
    }

    void restartGame() {
        // Reset grid size
        gridSize_ = INITIAL_GRID_SIZE;
        targetGridSize_ = INITIAL_GRID_SIZE;
        rebuildGroundMesh();

        // Reset animation state
        visualGridSize_ = static_cast<float>(INITIAL_GRID_SIZE);
        currentCameraDistance_ = BASE_CAMERA_DISTANCE;
        currentCameraHeight_ = BASE_CAMERA_HEIGHT;
        isExpanding_ = false;
        expansionTimer_ = 0.0f;

        // Clear obstacles
        obstacles_.clear();

        snake_.clear();
        snake_.push_back({{gridSize_ / 2, gridSize_ / 2}, {0.2f, 0.9f, 0.3f, 1.0f}});
        snake_.push_back({{gridSize_ / 2 - 1, gridSize_ / 2}, {0.25f, 0.85f, 0.35f, 1.0f}});
        snake_.push_back({{gridSize_ / 2 - 2, gridSize_ / 2}, {0.3f, 0.8f, 0.4f, 1.0f}});

        direction_ = Direction::Right;
        nextDirection_ = Direction::Right;
        inputDirection_ = Direction::Right;
        hasBufferedInput_ = false;
        moveInterval_ = INITIAL_MOVE_INTERVAL;  // Reset speed
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
        if (!snake_.empty()) {
            bestow::Vec3 headWorldPos = gridToWorld(snake_[0].pos);
            cameraTarget_ = cameraTarget_ + (headWorldPos - cameraTarget_) * (5.0f * dt);
        }

        bestow::Camera3D cam = graphics_->getCamera();
        updateCameraTransform(cam);
        graphics_->setCamera(cam);
    }

    void updateCameraTransform(bestow::Camera3D& cam) {
        // Use smoothly animated camera distance/height
        float angleRad = glm::radians(cameraAngle_);
        bestow::Vec3 cameraPos = {
            cameraTarget_.x + currentCameraDistance_ * std::sin(angleRad),
            cameraTarget_.y + currentCameraHeight_,
            cameraTarget_.z + currentCameraDistance_ * std::cos(angleRad)
        };

        cam.transform.position = cameraPos;

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
        float halfGrid = gridSize_ * CELL_SIZE * 0.5f;
        return {
            pos.x * CELL_SIZE - halfGrid + CELL_SIZE * 0.5f,
            CELL_SIZE * 0.5f,
            pos.z * CELL_SIZE - halfGrid + CELL_SIZE * 0.5f
        };
    }

    void drawGround() {
        if (!groundMesh_ || !groundMaterial_) return;

        // Ground matches current grid size exactly (instant transition)
        float scale = static_cast<float>(gridSize_) / static_cast<float>(gridSize_ + 10);
        bestow::Mat4 groundMatrix = glm::scale(glm::identity<glm::mat4>(), glm::vec3(scale, 1.0f, scale));
        graphics_->drawMesh(groundMesh_, groundMaterial_, groundMatrix, false, true);
    }

    void drawSnake() {
        if (!cubeMesh_) return;

        for (std::size_t i = 0; i < snake_.size(); ++i) {
            const auto& segment = snake_[i];
            bestow::Vec3 worldPos = gridToWorld(segment.pos);

            // Slight size variation - head is bigger
            float scale = (i == 0) ? 1.0f : 0.9f;

            bestow::Mat4 transform = glm::translate(glm::identity<glm::mat4>(),
                glm::vec3(worldPos.x, worldPos.y, worldPos.z));
            transform = glm::scale(transform, glm::vec3(scale));

            // Create material with segment's color
            bestow::PBRMaterial mat;
            mat.baseColorFactor = segment.color;
            mat.roughnessFactor = 0.4f;
            mat.metallicFactor = 0.1f;

            auto matResult = graphics_->createMaterial(mat);
            if (matResult) {
                graphics_->drawMesh(cubeMesh_, *matResult, transform, true, true);
                // Note: In a real game, we'd cache these materials to avoid creating every frame
            }
        }
    }

    void drawObstacles() {
        if (!cubeMesh_) return;

        // Dark gray/brown obstacle color
        bestow::PBRMaterial obstacleMat;
        obstacleMat.baseColorFactor = {0.3f, 0.25f, 0.2f, 1.0f};
        obstacleMat.roughnessFactor = 0.8f;
        obstacleMat.metallicFactor = 0.1f;

        auto matResult = graphics_->createMaterial(obstacleMat);
        if (!matResult) return;

        for (const auto& obstacle : obstacles_) {
            bestow::Vec3 worldPos = gridToWorld(obstacle);

            // Obstacles are flat and wide rectangles
            bestow::Mat4 transform = glm::translate(glm::identity<glm::mat4>(),
                glm::vec3(worldPos.x, worldPos.y * 0.5f, worldPos.z));
            transform = glm::scale(transform, glm::vec3(0.9f, 0.4f, 0.9f));

            graphics_->drawMesh(cubeMesh_, *matResult, transform, true, true);
        }
    }

    void drawFood() {
        if (!cubeMesh_) return;

        bestow::Vec3 worldPos = gridToWorld(foodPos_);

        // Pulsing effect
        float pulse = 0.85f + 0.15f * std::sin(gameTime_ * 6.0f);
        float bounce = 0.15f * std::abs(std::sin(gameTime_ * 4.0f));

        // Build transform: scale -> tilt -> spin -> translate
        // GLM applies right-to-left, so we write in reverse order
        float spinAngle = gameTime_ * FOOD_SPIN_SPEED;

        bestow::Mat4 transform = glm::identity<glm::mat4>();
        // 1. Translate to final position
        transform = glm::translate(transform, glm::vec3(worldPos.x, worldPos.y + bounce, worldPos.z));
        // 2. Spin around Y axis (world up) - no tilt, just flat spin
        transform = glm::rotate(transform, spinAngle, glm::vec3(0.0f, 1.0f, 0.0f));
        // 3. Scale with pulse
        transform = glm::scale(transform, glm::vec3(pulse));

        // Create material with food's color
        bestow::PBRMaterial mat;
        mat.baseColorFactor = foodColor_;
        mat.roughnessFactor = 0.2f;
        mat.metallicFactor = 0.3f;
        // Make food slightly emissive/bright
        mat.emissiveFactor = {foodColor_.x * 0.3f, foodColor_.y * 0.3f, foodColor_.z * 0.3f};

        auto matResult = graphics_->createMaterial(mat);
        if (matResult) {
            graphics_->drawMesh(cubeMesh_, *matResult, transform, true, true);
        }
    }

    void drawGridBorder() {
        // Current playing field border
        float halfGrid = gridSize_ * CELL_SIZE * 0.5f;
        bestow::Color borderColor{80, 60, 40, 255};

        graphics_->debugDrawLine(
            {-halfGrid, 0.02f, -halfGrid},
            {halfGrid, 0.02f, -halfGrid},
            borderColor, 0.0f, false
        );
        graphics_->debugDrawLine(
            {halfGrid, 0.02f, -halfGrid},
            {halfGrid, 0.02f, halfGrid},
            borderColor, 0.0f, false
        );
        graphics_->debugDrawLine(
            {halfGrid, 0.02f, halfGrid},
            {-halfGrid, 0.02f, halfGrid},
            borderColor, 0.0f, false
        );
        graphics_->debugDrawLine(
            {-halfGrid, 0.02f, halfGrid},
            {-halfGrid, 0.02f, -halfGrid},
            borderColor, 0.0f, false
        );

        // During expansion: draw the NEW outer border (glowing, waiting to merge)
        if (isExpanding_) {
            float newHalfGrid = targetGridSize_ * CELL_SIZE * 0.5f;

            // Pulsing glow effect
            float pulse = std::sin(expansionTimer_ * 15.0f) * 0.5f + 0.5f;
            uint8_t brightness = static_cast<uint8_t>(180 + pulse * 75);
            bestow::Color glowColor{brightness, brightness, static_cast<uint8_t>(brightness * 0.6f), 255};

            graphics_->debugDrawLine(
                {-newHalfGrid, 0.03f, -newHalfGrid},
                {newHalfGrid, 0.03f, -newHalfGrid},
                glowColor, 0.0f, false
            );
            graphics_->debugDrawLine(
                {newHalfGrid, 0.03f, -newHalfGrid},
                {newHalfGrid, 0.03f, newHalfGrid},
                glowColor, 0.0f, false
            );
            graphics_->debugDrawLine(
                {newHalfGrid, 0.03f, newHalfGrid},
                {-newHalfGrid, 0.03f, newHalfGrid},
                glowColor, 0.0f, false
            );
            graphics_->debugDrawLine(
                {-newHalfGrid, 0.03f, newHalfGrid},
                {-newHalfGrid, 0.03f, -newHalfGrid},
                glowColor, 0.0f, false
            );
        }
    }
};

}  // namespace snake
