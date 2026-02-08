// bestow-luabind/src/docs/config_binding_doc.cpp
// API documentation for bestow.config

module bestow.luabind;

import std;

namespace bestow {

void registerConfigDoc(DocRegistry& registry) {
    SystemDoc sys;
    sys.name = "config";
    sys.qualifiedName = "bestow.config";
    sys.description = "Configuration system for loading, querying, and hot-reloading Lua-based configuration files.";

    // --- Lifecycle ---

    sys.methods.push_back(MethodDoc{
        .name = "initialize",
        .qualifiedName = "bestow.config.initialize",
        .description = "Initialize the config system. Typically called by the engine during startup.",
        .returns = {{.type = "boolean", .description = "true if initialization succeeded"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "shutdown",
        .qualifiedName = "bestow.config.shutdown",
        .description = "Shut down the config system and release resources.",
    });

    // --- Lua File Parsing ---

    sys.methods.push_back(MethodDoc{
        .name = "parseLuaFile",
        .qualifiedName = "bestow.config.parseLuaFile",
        .description = "Parse and execute a Lua file, returning its result. Useful for loading data files (levels, configs, sound definitions).",
        .params = {
            {.name = "filePath", .type = "string", .description = "Path to the Lua file"},
        },
        .returns = {{.type = "any|nil", .description = "Result of executing the Lua file, or nil on failure"}},
        .example = "local data = bestow.config.parseLuaFile(\"config/game.lua\")\nif data then\n    print(data.title)\nend",
    });

    sys.methods.push_back(MethodDoc{
        .name = "loadConfig",
        .qualifiedName = "bestow.config.loadConfig",
        .description = "Load a configuration file into the config system. Values become queryable via getFloat, getString, etc.",
        .params = {
            {.name = "filePath", .type = "string", .description = "Path to the config file"},
        },
        .returns = {{.type = "boolean", .description = "true if loaded successfully"}},
        .seeAlso = {"bestow.config.reloadAll"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "reloadAll",
        .qualifiedName = "bestow.config.reloadAll",
        .description = "Reload all loaded configuration files from disk.",
        .returns = {{.type = "boolean", .description = "true if all files reloaded successfully"}},
        .seeAlso = {"bestow.config.loadConfig"},
    });

    // --- Value Access ---

    sys.methods.push_back(MethodDoc{
        .name = "getFloat",
        .qualifiedName = "bestow.config.getFloat",
        .description = "Get a float config value by key.",
        .params = {
            {.name = "key", .type = "string", .description = "Dot-separated config key (e.g., 'physics.gravity')"},
        },
        .returns = {{.type = "number|nil", .description = "The float value, or nil if not found"}},
        .seeAlso = {"bestow.config.getFloatOr"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getInt",
        .qualifiedName = "bestow.config.getInt",
        .description = "Get an integer config value by key.",
        .params = {
            {.name = "key", .type = "string", .description = "Config key"},
        },
        .returns = {{.type = "number|nil", .description = "The integer value, or nil if not found"}},
        .seeAlso = {"bestow.config.getIntOr"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getBool",
        .qualifiedName = "bestow.config.getBool",
        .description = "Get a boolean config value by key.",
        .params = {
            {.name = "key", .type = "string", .description = "Config key"},
        },
        .returns = {{.type = "boolean|nil", .description = "The boolean value, or nil if not found"}},
        .seeAlso = {"bestow.config.getBoolOr"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getString",
        .qualifiedName = "bestow.config.getString",
        .description = "Get a string config value by key.",
        .params = {
            {.name = "key", .type = "string", .description = "Config key"},
        },
        .returns = {{.type = "string|nil", .description = "The string value, or nil if not found"}},
        .seeAlso = {"bestow.config.getStringOr"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getFloatOr",
        .qualifiedName = "bestow.config.getFloatOr",
        .description = "Get a float config value, returning a default if the key does not exist.",
        .params = {
            {.name = "key", .type = "string", .description = "Config key"},
            {.name = "defaultValue", .type = "number", .description = "Value to return if key is missing"},
        },
        .returns = {{.type = "number", .description = "The config value or the default"}},
        .example = "local gravity = bestow.config.getFloatOr(\"physics.gravity\", -980.0)",
    });

    sys.methods.push_back(MethodDoc{
        .name = "getIntOr",
        .qualifiedName = "bestow.config.getIntOr",
        .description = "Get an integer config value, returning a default if the key does not exist.",
        .params = {
            {.name = "key", .type = "string", .description = "Config key"},
            {.name = "defaultValue", .type = "number", .description = "Value to return if key is missing"},
        },
        .returns = {{.type = "number", .description = "The config value or the default"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getBoolOr",
        .qualifiedName = "bestow.config.getBoolOr",
        .description = "Get a boolean config value, returning a default if the key does not exist.",
        .params = {
            {.name = "key", .type = "string", .description = "Config key"},
            {.name = "defaultValue", .type = "boolean", .description = "Value to return if key is missing"},
        },
        .returns = {{.type = "boolean", .description = "The config value or the default"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getStringOr",
        .qualifiedName = "bestow.config.getStringOr",
        .description = "Get a string config value, returning a default if the key does not exist.",
        .params = {
            {.name = "key", .type = "string", .description = "Config key"},
            {.name = "defaultValue", .type = "string", .description = "Value to return if key is missing"},
        },
        .returns = {{.type = "string", .description = "The config value or the default"}},
    });

    // --- State Queries ---

    sys.methods.push_back(MethodDoc{
        .name = "hasKey",
        .qualifiedName = "bestow.config.hasKey",
        .description = "Check if a config key exists.",
        .params = {
            {.name = "key", .type = "string", .description = "Config key to check"},
        },
        .returns = {{.type = "boolean", .description = "true if the key exists in the loaded configuration"}},
    });

    // --- Hot Reload ---

    sys.methods.push_back(MethodDoc{
        .name = "enableHotReload",
        .qualifiedName = "bestow.config.enableHotReload",
        .description = "Enable or disable hot reload for configuration files. When enabled, config files are automatically reloaded when modified on disk.",
        .params = {
            {.name = "enable", .type = "boolean", .description = "true to enable, false to disable"},
        },
        .seeAlso = {"bestow.config.isHotReloadEnabled"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "isHotReloadEnabled",
        .qualifiedName = "bestow.config.isHotReloadEnabled",
        .description = "Check if hot reload is currently enabled.",
        .returns = {{.type = "boolean", .description = "true if hot reload is enabled"}},
        .seeAlso = {"bestow.config.enableHotReload"},
    });

    registry.addSystem(std::move(sys));
}

} // namespace bestow
