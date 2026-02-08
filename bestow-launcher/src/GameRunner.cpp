// bestow-launcher/src/GameRunner.cpp
// Game execution - sets up engine, Lua, and runs the game loop

#include <sol/sol.hpp>
#include <spdlog/spdlog.h>

// CRITICAL for Windows: Include EnTT compatibility header before importing modules
// This defines comparison operators for EnTT iterators that MSVC's ADL can find
#include <bestow/entt_compat.hpp>

import std;
import bestow.core;
import bestow.services;
import bestow.types;      // PathResolver
import bestow.assets;     // AssetLibrary
import bestow.script;
import bestow.luabind;

// System implementations
import bestow.entity.impl;    // EntitySystem
import bestow.events.impl;    // EventSystem
import bestow.input.impl;     // InputSystem
import bestow.assets.impl;    // AssetSystem
import bestow.config.impl;    // ConfigSystem
import bestow.vulkan.impl;    // VulkanGraphics3DSystem
import bestow.audio.impl;     // FMODAudioSystem
import bestow.animation.impl; // AnimationSystem
import bestow.scene.impl;     // SceneSystem
import bestow.ui.impl;        // RmlUISystem

namespace bestow::launcher {

class GameRunner {
public:
    GameRunner() = default;
    ~GameRunner() {
        spdlog::info("[GameRunner] Destructor starting...");
        spdlog::default_logger()->flush();

        // CRITICAL: Call Lua destroy() function FIRST to let Lua clean up its own state
        // This unsubscribes from events, cancels timers, and releases sol::function refs
        // while Lua is still fully valid.
        callLuaDestroy();

        // Clear global binding managers that hold sol:: refs
        spdlog::info("[GameRunner] Cleaning up Lua bindings...");
        cleanupLuaBindings();

        // Run GC to collect any orphaned objects
        spdlog::info("[GameRunner] Running garbage collection...");
        lua_.collect_garbage();

        // Clear script manager (holds references to Lua tables)
        spdlog::info("[GameRunner] Clearing script manager...");
        scriptManager_.reset();

        // Clear Lua binder
        spdlog::info("[GameRunner] Clearing Lua binder...");
        binder_.reset();

        // Run GC again after clearing managers
        lua_.collect_garbage();

        // Clear all Lua globals to release references
        spdlog::info("[GameRunner] Clearing Lua globals...");
        lua_["bestow"] = sol::nil;
        lua_["app"] = sol::nil;

        // Final GC pass
        lua_.collect_garbage();
        lua_.collect_garbage();  // Run twice to catch weak refs

        // IMPORTANT: Destroy engine BEFORE lua_ because engine systems may store
        // callbacks that captured sol::function objects.
        spdlog::info("[GameRunner] Destroying engine...");
        engine_ = core::Engine();

        spdlog::info("[GameRunner] Engine destroyed, destructor body complete");
        spdlog::default_logger()->flush();

        // Let sol::state destructor handle lua_close() naturally
        // No manual close needed - the member destruction will handle it
    }

    /// Call the Lua destroy function to let game code clean up
    void callLuaDestroy() {
        spdlog::info("[GameRunner] Calling Lua destroy function...");

        try {
            sol::table app = lua_["app"];
            if (!app.valid()) {
                spdlog::debug("[GameRunner] No app table found, skipping destroy");
                return;
            }

            sol::table mainTable = app["main"];
            if (!mainTable.valid()) {
                spdlog::debug("[GameRunner] No app.main table found, skipping destroy");
                return;
            }

            sol::function destroyFunc = mainTable["destroy"];
            if (!destroyFunc.valid()) {
                spdlog::debug("[GameRunner] No app.main.destroy function found, skipping");
                return;
            }

            // Call destroy with self and nil scope
            auto result = destroyFunc(mainTable, sol::nil);
            if (!result.valid()) {
                sol::error err = result;
                spdlog::warn("[GameRunner] Error in app.main.destroy(): {}", err.what());
            } else {
                spdlog::info("[GameRunner] Lua destroy completed successfully");
            }
        } catch (const std::exception& e) {
            spdlog::warn("[GameRunner] Exception calling Lua destroy: {}", e.what());
        }
    }

