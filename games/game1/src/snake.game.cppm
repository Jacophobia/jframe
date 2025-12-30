// games/game1/src/snake.game.cppm
// 3D Isometric Snake Game
//
// A level-based snake game with predefined maps, roaming enemies, and ring attacks.
// Uses 3D isometric view with camera following the snake head.
// Controls: ,AOE (Dvorak) or Arrow Keys for movement

module;

#include <GLFW/glfw3.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

export module snake.game;

// Import the types partition
export import :types;

import std;
import bestow.services;   // All contract interfaces
import bestow.types;
import bestow.lua;        // ILuaRuntime
import bestow.graphics3d;

export namespace snake {

//==========================================================================
// Snake Game Application
//==========================================================================

class SnakeGame : public bestow::Application<SnakeGame,
    bestow::IGraphics3DSystem,
    bestow::IInputSystem,
    bestow::IAudioSystem,
    bestow::ILuaRuntime,
    bestow::IAssetSystem>
{
public:
    SnakeGame(bestow::IGraphics3DSystem& graphics,
              bestow::IInputSystem& input,
              bestow::IAudioSystem& audio,
              bestow::ILuaRuntime& lua,
              bestow::IAssetSystem& assets)
        : graphics_(&graphics)
        , input_(&input)
        , audio_(&audio)
        , lua_(&lua)
        , assets_(&assets) {}

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
    bestow::ILuaRuntime* lua_ = nullptr;
    bestow::IAssetSystem* assets_ = nullptr;

    //======================================================================
    // Meshes & Materials
    //======================================================================
    bestow::MeshHandle cubeMesh_ = 0;
    bestow::MeshHandle groundMesh_ = 0;
    bestow::MaterialHandle groundMaterial_ = 0;

    //======================================================================
    // Font for Text Rendering
    //======================================================================
    bestow::Font3DHandle gameFont_ = 0;
    bestow::Font3DHandle titleFont_ = 0;

    //======================================================================
    // Game Phase / State Machine
    //======================================================================
    GamePhase currentPhase_ = GamePhase::Playing;  // TODO: Start with MainMenu later
    GamePhase previousPhase_ = GamePhase::Playing;

    //======================================================================
    // Level State
    //======================================================================
    LevelMap currentLevel_;
    int currentLevelIndex_ = 0;
    int currentWorldIndex_ = 0;
    int foodCollected_ = 0;
    int foodRequired_ = 5;
    int levelMaxSize_ = 15;      // Full level dimensions
    int levelOffsetX_ = 0;       // Offset from current grid to level coords
    int levelOffsetZ_ = 0;

    //======================================================================
    // Enemy State
    //======================================================================
    std::vector<Enemy> enemies_;
    std::vector<DetachedSegment> detachedSegments_;
    std::vector<FoodPickup> foodPickups_;

    //======================================================================
    // World/Progress State
    //======================================================================
    WorldConfig currentWorld_;
    GameSaveData saveData_;
    int segmentsEarnedThisLevel_ = 0;
    int totalSegmentsInWorld_ = 0;  // Accumulated for boss fight

    //======================================================================
    // World Map State
    //======================================================================
    int selectedNodeIndex_ = 0;
    float worldMapCursorBob_ = 0.0f;  // Animation for cursor bobbing
    bestow::Vec3 worldMapCameraTarget_{0.0f, 0.0f, 0.0f};
    float worldMapCameraDistance_ = 15.0f;

    //======================================================================
    // Menu State
    //======================================================================
    int mainMenuSelection_ = 0;
    int pauseMenuSelection_ = 0;
    float menuAnimTime_ = 0.0f;
    static constexpr int MAIN_MENU_PLAY = 0;
    static constexpr int MAIN_MENU_QUIT = 1;
    static constexpr int MAIN_MENU_COUNT = 2;
    static constexpr int PAUSE_MENU_RESUME = 0;
    static constexpr int PAUSE_MENU_RESTART = 1;
    static constexpr int PAUSE_MENU_QUIT = 2;
    static constexpr int PAUSE_MENU_COUNT = 3;

    //======================================================================
    // Audio State
    //======================================================================
    bestow::AssetHandle soundEat_{};
    bestow::AssetHandle soundDeath_{};
    bestow::AssetHandle soundLevelComplete_{};
    bestow::AssetHandle soundMenuSelect_{};
    bestow::AssetHandle soundMenuMove_{};
    bestow::AssetHandle soundEnemyHit_{};
    bestow::AssetHandle soundChainBreak_{};
    bestow::AssetHandle soundPause_{};
    bestow::AssetHandle soundGameOver_{};
    bestow::AssetHandle musicGame_{};
    bestow::AssetHandle musicMenu_{};
    bool soundsLoaded_ = false;

    //======================================================================
    // Snake State
    //======================================================================
    std::vector<SnakeSegment> snake_;
    std::vector<GridPos> obstacles_;  // Legacy obstacles (will be replaced by level walls)
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
    // Screen Shake & Visual Effects
    //======================================================================
    float screenShakeIntensity_ = 0.0f;
    float screenShakeTimer_ = 0.0f;
    float screenShakeDuration_ = 0.0f;
    bestow::Vec3 screenShakeOffset_{0.0f, 0.0f, 0.0f};

    // Food collection pop effect
    float foodPopScale_ = 0.0f;
    float foodPopTimer_ = 0.0f;
    bestow::Vec3 lastFoodPos_{0.0f, 0.0f, 0.0f};

    //======================================================================
    // Pixel Font Rendering (since drawText3D is not implemented)
    //======================================================================

    // 5x7 pixel font - each char is 5 columns x 7 rows packed into bytes
    // Each byte represents one column (bottom bit = top row)
    static constexpr int FONT_WIDTH = 5;
    static constexpr int FONT_HEIGHT = 7;

    // Get pixel pattern for a character (returns 5 bytes, one per column)
    static std::array<uint8_t, 5> getCharPattern(char c) {
        // Simple 5x7 pixel font patterns
        switch (c) {
            case 'A': return {0x7E, 0x11, 0x11, 0x11, 0x7E};
            case 'B': return {0x7F, 0x49, 0x49, 0x49, 0x36};
            case 'C': return {0x3E, 0x41, 0x41, 0x41, 0x22};
            case 'D': return {0x7F, 0x41, 0x41, 0x41, 0x3E};
            case 'E': return {0x7F, 0x49, 0x49, 0x49, 0x41};
            case 'F': return {0x7F, 0x09, 0x09, 0x09, 0x01};
            case 'G': return {0x3E, 0x41, 0x49, 0x49, 0x7A};
            case 'H': return {0x7F, 0x08, 0x08, 0x08, 0x7F};
            case 'I': return {0x00, 0x41, 0x7F, 0x41, 0x00};
            case 'J': return {0x20, 0x40, 0x41, 0x3F, 0x01};
            case 'K': return {0x7F, 0x08, 0x14, 0x22, 0x41};
            case 'L': return {0x7F, 0x40, 0x40, 0x40, 0x40};
            case 'M': return {0x7F, 0x02, 0x0C, 0x02, 0x7F};
            case 'N': return {0x7F, 0x04, 0x08, 0x10, 0x7F};
            case 'O': return {0x3E, 0x41, 0x41, 0x41, 0x3E};
            case 'P': return {0x7F, 0x09, 0x09, 0x09, 0x06};
            case 'Q': return {0x3E, 0x41, 0x51, 0x21, 0x5E};
            case 'R': return {0x7F, 0x09, 0x19, 0x29, 0x46};
            case 'S': return {0x46, 0x49, 0x49, 0x49, 0x31};
            case 'T': return {0x01, 0x01, 0x7F, 0x01, 0x01};
            case 'U': return {0x3F, 0x40, 0x40, 0x40, 0x3F};
            case 'V': return {0x1F, 0x20, 0x40, 0x20, 0x1F};
            case 'W': return {0x3F, 0x40, 0x38, 0x40, 0x3F};
            case 'X': return {0x63, 0x14, 0x08, 0x14, 0x63};
            case 'Y': return {0x07, 0x08, 0x70, 0x08, 0x07};
            case 'Z': return {0x61, 0x51, 0x49, 0x45, 0x43};
            case '0': return {0x3E, 0x51, 0x49, 0x45, 0x3E};
            case '1': return {0x00, 0x42, 0x7F, 0x40, 0x00};
            case '2': return {0x42, 0x61, 0x51, 0x49, 0x46};
            case '3': return {0x21, 0x41, 0x45, 0x4B, 0x31};
            case '4': return {0x18, 0x14, 0x12, 0x7F, 0x10};
            case '5': return {0x27, 0x45, 0x45, 0x45, 0x39};
            case '6': return {0x3C, 0x4A, 0x49, 0x49, 0x30};
            case '7': return {0x01, 0x71, 0x09, 0x05, 0x03};
            case '8': return {0x36, 0x49, 0x49, 0x49, 0x36};
            case '9': return {0x06, 0x49, 0x49, 0x29, 0x1E};
            case '/': return {0x20, 0x10, 0x08, 0x04, 0x02};
            case ':': return {0x00, 0x36, 0x36, 0x00, 0x00};
            case ',': return {0x00, 0x00, 0x58, 0x38, 0x00};
            case '-': return {0x08, 0x08, 0x08, 0x08, 0x08};
            case '.': return {0x00, 0x60, 0x60, 0x00, 0x00};
            case '!': return {0x00, 0x00, 0x5F, 0x00, 0x00};
            case '?': return {0x02, 0x01, 0x51, 0x09, 0x06};
            case '>': return {0x41, 0x22, 0x14, 0x08, 0x00};
            case '<': return {0x00, 0x08, 0x14, 0x22, 0x41};
            case ' ': return {0x00, 0x00, 0x00, 0x00, 0x00};
            default:  return {0x00, 0x00, 0x00, 0x00, 0x00};
        }
    }

    // Draw a single character at 3D world position using debug lines
    void drawPixelChar(char c, float x, float y, float z, float pixelSize,
                       const bestow::Color& color, bool screenSpace = false) {
        auto pattern = getCharPattern(c);
        float depth = screenSpace ? 0.0f : z;

        for (int col = 0; col < FONT_WIDTH; ++col) {
            uint8_t colBits = pattern[col];
            for (int row = 0; row < FONT_HEIGHT; ++row) {
                if (colBits & (1 << row)) {
                    // Draw this pixel as a small square (4 lines)
                    float px = x + col * pixelSize;
                    float py = y + (FONT_HEIGHT - 1 - row) * pixelSize;
                    float halfPx = pixelSize * 0.45f;

                    // Draw filled pixel using horizontal lines
                    for (float dy = -halfPx; dy <= halfPx; dy += pixelSize * 0.2f) {
                        graphics_->debugDrawLine(
                            {px - halfPx, py + dy, depth},
                            {px + halfPx, py + dy, depth},
                            color, 0.0f, false
                        );
                    }
                }
            }
        }
    }

    // Draw text string centered at position
    void drawPixelText(const std::string& text, float centerX, float y, float z,
                       float pixelSize, const bestow::Color& color, bool centered = true) {
        float charWidth = FONT_WIDTH * pixelSize + pixelSize; // Char width + spacing
        float totalWidth = text.length() * charWidth - pixelSize; // No trailing space

        float startX = centered ? centerX - totalWidth * 0.5f : centerX;

        for (size_t i = 0; i < text.length(); ++i) {
            float x = startX + i * charWidth;
            drawPixelChar(std::toupper(text[i]), x, y, z, pixelSize, color);
        }
    }

    // Draw text with shadow for better visibility
    void drawPixelTextShadow(const std::string& text, float centerX, float y, float z,
                             float pixelSize, const bestow::Color& color, bool centered = true) {
        // Shadow (slightly offset and darker)
        bestow::Color shadowColor{20, 20, 20, 200};
        float shadowOffset = pixelSize * 0.5f;
        drawPixelText(text, centerX + shadowOffset, y - shadowOffset, z + 0.01f,
                      pixelSize, shadowColor, centered);
        // Main text
        drawPixelText(text, centerX, y, z, pixelSize, color, centered);
    }

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

        // Initialize Lua runtime for config/level parsing
        if (lua_) {
            lua_->initialize();
            // Load app configuration
            auto appResult = lua_->loadApp("data/app.lua");
            if (!appResult) {
                std::cerr << "Warning: Failed to load app.lua\n";
            }
        }

        // Initialize audio system
        if (audio_) {
            audio_->initialize();
        }

        // Load sound configuration
        loadSoundConfig();

        // Load fonts for text rendering
        loadFonts();

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

        // Load world configuration
        if (loadWorld("data/worlds/world1/world.lua")) {
            // Start on the main menu
            transitionTo(GamePhase::MainMenu);
        } else {
            // Fallback: load level01 directly if world config fails
            std::cerr << "Failed to load world config, loading level directly\n";
            if (loadLevel("data/worlds/world1/level01.lua")) {
                applyLevelToGame();
            } else {
                // Fallback: Initialize snake with gradient green colors (default setup)
                std::cerr << "Using default level setup\n";
                snake_.clear();
                snake_.push_back({{gridSize_ / 2, gridSize_ / 2}, {0.2f, 0.9f, 0.3f, 1.0f}});
                snake_.push_back({{gridSize_ / 2 - 1, gridSize_ / 2}, {0.25f, 0.85f, 0.35f, 1.0f}});
                snake_.push_back({{gridSize_ / 2 - 2, gridSize_ / 2}, {0.3f, 0.8f, 0.4f, 1.0f}});

                direction_ = Direction::Right;
                nextDirection_ = Direction::Right;

                // Spawn initial food
                spawnFood();
            }
        }

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

                // Only run gameplay during Playing phase
                if (currentPhase_ == GamePhase::Playing) {
                    moveTimer_ += fixedDt;
                    if (moveTimer_ >= moveInterval_) {
                        moveTimer_ = 0.0f;
                        applyBufferedInput();
                        moveSnake();
                    }
                    updateEnemies(fixedDt);
                    updateDetachedSegments(fixedDt);
                    checkFoodPickups();
                }

                updateAnimations(fixedDt);
                updateCamera(fixedDt);
                accumulator -= fixedDt;
            }

