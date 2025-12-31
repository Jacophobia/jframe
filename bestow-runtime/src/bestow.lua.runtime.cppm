// bestow-runtime/src/bestow.lua.runtime.cppm
// Lua Runtime Application Module
//
// Defines LuaApplication which runs Lua games with full engine integration:
// - Vulkan 3D graphics with window and rendering
// - Input system with action bindings exposed to Lua
// - Audio system for sound playback from Lua
// - Asset system for loading textures, models, sounds

module;

// Sol2 and Lua must be in global module fragment
#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

// GLFW for key codes
#include <GLFW/glfw3.h>

// GLM for math types
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

// spdlog for logging
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

// EnTT for ECS
#include <entt/entt.hpp>

export module bestow.lua.runtime;

import std;
import bestow.services;
import bestow.types;

export namespace bestow::lua {

//==========================================================================
// Component Types for Lua
//==========================================================================

struct Position2D {
    float x = 0.0f;
    float y = 0.0f;
};

struct Position3D {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct Scale2D {
    float x = 1.0f;
    float y = 1.0f;
};

struct Rotation2D {
    float degrees = 0.0f;
};

struct Velocity2D {
    float x = 0.0f;
    float y = 0.0f;
};

struct TagComponent {
    std::vector<std::string> tags;

    void addTag(const std::string& tag) {
        if (!hasTag(tag)) {
            tags.push_back(tag);
        }
    }

    bool hasTag(const std::string& tag) const {
        return std::find(tags.begin(), tags.end(), tag) != tags.end();
    }

    void removeTag(const std::string& tag) {
        tags.erase(std::remove(tags.begin(), tags.end(), tag), tags.end());
    }
};

struct SpriteComponent {
    std::string texturePath;
    Vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
    float width = 1.0f;
    float height = 1.0f;
};

//==========================================================================
// Lua Entity Wrapper
//==========================================================================

class LuaEntity {
public:
    LuaEntity() = default;
    LuaEntity(entt::entity entity, entt::registry* registry, sol::state* lua)
        : entity_(entity), registry_(registry), lua_(lua) {}

    bool isValid() const {
        return registry_ && registry_->valid(entity_);
    }

    void destroy() {
        if (isValid()) {
            registry_->destroy(entity_);
        }
    }

    // Fluent builders
    LuaEntity& at(float x, float y) {
        if (!isValid()) return *this;
        if (registry_->all_of<Position2D>(entity_)) {
            auto& pos = registry_->get<Position2D>(entity_);
            pos.x = x;
            pos.y = y;
        } else {
            registry_->emplace<Position2D>(entity_, x, y);
        }
        return *this;
    }

    LuaEntity& at3D(float x, float y, float z) {
        if (!isValid()) return *this;
        if (registry_->all_of<Position3D>(entity_)) {
            auto& pos = registry_->get<Position3D>(entity_);
            pos.x = x;
            pos.y = y;
            pos.z = z;
        } else {
            registry_->emplace<Position3D>(entity_, x, y, z);
        }
        return *this;
    }

    LuaEntity& withTag(const std::string& tag) {
        if (!isValid()) return *this;
        if (registry_->all_of<TagComponent>(entity_)) {
            registry_->get<TagComponent>(entity_).addTag(tag);
        } else {
            TagComponent tc;
            tc.addTag(tag);
            registry_->emplace<TagComponent>(entity_, std::move(tc));
        }
        return *this;
    }

    LuaEntity& withScale(float s) {
        if (!isValid()) return *this;
        if (registry_->all_of<Scale2D>(entity_)) {
            auto& scale = registry_->get<Scale2D>(entity_);
            scale.x = s;
            scale.y = s;
        } else {
            registry_->emplace<Scale2D>(entity_, s, s);
        }
        return *this;
    }

    LuaEntity& withVelocity(float vx, float vy) {
        if (!isValid()) return *this;
        if (registry_->all_of<Velocity2D>(entity_)) {
            auto& vel = registry_->get<Velocity2D>(entity_);
            vel.x = vx;
            vel.y = vy;
        } else {
            registry_->emplace<Velocity2D>(entity_, vx, vy);
        }
        return *this;
    }

    LuaEntity& withSprite(const std::string& texturePath, float w, float h) {
        if (!isValid()) return *this;
        SpriteComponent sprite;
        sprite.texturePath = texturePath;
        sprite.width = w;
        sprite.height = h;
        registry_->emplace_or_replace<SpriteComponent>(entity_, std::move(sprite));
        return *this;
    }

    // Component access
    sol::table getPosition2D() {
        if (!isValid() || !lua_) return sol::table{};
        if (!registry_->all_of<Position2D>(entity_)) return sol::table{};

        const auto& pos = registry_->get<Position2D>(entity_);
        sol::table t = lua_->create_table();
        t["x"] = pos.x;
        t["y"] = pos.y;
        return t;
    }

    void setPosition2D(float x, float y) {
        if (!isValid()) return;
        if (registry_->all_of<Position2D>(entity_)) {
            auto& pos = registry_->get<Position2D>(entity_);
            pos.x = x;
            pos.y = y;
        } else {
            registry_->emplace<Position2D>(entity_, x, y);
        }
    }

    sol::table getPosition3D() {
        if (!isValid() || !lua_) return sol::table{};
        if (!registry_->all_of<Position3D>(entity_)) return sol::table{};

        const auto& pos = registry_->get<Position3D>(entity_);
        sol::table t = lua_->create_table();
        t["x"] = pos.x;
        t["y"] = pos.y;
        t["z"] = pos.z;
        return t;
    }

    void setPosition3D(float x, float y, float z) {
        if (!isValid()) return;
        if (registry_->all_of<Position3D>(entity_)) {
            auto& pos = registry_->get<Position3D>(entity_);
            pos.x = x;
            pos.y = y;
            pos.z = z;
        } else {
            registry_->emplace<Position3D>(entity_, x, y, z);
        }
    }

