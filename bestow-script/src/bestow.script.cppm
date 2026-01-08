// bestow-script/src/bestow.script.cppm
// Script management system - Lua script loading with hot reload and app.* namespace
//
// This module provides the core infrastructure for the Lua-driven game engine:
// - ScriptManager: Manages the app.* global table, loads Lua files, handles hot reload
// - ScriptLinter: Pattern-based linter to catch common hot-reload-breaking patterns

module;

#include <bestow/sol2_compat.hpp>
#include <spdlog/spdlog.h>

export module bestow.script;

import std;
import bestow.services;  // Re-exports all contracts including bestow.assets, bestow.types

export namespace bestow {

//=============================================================================
// LintResult - Result of linting a Lua file
//=============================================================================

struct LintResult {
    bool passed = true;
    std::string message;
    int lineNumber = 0;
};

//=============================================================================
// ScriptLinter - Pattern-based linter for hot-reload-safe Lua code
//=============================================================================

/// Lints a Lua source file to detect patterns that break hot reload.
///
/// Detects:
/// - `local X = app.Y` at file scope (captures reference at load time)
/// - `local X = require(...)` would be caught by disabled require(), but lint too
///
/// @param source The Lua source code to lint
/// @param filepath The file path (for error messages)
/// @return LintResult with passed=true if no issues, or message/line if issues found
LintResult lintLuaFile(const std::string& source, const std::string& filepath);

//=============================================================================
// ScriptManager - Manages app.* namespace and Lua script loading
//=============================================================================

/// ScriptManager is the core of the Lua-driven game engine.
///
/// It manages:
/// - The `app` global Lua table where all user scripts are registered
/// - Auto-discovery of Lua files from a game directory
/// - Path-to-table conversion (e.g., entities/player.lua -> app.entities.player)
/// - Hot reload when files change
/// - Disabling require/dofile/loadfile with helpful error messages
/// - Running the linter before executing each file
///
/// Usage:
/// @code
/// sol::state lua;
/// ScriptManager scripts(lua);
/// scripts.initialize("/path/to/my-game");
/// scripts.enableHotReload(true);
/// scripts.loadAllScripts();
///
/// // In game loop:
/// scripts.update();  // Process hot reload queue
/// @endcode
class ScriptManager {
public:
    /// Construct a ScriptManager with a Lua state.
    /// @param lua Reference to the sol::state (must outlive ScriptManager)
    /// @param assets Optional pointer to IAssetSystem for file watching (hot reload)
    explicit ScriptManager(sol::state& lua, IAssetSystem* assets = nullptr);

    ~ScriptManager();

    // Non-copyable and non-movable (due to std::mutex member)
    ScriptManager(const ScriptManager&) = delete;
    ScriptManager& operator=(const ScriptManager&) = delete;
    ScriptManager(ScriptManager&&) = delete;
    ScriptManager& operator=(ScriptManager&&) = delete;

    //=========================================================================
    // Initialization
    //=========================================================================

    /// Initialize the script manager with a game root directory.
    /// Creates the `app` global table and sets up sandboxing.
    /// @param gameRoot The root directory of the game (where main.lua lives)
    /// @return true if initialization succeeded
    bool initialize(const std::filesystem::path& gameRoot);

    /// Enable or disable hot reload.
    /// When enabled, file changes are detected and scripts are reloaded.
    /// @param enable true to enable hot reload
    void enableHotReload(bool enable);

    /// Check if hot reload is enabled.
    bool isHotReloadEnabled() const { return hotReloadEnabled_; }

    //=========================================================================
    // Script Loading
    //=========================================================================

    /// Load a single Lua script file.
    /// The file is linted, executed, and its return value registered in app.*.
    /// @param path Absolute or relative path to the Lua file
    /// @return true if the script loaded successfully
    bool loadScript(const std::filesystem::path& path);

    /// Reload a previously loaded script.
    /// The file is re-executed and its return value replaces the old one in app.*.
    /// @param path Path to the Lua file (same as used in loadScript)
    /// @return true if reload succeeded
    bool reloadScript(const std::filesystem::path& path);

    /// Load all Lua scripts from the game directory.
    /// Recursively discovers .lua files and loads them in dependency order.
    void loadAllScripts();

    //=========================================================================
    // Configuration
    //=========================================================================

    /// Set folders to ignore during script discovery.
    /// Default: ["assets", "build", ".git", "node_modules"]
    /// @param folders List of folder names to ignore
    void setIgnoredFolders(std::vector<std::string> folders);

    /// Add a folder to the ignore list.
    void addIgnoredFolder(const std::string& folder);

    //=========================================================================
    // Runtime
    //=========================================================================

    /// Update the script manager.
    /// Processes the hot reload queue and executes pending reloads.
    void update();

    /// Get the game root directory.
    const std::filesystem::path& getGameRoot() const { return gameRoot_; }

    /// Get all loaded script paths.
    std::vector<std::filesystem::path> getLoadedScripts() const;

    /// Check if a script is loaded.
    bool isScriptLoaded(const std::filesystem::path& path) const;

    //=========================================================================
    // Direct Lua Access
    //=========================================================================

    /// Get the app table.
    sol::table getAppTable() const;

    /// Get a value from the app table by path.
    /// @param path Dot-separated path (e.g., "entities.player")
    /// @return The value at that path, or nil if not found
    sol::object getAppValue(const std::string& path) const;

private:
    // Setup helpers
    void setupSandbox();
    void disableRequire();
    void disableDofile();
    void disableLoadfile();

    // Path conversion
    std::string pathToTableKey(const std::filesystem::path& scriptPath) const;
    sol::table ensureTablePath(const std::string& dotPath);

    // File discovery
    std::vector<std::filesystem::path> discoverScripts() const;
    bool shouldIgnorePath(const std::filesystem::path& path) const;

    // Hot reload
    void onFileChanged(const std::filesystem::path& path);
    void processReloadQueue();
    void mergeTablesForHotReload(sol::table existing, sol::table newTable);

    // Script execution
    bool executeScript(const std::filesystem::path& path, const std::string& source);

    // Members
    sol::state* lua_ = nullptr;
    IAssetSystem* assets_ = nullptr;
    std::filesystem::path gameRoot_;
    bool initialized_ = false;
    bool hotReloadEnabled_ = false;

    // Loaded scripts: table path -> file path
    std::unordered_map<std::string, std::filesystem::path> loadedScripts_;

    // Reverse lookup: file path -> table path
    std::unordered_map<std::string, std::string> pathToKey_;

    // Ignored folders
    std::vector<std::string> ignoredFolders_ = {
        "assets", "build", ".git", "node_modules", "vendor", "external"
    };

    // Hot reload queue
    std::vector<std::filesystem::path> reloadQueue_;
    std::mutex reloadMutex_;

    // Asset subscription for hot reload
    SubscriptionId assetSubscriptionId_ = 0;

    // File watcher (efsw) - defined in implementation
    class FileWatchListener;
    std::unique_ptr<void, void(*)(void*)> fileWatcher_;
    std::unique_ptr<FileWatchListener> fileWatchListener_;
    std::unordered_set<std::string> watchedDirectories_;
};

}  // namespace bestow
