// bestow-runtime/src/LuaBindings.hpp
// Lua bindings for Bestow runtime (PROTOTYPE)
//
// This is a simplified prototype that doesn't depend on engine interfaces.
// Full integration will connect to IEntitySystem, IInputSystem, etc.

#pragma once

#include <bestow/sol2_compat.hpp>
#include <bestow/entt_compat.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <memory>
#include <string>
#include <vector>

namespace bestow::runtime {

class ComponentRegistry;

/// Entity wrapper for Lua with fluent API
class LuaEntity {
public:
    LuaEntity(entt::entity entity, entt::registry* registry, sol::state* lua);

    /// Get the raw entity handle
    entt::entity handle() const { return entity_; }

    /// Check if entity is valid
    bool isValid() const;

    /// Destroy this entity
    void destroy();

    // ========================================================================
    // Fluent Builders (chainable)
    // ========================================================================

    /// Set 2D position
    LuaEntity& at(float x, float y);

    /// Set 3D position
    LuaEntity& at3D(float x, float y, float z);

    /// Add tag for queries
    LuaEntity& withTag(const std::string& tag);

    /// Set scale (uniform)
    LuaEntity& withScale(float s);

    /// Set rotation (2D, degrees)
    LuaEntity& withRotation(float degrees);

    // ========================================================================
    // Component Access
    // ========================================================================

    /// Add component by name with Lua table data
    LuaEntity& addComponent(const std::string& name, sol::table data);

    /// Get component as Lua table
    sol::table get(const std::string& name);

    /// Check if entity has component
    bool has(const std::string& name);

    /// Remove component by name
    void remove(const std::string& name);

    // ========================================================================
    // Tag Access
    // ========================================================================

    /// Check if entity has a specific tag
    bool hasTag(const std::string& tag);

    /// Get all tags on this entity
    std::vector<std::string> getTags();

private:
    entt::entity entity_;
    entt::registry* registry_;
    sol::state* lua_;
};

/// Tag component for entity queries
struct TagComponent {
    std::vector<std::string> tags;

    bool hasTag(const std::string& tag) const {
        for (const auto& t : tags) {
            if (t == tag) return true;
        }
        return false;
    }

    void addTag(const std::string& tag) {
        if (!hasTag(tag)) {
            tags.push_back(tag);
        }
    }
};

/// Position component for entities
struct Position2D {
    float x = 0.0f;
    float y = 0.0f;
};

struct Position3D {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct Scale2D {
    float x = 1.0f;
    float y = 1.0f;
};

struct Rotation2D {
    float degrees = 0.0f;
};

/// Main Lua bindings class
class LuaBindings {
public:
    explicit LuaBindings(sol::state& lua);

    /// Register all bindings (standalone prototype version)
    void registerAll();

    /// Register math types (Vec2, Vec3, etc.)
    void registerMathTypes();

    /// Register component types
    void registerComponents();

    /// Register entity bindings with a registry
    void registerEntityBindings(entt::registry* registry);

    /// Get the registry
    entt::registry* getRegistry() { return registry_; }

private:
    sol::state& lua_;
    entt::registry* registry_ = nullptr;
    std::unique_ptr<entt::registry> ownedRegistry_;
};

}  // namespace bestow::runtime
