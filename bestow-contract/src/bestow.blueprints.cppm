// bestow-contract/src/bestow.blueprints.cppm
// Blueprint Factory interface for data-driven entity creation

module;

#include <string>
#include <memory>
#include <optional>
#include <functional>
#include <unordered_map>
#include <any>
#include <vector>

export module bestow.blueprints;

import bestow.types;
import bestow.entity;   // For IEntitySystem in ComponentCreator

export namespace bestow {

//==========================================================================
// Blueprint Definition Types
//==========================================================================

/// Property map for component data
using PropertyMap = std::unordered_map<std::string, std::any>;

/// Component definition within a blueprint
struct ComponentDef {
    std::string name;              // Component type name (e.g., "Health", "DebugRect")
    PropertyMap properties;        // Component properties
};

/// Physics configuration for a blueprint
struct BlueprintPhysicsDef {
    std::string bodyType = "dynamic";  // "static", "dynamic", "kinematic"
    std::optional<Vec2> size;          // Override size (uses component size if not set)
    bool sensor = false;
    bool fixedRotation = true;
    float density = 1.0f;
    float friction = 0.3f;
    float restitution = 0.0f;
    float linearDamping = 0.0f;
    std::string collisionLayer;        // Named collision layer
};

/// Complete blueprint definition
struct BlueprintDef {
    std::string name;                              // Blueprint identifier
    std::string inherits;                          // Parent blueprint name (optional)
    std::vector<ComponentDef> components;          // Components to add
    std::optional<BlueprintPhysicsDef> physics;    // Physics body configuration
    PropertyMap metadata;                          // Additional metadata
};

//==========================================================================
// Blueprint Factory Interface
//==========================================================================

class IBlueprintFactory {
public:
    virtual ~IBlueprintFactory() = default;

    //======================================================================
    // Blueprint Loading
    //======================================================================

    /// Load blueprint definitions from Lua source code
    /// Returns true if parsing succeeded
    virtual bool loadBlueprints(const std::string& luaSource) = 0;

    /// Reload blueprints from previously loaded source
    virtual void reloadBlueprints() = 0;

    /// Clear all loaded blueprints
    virtual void clearBlueprints() = 0;

    //======================================================================
    // Blueprint Queries
    //======================================================================

    /// Check if a blueprint exists
    virtual bool hasBlueprint(const std::string& name) const = 0;

    /// Get all blueprint names
    virtual std::vector<std::string> getBlueprintNames() const = 0;

    /// Get a blueprint definition (for inspection)
    virtual std::optional<BlueprintDef> getBlueprint(const std::string& name) const = 0;

    //======================================================================
    // Entity Creation
    //======================================================================

    /// Create an entity from a blueprint at a position
    virtual Entity create(const std::string& blueprintName, float x, float y) = 0;

    /// Create an entity from a blueprint with size
    virtual Entity create(const std::string& blueprintName,
                          float x, float y,
                          float width, float height) = 0;

    /// Create an entity from a blueprint with property overrides
    virtual Entity create(const std::string& blueprintName,
                          float x, float y,
                          const PropertyMap& overrides) = 0;

    /// Create an entity from a blueprint with size and overrides
    virtual Entity create(const std::string& blueprintName,
                          float x, float y,
                          float width, float height,
                          const PropertyMap& overrides) = 0;

    //======================================================================
    // Component Registration
    //======================================================================

    /// Register a component creator function
    /// The creator receives entity, entity system, and property map
    using ComponentCreator = std::function<void(Entity, IEntitySystem&, const PropertyMap&)>;
    virtual void registerComponent(const std::string& name, ComponentCreator creator) = 0;

    /// Check if a component type is registered
    virtual bool isComponentRegistered(const std::string& name) const = 0;
};

}  // namespace bestow