    bool hasTag(const std::string& tag) {
        if (!isValid()) return false;
        if (registry_->all_of<TagComponent>(entity_)) {
            return registry_->get<TagComponent>(entity_).hasTag(tag);
        }
        return false;
    }

    std::vector<std::string> getTags() {
        if (!isValid()) return {};
        if (registry_->all_of<TagComponent>(entity_)) {
            return registry_->get<TagComponent>(entity_).tags;
        }
        return {};
    }

    std::uint32_t id() const {
        return static_cast<std::uint32_t>(entity_);
    }

private:
    entt::entity entity_ = entt::null;
    entt::registry* registry_ = nullptr;
    sol::state* lua_ = nullptr;
};

//==========================================================================
// Timer System
//==========================================================================

struct Timer {
    std::uint64_t id;
    float interval;
    float remaining;
    bool repeating;
    sol::function callback;
};

//==========================================================================
// Game Configuration from Lua
//==========================================================================

struct LuaGameConfig {
    std::string title = "Bestow Game";
    int windowWidth = 1280;
    int windowHeight = 720;
    bool vsync = true;
    bool fullscreen = false;
    Color clearColor{30, 30, 40, 255};
};

//==========================================================================
// Lua Application
//==========================================================================

class LuaApplication : public Application<LuaApplication,
    IGraphics3DSystem,
    IInputSystem,
    IAudioSystem,
    IConfigSystem,
    IAssetSystem>
{
public:
    LuaApplication(IGraphics3DSystem& graphics,
                   IInputSystem& input,
                   IAudioSystem& audio,
                   IConfigSystem& config,
                   IAssetSystem& assets)
        : graphics_(&graphics)
        , input_(&input)
        , audio_(&audio)
        , config_(&config)
        , assets_(&assets)
        , registry_(std::make_unique<entt::registry>())
    {
    }

    ~LuaApplication() override = default;

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

    // Set the Lua script to run (must be called before run())
    static void setScriptPath(const std::string& path) {
        scriptPath_ = path;
    }

    static void setVerbose(bool verbose) {
        verbose_ = verbose;
    }

private:
    //======================================================================
    // Static Configuration
    //======================================================================
    static inline std::string scriptPath_;
    static inline bool verbose_ = false;

    //======================================================================
    // Dependencies
    //======================================================================
    IGraphics3DSystem* graphics_ = nullptr;
    IInputSystem* input_ = nullptr;
    IAudioSystem* audio_ = nullptr;
    IConfigSystem* config_ = nullptr;
    IAssetSystem* assets_ = nullptr;

    //======================================================================
    // Lua State
    //======================================================================
    sol::state lua_;
    LuaGameConfig gameConfig_;
    bool running_ = false;
    bool shouldQuit_ = false;

    //======================================================================
    // ECS
    //======================================================================
    std::unique_ptr<entt::registry> registry_;

    //======================================================================
    // Timers & Events
    //======================================================================
    std::vector<Timer> timers_;
    std::uint64_t nextTimerId_ = 1;

    struct EventSub {
        std::string eventName;
        sol::function callback;
    };
    std::vector<EventSub> eventSubscriptions_;

    //======================================================================
    // Time Tracking
    //======================================================================
    float totalTime_ = 0.0f;
    float currentDeltaTime_ = 0.0f;

    //======================================================================
    // Sound Cache (path -> AssetHandle for quick lookup)
    //======================================================================
    std::unordered_map<std::string, AssetHandle> soundCache_;

    //======================================================================
    // Initialization
    //======================================================================

    bool initialize() {
        // Setup logging
        spdlog::set_level(verbose_ ? spdlog::level::debug : spdlog::level::info);

        spdlog::info("========================================");
        spdlog::info("Bestow Lua Runtime");
        spdlog::info("========================================");

        // Check script exists
        if (scriptPath_.empty()) {
            spdlog::critical("No script path set");
            return false;
        }

        if (!std::filesystem::exists(scriptPath_)) {
            spdlog::critical("Script not found: {}", scriptPath_);
            return false;
        }

        spdlog::info("Loading: {}", scriptPath_);

        // Initialize Lua
        if (!initializeLua()) {
            return false;
        }

        // Load and execute the script
        if (!loadScript()) {
            return false;
        }

        // Parse config from game table
        parseGameConfig();

        // Initialize graphics with config
        if (!initializeGraphics()) {
            return false;
        }

        // Initialize input
        if (input_) {
            input_->initialize(graphics_->getNativeWindowHandle());
            setupInputBindings();
        }

        // Initialize audio
        if (audio_) {
            audio_->initialize();
        }

        // Call Lua game:init()
        callLuaInit();

        return true;
    }

    bool initializeLua() {
        lua_.open_libraries(
            sol::lib::base,
            sol::lib::math,
            sol::lib::string,
            sol::lib::table,
            sol::lib::coroutine,
            sol::lib::package  // Needed for require() and package.path
        );

        // Sandbox - remove dangerous functions
        lua_["os"] = sol::lua_nil;
        lua_["io"] = sol::lua_nil;
        lua_["loadfile"] = sol::lua_nil;
        lua_["dofile"] = sol::lua_nil;
        lua_["load"] = sol::lua_nil;
        lua_["loadstring"] = sol::lua_nil;
        lua_["debug"] = sol::lua_nil;

        // Create the bestow namespace table first
        lua_["bestow"] = lua_.create_table();
        spdlog::debug("Created bestow table");

        // Register bindings
        registerMathTypes();
        registerEntityBindings();
        registerInputBindings();
        registerAudioBindings();
        registerGraphicsBindings();
        registerTimeBindings();
        registerGameAPI();

        spdlog::debug("Lua bindings registered");
        return true;
    }

    bool loadScript() {
        spdlog::debug("loadScript() starting...");

        // Set working directory and package path
        auto scriptPath = std::filesystem::absolute(scriptPath_);
        auto scriptDir = scriptPath.parent_path();

        std::string packagePath = scriptDir.string() + "/?.lua;" +
                                  scriptDir.string() + "/?/init.lua";
        spdlog::debug("Setting package path: {}", packagePath);
        lua_["package"]["path"] = packagePath;

        spdlog::debug("Executing script file: {}", scriptPath_);
        // Execute script
        auto result = lua_.safe_script_file(scriptPath_, sol::script_pass_on_error);
        spdlog::debug("Script executed, checking result...");
        if (!result.valid()) {
            sol::error err = result;
            spdlog::critical("Failed to load {}: {}", scriptPath_, err.what());
            return false;
        }
        spdlog::debug("Script execution succeeded");

        // Verify game table exists
        spdlog::debug("Checking for game table...");
        sol::table game = lua_["game"];
        if (!game.valid()) {
            spdlog::critical("main.lua must create a 'game' table");
            spdlog::info("Example:");
            spdlog::info("  game = {{");
            spdlog::info("      title = \"My Game\",");
            spdlog::info("      init = function(self) end,");
            spdlog::info("      update = function(self, dt) end,");
            spdlog::info("  }}");
            return false;
        }
        spdlog::debug("Game table found");

        return true;
    }

    void parseGameConfig() {
        sol::table game = lua_["game"];

        if (game["title"].valid()) {
            gameConfig_.title = game["title"].get<std::string>();
        }

        if (game["window"].valid()) {
            sol::table window = game["window"];
            if (window["width"].valid()) {
                gameConfig_.windowWidth = window["width"].get<int>();
            }
            if (window["height"].valid()) {
                gameConfig_.windowHeight = window["height"].get<int>();
            }
        }

        if (game["vsync"].valid()) {
            gameConfig_.vsync = game["vsync"].get<bool>();
        }

        if (game["fullscreen"].valid()) {
            gameConfig_.fullscreen = game["fullscreen"].get<bool>();
        }

        if (game["clearColor"].valid()) {
            sol::table cc = game["clearColor"];
            if (cc[1].valid()) {
                gameConfig_.clearColor.r = static_cast<std::uint8_t>(cc[1].get<int>());
                gameConfig_.clearColor.g = static_cast<std::uint8_t>(cc[2].get<int>());
                gameConfig_.clearColor.b = static_cast<std::uint8_t>(cc[3].get<int>());
                gameConfig_.clearColor.a = cc[4].valid() ?
                    static_cast<std::uint8_t>(cc[4].get<int>()) : 255;
            }
        }

        spdlog::info("Game: {}", gameConfig_.title);
        spdlog::info("Window: {}x{}", gameConfig_.windowWidth, gameConfig_.windowHeight);
    }

    bool initializeGraphics() {
        if (!graphics_) {
            spdlog::critical("Graphics system not available");
            return false;
        }

        Graphics3DConfig gfxConfig{
            .windowWidth = gameConfig_.windowWidth,
            .windowHeight = gameConfig_.windowHeight,
            .windowTitle = gameConfig_.title,
            .vsync = gameConfig_.vsync,
            .fullscreen = gameConfig_.fullscreen
        };

        if (!graphics_->initialize(gfxConfig)) {
            spdlog::critical("Failed to initialize graphics");
            return false;
        }

        graphics_->setClearColor(gameConfig_.clearColor);

        // Set up default camera looking at origin from above
        Camera3D cam;
        cam.transform.position = {0.0f, 10.0f, 10.0f};
        // Look toward origin (camera looks along negative Z in local space)
        cam.fovY = 60.0f;
        cam.nearPlane = 0.1f;
        cam.farPlane = 1000.0f;
        graphics_->setCamera(cam);

        // Set up default lighting
        graphics_->setAmbientLight({0.3f, 0.3f, 0.35f});
        DirectionalLight sun{
            .direction = {-0.5f, -1.0f, -0.3f},
            .color = {1.0f, 0.95f, 0.9f},
            .intensity = 2.0f
        };
        graphics_->setDirectionalLight(sun);

        return true;
    }

    void callLuaInit() {
        sol::table game = lua_["game"];
        if (game["init"].valid()) {
            spdlog::info("Calling game:init()...");
            sol::function initFn = game["init"];
            auto result = initFn(game);
            if (!result.valid()) {
                sol::error err = result;
                spdlog::error("game:init() error: {}", err.what());
            }
        }
    }

    //======================================================================
    // Game Loop
    //======================================================================

    void gameLoop() {
        constexpr DeltaTime fixedDt = 1.0f / 60.0f;
        auto previousTime = std::chrono::high_resolution_clock::now();
        DeltaTime accumulator = 0.0f;

        running_ = true;

        spdlog::info("Starting game loop...");

        while (running_ && !shouldQuit_ && !graphics_->shouldClose()) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            DeltaTime frameTime =
                std::chrono::duration<float>(currentTime - previousTime).count();
            previousTime = currentTime;

            if (frameTime > 0.25f) {
                frameTime = 0.25f;
            }
            accumulator += frameTime;

            // Update input
            if (input_) {
                input_->update();
            }

            // Handle input ONCE per frame (before fixed timestep loop)
            // This matches C++ game pattern and prevents missed inputs at high framerates
            callLuaHandleInput();

            // Update audio
            if (audio_) {
                audio_->update(frameTime);
            }

            // Fixed timestep updates
            while (accumulator >= fixedDt) {
                currentDeltaTime_ = fixedDt;
                totalTime_ += fixedDt;
                updateTimers(fixedDt);
                callLuaUpdate(fixedDt);
                updateEntities(fixedDt);
                accumulator -= fixedDt;
            }

            // Render
            graphics_->beginFrame();
            callLuaDraw();
            renderEntities();
            graphics_->flushRenderQueue();  // Actually render all queued meshes
            graphics_->endFrame();
        }
    }

