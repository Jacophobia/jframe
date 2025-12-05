// bestow-entity/src/bestow.entity.impl.cppm
// Entity system implementation

module;

// Use compatibility header for MSVC C++23 module support
#include <bestow/entt_compat.hpp>

export module bestow.entity.impl;

import std;
import bestow.entity;
import bestow.types;

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

private:
    entt::registry registry_;
};

// Factory function (exported via namespace)
inline std::unique_ptr<IEntitySystem> createEntitySystem() {
    return std::make_unique<EntitySystem>();
}

}  // namespace bestow
