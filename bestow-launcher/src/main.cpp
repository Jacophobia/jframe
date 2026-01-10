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
    // Covers various installation layouts:
    // - Unix FHS: /usr/local/bin/bestow + /usr/local/share/bestow/template
    // - Windows: C:\Program Files\Bestow\bestow.exe + C:\Program Files\Bestow\share\bestow\template
    // - Development: ./build/bestow + ./template
    std::vector<std::filesystem::path> searchPaths = {
        exeDir / "template",                              // Dev: exe alongside template/
        exeDir / ".." / "template",                       // Dev: exe in build/, template in root
        exeDir / ".." / "share" / "bestow" / "template",  // Unix FHS: exe in bin/
        exeDir / "share" / "bestow" / "template",         // Windows: exe in install root
        exeDir / ".." / "lib" / "bestow" / "template",    // Alternative lib layout
        exeDir / "library" / "template",                  // Bestow library layout
        exeDir / ".." / "library" / "template",           // Bestow library layout (exe in bin/)
        exeDir / ".." / "Resources" / "template",         // macOS app bundle: Contents/Resources/template
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

//=============================================================================
// Update/Upgrade Implementation
//=============================================================================

std::optional<std::filesystem::path> getExecutablePath() {
#if defined(__APPLE__)
    char path[PATH_MAX];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        char resolvedPath[PATH_MAX];
        if (realpath(path, resolvedPath) != nullptr) {
            return std::filesystem::path(resolvedPath);
        }
        return std::filesystem::path(path);
    }
#elif defined(__linux__)
    char path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
    if (count != -1) {
        path[count] = '\0';
        return std::filesystem::path(path);
    }
#elif defined(_WIN32)
    char path[MAX_PATH];
    if (GetModuleFileNameA(nullptr, path, MAX_PATH) != 0) {
        return std::filesystem::path(path);
    }
#endif
    return std::nullopt;
}

int handleUpdate([[maybe_unused]] const CommandLineArgs& args) {
    spdlog::info("Bestow Engine - Self Update");
    spdlog::info("");

    // Get current executable path
    auto exePath = getExecutablePath();
    if (!exePath) {
        spdlog::error("Could not determine executable path");
        return 1;
    }

    spdlog::info("Current installation: {}", exePath->string());

    // Determine platform-specific archive name
    std::string platform;
    std::string extension;
#if defined(_WIN32)
    platform = "windows-x64";
    extension = ".zip";
#elif defined(__APPLE__)
    platform = "macos-x64";
    extension = ".tar.gz";
#elif defined(__linux__)
    platform = "linux-x64";
    extension = ".tar.gz";
#else
    spdlog::error("Unsupported platform for auto-update");
    return 1;
#endif

    std::string archiveName = "bestow-" + platform + extension;
    spdlog::info("Looking for latest release: {}", archiveName);
    spdlog::info("");

    // Use GitHub CLI or curl to fetch latest release info
    spdlog::info("Fetching latest release information...");

    std::string apiCmd = "curl -sL https://api.github.com/repos/radical-beard/bestow/releases/latest";
    FILE* pipe = popen(apiCmd.c_str(), "r");
    if (!pipe) {
        spdlog::error("Failed to query GitHub API");
        spdlog::info("Try manually downloading from: https://github.com/radical-beard/bestow/releases/latest");
        return 1;
    }

    std::string apiResponse;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        apiResponse += buffer;
    }
    int apiResult = pclose(pipe);

    if (apiResult != 0 || apiResponse.empty()) {
        spdlog::error("Failed to fetch release information");
        spdlog::info("Try manually downloading from: https://github.com/radical-beard/bestow/releases/latest");
        return 1;
    }

    // Parse JSON to find download URL (simple string search since we know the pattern)
    std::string searchPattern = "\"browser_download_url\": \"";
    size_t urlStart = apiResponse.find(searchPattern);
    std::string downloadUrl;

    while (urlStart != std::string::npos) {
        urlStart += searchPattern.length();
        size_t urlEnd = apiResponse.find("\"", urlStart);
        if (urlEnd != std::string::npos) {
            std::string url = apiResponse.substr(urlStart, urlEnd - urlStart);
            if (url.find(archiveName) != std::string::npos) {
                downloadUrl = url;
                break;
            }
        }
        urlStart = apiResponse.find(searchPattern, urlEnd);
    }

    if (downloadUrl.empty()) {
        spdlog::error("Could not find {} in latest release", archiveName);
        spdlog::info("Available releases: https://github.com/radical-beard/bestow/releases/latest");
        return 1;
    }

    spdlog::info("Found latest release");
    spdlog::info("Download URL: {}", downloadUrl);
    spdlog::info("");

    // Create temp directory for download
    auto tempDir = std::filesystem::temp_directory_path() / ("bestow-update-" + std::to_string(std::time(nullptr)));
    std::filesystem::create_directories(tempDir);
    auto tempArchive = tempDir / archiveName;

    spdlog::info("Downloading update...");
    std::string downloadCmd = "curl -L -o \"" + tempArchive.string() + "\" \"" + downloadUrl + "\"";

    int downloadResult = std::system(downloadCmd.c_str());
    if (downloadResult != 0) {
        spdlog::error("Failed to download update");
        std::filesystem::remove_all(tempDir);
        return 1;
    }

    if (!std::filesystem::exists(tempArchive)) {
        spdlog::error("Download succeeded but file not found");
        std::filesystem::remove_all(tempDir);
        return 1;
    }

    spdlog::info("Downloaded: {} ({} MB)",
        archiveName,
        std::filesystem::file_size(tempArchive) / (1024 * 1024));
    spdlog::info("");

    // Extract archive
    spdlog::info("Extracting update...");
    auto extractDir = tempDir / "extracted";
    std::filesystem::create_directories(extractDir);

    std::string extractCmd;
