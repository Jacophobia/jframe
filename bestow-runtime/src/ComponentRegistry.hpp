// bestow-runtime/src/ComponentRegistry.hpp
// Component registry for runtime type access from Lua
//
// This enables Lua to create and access components by string name:
//   entity:addComponent("Transform2D", {x=100, y=200})
//   local t = entity:get("Transform2D")

#pragma once

#include <bestow/sol2_compat.hpp>
#include <bestow/entt_compat.hpp>

#include <string>
#include <unordered_map>
#include <functional>
#include <typeindex>
#include <any>

namespace bestow::runtime {

/// Information about a registered component type
struct ComponentInfo {
    std::string name;
    entt::id_type typeId;
    std::size_t size;

    /// Create component on entity from Lua table data
    std::function<void(entt::registry&, entt::entity, sol::table)> create;

    /// Get component as Lua table
    std::function<sol::table(sol::state&, entt::registry&, entt::entity)> toLua;

    /// Update component from Lua table
    std::function<void(entt::registry&, entt::entity, sol::table)> fromLua;

    /// Check if entity has this component
    std::function<bool(entt::registry&, entt::entity)> has;

    /// Remove component from entity
    std::function<void(entt::registry&, entt::entity)> remove;
};

/// Registry for runtime component type lookup
class ComponentRegistry {
public:
    static ComponentRegistry& instance();

    /// Register a component type with conversion functions
    template<typename T>
    void registerComponent(
        const std::string& name,
        std::function<void(T&, sol::table)> fromLuaFn,
        std::function<sol::table(sol::state&, const T&)> toLuaFn,
        std::function<T()> defaultFactory = []() { return T{}; }
    );

    /// Get component info by name
    const ComponentInfo* getInfo(const std::string& name) const;

    /// Get all registered component names
    std::vector<std::string> getRegisteredNames() const;

    /// Create component on entity from Lua table
    bool createComponent(entt::registry& reg, entt::entity entity,
                         const std::string& name, sol::table data);

    /// Get component as Lua table
    sol::table getComponent(sol::state& lua, entt::registry& reg,
                            entt::entity entity, const std::string& name);

    /// Update component from Lua table
    bool updateComponent(entt::registry& reg, entt::entity entity,
                         const std::string& name, sol::table data);

    /// Check if entity has component
    bool hasComponent(entt::registry& reg, entt::entity entity,
                      const std::string& name);

    /// Remove component from entity
    bool removeComponent(entt::registry& reg, entt::entity entity,
                         const std::string& name);

    /// Register all built-in Bestow components
    void registerBuiltinComponents();

private:
    ComponentRegistry() = default;
    std::unordered_map<std::string, ComponentInfo> registry_;
};

// ============================================================================
// Template Implementation
// ============================================================================

template<typename T>
void ComponentRegistry::registerComponent(
    const std::string& name,
    std::function<void(T&, sol::table)> fromLuaFn,
    std::function<sol::table(sol::state&, const T&)> toLuaFn,
    std::function<T()> defaultFactory
) {
    ComponentInfo info;
    info.name = name;
    info.typeId = entt::type_id<T>().hash();
    info.size = sizeof(T);

    info.create = [fromLuaFn, defaultFactory](entt::registry& reg, entt::entity e, sol::table data) {
        T component = defaultFactory();
        fromLuaFn(component, data);
        reg.emplace<T>(e, std::move(component));
    };

    info.toLua = [toLuaFn](sol::state& lua, entt::registry& reg, entt::entity e) -> sol::table {
        if (reg.all_of<T>(e)) {
            const T& component = reg.get<T>(e);
            return toLuaFn(lua, component);
        }
        return sol::table{};
    };

    info.fromLua = [fromLuaFn](entt::registry& reg, entt::entity e, sol::table data) {
        if (reg.all_of<T>(e)) {
            T& component = reg.get<T>(e);
            fromLuaFn(component, data);
        }
    };

    info.has = [](entt::registry& reg, entt::entity e) -> bool {
        return reg.all_of<T>(e);
    };

    info.remove = [](entt::registry& reg, entt::entity e) {
        if (reg.all_of<T>(e)) {
            reg.remove<T>(e);
        }
    };

    registry_[name] = std::move(info);
}

}  // namespace bestow::runtime
