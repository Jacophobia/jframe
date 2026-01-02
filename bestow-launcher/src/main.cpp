// bestow-launcher/src/main.cpp
// Single executable entry point for Bestow Engine

#include <spdlog/spdlog.h>

import std;
import bestow.luabind;  // For StubGenerator

using bestow::StubGenerator;

// Forward declarations from CommandLine.cpp
namespace bestow::launcher {

enum class Command {
    None,
    Run,
    GenerateStubs,
    New,
    Version,
    Help
};

struct CommandLineArgs {
    Command command = Command::None;
    std::filesystem::path mainScript;
    std::filesystem::path outputDir;
    std::filesystem::path assetLibraryPath;
    std::string projectName;
    bool verbose = false;
    bool debug = false;
};

void printUsage(const char* programName);
void printVersion();
std::optional<CommandLineArgs> parseCommandLine(int argc, char* argv[]);

// Forward declarations from GameRunner.cpp
class GameRunner;
GameRunner* createGameRunner();
void destroyGameRunner(GameRunner* runner);
bool initializeRunner(GameRunner* runner, const std::filesystem::path& mainScript,
                      const std::filesystem::path& assetLibraryPath, bool verbose, bool debug);
int runGame(GameRunner* runner);

// Custom deleter that calls destroyGameRunner
struct GameRunnerDeleter {
    void operator()(GameRunner* runner) const {
        destroyGameRunner(runner);
    }
};

using GameRunnerPtr = std::unique_ptr<GameRunner, GameRunnerDeleter>;

}  // namespace bestow::launcher

namespace bestow::launcher {

//=============================================================================
// Command Handlers
//=============================================================================

int handleRun(const CommandLineArgs& args) {
    spdlog::info("Bestow Engine - Running game...");

    GameRunnerPtr runner(createGameRunner());
    if (!initializeRunner(runner.get(), args.mainScript, args.assetLibraryPath,
                          args.verbose, args.debug)) {
        spdlog::error("Failed to initialize game runner");
        return 1;
    }

    return runGame(runner.get());
}

int handleGenerateStubs(const CommandLineArgs& args) {
    spdlog::info("Bestow Engine - Generating IDE stubs...");
    spdlog::info("Output directory: {}", args.outputDir.string());

    // Use the StubGenerator to create EmmyLua-annotated stub files
    StubGenerator generator;
    generator.generate(args.outputDir);

    spdlog::info("Stub files generated successfully!");
    spdlog::info("");
    spdlog::info("To use with VS Code + Lua Language Server:");
    spdlog::info("  Add to .vscode/settings.json:");
    spdlog::info("  {{");
    spdlog::info("    \"Lua.workspace.library\": [\"{}\"]", args.outputDir.string());
    spdlog::info("  }}");

    return 0;
}

int handleNew(const CommandLineArgs& args) {
    spdlog::info("Bestow Engine - Creating new project...");
    spdlog::info("Project name: {}", args.projectName);

    std::filesystem::path projectPath = std::filesystem::current_path() / args.projectName;

    if (std::filesystem::exists(projectPath)) {
        spdlog::error("Directory already exists: {}", projectPath.string());
        return 1;
    }

    // Create project structure
    std::filesystem::create_directories(projectPath);
    std::filesystem::create_directories(projectPath / "entities");
    std::filesystem::create_directories(projectPath / "systems");
    std::filesystem::create_directories(projectPath / "levels");
    std::filesystem::create_directories(projectPath / "assets" / "textures");
    std::filesystem::create_directories(projectPath / "assets" / "sounds");
    std::filesystem::create_directories(projectPath / "assets" / "materials");

    // Create main.lua template
    std::ofstream mainLua(projectPath / "main.lua");
    mainLua << R"lua(-- main.lua
-- Entry point for your Bestow game

return {
    title = ")lua" << args.projectName << R"lua(",
    width = 1280,
    height = 720,

    init = function()
        -- Initialize game state
        print("Game initialized!")
    end,

    update = function(dt)
        -- Update game logic each frame
        -- dt is the delta time in seconds

        -- Return false to stop the game loop
        return true
    end,

    render = function()
        -- Render the game
        -- bestow.graphics3d is available for 3D rendering
    end,

    run = function()
        -- Main game loop
        local main = app.main
        main.init()

        while not bestow.input.isActionActive("quit") do
            local dt = bestow.core.deltaTime()
            if not main.update(dt) then
                break
            end
            main.render()
        end
    end
}
)lua";
    mainLua.close();

    spdlog::info("Created new project: {}", projectPath.string());
    spdlog::info("");
    spdlog::info("To run your game:");
    spdlog::info("  bestow run {}/main.lua", args.projectName);
    spdlog::info("");

    return 0;
}

int handleVersion([[maybe_unused]] const CommandLineArgs& args) {
    printVersion();
    return 0;
}

int handleHelp(const CommandLineArgs& args, const char* programName) {
    printUsage(programName);
    return 0;
}

}  // namespace bestow::launcher

//=============================================================================
// Main Entry Point
//=============================================================================

int main(int argc, char* argv[]) {
    using namespace bestow::launcher;

    // Parse command line arguments
    auto args = parseCommandLine(argc, argv);
    if (!args) {
        return 1;
    }

    // Set log level based on verbose flag
    if (args->verbose) {
        spdlog::set_level(spdlog::level::debug);
    } else {
        spdlog::set_level(spdlog::level::info);
    }

    // Dispatch to appropriate command handler
    switch (args->command) {
        case Command::Run:
            return handleRun(*args);

        case Command::GenerateStubs:
            return handleGenerateStubs(*args);

        case Command::New:
            return handleNew(*args);

        case Command::Version:
            return handleVersion(*args);

        case Command::Help:
            return handleHelp(*args, argv[0]);

        case Command::None:
        default:
            spdlog::error("No command specified");
            printUsage(argv[0]);
            return 1;
    }
}
