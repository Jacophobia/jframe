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
import bestow.physics3d.impl; // JoltPhysics3DSystem

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

        // Load inputs.lua for input action configuration (REQUIRED)
        if (!loadInputsConfig()) {
            spdlog::error("[GameRunner] Failed to load inputs.lua");
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

        // Register asset system (needed by other systems)
        engine_.use<IAssetSystem, AssetSystem>();

        // Register config system
        engine_.use<IConfigSystem, ConfigSystem>();

        // Register input system
        engine_.use<IInputSystem, InputSystem>();

        // Register audio system (uses FMOD if available, otherwise miniaudio)
        engine_.use<IAudioSystem, AudioSystem>();

        // Register graphics 3D system (Vulkan)
        engine_.use<IGraphics3DSystem, VulkanGraphics3DSystem>();

        // Register 3D physics system (Jolt)
        engine_.use<IPhysics3DSystem, JoltPhysics3DSystem>();

        // Register animation system
        engine_.use<IAnimationSystem, AnimationSystem>();

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

    bool loadInputsConfig() {
        auto inputsPath = gameRoot_ / "inputs.lua";

        // Check if inputs.lua exists (REQUIRED)
        if (!std::filesystem::exists(inputsPath)) {
            spdlog::error("[GameRunner] Required file 'inputs.lua' not found in game directory: {}",
                         gameRoot_.string());
            spdlog::error("[GameRunner] Create an inputs.lua file to define input actions. Example:");
            spdlog::error(R"(
-- inputs.lua - Input action definitions
-- Use bestow.action.builder() to register input actions

-- Example: Jump action
bestow.action.builder()
    :duringPhase("game")
    :whenPressed(bestow.input.keys.Space)
    :emitAction("Jump")
    :discretely()

-- Example: Movement action (continuous)
bestow.action.builder()
    :duringPhase("game")
    :whenActive(bestow.input.keys.D)
    :emitAction("MoveRight")
    :continuously()
)");
            return false;
        }

        spdlog::debug("[GameRunner] Loading inputs.lua: {}", inputsPath.string());

        // Execute inputs.lua to register actions
        auto result = lua_.safe_script_file(inputsPath.string(), sol::script_pass_on_error);
        if (!result.valid()) {
            sol::error err = result;
            spdlog::error("[GameRunner] Failed to load inputs.lua: {}", err.what());
            return false;
        }

        spdlog::info("[GameRunner] Loaded input configuration from inputs.lua");
        return true;
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

            // Call render if present
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
