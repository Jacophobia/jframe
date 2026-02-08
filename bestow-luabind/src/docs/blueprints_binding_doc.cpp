// bestow-luabind/src/docs/blueprints_binding_doc.cpp
// API documentation for bestow.blueprints

module bestow.luabind;

import std;

namespace bestow {

void registerBlueprintsDoc(DocRegistry& registry) {
    SystemDoc sys;
    sys.name = "blueprints";
    sys.qualifiedName = "bestow.blueprints";
    sys.description = "Blueprint factory system for data-driven entity creation. Load reusable entity templates defined in Lua and spawn entities from them with optional property overrides.";

    //=========================================================================
    // Types
    //=========================================================================

    sys.types.push_back(TypeDoc{
        .name = "ComponentDef",
        .qualifiedName = "ComponentDef",
        .description = "Defines a single component within a blueprint, including its type name and property values.",
        .fields = {
            {.name = "name", .type = "string", .description = "Component type name (e.g., \"Health\", \"DebugRect\")"},
            {.name = "properties", .type = "table", .description = "Key-value map of component properties"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "BlueprintPhysicsDef",
        .qualifiedName = "BlueprintPhysicsDef",
        .description = "Physics body configuration for a blueprint. Controls how the spawned entity interacts with the physics simulation.",
        .fields = {
            {.name = "bodyType", .type = "string", .description = "Physics body type: \"static\", \"dynamic\", or \"kinematic\""},
            {.name = "size", .type = "Vec2", .description = "Override collision size (uses component size if not set)"},
            {.name = "sensor", .type = "boolean", .description = "If true, detects overlaps but does not collide physically"},
            {.name = "fixedRotation", .type = "boolean", .description = "If true, prevents the body from rotating"},
            {.name = "density", .type = "number", .description = "Body density (affects mass)"},
            {.name = "friction", .type = "number", .description = "Surface friction coefficient"},
            {.name = "restitution", .type = "number", .description = "Bounciness (0 = no bounce, 1 = full bounce)"},
            {.name = "linearDamping", .type = "number", .description = "Linear velocity damping"},
            {.name = "collisionLayer", .type = "string", .description = "Named collision layer for filtering"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "BlueprintDef",
        .qualifiedName = "BlueprintDef",
        .description = "Complete blueprint definition describing an entity template with components, physics, and metadata.",
        .fields = {
            {.name = "name", .type = "string", .description = "Blueprint identifier used for spawning"},
            {.name = "inherits", .type = "string", .description = "Parent blueprint name for inheritance (optional)"},
            {.name = "components", .type = "ComponentDef[]", .description = "Array of component definitions to add to the entity"},
            {.name = "physics", .type = "BlueprintPhysicsDef", .description = "Physics body configuration (optional)"},
            {.name = "metadata", .type = "table", .description = "Additional metadata key-value pairs"},
        },
    });

    //=========================================================================
    // Blueprint Loading
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "loadBlueprints",
        .qualifiedName = "bestow.blueprints.loadBlueprints",
        .description = "Load blueprint definitions from Lua source code. Parses the source and registers all blueprints found within it.",
        .params = {
            {.name = "luaSource", .type = "string", .description = "Lua source code containing blueprint definitions"},
        },
        .returns = {{.type = "boolean", .description = "true if parsing succeeded"}},
        .example = "local ok = bestow.blueprints.loadBlueprints([[\n  return {\n    Player = {\n      components = {\n        { name = \"Health\", properties = { max = 100 } },\n      },\n    },\n  }\n]])",
    });

    sys.methods.push_back(MethodDoc{
        .name = "reloadBlueprints",
        .qualifiedName = "bestow.blueprints.reloadBlueprints",
        .description = "Reload blueprints from previously loaded source. Useful for hot-reload scenarios.",
    });

    sys.methods.push_back(MethodDoc{
        .name = "clearBlueprints",
        .qualifiedName = "bestow.blueprints.clearBlueprints",
        .description = "Clear all loaded blueprints from the registry.",
    });

    //=========================================================================
    // Blueprint Queries
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "hasBlueprint",
        .qualifiedName = "bestow.blueprints.hasBlueprint",
        .description = "Check if a blueprint with the given name exists in the registry.",
        .params = {
            {.name = "name", .type = "string", .description = "Blueprint name to look up"},
        },
        .returns = {{.type = "boolean", .description = "true if the blueprint exists"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getBlueprintNames",
        .qualifiedName = "bestow.blueprints.getBlueprintNames",
        .description = "Get all registered blueprint names.",
        .returns = {{.type = "string[]", .description = "Array of blueprint names"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getBlueprint",
        .qualifiedName = "bestow.blueprints.getBlueprint",
        .description = "Get a blueprint definition by name for inspection.",
        .params = {
            {.name = "name", .type = "string", .description = "Blueprint name to retrieve"},
        },
        .returns = {{.type = "BlueprintDef|nil", .description = "The blueprint definition, or nil if not found"}},
    });

    //=========================================================================
    // Entity Creation
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "create",
        .qualifiedName = "bestow.blueprints.create",
        .description = "Create an entity from a blueprint at a given position. Optionally specify size and/or property overrides.",
        .params = {
            {.name = "blueprintName", .type = "string", .description = "Name of the blueprint to instantiate"},
            {.name = "x", .type = "number", .description = "X position for the new entity"},
            {.name = "y", .type = "number", .description = "Y position for the new entity"},
            {.name = "width", .type = "number", .description = "Width override for the entity", .optional = true},
            {.name = "height", .type = "number", .description = "Height override for the entity", .optional = true},
            {.name = "overrides", .type = "table", .description = "Property overrides applied to components", .optional = true},
        },
        .returns = {{.type = "Entity", .description = "The newly created entity"}},
        .example = "-- Simple spawn at position\nlocal enemy = bestow.blueprints.create(\"Goblin\", 200, 300)\n\n-- Spawn with size\nlocal wall = bestow.blueprints.create(\"Wall\", 0, 0, 100, 50)\n\n-- Spawn with overrides\nlocal boss = bestow.blueprints.create(\"Goblin\", 400, 300, { Health = { max = 500 } })",
    });

    //=========================================================================
    // Component Registration
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "registerComponent",
        .qualifiedName = "bestow.blueprints.registerComponent",
        .description = "Register a component creator function. The creator receives the entity, entity system, and property map when a blueprint containing this component is instantiated.",
        .params = {
            {.name = "name", .type = "string", .description = "Component type name (e.g., \"Health\")"},
            {.name = "creator", .type = "function", .description = "Creator function(entity, entitySystem, properties)"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "isComponentRegistered",
        .qualifiedName = "bestow.blueprints.isComponentRegistered",
        .description = "Check if a component type has a registered creator function.",
        .params = {
            {.name = "name", .type = "string", .description = "Component type name to check"},
        },
        .returns = {{.type = "boolean", .description = "true if the component type is registered"}},
    });

    registry.addSystem(std::move(sys));
}

} // namespace bestow
