// jframe-blueprints/src/BlueprintFactory.cpp
// Blueprint Factory implementation

module;

#include <sol/sol.hpp>
#include <entt/entity/entity.hpp>

module jframe.blueprints.impl;

#include <algorithm>
#include <any>
#include <optional>
#include <string>
#include <utility>
#include <vector>

import jframe.types;
import jframe.blueprints;
import jframe.entity;
import jframe.physics;

namespace jframe {

//==============================================================================
// Constructor
//==============================================================================

BlueprintFactory::BlueprintFactory(IEntitySystem& entities, IPhysicsSystem* physics)
    : entities_(entities)
    , physics_(physics)
{
    registerBuiltinComponents();
}

//==============================================================================
// Blueprint Loading
//==============================================================================

bool BlueprintFactory::loadBlueprints(const std::string& luaSource) {
    lastLuaSource_ = luaSource;

    sol::state lua;
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table, sol::lib::string);

    // Sandbox - remove dangerous functions
    lua["os"] = sol::lua_nil;
    lua["io"] = sol::lua_nil;
    lua["loadfile"] = sol::lua_nil;
    lua["dofile"] = sol::lua_nil;
    lua["load"] = sol::lua_nil;
    lua["loadstring"] = sol::lua_nil;
    lua["require"] = sol::lua_nil;
    lua["package"] = sol::lua_nil;

    // Execute Lua code
    sol::protected_function_result result = lua.safe_script(luaSource, sol::script_pass_on_error);
    if (!result.valid()) {
        sol::error err = result;
        // Could log error here
        return false;
    }

    // Look for Blueprints table
    sol::optional<sol::table> blueprintsTable = lua["Blueprints"];
    if (!blueprintsTable) {
        // Try return value from script
        if (result.get_type() == sol::type::table) {
            blueprintsTable = result.get<sol::table>();
        }
    }

    if (!blueprintsTable) {
        return false;
    }

    // Parse each blueprint
    blueprints_.clear();
    for (const auto& [key, value] : *blueprintsTable) {
        if (key.get_type() != sol::type::string || value.get_type() != sol::type::table) {
            continue;
        }

        std::string name = key.as<std::string>();
        sol::table blueprintTable = value.as<sol::table>();

        BlueprintDef def;
        def.name = name;
        parseBlueprintTable(blueprintTable, def);
        blueprints_[name] = std::move(def);
    }

    return true;
}

void BlueprintFactory::reloadBlueprints() {
    if (!lastLuaSource_.empty()) {
        loadBlueprints(lastLuaSource_);
    }
}

void BlueprintFactory::clearBlueprints() {
    blueprints_.clear();
    lastLuaSource_.clear();
}

//==============================================================================
// Blueprint Queries
//==============================================================================

bool BlueprintFactory::hasBlueprint(const std::string& name) const {
    return blueprints_.contains(name);
}

std::vector<std::string> BlueprintFactory::getBlueprintNames() const {
    std::vector<std::string> names;
    names.reserve(blueprints_.size());
    for (const auto& [name, _] : blueprints_) {
        names.push_back(name);
    }
    return names;
}

std::optional<BlueprintDef> BlueprintFactory::getBlueprint(const std::string& name) const {
    auto it = blueprints_.find(name);
    if (it == blueprints_.end()) {
        return std::nullopt;
    }
    return resolveInheritance(it->second);
}

//==============================================================================
// Entity Creation
//==============================================================================

Entity BlueprintFactory::create(const std::string& blueprintName, float x, float y) {
    return create(blueprintName, x, y, 0.0f, 0.0f, {});
}

Entity BlueprintFactory::create(const std::string& blueprintName,
                                 float x, float y,
                                 float width, float height) {
    return create(blueprintName, x, y, width, height, {});
}

Entity BlueprintFactory::create(const std::string& blueprintName,
                                 float x, float y,
                                 const PropertyMap& overrides) {
    return create(blueprintName, x, y, 0.0f, 0.0f, overrides);
}

Entity BlueprintFactory::create(const std::string& blueprintName,
                                 float x, float y,
                                 float width, float height,
                                 const PropertyMap& overrides) {
    auto it = blueprints_.find(blueprintName);
    if (it == blueprints_.end()) {
        // Blueprint not found - return null entity
        return entt::null;
    }

    BlueprintDef resolved = resolveInheritance(it->second);
    return createEntityFromBlueprint(resolved, x, y, width, height, overrides);
}

