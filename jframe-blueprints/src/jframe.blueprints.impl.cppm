// jframe-blueprints/src/jframe.blueprints.impl.cppm
// Blueprint Factory implementation module

module;

#include <sol/sol.hpp>

export module jframe.blueprints.impl;

import std;
import jframe.types;
import jframe.blueprints;
import jframe.entity;
import jframe.physics;

export namespace jframe {

class BlueprintFactory : public IBlueprintFactory {
public:
    BlueprintFactory(IEntitySystem& entities, IPhysicsSystem* physics = nullptr);
    ~BlueprintFactory() override = default;

    //======================================================================
    // Blueprint Loading
    //======================================================================

    bool loadBlueprints(const std::string& luaSource) override;
    void reloadBlueprints() override;
    void clearBlueprints() override;

    //======================================================================
    // Blueprint Queries
    //======================================================================

    bool hasBlueprint(const std::string& name) const override;
    std::vector<std::string> getBlueprintNames() const override;
    std::optional<BlueprintDef> getBlueprint(const std::string& name) const override;

    //======================================================================
    // Entity Creation
    //======================================================================

    Entity create(const std::string& blueprintName, float x, float y) override;
    Entity create(const std::string& blueprintName,
                  float x, float y,
                  float width, float height) override;
    Entity create(const std::string& blueprintName,
                  float x, float y,
                  const PropertyMap& overrides) override;
    Entity create(const std::string& blueprintName,
                  float x, float y,
                  float width, float height,
                  const PropertyMap& overrides) override;

    //======================================================================
    // Component Registration
    //======================================================================

    void registerComponent(const std::string& name, ComponentCreator creator) override;
    bool isComponentRegistered(const std::string& name) const override;

private:
    IEntitySystem& entities_;
    IPhysicsSystem* physics_;
    std::string lastLuaSource_;
    std::unordered_map<std::string, BlueprintDef> blueprints_;
    std::unordered_map<std::string, ComponentCreator> componentCreators_;

    // Parsing helpers
    void parseBlueprintTable(const sol::table& blueprintTable, BlueprintDef& def);
    void parseComponentsTable(const sol::table& componentsTable, BlueprintDef& def);
    void parsePhysicsTable(const sol::table& physicsTable, BlueprintDef& def);
    PropertyMap parsePropertyTable(const sol::table& table);

    // Blueprint resolution
    BlueprintDef resolveInheritance(const BlueprintDef& def) const;
    void mergeBlueprints(BlueprintDef& base, const BlueprintDef& override) const;
    void mergeProperties(PropertyMap& base, const PropertyMap& override) const;

    // Entity creation helpers
    Entity createEntityFromBlueprint(const BlueprintDef& def,
                                      float x, float y,
                                      float width, float height,
                                      const PropertyMap& overrides);
    void applyComponents(Entity entity, const BlueprintDef& def, const PropertyMap& overrides);
    void applyPhysics(Entity entity, const BlueprintDef& def, float x, float y, float width, float height);
    void applyNestedOverride(PropertyMap& props, const std::string& path, const std::any& value);

    // Built-in component registration
    void registerBuiltinComponents();
};

// Factory function
inline std::unique_ptr<IBlueprintFactory> createBlueprintFactory(
    IEntitySystem& entities,
    IPhysicsSystem* physics = nullptr)
{
    return std::make_unique<BlueprintFactory>(entities, physics);
}

}  // namespace jframe