            // Render based on current phase
            graphics_->beginFrame();
            if (currentPhase_ == GamePhase::MainMenu) {
                menuAnimTime_ += fixedDt;
                drawMainMenu();
            } else if (currentPhase_ == GamePhase::WorldMap) {
                updateWorldMap(fixedDt);
                drawWorldMap();
            } else if (currentPhase_ == GamePhase::Paused) {
                // Draw game underneath (frozen)
                drawGround();
                drawObstacles();
                drawEnemies();
                drawSnake();
                drawDetachedSegments();
                drawFoodPickups();
                drawFood();
                drawGridBorder();
                // Overlay pause menu
                drawPauseMenu();
            } else {
                drawGround();
                drawObstacles();
                drawEnemies();
                drawSnake();
                drawDetachedSegments();
                drawFoodPickups();
                drawFood();
                drawFoodPopEffect();
                drawGridBorder();
                drawHUD();
            }
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
    // State Machine
    //======================================================================

    void transitionTo(GamePhase newPhase) {
        if (newPhase == currentPhase_) return;

        // Exit current state
        exitPhase(currentPhase_);

        // Transition
        previousPhase_ = currentPhase_;
        currentPhase_ = newPhase;

        // Enter new state
        enterPhase(newPhase);
    }

    void exitPhase(GamePhase phase) {
        switch (phase) {
            case GamePhase::Playing:
                // Nothing special on exit
                break;
            case GamePhase::Paused:
                // Resume timers, etc.
                break;
            case GamePhase::LevelComplete:
                // Save progress
                break;
            default:
                break;
        }
    }

    void enterPhase(GamePhase phase) {
        switch (phase) {
            case GamePhase::MainMenu:
                // Start menu music
                playMusicTrack(musicMenu_);
                break;
            case GamePhase::WorldMap:
                initializeWorldMap();
                // Continue menu music (already playing from MainMenu)
                break;
            case GamePhase::Playing:
                gameOver_ = false;
                // Switch to game music
                playMusicTrack(musicGame_);
                break;
            case GamePhase::Paused:
                // Pause timers - keep music playing but could lower volume
                break;
            case GamePhase::BossFight:
                // Initialize boss fight (accumulated segments!)
                break;
            case GamePhase::LevelComplete:
                // Show victory screen, update progress
                break;
            case GamePhase::GameOver:
                gameOver_ = true;
                // Play game over sound and switch to menu music
                playSound(soundGameOver_);
                playMusicTrack(musicMenu_);  // Switch back to calm menu music
                break;
        }
    }

    //======================================================================
    // Level Loading
    //======================================================================

