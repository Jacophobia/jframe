// jframe-level/src/LevelLoader.cpp
module;

#include <sol/sol.hpp>

module jframe.level.impl;

import jframe.assets;
import jframe.assets.impl;

namespace jframe {


bool LevelSystem::parseLevelLua(const std::string& luaCode, LoadedLevel& level) {
    try {
        // Execute the Lua code - use script_pass_on_error to avoid throwing exceptions
        sol::protected_function_result result = lua_.safe_script(luaCode, sol::script_pass_on_error);

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
            sol::object entitiesObj = levelTable["entities"];
            sol::table entitiesTable;

            // Support callable functions that generate entity tables
            if (entitiesObj.get_type() == sol::type::function) {
                sol::protected_function generator = entitiesObj;
                sol::protected_function_result genResult = generator();
                if (genResult.valid()) {
                    entitiesTable = genResult;
                } else {
                    return false;  // Generator function failed
                }
            } else if (entitiesObj.get_type() == sol::type::table) {
                entitiesTable = entitiesObj;
            } else {
                return false;  // Invalid entities type
            }

            // Iterate through all entities in the array
            for (size_t i = 1; i <= entitiesTable.size(); ++i) {
                sol::table entityTable = entitiesTable[i];

                EntityDef entityDef;

                // Get entity type (required)
                sol::optional<std::string> type = entityTable["type"];
                if (!type) {
                    continue; // Skip entities without type
                }
                entityDef.type = *type;

                // Get transform properties
                sol::optional<float> x = entityTable["x"];
                sol::optional<float> y = entityTable["y"];
                sol::optional<float> rotation = entityTable["rotation"];
                sol::optional<float> scaleX = entityTable["scaleX"];
                sol::optional<float> scaleY = entityTable["scaleY"];

                if (x) entityDef.transform.x = *x;
                if (y) entityDef.transform.y = *y;
                if (rotation) entityDef.transform.rotation = *rotation;
                if (scaleX) entityDef.transform.scaleX = *scaleX;
                if (scaleY) entityDef.transform.scaleY = *scaleY;

                // Parse all other properties as custom properties
                for (const auto& pair : entityTable) {
                    std::string key = pair.first.as<std::string>();

                    // Skip the properties we've already handled
                    if (key == "type" || key == "x" || key == "y" ||
                        key == "rotation" || key == "scaleX" || key == "scaleY") {
                        continue;
                    }

                    // Store the property value
                    sol::object value = pair.second;
                    sol::type valueType = value.get_type();

                    if (valueType == sol::type::boolean) {
                        entityDef.properties[key] = value.as<bool>();
                    } else if (valueType == sol::type::string) {
                        entityDef.properties[key] = value.as<std::string>();
                    } else if (valueType == sol::type::number) {
                        // Lua numbers are stored as doubles
                        entityDef.properties[key] = value.as<double>();
                    }
                    // Note: More complex types (tables, functions) are not supported
                }

                level.entityDefs.push_back(std::move(entityDef));
            }
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
