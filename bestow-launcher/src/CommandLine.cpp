// bestow-launcher/src/CommandLine.cpp
// Command line argument parsing

#include <spdlog/spdlog.h>

import std;

namespace bestow::launcher {

enum class Command {
    None,
    Run,
    GenerateStubs,
    New,
    Init,
    Version,
    Help,
    Update
};

struct CommandLineArgs {
    Command command = Command::None;
    std::filesystem::path mainScript;
    std::filesystem::path outputDir;
    std::string projectName;
    bool verbose = false;
    bool debug = false;
    bool nightly = false;
    bool overwrite = false;
};

void printUsage(const char* programName) {
    spdlog::info("");
    spdlog::info("Bestow Game Engine - Lua-Driven Game Development");
    spdlog::info("");
    spdlog::info("Usage: {} <command> [options]", programName);
    spdlog::info("");
    spdlog::info("Commands:");
    spdlog::info("  run <main.lua>      Run a game from its main Lua script");
    spdlog::info("  generate-stubs <out> Generate IDE type stubs to output directory");
    spdlog::info("  new <name>          Create a new project in a new directory");
    spdlog::info("  init                Initialize current directory with template files");
    spdlog::info("    --overwrite              Overwrite existing files");
    spdlog::info("  update | upgrade    Update Bestow to the latest version");
    spdlog::info("    --nightly                Use latest nightly (pre-release) build");
    spdlog::info("  version             Show version information");
    spdlog::info("  help                Show this help message");
    spdlog::info("");
    spdlog::info("Options:");
    spdlog::info("  -v, --verbose              Enable verbose logging");
    spdlog::info("  -d, --debug                Enable debug mode (no hot reload, extra logging)");
    spdlog::info("");
    spdlog::info("Examples:");
    spdlog::info("  {} run games/my-game/main.lua", programName);
    spdlog::info("  {} generate-stubs sdk/stubs/", programName);
    spdlog::info("  {} new my-platformer", programName);
    spdlog::info("");
}

void printVersion() {
    spdlog::info("Bestow Game Engine v1.0.0");
    spdlog::info("Built with C++23, Lua 5.4");
    spdlog::info("https://github.com/your-org/bestow");
}

std::optional<CommandLineArgs> parseCommandLine(int argc, char* argv[]) {
    CommandLineArgs args;
    const char* programName = argv[0];

    if (argc < 2) {
        printUsage(programName);
        return std::nullopt;
    }

    int i = 1;
    while (i < argc) {
        std::string_view arg = argv[i];

        // Check for options first
        if (arg == "-v" || arg == "--verbose") {
            args.verbose = true;
            ++i;
            continue;
        }
        if (arg == "-d" || arg == "--debug") {
            args.debug = true;
            ++i;
            continue;
        }

        // Check for commands
        if (arg == "run") {
            args.command = Command::Run;
            if (i + 1 >= argc) {
                spdlog::error("run command requires a path to main.lua");
                return std::nullopt;
            }
            args.mainScript = argv[i + 1];
            i += 2;
            continue;
        }

        if (arg == "generate-stubs") {
            args.command = Command::GenerateStubs;
            if (i + 1 >= argc) {
                spdlog::error("generate-stubs command requires an output directory");
                return std::nullopt;
            }
            args.outputDir = argv[i + 1];
            i += 2;
            continue;
        }

        if (arg == "new") {
            args.command = Command::New;
            if (i + 1 >= argc) {
                spdlog::error("new command requires a project name");
                return std::nullopt;
            }
            args.projectName = argv[i + 1];
            i += 2;
            continue;
        }

        if (arg == "init") {
            args.command = Command::Init;
            ++i;
            // Check for --overwrite flag
            if (i < argc) {
                std::string_view nextArg = argv[i];
                if (nextArg == "--overwrite") {
                    args.overwrite = true;
                    ++i;
                }
            }
            continue;
        }

        if (arg == "version") {
            args.command = Command::Version;
            ++i;
            continue;
        }

        if (arg == "help" || arg == "-h" || arg == "--help") {
            args.command = Command::Help;
            ++i;
            continue;
        }

        if (arg == "update" || arg == "upgrade") {
            args.command = Command::Update;
            ++i;
            // Check for --nightly flag
            if (i < argc) {
                std::string_view nextArg = argv[i];
                if (nextArg == "--nightly") {
                    args.nightly = true;
                    ++i;
                }
            }
            continue;
        }

        // Unknown argument
        spdlog::error("Unknown command or option: {}", arg);
        printUsage(programName);
        return std::nullopt;
    }

    // Validate command
    if (args.command == Command::None) {
        spdlog::error("No command specified");
        printUsage(programName);
        return std::nullopt;
    }

    // Validate run command
    if (args.command == Command::Run) {
        if (!std::filesystem::exists(args.mainScript)) {
            spdlog::error("Script not found: {}", args.mainScript.string());
            return std::nullopt;
        }
        if (!args.mainScript.has_extension() || args.mainScript.extension() != ".lua") {
            spdlog::warn("Script does not have .lua extension: {}", args.mainScript.string());
        }
    }

    return args;
}

}  // namespace bestow::launcher