    /// Initialize the engine with all default systems
    bool initialize(const std::filesystem::path& mainScript,
                    bool verbose, bool debugMode) {
        verbose_ = verbose;
        debugMode_ = debugMode;

        // Make path absolute to properly resolve parent directory
        mainScript_ = std::filesystem::absolute(mainScript);
        gameRoot_ = mainScript_.parent_path();

        if (verbose_) {
            spdlog::set_level(spdlog::level::debug);
        }

        spdlog::info("[GameRunner] Initializing Bestow Engine...");
        spdlog::info("[GameRunner] Game root: {}", gameRoot_.string());
        spdlog::info("[GameRunner] Main script: {}", mainScript_.string());

        // Initialize PathResolver with library path
        initializePathResolver();

        // Register all default systems
        registerDefaultSystems();

        // Initialize Lua state
        if (!initializeLua()) {
            spdlog::error("[GameRunner] Failed to initialize Lua state");
            return false;
        }

        // Bind C++ contracts to Lua
        if (!bindContracts()) {
            spdlog::error("[GameRunner] Failed to bind C++ contracts to Lua");
            return false;
        }

        // Load config/*.cfg.lua files and initialize systems
        if (!loadConfigs()) {
            spdlog::error("[GameRunner] Failed to load configuration");
            return false;
        }

        // Initialize script manager for app.* namespace
        if (!initializeScriptManager()) {
            spdlog::error("[GameRunner] Failed to initialize script manager");
            return false;
        }

        // Load and execute main.lua
        if (!loadMainScript()) {
            spdlog::error("[GameRunner] Failed to load main script");
            return false;
        }

        initialized_ = true;
        spdlog::info("[GameRunner] Initialization complete");
        return true;
    }

    /// Run the game
    int run() {
        if (!initialized_) {
            spdlog::error("[GameRunner] Not initialized");
            return 1;
        }

        spdlog::info("[GameRunner] Starting game...");

        // Check if main.lua has a run() function
        sol::table app = lua_["app"];
        sol::table mainTable = app["main"];

        if (!mainTable.valid()) {
            spdlog::error("[GameRunner] app.main table not found in main.lua");
            return 1;
        }

        // Try to get and call the run function
        sol::function runFunc = mainTable["run"];
        if (runFunc.valid()) {
            spdlog::info("[GameRunner] Calling app.main.run()...");
            auto result = runFunc();
            if (!result.valid()) {
                sol::error err = result;
                spdlog::error("[GameRunner] Error in app.main.run(): {}", err.what());
                return 1;
            }
        } else {
            // If no run() function, try the update loop pattern
            sol::function initFunc = mainTable["init"];
            sol::function updateFunc = mainTable["update"];
            sol::function renderFunc = mainTable["render"];

            if (!updateFunc.valid()) {
                spdlog::error("[GameRunner] app.main has no run() or update() function");
                return 1;
            }

            // Call init if present
            if (initFunc.valid()) {
                spdlog::info("[GameRunner] Calling app.main.init()...");
                auto result = initFunc();
                if (!result.valid()) {
                    sol::error err = result;
                    spdlog::error("[GameRunner] Error in app.main.init(): {}", err.what());
                    return 1;
                }
            }

            // Validate that a phase has been set (REQUIRED for event-driven input)
            if (!validatePhaseSet()) {
                return 1;
            }

            // Run the game loop
            spdlog::info("[GameRunner] Entering game loop...");
            runGameLoop(updateFunc, renderFunc);
        }

        spdlog::info("[GameRunner] Game finished");
        return 0;
    }

private:
    void initializePathResolver() {
        // Initialize PathResolver with basic defaults
        PathResolver::initialize();

        // Use AssetLibrary for robust auto-detection of library path
        // (checks env var, exe-relative, cwd-relative locations)
        auto assetLib = AssetLibrary::create();
        if (assetLib) {
            PathResolver::setLibraryPath(assetLib->root().string());
            spdlog::info("[GameRunner] Using library path: {}", assetLib->root().string());
        } else {
            spdlog::warn("[GameRunner] Could not auto-detect asset library directory");
            spdlog::warn("[GameRunner] Set BESTOW_LIBRARY_PATH environment variable to specify the location");
            spdlog::info("[GameRunner] Using fallback library path: {}", PathResolver::getLibraryPath().string());
        }

        // Set assets path to game root (where main.lua is located)
        spdlog::info("[GameRunner] Using assets path: {}", gameRoot_.string());
        PathResolver::setAssetsPath(gameRoot_.string());
    }

