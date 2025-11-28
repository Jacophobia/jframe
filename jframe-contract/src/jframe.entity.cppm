// jframe-contract/src/jframe.entity.cppm
// Entity system interface

module;

#include <cstddef>
#include <functional>
#include <optional>
#include <utility>
#include <vector>

#include <entt/entt.hpp>

export module jframe.entity;

import jframe.types;

export namespace jframe {

struct EntitySelector {
    std::vector<entt::id_type> requiredComponents;
    std::vector<entt::id_type> excludedComponents;
    std::optional<std::function<bool(Entity)>> predicate;
};

class IEntitySystem {
public:
    virtual ~IEntitySystem() = default;

    //======================================================================
    // Entity Lifecycle
    //======================================================================

    virtual Entity createEntity() = 0;
    virtual void destroyEntity(Entity entity) = 0;
    virtual bool isValid(Entity entity) const = 0;
    virtual std::size_t entityCount() const = 0;

    //======================================================================
    // Component Access (Type-Erased for Interface Boundary)
    //======================================================================

    virtual void* addComponent(Entity entity, entt::id_type typeId,
                               const void* data, std::size_t size) = 0;
    virtual void removeComponent(Entity entity, entt::id_type typeId) = 0;
    virtual void* getComponent(Entity entity, entt::id_type typeId) = 0;
    virtual const void* getComponent(Entity entity, entt::id_type typeId) const = 0;
    virtual bool hasComponent(Entity entity, entt::id_type typeId) const = 0;

    //======================================================================
    // Typed Component Helpers
    //======================================================================

    template<typename T, typename... Args>
    T& emplace(Entity entity, Args&&... args) {
        auto& registry = getRegistry();
        return registry.emplace<T>(entity, std::forward<Args>(args)...);
    }

    template<typename T>
    void remove(Entity entity) {
        getRegistry().remove<T>(entity);
    }

    template<typename T>
    T& get(Entity entity) {
        return getRegistry().get<T>(entity);
    }

    template<typename T>
    const T& get(Entity entity) const {
        return getRegistry().get<T>(entity);
    }

    template<typename T>
    T* tryGet(Entity entity) {
        return getRegistry().try_get<T>(entity);
    }

    template<typename T>
    const T* tryGet(Entity entity) const {
        return getRegistry().try_get<T>(entity);
    }

    template<typename... Ts>
    bool allOf(Entity entity) const {
        return getRegistry().all_of<Ts...>(entity);
    }

    template<typename... Ts>
    bool anyOf(Entity entity) const {
        return getRegistry().any_of<Ts...>(entity);
    }

    //======================================================================
    // Querying
    //======================================================================

    virtual std::vector<Entity> query(const EntitySelector& selector) const = 0;

    template<typename... Components>
    auto view() {
        return getRegistry().view<Components...>();
    }

    template<typename... Components>
    auto view() const {
        return getRegistry().view<Components...>();
    }

    //======================================================================
    // Entity Groups - Convenient Query Methods
    //======================================================================

    /// Get count of entities with specified components (O(n) iteration)
    template<typename... Components>
    std::size_t groupCount() const {
        std::size_t count = 0;
        for ([[maybe_unused]] auto entity : getRegistry().view<Components...>()) {
            ++count;
        }
        return count;
    }

    /// Check if any entities exist with specified components
    template<typename... Components>
    bool hasAny() const {
        auto v = getRegistry().view<Components...>();
        return v.begin() != v.end();
    }

    /// Get first entity with specified components, or nullopt if none
    template<typename... Components>
    std::optional<Entity> first() const {
        auto v = getRegistry().view<Components...>();
        auto it = v.begin();
        if (it != v.end()) {
            return *it;
        }
        return std::nullopt;
    }

    /// Get the single entity with specified components
    /// Returns nullopt if zero entities, the entity if exactly one
    /// Note: Does not throw - returns nullopt for 0 or 2+ entities
    template<typename... Components>
    std::optional<Entity> single() const {
        auto v = getRegistry().view<Components...>();
        auto it = v.begin();
        if (it == v.end()) {
            return std::nullopt;  // No entities
        }
        Entity result = *it;
        ++it;
        if (it != v.end()) {
            return std::nullopt;  // More than one entity
        }
        return result;
    }

    /// Collect all entities with specified components into a vector
    /// Use this when you need to modify entities during iteration
    template<typename... Components>
    std::vector<Entity> collect() const {
        std::vector<Entity> result;
        for (auto entity : getRegistry().view<Components...>()) {
            result.push_back(entity);
        }
        return result;
    }

    /// Collect entities with exclusion filter
    /// Usage: collectExcluding<Include, Exclude1, Exclude2, ...>()
    /// The first template parameter is the required component,
    /// all subsequent parameters are excluded components
    template<typename Include, typename... Exclude>
    std::vector<Entity> collectExcluding() const {
        std::vector<Entity> result;
        if constexpr (sizeof...(Exclude) > 0) {
            // With exclusions
            for (auto entity : getRegistry().view<Include>(entt::exclude<Exclude...>)) {
                result.push_back(entity);
            }
        } else {
            // No exclusions, same as collect<Include>()
            for (auto entity : getRegistry().view<Include>()) {
                result.push_back(entity);
            }
        }
        return result;
    }

    //======================================================================
    // Registry Access
    //======================================================================

    virtual entt::registry& getRegistry() = 0;
    virtual const entt::registry& getRegistry() const = 0;

    //======================================================================
    // Iteration
    //======================================================================

    virtual void each(std::function<void(Entity)> callback) = 0;
    virtual void update(DeltaTime dt) = 0;
};

}  // namespace jframe
