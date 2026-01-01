// bestow-luabind/src/bindings/entity_binding.cpp
// Entity system Lua bindings

module;

#include <bestow/sol2_compat.hpp>
#include <spdlog/spdlog.h>

module bestow.luabind;

import std;

namespace bestow {

// Helper to convert ComponentFieldValue to sol::object
sol::object fieldValueToLua(sol::state& lua, const ComponentFieldValue& value) {
    return std::visit([&lua](auto&& v) -> sol::object {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return sol::nil;
        } else {
            return sol::make_object(lua, v);
        }
    }, value);
}

// Helper to convert sol::object to ComponentFieldValue
ComponentFieldValue luaToFieldValue(sol::object obj) {
    if (obj.is<bool>()) {
        return obj.as<bool>();
    } else if (obj.is<std::int64_t>()) {
        return obj.as<std::int64_t>();
    } else if (obj.is<double>()) {
        return obj.as<double>();
    } else if (obj.is<std::string>()) {
        return obj.as<std::string>();
    } else if (obj.is<Vec2>()) {
        return obj.as<Vec2>();
    } else if (obj.is<Vec3>()) {
        return obj.as<Vec3>();
    } else if (obj.is<Vec4>()) {
        return obj.as<Vec4>();
    } else if (obj.is<Quat>()) {
        return obj.as<Quat>();
    } else if (obj.is<Color>()) {
        return obj.as<Color>();
    } else if (obj.is<Entity>()) {
        return obj.as<Entity>();
    } else if (obj.is<std::uint64_t>()) {
        return obj.as<std::uint64_t>();
    }
    return std::monostate{};
}

// Helper to convert ComponentData to Lua table
sol::table componentDataToLua(sol::state& lua, const ComponentData& data) {
    sol::table result = lua.create_table();
    for (const auto& [key, value] : data) {
        result[key] = fieldValueToLua(lua, value);
    }
    return result;
}

// Helper to convert Lua table to ComponentData
ComponentData luaToComponentData(sol::table table) {
    ComponentData result;
    for (auto& [key, value] : table) {
        if (key.is<std::string>()) {
            result[key.as<std::string>()] = luaToFieldValue(value);
        }
    }
    return result;
}