    void registerDefaultSystems() {
        spdlog::debug("[GameRunner] Registering default systems...");

        // Register entity system first (foundation for all other systems)
        engine_.use<IEntitySystem, EntitySystem>();

        // Register event system
        engine_.use<IEventSystem, EventSystem>();

        // Register asset system (depends on EventSystem)
        engine_.use<IAssetSystem, AssetSystem>(
            [](di::ServiceProvider& sp) { return new AssetSystem(sp.get<IEventSystem>()); });

        // Register config system (depends on AssetSystem, EventSystem)
        engine_.use<IConfigSystem, ConfigSystem>(
            [](di::ServiceProvider& sp) {
                return new ConfigSystem(sp.get<IAssetSystem>(), sp.get<IEventSystem>());
            });

        // Register input system (depends on EventSystem, AssetSystem)
        engine_.use<IInputSystem, InputSystem>(
            [](di::ServiceProvider& sp) {
                return new InputSystem(sp.get<IEventSystem>(), sp.get<IAssetSystem>());
            });

        // Register audio system (depends on AssetSystem)
        engine_.use<IAudioSystem, AudioSystem>(
            [](di::ServiceProvider& sp) { return new AudioSystem(sp.get<IAssetSystem>()); });

        // Register graphics 3D system (depends on AssetSystem, ConfigSystem)
        engine_.use<IGraphics3DSystem, VulkanGraphics3DSystem>(
            [](di::ServiceProvider& sp) {
                return new VulkanGraphics3DSystem(sp.get<IAssetSystem>(), sp.get<IConfigSystem>());
            });

        // Register UI system (depends on IGraphicsContext [base of IGraphics3DSystem], IAssetSystem)
        engine_.use<IUISystem, RmlUISystem>(
            [](di::ServiceProvider& sp) {
                return new RmlUISystem(sp.get<IGraphics3DSystem>(), sp.get<IAssetSystem>());
            });

        // Register scene system (depends on IAssetSystem, IInputSystem, IUISystem, IEventSystem)
        engine_.use<ISceneSystem, SceneSystem>(
            [](di::ServiceProvider& sp) {
                return new SceneSystem(
                    sp.get<IAssetSystem>(), sp.get<IInputSystem>(),
                    sp.get<IUISystem>(), sp.get<IEventSystem>());
            });

        // Register animation system
        engine_.use<IAnimationSystem, AnimationSystem>();

        // Build the service provider so has<>() and get<>() work
        engine_.build();

        // Initialize scene system with Lua state
        if (engine_.has<ISceneSystem>()) {
            auto& scene = dynamic_cast<SceneSystem&>(engine_.get<ISceneSystem>());
            scene.initialize(&lua_);
        }

        // Initialize animation system (required before use)
        if (engine_.has<IAnimationSystem>()) {
            engine_.get<IAnimationSystem>().initialize();
        }

        spdlog::debug("[GameRunner] Default systems registered");
    }

    bool initializeLua() {
        spdlog::debug("[GameRunner] Initializing Lua state...");

        // Open standard libraries
        lua_.open_libraries(
            sol::lib::base,
            sol::lib::math,
            sol::lib::table,
            sol::lib::string,
            sol::lib::coroutine,
            sol::lib::os,      // Limited - time functions only
            sol::lib::utf8
        );

        // Remove dangerous functions for sandboxing
        lua_["os"]["execute"] = sol::nil;
        lua_["os"]["exit"] = sol::nil;
        lua_["os"]["remove"] = sol::nil;
        lua_["os"]["rename"] = sol::nil;
        lua_["os"]["setlocale"] = sol::nil;
        lua_["os"]["getenv"] = sol::nil;
        lua_["os"]["tmpname"] = sol::nil;
        lua_["io"] = sol::nil;
        lua_["loadfile"] = sol::nil;
        lua_["dofile"] = sol::nil;
        lua_["load"] = sol::nil;

        spdlog::debug("[GameRunner] Lua state initialized");
        return true;
    }

