// bestow-runtime/src/main.cpp
// Bestow Lua Runtime - Entry Point (PROTOTYPE)
//
// Usage: bestow [options] <main.lua>
//
// This executable loads a Lua game script and runs it.
// Game developers write pure Lua - no C++ required!
//
// PROTOTYPE: Currently runs Lua scripts without full engine integration.
// Full integration will add: rendering, input, audio, physics.

#include <bestow/sol2_compat.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "LuaGameAPI.hpp"
#include "LuaBindings.hpp"

#include <iostream>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ============================================================================
// Command Line Parsing
// ============================================================================

struct RuntimeArgs {
    std::string mainScript;
    std::string configFile;
    std::vector<std::string> assetPaths;
    bool devMode = false;
    bool verbose = false;
    bool help = false;
    int maxFrames = 5;  // Prototype: limit frames for testing
};

void printUsage(const char* programName) {
    std::cout << "Bestow Lua Runtime (PROTOTYPE)\n\n";
    std::cout << "Usage: " << programName << " [options] <main.lua>\n\n";
    std::cout << "Options:\n";
    std::cout << "  --config <path>    Engine config file\n";
    std::cout << "  --assets <path>    Asset search path (can be specified multiple times)\n";
    std::cout << "  --dev              Enable dev tools (hot reload, debug overlay)\n";
    std::cout << "  --verbose          Verbose logging\n";
    std::cout << "  --frames <n>       Number of frames to simulate (default: 5)\n";
    std::cout << "  --help             Show this help message\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << programName << " games/snake/main.lua\n";
    std::cout << "  " << programName << " --verbose --frames 10 examples/lua-test-game/main.lua\n";
}

RuntimeArgs parseArgs(int argc, char* argv[]) {
    RuntimeArgs args;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            args.help = true;
        } else if (arg == "--dev") {
            args.devMode = true;
        } else if (arg == "--verbose" || arg == "-v") {
            args.verbose = true;
        } else if (arg == "--config") {
            if (i + 1 < argc) {
                args.configFile = argv[++i];
            }
        } else if (arg == "--assets") {
            if (i + 1 < argc) {
                args.assetPaths.push_back(argv[++i]);
            }
        } else if (arg == "--frames") {
            if (i + 1 < argc) {
                args.maxFrames = std::stoi(argv[++i]);
            }
        } else if (!arg.starts_with("-")) {
            args.mainScript = arg;
        }
    }

    return args;
}

// ============================================================================
// Main Entry Point
// ============================================================================

int main(int argc, char* argv[]) {
    // Parse command line arguments
    RuntimeArgs args = parseArgs(argc, argv);

    if (args.help) {
        printUsage(argv[0]);
        return 0;
    }

    if (args.mainScript.empty()) {
        std::cerr << "Error: No main.lua script specified\n\n";
        printUsage(argv[0]);
        return 1;
    }

    // Setup logging
    auto logger = spdlog::stdout_color_mt("bestow");
    spdlog::set_default_logger(logger);
    spdlog::set_level(args.verbose ? spdlog::level::debug : spdlog::level::info);

    spdlog::info("========================================");
    spdlog::info("Bestow Lua Runtime (PROTOTYPE)");
    spdlog::info("========================================");
    spdlog::info("Loading: {}", args.mainScript);

    // Check if script exists
    if (!fs::exists(args.mainScript)) {
        spdlog::critical("Script not found: {}", args.mainScript);
        return 1;
    }

    // Create Lua state
    sol::state lua;
    lua.open_libraries(
        sol::lib::base,
        sol::lib::math,
        sol::lib::string,
        sol::lib::table,
        sol::lib::coroutine
    );

    // Sandbox - remove dangerous functions
    lua["os"] = sol::lua_nil;
    lua["io"] = sol::lua_nil;
    lua["loadfile"] = sol::lua_nil;
    lua["dofile"] = sol::lua_nil;
    lua["load"] = sol::lua_nil;
    lua["loadstring"] = sol::lua_nil;
    lua["debug"] = sol::lua_nil;

    // Create and initialize Game API
    bestow::runtime::LuaGameAPI gameAPI(lua);
    gameAPI.initialize();
    gameAPI.registerInLua();

    // Set working directory to script's directory for relative paths
    fs::path scriptPath = fs::absolute(args.mainScript);
    fs::path scriptDir = scriptPath.parent_path();
    spdlog::debug("Script directory: {}", scriptDir.string());

    // Add script directory to Lua's package.path
    std::string packagePath = scriptDir.string() + "/?.lua;" +
                              scriptDir.string() + "/?/init.lua";
    lua["package"]["path"] = packagePath;

    // Load and execute the main script
    spdlog::info("Executing script...");
    auto result = lua.safe_script_file(args.mainScript, sol::script_pass_on_error);
    if (!result.valid()) {
        sol::error err = result;
        spdlog::critical("Failed to load {}: {}", args.mainScript, err.what());
        return 1;
    }

    // Check if game object was created
    sol::table game = lua["game"];
    if (!game.valid()) {
        spdlog::critical("main.lua must create a 'game' table");
        spdlog::info("Example:");
        spdlog::info("  game = {{");
        spdlog::info("      title = \"My Game\",");
        spdlog::info("      init = function(self) end,");
        spdlog::info("      update = function(self, dt) end,");
        spdlog::info("  }}");
        return 1;
    }

    // Parse game config
    bestow::runtime::GameConfig config = gameAPI.parseConfig(game);

    spdlog::info("----------------------------------------");
    spdlog::info("Game: {}", config.title);
    spdlog::info("Window: {}x{}", config.windowWidth, config.windowHeight);
    spdlog::info("----------------------------------------");

    // Call game:init() if defined
    if (game["init"].valid()) {
        spdlog::info("Calling game:init()...");
        sol::function initFn = game["init"];
        auto initResult = initFn(game);
        if (!initResult.valid()) {
            sol::error err = initResult;
            spdlog::error("game:init() error: {}", err.what());
            return 1;
        }
    }

    // ========================================================================
    // Simulate game loop (prototype mode)
    // In full implementation, this would be a real render loop
    // ========================================================================

    spdlog::info("");
    spdlog::info("=== PROTOTYPE MODE ===");
    spdlog::info("Simulating {} update frames...", args.maxFrames);
    spdlog::info("");

    float dt = 1.0f / 60.0f;  // 60 FPS
    for (int frame = 0; frame < args.maxFrames && !gameAPI.shouldQuit(); ++frame) {
        spdlog::debug("--- Frame {} ---", frame + 1);

        // Update game (calls Lua game:update(dt) and processes timers)
        gameAPI.update(dt);
    }

    spdlog::info("");

    // Call game:shutdown() if defined
    if (game["shutdown"].valid()) {
        spdlog::info("Calling game:shutdown()...");
        sol::function shutdownFn = game["shutdown"];
        auto shutdownResult = shutdownFn(game);
        if (!shutdownResult.valid()) {
            sol::error err = shutdownResult;
            spdlog::error("game:shutdown() error: {}", err.what());
        }
    }

    spdlog::info("========================================");
    spdlog::info("Bestow Lua Runtime finished");
    spdlog::info("========================================");
    return 0;
}
