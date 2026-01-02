// bestow-launcher/src/main.cpp
// Single executable entry point for Bestow Engine

#include <spdlog/spdlog.h>

// Platform-specific includes for executable path detection
#if defined(__APPLE__)
    #include <mach-o/dyld.h>
    #include <climits>
#elif defined(__linux__)
    #include <unistd.h>
    #include <linux/limits.h>
#elif defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

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
    Init,
    Version,
    Help
};

struct CommandLineArgs {
    Command command = Command::None;
    std::filesystem::path mainScript;
    std::filesystem::path outputDir;
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
                      bool verbose, bool debug);
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
    if (!initializeRunner(runner.get(), args.mainScript, args.verbose, args.debug)) {
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

//=============================================================================
// Template Directory Discovery
//=============================================================================

std::optional<std::filesystem::path> findTemplateDirectory() {
    // 1. Check environment variable override
    if (const char* envPath = std::getenv("BESTOW_TEMPLATE_PATH")) {
        std::filesystem::path templatePath(envPath);
        if (std::filesystem::exists(templatePath) && std::filesystem::is_directory(templatePath)) {
            return templatePath;
        }
        spdlog::warn("BESTOW_TEMPLATE_PATH is set but path does not exist: {}", envPath);
    }

    // 2. Get executable directory and search relative to it
    std::filesystem::path exeDir;

#if defined(__APPLE__)
    char path[PATH_MAX];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        // _NSGetExecutablePath returns the invoked path (may be symlink)
        // Use realpath to resolve to actual executable location
        char resolvedPath[PATH_MAX];
        if (realpath(path, resolvedPath) != nullptr) {
            exeDir = std::filesystem::path(resolvedPath).parent_path();
        } else {
            exeDir = std::filesystem::path(path).parent_path();
        }
    }
#elif defined(__linux__)
    char path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
    if (count != -1) {
        path[count] = '\0';
        exeDir = std::filesystem::path(path).parent_path();
    }
#elif defined(_WIN32)
    char path[MAX_PATH];
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    exeDir = std::filesystem::path(path).parent_path();
#endif

    // Search patterns relative to executable
    std::vector<std::filesystem::path> searchPaths = {
        exeDir / "template",
        exeDir / ".." / "template",
        exeDir / ".." / "share" / "bestow" / "template",
        exeDir / ".." / "lib" / "bestow" / "template",
        exeDir / "library" / "template",
        exeDir / ".." / "library" / "template",
    };

    // Also check BESTOW_LIBRARY_PATH + template
    if (const char* libPath = std::getenv("BESTOW_LIBRARY_PATH")) {
        searchPaths.push_back(std::filesystem::path(libPath) / "template");
    }

    for (const auto& searchPath : searchPaths) {
        std::error_code ec;
        auto canonical = std::filesystem::canonical(searchPath, ec);
        if (!ec && std::filesystem::exists(canonical) && std::filesystem::is_directory(canonical)) {
            return canonical;
        }
    }

    // 3. Fallback: check relative to cwd
    std::filesystem::path cwdTemplate = std::filesystem::current_path() / "template";
    if (std::filesystem::exists(cwdTemplate) && std::filesystem::is_directory(cwdTemplate)) {
        return cwdTemplate;
    }

    return std::nullopt;
}

void copyDirectoryRecursive(const std::filesystem::path& src, const std::filesystem::path& dst,
                            int& copiedFiles, int& skippedFiles) {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(src)) {
        const auto& srcPath = entry.path();
        auto relativePath = std::filesystem::relative(srcPath, src);
        auto dstPath = dst / relativePath;

        if (entry.is_directory()) {
            std::filesystem::create_directories(dstPath);
        } else if (entry.is_regular_file()) {
            if (std::filesystem::exists(dstPath)) {
                spdlog::warn("Skipping existing file: {}", relativePath.string());
                ++skippedFiles;
            } else {
                std::filesystem::create_directories(dstPath.parent_path());
                std::filesystem::copy_file(srcPath, dstPath);
                spdlog::debug("Copied: {}", relativePath.string());
                ++copiedFiles;
            }
        }
    }
}

int handleInit([[maybe_unused]] const CommandLineArgs& args) {
    spdlog::info("Bestow Engine - Initializing project in current directory...");

    auto templateDir = findTemplateDirectory();
    if (!templateDir) {
        spdlog::error("Could not find template directory");
        spdlog::info("Set BESTOW_TEMPLATE_PATH environment variable to specify location");
        return 1;
    }

    spdlog::info("Using template from: {}", templateDir->string());

    std::filesystem::path targetDir = std::filesystem::current_path();

    int copiedFiles = 0;
    int skippedFiles = 0;

    try {
        copyDirectoryRecursive(*templateDir, targetDir, copiedFiles, skippedFiles);
    } catch (const std::filesystem::filesystem_error& e) {
        spdlog::error("Failed to copy template files: {}", e.what());
        return 1;
    }

    spdlog::info("");
    spdlog::info("Initialized project with {} files", copiedFiles);
    if (skippedFiles > 0) {
        spdlog::info("Skipped {} existing files", skippedFiles);
    }
    spdlog::info("");
    spdlog::info("Added:");
    spdlog::info("  CLAUDE.md           - AI agent guidance for game development");
    spdlog::info("  .claude/skills/     - Claude skills for Bestow game development");
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

        case Command::Init:
            return handleInit(*args);

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