    bool bindContracts() {
        spdlog::debug("[GameRunner] Binding C++ contracts to Lua...");

        binder_ = std::make_unique<LuaContractBinder>(engine_, lua_);
        binder_->bindAll();

        spdlog::debug("[GameRunner] Contracts bound");
        return true;
    }

    bool initializeScriptManager() {
        spdlog::debug("[GameRunner] Initializing script manager...");

        scriptManager_ = std::make_unique<ScriptManager>(lua_);
        scriptManager_->initialize(gameRoot_);

        if (!debugMode_) {
            scriptManager_->enableHotReload(true);
        }

        // Load all scripts in the game directory
        scriptManager_->loadAllScripts();

        spdlog::debug("[GameRunner] Script manager initialized");
        return true;
    }

    bool loadMainScript() {
        spdlog::debug("[GameRunner] Loading main script: {}", mainScript_.string());

        bool success = scriptManager_->loadScript(mainScript_);
        if (!success) {
            spdlog::error("[GameRunner] Failed to load main script");
            return false;
        }

        spdlog::debug("[GameRunner] Main script loaded");
        return true;
    }

    //=========================================================================
    // Config file loading — reads config/*.cfg.lua and initializes systems
    //=========================================================================

    /// Load a .cfg.lua file and return the table it produces.
    /// Config files are executed in a bare sandbox — no access to bestow.*,
    /// math.*, string.*, or any other globals.  Only Lua syntax (table
    /// constructors, literals, local variables) is available.
    /// Returns sol::nil if the file doesn't exist or fails to parse.
    sol::object loadConfigFile(const std::string& relativePath) {
        auto fullPath = gameRoot_ / relativePath;
        if (!std::filesystem::exists(fullPath)) {
            return sol::nil;
        }

        spdlog::debug("[GameRunner] Loading config: {}", fullPath.string());

        // Bare environment — config files are pure data, no API access
        sol::environment sandbox(lua_, sol::create);

        auto result = lua_.safe_script_file(fullPath.string(), sandbox,
                                            sol::script_pass_on_error);
        if (!result.valid()) {
            sol::error err = result;
            spdlog::error("[GameRunner] Failed to parse {}: {}", relativePath, err.what());
            return sol::nil;
        }

        return result.get<sol::object>();
    }

    /// Master config loader — loads all config/*.cfg.lua files
    bool loadConfigs() {
        spdlog::info("[GameRunner] Loading configuration files...");

        // Input config is REQUIRED (replaces inputs.lua)
        if (!loadInputConfig()) {
            return false;
        }

        // Graphics config (optional — falls back to defaults)
        loadGraphicsConfig();

        // Audio config (optional)
        loadAudioConfig();

        // UI config (optional)
        loadUIConfig();

        // Save config (stub — just log that we loaded it)
        auto saveCfg = loadConfigFile("config/save.cfg.lua");
        if (saveCfg.valid() && saveCfg.get_type() == sol::type::table) {
            spdlog::info("[GameRunner] Loaded save config (save system not yet implemented)");
        }

        spdlog::info("[GameRunner] Configuration loaded");
        return true;
    }

