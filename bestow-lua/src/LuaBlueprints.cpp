// bestow-lua/src/LuaBlueprints.cpp
// Blueprint loading and entity spawning implementation

module;

#include <sol/sol.hpp>
#include <spdlog/spdlog.h>

module bestow.lua.impl;

import std;
import bestow.services;

namespace bestow {

//==============================================================================
// Blueprint Loading
//==============================================================================

LuaResult<void> LuaRuntime::loadBlueprints(std::string_view path) {
    std::filesystem::path resolvedPath = PathResolver::resolve(std::string(path));

    // Check if it's a directory or a file
    if (std::filesystem::is_directory(resolvedPath)) {
        // Load all .lua files in the directory
        for (const auto& entry : std::filesystem::directory_iterator(resolvedPath)) {
            if (entry.path().extension() == ".lua") {
                auto result = loadBlueprints(entry.path().string());
                if (!result) {
                    spdlog::warn("[LuaRuntime] Failed to load blueprint file '{}': {}",
                                 entry.path().string(), result.error().message);
                }
            }
        }
        return {};
    }

    // Load single file
    std::ifstream file(resolvedPath);
    if (!file.is_open()) {
        return std::unexpected(LuaError{
            .code = LuaErrorCode::FileNotFound,
            .message = std::format("Could not open blueprint file: {}", resolvedPath.string()),
            .source = std::string(path)
        });
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string luaCode = buffer.str();

    auto result = lua_.safe_script(luaCode, sol::script_pass_on_error);
    if (!result.valid()) {
        sol::error err = result;
        return std::unexpected(LuaError{
            .code = LuaErrorCode::ParseError,
            .message = err.what(),
            .source = std::string(path)
        });
    }

    // The file should return a table of blueprints
    if (result.get_type() != sol::type::table) {
        return std::unexpected(LuaError{
            .code = LuaErrorCode::InvalidBlueprint,
            .message = "Blueprint file must return a table",
            .source = std::string(path)
        });
    }

    sol::table blueprintTable = result.get<sol::table>();

    // Parse each blueprint in the table
    for (const auto& [key, value] : blueprintTable) {
        if (!key.is<std::string>() || !value.is<sol::table>()) {
            continue;
        }

        std::string name = key.as<std::string>();
        sol::table bpTable = value.as<sol::table>();

        auto parseResult = parseBlueprintTable(bpTable, name);
        if (!parseResult) {
            spdlog::warn("[LuaRuntime] Failed to parse blueprint '{}': {}",
                         name, parseResult.error().message);
            continue;
        }

        blueprints_[name] = std::move(*parseResult);
        spdlog::debug("[LuaRuntime] Loaded blueprint: {}", name);
    }

    // Register asset for hot reload
    if (pIAssetSystem_ && hotReloadEnabled_) {
        AssetHandle handle = pIAssetSystem_->registerAsset(AssetType::Data, resolvedPath);
        blueprintAssets_.push_back(handle);
    }

    spdlog::info("[LuaRuntime] Loaded blueprints from: {}", path);
    return {};
}

LuaResult<void> LuaRuntime::loadAllBlueprints() {
    std::filesystem::path blueprintsDir = PathResolver::resolve(std::string(LuaPath::Blueprints));

    if (!std::filesystem::exists(blueprintsDir)) {
        spdlog::debug("[LuaRuntime] Blueprints directory does not exist: {}", blueprintsDir.string());
        return {};
    }

    return loadBlueprints(blueprintsDir.string());
}

bool LuaRuntime::hasBlueprint(std::string_view name) const {
    return blueprints_.contains(std::string(name));
}

std::optional<LuaBlueprintDef> LuaRuntime::getBlueprint(std::string_view name) const {
    auto it = blueprints_.find(std::string(name));
    if (it == blueprints_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<std::string> LuaRuntime::getBlueprintNames() const {
    std::vector<std::string> names;
    names.reserve(blueprints_.size());
    for (const auto& [name, _] : blueprints_) {
        names.push_back(name);
    }
    return names;
}

void LuaRuntime::clearBlueprints() {
    blueprints_.clear();
    blueprintAssets_.clear();
}

//==============================================================================
// Blueprint Parsing
//==============================================================================

LuaResult<LuaBlueprintDef> LuaRuntime::parseBlueprintTable(const sol::table& table,
                                                            std::string_view name) {
    LuaBlueprintDef blueprint;
    blueprint.name = std::string(name);

    // Parse inheritance
    sol::optional<std::string> inherits = table["inherits"];
    if (inherits) {
        blueprint.inherits = *inherits;
    }

    // Parse behavior
    sol::optional<std::string> behavior = table["behavior"];
    if (behavior) {
        blueprint.behavior = *behavior;
    }

    // Parse components
    sol::optional<sol::table> components = table["components"];
    if (components) {
        for (const auto& [key, value] : *components) {
            if (!value.is<sol::table>()) continue;

            LuaComponentDef compDef;

            if (key.is<std::string>()) {
                compDef.type = key.as<std::string>();
            } else if (key.is<int>()) {
                // Array-style: component type in "type" field
                sol::table compTable = value.as<sol::table>();
                sol::optional<std::string> type = compTable["type"];
                if (!type) {
                    spdlog::warn("[LuaRuntime] Component missing 'type' field in blueprint '{}'", name);
                    continue;
                }
                compDef.type = *type;
            }

            // Parse properties
            sol::table compTable = value.as<sol::table>();
            for (const auto& [propKey, propValue] : compTable) {
                if (!propKey.is<std::string>()) continue;
                std::string propName = propKey.as<std::string>();
                if (propName == "type") continue;  // Skip the type field

                compDef.properties[propName] = parsePropertyValue(propValue);
            }

            blueprint.components.push_back(std::move(compDef));
        }
    }

    // Parse physics
    sol::optional<sol::table> physics = table["physics"];
    if (physics) {
        LuaPhysicsDef physicsDef;

        sol::optional<std::string> bodyType = (*physics)["bodyType"];
        if (bodyType) physicsDef.bodyType = *bodyType;

        sol::optional<sol::table> size = (*physics)["size"];
        if (size) {
            float w = (*size)[1].get_or(1.0f);
            float h = (*size)[2].get_or(1.0f);
            physicsDef.size = std::array<float, 2>{w, h};
        }

        physicsDef.sensor = (*physics)["sensor"].get_or(false);
        physicsDef.fixedRotation = (*physics)["fixedRotation"].get_or(true);
        physicsDef.density = (*physics)["density"].get_or(1.0f);
        physicsDef.friction = (*physics)["friction"].get_or(0.3f);
        physicsDef.restitution = (*physics)["restitution"].get_or(0.0f);
        physicsDef.linearDamping = (*physics)["linearDamping"].get_or(0.0f);
        physicsDef.angularDamping = (*physics)["angularDamping"].get_or(0.0f);

        sol::optional<std::string> collisionLayer = (*physics)["collisionLayer"];
        if (collisionLayer) physicsDef.collisionLayer = *collisionLayer;

        blueprint.physics = physicsDef;
    }

    // Parse metadata
    sol::optional<sol::table> metadata = table["metadata"];
    if (metadata) {
        for (const auto& [key, value] : *metadata) {
            if (!key.is<std::string>()) continue;
            blueprint.metadata[key.as<std::string>()] = parsePropertyValue(value);
        }
    }

    return blueprint;
}

PropertyValue LuaRuntime::parsePropertyValue(const sol::object& obj) {
    if (obj.is<bool>()) {
        return obj.as<bool>();
    }
    if (obj.is<std::int64_t>()) {
        return obj.as<std::int64_t>();
    }
    if (obj.is<double>()) {
        return obj.as<double>();
    }
    if (obj.is<std::string>()) {
        return obj.as<std::string>();
    }
    if (obj.is<sol::table>()) {
        sol::table table = obj.as<sol::table>();

        // Check if it's a numeric array (vector)
        bool isNumericArray = true;
        bool isStringArray = true;
        bool isIntArray = true;

        for (const auto& [key, value] : table) {
            if (!key.is<int>()) {
                isNumericArray = false;
                isStringArray = false;
                isIntArray = false;
                break;
            }
            if (!value.is<double>() && !value.is<std::int64_t>()) {
                isNumericArray = false;
            }
            if (!value.is<std::int64_t>()) {
                isIntArray = false;
            }
            if (!value.is<std::string>()) {
                isStringArray = false;
            }
        }

        if (isIntArray) {
            std::vector<std::int64_t> values;
            for (const auto& [_, value] : table) {
                values.push_back(value.as<std::int64_t>());
            }
            return values;
        }

        if (isNumericArray) {
            std::vector<double> values;
            for (const auto& [_, value] : table) {
                values.push_back(value.as<double>());
            }
            return values;
        }

        if (isStringArray) {
            std::vector<std::string> values;
            for (const auto& [_, value] : table) {
                values.push_back(value.as<std::string>());
            }
            return values;
        }
    }

    return std::monostate{};
}

//==============================================================================
// Entity Creation
//==============================================================================

LuaResult<Entity> LuaRuntime::spawn(std::string_view blueprintName, float x, float y) {
    return spawn(blueprintName, x, y, PropertyMap{});
}

LuaResult<Entity> LuaRuntime::spawn(std::string_view blueprintName,
                                     float x, float y,
                                     float width, float height) {
    PropertyMap overrides;
    overrides["width"] = width;
    overrides["height"] = height;
    return spawn(blueprintName, x, y, overrides);
}

LuaResult<Entity> LuaRuntime::spawn(std::string_view blueprintName,
                                     float x, float y,
                                     const PropertyMap& overrides) {
    return spawn(blueprintName, x, y, 0.0f, 0.0f, overrides);
}

LuaResult<Entity> LuaRuntime::spawn(std::string_view blueprintName,
                                     float x, float y,
                                     float width, float height,
                                     const PropertyMap& overrides) {
    // Find the blueprint
    auto it = blueprints_.find(std::string(blueprintName));
    if (it == blueprints_.end()) {
        return std::unexpected(LuaError{
            .code = LuaErrorCode::InvalidBlueprint,
            .message = std::format("Blueprint '{}' not found", blueprintName),
            .source = "spawn"
        });
    }

    // Resolve inheritance
    LuaBlueprintDef resolvedBlueprint = resolveInheritance(it->second);

    // Create entity
    if (!pIEntitySystem_) {
        return std::unexpected(LuaError{
            .code = LuaErrorCode::SystemNotAvailable,
            .message = "EntitySystem not available",
            .source = "spawn"
        });
    }

    Entity entity = pIEntitySystem_->createEntity();

    // Merge overrides with size
    PropertyMap mergedOverrides = overrides;
    mergedOverrides["x"] = static_cast<double>(x);
    mergedOverrides["y"] = static_cast<double>(y);
    if (width > 0.0f) mergedOverrides["width"] = static_cast<double>(width);
    if (height > 0.0f) mergedOverrides["height"] = static_cast<double>(height);

    // Apply components
    applyComponentsToEntity(entity, resolvedBlueprint, mergedOverrides);

    // Attach default behavior if specified
    if (!resolvedBlueprint.behavior.empty()) {
        auto result = attachBehavior(entity, resolvedBlueprint.behavior);
        if (!result) {
            spdlog::warn("[LuaRuntime] Failed to attach behavior '{}' to entity: {}",
                         resolvedBlueprint.behavior, result.error().message);
        }
    }

    return entity;
}

LuaBlueprintDef LuaRuntime::resolveInheritance(const LuaBlueprintDef& blueprint) {
    if (blueprint.inherits.empty()) {
        return blueprint;
    }

    // Find parent blueprint
    auto parentIt = blueprints_.find(blueprint.inherits);
    if (parentIt == blueprints_.end()) {
        spdlog::warn("[LuaRuntime] Parent blueprint '{}' not found for '{}'",
                     blueprint.inherits, blueprint.name);
        return blueprint;
    }

    // Recursively resolve parent
    LuaBlueprintDef parent = resolveInheritance(parentIt->second);

    // Merge: child overrides parent
    LuaBlueprintDef resolved = parent;
    resolved.name = blueprint.name;
    resolved.inherits.clear();  // Fully resolved

    // Merge components (child components override parent with same type)
    std::unordered_map<std::string, std::size_t> componentIndex;
    for (std::size_t i = 0; i < resolved.components.size(); ++i) {
        componentIndex[resolved.components[i].type] = i;
    }

    for (const auto& childComp : blueprint.components) {
        auto it = componentIndex.find(childComp.type);
        if (it != componentIndex.end()) {
            // Merge properties
            auto& parentComp = resolved.components[it->second];
            for (const auto& [key, value] : childComp.properties) {
                parentComp.properties[key] = value;
            }
        } else {
            // Add new component
            resolved.components.push_back(childComp);
        }
    }

    // Override physics if child specifies it
    if (blueprint.physics) {
        resolved.physics = blueprint.physics;
    }

    // Override behavior if child specifies it
    if (!blueprint.behavior.empty()) {
        resolved.behavior = blueprint.behavior;
    }

    // Merge metadata
    for (const auto& [key, value] : blueprint.metadata) {
        resolved.metadata[key] = value;
    }

    return resolved;
}

void LuaRuntime::applyComponentsToEntity(Entity entity,
                                          const LuaBlueprintDef& blueprint,
                                          const PropertyMap& overrides) {
    for (const auto& compDef : blueprint.components) {
        auto creatorIt = componentCreators_.find(compDef.type);
        if (creatorIt == componentCreators_.end()) {
            spdlog::warn("[LuaRuntime] Unknown component type '{}' in blueprint '{}'",
                         compDef.type, blueprint.name);
            continue;
        }

        // Merge component properties with overrides
        PropertyMap mergedProps = compDef.properties;
        for (const auto& [key, value] : overrides) {
            mergedProps[key] = value;
        }

        // Create the component
        creatorIt->second(entity, *pIEntitySystem_, mergedProps);
    }
}

//==============================================================================
// Component Registration
//==============================================================================

void LuaRuntime::registerComponent(std::string_view name, ComponentCreator creator) {
    componentCreators_[std::string(name)] = std::move(creator);
    spdlog::debug("[LuaRuntime] Registered component: {}", name);
}

bool LuaRuntime::isComponentRegistered(std::string_view name) const {
    return componentCreators_.contains(std::string(name));
}

std::vector<std::string> LuaRuntime::getRegisteredComponents() const {
    std::vector<std::string> names;
    names.reserve(componentCreators_.size());
    for (const auto& [name, _] : componentCreators_) {
        names.push_back(name);
    }
    return names;
}

void LuaRuntime::registerBuiltinComponents() {
    // Register Transform component
    registerComponent("Transform", [](Entity entity, IEntitySystem& entities, const PropertyMap& props) {
        float x = 0.0f, y = 0.0f;
        float rotation = 0.0f;
        float scaleX = 1.0f, scaleY = 1.0f;

        if (auto it = props.find("x"); it != props.end()) {
            if (auto* v = std::get_if<double>(&it->second)) x = static_cast<float>(*v);
        }
        if (auto it = props.find("y"); it != props.end()) {
            if (auto* v = std::get_if<double>(&it->second)) y = static_cast<float>(*v);
        }
        if (auto it = props.find("rotation"); it != props.end()) {
            if (auto* v = std::get_if<double>(&it->second)) rotation = static_cast<float>(*v);
        }
        if (auto it = props.find("scaleX"); it != props.end()) {
            if (auto* v = std::get_if<double>(&it->second)) scaleX = static_cast<float>(*v);
        }
        if (auto it = props.find("scaleY"); it != props.end()) {
            if (auto* v = std::get_if<double>(&it->second)) scaleY = static_cast<float>(*v);
        }

        // Handle position array
        if (auto it = props.find("position"); it != props.end()) {
            if (auto* vec = std::get_if<std::vector<double>>(&it->second)) {
                if (vec->size() >= 2) {
                    x = static_cast<float>((*vec)[0]);
                    y = static_cast<float>((*vec)[1]);
                }
            }
        }

        // Handle scale array
        if (auto it = props.find("scale"); it != props.end()) {
            if (auto* vec = std::get_if<std::vector<double>>(&it->second)) {
                if (vec->size() >= 2) {
                    scaleX = static_cast<float>((*vec)[0]);
                    scaleY = static_cast<float>((*vec)[1]);
                } else if (vec->size() == 1) {
                    scaleX = scaleY = static_cast<float>((*vec)[0]);
                }
            }
        }

        entities.emplace<Transform2D>(entity, Transform2D{
            .x = x,
            .y = y,
            .rotation = rotation,
            .scaleX = scaleX,
            .scaleY = scaleY
        });
    });

    // Register Velocity component
    registerComponent("Velocity", [](Entity entity, IEntitySystem& entities, const PropertyMap& props) {
        float vx = 0.0f, vy = 0.0f;

        if (auto it = props.find("x"); it != props.end()) {
            if (auto* v = std::get_if<double>(&it->second)) vx = static_cast<float>(*v);
        }
        if (auto it = props.find("y"); it != props.end()) {
            if (auto* v = std::get_if<double>(&it->second)) vy = static_cast<float>(*v);
        }
        if (auto it = props.find("velocity"); it != props.end()) {
            if (auto* vec = std::get_if<std::vector<double>>(&it->second)) {
                if (vec->size() >= 2) {
                    vx = static_cast<float>((*vec)[0]);
                    vy = static_cast<float>((*vec)[1]);
                }
            }
        }

        entities.emplace<Velocity2D>(entity, Velocity2D{.x = vx, .y = vy});
    });

    // Register Tag component (string-based tagging)
    registerComponent("Tag", [](Entity entity, IEntitySystem& entities, const PropertyMap& props) {
        if (auto it = props.find("value"); it != props.end()) {
            if (auto* str = std::get_if<std::string>(&it->second)) {
                entities.emplace<Tag>(entity, Tag{.value = *str});
            }
        }
    });

    // Register Name component
    registerComponent("Name", [](Entity entity, IEntitySystem& entities, const PropertyMap& props) {
        if (auto it = props.find("value"); it != props.end()) {
            if (auto* str = std::get_if<std::string>(&it->second)) {
                entities.emplace<Name>(entity, Name{.value = *str});
            }
        }
    });

    spdlog::debug("[LuaRuntime] Registered {} built-in components", componentCreators_.size());
}

}  // namespace bestow
