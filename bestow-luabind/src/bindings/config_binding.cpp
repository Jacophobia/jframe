// bestow-luabind/src/bindings/config_binding.cpp
// Config system Lua bindings

module;

#include <bestow/sol2_compat.hpp>
#include <spdlog/spdlog.h>
#include <fstream>
#include <sstream>

module bestow.luabind;

import std;

namespace bestow {

void bindConfigSystem(sol::state& lua, IConfigSystem& config) {
    //=========================================================================
    // bestow.config table
    //=========================================================================

    sol::table bestow = lua["bestow"];
    sol::table configTable = lua.create_table();

    //-------------------------------------------------------------------------
    // Lifecycle
    //-------------------------------------------------------------------------

    configTable["initialize"] = [&config]() {
        return config.initialize();
    };

    configTable["shutdown"] = [&config]() {
        config.shutdown();
    };

    //-------------------------------------------------------------------------
    // Lua File Parsing (for data files like levels, configs, sounds)
    //-------------------------------------------------------------------------

    configTable["parseLuaFile"] = [&lua](const std::string& filePath) -> sol::object {
        // Read file contents
        std::ifstream file(filePath);
        if (!file.is_open()) {
            spdlog::error("[Config] Failed to open file: {}", filePath);
            return sol::nil;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string contents = buffer.str();

        // Execute the Lua code and return the result
        auto result = lua.safe_script(contents, sol::script_pass_on_error);
        if (!result.valid()) {
            sol::error err = result;
            spdlog::error("[Config] Failed to parse Lua file {}: {}", filePath, err.what());
            return sol::nil;
        }

        return result.get<sol::object>();
    };

    //-------------------------------------------------------------------------
    // Configuration Loading
    //-------------------------------------------------------------------------

    configTable["loadConfig"] = [&config](const std::string& filePath) {
        return config.loadConfig(filePath);
    };

    configTable["reloadAll"] = [&config]() {
        return config.reloadAll();
    };

    //-------------------------------------------------------------------------
    // Value Access
    //-------------------------------------------------------------------------

    configTable["getFloat"] = [&config, &lua](const std::string& key) -> sol::object {
        auto val = config.getFloat(key);
        if (val) return sol::make_object(lua, *val);
        return sol::nil;
    };

    configTable["getInt"] = [&config, &lua](const std::string& key) -> sol::object {
        auto val = config.getInt(key);
        if (val) return sol::make_object(lua, *val);
        return sol::nil;
    };

    configTable["getBool"] = [&config, &lua](const std::string& key) -> sol::object {
        auto val = config.getBool(key);
        if (val) return sol::make_object(lua, *val);
        return sol::nil;
    };

    configTable["getString"] = [&config, &lua](const std::string& key) -> sol::object {
        auto val = config.getString(key);
        if (val) return sol::make_object(lua, *val);
        return sol::nil;
    };

    configTable["getFloatOr"] = [&config](const std::string& key, float defaultValue) {
        return config.getFloatOr(key, defaultValue);
    };

    configTable["getIntOr"] = [&config](const std::string& key, int defaultValue) {
        return config.getIntOr(key, defaultValue);
    };

    configTable["getBoolOr"] = [&config](const std::string& key, bool defaultValue) {
        return config.getBoolOr(key, defaultValue);
    };

    configTable["getStringOr"] = [&config](const std::string& key, const std::string& defaultValue) {
        return config.getStringOr(key, defaultValue);
    };

    //-------------------------------------------------------------------------
    // State Queries
    //-------------------------------------------------------------------------

    configTable["hasKey"] = [&config](const std::string& key) {
        return config.hasKey(key);
    };

    //-------------------------------------------------------------------------
    // Hot Reload
    //-------------------------------------------------------------------------

    configTable["enableHotReload"] = [&config](bool enable) {
        config.enableHotReload(enable);
    };

    configTable["isHotReloadEnabled"] = [&config]() {
        return config.isHotReloadEnabled();
    };

    bestow["config"] = configTable;
}

}  // namespace bestow
