// bestow-launcher/src/GameRunner.cpp
// Game execution - sets up engine, Lua, and runs the game loop

#include <sol/sol.hpp>
#include <spdlog/spdlog.h>

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

namespace bestow::launcher {

class GameRunner {
public:
    GameRunner() = default;
    ~GameRunner() = default;

    /// Initialize the engine with all default systems
    bool initialize(const std::filesystem::path& mainScript,
                    bool verbose, bool debugMode) {
        mainScript_ = mainScript;
        gameRoot_ = mainScript.parent_path();
        verbose_ = verbose;
        debugMode_ = debugMode;

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

        // Register audio system
        engine_.use<IAudioSystem, FMODAudioSystem>();

        // Register graphics 3D system (Vulkan)
        engine_.use<IGraphics3DSystem, VulkanGraphics3DSystem>();

        // Register animation system
        engine_.use<IAnimationSystem, AnimationSystem>();

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

    void runGameLoop(sol::function& updateFunc, sol::function& renderFunc) {
        bool running = true;
        auto lastTime = std::chrono::high_resolution_clock::now();

        while (running) {
            // Calculate delta time
            auto now = std::chrono::high_resolution_clock::now();
            float dt = std::chrono::duration<float>(now - lastTime).count();
            lastTime = now;

            // Process hot reload
            if (scriptManager_) {
                scriptManager_->update();
            }

            // Call update
            auto updateResult = updateFunc(dt);
            if (!updateResult.valid()) {
                sol::error err = updateResult;
                spdlog::error("[GameRunner] Error in update(): {}", err.what());
                running = false;
                continue;
            }

            // Check if update returned false to stop the loop
            if (updateResult.get_type() == sol::type::boolean && !updateResult.get<bool>()) {
                running = false;
                continue;
            }

            // Call render if present
            if (renderFunc.valid()) {
                auto renderResult = renderFunc();
                if (!renderResult.valid()) {
                    sol::error err = renderResult;
                    spdlog::error("[GameRunner] Error in render(): {}", err.what());
                    running = false;
                    continue;
                }
            }

            // Small sleep to prevent spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    core::Engine engine_;
    sol::state lua_;
    std::unique_ptr<LuaContractBinder> binder_;
    std::unique_ptr<ScriptManager> scriptManager_;

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
