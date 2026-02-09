// bestow-script/src/ScriptManager.cpp
// Core script manager implementation

module;

#include <bestow/sol2_compat.hpp>
#include <efsw/efsw.hpp>
#include <spdlog/spdlog.h>

module bestow.script;

import std;

namespace bestow {

//=============================================================================
// FileWatchListener - Handles efsw file change callbacks
//=============================================================================

class ScriptManager::FileWatchListener : public efsw::FileWatchListener {
public:
    explicit FileWatchListener(ScriptManager* manager) : manager_(manager) {}

    void handleFileAction(
        efsw::WatchID watchId,
        const std::string& dir,
        const std::string& filename,
        efsw::Action action,
        std::string oldFilename) override
    {
        // Only handle modifications of .lua files
        if (action != efsw::Actions::Modified) {
            return;
        }

        // Check if this is a Lua file
        if (filename.size() < 4 || filename.substr(filename.size() - 4) != ".lua") {
            return;
        }

        // Build full path
        std::filesystem::path fullPath = std::filesystem::path(dir) / filename;

        spdlog::debug("[ScriptManager] File changed: {}", fullPath.string());

        // Queue the file for reload
        manager_->onFileChanged(fullPath);
    }

private:
    ScriptManager* manager_;
};

//=============================================================================
// Constructor / Destructor
//=============================================================================

// Custom deleter for efsw::FileWatcher (type-erased in header)
static void deleteFileWatcher(void* ptr) {
    delete static_cast<efsw::FileWatcher*>(ptr);
}

ScriptManager::ScriptManager(sol::state& lua, IAssetSystem* assets)
    : lua_(&lua), assets_(assets), fileWatcher_(nullptr, deleteFileWatcher) {}

ScriptManager::~ScriptManager() {
    // Stop file watching first
    if (fileWatcher_) {
        fileWatcher_.reset();
        fileWatchListener_.reset();
        watchedDirectories_.clear();
    }

    // Unsubscribe from asset changes if we have a subscription
    if (assets_ && assetSubscriptionId_ != 0) {
        assets_->unsubscribe(assetSubscriptionId_);
        assetSubscriptionId_ = 0;
    }
}

//=============================================================================
// Initialization
//=============================================================================

bool ScriptManager::initialize(const std::filesystem::path& gameRoot) {
    if (initialized_) {
        spdlog::warn("[ScriptManager] Already initialized, reinitializing with new root: {}",
                     gameRoot.string());
    }

    gameRoot_ = std::filesystem::absolute(gameRoot);

    if (!std::filesystem::exists(gameRoot_)) {
        spdlog::error("[ScriptManager] Game root directory does not exist: {}",
                      gameRoot_.string());
        return false;
    }

    if (!std::filesystem::is_directory(gameRoot_)) {
        // If a file was passed (e.g., main.lua), use its parent directory
        gameRoot_ = gameRoot_.parent_path();
    }

    spdlog::info("[ScriptManager] Initializing with game root: {}", gameRoot_.string());

    // Setup Lua sandbox and create app table
    setupSandbox();

    // Create the app global table
    (*lua_)["app"] = lua_->create_table();

    // Load optional app.config.lua if it exists
    auto configPath = gameRoot_ / "app.config.lua";
    if (std::filesystem::exists(configPath)) {
        std::ifstream file(configPath);
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string source = buffer.str();

        auto result = lua_->safe_script(source, sol::script_pass_on_error);
        if (result.valid()) {
            sol::table config = result;
            if (config["ignore"].valid()) {
                sol::table ignoreTable = config["ignore"];
                for (auto& pair : ignoreTable) {
                    if (pair.second.is<std::string>()) {
                        ignoredFolders_.push_back(pair.second.as<std::string>());
                    }
                }
                spdlog::info("[ScriptManager] Loaded {} ignore folders from app.config.lua",
                             ignoreTable.size());
            }
        } else {
            sol::error err = result;
            spdlog::warn("[ScriptManager] Failed to load app.config.lua: {}", err.what());
        }
    }

    initialized_ = true;
    return true;
}

void ScriptManager::enableHotReload(bool enable) {
    if (enable && !hotReloadEnabled_) {
        // Start file watching
        if (!fileWatcher_) {
            auto* watcher = new efsw::FileWatcher();
            fileWatcher_.reset(watcher);
            fileWatchListener_ = std::make_unique<FileWatchListener>(this);
        }

        // Watch the game root directory recursively
        if (initialized_ && !gameRoot_.empty()) {
            std::string dirStr = gameRoot_.string();
            if (watchedDirectories_.find(dirStr) == watchedDirectories_.end()) {
                auto* watcher = static_cast<efsw::FileWatcher*>(fileWatcher_.get());
                watcher->addWatch(dirStr, fileWatchListener_.get(), true);  // recursive=true
                watchedDirectories_.insert(dirStr);
                spdlog::info("[ScriptManager] Watching directory: {}", dirStr);
            }
        }

        // Start the file watcher background thread
        auto* watcher = static_cast<efsw::FileWatcher*>(fileWatcher_.get());
        watcher->watch();

        spdlog::info("[ScriptManager] Hot reload enabled with efsw file watcher");

    } else if (!enable && hotReloadEnabled_) {
        // Stop file watching
        fileWatcher_.reset();
        fileWatchListener_.reset();
        watchedDirectories_.clear();
        spdlog::info("[ScriptManager] Hot reload disabled");
    }

    hotReloadEnabled_ = enable;
}

//=============================================================================
// Sandbox Setup
//=============================================================================

void ScriptManager::setupSandbox() {
    // Remove dangerous functions
    (*lua_)["os"] = sol::lua_nil;
    (*lua_)["io"] = sol::lua_nil;
    (*lua_)["debug"] = sol::lua_nil;
    (*lua_)["package"] = sol::lua_nil;
    (*lua_)["rawget"] = sol::lua_nil;
    (*lua_)["rawset"] = sol::lua_nil;

    // Disable require/dofile/loadfile with helpful error messages
    disableRequire();
    disableDofile();
    disableLoadfile();
}

void ScriptManager::disableRequire() {
    (*lua_)["require"] = [](const std::string& moduleName) -> sol::object {
        std::string msg = std::format(
            R"(require() is disabled in Bestow.

Access other scripts through 'app' INSIDE YOUR FUNCTIONS:

  -- WRONG: cached at load time, breaks hot reload
  local physics = require("systems/physics")
  local physics = app.systems.physics

  -- RIGHT: resolved fresh each call
  return {{
    update = function(self, dt)
      local physics = app.systems.physics
      physics.step(dt)
    end
  }}

Requested module: {})",
            moduleName);

        throw std::runtime_error(msg);
    };
}

