// bestow-runtime/src/ComponentRegistry.cpp
// Component registry implementation with built-in component registrations

#include "ComponentRegistry.hpp"
#include <spdlog/spdlog.h>

// Forward declare the types we'll register
// (actual imports happen in the module units that include this)

namespace bestow::runtime {

ComponentRegistry& ComponentRegistry::instance() {
    static ComponentRegistry registry;
    return registry;
}

const ComponentInfo* ComponentRegistry::getInfo(const std::string& name) const {
    auto it = registry_.find(name);
    if (it != registry_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<std::string> ComponentRegistry::getRegisteredNames() const {
    std::vector<std::string> names;
    names.reserve(registry_.size());
    for (const auto& [name, info] : registry_) {
        names.push_back(name);
    }
    return names;
}

bool ComponentRegistry::createComponent(
    entt::registry& reg, entt::entity entity,
    const std::string& name, sol::table data
) {
    auto* info = getInfo(name);
    if (!info) {
        spdlog::warn("[ComponentRegistry] Unknown component type: {}", name);
        return false;
    }
    info->create(reg, entity, data);
    return true;
}

sol::table ComponentRegistry::getComponent(
    sol::state& lua, entt::registry& reg,
    entt::entity entity, const std::string& name
) {
    auto* info = getInfo(name);
    if (!info) {
        spdlog::warn("[ComponentRegistry] Unknown component type: {}", name);
        return sol::table{};
    }
    return info->toLua(lua, reg, entity);
}

bool ComponentRegistry::updateComponent(
    entt::registry& reg, entt::entity entity,
    const std::string& name, sol::table data
) {
    auto* info = getInfo(name);
    if (!info) {
        spdlog::warn("[ComponentRegistry] Unknown component type: {}", name);
        return false;
    }
    info->fromLua(reg, entity, data);
    return true;
}

bool ComponentRegistry::hasComponent(
    entt::registry& reg, entt::entity entity,
    const std::string& name
) {
    auto* info = getInfo(name);
    if (!info) {
        return false;
    }
    return info->has(reg, entity);
}

bool ComponentRegistry::removeComponent(
    entt::registry& reg, entt::entity entity,
    const std::string& name
) {
    auto* info = getInfo(name);
    if (!info) {
        spdlog::warn("[ComponentRegistry] Unknown component type: {}", name);
        return false;
    }
    info->remove(reg, entity);
    return true;
}

}  // namespace bestow::runtime
