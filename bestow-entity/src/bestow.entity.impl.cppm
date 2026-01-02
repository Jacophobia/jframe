// bestow-entity/src/bestow.entity.impl.cppm
// Entity system implementation

module;

#include <kangaru/kangaru.hpp>
// Use compatibility header for MSVC C++23 module support
#include <bestow/entt_compat.hpp>

export module bestow.entity.impl;

import std;
import bestow.entity;
import bestow.types;
import bestow.services;

export namespace bestow {

class EntitySystem : public IEntitySystem {
public:
    EntitySystem() = default;
    ~EntitySystem() override = default;

    //======================================================================
    // Entity Lifecycle
    //======================================================================

    Entity createEntity() override {
        return registry_.create();
    }

    void destroyEntity(Entity entity) override {
        if (registry_.valid(entity)) {
            registry_.destroy(entity);
        }
    }

    bool isValid(Entity entity) const override {
        return registry_.valid(entity);
    }

    std::size_t entityCount() const override {
        std::size_t count = 0;
        auto* storage = registry_.storage<Entity>();
        if (storage) {
            for (auto entity : *storage) {
                if (registry_.valid(entity)) {
                    ++count;
                }
            }
        }
        return count;
    }

    //======================================================================
    // Component Access (Type-Erased)
    //======================================================================

    void* addComponent(Entity entity, entt::id_type typeId,
                       const void* data, std::size_t size) override {
        // Type-erased component addition is complex with EnTT
        // In practice, use the typed template methods
        return nullptr;
    }

    void removeComponent(Entity entity, entt::id_type typeId) override {
        // Type-erased removal - in practice use typed template methods
    }

    void* getComponent(Entity entity, entt::id_type typeId) override {
        return nullptr;
    }

    const void* getComponent(Entity entity, entt::id_type typeId) const override {
        return nullptr;
    }

    bool hasComponent(Entity entity, entt::id_type typeId) const override {
        return false;
    }

    //======================================================================
    // Querying
    //======================================================================

    std::vector<Entity> query(const EntitySelector& selector) const override {
        std::vector<Entity> result;

        for (auto entity : *registry_.storage<Entity>()) {
            bool matches = true;

            // Check required components
            for (auto typeId : selector.requiredComponents) {
                // Type-erased component checking would go here
            }

            // Check excluded components
            for (auto typeId : selector.excludedComponents) {
                // Type-erased component checking would go here
            }

            // Check predicate
            if (matches && selector.predicate) {
                matches = (*selector.predicate)(entity);
            }

            if (matches) {
                result.push_back(entity);
            }
        }

        return result;
    }

    //======================================================================
    // Registry Access
    //======================================================================

    entt::registry& getRegistry() override {
        return registry_;
    }

    const entt::registry& getRegistry() const override {
        return registry_;
    }

    //======================================================================
    // Iteration
    //======================================================================

    void each(std::function<void(Entity)> callback) override {
        auto& storage = registry_.storage<Entity>();
        for (auto entity : storage) {
            if (registry_.valid(entity)) {
                callback(entity);
            }
        }
    }

    void update(DeltaTime dt) override {
        // Entity system doesn't need per-frame updates by default
    }

    //======================================================================
    // Reflection-Based Component Access (for Lua/Scripting)
    //======================================================================

    void registerComponentType(std::string_view typeName,
                                ComponentTypeInfo typeInfo) override {
        componentTypes_[std::string(typeName)] = std::move(typeInfo);
    }

    void unregisterComponentType(std::string_view typeName) override {
        componentTypes_.erase(std::string(typeName));
        componentFactories_.erase(std::string(typeName));
    }

    bool isComponentTypeRegistered(std::string_view typeName) const override {
        return componentTypes_.contains(std::string(typeName));
    }