void ScriptManager::disableDofile() {
    (*lua_)["dofile"] = [](const std::string& filename) -> sol::object {
        std::string msg = std::format(
            R"(dofile() is disabled in Bestow.

Use the app.* table to access other scripts instead.
All Lua files in your game directory are automatically loaded
and available through the app table.

Requested file: {})",
            filename);

        throw std::runtime_error(msg);
    };
}

void ScriptManager::disableLoadfile() {
    (*lua_)["loadfile"] = [](const std::string& filename) -> sol::object {
        std::string msg = std::format(
            R"(loadfile() is disabled in Bestow.

Use the app.* table to access other scripts instead.
All Lua files in your game directory are automatically loaded
and available through the app table.

Requested file: {})",
            filename);

        throw std::runtime_error(msg);
    };

    (*lua_)["load"] = [](const std::string& chunk) -> sol::object {
        throw std::runtime_error(
            R"(load() is disabled in Bestow for security reasons.

Dynamic code loading is not supported. Define your code in .lua files
and access them through the app.* table.)");
    };

    (*lua_)["loadstring"] = [](const std::string& chunk) -> sol::object {
        throw std::runtime_error(
            R"(loadstring() is disabled in Bestow for security reasons.

Dynamic code loading is not supported. Define your code in .lua files
and access them through the app.* table.)");
    };
}

//=============================================================================
// Script Loading
//=============================================================================