#if defined(_WIN32)
    // Use PowerShell with proper escaping for paths with spaces
    // Convert backslashes to forward slashes for PowerShell
    std::string psArchivePath = tempArchive.string();
    std::string psExtractPath = extractDir.string();
    std::replace(psArchivePath.begin(), psArchivePath.end(), '\\', '/');
    std::replace(psExtractPath.begin(), psExtractPath.end(), '\\', '/');
    extractCmd = "powershell -NoProfile -ExecutionPolicy Bypass -Command \"Expand-Archive -LiteralPath '" +
                 psArchivePath + "' -DestinationPath '" + psExtractPath + "' -Force\"";
#else
    if (extension == ".tar.gz") {
        extractCmd = "tar -xzf \"" + tempArchive.string() + "\" -C \"" + extractDir.string() + "\"";
    } else {
        extractCmd = "unzip -q \"" + tempArchive.string() + "\" -d \"" + extractDir.string() + "\"";
    }
#endif

    int extractResult = std::system(extractCmd.c_str());
    if (extractResult != 0) {
        spdlog::error("Failed to extract update");
        std::filesystem::remove_all(tempDir);
        return 1;
    }

    spdlog::info("Extraction complete");
    spdlog::info("");

    // Find the installation root (parent of bin/ directory)
    auto installRoot = exePath->parent_path().parent_path();

    spdlog::warn("IMPORTANT: Bestow will now replace the current installation");
    spdlog::warn("Installation directory: {}", installRoot.string());
    spdlog::info("");
    spdlog::info("Press Enter to continue or Ctrl+C to cancel...");
    std::cin.get();

    // Backup current installation
    auto backupDir = installRoot.parent_path() / ("bestow-backup-" + std::to_string(std::time(nullptr)));
    spdlog::info("Creating backup at: {}", backupDir.string());

    try {
        std::filesystem::create_directories(backupDir);
        for (const auto& entry : std::filesystem::directory_iterator(installRoot)) {
            auto dest = backupDir / entry.path().filename();
            if (std::filesystem::is_directory(entry)) {
                std::filesystem::copy(entry, dest, std::filesystem::copy_options::recursive);
            } else {
                std::filesystem::copy_file(entry, dest, std::filesystem::copy_options::overwrite_existing);
            }
        }
    } catch (const std::exception& e) {
        spdlog::error("Failed to create backup: {}", e.what());
        std::filesystem::remove_all(tempDir);
        return 1;
    }

    // Replace installation
    spdlog::info("Installing update...");

