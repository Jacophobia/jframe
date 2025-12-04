// bestow-dev/src/ComponentRegistry.cpp
// Component introspection system for entity inspector

module;

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

// MSVC C++23 module compatibility - include full EnTT before import std
#include <bestow/entt_compat.hpp>

module bestow.dev;

import bestow;

namespace bestow::dev {

//==========================================================================
// Component Inspector
//==========================================================================

struct ComponentInfo {
    std::string name;
    std::function<bool(const BestowEngine&, Entity)> hasComponent;
    std::function<std::string(const BestowEngine&, Entity)> serialize;
};

class ComponentRegistry {
public:
    static ComponentRegistry& instance() {
        static ComponentRegistry registry;
        return registry;
    }

    void registerComponent(const std::string& name,
                            std::function<bool(const BestowEngine&, Entity)> hasComponent,
                            std::function<std::string(const BestowEngine&, Entity)> serialize) {
        components_[name] = ComponentInfo{name, hasComponent, serialize};
    }

    std::vector<std::string> getComponentsForEntity(const BestowEngine& engine, Entity entity) const {
        std::vector<std::string> result;
        for (const auto& [name, info] : components_) {
            if (info.hasComponent(engine, entity)) {
                result.push_back(name);
            }
        }
        return result;
    }

    std::string serializeComponent(const BestowEngine& engine, Entity entity,
                                    const std::string& componentName) const {
        auto it = components_.find(componentName);
        if (it != components_.end()) {
            return it->second.serialize(engine, entity);
        }
        return "-- Component not found\n";
    }

    const std::unordered_map<std::string, ComponentInfo>& getAllComponents() const {
        return components_;
    }

private:
    ComponentRegistry() {
        // Register standard components
        registerStandardComponents();
    }

    void registerStandardComponents() {
        // Transform2D - core framework component
        registerComponent(
            "Transform2D",
            [](const BestowEngine& engine, Entity e) {
                return engine.entities && engine.entities->allOf<Transform2D>(e);
            },
            [](const BestowEngine& engine, Entity e) -> std::string {
                if (!engine.entities || !engine.entities->allOf<Transform2D>(e)) {
                    return "-- No Transform2D\n";
                }
                const auto& t = engine.entities->get<Transform2D>(e);
                return std::format(
                    "Transform2D = {{\n"
                    "  x = {:.2f},\n"
                    "  y = {:.2f},\n"
                    "  rotation = {:.2f},\n"
                    "  scaleX = {:.2f},\n"
                    "  scaleY = {:.2f}\n"
                    "}}", t.x, t.y, t.rotation, t.scaleX, t.scaleY);
            }
        );

        // Camera - framework component
        registerComponent(
            "Camera",
            [](const BestowEngine& engine, Entity e) {
                return engine.entities && engine.entities->allOf<Camera>(e);
            },
            [](const BestowEngine& engine, Entity e) -> std::string {
                if (!engine.entities || !engine.entities->allOf<Camera>(e)) {
                    return "-- No Camera\n";
                }
                const auto& c = engine.entities->get<Camera>(e);
                return std::format(
                    "Camera = {{\n"
                    "  transform = {{ x = {:.2f}, y = {:.2f} }},\n"
                    "  zoom = {:.2f}\n"
                    "}}", c.transform.x, c.transform.y, c.zoom);
            }
        );

        // Sprite - framework component
        registerComponent(
            "Sprite",
            [](const BestowEngine& engine, Entity e) {
                return engine.entities && engine.entities->allOf<Sprite>(e);
            },
            [](const BestowEngine& engine, Entity e) -> std::string {
                if (!engine.entities || !engine.entities->allOf<Sprite>(e)) {
                    return "-- No Sprite\n";
                }
                const auto& s = engine.entities->get<Sprite>(e);
                return std::format(
                    "Sprite = {{\n"
                    "  -- Texture handle pointer\n"
                    "  tint = {{ r = {}, g = {}, b = {}, a = {} }}\n"
                    "}}", s.tint.r, s.tint.g, s.tint.b, s.tint.a);
            }
        );

        // Note: Game-specific components (Health, Velocity2D, etc.) should be
        // registered by the game code using ComponentRegistry::instance().registerComponent()
    }

    std::unordered_map<std::string, ComponentInfo> components_;
};

// Accessor function for external use
ComponentRegistry& getComponentRegistry() {
    return ComponentRegistry::instance();
}

}  // namespace bestow::dev