void bindEntitySystem(sol::state& lua, IEntitySystem& entities) {
    //=========================================================================
    // Entity-related types
    //=========================================================================

    // ComponentFieldType enum
    lua.new_enum<ComponentFieldType>("ComponentFieldType",
        {
            {"Unknown", ComponentFieldType::Unknown},
            {"Bool", ComponentFieldType::Bool},
            {"Int", ComponentFieldType::Int},
            {"Float", ComponentFieldType::Float},
            {"Double", ComponentFieldType::Double},
            {"String", ComponentFieldType::String},
            {"Vec2", ComponentFieldType::Vec2},
            {"Vec3", ComponentFieldType::Vec3},
            {"Vec4", ComponentFieldType::Vec4},
            {"Quat", ComponentFieldType::Quat},
            {"Color", ComponentFieldType::Color},
            {"Entity", ComponentFieldType::Entity},
            {"Handle", ComponentFieldType::Handle},
            {"Enum", ComponentFieldType::Enum},
            {"Struct", ComponentFieldType::Struct},
            {"Array", ComponentFieldType::Array}
        }
    );

    // ComponentFieldInfo struct
    lua.new_usertype<ComponentFieldInfo>("ComponentFieldInfo",
        sol::constructors<ComponentFieldInfo()>(),
        "name", &ComponentFieldInfo::name,
        "type", &ComponentFieldInfo::type,
        "offset", &ComponentFieldInfo::offset,
        "size", &ComponentFieldInfo::size,
        "readOnly", &ComponentFieldInfo::readOnly,
        "enumTypeName", &ComponentFieldInfo::enumTypeName
    );

    // ComponentTypeInfo struct
    lua.new_usertype<ComponentTypeInfo>("ComponentTypeInfo",
        sol::constructors<ComponentTypeInfo()>(),
        "name", &ComponentTypeInfo::name,
        "size", &ComponentTypeInfo::size,
        "fields", &ComponentTypeInfo::fields,
        "canConstruct", &ComponentTypeInfo::canConstruct
    );

    //=========================================================================
    // bestow.entity table
    //=========================================================================

    sol::table bestow = lua["bestow"];
    sol::table entityTable = lua.create_table();

    //-------------------------------------------------------------------------
    // Entity Lifecycle
    //-------------------------------------------------------------------------

    entityTable["create"] = [&entities]() {
        return entities.createEntity();
    };

    entityTable["destroy"] = [&entities](Entity entity) {
        entities.destroyEntity(entity);
    };

    entityTable["isValid"] = [&entities](Entity entity) {
        return entities.isValid(entity);
    };

    entityTable["count"] = [&entities]() {
        return entities.entityCount();
    };

    //-------------------------------------------------------------------------
    // Iteration
    //-------------------------------------------------------------------------

    entityTable["each"] = [&entities](sol::function callback) {
        entities.each([&callback](Entity entity) {
            callback(entity);
        });
    };

    //-------------------------------------------------------------------------
    // Component Type Registration (for reflection)
    //-------------------------------------------------------------------------

    entityTable["isTypeRegistered"] = [&entities](const std::string& typeName) {
        return entities.isComponentTypeRegistered(typeName);
    };

    entityTable["getRegisteredTypes"] = [&entities]() {
        return entities.getRegisteredComponentTypes();
    };

    entityTable["getTypeInfo"] = [&entities, &lua](const std::string& typeName) -> sol::object {
        auto info = entities.getComponentTypeInfo(typeName);
        if (info) {
            return sol::make_object(lua, *info);
        }
        return sol::nil;
    };

    entityTable["getFields"] = [&entities](const std::string& typeName) {
        return entities.getComponentFields(typeName);
    };

    //-------------------------------------------------------------------------
    // Component Access (by type name)
    //-------------------------------------------------------------------------

    entityTable["addComponent"] = sol::overload(
        [&entities](Entity entity, const std::string& typeName) {
            return entities.addComponentByName(entity, typeName);
        },
        [&entities](Entity entity, const std::string& typeName, sol::table data) {
            return entities.addComponentByName(entity, typeName, luaToComponentData(data));
        }
    );

    entityTable["removeComponent"] = [&entities](Entity entity, const std::string& typeName) {
        return entities.removeComponentByName(entity, typeName);
    };

    entityTable["hasComponent"] = [&entities](Entity entity, const std::string& typeName) {
        return entities.hasComponentByName(entity, typeName);
    };

    entityTable["getComponent"] = [&entities, &lua](Entity entity, const std::string& typeName) -> sol::object {
        auto data = entities.getComponentByName(entity, typeName);
        if (data) {
            return componentDataToLua(lua, *data);
        }
        return sol::nil;
    };

    entityTable["setComponent"] = [&entities](Entity entity, const std::string& typeName, sol::table data) {
        return entities.setComponentByName(entity, typeName, luaToComponentData(data));
    };

    //-------------------------------------------------------------------------
    // Single Field Access
    //-------------------------------------------------------------------------

    entityTable["getField"] = [&entities, &lua](Entity entity, const std::string& typeName, const std::string& fieldName) -> sol::object {
        auto value = entities.getComponentField(entity, typeName, fieldName);
        if (value) {
            return fieldValueToLua(lua, *value);
        }
        return sol::nil;
    };

    entityTable["setField"] = [&entities](Entity entity, const std::string& typeName, const std::string& fieldName, sol::object value) {
        return entities.setComponentField(entity, typeName, fieldName, luaToFieldValue(value));
    };

    bestow["entity"] = entityTable;
}

}  // namespace bestow