//==============================================================================
// Component Registration
//==============================================================================

void BlueprintFactory::registerComponent(const std::string& name, ComponentCreator creator) {
    componentCreators_[name] = std::move(creator);
}

bool BlueprintFactory::isComponentRegistered(const std::string& name) const {
    return componentCreators_.contains(name);
}

//==============================================================================
// Parsing Helpers
//==============================================================================

void BlueprintFactory::parseBlueprintTable(const sol::table& blueprintTable, BlueprintDef& def) {
    // Parse inheritance
    sol::optional<std::string> inherits = blueprintTable["inherits"];
    if (inherits) {
        def.inherits = *inherits;
    }

    // Parse components
    sol::optional<sol::table> components = blueprintTable["components"];
    if (components) {
        parseComponentsTable(*components, def);
    }

    // Parse physics
    sol::optional<sol::table> physics = blueprintTable["physics"];
    if (physics) {
        def.physics = BlueprintPhysicsDef{};
        parsePhysicsTable(*physics, def);
    }

    // Parse metadata
    sol::optional<sol::table> metadata = blueprintTable["metadata"];
    if (metadata) {
        def.metadata = parsePropertyTable(*metadata);
    }
}

void BlueprintFactory::parseComponentsTable(const sol::table& componentsTable, BlueprintDef& def) {
    for (const auto& [key, value] : componentsTable) {
        if (key.get_type() != sol::type::string) continue;

        std::string componentName = key.as<std::string>();
        ComponentDef compDef;
        compDef.name = componentName;

        if (value.get_type() == sol::type::table) {
            compDef.properties = parsePropertyTable(value.as<sol::table>());
        }

        def.components.push_back(std::move(compDef));
    }
}

void BlueprintFactory::parsePhysicsTable(const sol::table& physicsTable, BlueprintDef& def) {
    if (!def.physics) return;

    auto& physics = *def.physics;

    sol::optional<std::string> bodyType = physicsTable["type"];
    if (bodyType) physics.bodyType = *bodyType;

    sol::optional<sol::table> sizeTable = physicsTable["size"];
    if (sizeTable) {
        sol::optional<float> w = (*sizeTable)[1];
        sol::optional<float> h = (*sizeTable)[2];
        if (w && h) {
            physics.size = Vec2{*w, *h};
        }
    }

    sol::optional<bool> sensor = physicsTable["sensor"];
    if (sensor) physics.sensor = *sensor;

    sol::optional<bool> fixedRotation = physicsTable["fixedRotation"];
    if (fixedRotation) physics.fixedRotation = *fixedRotation;

    sol::optional<float> density = physicsTable["density"];
    if (density) physics.density = *density;

    sol::optional<float> friction = physicsTable["friction"];
    if (friction) physics.friction = *friction;

    sol::optional<float> restitution = physicsTable["restitution"];
    if (restitution) physics.restitution = *restitution;

    sol::optional<float> linearDamping = physicsTable["linearDamping"];
    if (linearDamping) physics.linearDamping = *linearDamping;

    sol::optional<std::string> collisionLayer = physicsTable["collisionLayer"];
    if (collisionLayer) physics.collisionLayer = *collisionLayer;
}

PropertyMap BlueprintFactory::parsePropertyTable(const sol::table& table) {
    PropertyMap props;

    for (const auto& [key, value] : table) {
        if (key.get_type() != sol::type::string) continue;

        std::string propName = key.as<std::string>();
        sol::type valueType = value.get_type();

        switch (valueType) {
            case sol::type::boolean:
                props[propName] = value.as<bool>();
                break;
            case sol::type::number:
                props[propName] = value.as<double>();
                break;
            case sol::type::string:
                props[propName] = value.as<std::string>();
                break;
            case sol::type::table: {
                // Check if it's an array (color, vec2, etc.)
                sol::table tbl = value.as<sol::table>();
                sol::optional<double> first = tbl[1];
                if (first) {
                    // Treat as array of numbers
                    std::vector<double> arr;
                    for (const auto& [k, v] : tbl) {
                        if (v.get_type() == sol::type::number) {
                            arr.push_back(v.as<double>());
                        }
                    }
                    props[propName] = arr;
                } else {
                    // Nested table - store as property map
                    props[propName] = parsePropertyTable(tbl);
                }
                break;
            }
            default:
                break;
        }
    }

    return props;
}

//==============================================================================
// Blueprint Resolution
//==============================================================================