    void updateTimers(float dt) {
        std::vector<std::uint64_t> toRemove;

        for (auto& timer : timers_) {
            timer.remaining -= dt;
            if (timer.remaining <= 0.0f) {
                auto result = timer.callback();
                if (!result.valid()) {
                    sol::error err = result;
                    spdlog::error("Timer callback error: {}", err.what());
                }

                if (timer.repeating) {
                    timer.remaining = timer.interval;
                } else {
                    toRemove.push_back(timer.id);
                }
            }
        }

        for (auto id : toRemove) {
            timers_.erase(
                std::remove_if(timers_.begin(), timers_.end(),
                    [id](const Timer& t) { return t.id == id; }),
                timers_.end()
            );
        }
    }

    void callLuaHandleInput() {
        sol::table game = lua_["game"];
        if (game.valid() && game["handleInput"].valid()) {
            sol::function handleInputFn = game["handleInput"];
            auto result = handleInputFn(game);
            if (!result.valid()) {
                sol::error err = result;
                spdlog::error("game:handleInput() error: {}", err.what());
            }
        }
    }

    void callLuaUpdate(float dt) {
        sol::table game = lua_["game"];
        if (game.valid() && game["update"].valid()) {
            sol::function updateFn = game["update"];
            auto result = updateFn(game, dt);
            if (!result.valid()) {
                sol::error err = result;
                spdlog::error("game:update() error: {}", err.what());
            }
        }
    }