    bool loadLevel(const std::string& levelPath) {
        if (!lua_) {
            std::cerr << "Lua runtime not available for level loading\n";
            return false;
        }

        // Read level file
        std::ifstream file(levelPath);
        if (!file) {
            std::cerr << "Failed to open level file: " << levelPath << "\n";
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string luaContent = buffer.str();

        // Parse Lua using LuaRuntime
        auto result = lua_->execute(luaContent, levelPath);
        if (!result) {
            std::cerr << "Failed to parse level Lua: " << levelPath << " - " << result.error().message << "\n";
            return false;
        }

        // Extract level data from sol::object
        sol::table levelTable = result->as<sol::table>();
        return parseLevelTable(levelTable);
    }

    bool parseLevelTable(const sol::table& table) {
        // Clear current level
        currentLevel_ = LevelMap{};

        // Basic properties
        currentLevel_.name = table.get_or("name", std::string("Unnamed Level"));
        currentLevel_.width = table.get_or("width", 15);
        currentLevel_.height = table.get_or("height", 15);
        currentLevel_.foodRequired = table.get_or("foodRequired", 5);
        currentLevel_.isBossLevel = table.get_or("isBossLevel", false);

        // Player start position
        if (sol::table playerStart = table["playerStart"]; playerStart.valid()) {
            currentLevel_.playerStart.x = playerStart.get_or("x", currentLevel_.width / 2);
            currentLevel_.playerStart.z = playerStart.get_or("z", currentLevel_.height / 2);
        }

        // Walls
        if (sol::table walls = table["walls"]; walls.valid()) {
            for (auto& pair : walls) {
                sol::table wallPos = pair.second.as<sol::table>();
                currentLevel_.walls.push_back({
                    wallPos.get_or("x", 0),
                    wallPos.get_or("z", 0)
                });
            }
        }

        // Food spawn points
        if (sol::table spawnPoints = table["foodSpawnPoints"]; spawnPoints.valid()) {
            for (auto& pair : spawnPoints) {
                sol::table pos = pair.second.as<sol::table>();
                currentLevel_.foodSpawnPoints.push_back({
                    pos.get_or("x", 0),
                    pos.get_or("z", 0)
                });
            }
        }

        // Enemy zones
        if (sol::table enemies = table["enemies"]; enemies.valid()) {
            for (auto& pair : enemies) {
                sol::table enemyDef = pair.second.as<sol::table>();
                sol::table zone = enemyDef["zone"];

                EnemyZone ez;
                ez.bounds.min.x = zone.get_or("minX", 0);
                ez.bounds.min.z = zone.get_or("minZ", 0);
                ez.bounds.max.x = zone.get_or("maxX", 0);
                ez.bounds.max.z = zone.get_or("maxZ", 0);
                ez.enemyCount = enemyDef.get_or("count", 1);
                ez.moveInterval = enemyDef.get_or("moveInterval", static_cast<double>(ENEMY_MOVE_INTERVAL));
                ez.health = enemyDef.get_or("health", 1);

                currentLevel_.enemyZones.push_back(ez);
            }
        }

        // Update grid size from level
        gridSize_ = currentLevel_.width;
        foodRequired_ = currentLevel_.foodRequired;

        std::cerr << "Loaded level: " << currentLevel_.name
                  << " (" << currentLevel_.width << "x" << currentLevel_.height << ")"
                  << " with " << currentLevel_.walls.size() << " walls, "
                  << currentLevel_.foodSpawnPoints.size() << " spawn points, "
                  << currentLevel_.enemyZones.size() << " enemy zones\n";

        return true;
    }

    //======================================================================
    // Sound System
    //======================================================================

    void loadSoundConfig() {
        if (!lua_ || !audio_) {
            std::cerr << "Lua or audio system not available for sound loading\n";
            return;
        }

        // Read sounds config file
        std::ifstream file("data/config/sounds.lua");
        if (!file) {
            std::cerr << "No sounds.lua config found, audio disabled\n";
            return;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string luaContent = buffer.str();

        auto result = lua_->execute(luaContent, "sounds.lua");
        if (!result) {
            std::cerr << "Failed to parse sounds.lua: " << result.error().message << "\n";
            return;
        }

        sol::table soundsTable = result->as<sol::table>();

        // Load volume settings
        if (soundsTable["volumes"].valid()) {
            sol::table volumes = soundsTable["volumes"];
            if (volumes["master"].valid()) {
                audio_->setMasterVolume(volumes["master"].get<float>());
            }
            if (volumes["sfx"].valid()) {
                audio_->setGroupVolume("SFX", volumes["sfx"].get<float>());
            }
            if (volumes["music"].valid()) {
                audio_->setGroupVolume("Music", volumes["music"].get<float>());
            }
        }

        // Load sound effects
        if (soundsTable["sfx"].valid()) {
            sol::table sfx = soundsTable["sfx"];
            soundEat_ = tryLoadSound(sfx, "eat");
            soundDeath_ = tryLoadSound(sfx, "death");
            soundLevelComplete_ = tryLoadSound(sfx, "level_complete");
            soundMenuSelect_ = tryLoadSound(sfx, "menu_select");
            soundMenuMove_ = tryLoadSound(sfx, "menu_move");
            soundEnemyHit_ = tryLoadSound(sfx, "enemy_hit");
            soundChainBreak_ = tryLoadSound(sfx, "chain_break");
            soundPause_ = tryLoadSound(sfx, "pause");
            soundGameOver_ = tryLoadSound(sfx, "game_over");
        }

        // Load music
        if (soundsTable["music"].valid()) {
            sol::table music = soundsTable["music"];
            musicGame_ = tryLoadMusic(music, "game");
            musicMenu_ = tryLoadMusic(music, "menu");
        }

        soundsLoaded_ = true;
        std::cerr << "Sound config loaded successfully\n";
    }

    bestow::AssetHandle tryLoadSound(sol::table& table, const std::string& key) {
        if (!table[key].valid() || !assets_) return {};

        std::string path = table[key].get<std::string>();
        std::cerr << "Loading sound: " << key << " -> " << path << "\n";

        // Register and load the sound asset
        bestow::AssetHandle handle = assets_->registerAsset(bestow::AssetType::Sound, path);
        if (handle.isValid()) {
            assets_->loadAsset(handle);
        }
        return handle;
    }

    bestow::AssetHandle tryLoadMusic(sol::table& table, const std::string& key) {
        if (!table[key].valid() || !assets_) return {};

        std::string path = table[key].get<std::string>();
        std::cerr << "Loading music: " << key << " -> " << path << "\n";

        // Register and load the music asset
        bestow::AssetHandle handle = assets_->registerAsset(bestow::AssetType::Sound, path);
        if (handle.isValid()) {
            assets_->loadAsset(handle);
        }
        return handle;
    }

    void playSound(bestow::AssetHandle sound, float volume = 1.0f) {
        if (!audio_ || !soundsLoaded_ || !sound.isValid()) return;
        audio_->playOnChannel(bestow::Channels::UI, {
            .asset = sound,
            .volume = volume,
            .pitch = 1.0f,
            .looping = false
        });
    }

    void playSoundPositional(bestow::AssetHandle sound, const bestow::Vec3& pos, float volume = 1.0f) {
        if (!audio_ || !soundsLoaded_ || !sound.isValid()) return;

        audio_->playPositional({
            .asset = sound,
            .position = pos,
            .volume = volume,
            .minDistance = 5.0f,
            .maxDistance = 50.0f
        });
    }

    void playMusicTrack(bestow::AssetHandle music, bool loop = true, float fadeIn = 1.0f) {
        if (!audio_ || !soundsLoaded_ || !music.isValid()) return;

        audio_->playOnChannel(bestow::Channels::Music, {
            .asset = music,
            .volume = 1.0f,
            .looping = loop,
            .fadeInTime = fadeIn
        });
    }

    void stopMusic(float fadeOut = 1.0f) {
        if (!audio_) return;
        audio_->stopChannel(bestow::Channels::Music, fadeOut);
    }

    //======================================================================
    // Font Loading
    //======================================================================

    void loadFonts() {
        if (!assets_ || !graphics_) {
            std::cerr << "Assets or graphics not available for font loading\n";
            return;
        }

        // Load Press Start 2P for retro game feel (menus)
        auto gameFontAsset = assets_->registerAsset(
            bestow::AssetType::Font,
            ":library:/fonts/Press_Start_2P/PressStart2P-Regular.ttf"
        );
        if (gameFontAsset.isValid()) {
            assets_->loadAsset(gameFontAsset);
            auto fontResult = graphics_->loadFont3D(gameFontAsset);
            if (fontResult) {
                gameFont_ = *fontResult;
                std::cerr << "Loaded game font: PressStart2P\n";
            } else {
                std::cerr << "Failed to create Font3D from PressStart2P\n";
            }
        }

        // Load Orbitron for titles (more stylized)
        auto titleFontAsset = assets_->registerAsset(
            bestow::AssetType::Font,
            ":library:/fonts/Orbitron/static/Orbitron-Bold.ttf"
        );
        if (titleFontAsset.isValid()) {
            assets_->loadAsset(titleFontAsset);
            auto fontResult = graphics_->loadFont3D(titleFontAsset);
            if (fontResult) {
                titleFont_ = *fontResult;
                std::cerr << "Loaded title font: Orbitron-Bold\n";
            } else {
                std::cerr << "Failed to create Font3D from Orbitron-Bold\n";
            }
        }
    }

    bool loadWorld(const std::string& worldPath) {
        if (!lua_) {
            std::cerr << "Lua runtime not available for world loading\n";
            return false;
        }

        // Read world file
        std::ifstream file(worldPath);
        if (!file) {
            std::cerr << "Failed to open world file: " << worldPath << "\n";
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string luaContent = buffer.str();

        // Parse Lua using LuaRuntime
        auto result = lua_->execute(luaContent, worldPath);
        if (!result) {
            std::cerr << "Failed to parse world Lua: " << worldPath << " - " << result.error().message << "\n";
            return false;
        }

        // Extract world data from sol::object
        sol::table worldTable = result->as<sol::table>();
        return parseWorldTable(worldTable);
    }

    bool parseWorldTable(const sol::table& table) {
        // Clear current world
        currentWorld_ = WorldConfig{};

        // Basic properties
        currentWorld_.name = table.get_or("name", std::string("Unnamed World"));
        currentWorld_.theme = table.get_or("theme", std::string("default"));

        // Level files
        if (sol::table levels = table["levels"]; levels.valid()) {
            for (auto& pair : levels) {
                currentWorld_.levelFiles.push_back(pair.second.as<std::string>());
            }
        }

        // Unlock requirements
        if (sol::table unlocks = table["unlockRequirements"]; unlocks.valid()) {
            for (auto& pair : unlocks) {
                int idx = pair.first.as<int>() - 1;  // Lua is 1-indexed
                int requirement = pair.second.as<int>();
                while (static_cast<int>(currentWorld_.unlockRequirements.size()) <= idx) {
                    currentWorld_.unlockRequirements.push_back(0);
                }
                currentWorld_.unlockRequirements[idx] = requirement;
            }
        }

        // Map nodes
        if (sol::table nodes = table["nodes"]; nodes.valid()) {
            for (auto& pair : nodes) {
                sol::table nodeDef = pair.second.as<sol::table>();
                WorldMapNode node;
                node.gridPos.x = nodeDef.get_or("x", 0);
                node.gridPos.z = nodeDef.get_or("z", 0);
                node.levelIndex = nodeDef.get_or("levelIndex", 0) - 1;  // Lua 1-indexed
                node.displayName = nodeDef.get_or("name", std::string("Level"));
                node.isBoss = nodeDef.get_or("isBoss", false);
                currentWorld_.nodes.push_back(node);
            }
        }

        // Paths (connections between nodes)
        if (sol::table paths = table["paths"]; paths.valid()) {
            for (auto& pair : paths) {
                sol::table pathDef = pair.second.as<sol::table>();
                int a = pathDef[1].get<int>() - 1;  // Lua 1-indexed
                int b = pathDef[2].get<int>() - 1;
                currentWorld_.paths.push_back({a, b});
            }
        }

        // Boss config
        if (sol::table boss = table["boss"]; boss.valid()) {
            currentWorld_.bossName = boss.get_or("name", std::string("Boss"));
            currentWorld_.bossType = boss.get_or("type", std::string("default"));
            currentWorld_.bossHealth = boss.get_or("health", 10);
        }

        std::cerr << "Loaded world: " << currentWorld_.name
                  << " with " << currentWorld_.levelFiles.size() << " levels, "
                  << currentWorld_.nodes.size() << " nodes, "
                  << currentWorld_.paths.size() << " paths\n";

        return true;
    }

    void applyLevelToGame() {
        // Store the level's max size, but start with initial small grid
        levelMaxSize_ = currentLevel_.width;
        gridSize_ = INITIAL_GRID_SIZE;
        targetGridSize_ = INITIAL_GRID_SIZE;
        rebuildGroundMesh();

        // Calculate offset: how far into the level our current view is
        // We start centered in the level
        updateLevelOffset();

        // Clear all transient game state
        snake_.clear();
        detachedSegments_.clear();
        foodPickups_.clear();

        // Reset snake at center of current grid
        int centerX = gridSize_ / 2;
        int centerZ = gridSize_ / 2;
        snake_.push_back({{centerX, centerZ}, {0.2f, 0.9f, 0.3f, 1.0f}});
        snake_.push_back({{centerX - 1, centerZ}, {0.25f, 0.85f, 0.35f, 1.0f}});
        snake_.push_back({{centerX - 2, centerZ}, {0.3f, 0.8f, 0.4f, 1.0f}});

        // Build visible obstacles from level walls
        rebuildVisibleObstacles();

        // Reset game state
        direction_ = Direction::Right;
        nextDirection_ = Direction::Right;
        foodCollected_ = 0;
        score_ = 0;
        gameOver_ = false;
        moveInterval_ = INITIAL_MOVE_INTERVAL;

        // Spawn enemies from zones (only those in current view)
        spawnEnemiesFromZones();

        // Spawn initial food
        spawnFood();
    }

    void updateLevelOffset() {
        // Offset from current grid origin to level origin
        // As grid expands, offset decreases (we see more of the level)
        levelOffsetX_ = (levelMaxSize_ - gridSize_) / 2;
        levelOffsetZ_ = (levelMaxSize_ - gridSize_) / 2;
    }

    void rebuildVisibleObstacles() {
        obstacles_.clear();

        // Convert level walls to current grid coordinates
        // Only include walls that are within current grid bounds
        for (const auto& wall : currentLevel_.walls) {
            // Transform from level coords to current grid coords
            int gridX = wall.x - levelOffsetX_;
            int gridZ = wall.z - levelOffsetZ_;

            // Only include if within current playable area
            if (gridX >= 0 && gridX < gridSize_ && gridZ >= 0 && gridZ < gridSize_) {
                obstacles_.push_back({gridX, gridZ});
            }
        }
    }

    void spawnEnemiesFromZones() {
        enemies_.clear();
        for (const auto& zone : currentLevel_.enemyZones) {
            // Transform zone bounds from level coords to current grid coords
            AABB2Di gridZone;
            gridZone.min.x = zone.bounds.min.x - levelOffsetX_;
            gridZone.min.z = zone.bounds.min.z - levelOffsetZ_;
            gridZone.max.x = zone.bounds.max.x - levelOffsetX_;
            gridZone.max.z = zone.bounds.max.z - levelOffsetZ_;

            // Skip zones that are entirely outside current grid
            if (gridZone.max.x < 0 || gridZone.min.x >= gridSize_ ||
                gridZone.max.z < 0 || gridZone.min.z >= gridSize_) {
                continue;
            }

            // Clamp zone to current grid bounds
            gridZone.min.x = std::max(0, gridZone.min.x);
            gridZone.min.z = std::max(0, gridZone.min.z);
            gridZone.max.x = std::min(gridSize_ - 1, gridZone.max.x);
            gridZone.max.z = std::min(gridSize_ - 1, gridZone.max.z);

            for (int i = 0; i < zone.enemyCount; ++i) {
                Enemy enemy;
                enemy.patrolZone = gridZone;
                enemy.moveInterval = zone.moveInterval;
                enemy.maxHealth = zone.health;
                enemy.currentHealth = zone.health;

                // Spawn at random position within the visible part of the zone
                std::uniform_int_distribution<int> xDist(gridZone.min.x, gridZone.max.x);
                std::uniform_int_distribution<int> zDist(gridZone.min.z, gridZone.max.z);
                enemy.pos = {xDist(rng_), zDist(rng_)};

                // Initialize visual position to match grid position
                enemy.visualX = static_cast<float>(enemy.pos.x);
                enemy.visualZ = static_cast<float>(enemy.pos.z);
                enemy.visualInitialized = true;

                // Random starting direction
                std::uniform_int_distribution<int> dirDist(0, 3);
                enemy.currentDir = static_cast<Direction>(dirDist(rng_));

                enemies_.push_back(enemy);
            }
        }
    }

    //======================================================================
    // Enemy AI and Update
    //======================================================================

    void updateEnemies(float dt) {
        if (gameOver_) return;

        constexpr float ENEMY_LERP_SPEED = 8.0f;  // How fast enemies glide to new position

        for (auto& enemy : enemies_) {
            // Initialize visual position if needed
            if (!enemy.visualInitialized) {
                enemy.visualX = static_cast<float>(enemy.pos.x);
                enemy.visualZ = static_cast<float>(enemy.pos.z);
                enemy.visualInitialized = true;
            }

            // Smoothly interpolate visual position toward grid position
            float targetX = static_cast<float>(enemy.pos.x);
            float targetZ = static_cast<float>(enemy.pos.z);
            enemy.visualX += (targetX - enemy.visualX) * ENEMY_LERP_SPEED * dt;
            enemy.visualZ += (targetZ - enemy.visualZ) * ENEMY_LERP_SPEED * dt;

            // Update damage flash timer
            if (enemy.isDamaged) {
                enemy.damageFlashTimer -= dt;
                if (enemy.damageFlashTimer <= 0.0f) {
                    enemy.isDamaged = false;
                }
            }

            // Update health bar visibility timer
            if (enemy.showHealthBar) {
                enemy.healthBarTimer -= dt;
                if (enemy.healthBarTimer <= 0.0f) {
                    enemy.showHealthBar = false;
                }
            }

            // Update movement timer
            enemy.moveTimer += dt;
            if (enemy.moveTimer >= enemy.moveInterval) {
                enemy.moveTimer = 0.0f;
                moveEnemy(enemy);
            }
        }

        // Check enemy-snake collision
        checkEnemySnakeCollision();
    }

    void moveEnemy(Enemy& enemy) {
        // Collect valid moves within patrol zone
        std::vector<Direction> validMoves;
        for (int d = 0; d < 4; ++d) {
            Direction dir = static_cast<Direction>(d);
            GridPos offset = directionToOffset(dir);
            GridPos newPos = enemy.pos + offset;

            // Check if within patrol zone
            if (enemy.patrolZone.contains(newPos)) {
                // Check if not blocked by wall or other enemy
                if (!isObstacleAt(newPos) && !isEnemyAt(newPos, &enemy)) {
                    validMoves.push_back(dir);
                }
            }
        }

        if (validMoves.empty()) return;

        // 85% chance to continue in current direction if valid (less random)
        std::uniform_real_distribution<float> chanceDist(0.0f, 1.0f);
        bool preferCurrent = chanceDist(rng_) < 0.85f;

        Direction chosenDir = enemy.currentDir;
        bool currentIsValid = false;

        for (Direction dir : validMoves) {
            if (dir == enemy.currentDir) {
                currentIsValid = true;
                break;
            }
        }

        if (preferCurrent && currentIsValid) {
            chosenDir = enemy.currentDir;
        } else {
            // Pick random valid direction
            std::uniform_int_distribution<size_t> moveDist(0, validMoves.size() - 1);
            chosenDir = validMoves[moveDist(rng_)];
        }

        // Apply movement
        GridPos offset = directionToOffset(chosenDir);
        enemy.pos = enemy.pos + offset;
        enemy.currentDir = chosenDir;
    }

    bool isEnemyAt(const GridPos& pos, const Enemy* exclude = nullptr) const {
        for (const auto& enemy : enemies_) {
            if (&enemy != exclude && enemy.pos == pos) {
                return true;
            }
        }
        return false;
    }

    void checkEnemySnakeCollision() {
        if (snake_.empty()) return;

        // Check if snake head hit any enemy
        const GridPos& headPos = snake_[0].pos;
        for (const auto& enemy : enemies_) {
            if (enemy.pos == headPos) {
                // Snake head hit enemy = game over with explosion!
                explodeSnake();
                transitionTo(GamePhase::GameOver);
                return;
            }
        }

        // Check if any enemy hit the snake body - trigger chain break
        for (const auto& enemy : enemies_) {
            for (size_t i = 1; i < snake_.size(); ++i) {
                if (enemy.pos == snake_[i].pos) {
                    // Chain break at collision point!
                    triggerChainBreak(static_cast<int>(i));
                    return;  // Only one break per frame
                }
            }
        }
    }

    //======================================================================
    // Ring Attack System
    //======================================================================

    RingResult detectRing(const GridPos& headPos) {
        RingResult result;

        // Check if head would touch any body segment
        for (size_t i = 2; i < snake_.size(); ++i) {  // Start at 2, skip neck
            if (headPos == snake_[i].pos) {
                result.formed = true;
                result.ringStartIndex = static_cast<int>(i);
                result.enclosedCells = calculateEnclosedCells(static_cast<int>(i));
                return result;
            }
        }

        return result;
    }

    std::vector<GridPos> calculateEnclosedCells(int ringEndIndex) {
        // Ring boundary = segments [0..ringEndIndex]
        // Use flood fill from outside to find exterior cells
        // Everything not exterior and not boundary is interior

        std::set<GridPos> boundary;
        for (int i = 0; i <= ringEndIndex; ++i) {
            boundary.insert(snake_[i].pos);
        }

        // Also add obstacles to prevent flood fill through them
        std::set<GridPos> blocked = boundary;
        for (const auto& obstacle : obstacles_) {
            blocked.insert(obstacle);
        }

        // Flood fill from all edge cells to find exterior
        std::vector<std::vector<bool>> exterior(gridSize_, std::vector<bool>(gridSize_, false));
        std::queue<GridPos> queue;

        // Start flood fill from all edge cells that aren't blocked
        for (int x = 0; x < gridSize_; ++x) {
            GridPos top{x, 0};
            GridPos bottom{x, gridSize_ - 1};
            if (blocked.find(top) == blocked.end()) {
                queue.push(top);
                exterior[top.x][top.z] = true;
            }
            if (blocked.find(bottom) == blocked.end()) {
                queue.push(bottom);
                exterior[bottom.x][bottom.z] = true;
            }
        }
        for (int z = 1; z < gridSize_ - 1; ++z) {
            GridPos left{0, z};
            GridPos right{gridSize_ - 1, z};
            if (blocked.find(left) == blocked.end()) {
                queue.push(left);
                exterior[left.x][left.z] = true;
            }
            if (blocked.find(right) == blocked.end()) {
                queue.push(right);
                exterior[right.x][right.z] = true;
            }
        }

        // BFS to fill exterior
        while (!queue.empty()) {
            GridPos current = queue.front();
            queue.pop();

            for (int d = 0; d < 4; ++d) {
                Direction dir = static_cast<Direction>(d);
                GridPos offset = directionToOffset(dir);
                GridPos next = current + offset;

                // Don't wrap around for ring detection
                if (next.x < 0 || next.x >= gridSize_ ||
                    next.z < 0 || next.z >= gridSize_) {
                    continue;
                }

                if (!exterior[next.x][next.z] && blocked.find(next) == blocked.end()) {
                    exterior[next.x][next.z] = true;
                    queue.push(next);
                }
            }
        }

        // Collect interior cells (not exterior, not boundary)
        std::vector<GridPos> interior;
        for (int x = 0; x < gridSize_; ++x) {
            for (int z = 0; z < gridSize_; ++z) {
                GridPos pos{x, z};
                if (!exterior[x][z] && boundary.find(pos) == boundary.end()) {
                    interior.push_back(pos);
                }
            }
        }

        return interior;
    }

    void executeRingAttack(const RingResult& ring) {
        if (!ring.formed || ring.enclosedCells.empty()) return;

        // Find and damage enemies inside the ring
        std::set<GridPos> enclosed(ring.enclosedCells.begin(), ring.enclosedCells.end());

        for (auto& enemy : enemies_) {
            if (enclosed.count(enemy.pos) > 0) {
                damageEnemy(enemy, 1);
            }
        }

        // Remove dead enemies
        enemies_.erase(
            std::remove_if(enemies_.begin(), enemies_.end(),
                [](const Enemy& e) { return e.currentHealth <= 0; }),
            enemies_.end()
        );

        // Check if any enemies are ON the ring boundary (not inside, but on snake body)
        // This triggers chain break
        for (const auto& enemy : enemies_) {
            for (int i = 1; i <= ring.ringStartIndex && i < static_cast<int>(snake_.size()); ++i) {
                if (enemy.pos == snake_[i].pos) {
                    triggerChainBreak(i);
                    return;  // Only one break
                }
            }
        }
    }

    void damageEnemy(Enemy& enemy, int damage) {
        enemy.currentHealth -= damage;
        enemy.isDamaged = true;
        enemy.damageFlashTimer = 0.3f;  // Flash for 0.3 seconds
        enemy.showHealthBar = true;
        enemy.healthBarTimer = HEALTH_BAR_DURATION;

        // Play hit sound
        playSound(soundEnemyHit_);
    }

    //======================================================================
    // Chain Break Mechanic
    //======================================================================

    void triggerChainBreak(int breakIndex) {
        if (breakIndex <= 0 || breakIndex >= static_cast<int>(snake_.size())) return;

        // Play chain break sound
        playSound(soundChainBreak_);

        // Detach all segments after break point
        for (size_t i = static_cast<size_t>(breakIndex + 1); i < snake_.size(); ++i) {
            DetachedSegment detached;
            detached.pos = snake_[i].pos;
            detached.color = snake_[i].color;
            detached.timer = DETACH_ANIMATION_TIME;

            // Roll for shatter chance (1/6)
            std::uniform_real_distribution<float> chanceDist(0.0f, 1.0f);
            detached.willShatter = chanceDist(rng_) < SHATTER_CHANCE;

            // Give random explosion velocity
            std::uniform_real_distribution<float> velDist(-3.0f, 3.0f);
            detached.velocity = {velDist(rng_), std::abs(velDist(rng_)) + 2.0f, velDist(rng_)};

            detachedSegments_.push_back(detached);
        }

        // Trim snake to break point (keep segment at breakIndex as new tail)
        snake_.resize(static_cast<size_t>(breakIndex + 1));
    }

    void explodeSnake() {
        if (snake_.empty()) return;

        // Play death sound
        playSound(soundDeath_);

        // Trigger screen shake for death
        triggerScreenShake(0.5f, 0.4f);

        // Calculate center of snake for explosion direction
        float centerX = 0.0f, centerZ = 0.0f;
        for (const auto& seg : snake_) {
            centerX += static_cast<float>(seg.pos.x);
            centerZ += static_cast<float>(seg.pos.z);
        }
        centerX /= snake_.size();
        centerZ /= snake_.size();

        // Random distributions for particle variation
        std::uniform_real_distribution<float> velDist(-1.0f, 1.0f);
        std::uniform_real_distribution<float> offsetDist(-0.3f, 0.3f);
        // For 8 particles per segment, each should be ~0.5 scale (cube root of 1/8)
        // Use slight variation for visual interest
        std::uniform_real_distribution<float> scaleDist(0.4f, 0.55f);
        std::uniform_real_distribution<float> timerDist(2.0f, 4.0f);  // Longer life for bounce/scatter

        // Convert each snake segment into many tiny exploding cubes
        constexpr int PARTICLES_PER_SEGMENT = 8;

        for (size_t i = 0; i < snake_.size(); ++i) {
            // Get world position of this segment
            float segWorldX = snake_[i].pos.x * CELL_SIZE - gridSize_ * CELL_SIZE * 0.5f + CELL_SIZE * 0.5f;
            float segWorldZ = snake_[i].pos.z * CELL_SIZE - gridSize_ * CELL_SIZE * 0.5f + CELL_SIZE * 0.5f;
            float segWorldY = CELL_SIZE * 0.5f;

            // Calculate base outward direction from snake center
            float dx = static_cast<float>(snake_[i].pos.x) - centerX;
            float dz = static_cast<float>(snake_[i].pos.z) - centerZ;
            float dist = std::sqrt(dx * dx + dz * dz);
            if (dist < 0.1f) dist = 0.1f;
            dx /= dist;
            dz /= dist;

            // Spawn multiple tiny particles per segment
            for (int p = 0; p < PARTICLES_PER_SEGMENT; ++p) {
                DetachedSegment particle;
                particle.pos = snake_[i].pos;
                particle.color = snake_[i].color;
                particle.timer = DETACH_ANIMATION_TIME * timerDist(rng_);
                particle.willShatter = true;
                particle.isParticle = true;
                particle.scale = scaleDist(rng_);

                // Start position with slight random offset within the segment
                particle.worldPos = {
                    segWorldX + offsetDist(rng_),
                    segWorldY + offsetDist(rng_),
                    segWorldZ + offsetDist(rng_)
                };

                // Explosive velocity - outward from center with randomness
                float speed = 4.0f + velDist(rng_) * 2.0f;
                particle.velocity = {
                    dx * speed + velDist(rng_) * 3.0f,
                    3.0f + std::abs(velDist(rng_)) * 4.0f,  // Always some upward
                    dz * speed + velDist(rng_) * 3.0f
                };

                // Random angular velocity for tumbling
                particle.angularVel = velDist(rng_) * 15.0f;
                particle.bounceCount = 0;
                particle.isResting = false;

                detachedSegments_.push_back(particle);
            }
        }

        // Clear the snake (it's now all particles)
        snake_.clear();
    }

    void updateDetachedSegments(float dt) {
        // Collect new particles from breaking
        std::vector<DetachedSegment> newParticles;

        for (auto& segment : detachedSegments_) {
            segment.timer -= dt;

            // Skip resting particles (just waiting to fade)
            if (segment.isResting) continue;

            // Apply physics for explosion animation
            segment.velocity.y -= 15.0f * dt;  // Gravity

            // Move particles by velocity
            if (segment.isParticle) {
                segment.worldPos.x += segment.velocity.x * dt;
                segment.worldPos.y += segment.velocity.y * dt;
                segment.worldPos.z += segment.velocity.z * dt;

                // Ground height depends on particle scale
                float groundLevel = segment.scale * CELL_SIZE * 0.5f;

                // Bounce when hitting ground
                if (segment.worldPos.y < groundLevel) {
                    segment.worldPos.y = groundLevel;
                    segment.bounceCount++;

                    // Check if particle should break on impact
                    if (segment.bounceCount == 1 && segment.scale > 0.2f) {
                        // First bounce: chance to break into smaller pieces
                        std::uniform_real_distribution<float> breakChance(0.0f, 1.0f);
                        if (breakChance(rng_) < 0.4f) {  // 40% chance to shatter
                            // Spawn 3-4 smaller fragments
                            std::uniform_int_distribution<int> fragCount(3, 4);
                            std::uniform_real_distribution<float> velDist(-2.0f, 2.0f);
                            int numFrags = fragCount(rng_);

                            for (int f = 0; f < numFrags; ++f) {
                                DetachedSegment frag;
                                frag.isParticle = true;
                                frag.scale = segment.scale * 0.4f;  // Fragments are smaller
                                frag.color = segment.color;
                                frag.timer = segment.timer * 0.6f;
                                frag.worldPos = segment.worldPos;
                                frag.worldPos.x += velDist(rng_) * 0.1f;
                                frag.worldPos.z += velDist(rng_) * 0.1f;
                                frag.worldPos.y = frag.scale * CELL_SIZE * 0.5f;
                                frag.velocity = {
                                    segment.velocity.x * 0.3f + velDist(rng_) * 1.5f,
                                    std::abs(segment.velocity.y) * 0.2f + 1.0f,
                                    segment.velocity.z * 0.3f + velDist(rng_) * 1.5f
                                };
                                frag.angularVel = velDist(rng_) * 20.0f;
                                frag.bounceCount = 1;  // Skip further breaking
                                frag.pos = segment.pos;
                                frag.willShatter = true;  // Fragments don't spawn food
                                newParticles.push_back(frag);
                            }

                            // Mark original as done
                            segment.timer = 0.0f;
                            segment.willShatter = true;
                            continue;
                        }
                    }

                    // Bounce with energy loss
                    float bounciness = 0.5f;
                    if (segment.bounceCount > 2) bounciness = 0.3f;
                    if (segment.bounceCount > 4) bounciness = 0.1f;

                    segment.velocity.y = -segment.velocity.y * bounciness;

                    // Friction on horizontal movement
                    float friction = 0.7f;
                    segment.velocity.x *= friction;
                    segment.velocity.z *= friction;

                    // Reduce spin on bounce
                    segment.angularVel *= 0.6f;

                    // Come to rest after enough bounces or low velocity
                    float speed = std::sqrt(
                        segment.velocity.x * segment.velocity.x +
                        segment.velocity.y * segment.velocity.y +
                        segment.velocity.z * segment.velocity.z
                    );
                    if (speed < 0.3f || segment.bounceCount > 5) {
                        segment.isResting = true;
                        segment.velocity = {0.0f, 0.0f, 0.0f};
                    }
                }
            }
        }

        // Add newly spawned fragments
        for (auto& p : newParticles) {
            detachedSegments_.push_back(p);
        }

        // Process completed segments
        std::vector<DetachedSegment> remaining;
        for (auto& segment : detachedSegments_) {
            if (segment.timer <= 0.0f) {
                if (!segment.willShatter) {
                    // Spawn food pickup
                    FoodPickup food;
                    food.pos = segment.pos;
                    food.color = segment.color;
                    food.spawnTime = gameTime_;
                    foodPickups_.push_back(food);
                }
                // If willShatter, just disappear (could add particle effect)
            } else {
                remaining.push_back(segment);
            }
        }
        detachedSegments_ = remaining;
    }

    void checkFoodPickups() {
        if (snake_.empty()) return;

        const GridPos& headPos = snake_[0].pos;
        std::vector<FoodPickup> remaining;

        for (const auto& pickup : foodPickups_) {
            if (pickup.pos == headPos) {
                // Collect the food - add segment with pickup's color
                SnakeSegment newSegment;
                newSegment.pos = snake_.back().pos;  // Will be at tail
                newSegment.color = pickup.color;
                snake_.push_back(newSegment);
                score_++;
            } else {
                remaining.push_back(pickup);
            }
        }
        foodPickups_ = remaining;
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
        // Don't expand beyond the level's max size
        if (gridSize_ >= levelMaxSize_) {
            return;
        }

        isExpanding_ = true;
        expansionTimer_ = 0.0f;
        targetGridSize_ = std::min(gridSize_ + 2, levelMaxSize_);  // Expand by 2, capped at level size
        // New border snaps into existence immediately
    }

    void finalizeExpansion() {
        // Calculate how much we're expanding (should be 2 for symmetrical growth)
        int expansion = targetGridSize_ - gridSize_;
        int shift = expansion / 2;  // How much to shift existing items

        // Shift all existing positions to center the expansion
        for (auto& segment : snake_) {
            segment.pos.x += shift;
            segment.pos.z += shift;
        }
        foodPos_.x += shift;
        foodPos_.z += shift;

        // Shift enemies
        for (auto& enemy : enemies_) {
            enemy.pos.x += shift;
            enemy.pos.z += shift;
            enemy.visualX += static_cast<float>(shift);
            enemy.visualZ += static_cast<float>(shift);
            // Also shift patrol zone
            enemy.patrolZone.min.x += shift;
            enemy.patrolZone.min.z += shift;
            enemy.patrolZone.max.x += shift;
            enemy.patrolZone.max.z += shift;
        }

        // Shift food pickups
        for (auto& pickup : foodPickups_) {
            pickup.pos.x += shift;
            pickup.pos.z += shift;
        }

        // Shift detached segments
        for (auto& segment : detachedSegments_) {
            segment.pos.x += shift;
            segment.pos.z += shift;
        }

        gridSize_ = targetGridSize_;

        // Update level offset and rebuild visible obstacles from level data
        updateLevelOffset();
        rebuildVisibleObstacles();

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

        // Update screen shake
        if (screenShakeTimer_ > 0.0f) {
            screenShakeTimer_ -= dt;
            float progress = screenShakeTimer_ / screenShakeDuration_;
            float currentIntensity = screenShakeIntensity_ * progress;

            // Random shake offset
            std::uniform_real_distribution<float> shakeDist(-1.0f, 1.0f);
            screenShakeOffset_ = {
                shakeDist(rng_) * currentIntensity,
                shakeDist(rng_) * currentIntensity * 0.5f,
                shakeDist(rng_) * currentIntensity
            };
        } else {
            screenShakeOffset_ = {0.0f, 0.0f, 0.0f};
        }

        // Update food pop effect
        if (foodPopTimer_ > 0.0f) {
            foodPopTimer_ -= dt;
            foodPopScale_ = foodPopTimer_ / 0.3f;  // 0.3 second pop duration
        }
    }

    void triggerScreenShake(float intensity, float duration) {
        screenShakeIntensity_ = intensity;
        screenShakeDuration_ = duration;
        screenShakeTimer_ = duration;
    }

    void triggerFoodPop(const bestow::Vec3& pos) {
        lastFoodPos_ = pos;
        foodPopTimer_ = 0.3f;
        foodPopScale_ = 1.0f;
    }

    //======================================================================
    // Input Handling - now buffered and responsive
    //======================================================================

    void handleInput() {
        if (!input_) return;

        // Handle main menu phase
        if (currentPhase_ == GamePhase::MainMenu) {
            handleMainMenuInput();
            return;
        }

        // Handle pause menu phase
        if (currentPhase_ == GamePhase::Paused) {
            handlePauseMenuInput();
            return;
        }

        // Handle world map phase
        if (currentPhase_ == GamePhase::WorldMap) {
            handleWorldMapInput();
            return;
        }

        // Handle level complete phase
        if (currentPhase_ == GamePhase::LevelComplete) {
            if (input_->wasKeyJustPressed(GLFW_KEY_ENTER) ||
                input_->wasKeyJustPressed(GLFW_KEY_SPACE)) {
                // Accumulate segments for boss fight
                totalSegmentsInWorld_ += segmentsEarnedThisLevel_;
                proceedToNextLevel();
            }
            if (input_->wasKeyJustPressed(GLFW_KEY_ESCAPE)) {
                returnToWorldMap();
            }
            return;
        }

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

        // ESC to pause
        if (input_->wasKeyJustPressed(GLFW_KEY_ESCAPE)) {
            pauseMenuSelection_ = 0;
            playSound(soundPause_);
            transitionTo(GamePhase::Paused);
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

        // Check self-collision - now triggers ring attack instead of death!
        RingResult ring = detectRing(newHead);
        if (ring.formed) {
            // Execute ring attack (damage enemies inside)
            executeRingAttack(ring);
            // The snake continues - ring attack is not death
        }

        // Check obstacle collision
        if (isObstacleAt(newHead)) {
            explodeSnake();
            transitionTo(GamePhase::GameOver);
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
            foodCollected_++;

            // Visual feedback for eating
            triggerFoodPop(gridToWorld(foodPos_));
            triggerScreenShake(0.1f, 0.1f);  // Small satisfying bump
            playSound(soundEat_);

            // Add the segment first
            snake_.insert(snake_.begin(), newSegment);

            // Check for level completion
            if (foodCollected_ >= foodRequired_) {
                std::cerr << "Level complete! Food collected: " << foodCollected_ << "/" << foodRequired_ << "\n";
                completeLevel();
                return;
            }

            // Expand the grid to reveal more of the level
            startExpansion();

            spawnFood();
            // Speed up! (decrease interval, but not below minimum)
            moveInterval_ = std::max(MIN_MOVE_INTERVAL, moveInterval_ - SPEED_INCREASE);
        } else {
            // Inherit color from previous head (slight fade)
            newSegment.color = snake_[0].color;
            snake_.insert(snake_.begin(), newSegment);
            snake_.pop_back();
        }
    }

    void spawnFood() {
        // Minimum distance from previous food position (in grid cells)
        constexpr int MIN_FOOD_DISTANCE = 5;
        GridPos previousFood = foodPos_;  // Save current position before spawning new

        // Try level-defined spawn points first (transformed to current grid coords)
        if (!currentLevel_.foodSpawnPoints.empty()) {
            // Gather and shuffle spawn points
            std::vector<GridPos> candidates;
            for (const auto& levelPos : currentLevel_.foodSpawnPoints) {
                // Transform from level coords to current grid coords
                GridPos gridPos = {
                    levelPos.x - levelOffsetX_,
                    levelPos.z - levelOffsetZ_
                };
                // Only include if within current grid
                if (gridPos.x >= 0 && gridPos.x < gridSize_ &&
                    gridPos.z >= 0 && gridPos.z < gridSize_) {
                    candidates.push_back(gridPos);
                }
            }

            std::shuffle(candidates.begin(), candidates.end(), rng_);

            // First pass: try to find spawn point that meets distance requirement
            for (const auto& pos : candidates) {
                int dx = pos.x - previousFood.x;
                int dz = pos.z - previousFood.z;
                int distSq = dx * dx + dz * dz;

                if (distSq >= MIN_FOOD_DISTANCE * MIN_FOOD_DISTANCE &&
                    isValidFoodPosition(pos) && isFoodReachable(pos)) {
                    foodPos_ = pos;
                    foodColor_ = generateRandomColor();
                    return;
                }
            }

            // Second pass: relax distance requirement if no suitable spawn found
            for (const auto& pos : candidates) {
                if (isValidFoodPosition(pos) && isFoodReachable(pos)) {
                    foodPos_ = pos;
                    foodColor_ = generateRandomColor();
                    return;
                }
            }
        }

        // Fallback: random position with edge margin and reachability check
        std::uniform_int_distribution<> posDist(1, gridSize_ - 2);  // 1-cell edge margin
        int attempts = 0;
        constexpr int MAX_ATTEMPTS = 100;

        // Try to find a position that meets distance requirement
        do {
            GridPos candidate = {posDist(rng_), posDist(rng_)};
            int dx = candidate.x - previousFood.x;
            int dz = candidate.z - previousFood.z;
            int distSq = dx * dx + dz * dz;

            if (distSq >= MIN_FOOD_DISTANCE * MIN_FOOD_DISTANCE &&
                isValidFoodPosition(candidate) && isFoodReachable(candidate)) {
                foodPos_ = candidate;
                foodColor_ = generateRandomColor();
                return;
            }
            attempts++;
        } while (attempts < MAX_ATTEMPTS);

        // Fallback: any valid position
        attempts = 0;
        do {
            foodPos_ = {posDist(rng_), posDist(rng_)};
            attempts++;
        } while (attempts < MAX_ATTEMPTS &&
                 (!isValidFoodPosition(foodPos_) || !isFoodReachable(foodPos_)));

        // If still no valid position, relax constraints (just avoid snake/obstacles)
        if (attempts >= MAX_ATTEMPTS) {
            std::uniform_int_distribution<> fullDist(0, gridSize_ - 1);
            do {
                foodPos_ = {fullDist(rng_), fullDist(rng_)};
            } while (isSnakeAt(foodPos_) || isObstacleAt(foodPos_) || isEnemyAt(foodPos_));
        }

        foodColor_ = generateRandomColor();
    }

    bool isValidFoodPosition(const GridPos& pos) const {
        // Not on snake
        if (isSnakeAt(pos)) return false;
        // Not on obstacle/wall
        if (isObstacleAt(pos)) return false;
        // Not on enemy
        if (isEnemyAt(pos)) return false;
        // Edge margin: at least 1 cell from boundary
        if (pos.x <= 0 || pos.x >= gridSize_ - 1) return false;
        if (pos.z <= 0 || pos.z >= gridSize_ - 1) return false;
        return true;
    }

    bool isFoodReachable(const GridPos& target) const {
        if (snake_.empty()) return true;

        // BFS from snake head to target
        const GridPos& start = snake_[0].pos;
        if (start == target) return true;

        std::vector<std::vector<bool>> visited(gridSize_, std::vector<bool>(gridSize_, false));
        std::queue<GridPos> queue;

        queue.push(start);
        visited[start.x][start.z] = true;

        // Mark snake body and obstacles as blocked
        for (const auto& segment : snake_) {
            if (segment.pos.x >= 0 && segment.pos.x < gridSize_ &&
                segment.pos.z >= 0 && segment.pos.z < gridSize_) {
                visited[segment.pos.x][segment.pos.z] = true;
            }
        }
        for (const auto& obstacle : obstacles_) {
            if (obstacle.x >= 0 && obstacle.x < gridSize_ &&
                obstacle.z >= 0 && obstacle.z < gridSize_) {
                visited[obstacle.x][obstacle.z] = true;
            }
        }
        // Reset start so BFS can begin
        visited[start.x][start.z] = false;
        queue.push(start);
        visited[start.x][start.z] = true;

        while (!queue.empty()) {
            GridPos current = queue.front();
            queue.pop();

            // Check all 4 directions
            for (int d = 0; d < 4; ++d) {
                Direction dir = static_cast<Direction>(d);
                GridPos offset = directionToOffset(dir);
                GridPos next = current + offset;

                // Handle wrap-around
                if (next.x < 0) next.x = gridSize_ - 1;
                if (next.x >= gridSize_) next.x = 0;
                if (next.z < 0) next.z = gridSize_ - 1;
                if (next.z >= gridSize_) next.z = 0;

                if (next == target) return true;

                if (!visited[next.x][next.z]) {
                    visited[next.x][next.z] = true;
                    queue.push(next);
                }
            }
        }

        return false;  // Target not reachable
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

    //======================================================================
    // Level Progression
    //======================================================================

    void completeLevel() {
        // Record progress
        updateWorldProgress();

        // Play victory sound
        playSound(soundLevelComplete_);

        // Transition to level complete phase
        transitionTo(GamePhase::LevelComplete);

        // Store the snake length for potential boss fight accumulation
        segmentsEarnedThisLevel_ = static_cast<int>(snake_.size());
    }

    void updateWorldProgress() {
        // Ensure we have progress for this world
        while (saveData_.worldProgress.size() <= static_cast<size_t>(currentWorldIndex_)) {
            WorldProgress wp;
            wp.levelsCompleted.resize(4, false);  // Assume 4 levels per world
            wp.foodCollected.resize(4, 0);
            saveData_.worldProgress.push_back(wp);
        }

        auto& worldProg = saveData_.worldProgress[currentWorldIndex_];

        // Mark level as completed
        if (currentLevelIndex_ < static_cast<int>(worldProg.levelsCompleted.size())) {
            worldProg.levelsCompleted[currentLevelIndex_] = true;
            worldProg.foodCollected[currentLevelIndex_] = foodCollected_;
        }

        // Update total food earned in world
        worldProg.totalFoodEarned = 0;
        for (int food : worldProg.foodCollected) {
            worldProg.totalFoodEarned += food;
        }
    }

    void proceedToNextLevel() {
        currentLevelIndex_++;

        // Check if we've completed all levels in the world
        if (currentLevelIndex_ >= static_cast<int>(currentWorld_.levelFiles.size())) {
            // World complete! Go to world map
            transitionTo(GamePhase::WorldMap);
            return;
        }

        // Load next level
        std::string levelPath = "data/worlds/world1/" + currentWorld_.levelFiles[currentLevelIndex_];
        if (loadLevel(levelPath)) {
            applyLevelToGame();
            transitionTo(GamePhase::Playing);
        }
    }

    void returnToWorldMap() {
        transitionTo(GamePhase::WorldMap);
    }

    int getTotalFoodInCurrentWorld() const {
        if (currentWorldIndex_ < static_cast<int>(saveData_.worldProgress.size())) {
            return saveData_.worldProgress[currentWorldIndex_].totalFoodEarned;
        }
        return 0;
    }

    bool isLevelUnlocked(int levelIndex) const {
        if (levelIndex == 0) return true;  // First level always unlocked

        if (levelIndex >= static_cast<int>(currentWorld_.unlockRequirements.size())) {
            return false;
        }

        int required = currentWorld_.unlockRequirements[levelIndex];
        return getTotalFoodInCurrentWorld() >= required;
    }

    //======================================================================
    // World Map
    //======================================================================

    void handleWorldMapInput() {
        if (!input_ || currentWorld_.nodes.empty()) return;

        // Find connected nodes from current selection
        std::vector<int> connected;
        for (const auto& [a, b] : currentWorld_.paths) {
            if (a == selectedNodeIndex_) connected.push_back(b);
            if (b == selectedNodeIndex_) connected.push_back(a);
        }

        // Navigation - find node in the pressed direction
        const auto& currentNode = currentWorld_.nodes[selectedNodeIndex_];

        auto findNodeInDirection = [&](int dx, int dz) -> int {
            int bestNode = -1;
            float bestDist = 999999.0f;
            for (int idx : connected) {
                const auto& node = currentWorld_.nodes[idx];
                int nodeDx = node.gridPos.x - currentNode.gridPos.x;
                int nodeDz = node.gridPos.z - currentNode.gridPos.z;

                // Check if the node is roughly in the desired direction
                bool matchesX = (dx == 0) || (dx > 0 && nodeDx > 0) || (dx < 0 && nodeDx < 0);
                bool matchesZ = (dz == 0) || (dz > 0 && nodeDz > 0) || (dz < 0 && nodeDz < 0);

                // If both match, or if we're only checking one direction
                if ((dx != 0 && matchesX) || (dz != 0 && matchesZ)) {
                    float dist = std::abs(nodeDx) + std::abs(nodeDz);
                    if (dist < bestDist) {
                        bestDist = dist;
                        bestNode = idx;
                    }
                }
            }
            return bestNode;
        };

        int prevSelection = selectedNodeIndex_;

        // Up: Comma (Dvorak W) or Up Arrow - decrease Z (forward in isometric view)
        if (input_->wasKeyJustPressed(GLFW_KEY_COMMA) ||
            input_->wasKeyJustPressed(GLFW_KEY_UP)) {
            int node = findNodeInDirection(0, -1);
            if (node >= 0) selectedNodeIndex_ = node;
        }
        // Down: O (Dvorak S) or Down Arrow - increase Z
        if (input_->wasKeyJustPressed(GLFW_KEY_O) ||
            input_->wasKeyJustPressed(GLFW_KEY_DOWN)) {
            int node = findNodeInDirection(0, 1);
            if (node >= 0) selectedNodeIndex_ = node;
        }
        // Left: A or Left Arrow
        if (input_->wasKeyJustPressed(GLFW_KEY_A) ||
            input_->wasKeyJustPressed(GLFW_KEY_LEFT)) {
            int node = findNodeInDirection(-1, 0);
            if (node >= 0) selectedNodeIndex_ = node;
        }
        // Right: E (Dvorak D) or Right Arrow
        if (input_->wasKeyJustPressed(GLFW_KEY_E) ||
            input_->wasKeyJustPressed(GLFW_KEY_RIGHT)) {
            int node = findNodeInDirection(1, 0);
            if (node >= 0) selectedNodeIndex_ = node;
        }

        // Play sound if selection changed
        if (selectedNodeIndex_ != prevSelection) {
            playSound(soundMenuMove_);
        }

        // Select level with Enter/Space
        if (input_->wasKeyJustPressed(GLFW_KEY_ENTER) ||
            input_->wasKeyJustPressed(GLFW_KEY_SPACE)) {
            const auto& node = currentWorld_.nodes[selectedNodeIndex_];
            if (node.levelIndex >= 0 && isLevelUnlocked(node.levelIndex)) {
                playSound(soundMenuSelect_);
                currentLevelIndex_ = node.levelIndex;
                std::string levelPath = "data/worlds/world1/" + currentWorld_.levelFiles[currentLevelIndex_];
                if (loadLevel(levelPath)) {
                    applyLevelToGame();
                    transitionTo(GamePhase::Playing);
                }
            }
        }

        // Exit to main menu with Escape
        if (input_->wasKeyJustPressed(GLFW_KEY_ESCAPE)) {
            transitionTo(GamePhase::MainMenu);
        }
    }

    void handleMainMenuInput() {
        // Navigate with Up/Down (Dvorak: Comma/O)
        if (input_->wasKeyJustPressed(GLFW_KEY_COMMA) ||
            input_->wasKeyJustPressed(GLFW_KEY_UP)) {
            mainMenuSelection_ = (mainMenuSelection_ - 1 + MAIN_MENU_COUNT) % MAIN_MENU_COUNT;
            playSound(soundMenuMove_);
        }
        if (input_->wasKeyJustPressed(GLFW_KEY_O) ||
            input_->wasKeyJustPressed(GLFW_KEY_DOWN)) {
            mainMenuSelection_ = (mainMenuSelection_ + 1) % MAIN_MENU_COUNT;
            playSound(soundMenuMove_);
        }

        // Select with Enter/Space
        if (input_->wasKeyJustPressed(GLFW_KEY_ENTER) ||
            input_->wasKeyJustPressed(GLFW_KEY_SPACE)) {
            playSound(soundMenuSelect_);
            if (mainMenuSelection_ == MAIN_MENU_PLAY) {
                transitionTo(GamePhase::WorldMap);
            } else if (mainMenuSelection_ == MAIN_MENU_QUIT) {
                running_ = false;
            }
        }
    }

    void handlePauseMenuInput() {
        // Navigate with Up/Down (Dvorak: Comma/O)
        if (input_->wasKeyJustPressed(GLFW_KEY_COMMA) ||
            input_->wasKeyJustPressed(GLFW_KEY_UP)) {
            pauseMenuSelection_ = (pauseMenuSelection_ - 1 + PAUSE_MENU_COUNT) % PAUSE_MENU_COUNT;
            playSound(soundMenuMove_);
        }
        if (input_->wasKeyJustPressed(GLFW_KEY_O) ||
            input_->wasKeyJustPressed(GLFW_KEY_DOWN)) {
            pauseMenuSelection_ = (pauseMenuSelection_ + 1) % PAUSE_MENU_COUNT;
            playSound(soundMenuMove_);
        }

        // Select with Enter/Space
        if (input_->wasKeyJustPressed(GLFW_KEY_ENTER) ||
            input_->wasKeyJustPressed(GLFW_KEY_SPACE)) {
            playSound(soundMenuSelect_);
            if (pauseMenuSelection_ == PAUSE_MENU_RESUME) {
                transitionTo(GamePhase::Playing);
            } else if (pauseMenuSelection_ == PAUSE_MENU_RESTART) {
                restartGame();
                transitionTo(GamePhase::Playing);
            } else if (pauseMenuSelection_ == PAUSE_MENU_QUIT) {
                transitionTo(GamePhase::WorldMap);
            }
        }

        // ESC to resume
        if (input_->wasKeyJustPressed(GLFW_KEY_ESCAPE)) {
            playSound(soundMenuSelect_);
            transitionTo(GamePhase::Playing);
        }
    }

    void updateWorldMap(float dt) {
        // Animate cursor bob
        worldMapCursorBob_ += dt * 4.0f;

        // Update camera to look at selected node
        if (!currentWorld_.nodes.empty() && selectedNodeIndex_ < static_cast<int>(currentWorld_.nodes.size())) {
            const auto& node = currentWorld_.nodes[selectedNodeIndex_];
            float targetX = static_cast<float>(node.gridPos.x);
            float targetZ = static_cast<float>(node.gridPos.z);

            // Smooth camera follow
            worldMapCameraTarget_.x += (targetX - worldMapCameraTarget_.x) * dt * 3.0f;
            worldMapCameraTarget_.z += (targetZ - worldMapCameraTarget_.z) * dt * 3.0f;
        }
    }

    void drawWorldMap() {
        // Set up isometric camera for world map
        bestow::Camera3D cam = graphics_->getCamera();
        float angleRad = glm::radians(45.0f);
        bestow::Vec3 cameraPos = {
            worldMapCameraTarget_.x + worldMapCameraDistance_ * std::sin(angleRad),
            worldMapCameraDistance_ * 0.8f,  // Height
            worldMapCameraTarget_.z + worldMapCameraDistance_ * std::cos(angleRad)
        };
        cam.transform.position = cameraPos;
        cam.fovY = 45.0f;
        cam.aspectRatio = 16.0f / 9.0f;
        cam.nearPlane = 0.1f;
        cam.farPlane = 100.0f;

        // Calculate rotation to look at target
        glm::vec3 lookDir = glm::normalize(glm::vec3(
            worldMapCameraTarget_.x - cameraPos.x,
            worldMapCameraTarget_.y - cameraPos.y,
            worldMapCameraTarget_.z - cameraPos.z
        ));
        glm::vec3 up(0.0f, 1.0f, 0.0f);
        glm::quat rotation = glm::quatLookAt(lookDir, up);
        cam.transform.rotation = {rotation.w, rotation.x, rotation.y, rotation.z};

        graphics_->setCamera(cam);

        // Draw ground plane (larger for world map)
        bestow::PBRMaterial groundMat;
        groundMat.baseColorFactor = {0.2f, 0.5f, 0.25f, 1.0f};  // Forest green
        groundMat.roughnessFactor = 0.9f;
        auto groundMatResult = graphics_->createMaterial(groundMat);

        bestow::Transform3D groundTransform;
        groundTransform.position = {worldMapCameraTarget_.x, -0.1f, worldMapCameraTarget_.z};
        groundTransform.scale = {50.0f, 0.1f, 50.0f};
        if (groundMatResult) {
            graphics_->drawMesh(cubeMesh_, *groundMatResult, groundTransform, true, true);
        }

        // Draw paths between nodes
        bestow::Color pathColor{139, 90, 43, 255};  // Brown for paths
        for (const auto& [a, b] : currentWorld_.paths) {
            if (a >= static_cast<int>(currentWorld_.nodes.size()) ||
                b >= static_cast<int>(currentWorld_.nodes.size())) continue;

            const auto& nodeA = currentWorld_.nodes[a];
            const auto& nodeB = currentWorld_.nodes[b];

            bestow::Vec3 posA{static_cast<float>(nodeA.gridPos.x), 0.05f, static_cast<float>(nodeA.gridPos.z)};
            bestow::Vec3 posB{static_cast<float>(nodeB.gridPos.x), 0.05f, static_cast<float>(nodeB.gridPos.z)};

            // Draw thicker path by drawing multiple lines
            for (float offset = -0.1f; offset <= 0.1f; offset += 0.05f) {
                graphics_->debugDrawLine(
                    {posA.x + offset, posA.y, posA.z},
                    {posB.x + offset, posB.y, posB.z},
                    pathColor, 0.0f, false
                );
            }
        }

        // Draw nodes
        for (size_t i = 0; i < currentWorld_.nodes.size(); ++i) {
            const auto& node = currentWorld_.nodes[i];
            float x = static_cast<float>(node.gridPos.x);
            float z = static_cast<float>(node.gridPos.z);
            bool isSelected = (static_cast<int>(i) == selectedNodeIndex_);
            bool unlocked = isLevelUnlocked(node.levelIndex);

            // Node base (platform)
            bestow::Transform3D nodeTransform;
            nodeTransform.position = {x, 0.1f, z};
            nodeTransform.scale = {0.8f, 0.2f, 0.8f};

            bestow::PBRMaterial nodeMat;
            if (node.isBoss) {
                nodeMat.baseColorFactor = {0.6f, 0.1f, 0.1f, 1.0f};  // Red for boss
            } else if (!unlocked) {
                nodeMat.baseColorFactor = {0.3f, 0.3f, 0.3f, 1.0f};  // Gray for locked
            } else if (node.isCompleted) {
                nodeMat.baseColorFactor = {0.2f, 0.6f, 0.2f, 1.0f};  // Green for completed
            } else {
                nodeMat.baseColorFactor = {0.5f, 0.4f, 0.2f, 1.0f};  // Brown for available
            }
            nodeMat.roughnessFactor = 0.6f;

            auto nodeMatResult = graphics_->createMaterial(nodeMat);
            if (nodeMatResult) {
                graphics_->drawMesh(cubeMesh_, *nodeMatResult, nodeTransform, true, true);
            }

            // Draw level indicator on top
            if (node.levelIndex >= 0) {
                bestow::Transform3D indicatorTransform;
                float bobY = isSelected ? 0.5f + std::sin(worldMapCursorBob_) * 0.15f : 0.4f;
                indicatorTransform.position = {x, bobY, z};
                indicatorTransform.scale = {0.3f, 0.3f, 0.3f};

                bestow::PBRMaterial indicatorMat;
                if (node.isBoss) {
                    indicatorMat.baseColorFactor = {1.0f, 0.3f, 0.3f, 1.0f};
                    indicatorMat.emissiveFactor = {0.5f, 0.1f, 0.1f};
                } else if (!unlocked) {
                    indicatorMat.baseColorFactor = {0.5f, 0.5f, 0.5f, 1.0f};
                } else {
                    indicatorMat.baseColorFactor = {1.0f, 0.85f, 0.0f, 1.0f};  // Gold
                    indicatorMat.emissiveFactor = {0.3f, 0.25f, 0.0f};
                }

                if (isSelected) {
                    indicatorMat.emissiveFactor = {0.5f, 0.5f, 0.5f};  // Glow when selected
                }

                auto indicatorMatResult = graphics_->createMaterial(indicatorMat);
                if (indicatorMatResult) {
                    graphics_->drawMesh(cubeMesh_, *indicatorMatResult, indicatorTransform, true, true);
                }
            }

            // Selection ring for selected node
            if (isSelected) {
                float ringRadius = 0.6f;
                bestow::Color ringColor{255, 255, 100, 255};
                int segments = 16;
                for (int s = 0; s < segments; ++s) {
                    float angle1 = (static_cast<float>(s) / segments) * 3.14159f * 2.0f;
                    float angle2 = (static_cast<float>(s + 1) / segments) * 3.14159f * 2.0f;
                    graphics_->debugDrawLine(
                        {x + std::cos(angle1) * ringRadius, 0.05f, z + std::sin(angle1) * ringRadius},
                        {x + std::cos(angle2) * ringRadius, 0.05f, z + std::sin(angle2) * ringRadius},
                        ringColor, 0.0f, false
                    );
                }
            }
        }

        // Draw HUD info for selected node
        if (!currentWorld_.nodes.empty() && selectedNodeIndex_ < static_cast<int>(currentWorld_.nodes.size())) {
            const auto& node = currentWorld_.nodes[selectedNodeIndex_];
            bool unlocked = isLevelUnlocked(node.levelIndex);

            // Draw unlock requirement indicator if locked
            if (!unlocked && node.levelIndex < static_cast<int>(currentWorld_.unlockRequirements.size())) {
                int required = currentWorld_.unlockRequirements[node.levelIndex];
                int current = getTotalFoodInCurrentWorld();

                // Draw a progress bar above the camera
                float barWidth = 3.0f;
                float progress = static_cast<float>(current) / static_cast<float>(required);
                progress = std::min(1.0f, progress);

                float barX = worldMapCameraTarget_.x;
                float barY = 3.0f;
                float barZ = worldMapCameraTarget_.z - 2.0f;

                // Background
                bestow::Color bgColor{60, 60, 60, 255};
                graphics_->debugDrawLine({barX - barWidth/2, barY, barZ}, {barX + barWidth/2, barY, barZ}, bgColor, 0.0f, false);

                // Progress
                bestow::Color progressColor{100, 200, 100, 255};
                graphics_->debugDrawLine(
                    {barX - barWidth/2, barY, barZ},
                    {barX - barWidth/2 + barWidth * progress, barY, barZ},
                    progressColor, 0.0f, false
                );
            }
        }

        // Title indicator
        bestow::Color titleColor{255, 255, 200, 255};
        float titleY = 4.0f;
        graphics_->debugDrawLine(
            {worldMapCameraTarget_.x - 2.0f, titleY, worldMapCameraTarget_.z - 3.0f},
            {worldMapCameraTarget_.x + 2.0f, titleY, worldMapCameraTarget_.z - 3.0f},
            titleColor, 0.0f, false
        );
    }

    void initializeWorldMap() {
        // Update node states from save data
        if (currentWorldIndex_ < static_cast<int>(saveData_.worldProgress.size())) {
            const auto& progress = saveData_.worldProgress[currentWorldIndex_];
            for (size_t i = 0; i < currentWorld_.nodes.size(); ++i) {
                auto& node = currentWorld_.nodes[i];
                node.isUnlocked = isLevelUnlocked(node.levelIndex);
                if (node.levelIndex >= 0 && node.levelIndex < static_cast<int>(progress.levelsCompleted.size())) {
                    node.isCompleted = progress.levelsCompleted[node.levelIndex];
                }
            }
        }

        // Start at first unlocked but incomplete level, or first level
        selectedNodeIndex_ = 0;
        for (size_t i = 0; i < currentWorld_.nodes.size(); ++i) {
            const auto& node = currentWorld_.nodes[i];
            if (node.isUnlocked && !node.isCompleted) {
                selectedNodeIndex_ = static_cast<int>(i);
                break;
            }
        }
    }

    void restartGame() {
        // Reset animation state
        visualGridSize_ = static_cast<float>(gridSize_);
        currentCameraDistance_ = BASE_CAMERA_DISTANCE + (gridSize_ - INITIAL_GRID_SIZE) * CAMERA_SCALE;
        currentCameraHeight_ = BASE_CAMERA_HEIGHT + (gridSize_ - INITIAL_GRID_SIZE) * CAMERA_SCALE * 0.8f;
        isExpanding_ = false;
        expansionTimer_ = 0.0f;

        // Clear transient state
        detachedSegments_.clear();
        foodPickups_.clear();

        // Reset input state
        direction_ = Direction::Right;
        nextDirection_ = Direction::Right;
        inputDirection_ = Direction::Right;
        hasBufferedInput_ = false;

        // Reload the current level
        applyLevelToGame();
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

        // Apply screen shake offset
        cameraPos.x += screenShakeOffset_.x;
        cameraPos.y += screenShakeOffset_.y;
        cameraPos.z += screenShakeOffset_.z;

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

    // Float version for smooth interpolated positions
    bestow::Vec3 gridToWorldFloat(float x, float z) const {
        float halfGrid = gridSize_ * CELL_SIZE * 0.5f;
        return {
            x * CELL_SIZE - halfGrid + CELL_SIZE * 0.5f,
            CELL_SIZE * 0.5f,
            z * CELL_SIZE - halfGrid + CELL_SIZE * 0.5f
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

    void drawEnemies() {
        if (!cubeMesh_) return;

        for (const auto& enemy : enemies_) {
            // Use interpolated visual position for smooth movement
            bestow::Vec3 worldPos = gridToWorldFloat(enemy.visualX, enemy.visualZ);

            // Enemy animation - slight bobbing
            float bob = 0.05f * std::sin(gameTime_ * 3.0f + enemy.visualX * 0.5f);

            bestow::Mat4 transform = glm::translate(glm::identity<glm::mat4>(),
                glm::vec3(worldPos.x, worldPos.y + bob, worldPos.z));

            // Slightly smaller than snake segments
            transform = glm::scale(transform, glm::vec3(0.8f));

            // Enemy color: red/purple, flashing white when damaged
            bestow::PBRMaterial mat;
            if (enemy.isDamaged) {
                // Flash white when damaged
                float flash = std::sin(enemy.damageFlashTimer * 30.0f) > 0 ? 1.0f : 0.0f;
                mat.baseColorFactor = {1.0f, flash, flash, 1.0f};
            } else {
                // Normal color: red-purple
                mat.baseColorFactor = {0.8f, 0.2f, 0.3f, 1.0f};
            }
            mat.roughnessFactor = 0.5f;
            mat.metallicFactor = 0.2f;

            auto matResult = graphics_->createMaterial(mat);
            if (matResult) {
                graphics_->drawMesh(cubeMesh_, *matResult, transform, true, true);
            }

            // Draw health bar if visible
            if (enemy.showHealthBar && enemy.maxHealth > 1) {
                drawEnemyHealthBar(enemy, worldPos);
            }
        }
    }

    void drawEnemyHealthBar(const Enemy& enemy, const bestow::Vec3& worldPos) {
        // Health bar above enemy
        float barWidth = CELL_SIZE * 0.8f;
        float barHeight = worldPos.y + CELL_SIZE * 0.8f;
        float healthPercent = static_cast<float>(enemy.currentHealth) / static_cast<float>(enemy.maxHealth);

        // Background (dark red)
        bestow::Color bgColor{80, 20, 20, 255};
        // Foreground (green to yellow to red based on health)
        uint8_t r = static_cast<uint8_t>((1.0f - healthPercent) * 255);
        uint8_t g = static_cast<uint8_t>(healthPercent * 255);
        bestow::Color fgColor{r, g, 0, 255};

        // Draw background bar
        float halfBar = barWidth * 0.5f;
        graphics_->debugDrawLine(
            {worldPos.x - halfBar, barHeight, worldPos.z},
            {worldPos.x + halfBar, barHeight, worldPos.z},
            bgColor, 0.0f, false
        );

        // Draw foreground (health remaining)
        float healthWidth = barWidth * healthPercent;
        graphics_->debugDrawLine(
            {worldPos.x - halfBar, barHeight + 0.02f, worldPos.z},
            {worldPos.x - halfBar + healthWidth, barHeight + 0.02f, worldPos.z},
            fgColor, 0.0f, false
        );
    }

    void drawDetachedSegments() {
        if (!cubeMesh_) return;

        for (const auto& segment : detachedSegments_) {
            bestow::Vec3 worldPos;
            float scale;
            float spin;

            if (segment.isParticle) {
                // Particles use their own worldPos directly (updated by physics)
                worldPos = segment.worldPos;

                // Only shrink in the last second of lifetime
                float fadeStart = 1.0f;
                if (segment.timer < fadeStart) {
                    scale = segment.scale * (segment.timer / fadeStart);
                } else {
                    scale = segment.scale;
                }

                // Use angular velocity for proper tumbling
                // Accumulate rotation based on time alive (approximate)
                float timeAlive = 3.0f - segment.timer;  // Rough estimate
                spin = segment.angularVel * timeAlive;
            } else {
                // Regular detached segments use grid position with velocity offset
                worldPos = gridToWorld(segment.pos);

                float progress = 1.0f - (segment.timer / DETACH_ANIMATION_TIME);
                worldPos.x += segment.velocity.x * progress * 0.3f;
                worldPos.y += segment.velocity.y * progress * 0.3f;
                worldPos.z += segment.velocity.z * progress * 0.3f;

                scale = (segment.timer / DETACH_ANIMATION_TIME) * 0.85f;
                spin = progress * 20.0f;
            }

            bestow::Mat4 transform = glm::translate(glm::identity<glm::mat4>(),
                glm::vec3(worldPos.x, worldPos.y, worldPos.z));
            transform = glm::rotate(transform, spin, glm::vec3(0.3f, 1.0f, 0.5f));
            transform = glm::scale(transform, glm::vec3(scale));

            // Color: particles keep their original color, regular segments flash
            bestow::PBRMaterial mat;
            if (segment.isParticle) {
                // Particles keep segment color, fade to ember glow near end
                float fadeStart = 1.0f;
                float colorAlpha = segment.timer < fadeStart ? (segment.timer / fadeStart) : 1.0f;
                float emberBlend = 1.0f - colorAlpha;
                mat.baseColorFactor = {
                    segment.color.x * colorAlpha + 0.8f * emberBlend,
                    segment.color.y * colorAlpha + 0.2f * emberBlend,
                    segment.color.z * colorAlpha + 0.1f * emberBlend,
                    1.0f
                };
                mat.emissiveFactor = {0.2f * emberBlend, 0.05f * emberBlend, 0.0f};  // Glowing embers at end
            } else if (segment.willShatter) {
                float progress = 1.0f - (segment.timer / DETACH_ANIMATION_TIME);
                float flash = std::sin(progress * 30.0f) > 0 ? 1.0f : 0.3f;
                mat.baseColorFactor = {flash, 0.1f, 0.1f, 1.0f};
            } else {
                mat.baseColorFactor = segment.color;
            }
            mat.roughnessFactor = 0.4f;
            mat.metallicFactor = 0.1f;
            if (!segment.isParticle) {
                mat.emissiveFactor = {mat.baseColorFactor.x * 0.2f,
                                      mat.baseColorFactor.y * 0.2f,
                                      mat.baseColorFactor.z * 0.2f};
            }

            auto matResult = graphics_->createMaterial(mat);
            if (matResult) {
                graphics_->drawMesh(cubeMesh_, *matResult, transform, true, true);
            }
        }
    }

    void drawFoodPickups() {
        if (!cubeMesh_) return;

        for (const auto& pickup : foodPickups_) {
            bestow::Vec3 worldPos = gridToWorld(pickup.pos);

            // Spawn animation: pop in and gentle bounce
            float age = gameTime_ - pickup.spawnTime;
            float spawnScale = std::min(1.0f, age * 5.0f);  // Pop in over 0.2s
            float bounce = 0.1f * std::abs(std::sin(age * 5.0f));

            bestow::Mat4 transform = glm::translate(glm::identity<glm::mat4>(),
                glm::vec3(worldPos.x, worldPos.y + bounce, worldPos.z));
            transform = glm::scale(transform, glm::vec3(spawnScale * 0.7f));

            // Slightly different from regular food - more sparkly
            bestow::PBRMaterial mat;
            mat.baseColorFactor = pickup.color;
            mat.roughnessFactor = 0.2f;
            mat.metallicFactor = 0.4f;
            mat.emissiveFactor = {pickup.color.x * 0.4f,
                                  pickup.color.y * 0.4f,
                                  pickup.color.z * 0.4f};

            auto matResult = graphics_->createMaterial(mat);
            if (matResult) {
                graphics_->drawMesh(cubeMesh_, *matResult, transform, true, true);
            }
        }
    }

    void drawFood() {
        if (!cubeMesh_) return;
        // Don't draw food when level is complete (it was just collected)
        if (currentPhase_ == GamePhase::LevelComplete) return;

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

    void drawFoodPopEffect() {
        if (foodPopTimer_ <= 0.0f) return;

        // Draw expanding burst lines from where food was collected
        float progress = 1.0f - foodPopScale_;  // 0 to 1 as effect progresses
        float radius = 0.3f + progress * 1.5f;  // Expanding radius
        float alpha = foodPopScale_;  // Fade out

        bestow::Color popColor{
            static_cast<uint8_t>(255 * alpha),
            static_cast<uint8_t>(200 * alpha),
            static_cast<uint8_t>(50 * alpha),
            255
        };

        // Draw 8 burst lines radiating outward
        constexpr int NUM_RAYS = 8;
        for (int i = 0; i < NUM_RAYS; ++i) {
            float angle = static_cast<float>(i) * (2.0f * 3.14159f / NUM_RAYS);
            float dx = std::cos(angle) * radius;
            float dz = std::sin(angle) * radius;

            float innerRadius = radius * 0.3f;
            float innerDx = std::cos(angle) * innerRadius;
            float innerDz = std::sin(angle) * innerRadius;

            graphics_->debugDrawLine(
                {lastFoodPos_.x + innerDx, lastFoodPos_.y + 0.5f, lastFoodPos_.z + innerDz},
                {lastFoodPos_.x + dx, lastFoodPos_.y + 0.5f, lastFoodPos_.z + dz},
                popColor, 0.0f, false
            );
        }
    }

    void drawHUD() {
        float halfGrid = gridSize_ * CELL_SIZE * 0.5f;
        float hudY = 0.05f;  // Just above ground
        float hudZ = -halfGrid - 0.5f;  // Behind the play area

        // Food progress bar (how many food collected vs required)
        float foodBarWidth = gridSize_ * CELL_SIZE * 0.6f;
        float foodProgress = static_cast<float>(foodCollected_) / static_cast<float>(foodRequired_);
        foodProgress = std::min(1.0f, foodProgress);

        // Background (dark)
        bestow::Color bgColor{40, 40, 40, 255};
        graphics_->debugDrawLine(
            {-foodBarWidth * 0.5f, hudY, hudZ},
            {foodBarWidth * 0.5f, hudY, hudZ},
            bgColor, 0.0f, false
        );

        // Foreground (gold/yellow for food)
        bestow::Color foodColor{255, 200, 50, 255};
        if (foodProgress > 0.0f) {
            graphics_->debugDrawLine(
                {-foodBarWidth * 0.5f, hudY + 0.02f, hudZ},
                {-foodBarWidth * 0.5f + foodBarWidth * foodProgress, hudY + 0.02f, hudZ},
                foodColor, 0.0f, false
            );
        }

        // Segment count indicator (snake length) - vertical bar on left side
        float segmentBarHeight = 3.0f;
        float segmentBarX = -halfGrid - 0.5f;
        float segmentProgress = std::min(1.0f, static_cast<float>(snake_.size()) / 20.0f);  // Cap at 20 for display

        // Background
        graphics_->debugDrawLine(
            {segmentBarX, hudY, -halfGrid},
            {segmentBarX, hudY + segmentBarHeight, -halfGrid},
            bgColor, 0.0f, false
        );

        // Foreground (green for snake)
        bestow::Color snakeColor{100, 255, 100, 255};
        if (segmentProgress > 0.0f) {
            graphics_->debugDrawLine(
                {segmentBarX - 0.02f, hudY, -halfGrid},
                {segmentBarX - 0.02f, hudY + segmentBarHeight * segmentProgress, -halfGrid},
                snakeColor, 0.0f, false
            );
        }

        // Draw segment count as small markers
        for (size_t i = 0; i < snake_.size() && i < 20; ++i) {
            float markerY = hudY + (static_cast<float>(i) / 20.0f) * segmentBarHeight;
            bestow::Color markerColor = (i == 0) ? bestow::Color{255, 255, 100, 255} : snakeColor;
            graphics_->debugDrawLine(
                {segmentBarX - 0.1f, markerY, -halfGrid},
                {segmentBarX + 0.05f, markerY, -halfGrid},
                markerColor, 0.0f, false
            );
        }

        // Level complete screen - draw relative to snake head so it's always visible
        if (currentPhase_ == GamePhase::LevelComplete && !snake_.empty()) {
            // Get snake head position for drawing victory visuals
            // Head is at front() - that's where new segments are inserted during movement
            float headX = snake_.front().pos.x * CELL_SIZE - gridSize_ * CELL_SIZE * 0.5f + CELL_SIZE * 0.5f;
            float headZ = snake_.front().pos.z * CELL_SIZE - gridSize_ * CELL_SIZE * 0.5f + CELL_SIZE * 0.5f;

            // Celebratory golden border - pulsing (around the whole grid)
            float flash = std::sin(gameTime_ * 4.0f) * 0.3f + 0.7f;
            uint8_t brightness = static_cast<uint8_t>(200 * flash + 55);
            bestow::Color victoryColor{brightness, static_cast<uint8_t>(brightness * 0.85f), 50, 255};

            // Draw multiple concentric borders for emphasis
            for (float offset = 0.0f; offset < 0.3f; offset += 0.1f) {
                float vBorder = halfGrid + 0.1f + offset;
                graphics_->debugDrawLine({-vBorder, 0.15f + offset, -vBorder}, {vBorder, 0.15f + offset, -vBorder}, victoryColor, 0.0f, false);
                graphics_->debugDrawLine({vBorder, 0.15f + offset, -vBorder}, {vBorder, 0.15f + offset, vBorder}, victoryColor, 0.0f, false);
                graphics_->debugDrawLine({vBorder, 0.15f + offset, vBorder}, {-vBorder, 0.15f + offset, vBorder}, victoryColor, 0.0f, false);
                graphics_->debugDrawLine({-vBorder, 0.15f + offset, vBorder}, {-vBorder, 0.15f + offset, -vBorder}, victoryColor, 0.0f, false);
            }

            // Draw checkmark ABOVE the snake head so it's always visible
            float checkSize = 2.0f;
            float checkY = 2.0f;  // High above the snake
            bestow::Color checkColor{100, 255, 100, 255};

            // Make the checkmark thicker by drawing multiple lines offset slightly
            for (float thickness = -0.05f; thickness <= 0.05f; thickness += 0.025f) {
                // Left part of check
                graphics_->debugDrawLine(
                    {headX - checkSize * 0.5f + thickness, checkY + thickness, headZ},
                    {headX, checkY - checkSize * 0.3f + thickness, headZ},
                    checkColor, 0.0f, false
                );
                // Right part of check
                graphics_->debugDrawLine(
                    {headX, checkY - checkSize * 0.3f + thickness, headZ},
                    {headX + checkSize * 0.7f + thickness, checkY + checkSize * 0.5f + thickness, headZ},
                    checkColor, 0.0f, false
                );
            }

            // Draw floating cubes around the snake head as celebration
            float starRadius = 1.5f;
            int numStars = 8;
            for (int i = 0; i < numStars; ++i) {
                float angle = (static_cast<float>(i) / numStars) * 3.14159f * 2.0f + gameTime_ * 2.0f;
                float starX = headX + std::cos(angle) * starRadius;
                float starZ = headZ + std::sin(angle) * starRadius;
                float starY = 0.5f + std::sin(gameTime_ * 3.0f + i) * 0.3f;

                bestow::Transform3D starTransform;
                starTransform.position = {starX, starY, starZ};
                starTransform.scale = {0.15f, 0.15f, 0.15f};

                bestow::PBRMaterial starMat;
                starMat.baseColorFactor = {1.0f, 0.84f, 0.0f, 1.0f};  // Gold
                starMat.roughnessFactor = 0.3f;
                starMat.metallicFactor = 0.8f;
                starMat.emissiveFactor = {0.5f, 0.4f, 0.0f};

                auto matResult = graphics_->createMaterial(starMat);
                if (matResult) {
                    graphics_->drawMesh(cubeMesh_, *matResult, starTransform, true, true);
                }
            }
        }

        // Game over indicator
        if (gameOver_) {
            // Red flashing X across the play area
            float flash = std::sin(gameTime_ * 4.0f) * 0.5f + 0.5f;
            uint8_t brightness = static_cast<uint8_t>(100 + flash * 155);
            bestow::Color deathColor{brightness, 30, 30, 255};

            float xSize = halfGrid * 0.7f;
            graphics_->debugDrawLine({-xSize, 0.5f, -xSize}, {xSize, 0.5f, xSize}, deathColor, 0.0f, false);
            graphics_->debugDrawLine({xSize, 0.5f, -xSize}, {-xSize, 0.5f, xSize}, deathColor, 0.0f, false);

            // Game Over text using pixel font
            drawPixelTextShadow("GAME OVER", 0.0f, 2.5f, 0.0f, 0.12f,
                                bestow::Color{255, 100, 100, 255});
            drawPixelTextShadow("PRESS O TO RESTART", 0.0f, 1.5f, 0.0f, 0.04f,
                                bestow::Color{200, 200, 200, 255});
        }

        // ==================== HUD TEXT LABELS (Pixel Font) ====================
        if (currentPhase_ == GamePhase::Playing && !gameOver_) {
            // Food counter text (above the food bar) - e.g., "3/5"
            std::string foodText = std::to_string(foodCollected_) + "/" + std::to_string(foodRequired_);
            drawPixelTextShadow(foodText, 0.0f, hudY + 0.6f, hudZ - 0.3f, 0.04f,
                                bestow::Color{255, 200, 50, 255});

            // Snake length counter (next to segment bar)
            std::string segmentText = std::to_string(snake_.size());
            drawPixelTextShadow(segmentText, segmentBarX - 0.3f, hudY + segmentBarHeight + 0.3f, -halfGrid, 0.03f,
                                bestow::Color{100, 255, 100, 255});

            // Level name (top of screen)
            if (!currentLevel_.name.empty()) {
                drawPixelTextShadow(currentLevel_.name, 0.0f, 3.0f, -halfGrid - 0.5f, 0.05f,
                                    bestow::Color{200, 200, 255, 255});
            }
        }
    }

    void drawMainMenu() {
        // Set up camera for menu view - more top-down angle
        bestow::Camera3D cam = graphics_->getCamera();
        cam.fovY = 45.0f;
        cam.transform.position = {0.0f, 15.0f, 10.0f};
        cam.transform.rotation = glm::quatLookAt(
            glm::normalize(glm::vec3(0.0f, -0.8f, -0.4f)),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        graphics_->setCamera(cam);

        // Draw simple dark ground plane
        {
            bestow::PBRMaterial groundMat;
            groundMat.baseColorFactor = {0.05f, 0.08f, 0.05f, 1.0f};  // Very dark green
            auto matResult = graphics_->createMaterial(groundMat);
            if (matResult && groundMesh_) {
                bestow::Mat4 transform = glm::translate(glm::identity<glm::mat4>(), glm::vec3(0.0f, -0.5f, 0.0f));
                transform = glm::scale(transform, glm::vec3(25.0f, 1.0f, 25.0f));
                graphics_->drawMesh(groundMesh_, *matResult, transform, true, true);
            }
        }

        // Draw a coiled serpent in the center as decoration
        float snakeAnim = menuAnimTime_ * 0.8f;
        int coilSegments = 20;
        for (int i = 0; i < coilSegments; ++i) {
            float t = static_cast<float>(i) / coilSegments;
            float angle = t * 6.28f * 2.5f + snakeAnim;  // 2.5 coils
            float radius = 2.0f + t * 1.5f;  // Spiral outward
            float x = std::cos(angle) * radius;
            float z = std::sin(angle) * radius;
            float y = 0.4f + std::sin(snakeAnim * 2.0f + t * 6.28f) * 0.1f;

            float green = 0.9f - t * 0.4f;
            bestow::PBRMaterial mat;
            mat.baseColorFactor = {0.15f, green, 0.2f, 1.0f};
            mat.emissiveFactor = {0.0f, green * 0.1f, 0.0f};

            float segmentScale = 0.6f - t * 0.2f;  // Smaller toward tail
            bestow::Mat4 transform = glm::translate(glm::identity<glm::mat4>(), glm::vec3(x, y, z));
            transform = glm::scale(transform, glm::vec3(segmentScale));
            auto matResult = graphics_->createMaterial(mat);
            if (matResult && cubeMesh_) {
                graphics_->drawMesh(cubeMesh_, *matResult, transform, true, true);
            }
        }

        // Menu options - simple cubes with selection indicator
        float menuZ = 5.0f;
        float menuSpacing = 2.0f;

        for (int i = 0; i < MAIN_MENU_COUNT; ++i) {
            float z = menuZ + static_cast<float>(i) * menuSpacing;
            bool selected = (i == mainMenuSelection_);

            // Number of cubes for each option (PLAY = 4 cubes, QUIT = 4 cubes)
            int numCubes = 4;
            float cubeSpacing = 1.2f;
            float startX = -static_cast<float>(numCubes - 1) * cubeSpacing * 0.5f;

            for (int c = 0; c < numCubes; ++c) {
                float x = startX + static_cast<float>(c) * cubeSpacing;
                float y = 0.5f;

                // Animate selected row
                if (selected) {
                    float bounce = std::sin(menuAnimTime_ * 4.0f + static_cast<float>(c) * 0.5f) * 0.2f;
                    y += bounce + 0.3f;
                }

                bestow::PBRMaterial mat;
                if (selected) {
                    // Bright golden color for selected
                    float pulse = std::sin(menuAnimTime_ * 3.0f) * 0.2f + 0.8f;
                    mat.baseColorFactor = {1.0f, 0.8f * pulse, 0.2f, 1.0f};
                    mat.emissiveFactor = {0.3f, 0.2f, 0.0f};
                } else {
                    // Dim gray for unselected
                    mat.baseColorFactor = {0.4f, 0.4f, 0.5f, 1.0f};
                }

                bestow::Mat4 transform = glm::translate(glm::identity<glm::mat4>(), glm::vec3(x, y, z));
                float scale = selected ? 0.9f : 0.7f;
                transform = glm::scale(transform, glm::vec3(scale));

                auto matResult = graphics_->createMaterial(mat);
                if (matResult && cubeMesh_) {
                    graphics_->drawMesh(cubeMesh_, *matResult, transform, true, true);
                }
            }

            // Draw selection arrows on sides
            if (selected) {
                float arrowX = startX - 1.5f;
                float pulse = std::sin(menuAnimTime_ * 5.0f) * 0.3f;

                bestow::PBRMaterial arrowMat;
                arrowMat.baseColorFactor = {1.0f, 1.0f, 0.3f, 1.0f};
                arrowMat.emissiveFactor = {0.5f, 0.5f, 0.0f};

                // Left arrow (triangle of cubes pointing right)
                for (int a = 0; a < 3; ++a) {
                    float ax = arrowX - static_cast<float>(a) * 0.4f + pulse;
                    float ay = 0.5f + (a == 1 ? 0.4f : (a == 2 ? 0.0f : -0.4f));
                    if (a == 0) ay = 0.5f;

                    bestow::Mat4 transform = glm::translate(glm::identity<glm::mat4>(), glm::vec3(ax, 0.5f, z));
                    transform = glm::scale(transform, glm::vec3(0.4f));
                    auto matResult = graphics_->createMaterial(arrowMat);
                    if (matResult && cubeMesh_) {
                        graphics_->drawMesh(cubeMesh_, *matResult, transform, true, true);
                    }
                }

                // Right arrow
                arrowX = startX + static_cast<float>(numCubes - 1) * cubeSpacing + 1.5f;
                for (int a = 0; a < 3; ++a) {
                    float ax = arrowX + static_cast<float>(a) * 0.4f - pulse;
                    bestow::Mat4 transform = glm::translate(glm::identity<glm::mat4>(), glm::vec3(ax, 0.5f, z));
                    transform = glm::scale(transform, glm::vec3(0.4f));
                    auto matResult = graphics_->createMaterial(arrowMat);
                    if (matResult && cubeMesh_) {
                        graphics_->drawMesh(cubeMesh_, *matResult, transform, true, true);
                    }
                }
            }
        }

        // Draw title as a row of cubes at the front
        float titleZ = -2.0f;
        int titleCubes = 7;
        float titleSpacing = 0.8f;
        float titleStartX = -static_cast<float>(titleCubes - 1) * titleSpacing * 0.5f;

        for (int t = 0; t < titleCubes; ++t) {
            float x = titleStartX + static_cast<float>(t) * titleSpacing;
            float y = 0.5f + std::sin(menuAnimTime_ * 2.0f + static_cast<float>(t) * 0.4f) * 0.15f;

            bestow::PBRMaterial mat;
            float hue = static_cast<float>(t) / static_cast<float>(titleCubes);
            mat.baseColorFactor = {0.2f + hue * 0.3f, 0.8f, 0.3f + (1.0f - hue) * 0.3f, 1.0f};
            mat.emissiveFactor = {0.1f, 0.2f, 0.1f};

            bestow::Mat4 transform = glm::translate(glm::identity<glm::mat4>(), glm::vec3(x, y, titleZ));
            transform = glm::scale(transform, glm::vec3(0.6f));
            auto matResult = graphics_->createMaterial(mat);
            if (matResult && cubeMesh_) {
                graphics_->drawMesh(cubeMesh_, *matResult, transform, true, true);
            }
        }

        // Instructions hint - small cubes indicating controls
        // Arrow pattern on the right side
        float hintX = 6.0f;
        float hintZ = 3.0f;
        bestow::PBRMaterial hintMat;
        hintMat.baseColorFactor = {0.6f, 0.6f, 0.7f, 1.0f};

        // Up arrow hint
        auto drawHintCube = [&](float x, float y, float z) {
            bestow::Mat4 transform = glm::translate(glm::identity<glm::mat4>(), glm::vec3(x, y, z));
            transform = glm::scale(transform, glm::vec3(0.3f));
            auto matResult = graphics_->createMaterial(hintMat);
            if (matResult && cubeMesh_) {
                graphics_->drawMesh(cubeMesh_, *matResult, transform, true, true);
            }
        };

        // Vertical arrow (up/down navigation)
        drawHintCube(hintX, 0.3f, hintZ - 0.5f);
        drawHintCube(hintX, 0.3f, hintZ);
        drawHintCube(hintX, 0.3f, hintZ + 0.5f);

        // ==================== TEXT LABELS (Pixel Font) ====================
        // Title text - "SERPENT"
        float titlePixelSize = 0.15f;
        drawPixelTextShadow("SERPENT", 0.0f, 2.0f, titleZ, titlePixelSize,
                            bestow::Color{100, 255, 150, 255});

        // Menu option labels
        std::array<const char*, MAIN_MENU_COUNT> menuLabels = {"PLAY", "QUIT"};
        for (int i = 0; i < MAIN_MENU_COUNT; ++i) {
            float z = menuZ + static_cast<float>(i) * menuSpacing;
            bool selected = (i == mainMenuSelection_);

            bestow::Color textColor = selected
                ? bestow::Color{255, 220, 100, 255}  // Gold for selected
                : bestow::Color{180, 180, 200, 255}; // Gray for unselected

            float textY = selected ? 1.5f : 1.2f;
            float pixelSize = selected ? 0.08f : 0.05f;

            drawPixelTextShadow(menuLabels[i], 0.0f, textY, z, pixelSize, textColor);
        }

        // Controls hint text
        drawPixelText(",O SELECT", hintX, 0.8f, hintZ + 1.5f, 0.03f,
                      bestow::Color{150, 150, 170, 255}, false);
    }

    void drawPauseMenu() {
        // Darken overlay - draw large dark rectangle using multiple lines
        bestow::Color overlayColor{20, 20, 30, 200};
        float halfGrid = gridSize_ * CELL_SIZE * 0.5f;
        for (float z = -halfGrid; z <= halfGrid; z += 0.5f) {
            graphics_->debugDrawLine({-halfGrid, 0.1f, z}, {halfGrid, 0.1f, z}, overlayColor, 0.0f, false);
        }

        // Draw menu in world space, centered on camera target
        float menuY = 2.0f;
        float menuSpacing = 1.0f;
        float cx = cameraTarget_.x;
        float cz = cameraTarget_.z;

        std::array<const char*, PAUSE_MENU_COUNT> options = {"RESUME", "RESTART", "QUIT"};

        for (int i = 0; i < PAUSE_MENU_COUNT; ++i) {
            float y = menuY - static_cast<float>(i) * menuSpacing;
            bool selected = (i == pauseMenuSelection_);

            // Selection indicator
            if (selected) {
                float pulse = std::sin(gameTime_ * 4.0f) * 0.1f + 0.9f;
                bestow::Color selectColor{255, 200, 50, 255};
                graphics_->debugDrawLine({cx - 2.5f * pulse, y, cz}, {cx - 1.5f, y, cz}, selectColor, 0.0f, false);
                graphics_->debugDrawLine({cx + 1.5f, y, cz}, {cx + 2.5f * pulse, y, cz}, selectColor, 0.0f, false);
            }

            // Draw text label using pixel font
            bestow::Color textColor = selected
                ? bestow::Color{255, 220, 100, 255}
                : bestow::Color{150, 150, 160, 255};

            float pixelSize = selected ? 0.06f : 0.04f;
            drawPixelTextShadow(options[i], cx, y, cz, pixelSize, textColor);
        }

        // PAUSED title
        drawPixelTextShadow("PAUSED", cx, menuY + 1.5f, cz, 0.1f,
                            bestow::Color{255, 255, 100, 255});
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