BlueprintDef BlueprintFactory::resolveInheritance(const BlueprintDef& def) const {
    if (def.inherits.empty()) {
        return def;
    }

    auto parentIt = blueprints_.find(def.inherits);
    if (parentIt == blueprints_.end()) {
        return def;
    }

    // Recursively resolve parent
    BlueprintDef parent = resolveInheritance(parentIt->second);

    // Merge child onto parent
    BlueprintDef result = parent;
    mergeBlueprints(result, def);
    return result;
}

void BlueprintFactory::mergeBlueprints(BlueprintDef& base, const BlueprintDef& override) const {
    // base starts as a copy of parent, override is the child
    // We want child properties to override parent properties
    base.name = override.name;

    // Merge components - child components override parent components
    for (const auto& overrideComp : override.components) {
        bool found = false;
        for (auto& baseComp : base.components) {
            if (baseComp.name == overrideComp.name) {
                // Merge properties - child properties override parent properties
                mergeProperties(baseComp.properties, overrideComp.properties);
                found = true;
                break;
            }
        }
        if (!found) {
            base.components.push_back(overrideComp);
        }
    }

    // Child physics overrides parent physics
    if (override.physics) {
        base.physics = override.physics;
    }
    // If child has no physics, parent physics is already in base (from copy)

    // Merge metadata - child metadata overrides parent metadata
    for (const auto& [key, value] : override.metadata) {
        base.metadata[key] = value;
    }
}

void BlueprintFactory::mergeProperties(PropertyMap& base, const PropertyMap& override) const {
    for (const auto& [key, value] : override) {
        // If override has a PropertyMap (nested table), merge recursively
        if (auto* overrideMap = std::any_cast<PropertyMap>(&value)) {
            auto baseIt = base.find(key);
            if (baseIt != base.end()) {
                if (auto* baseMap = std::any_cast<PropertyMap>(&baseIt->second)) {
                    // Both are PropertyMaps, merge recursively
                    mergeProperties(*baseMap, *overrideMap);
                    continue;
                }
            }
            // Base doesn't have this property or it's not a PropertyMap, just overwrite
            base[key] = value;
        } else {
            // Simple value, just overwrite
            base[key] = value;
        }
    }
}

//==============================================================================
// Entity Creation Helpers
//==============================================================================

Entity BlueprintFactory::createEntityFromBlueprint(const BlueprintDef& def,
                                                    float x, float y,
                                                    float width, float height,
                                                    const PropertyMap& overrides) {
    Entity entity = entities_.createEntity();

    // Add Transform2D
    entities_.emplace<Transform2D>(entity, Transform2D{
        .x = x,
        .y = y,
        .rotation = 0.0f,
        .scaleX = 1.0f,
        .scaleY = 1.0f
    });

    // Apply components
    applyComponents(entity, def, overrides);

    // Apply physics first (so we know the final body size)
    if (def.physics && physics_) {
        applyPhysics(entity, def, x, y, width, height);

        // Sync DebugRect/DebugCircle size to match physics body size
        // This ensures visual matches collision at creation time
        Vec2 bodySize = physics_->getBodySize(entity);
        if (bodySize.x > 0 && bodySize.y > 0) {
            if (auto* rect = entities_.tryGet<DebugRect>(entity)) {
                rect->size = bodySize;
            }
            if (auto* circle = entities_.tryGet<DebugCircle>(entity)) {
                circle->radius = std::min(bodySize.x, bodySize.y) / 2.0f;
            }
        }
    }

    return entity;
}

void BlueprintFactory::applyComponents(Entity entity, const BlueprintDef& def, const PropertyMap& overrides) {
    for (const auto& compDef : def.components) {
        auto creatorIt = componentCreators_.find(compDef.name);
        if (creatorIt == componentCreators_.end()) {
            continue;  // Unknown component type
        }

        // Merge properties with overrides
        PropertyMap props = compDef.properties;

        // Apply overrides in format "ComponentName.property" or "ComponentName.nested.property"
        for (const auto& [key, value] : overrides) {
            // Check if override is for this component
            size_t dotPos = key.find('.');
            if (dotPos != std::string::npos) {
                std::string compName = key.substr(0, dotPos);
                if (compName == compDef.name) {
                    std::string propPath = key.substr(dotPos + 1);
                    applyNestedOverride(props, propPath, value);
                }
            }
        }

        // Create component
        creatorIt->second(entity, entities_, props);
    }
}

