// bestow-contract/src/bestow.entity.cppm
// Entity system interface

module;

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

// Use compatibility header for MSVC C++23 module support
#include <bestow/entt_compat.hpp>

export module bestow.entity;

import bestow.types;

export namespace bestow {

//==========================================================================
// Component Field Types (for Lua/Script Reflection)
//==========================================================================

/// Type of a component field, used for reflection
enum class ComponentFieldType : std::uint8_t {
    Unknown,
    Bool,
    Int,
    Float,
    Double,
    String,
    Vec2,
    Vec3,
    Vec4,
    Quat,
    Color,
    Entity,
    Handle,      // Generic handle type (uint64)
    Enum,        // Integer-backed enum
    Struct,      // Nested struct (not directly supported, use sub-fields)
    Array        // Array/vector (not directly supported yet)
};

/// Information about a single field in a component
struct ComponentFieldInfo {
    std::string name;
    ComponentFieldType type = ComponentFieldType::Unknown;
    std::size_t offset = 0;      // Byte offset within the component
    std::size_t size = 0;        // Size in bytes
    bool readOnly = false;       // If true, cannot be modified from Lua
    std::string enumTypeName;    // For Enum types, the enum's type name
};

/// Dynamic value container for component field values
/// Used for getting/setting component values by name at runtime
using ComponentFieldValue = std::variant<
    std::monostate,    // null/unset
    bool,
    std::int64_t,      // For all integer types
    double,            // For float/double
    std::string,
    Vec2,
    Vec3,
    Vec4,
    Quat,
    Color,
    Entity,
    std::uint64_t      // For handles
>;

/// A map of field name to value, representing a component's data
using ComponentData = std::unordered_map<std::string, ComponentFieldValue>;

/// Information about a registered component type
struct ComponentTypeInfo {
    std::string name;
    std::size_t size = 0;
    std::vector<ComponentFieldInfo> fields;
    bool canConstruct = false;   // Can be constructed from Lua
};

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

    //======================================================================
    // Reflection-Based Component Access (for Lua/Scripting)
    //======================================================================

    /// Register a component type for runtime reflection.
    /// Must be called before any ByName methods for that component.
    /// @param typeName The string name to use for this component
    /// @param typeInfo Information about the component's fields
    virtual void registerComponentType(std::string_view typeName,
                                        ComponentTypeInfo typeInfo) = 0;

    /// Unregister a component type from runtime reflection.
    virtual void unregisterComponentType(std::string_view typeName) = 0;

    /// Check if a component type is registered for reflection.
    virtual bool isComponentTypeRegistered(std::string_view typeName) const = 0;

    /// Get information about a registered component type.
    /// @return The type info if registered, nullopt otherwise
    virtual std::optional<ComponentTypeInfo> getComponentTypeInfo(
        std::string_view typeName) const = 0;

    /// Get list of all registered component type names.
    virtual std::vector<std::string> getRegisteredComponentTypes() const = 0;

    /// Add a component to an entity by type name.
    /// @param entity The entity to add the component to
    /// @param typeName The registered name of the component type
    /// @param data Initial field values (optional fields use defaults)
    /// @return true if component was added, false if type unknown or entity invalid
    virtual bool addComponentByName(Entity entity,
                                     std::string_view typeName,
                                     const ComponentData& data = {}) = 0;

    /// Remove a component from an entity by type name.
    /// @return true if component was removed, false if not present or type unknown
    virtual bool removeComponentByName(Entity entity,
                                        std::string_view typeName) = 0;

    /// Check if an entity has a component by type name.
    virtual bool hasComponentByName(Entity entity,
                                     std::string_view typeName) const = 0;

    /// Get a component's data by type name.
    /// @return The component's field values if present, nullopt otherwise
    virtual std::optional<ComponentData> getComponentByName(
        Entity entity,
        std::string_view typeName) const = 0;

    /// Set a component's field values by type name.
    /// Only updates fields present in data, leaves others unchanged.
    /// @return true if component was updated, false if not present or type unknown
    virtual bool setComponentByName(Entity entity,
                                     std::string_view typeName,
                                     const ComponentData& data) = 0;

    /// Get a single field value from a component.
    /// @return The field value if present, nullopt otherwise
    virtual std::optional<ComponentFieldValue> getComponentField(
        Entity entity,
        std::string_view typeName,
        std::string_view fieldName) const = 0;

    /// Set a single field value on a component.
    /// @return true if field was set, false if component/field not found
    virtual bool setComponentField(Entity entity,
                                    std::string_view typeName,
                                    std::string_view fieldName,
                                    const ComponentFieldValue& value) = 0;

    /// Get information about a component's fields.
    /// @return Field info list if type is registered, empty vector otherwise
    virtual std::vector<ComponentFieldInfo> getComponentFields(
        std::string_view typeName) const = 0;
};

}  // namespace bestow