    void callLuaDraw() {
        sol::table game = lua_["game"];
        if (game.valid() && game["draw"].valid()) {
            sol::function drawFn = game["draw"];
            auto result = drawFn(game);
            if (!result.valid()) {
                sol::error err = result;
                spdlog::error("game:draw() error: {}", err.what());
            }
        }
    }

    void updateEntities(float dt) {
        // Update entities with velocity
        auto view = registry_->view<Position2D, Velocity2D>();
        for (auto entity : view) {
            auto& pos = view.get<Position2D>(entity);
            const auto& vel = view.get<Velocity2D>(entity);
            pos.x += vel.x * dt;
            pos.y += vel.y * dt;
        }
    }

    void renderEntities() {
        // Render entities with sprites and positions
        // For now this is a placeholder - actual rendering would
        // use the graphics system to draw sprites/meshes
    }

    //======================================================================
    // Cleanup
    //======================================================================

    void cleanup() {
        // Call Lua shutdown
        sol::table game = lua_["game"];
        if (game.valid() && game["shutdown"].valid()) {
            spdlog::info("Calling game:shutdown()...");
            sol::function shutdownFn = game["shutdown"];
            auto result = shutdownFn(game);
            if (!result.valid()) {
                sol::error err = result;
                spdlog::error("game:shutdown() error: {}", err.what());
            }
        }

        if (input_) {
            input_->shutdown();
        }
        if (graphics_) {
            graphics_->shutdown();
        }

        spdlog::info("========================================");
        spdlog::info("Bestow Lua Runtime finished");
        spdlog::info("========================================");
    }

    //======================================================================
    // Lua Bindings Registration
    //======================================================================

    void registerMathTypes() {
        // Vec2
        lua_.new_usertype<glm::vec2>("Vec2",
            sol::constructors<glm::vec2(), glm::vec2(float, float)>(),
            "x", &glm::vec2::x,
            "y", &glm::vec2::y,
            sol::meta_function::addition, [](const glm::vec2& a, const glm::vec2& b) { return a + b; },
            sol::meta_function::subtraction, [](const glm::vec2& a, const glm::vec2& b) { return a - b; },
            sol::meta_function::multiplication, sol::overload(
                [](const glm::vec2& a, float s) { return a * s; },
                [](float s, const glm::vec2& a) { return s * a; }
            ),
            "length", [](const glm::vec2& v) { return glm::length(v); },
            "normalize", [](const glm::vec2& v) {
                float len = glm::length(v);
                return len > 0.0f ? v / len : v;
            },
            "dot", [](const glm::vec2& a, const glm::vec2& b) { return glm::dot(a, b); }
        );

        // Vec3
        lua_.new_usertype<glm::vec3>("Vec3",
            sol::constructors<glm::vec3(), glm::vec3(float, float, float)>(),
            "x", &glm::vec3::x,
            "y", &glm::vec3::y,
            "z", &glm::vec3::z,
            sol::meta_function::addition, [](const glm::vec3& a, const glm::vec3& b) { return a + b; },
            sol::meta_function::subtraction, [](const glm::vec3& a, const glm::vec3& b) { return a - b; },
            sol::meta_function::multiplication, sol::overload(
                [](const glm::vec3& a, float s) { return a * s; },
                [](float s, const glm::vec3& a) { return s * a; }
            ),
            "length", [](const glm::vec3& v) { return glm::length(v); },
            "normalize", [](const glm::vec3& v) {
                float len = glm::length(v);
                return len > 0.0f ? v / len : v;
            },
            "dot", [](const glm::vec3& a, const glm::vec3& b) { return glm::dot(a, b); },
            "cross", [](const glm::vec3& a, const glm::vec3& b) { return glm::cross(a, b); }
        );

        spdlog::debug("Math types registered");
    }

    void registerEntityBindings() {
        // Register LuaEntity usertype
        lua_.new_usertype<LuaEntity>("Entity",
            sol::no_constructor,
            "isValid", &LuaEntity::isValid,
            "destroy", &LuaEntity::destroy,
            "id", &LuaEntity::id,

            // Fluent builders
            "at", &LuaEntity::at,
            "at3D", &LuaEntity::at3D,
            "withTag", &LuaEntity::withTag,
            "withScale", &LuaEntity::withScale,
            "withVelocity", &LuaEntity::withVelocity,
            "withSprite", &LuaEntity::withSprite,

            // Component access
            "getPosition2D", &LuaEntity::getPosition2D,
            "setPosition2D", &LuaEntity::setPosition2D,
            "getPosition3D", &LuaEntity::getPosition3D,
            "setPosition3D", &LuaEntity::setPosition3D,
            "hasTag", &LuaEntity::hasTag,
            "getTags", &LuaEntity::getTags
        );

        spdlog::debug("Entity bindings registered");
    }