bool ScriptManager::loadScript(const std::filesystem::path& path) {
    if (!initialized_) {
        spdlog::error("[ScriptManager] Cannot load script: not initialized");
        return false;
    }

    auto absPath = std::filesystem::absolute(path);

    // Read the file
    if (!std::filesystem::exists(absPath)) {
        spdlog::error("[ScriptManager] Script file not found: {}", absPath.string());
        return false;
    }

    std::ifstream file(absPath);
    if (!file.is_open()) {
        spdlog::error("[ScriptManager] Failed to open script: {}", absPath.string());
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    // Lint the file
    LintResult lint = lintLuaFile(source, absPath.string());
    if (!lint.passed) {
        spdlog::error("[ScriptManager] Lint failed:\n{}", lint.message);
        return false;
    }

    // Execute the script
    return executeScript(absPath, source);
}

bool ScriptManager::reloadScript(const std::filesystem::path& path) {
    auto absPath = std::filesystem::absolute(path);
    std::string pathStr = absPath.string();

    // Check if this script was previously loaded
    auto it = pathToKey_.find(pathStr);
    if (it == pathToKey_.end()) {
        spdlog::warn("[ScriptManager] Script not previously loaded, loading fresh: {}",
                     absPath.string());
        return loadScript(path);
    }

    spdlog::info("[ScriptManager] Reloading script: {}", absPath.string());
    return loadScript(path);
}

void ScriptManager::setMainScript(const std::filesystem::path& path) {
    mainScriptPath_ = std::filesystem::absolute(path);
    spdlog::debug("[ScriptManager] Main script set to: {}", mainScriptPath_.string());
}

void ScriptManager::loadAllScripts() {
    if (!initialized_) {
        spdlog::error("[ScriptManager] Cannot load scripts: not initialized");
        return;
    }

    auto scripts = discoverScripts();
    spdlog::info("[ScriptManager] Discovered {} Lua scripts", scripts.size());

    // Sort scripts so that the entry point is loaded last (it may depend on others)
    std::vector<std::filesystem::path> regularScripts;
    std::filesystem::path mainScript;

    for (const auto& script : scripts) {
        bool isMain = !mainScriptPath_.empty()
            ? (std::filesystem::absolute(script) == mainScriptPath_)
            : (script.filename() == "main.lua");
        if (isMain) {
            mainScript = script;
        } else {
            regularScripts.push_back(script);
        }
    }

    // Sort alphabetically for deterministic load order
    std::sort(regularScripts.begin(), regularScripts.end());

    // Load regular scripts first
    for (const auto& script : regularScripts) {
        if (!loadScript(script)) {
            spdlog::warn("[ScriptManager] Failed to load: {}", script.string());
        }
    }

    // Load main.lua last
    if (!mainScript.empty()) {
        if (!loadScript(mainScript)) {
            spdlog::error("[ScriptManager] Failed to load main.lua!");
        }
    }

    spdlog::info("[ScriptManager] Loaded {} scripts successfully", loadedScripts_.size());
}

//=============================================================================
// Configuration
//=============================================================================

void ScriptManager::setIgnoredFolders(std::vector<std::string> folders) {
    ignoredFolders_ = std::move(folders);
}

void ScriptManager::addIgnoredFolder(const std::string& folder) {
    ignoredFolders_.push_back(folder);
}

//=============================================================================
// Runtime
//=============================================================================

void ScriptManager::update() {
    if (!hotReloadEnabled_) {
        return;
    }

    processReloadQueue();
}

std::vector<std::filesystem::path> ScriptManager::getLoadedScripts() const {
    std::vector<std::filesystem::path> result;
    result.reserve(loadedScripts_.size());
    for (const auto& [key, path] : loadedScripts_) {
        result.push_back(path);
    }
    return result;
}

bool ScriptManager::isScriptLoaded(const std::filesystem::path& path) const {
    auto absPath = std::filesystem::absolute(path);
    return pathToKey_.contains(absPath.string());
}

//=============================================================================
// Direct Lua Access
//=============================================================================

sol::table ScriptManager::getAppTable() const {
    return (*lua_)["app"];
}

sol::object ScriptManager::getAppValue(const std::string& path) const {
    sol::table app = (*lua_)["app"];

    std::istringstream stream(path);
    std::string segment;
    sol::object current = app;

    while (std::getline(stream, segment, '.')) {
        if (!current.is<sol::table>()) {
            return sol::lua_nil;
        }
        current = current.as<sol::table>()[segment];
    }

    return current;
}

//=============================================================================
// Private Helpers
//=============================================================================

std::string ScriptManager::pathToTableKey(const std::filesystem::path& scriptPath) const {
    // If this is the designated entry point, always map to "main"
    if (!mainScriptPath_.empty() &&
        std::filesystem::absolute(scriptPath) == mainScriptPath_) {
        return "main";
    }

    // Get relative path from game root
    auto relPath = std::filesystem::relative(scriptPath, gameRoot_);

    // Convert path separators to dots and remove .lua extension
    std::string key;
    for (const auto& part : relPath) {
        if (!key.empty()) {
            key += ".";
        }
        std::string partStr = part.string();
        // Remove .lua extension from last part
        if (part == relPath.filename() && partStr.size() > 4 &&
            partStr.substr(partStr.size() - 4) == ".lua") {
            partStr = partStr.substr(0, partStr.size() - 4);
        }
        key += partStr;
    }

    return key;
}

sol::table ScriptManager::ensureTablePath(const std::string& dotPath) {
    sol::table app = (*lua_)["app"];

    std::istringstream stream(dotPath);
    std::string segment;
    sol::table current = app;
    std::string lastSegment;

    // Navigate to parent, creating tables as needed
    std::vector<std::string> segments;
    while (std::getline(stream, segment, '.')) {
        segments.push_back(segment);
    }

    // Create all parent tables
    for (size_t i = 0; i + 1 < segments.size(); ++i) {
        const auto& seg = segments[i];
        if (!current[seg].valid() || current[seg].get_type() == sol::type::nil) {
            current[seg] = lua_->create_table();
        }
        current = current[seg];
    }

    return current;
}

std::vector<std::filesystem::path> ScriptManager::discoverScripts() const {
    std::vector<std::filesystem::path> scripts;

    for (auto it = std::filesystem::recursive_directory_iterator(gameRoot_);
         it != std::filesystem::recursive_directory_iterator(); ++it) {

        // Check if we should skip this directory
        if (it->is_directory()) {
            if (shouldIgnorePath(it->path())) {
                it.disable_recursion_pending();
                continue;
            }
        }

        // Collect .lua files
        if (it->is_regular_file() && it->path().extension() == ".lua") {
            // Skip app.config.lua (already loaded during init)
            if (it->path().filename() != "app.config.lua") {
                scripts.push_back(it->path());
            }
        }
    }

    return scripts;
}

bool ScriptManager::shouldIgnorePath(const std::filesystem::path& path) const {
    std::string name = path.filename().string();

    // Check against ignored folder list
    for (const auto& ignored : ignoredFolders_) {
        if (name == ignored) {
            return true;
        }
    }

    // Also ignore hidden folders (starting with .)
    if (!name.empty() && name[0] == '.') {
        return true;
    }

    return false;
}

void ScriptManager::onFileChanged(const std::filesystem::path& path) {
    std::lock_guard<std::mutex> lock(reloadMutex_);
    reloadQueue_.push_back(path);
}

void ScriptManager::processReloadQueue() {
    std::vector<std::filesystem::path> toReload;

    {
        std::lock_guard<std::mutex> lock(reloadMutex_);
        toReload = std::move(reloadQueue_);
        reloadQueue_.clear();
    }

    for (const auto& path : toReload) {
        reloadScript(path);
    }
}

void ScriptManager::mergeTablesForHotReload(sol::table existing, sol::table newTable) {
    // Recursively merge tables: replace functions, preserve data
    for (auto& pair : newTable) {
        sol::object key = pair.first;
        sol::object newValue = pair.second;

        if (newValue.get_type() == sol::type::function) {
            // Always replace functions with new implementation
            existing[key] = newValue;
        } else if (newValue.get_type() == sol::type::table) {
            // Recursively merge nested tables
            sol::object existingNested = existing[key];
            if (existingNested.valid() && existingNested.get_type() == sol::type::table) {
                mergeTablesForHotReload(existingNested.as<sol::table>(), newValue.as<sol::table>());
            } else {
                // No existing nested table, set it
                existing[key] = newValue;
            }
        } else {
            // Data field: only set if doesn't exist (new field in updated code)
            sol::object existingValue = existing[key];
            if (!existingValue.valid() || existingValue.get_type() == sol::type::nil) {
                existing[key] = newValue;
            }
            // If exists, keep the existing value (preserve state)
        }
    }
}

bool ScriptManager::executeScript(const std::filesystem::path& path, const std::string& source) {
    std::string tableKey = pathToTableKey(path);
    std::string pathStr = path.string();

    spdlog::debug("[ScriptManager] Executing {} -> app.{}", pathStr, tableKey);

    // Execute the Lua code
    sol::protected_function_result result = lua_->safe_script(source, sol::script_pass_on_error);

    if (!result.valid()) {
        sol::error err = result;
        spdlog::error("[ScriptManager] Lua error in {}:\n{}", pathStr, err.what());
        return false;
    }

    // Get the returned value (should be a table)
    sol::object returnValue = result;

    // Parse the table key to find parent and final key
    std::vector<std::string> segments;
    std::istringstream stream(tableKey);
    std::string segment;
    while (std::getline(stream, segment, '.')) {
        segments.push_back(segment);
    }

    if (segments.empty()) {
        spdlog::error("[ScriptManager] Invalid table key for {}", pathStr);
        return false;
    }

    // Ensure parent tables exist and get the parent
    sol::table parent = ensureTablePath(tableKey);
    const std::string& finalKey = segments.back();

    // Check if this is a reload (existing table at this path)
    sol::object existingObj = parent[finalKey];
    bool isReload = existingObj.valid() && existingObj.get_type() == sol::type::table;

    if (isReload && returnValue.is<sol::table>()) {
        // HOT RELOAD: Preserve table identity and state
        // Keep existing table reference, only replace functions
        sol::table existingTable = existingObj.as<sol::table>();
        sol::table newTable = returnValue.as<sol::table>();

        int functionsReplaced = 0;
        int dataPreserved = 0;

        // Iterate through new table and selectively update existing table
        for (auto& pair : newTable) {
            sol::object key = pair.first;
            sol::object newValue = pair.second;

            if (newValue.get_type() == sol::type::function) {
                // REPLACE: Function values get updated to new implementation
                existingTable[key] = newValue;
                functionsReplaced++;
            } else if (key.is<std::string>() && key.as<std::string>() == "transient") {
                // SPECIAL: 'transient' table - preserve existing, ensure it exists
                if (!existingTable["transient"].valid() ||
                    existingTable["transient"].get_type() != sol::type::table) {
                    existingTable["transient"] = lua_->create_table();
                }
                // Keep existing transient data, don't replace with new defaults
                dataPreserved++;
            } else if (newValue.get_type() == sol::type::table) {
                // NESTED TABLE: Recursively merge (could be methods table, etc.)
                sol::object existingNested = existingTable[key];
                if (existingNested.valid() && existingNested.get_type() == sol::type::table) {
                    // Merge: update functions, keep data
                    mergeTablesForHotReload(existingNested.as<sol::table>(),
                                            newValue.as<sol::table>());
                } else {
                    // No existing nested table, just set it
                    existingTable[key] = newValue;
                }
            } else {
                // DATA: Check if field already exists in existing table
                sol::object existingValue = existingTable[key];
                if (!existingValue.valid() || existingValue.get_type() == sol::type::nil) {
                    // New field added in code update - initialize with new default
                    existingTable[key] = newValue;
                    spdlog::debug("[ScriptManager] Hot reload: new field '{}' initialized",
                                  key.is<std::string>() ? key.as<std::string>() : "?");
                } else {
                    // Existing field - preserve current value (don't reset state)
                    dataPreserved++;
                }
            }
        }

        spdlog::info("[ScriptManager] Hot reloaded: app.{} ({} functions updated, {} data fields preserved)",
                     tableKey, functionsReplaced, dataPreserved);
    } else {
        // FIRST LOAD: Set the value directly
        parent[finalKey] = returnValue;

        // Pre-create transient table if this is a table
        if (returnValue.is<sol::table>()) {
            sol::table newTable = returnValue.as<sol::table>();
            if (!newTable["transient"].valid() ||
                newTable["transient"].get_type() != sol::type::table) {
                newTable["transient"] = lua_->create_table();
            }
        }

        spdlog::info("[ScriptManager] Loaded: app.{}", tableKey);
    }

    // Track the loaded script
    loadedScripts_[tableKey] = path;
    pathToKey_[pathStr] = tableKey;

    return true;
}

}  // namespace bestow
