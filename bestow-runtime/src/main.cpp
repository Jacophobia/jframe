// bestow-runtime/src/main.cpp
// Bestow Lua Runtime - Entry Point
//
// Usage: bestow [options] <main.lua>
//
// This executable loads a Lua game script and runs it with the full
// Bestow engine integration: Vulkan graphics, input, audio, etc.
//
// Game developers write pure Lua - no C++ required!

import std;
import bestow.core;          // Engine class
import bestow.services;      // Contract interfaces
import bestow.types;         // PathResolver
import bestow.lua.runtime;   // LuaApplication

// Import Bestow's default implementations
import bestow.vulkan.impl;   // VulkanGraphics3DSystem
import bestow.input.impl;    // InputSystem
import bestow.events.impl;   // EventSystem
import bestow.audio.impl;    // AudioSystem
import bestow.assets.impl;   // AssetSystem
import bestow.config.impl;   // ConfigSystem

//==========================================================================
// Command Line Parsing
//==========================================================================

struct RuntimeArgs {
    std::string mainScript;
    std::string configFile;
    std::string libraryPath;
    std::vector<std::string> assetPaths;
    bool devMode = false;
    bool verbose = false;
    bool help = false;
};

void printUsage(const char* programName) {
    std::cout << "Bestow Lua Runtime\n\n";
    std::cout << "Usage: " << programName << " [options] <main.lua>\n\n";
    std::cout << "Options:\n";
    std::cout << "  --library <path>   Asset library path (for :library:/ resolution)\n";
    std::cout << "  --config <path>    Engine config file\n";
    std::cout << "  --assets <path>    Asset search path (can be specified multiple times)\n";
    std::cout << "  --dev              Enable dev tools (hot reload, debug overlay)\n";
    std::cout << "  --verbose          Verbose logging\n";
    std::cout << "  --help             Show this help message\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << programName << " games/snake/main.lua\n";
    std::cout << "  " << programName << " --verbose examples/lua-test-game/main.lua\n";
    std::cout << "\nControls:\n";
    std::cout << "  ,AOE or Arrow keys for movement (Dvorak-friendly)\n";
    std::cout << "  Space/Enter to confirm\n";
    std::cout << "  Escape to quit\n";
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
        } else if (arg == "--library") {
            if (i + 1 < argc) {
                args.libraryPath = argv[++i];
            }
        } else if (arg == "--config") {
            if (i + 1 < argc) {
                args.configFile = argv[++i];
            }
        } else if (arg == "--assets") {
            if (i + 1 < argc) {
                args.assetPaths.push_back(argv[++i]);
            }
        } else if (!arg.starts_with("-")) {
            args.mainScript = arg;
        }
    }

    return args;
}

//==========================================================================
// Main Entry Point
//==========================================================================

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

    // Check if script exists
    if (!std::filesystem::exists(args.mainScript)) {
        std::cerr << "Error: Script not found: " << args.mainScript << "\n";
        return 1;
    }

    // Change working directory to executable directory so shaders can be found
    auto exeDir = std::filesystem::path(argv[0]).parent_path();
    if (!exeDir.empty() && std::filesystem::exists(exeDir)) {
        std::filesystem::current_path(exeDir);
    }

    // Initialize PathResolver with the executable path
    bestow::PathResolver::initialize(argv[0]);

    // Set custom library path if provided
    if (!args.libraryPath.empty()) {
        bestow::PathResolver::setLibraryPath(args.libraryPath);
    }

    // Configure the LuaApplication before engine.run()
    bestow::lua::LuaApplication::setScriptPath(args.mainScript);
    bestow::lua::LuaApplication::setVerbose(args.verbose);

    // Create the Engine - the composition root
    bestow::core::Engine engine;

    // Register system implementations with engine.use<Contract, Implementation>()
    engine.use<bestow::IEventSystem, bestow::EventSystem>();
    engine.use<bestow::IAssetSystem, bestow::AssetSystem>();
    engine.use<bestow::IConfigSystem, bestow::ConfigSystem>();
    engine.use<bestow::IGraphics3DSystem, bestow::VulkanGraphics3DSystem>();
    engine.use<bestow::IInputSystem, bestow::InputSystem>();

    // Audio (optional - don't fail if not available)
    try {
        engine.use<bestow::IAudioSystem, bestow::AudioSystem>();
    } catch (...) {
        std::cerr << "Warning: Audio system not available\n";
    }

    // Run the Lua application - dependencies auto-detected from Application<> base
    engine.run<bestow::lua::LuaApplication>();

    return 0;
}