void BlueprintFactory::applyNestedOverride(PropertyMap& props, const std::string& path, const std::any& value) {
    // Handle nested property paths like "fillColor.r" or "size.x"
    size_t dotPos = path.find('.');
    if (dotPos == std::string::npos) {
        // No nesting, direct property
        props[path] = value;
        return;
    }

    // Split into first part and remaining path
    std::string firstKey = path.substr(0, dotPos);
    std::string remainingPath = path.substr(dotPos + 1);

    // Get or create nested PropertyMap
    auto it = props.find(firstKey);
    if (it == props.end()) {
        // Property doesn't exist, create new nested map
        PropertyMap nested;
        applyNestedOverride(nested, remainingPath, value);
        props[firstKey] = nested;
    } else {
        // Property exists
        if (auto* nestedMap = std::any_cast<PropertyMap>(&it->second)) {
            // It's already a PropertyMap, recurse into it
            applyNestedOverride(*nestedMap, remainingPath, value);
        } else if (auto* arr = std::any_cast<std::vector<double>>(&it->second)) {
            // It's an array (e.g., color {128, 128, 128, 255})
            // Convert to PropertyMap for nested overrides
            PropertyMap nested;
            // For colors, map array indices to r, g, b, a
            if (arr->size() >= 1) nested["r"] = (*arr)[0];
            if (arr->size() >= 2) nested["g"] = (*arr)[1];
            if (arr->size() >= 3) nested["b"] = (*arr)[2];
            if (arr->size() >= 4) nested["a"] = (*arr)[3];
            // Now apply the override
            applyNestedOverride(nested, remainingPath, value);
            props[firstKey] = nested;
        } else {
            // It's some other type, replace it with a PropertyMap
            PropertyMap nested;
            applyNestedOverride(nested, remainingPath, value);
            props[firstKey] = nested;
        }
    }
}

void BlueprintFactory::applyPhysics(Entity entity, const BlueprintDef& def,
                                     float x, float y, float width, float height) {
    if (!def.physics || !physics_) return;

    const auto& physicsDef = *def.physics;

    // Determine body type
    BodyType bodyType = BodyType::Dynamic;
    if (physicsDef.bodyType == "static") {
        bodyType = BodyType::Static;
    } else if (physicsDef.bodyType == "kinematic") {
        bodyType = BodyType::Kinematic;
    }

    // Determine size
    Vec2 bodySize = {width, height};
    if (physicsDef.size) {
        bodySize = *physicsDef.size;
    }
    if (bodySize.x <= 0 || bodySize.y <= 0) {
        // Try to get size from DebugRect component
        if (auto* rect = entities_.tryGet<DebugRect>(entity)) {
            bodySize = rect->size;
        } else {
            bodySize = {32.0f, 32.0f};  // Default
        }
    }

    // Create physics body
    PhysicsBodyDef bodyDef{
        .type = bodyType,
        .transform = {.x = x, .y = y},
        .size = bodySize,
        .fixedRotation = physicsDef.fixedRotation,
        .linearDamping = physicsDef.linearDamping,
        .angularDamping = 0.0f,
        .density = physicsDef.density,
        .friction = physicsDef.friction,
        .restitution = physicsDef.restitution,
        .isSensor = physicsDef.sensor
    };

    physics_->createBody(entity, bodyDef);
}

//==============================================================================
// Built-in Component Registration
//==============================================================================

