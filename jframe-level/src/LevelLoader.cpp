// jframe-level/src/LevelLoader.cpp
module;

#include <sol/sol.hpp>

module jframe.level.impl;

import jframe.assets;
import jframe.assets.impl;

namespace jframe {


bool LevelSystem::parseLevelLua(const std::string& luaCode, LoadedLevel& level) {
    try {
        // Execute the Lua code
        sol::protected_function_result result = lua_.safe_script(luaCode);

        if (!result.valid()) {
            sol::error err = result;
            // Lua execution failed
            return false;
        }

        // Get the returned table
        sol::table levelTable = result;

        // Parse metadata
        if (levelTable["name"].valid()) {
            sol::optional<std::string> name = levelTable["name"];
            if (name) {
                level.metadata.levelName = *name;
            }
        }

        if (levelTable["width"].valid()) {
            sol::optional<float> width = levelTable["width"];
            if (width) {
                level.metadata.width = *width;
            }
        }

        if (levelTable["height"].valid()) {
            sol::optional<float> height = levelTable["height"];
            if (height) {
                level.metadata.height = *height;
            }
        }

        // Parse spawn points
        if (levelTable["spawnPoints"].valid()) {
            sol::table spawnTable = levelTable["spawnPoints"];

            for (const auto& pair : spawnTable) {
                std::string spawnName = pair.first.as<std::string>();
                sol::table spawnData = pair.second;

                Transform2D transform;
                sol::optional<float> x = spawnData["x"];
                sol::optional<float> y = spawnData["y"];
                sol::optional<float> rot = spawnData["rotation"];

                if (x) transform.x = *x;
                if (y) transform.y = *y;
                if (rot) transform.rotation = *rot;

                level.spawnPoints[spawnName] = transform;
            }
        }

        // Parse entity definitions (for later spawning)
        // Note: We don't spawn entities here, just store the definitions
        // The game code will spawn entities using these definitions
        if (levelTable["entities"].valid()) {
            sol::table entitiesTable = levelTable["entities"];
            // TODO: Store entity definitions for later spawning by game code
            // For now, we just validate that the table exists
        }

        return true;
    } catch (const sol::error& e) {
        // Lua error occurred
        return false;
    } catch (const std::exception& e) {
        // Other error occurred
        return false;
    }
}

}  // namespace jframe