    /// Load config/input.cfg.lua and register actions with the input system
    bool loadInputConfig() {
        auto cfg = loadConfigFile("config/input.cfg.lua");

        if (!cfg.valid() || cfg.get_type() != sol::type::table) {
            spdlog::error("[GameRunner] Required file 'config/input.cfg.lua' not found or invalid");
            spdlog::error("[GameRunner] Create config/input.cfg.lua that returns a table with an 'actions' array.");
            return false;
        }

        if (!engine_.has<IInputSystem>()) {
            spdlog::error("[GameRunner] No input system registered");
            return false;
        }

        auto& input = engine_.get<IInputSystem>();
        sol::table config = cfg;

        // Apply hold threshold if present
        sol::optional<float> holdThreshold = config["holdThreshold"];
        if (holdThreshold) {
            input.setDefaultHoldThreshold(*holdThreshold);
        }

        // Enum lookup tables (already bound by bindContracts)
        sol::table keyCodeEnum   = lua_["KeyCode"];
        sol::table gamepadBtnEnum = lua_["GamepadButton"];
        sol::table gamepadAxisEnum = lua_["GamepadAxis"];

        // Process actions array
        sol::optional<sol::table> actions = config["actions"];
        if (!actions) {
            spdlog::warn("[GameRunner] config/input.cfg.lua has no 'actions' array");
            return true;
        }

        int registered = 0;
        for (auto& pair : *actions) {
            sol::table def = pair.second.as<sol::table>();
            std::string actionName = def.get<std::string>("action");
            std::string mode = def.get_or<std::string>("mode", "discrete");

            sol::optional<sol::table> phases = def["phases"];
            if (!phases) continue;

            for (auto& phasePair : *phases) {
                std::string phase = phasePair.second.as<std::string>();

                // Register keyboard bindings
                sol::optional<sol::table> keys = def["keys"];
                if (keys) {
                    for (auto& keyPair : *keys) {
                        std::string keyName = keyPair.second.as<std::string>();
                        sol::object keyObj = keyCodeEnum[keyName];
                        if (!keyObj.valid() || !keyObj.is<KeyCode>()) {
                            spdlog::warn("[GameRunner] Unknown key '{}' in action '{}'", keyName, actionName);
                            continue;
                        }
                        KeyCode key = keyObj.as<KeyCode>();

                        ActionRegistration reg;
                        reg.phase = phase;
                        ActionCondition cond;
                        cond.input = InputBinding::key(key);
                        cond.type = (mode == "discrete")
                            ? ActionConditionType::WhenPressed
                            : ActionConditionType::WhenActive;
                        reg.conditions.push_back(cond);
                        ActionEffect effect;
                        effect.type = ActionEffectType::EmitAction;
                        effect.value = actionName;
                        reg.effects.push_back(effect);
                        reg.terminal = (mode == "discrete")
                            ? ActionTerminal::Discrete
                            : ActionTerminal::Continuous;
                        reg.valid = true;
                        input.registerAction(reg);
                        registered++;
                    }
                }

                // Register gamepad button bindings
                sol::optional<sol::table> buttons = def["buttons"];
                if (buttons) {
                    for (auto& btnPair : *buttons) {
                        std::string btnName = btnPair.second.as<std::string>();
                        sol::object btnObj = gamepadBtnEnum[btnName];
                        if (!btnObj.valid() || !btnObj.is<GamepadButton>()) {
                            spdlog::warn("[GameRunner] Unknown button '{}' in action '{}'", btnName, actionName);
                            continue;
                        }
                        GamepadButton btn = btnObj.as<GamepadButton>();

                        ActionRegistration reg;
                        reg.phase = phase;
                        ActionCondition cond;
                        cond.input = InputBinding::gamepadButton(btn);
                        cond.type = (mode == "discrete")
                            ? ActionConditionType::WhenPressed
                            : ActionConditionType::WhenActive;
                        reg.conditions.push_back(cond);
                        ActionEffect effect;
                        effect.type = ActionEffectType::EmitAction;
                        effect.value = actionName;
                        reg.effects.push_back(effect);
                        reg.terminal = (mode == "discrete")
                            ? ActionTerminal::Discrete
                            : ActionTerminal::Continuous;
                        reg.valid = true;
                        input.registerAction(reg);
                        registered++;
                    }
                }

                // Register gamepad axis bindings
                sol::optional<sol::table> axes = def["axes"];
                if (axes) {
                    float deadzone = def.get_or(std::string("deadzone"), 0.15f);
                    for (auto& axisPair : *axes) {
                        std::string axisName = axisPair.second.as<std::string>();
                        sol::object axisObj = gamepadAxisEnum[axisName];
                        if (!axisObj.valid() || !axisObj.is<GamepadAxis>()) {
                            spdlog::warn("[GameRunner] Unknown axis '{}' in action '{}'", axisName, actionName);
                            continue;
                        }
                        GamepadAxis axis = axisObj.as<GamepadAxis>();

                        ActionRegistration reg;
                        reg.phase = phase;
                        reg.deadzone = deadzone;
                        ActionCondition cond;
                        cond.input = InputBinding::gamepadAxis(axis);
                        cond.type = ActionConditionType::WhenActive;
                        reg.conditions.push_back(cond);
                        ActionEffect effect;
                        effect.type = ActionEffectType::EmitAction;
                        effect.value = actionName;
                        reg.effects.push_back(effect);
                        reg.terminal = ActionTerminal::Continuous;
                        reg.valid = true;
                        input.registerAction(reg);
                        registered++;
                    }
                }
            }
        }

        spdlog::info("[GameRunner] Registered {} input bindings from config/input.cfg.lua", registered);
        return true;
    }