    void registerInputBindings() {
        sol::table input = lua_.create_table();

        // Key state queries
        input["isKeyDown"] = [this](int keyCode) -> bool {
            return input_ ? input_->isKeyDown(keyCode) : false;
        };

        input["wasKeyJustPressed"] = [this](int keyCode) -> bool {
            return input_ ? input_->wasKeyJustPressed(keyCode) : false;
        };

        input["wasKeyJustReleased"] = [this](int keyCode) -> bool {
            return input_ ? input_->wasKeyJustReleased(keyCode) : false;
        };

        // Mouse
        input["getMousePosition"] = [this]() -> std::tuple<float, float> {
            if (!input_) return {0.0f, 0.0f};
            auto pos = input_->getMousePosition();
            return {pos.x, pos.y};
        };

        input["getMouseDelta"] = [this]() -> std::tuple<float, float> {
            if (!input_) return {0.0f, 0.0f};
            auto delta = input_->getMouseDelta();
            return {delta.x, delta.y};
        };

        input["isMouseButtonDown"] = [this](int button) -> bool {
            return input_ ? input_->isMouseButtonDown(button) : false;
        };

        // Modifiers
        input["isShiftPressed"] = [this]() -> bool {
            return input_ ? input_->isShiftPressed() : false;
        };

        input["isCtrlPressed"] = [this]() -> bool {
            return input_ ? input_->isCtrlPressed() : false;
        };

        input["isAltPressed"] = [this]() -> bool {
            return input_ ? input_->isAltPressed() : false;
        };

        // Expose as bestow.input
        lua_["bestow"]["input"] = input;

        // Also expose key constants
        sol::table keys = lua_.create_table();
        keys["SPACE"] = GLFW_KEY_SPACE;
        keys["ESCAPE"] = GLFW_KEY_ESCAPE;
        keys["ENTER"] = GLFW_KEY_ENTER;
        keys["TAB"] = GLFW_KEY_TAB;
        keys["BACKSPACE"] = GLFW_KEY_BACKSPACE;
        keys["LEFT"] = GLFW_KEY_LEFT;
        keys["RIGHT"] = GLFW_KEY_RIGHT;
        keys["UP"] = GLFW_KEY_UP;
        keys["DOWN"] = GLFW_KEY_DOWN;
        // Dvorak movement keys
        keys["COMMA"] = GLFW_KEY_COMMA;    // , (up in Dvorak WASD)
        keys["A"] = GLFW_KEY_A;            // A (left)
        keys["O"] = GLFW_KEY_O;            // O (down in Dvorak WASD)
        keys["E"] = GLFW_KEY_E;            // E (right in Dvorak WASD)
        // WASD for QWERTY users
        keys["W"] = GLFW_KEY_W;
        keys["S"] = GLFW_KEY_S;
        keys["D"] = GLFW_KEY_D;
        // Letters
        for (int k = GLFW_KEY_A; k <= GLFW_KEY_Z; ++k) {
            std::string name(1, static_cast<char>('A' + (k - GLFW_KEY_A)));
            keys[name] = k;
        }
        // Numbers
        for (int k = GLFW_KEY_0; k <= GLFW_KEY_9; ++k) {
            std::string name(1, static_cast<char>('0' + (k - GLFW_KEY_0)));
            keys[name] = k;
        }

        lua_["KEY"] = keys;

        spdlog::debug("Input bindings registered");
    }

    void registerAudioBindings() {
        sol::table audio = lua_.create_table();

        // Helper to get or load a sound asset
        auto getOrLoadSound = [this](const std::string& path) -> AssetHandle {
            if (!assets_) return AssetHandle{};

            // Check cache first
            auto it = soundCache_.find(path);
            if (it != soundCache_.end() && assets_->isLoaded(it->second)) {
                return it->second;
            }

            // Register and load the sound
            AssetHandle handle = assets_->registerAsset(AssetType::Sound, path);
            if (handle.isValid()) {
                assets_->loadAsset(handle);
                soundCache_[path] = handle;
            }
            return handle;
        };

        // Play sound effect (one-shot, non-positional)
        auto playSoundFn = [this, getOrLoadSound](const std::string& path, sol::optional<sol::table> options) {
            if (!audio_ || !assets_) return;

            float volume = 1.0f;
            float pitch = 1.0f;
            bool looping = false;

            if (options) {
                if ((*options)["volume"].valid()) {
                    volume = (*options)["volume"].get<float>();
                }
                if ((*options)["pitch"].valid()) {
                    pitch = (*options)["pitch"].get<float>();
                }
                if ((*options)["looping"].valid()) {
                    looping = (*options)["looping"].get<bool>();
                }
            }

            AssetHandle handle = getOrLoadSound(path);
            if (!handle.isValid()) {
                spdlog::warn("Failed to load sound: {}", path);
                return;
            }

            // Play on a temporary channel (SFX channel 0)
            ChannelSound sound;
            sound.asset = handle;
            sound.volume = volume;
            sound.pitch = pitch;
            sound.looping = looping;

            audio_->playOnChannel(0, sound);  // Channel 0 for one-shot SFX
        };
        audio["play"] = playSoundFn;
        audio["playSound"] = playSoundFn;

        // Play music (looping, on music channel)
        audio["playMusic"] = [this, getOrLoadSound](const std::string& path, sol::optional<sol::table> options) {
            if (!audio_ || !assets_) return;

            float volume = 1.0f;
            float fadeInTime = 0.0f;

            if (options) {
                if ((*options)["volume"].valid()) {
                    volume = (*options)["volume"].get<float>();
                }
                if ((*options)["fadeIn"].valid()) {
                    fadeInTime = (*options)["fadeIn"].get<float>();
                }
            }

            AssetHandle handle = getOrLoadSound(path);
            if (!handle.isValid()) {
                spdlog::warn("Failed to load music: {}", path);
                return;
            }

            ChannelSound sound;
            sound.asset = handle;
            sound.volume = volume;
            sound.looping = true;
            sound.fadeInTime = fadeInTime;

            audio_->playOnChannel(1, sound);  // Channel 1 for music
        };

        // Stop music with optional fade out
        audio["stopMusic"] = [this](sol::optional<float> fadeOutTime) {
            if (audio_) {
                audio_->stopChannel(1, fadeOutTime.value_or(0.0f));
            }
        };

        audio["setMasterVolume"] = [this](float volume) {
            if (audio_) {
                audio_->setMasterVolume(volume);
            }
        };

        audio["setGroupVolume"] = [this](const std::string& group, float volume) {
            if (audio_) {
                audio_->setGroupVolume(group, volume);
            }
        };

        lua_["bestow"]["audio"] = audio;

        spdlog::debug("Audio bindings registered");
    }