#if defined(_WIN32)
    // Windows-specific: Can't replace running .exe, so create a batch script to do it after exit
    auto updaterScript = tempDir / "bestow-updater.bat";
    std::ofstream scriptFile(updaterScript);
    if (!scriptFile) {
        spdlog::error("Failed to create updater script");
        std::filesystem::remove_all(tempDir);
        return 1;
    }

    // Create batch script that waits for this process to exit, then replaces files
    scriptFile << "@echo off\n";
    scriptFile << "echo Waiting for Bestow to exit...\n";
    scriptFile << "timeout /t 2 /nobreak >nul\n";
    scriptFile << "echo Installing update...\n";
    scriptFile << "\n";

    // Remove old files
    scriptFile << "cd /d \"" << installRoot.string() << "\"\n";
    scriptFile << "for /d %%d in (*) do rd /s /q \"%%d\"\n";
    scriptFile << "for %%f in (*) do del /q \"%%f\"\n";
    scriptFile << "\n";

    // Copy new files
    scriptFile << "xcopy /s /e /h /y \"" << extractDir.string() << "\\*\" \"" << installRoot.string() << "\\\"\n";
    scriptFile << "if errorlevel 1 (\n";
    scriptFile << "    echo Update failed! Restoring backup...\n";
    scriptFile << "    for /d %%d in (*) do rd /s /q \"%%d\"\n";
    scriptFile << "    for %%f in (*) do del /q \"%%f\"\n";
    scriptFile << "    xcopy /s /e /h /y \"" << backupDir.string() << "\\*\" \"" << installRoot.string() << "\\\"\n";
    scriptFile << "    echo Backup restored.\n";
    scriptFile << "    pause\n";
    scriptFile << "    exit /b 1\n";
    scriptFile << ")\n";
    scriptFile << "\n";

    // Cleanup
    scriptFile << "rd /s /q \"" << tempDir.string() << "\"\n";
    scriptFile << "echo.\n";
    scriptFile << "echo Update complete!\n";
    scriptFile << "echo Backup saved to: " << backupDir.string() << "\n";
    scriptFile << "echo.\n";
    scriptFile << "pause\n";
    scriptFile << "del \"%~f0\"\n";  // Delete the batch script itself
    scriptFile.close();

    spdlog::info("");
    spdlog::info("Update prepared. Launching updater...");
    spdlog::info("");
    spdlog::warn("IMPORTANT: Do not close the update window that appears.");
    spdlog::warn("The update will complete after Bestow exits.");

    // Launch the batch script in a new window and exit immediately
    // Note: Do NOT use /wait - we want Bestow to exit so the .exe can be replaced
    std::string launchCmd = "start \"Bestow Update\" \"" + updaterScript.string() + "\"";
    int launchResult = std::system(launchCmd.c_str());

    if (launchResult != 0) {
        spdlog::error("Failed to launch updater script");
        spdlog::error("You can manually run: {}", updaterScript.string());
        return 1;
    }

    spdlog::info("Updater started. Exiting Bestow...");
    spdlog::info("");
    spdlog::warn("NOTE: If running from Program Files, you may need administrator privileges.");

    // Give the script a moment to start before we exit
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    return 0;

#else
    // Unix: We can replace files while running (executable is loaded into memory)
    try {
        // Remove old files (except backup)
        for (const auto& entry : std::filesystem::directory_iterator(installRoot)) {
            std::filesystem::remove_all(entry);
        }

        // Copy new files from extracted directory
        for (const auto& entry : std::filesystem::recursive_directory_iterator(extractDir)) {
            if (entry.is_regular_file()) {
                auto relativePath = std::filesystem::relative(entry.path(), extractDir);
                auto destPath = installRoot / relativePath;
                std::filesystem::create_directories(destPath.parent_path());
                std::filesystem::copy_file(entry.path(), destPath,
                    std::filesystem::copy_options::overwrite_existing);

                // Preserve execute permissions on Unix
                if (relativePath.parent_path().filename() == "bin") {
                    std::filesystem::permissions(destPath,
                        std::filesystem::perms::owner_exec | std::filesystem::perms::group_exec | std::filesystem::perms::others_exec,
                        std::filesystem::perm_options::add);
                }
            }
        }
    } catch (const std::exception& e) {
        spdlog::error("Failed to install update: {}", e.what());
        spdlog::error("Attempting to restore from backup...");

        try {
            std::filesystem::remove_all(installRoot);
            for (const auto& entry : std::filesystem::directory_iterator(backupDir)) {
                auto dest = installRoot / entry.path().filename();
                if (std::filesystem::is_directory(entry)) {
                    std::filesystem::copy(entry, dest, std::filesystem::copy_options::recursive);
                } else {
                    std::filesystem::copy_file(entry, dest, std::filesystem::copy_options::overwrite_existing);
                }
            }
            spdlog::info("Restored from backup");
        } catch (const std::exception& restoreError) {
            spdlog::error("Failed to restore backup: {}", restoreError.what());
            spdlog::error("Manual recovery required from: {}", backupDir.string());
        }

        std::filesystem::remove_all(tempDir);
        return 1;
    }

    // Cleanup
    std::filesystem::remove_all(tempDir);

    spdlog::info("");
    spdlog::info("Update complete!");
    spdlog::info("Backup saved to: {}", backupDir.string());
    spdlog::info("");
    spdlog::info("Run 'bestow version' to verify the new version");

    return 0;
#endif
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

        case Command::Update:
            return handleUpdate(*args);

        case Command::None:
        default:
            spdlog::error("No command specified");
            printUsage(argv[0]);
            return 1;
    }
}