    /// Load config/graphics.cfg.lua and initialize the graphics system
    void loadGraphicsConfig() {
        auto cfg = loadConfigFile("config/graphics.cfg.lua");
        if (!cfg.valid() || cfg.get_type() != sol::type::table) return;
        if (!engine_.has<IGraphics3DSystem>()) return;

        auto& graphics = engine_.get<IGraphics3DSystem>();
        sol::table config = cfg;

        // Window initialization
        sol::optional<sol::table> window = config["window"];
        if (window) {
            Graphics3DConfig gfxConfig{};
            gfxConfig.windowWidth  = window->get_or("width", 1280);
            gfxConfig.windowHeight = window->get_or("height", 720);
            gfxConfig.windowTitle  = window->get_or<std::string>("title", "Bestow Application");
            gfxConfig.vsync        = window->get_or("vsync", true);
            // Parse windowMode string, fall back to legacy fullscreen bool
            auto wmStr = window->get<sol::optional<std::string>>("windowMode");
            if (wmStr) {
                if (*wmStr == "borderless") gfxConfig.windowMode = WindowMode::BorderlessFullscreen;
                else if (*wmStr == "fullscreen") gfxConfig.windowMode = WindowMode::Fullscreen;
                else gfxConfig.windowMode = WindowMode::Windowed;
            } else {
                bool fs = window->get_or("fullscreen", false);
                gfxConfig.windowMode = fs ? WindowMode::BorderlessFullscreen : WindowMode::Windowed;
            }
            graphics.initialize(gfxConfig);
            spdlog::info("[GameRunner] Graphics initialized: {}x{} '{}'",
                         gfxConfig.windowWidth, gfxConfig.windowHeight, gfxConfig.windowTitle);
        }

        // Clear color
        sol::optional<sol::table> rendering = config["rendering"];
        if (rendering) {
            sol::optional<sol::table> cc = (*rendering)["clearColor"];
            if (cc) {
                float r = cc->get_or("r", 0.05f);
                float g = cc->get_or("g", 0.05f);
                float b = cc->get_or("b", 0.08f);
                float a = cc->get_or("a", 1.0f);
                graphics.setClearColor(Color::fromFloat(r, g, b, a));
            }
        }

        // Camera defaults
        sol::optional<sol::table> camera = config["camera"];
        if (camera) {
            Camera3D cam;
            cam.fovY      = camera->get_or("fov", 60.0f);
            cam.nearPlane  = camera->get_or("near", 0.1f);
            cam.farPlane   = camera->get_or("far", 1000.0f);
            graphics.setCamera(cam);
        }

        // Ambient lighting
        sol::optional<sol::table> lighting = config["lighting"];
        if (lighting) {
            sol::optional<sol::table> ambient = (*lighting)["ambient"];
            if (ambient) {
                Vec3 color{
                    ambient->get_or("r", 0.1f),
                    ambient->get_or("g", 0.1f),
                    ambient->get_or("b", 0.15f)
                };
                graphics.setAmbientLight(color);
            }
        }

        spdlog::info("[GameRunner] Applied graphics configuration");
    }

    /// Load config/audio.cfg.lua and set audio volumes
    void loadAudioConfig() {
        auto cfg = loadConfigFile("config/audio.cfg.lua");
        if (!cfg.valid() || cfg.get_type() != sol::type::table) return;
        if (!engine_.has<IAudioSystem>()) return;

        auto& audio = engine_.get<IAudioSystem>();
        sol::table config = cfg;

        // Master volume
        sol::optional<float> master = config["masterVolume"];
        if (master) {
            audio.setMasterVolume(*master);
        }

        // Group volumes
        sol::optional<sol::table> groups = config["groups"];
        if (groups) {
            for (auto& pair : *groups) {
                std::string group = pair.first.as<std::string>();
                float volume = pair.second.as<float>();
                audio.setGroupVolume(group, volume);
            }
        }

        spdlog::info("[GameRunner] Applied audio configuration");
    }

