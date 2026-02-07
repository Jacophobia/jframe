// tests/mocks/MockEntitySystem.hpp
// Shared mock entity system for testing

#pragma once

#include <bestow/entt_compat.hpp>
#include <kangaru/kangaru.hpp>

import std;
import bestow;
import bestow.types;

namespace bestow::tests {

class MockEntitySystem : public IEntitySystem {
public:
    void update(DeltaTime) override {}

    Entity createEntity() override { return registry_.create(); }

    void destroyEntity(Entity entity) override {
        if (registry_.valid(entity)) {
            registry_.destroy(entity);
        }
    }

    bool isValid(Entity entity) const override { return registry_.valid(entity); }

    std::size_t entityCount() const override {
        std::size_t count = 0;
        const auto* storage = registry_.storage<entt::entity>();
        if (storage) {
            for (auto entity : *storage) {
                if (registry_.valid(entity)) {
                    ++count;
                }
            }
        }
        return count;
    }

    entt::registry& getRegistry() override { return registry_; }
    const entt::registry& getRegistry() const override { return registry_; }

    std::vector<Entity> query(const EntitySelector& selector) const override {
        std::vector<Entity> result;
        const auto* storage = registry_.storage<entt::entity>();
        if (storage) {
            for (auto entity : *storage) {
                if (registry_.valid(entity)) {
                    if (!selector.predicate.has_value() || (*selector.predicate)(entity)) {
                        result.push_back(entity);
                    }
                }
            }
        }
        return result;
    }

    void each(std::function<void(Entity)> callback) override {
        auto& storage = registry_.storage<entt::entity>();
        for (auto entity : storage) {
            if (registry_.valid(entity)) {
                callback(entity);
            }
        }
    }

    // Type-erased component access (stubs)
    void* addComponent(Entity, entt::id_type, const void*, std::size_t) override {
        return nullptr;
    }
    void removeComponent(Entity, entt::id_type) override {}
    void* getComponent(Entity, entt::id_type) override { return nullptr; }
    const void* getComponent(Entity, entt::id_type) const override { return nullptr; }
    bool hasComponent(Entity, entt::id_type) const override { return false; }

    // Reflection-based component access (stubs)
    void registerComponentType(std::string_view, ComponentTypeInfo) override {}
    void unregisterComponentType(std::string_view) override {}
    bool isComponentTypeRegistered(std::string_view) const override { return false; }
    std::optional<ComponentTypeInfo> getComponentTypeInfo(std::string_view) const override {
        return std::nullopt;
    }
    std::vector<std::string> getRegisteredComponentTypes() const override { return {}; }

    bool addComponentByName(Entity, std::string_view, const ComponentData&) override {
        return false;
    }
    bool removeComponentByName(Entity, std::string_view) override { return false; }
    bool hasComponentByName(Entity, std::string_view) const override { return false; }
    std::optional<ComponentData> getComponentByName(Entity, std::string_view) const override {
        return std::nullopt;
    }
    bool setComponentByName(Entity, std::string_view, const ComponentData&) override {
        return false;
    }

    std::optional<ComponentFieldValue> getComponentField(Entity, std::string_view,
                                                          std::string_view) const override {
        return std::nullopt;
    }
    bool setComponentField(Entity, std::string_view, std::string_view,
                           const ComponentFieldValue&) override {
        return false;
    }
    std::vector<ComponentFieldInfo> getComponentFields(std::string_view) const override {
        return {};
    }

    // Template helpers (same as inline mock had)
    template <typename Component, typename... Args>
    decltype(auto) emplace(Entity entity, Args&&... args) {
        return registry_.emplace<Component>(entity, std::forward<Args>(args)...);
    }

    template <typename Component>
    Component& get(Entity entity) {
        return registry_.get<Component>(entity);
    }

    template <typename Component>
    const Component& get(Entity entity) const {
        return registry_.get<Component>(entity);
    }

    template <typename Component>
    Component* tryGet(Entity entity) {
        return registry_.try_get<Component>(entity);
    }

    template <typename Component>
    const Component* tryGet(Entity entity) const {
        return registry_.try_get<Component>(entity);
    }

    template <typename Component>
    void remove(Entity entity) {
        registry_.remove<Component>(entity);
    }

    template <typename... Components>
    bool allOf(Entity entity) const {
        return registry_.all_of<Components...>(entity);
    }

    template <typename... Components>
    bool anyOf(Entity entity) const {
        return registry_.any_of<Components...>(entity);
    }

    template <typename... Components>
    auto view() {
        return registry_.view<Components...>();
    }

    template <typename... Components>
    auto view() const {
        return registry_.view<Components...>();
    }

    template <typename... Components>
    std::size_t groupCount() const {
        std::size_t count = 0;
        for ([[maybe_unused]] auto entity : registry_.view<Components...>()) {
            ++count;
        }
        return count;
    }

    template <typename... Components>
    bool hasAny() const {
        return !registry_.view<Components...>().empty();
    }

    template <typename... Components>
    std::optional<Entity> first() const {
        auto view = registry_.view<Components...>();
        auto it = view.begin();
        return it != view.end() ? std::make_optional(*it) : std::nullopt;
    }

    template <typename... Components>
    std::optional<Entity> single() const {
        auto view = registry_.view<Components...>();
        if (view.size() == 1) {
            return *view.begin();
        }
        return std::nullopt;
    }

    template <typename... Components>
    std::vector<Entity> collect() const {
        auto view = registry_.view<Components...>();
        return std::vector<Entity>(view.begin(), view.end());
    }

    template <typename Include, typename Exclude>
    std::vector<Entity> collectExcluding() const {
        std::vector<Entity> result;
        auto view = registry_.view<Include>();
        for (auto entity : view) {
            if (!registry_.all_of<Exclude>(entity)) {
                result.push_back(entity);
            }
        }
        return result;
    }

private:
    entt::registry registry_;
};

}  // namespace bestow::tests