void BlueprintFactory::registerBuiltinComponents() {
    // Helper to get property with default
    auto getDouble = [](const PropertyMap& props, const std::string& key, double def) -> double {
        auto it = props.find(key);
        if (it == props.end()) return def;
        if (auto* d = std::any_cast<double>(&it->second)) return *d;
        if (auto* i = std::any_cast<int>(&it->second)) return static_cast<double>(*i);
        return def;
    };

    auto getBool = [](const PropertyMap& props, const std::string& key, bool def) -> bool {
        auto it = props.find(key);
        if (it == props.end()) return def;
        if (auto* b = std::any_cast<bool>(&it->second)) return *b;
        return def;
    };

    auto getString = [](const PropertyMap& props, const std::string& key, const std::string& def) -> std::string {
        auto it = props.find(key);
        if (it == props.end()) return def;
        if (auto* s = std::any_cast<std::string>(&it->second)) return *s;
        return def;
    };

    auto getColor = [&getDouble](const PropertyMap& props, const std::string& key, Color def) -> Color {
        auto it = props.find(key);
        if (it == props.end()) return def;

        // Try array format: {255, 128, 0, 255}
        if (auto* arr = std::any_cast<std::vector<double>>(&it->second)) {
            if (arr->size() >= 3) {
                return Color{
                    static_cast<uint8_t>((*arr)[0]),
                    static_cast<uint8_t>((*arr)[1]),
                    static_cast<uint8_t>((*arr)[2]),
                    arr->size() >= 4 ? static_cast<uint8_t>((*arr)[3]) : uint8_t(255)
                };
            }
        }

        // Try nested map format: {r = 255, g = 128, b = 0}
        if (auto* nestedMap = std::any_cast<PropertyMap>(&it->second)) {
            return Color{
                static_cast<uint8_t>(getDouble(*nestedMap, "r", def.r)),
                static_cast<uint8_t>(getDouble(*nestedMap, "g", def.g)),
                static_cast<uint8_t>(getDouble(*nestedMap, "b", def.b)),
                static_cast<uint8_t>(getDouble(*nestedMap, "a", def.a))
            };
        }

        return def;
    };

    // DebugRect component
    registerComponent("DebugRect", [=](Entity e, IEntitySystem& sys, const PropertyMap& props) {
        DebugRect rect;
        rect.size.x = static_cast<float>(getDouble(props, "width", 32.0));
        rect.size.y = static_cast<float>(getDouble(props, "height", 32.0));

        // Also support "size" as array
        auto sizeIt = props.find("size");
        if (sizeIt != props.end()) {
            if (auto* arr = std::any_cast<std::vector<double>>(&sizeIt->second)) {
                if (arr->size() >= 2) {
                    rect.size.x = static_cast<float>((*arr)[0]);
                    rect.size.y = static_cast<float>((*arr)[1]);
                }
            }
        }

        rect.fillColor = getColor(props, "fillColor", Color{128, 128, 128, 255});
        rect.outlineColor = getColor(props, "outlineColor", Color{0, 0, 0, 0});
        rect.outlineWidth = static_cast<float>(getDouble(props, "outlineWidth", 0.0));
        rect.layer = static_cast<RenderLayer>(getDouble(props, "layer", 0.0));
        rect.filled = getBool(props, "filled", true);

        sys.emplace<DebugRect>(e, rect);
    });

    // DebugCircle component
    registerComponent("DebugCircle", [=](Entity e, IEntitySystem& sys, const PropertyMap& props) {
        DebugCircle circle;
        circle.radius = static_cast<float>(getDouble(props, "radius", 16.0));
        circle.fillColor = getColor(props, "fillColor", Color{128, 128, 128, 255});
        circle.outlineColor = getColor(props, "outlineColor", Color{0, 0, 0, 0});
        circle.outlineWidth = static_cast<float>(getDouble(props, "outlineWidth", 0.0));
        circle.layer = static_cast<RenderLayer>(getDouble(props, "layer", 0.0));
        circle.filled = getBool(props, "filled", true);
        circle.segments = static_cast<int>(getDouble(props, "segments", 32.0));

        sys.emplace<DebugCircle>(e, circle);
    });

    // DebugLine component
    registerComponent("DebugLine", [=](Entity e, IEntitySystem& sys, const PropertyMap& props) {
        DebugLine line;

        auto endIt = props.find("endOffset");
        if (endIt != props.end()) {
            if (auto* arr = std::any_cast<std::vector<double>>(&endIt->second)) {
                if (arr->size() >= 2) {
                    line.endOffset.x = static_cast<float>((*arr)[0]);
                    line.endOffset.y = static_cast<float>((*arr)[1]);
                }
            }
        }

        line.color = getColor(props, "color", Color::white());
        line.thickness = static_cast<float>(getDouble(props, "thickness", 1.0));
        line.layer = static_cast<RenderLayer>(getDouble(props, "layer", 0.0));

        sys.emplace<DebugLine>(e, line);
    });

    // Empty tag components
    registerComponent("PlayerTag", [](Entity e, IEntitySystem& sys, const PropertyMap&) {
        // PlayerTag is likely game-specific, but we can register a placeholder
        // Games should register their own tag components
    });

    registerComponent("EnemyTag", [](Entity e, IEntitySystem& sys, const PropertyMap&) {
        // Placeholder
    });

    registerComponent("PlatformTag", [](Entity e, IEntitySystem& sys, const PropertyMap&) {
        // Placeholder
    });
}

}  // namespace jframe