    /// Load config/ui.cfg.lua and initialize the UI system
    void loadUIConfig() {
        auto cfg = loadConfigFile("config/ui.cfg.lua");
        if (!cfg.valid() || cfg.get_type() != sol::type::table) return;
        if (!engine_.has<IUISystem>()) {
            spdlog::debug("[GameRunner] No UI system registered, skipping UI config");
            return;
        }

        auto& ui = engine_.get<IUISystem>();
        sol::table config = cfg;

        UIConfig uiConfig{};
        uiConfig.baseScale       = config.get_or("baseScale", 1.0f);
        uiConfig.enableDebugMode = config.get_or("enableDebugMode", false);

        sol::optional<std::string> themePath = config["themePath"];
        sol::optional<std::string> assetsPath = config["assetsPath"];
        sol::optional<std::string> fontsPath = config["fontsPath"];

        if (assetsPath) uiConfig.assetsPath = *assetsPath;
        if (fontsPath)  uiConfig.fontsPath  = *fontsPath;

        auto result = ui.initialize(uiConfig);
        if (!result.has_value()) {
            spdlog::warn("[GameRunner] UI initialization failed");
            return;
        }

        // Load fonts from config array
        sol::optional<sol::table> fontsList = config["fonts"];
        if (fontsList) {
            for (auto& [key, val] : fontsList.value()) {
                if (val.get_type() == sol::type::string) {
                    std::string fontPath = val.as<std::string>();
                    auto fontResult = ui.loadFont(fontPath, "default");
                    if (fontResult.has_value()) {
                        spdlog::info("[GameRunner] Loaded UI font: {}", fontPath);
                    } else {
                        spdlog::warn("[GameRunner] Failed to load UI font: {}", fontPath);
                    }
                }
            }
        }

        // Load theme stylesheet
        if (themePath) {
            auto sheetResult = ui.loadStyleSheet(*themePath);
            if (sheetResult.has_value()) {
                spdlog::info("[GameRunner] Loaded UI theme: {}", *themePath);
            } else {
                spdlog::warn("[GameRunner] Failed to load UI theme: {}", *themePath);
            }
        }

        spdlog::info("[GameRunner] Applied UI configuration");
    }

    bool validatePhaseSet() {
        // Get the input system and check if a phase has been set
        if (!engine_.has<IInputSystem>()) {
            spdlog::warn("[GameRunner] No input system registered, skipping phase validation");
            return true;
        }

        auto& input = engine_.get<IInputSystem>();
        if (!input.hasPhaseBeenSet()) {
            spdlog::error("[GameRunner] No initial phase set!");
            spdlog::error("[GameRunner] You must call bestow.phase.change('your_phase') in main.lua's init() function.");
            spdlog::error("[GameRunner] Example:");
            spdlog::error(R"(
-- main.lua
local main = {}

function main.init()
    -- Set the initial phase (REQUIRED)
    bestow.phase.change("game")
end

function main.update(dt)
    -- Your game logic
end

return { main = main }
)");
            return false;
        }

        spdlog::debug("[GameRunner] Input phase validated: {}", input.getCurrentPhase());
        return true;
    }