    void registerGraphicsBindings() {
        sol::table gfx = lua_.create_table();

        // Camera control
        gfx["setCameraPosition"] = [this](float x, float y, float z) {
            if (!graphics_) return;
            auto cam = graphics_->getCamera();
            cam.transform.position = {x, y, z};
            graphics_->setCamera(cam);
        };

        gfx["setCameraTarget"] = [this](float x, float y, float z) {
            if (!graphics_) return;
            // Pass target to graphics system for view matrix computation
            graphics_->setCameraTarget({x, y, z});
        };

        gfx["setCameraFOV"] = [this](float fov) {
            if (!graphics_) return;
            auto cam = graphics_->getCamera();
            cam.fovY = fov;
            graphics_->setCamera(cam);
        };

        // Clear color
        gfx["setClearColor"] = [this](int r, int g, int b, sol::optional<int> a) {
            if (!graphics_) return;
            Color c;
            c.r = static_cast<std::uint8_t>(r);
            c.g = static_cast<std::uint8_t>(g);
            c.b = static_cast<std::uint8_t>(b);
            c.a = a ? static_cast<std::uint8_t>(*a) : 255;
            graphics_->setClearColor(c);
        };

        // Ambient light
        gfx["setAmbientLight"] = [this](float r, float g, float b) {
            if (!graphics_) return;
            graphics_->setAmbientLight({r, g, b});
        };

        // Directional light: setDirectionalLight(dirX, dirY, dirZ, r, g, b, intensity)
        gfx["setDirectionalLight"] = [this](float dirX, float dirY, float dirZ,
                                             float r, float g, float b,
                                             sol::optional<float> intensity,
                                             sol::optional<bool> castShadows) {
            if (!graphics_) return;
            DirectionalLight light;
            light.direction = {dirX, dirY, dirZ};
            light.color = {r, g, b};
            light.intensity = intensity.value_or(1.0f);
            light.castShadows = castShadows.value_or(true);
            graphics_->setDirectionalLight(light);
        };

        // Debug drawing - helper to convert Lua color table to Color
        auto parseColor = [](sol::optional<sol::table> colorOpt) -> Color {
            Color c{255, 255, 255, 255};
            if (colorOpt) {
                sol::table color = *colorOpt;
                if (color[1].valid()) {
                    // If values are > 1, treat as 0-255, otherwise treat as 0-1
                    float r = color[1].get<float>();
                    float g = color[2].valid() ? color[2].get<float>() : r;
                    float b = color[3].valid() ? color[3].get<float>() : g;
                    float a = color[4].valid() ? color[4].get<float>() : 1.0f;

                    // Normalize to 0-255 range
                    if (r <= 1.0f && g <= 1.0f && b <= 1.0f && a <= 1.0f) {
                        c.r = static_cast<std::uint8_t>(r * 255.0f);
                        c.g = static_cast<std::uint8_t>(g * 255.0f);
                        c.b = static_cast<std::uint8_t>(b * 255.0f);
                        c.a = static_cast<std::uint8_t>(a * 255.0f);
                    } else {
                        c.r = static_cast<std::uint8_t>(std::clamp(r, 0.0f, 255.0f));
                        c.g = static_cast<std::uint8_t>(std::clamp(g, 0.0f, 255.0f));
                        c.b = static_cast<std::uint8_t>(std::clamp(b, 0.0f, 255.0f));
                        c.a = static_cast<std::uint8_t>(std::clamp(a, 0.0f, 255.0f));
                    }
                }
            }
            return c;
        };

        gfx["drawLine"] = [this, parseColor](float x1, float y1, float z1,
                                              float x2, float y2, float z2,
                                              sol::optional<sol::table> color) {
            if (!graphics_) return;
            Color c = parseColor(color);
            graphics_->debugDrawLine({x1, y1, z1}, {x2, y2, z2}, c);
        };

        gfx["drawBox"] = [this, parseColor](float x, float y, float z,
                                             float sx, float sy, float sz,
                                             sol::optional<sol::table> color) {
            if (!graphics_) return;
            Color c = parseColor(color);

            // Draw box wireframe using debug lines
            float hx = sx / 2.0f, hy = sy / 2.0f, hz = sz / 2.0f;
            // Bottom face
            graphics_->debugDrawLine({x-hx, y-hy, z-hz}, {x+hx, y-hy, z-hz}, c);
            graphics_->debugDrawLine({x+hx, y-hy, z-hz}, {x+hx, y-hy, z+hz}, c);
            graphics_->debugDrawLine({x+hx, y-hy, z+hz}, {x-hx, y-hy, z+hz}, c);
            graphics_->debugDrawLine({x-hx, y-hy, z+hz}, {x-hx, y-hy, z-hz}, c);
            // Top face
            graphics_->debugDrawLine({x-hx, y+hy, z-hz}, {x+hx, y+hy, z-hz}, c);
            graphics_->debugDrawLine({x+hx, y+hy, z-hz}, {x+hx, y+hy, z+hz}, c);
            graphics_->debugDrawLine({x+hx, y+hy, z+hz}, {x-hx, y+hy, z+hz}, c);
            graphics_->debugDrawLine({x-hx, y+hy, z+hz}, {x-hx, y+hy, z-hz}, c);
            // Vertical edges
            graphics_->debugDrawLine({x-hx, y-hy, z-hz}, {x-hx, y+hy, z-hz}, c);
            graphics_->debugDrawLine({x+hx, y-hy, z-hz}, {x+hx, y+hy, z-hz}, c);
            graphics_->debugDrawLine({x+hx, y-hy, z+hz}, {x+hx, y+hy, z+hz}, c);
            graphics_->debugDrawLine({x-hx, y-hy, z+hz}, {x-hx, y+hy, z+hz}, c);
        };

        // Create a cube mesh
        gfx["createCubeMesh"] = [this](sol::optional<float> size) -> std::uint64_t {
            if (!graphics_) return 0;
            float s = size.value_or(1.0f);
            auto result = graphics_->createCubeMesh(s);
            if (result) {
                return *result;
            }
            return 0;
        };

        // Create a material with full PBR parameters
        // Can be called with just a color table {r, g, b, a} for simple materials
        // Or with a full params table: {color = {r,g,b,a}, metallic = 0.5, roughness = 0.3, emissive = {r,g,b}}
        gfx["createMaterial"] = [this](sol::table params) -> std::uint64_t {
            if (!graphics_) return 0;

            float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
            float metallic = 0.0f;
            float roughness = 0.5f;
            float emissiveR = 0.0f, emissiveG = 0.0f, emissiveB = 0.0f;

            // Check if this is a simple color table {r, g, b, a} or a full params table
            if (params[1].valid()) {
                // Simple color table format: {r, g, b, a}
                r = params.get_or(1, 1.0f);
                g = params.get_or(2, 1.0f);
                b = params.get_or(3, 1.0f);
                a = params.get_or(4, 1.0f);
            } else if (params["color"].valid()) {
                // Full params table format with color subtable
                sol::table color = params["color"];
                r = color.get_or(1, 1.0f);
                g = color.get_or(2, 1.0f);
                b = color.get_or(3, 1.0f);
                a = color.get_or(4, 1.0f);
            }

            // Normalize from 0-255 to 0-1 if needed
            if (r > 1.0f || g > 1.0f || b > 1.0f) {
                r /= 255.0f;
                g /= 255.0f;
                b /= 255.0f;
                a = a > 1.0f ? a / 255.0f : a;
            }

            // Optional PBR parameters
            if (params["metallic"].valid()) {
                metallic = params["metallic"].get<float>();
            }
            if (params["roughness"].valid()) {
                roughness = params["roughness"].get<float>();
            }
            if (params["emissive"].valid()) {
                sol::table emissive = params["emissive"];
                emissiveR = emissive.get_or(1, 0.0f);
                emissiveG = emissive.get_or(2, 0.0f);
                emissiveB = emissive.get_or(3, 0.0f);
            }

            PBRMaterial mat;
            mat.baseColorFactor = {r, g, b, a};
            mat.metallicFactor = metallic;
            mat.roughnessFactor = roughness;
            mat.emissiveFactor = {emissiveR, emissiveG, emissiveB};

            auto result = graphics_->createMaterial(mat);
            if (result) {
                return *result;
            }
            return 0;
        };

        // Draw a mesh with material and transform
        gfx["drawMesh"] = [this](std::uint64_t meshHandle, std::uint64_t materialHandle,
                                  float x, float y, float z,
                                  sol::optional<float> scale,
                                  sol::optional<bool> castShadows,
                                  sol::optional<bool> receiveShadows) {
            if (!graphics_ || meshHandle == 0 || materialHandle == 0) return;

            float s = scale.value_or(1.0f);
            Transform3D transform;
            transform.position = {x, y, z};
            transform.rotation = {1.0f, 0.0f, 0.0f, 0.0f}; // Identity quaternion
            transform.scale = {s, s, s};

            bool cast = castShadows.value_or(true);
            bool receive = receiveShadows.value_or(true);

            graphics_->drawMesh(meshHandle, materialHandle, transform, cast, receive);
        };

        lua_["bestow"]["graphics"] = gfx;

        spdlog::debug("Graphics bindings registered");
    }