    std::optional<ComponentTypeInfo> getComponentTypeInfo(
        std::string_view typeName) const override {
        auto it = componentTypes_.find(std::string(typeName));
        if (it != componentTypes_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    std::vector<std::string> getRegisteredComponentTypes() const override {
        std::vector<std::string> result;
        result.reserve(componentTypes_.size());
        for (const auto& [name, info] : componentTypes_) {
            result.push_back(name);
        }
        return result;
    }

    bool addComponentByName(Entity entity,
                             std::string_view typeName,
                             const ComponentData& data) override {
        if (!isValid(entity)) {
            return false;
        }

        auto it = componentFactories_.find(std::string(typeName));
        if (it == componentFactories_.end()) {
            return false;
        }

        return it->second.add(entity, data, registry_);
    }

    bool removeComponentByName(Entity entity,
                                std::string_view typeName) override {
        if (!isValid(entity)) {
            return false;
        }

        auto it = componentFactories_.find(std::string(typeName));
        if (it == componentFactories_.end()) {
            return false;
        }

        return it->second.remove(entity, registry_);
    }

    bool hasComponentByName(Entity entity,
                             std::string_view typeName) const override {
        if (!isValid(entity)) {
            return false;
        }

        auto it = componentFactories_.find(std::string(typeName));
        if (it == componentFactories_.end()) {
            return false;
        }

        return it->second.has(entity, registry_);
    }

    std::optional<ComponentData> getComponentByName(
        Entity entity,
        std::string_view typeName) const override {
        if (!isValid(entity)) {
            return std::nullopt;
        }

        auto it = componentFactories_.find(std::string(typeName));
        if (it == componentFactories_.end()) {
            return std::nullopt;
        }

        return it->second.get(entity, registry_);
    }

    bool setComponentByName(Entity entity,
                             std::string_view typeName,
                             const ComponentData& data) override {
        if (!isValid(entity)) {
            return false;
        }

        auto it = componentFactories_.find(std::string(typeName));
        if (it == componentFactories_.end()) {
            return false;
        }

        return it->second.set(entity, data, registry_);
    }

    std::optional<ComponentFieldValue> getComponentField(
        Entity entity,
        std::string_view typeName,
        std::string_view fieldName) const override {
        auto data = getComponentByName(entity, typeName);
        if (!data) {
            return std::nullopt;
        }

        auto it = data->find(std::string(fieldName));
        if (it != data->end()) {
            return it->second;
        }
        return std::nullopt;
    }

    bool setComponentField(Entity entity,
                            std::string_view typeName,
                            std::string_view fieldName,
                            const ComponentFieldValue& value) override {
        ComponentData data;
        data[std::string(fieldName)] = value;
        return setComponentByName(entity, typeName, data);
    }

    std::vector<ComponentFieldInfo> getComponentFields(
        std::string_view typeName) const override {
        auto it = componentTypes_.find(std::string(typeName));
        if (it != componentTypes_.end()) {
            return it->second.fields;
        }
        return {};
    }

    //======================================================================
    // Component Factory Registration (for Lua/Scripting)
    //======================================================================

    /// Type-erased component factory functions
    struct ComponentFactory {
        std::function<bool(Entity, const ComponentData&, entt::registry&)> add;
        std::function<bool(Entity, entt::registry&)> remove;
        std::function<bool(Entity, const entt::registry&)> has;
        std::function<std::optional<ComponentData>(Entity, const entt::registry&)> get;
        std::function<bool(Entity, const ComponentData&, entt::registry&)> set;
    };

    /// Register factory functions for a component type.
    /// This must be called in addition to registerComponentType() for
    /// the ByName methods to work.
    void registerComponentFactory(std::string_view typeName,
                                   ComponentFactory factory) {
        componentFactories_[std::string(typeName)] = std::move(factory);
    }

private:
    entt::registry registry_;
    std::unordered_map<std::string, ComponentTypeInfo> componentTypes_;
    std::unordered_map<std::string, ComponentFactory> componentFactories_;

public:
    // Forward declaration - defined after class is complete
    struct Service;
};

// Service type for Engine::use<IEntitySystem, EntitySystem>()
struct EntitySystem::Service : kgr::single_service<EntitySystem>, kgr::overrides<IEntitySystemService> {};

// Backwards compatibility alias
using EntitySystemService = EntitySystem::Service;

}  // namespace bestow