    void runGameLoop(sol::function& updateFunc, sol::function& renderFunc) {
        spdlog::info("[GameRunner] Entering runGameLoop()");
        bool running = true;
        auto lastTime = std::chrono::high_resolution_clock::now();
        std::uint64_t frameCount = 0;

        while (running) {
            // Calculate delta time
            auto now = std::chrono::high_resolution_clock::now();
            float dt = std::chrono::duration<float>(now - lastTime).count();
            lastTime = now;

            // Log first few frames and then periodically
            if (frameCount < 3 || frameCount % 60 == 0) {
                spdlog::debug("[GameRunner] Frame {} - dt={:.4f}s", frameCount, dt);
            }

            // Process hot reload
            if (scriptManager_) {
                scriptManager_->update();
            }

            // Update timers (hot-reload-safe timer callbacks)
            updateTimers(dt);

            // Update animation system (calculates bone transforms)
            if (engine_.has<IAnimationSystem>()) {
                engine_.get<IAnimationSystem>().update(dt);
            }

            // Update scene system (runs active scene's Lua update)
            if (engine_.has<ISceneSystem>()) {
                engine_.get<ISceneSystem>().update(dt);
            }

            // Call update
            if (frameCount == 0) {
                spdlog::debug("[GameRunner] Calling Lua update() for the first time...");
            }
            auto updateResult = updateFunc(dt);
            if (!updateResult.valid()) {
                sol::error err = updateResult;
                spdlog::error("[GameRunner] Error in update(): {}", err.what());
                running = false;
                continue;
            }

            // Check if update returned false to stop the loop
            if (updateResult.get_type() == sol::type::boolean && !updateResult.get<bool>()) {
                spdlog::info("[GameRunner] update() returned false, stopping game loop");
                running = false;
                continue;
            }

            // Begin frame (acquires swapchain image, starts render pass)
            if (engine_.has<IGraphics3DSystem>()) {
                engine_.get<IGraphics3DSystem>().beginFrame();
            }

            // Render scene system (runs active scene's Lua render)
            if (engine_.has<ISceneSystem>()) {
                engine_.get<ISceneSystem>().render();
            }

            // Update and render UI system (overlay, after 3D rendering)
            if (engine_.has<IUISystem>()) {
                auto& ui = engine_.get<IUISystem>();
                ui.update(dt);
                ui.render();
            }

            // Call render if present (game-level overlay, after scene + UI)
            if (renderFunc.valid()) {
                if (frameCount == 0) {
                    spdlog::debug("[GameRunner] Calling Lua render() for the first time...");
                }
                auto renderResult = renderFunc();
                if (!renderResult.valid()) {
                    sol::error err = renderResult;
                    spdlog::error("[GameRunner] Error in render(): {}", err.what());
                    running = false;
                    continue;
                }
            }

            // End frame (submits commands, presents, polls GLFW events)
            if (engine_.has<IGraphics3DSystem>()) {
                engine_.get<IGraphics3DSystem>().endFrame();
            }

            // Check if window was closed
            if (engine_.has<IGraphics3DSystem>() &&
                engine_.get<IGraphics3DSystem>().shouldClose()) {
                spdlog::info("[GameRunner] Window close requested");
                running = false;
            }

            frameCount++;

            // Small sleep to prevent spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        spdlog::info("[GameRunner] Exited game loop after {} frames", frameCount);
    }

    // RAII helper to log destruction phases
    struct DestructionLogger {
        const char* name;
        DestructionLogger(const char* n) : name(n) {}
        ~DestructionLogger() {
            spdlog::info("[GameRunner] Auto-destroying: {}", name);
            spdlog::default_logger()->flush();
        }
    };

    // CRITICAL: Member destruction order matters!
    // Members are destroyed in REVERSE order of declaration.
    // We need: scriptManager -> binder -> engine -> lua
    // So declare: lua (first, destroyed LAST) -> engine -> binder -> scriptManager
    DestructionLogger log0_{"lua_ complete"};
    sol::state lua_;                                    // Destroyed LAST - must outlive engine
    DestructionLogger log1_{"engine_ complete"};
    core::Engine engine_;                               // Destroyed after binder/scriptManager
    DestructionLogger log2_{"binder_ complete"};
    std::unique_ptr<LuaContractBinder> binder_;
    DestructionLogger log3_{"scriptManager_ complete"};
    std::unique_ptr<ScriptManager> scriptManager_;
    DestructionLogger log4_{"start of member destruction"};

    std::filesystem::path mainScript_;
    std::filesystem::path gameRoot_;
    bool verbose_ = false;
    bool debugMode_ = false;
    bool initialized_ = false;
};

// Free functions for main.cpp to use (avoids incomplete type issues with unique_ptr)
GameRunner* createGameRunner() {
    return new GameRunner();
}

void destroyGameRunner(GameRunner* runner) {
    delete runner;
}

bool initializeRunner(GameRunner* runner, const std::filesystem::path& mainScript,
                      bool verbose, bool debug) {
    return runner->initialize(mainScript, verbose, debug);
}

int runGame(GameRunner* runner) {
    return runner->run();
}

}  // namespace bestow::launcher