    void registerTimeBindings() {
        sol::table time = lua_.create_table();

        // Get total elapsed game time in seconds
        time["getTime"] = [this]() -> float {
            return totalTime_;
        };

        // Get current frame's delta time
        time["getDeltaTime"] = [this]() -> float {
            return currentDeltaTime_;
        };

        lua_["bestow"]["time"] = time;

        spdlog::debug("Time bindings registered");
    }

    void registerGameAPI() {
        sol::table bestow = lua_["bestow"];

        // Entity creation
        bestow["createEntity"] = [this]() -> LuaEntity {
            auto entity = registry_->create();
            spdlog::debug("Created entity {}", static_cast<std::uint32_t>(entity));
            return LuaEntity(entity, registry_.get(), &lua_);
        };

        // Query by tag
        bestow["withTag"] = [this](const std::string& tag) -> std::vector<LuaEntity> {
            std::vector<LuaEntity> result;
            auto view = registry_->view<TagComponent>();
            for (auto entity : view) {
                const auto& tc = view.get<TagComponent>(entity);
                if (tc.hasTag(tag)) {
                    result.emplace_back(entity, registry_.get(), &lua_);
                }
            }
            return result;
        };

        // Find first with tag
        bestow["find"] = [this](const std::string& tag) -> sol::object {
            auto view = registry_->view<TagComponent>();
            for (auto entity : view) {
                const auto& tc = view.get<TagComponent>(entity);
                if (tc.hasTag(tag)) {
                    return sol::make_object(lua_, LuaEntity(entity, registry_.get(), &lua_));
                }
            }
            return sol::lua_nil;
        };

        // Timers
        bestow["after"] = [this](float seconds, sol::function callback) -> std::uint64_t {
            Timer timer;
            timer.id = nextTimerId_++;
            timer.interval = seconds;
            timer.remaining = seconds;
            timer.repeating = false;
            timer.callback = std::move(callback);
            timers_.push_back(std::move(timer));
            spdlog::debug("Timer {} created: after {} seconds", timer.id, seconds);
            return timer.id;
        };

        bestow["every"] = [this](float seconds, sol::function callback) -> std::uint64_t {
            Timer timer;
            timer.id = nextTimerId_++;
            timer.interval = seconds;
            timer.remaining = seconds;
            timer.repeating = true;
            timer.callback = std::move(callback);
            timers_.push_back(std::move(timer));
            spdlog::debug("Timer {} created: every {} seconds", timer.id, seconds);
            return timer.id;
        };

        bestow["cancelTimer"] = [this](std::uint64_t timerId) {
            auto it = std::remove_if(timers_.begin(), timers_.end(),
                [timerId](const Timer& t) { return t.id == timerId; });
            if (it != timers_.end()) {
                timers_.erase(it, timers_.end());
                spdlog::debug("Timer {} cancelled", timerId);
            }
        };

        // Events
        bestow["on"] = [this](const std::string& eventName, sol::function callback) {
            eventSubscriptions_.push_back({eventName, std::move(callback)});
            spdlog::debug("Subscribed to event: {}", eventName);
        };

        bestow["emit"] = [this](const std::string& eventName, sol::optional<sol::table> data) {
            sol::table eventData = data ? *data : lua_.create_table();
            for (const auto& sub : eventSubscriptions_) {
                if (sub.eventName == eventName) {
                    auto result = sub.callback(eventData);
                    if (!result.valid()) {
                        sol::error err = result;
                        spdlog::error("Event '{}' callback error: {}", eventName, err.what());
                    }
                }
            }
        };

        // Include a Lua file (clears from cache for hot reloading)
        bestow["include"] = [this](const std::string& modulePath) -> sol::object {
            // Build absolute path from script directory
            auto scriptPath = std::filesystem::absolute(scriptPath_);
            auto scriptDir = scriptPath.parent_path();
            auto fullPath = scriptDir / modulePath;

            // Add .lua extension if not present
            if (!fullPath.has_extension()) {
                fullPath += ".lua";
            }

            if (!std::filesystem::exists(fullPath)) {
                spdlog::error("include: File not found: {}", fullPath.string());
                return sol::lua_nil;
            }

            // Clear from package.loaded to enable hot reloading
            std::string cacheKey = modulePath;
            // Replace / with .
            for (auto& c : cacheKey) {
                if (c == '/' || c == '\\') c = '.';
            }
            // Remove .lua extension
            if (cacheKey.size() > 4 && cacheKey.substr(cacheKey.size() - 4) == ".lua") {
                cacheKey = cacheKey.substr(0, cacheKey.size() - 4);
            }
            lua_["package"]["loaded"][cacheKey] = sol::lua_nil;

            // Load and execute the file
            auto result = lua_.safe_script_file(fullPath.string(), sol::script_pass_on_error);
            if (!result.valid()) {
                sol::error err = result;
                spdlog::error("include '{}': {}", modulePath, err.what());
                return sol::lua_nil;
            }

            spdlog::debug("Included: {}", fullPath.string());
            return result;
        };

        // Quit
        bestow["quit"] = [this]() {
            shouldQuit_ = true;
        };

        spdlog::debug("Game API registered");
    }

    void setupInputBindings() {
        // Set up default action mappings for common game actions
        if (!input_) return;

        // Helper to create keyboard binding
        auto keyBinding = [](int keyCode) -> InputBinding {
            return InputBinding{
                .deviceType = InputDeviceType::Keyboard,
                .keyCode = keyCode
            };
        };

        // Movement actions (Dvorak-friendly: ,AOE)
        input_->registerMapping({.binding = keyBinding(GLFW_KEY_COMMA), .action = "move_up"});
        input_->registerMapping({.binding = keyBinding(GLFW_KEY_UP), .action = "move_up"});
        input_->registerMapping({.binding = keyBinding(GLFW_KEY_W), .action = "move_up"});

        input_->registerMapping({.binding = keyBinding(GLFW_KEY_A), .action = "move_left"});
        input_->registerMapping({.binding = keyBinding(GLFW_KEY_LEFT), .action = "move_left"});

        input_->registerMapping({.binding = keyBinding(GLFW_KEY_O), .action = "move_down"});
        input_->registerMapping({.binding = keyBinding(GLFW_KEY_DOWN), .action = "move_down"});
        input_->registerMapping({.binding = keyBinding(GLFW_KEY_S), .action = "move_down"});

        input_->registerMapping({.binding = keyBinding(GLFW_KEY_E), .action = "move_right"});
        input_->registerMapping({.binding = keyBinding(GLFW_KEY_RIGHT), .action = "move_right"});
        input_->registerMapping({.binding = keyBinding(GLFW_KEY_D), .action = "move_right"});

        // Action buttons
        input_->registerMapping({.binding = keyBinding(GLFW_KEY_SPACE), .action = "confirm"});
        input_->registerMapping({.binding = keyBinding(GLFW_KEY_ENTER), .action = "confirm"});
        input_->registerMapping({.binding = keyBinding(GLFW_KEY_ESCAPE), .action = "cancel"});
    }
};

}  // namespace bestow::lua
