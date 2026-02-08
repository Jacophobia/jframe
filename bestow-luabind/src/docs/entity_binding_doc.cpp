// bestow-luabind/src/docs/entity_binding_doc.cpp
// API documentation for bestow.entity

module bestow.luabind;

import std;

namespace bestow {

void registerEntityDoc(DocRegistry& registry) {
    SystemDoc sys;
    sys.name = "entity";
    sys.qualifiedName = "bestow.entity";
    sys.description = "Entity-component system providing entity lifecycle management, component access by type name, field-level access, and type reflection.";

    // --- Enums ---

    sys.enums.push_back(EnumDoc{
        .name = "ComponentFieldType",
        .qualifiedName = "ComponentFieldType",
        .description = "Data types for component fields, used in reflection.",
        .values = {
            {"Unknown", "Unknown or unsupported type"},
            {"Bool", "Boolean value"},
            {"Int", "Integer value (64-bit)"},
            {"Float", "Single-precision float"},
            {"Double", "Double-precision float"},
            {"String", "String value"},
            {"Vec2", "2D vector (x, y)"},
            {"Vec3", "3D vector (x, y, z)"},
            {"Vec4", "4D vector (x, y, z, w)"},
            {"Quat", "Quaternion (x, y, z, w)"},
            {"Color", "RGBA color"},
            {"Entity", "Entity reference"},
            {"Handle", "Opaque handle (e.g., AssetHandle)"},
            {"Enum", "Enumeration value"},
            {"Struct", "Nested struct"},
            {"Array", "Array of values"},
        }
    });

    // --- Types ---

    sys.types.push_back(TypeDoc{
        .name = "ComponentFieldInfo",
        .qualifiedName = "ComponentFieldInfo",
        .description = "Reflection information about a single field within a component.",
        .fields = {
            {"name", "string", "Field name"},
            {"type", "ComponentFieldType", "Data type of the field"},
            {"offset", "number", "Byte offset within the component"},
            {"size", "number", "Size in bytes"},
            {"readOnly", "boolean", "Whether the field is read-only"},
            {"enumTypeName", "string", "Name of the enum type (if type is Enum)"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "ComponentTypeInfo",
        .qualifiedName = "ComponentTypeInfo",
        .description = "Reflection information about a registered component type.",
        .fields = {
            {"name", "string", "Component type name"},
            {"size", "number", "Size of the component in bytes"},
            {"fields", "ComponentFieldInfo[]", "Array of field descriptors"},
            {"canConstruct", "boolean", "Whether the component can be constructed from Lua"},
        },
    });

    // --- Entity Lifecycle ---

    sys.methods.push_back(MethodDoc{
        .name = "create",
        .qualifiedName = "bestow.entity.create",
        .description = "Create a new entity.",
        .returns = {{.type = "Entity", .description = "The newly created entity"}},
        .example = "local e = bestow.entity.create()",
        .seeAlso = {"bestow.entity.destroy"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "destroy",
        .qualifiedName = "bestow.entity.destroy",
        .description = "Destroy an entity and all its components.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity to destroy"},
        },
        .seeAlso = {"bestow.entity.create", "bestow.entity.isValid"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "isValid",
        .qualifiedName = "bestow.entity.isValid",
        .description = "Check if an entity is still valid (not destroyed).",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity to check"},
        },
        .returns = {{.type = "boolean", .description = "true if the entity exists and is valid"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "count",
        .qualifiedName = "bestow.entity.count",
        .description = "Get the total number of living entities.",
        .returns = {{.type = "number", .description = "Number of entities"}},
    });

    // --- Iteration ---

    sys.methods.push_back(MethodDoc{
        .name = "each",
        .qualifiedName = "bestow.entity.each",
        .description = "Iterate over all entities, calling a function for each one.",
        .params = {
            {.name = "callback", .type = "function", .description = "Function called with each Entity"},
        },
        .example = "bestow.entity.each(function(e)\n    print(e)\nend)",
    });

    // --- Component Type Registration ---

    sys.methods.push_back(MethodDoc{
        .name = "isTypeRegistered",
        .qualifiedName = "bestow.entity.isTypeRegistered",
        .description = "Check if a component type is registered for reflection.",
        .params = {
            {.name = "typeName", .type = "string", .description = "Component type name (e.g., 'Transform2D', 'Health')"},
        },
        .returns = {{.type = "boolean", .description = "true if the type is registered"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getRegisteredTypes",
        .qualifiedName = "bestow.entity.getRegisteredTypes",
        .description = "Get a list of all registered component type names.",
        .returns = {{.type = "string[]", .description = "Array of component type names"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getTypeInfo",
        .qualifiedName = "bestow.entity.getTypeInfo",
        .description = "Get reflection information about a registered component type.",
        .params = {
            {.name = "typeName", .type = "string", .description = "Component type name"},
        },
        .returns = {{.type = "ComponentTypeInfo|nil", .description = "Type info, or nil if not registered"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getFields",
        .qualifiedName = "bestow.entity.getFields",
        .description = "Get the field descriptors for a registered component type.",
        .params = {
            {.name = "typeName", .type = "string", .description = "Component type name"},
        },
        .returns = {{.type = "ComponentFieldInfo[]", .description = "Array of field descriptors"}},
    });

    // --- Component Access ---

    sys.methods.push_back(MethodDoc{
        .name = "addComponent",
        .qualifiedName = "bestow.entity.addComponent",
        .description = "Add a component to an entity by type name, optionally with initial data.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "typeName", .type = "string", .description = "Component type name (e.g., 'Transform2D')"},
            {.name = "data", .type = "table", .description = "Initial field values", .optional = true},
        },
        .returns = {{.type = "boolean", .description = "true if the component was added"}},
        .example = "bestow.entity.addComponent(e, \"Transform2D\", { x = 100, y = 200 })\nbestow.entity.addComponent(e, \"Health\", { current = 100, max = 100 })",
        .seeAlso = {"bestow.entity.removeComponent", "bestow.entity.hasComponent"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "removeComponent",
        .qualifiedName = "bestow.entity.removeComponent",
        .description = "Remove a component from an entity by type name.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "typeName", .type = "string", .description = "Component type name"},
        },
        .returns = {{.type = "boolean", .description = "true if the component was removed"}},
        .seeAlso = {"bestow.entity.addComponent"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "hasComponent",
        .qualifiedName = "bestow.entity.hasComponent",
        .description = "Check if an entity has a specific component type.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "typeName", .type = "string", .description = "Component type name"},
        },
        .returns = {{.type = "boolean", .description = "true if the entity has the component"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getComponent",
        .qualifiedName = "bestow.entity.getComponent",
        .description = "Get all field values of a component as a table.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "typeName", .type = "string", .description = "Component type name"},
        },
        .returns = {{.type = "table|nil", .description = "Table of field name/value pairs, or nil if not found"}},
        .example = "local transform = bestow.entity.getComponent(e, \"Transform2D\")\nif transform then\n    print(transform.x, transform.y)\nend",
    });

    sys.methods.push_back(MethodDoc{
        .name = "setComponent",
        .qualifiedName = "bestow.entity.setComponent",
        .description = "Set all fields of a component from a table.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "typeName", .type = "string", .description = "Component type name"},
            {.name = "data", .type = "table", .description = "Table of field name/value pairs to set"},
        },
        .returns = {{.type = "boolean", .description = "true if the component was updated"}},
    });

    // --- Single Field Access ---

    sys.methods.push_back(MethodDoc{
        .name = "getField",
        .qualifiedName = "bestow.entity.getField",
        .description = "Get a single field value from a component.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "typeName", .type = "string", .description = "Component type name"},
            {.name = "fieldName", .type = "string", .description = "Field name within the component"},
        },
        .returns = {{.type = "any|nil", .description = "Field value, or nil if not found"}},
        .example = "local x = bestow.entity.getField(e, \"Transform2D\", \"x\")",
    });

    sys.methods.push_back(MethodDoc{
        .name = "setField",
        .qualifiedName = "bestow.entity.setField",
        .description = "Set a single field value on a component.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "typeName", .type = "string", .description = "Component type name"},
            {.name = "fieldName", .type = "string", .description = "Field name within the component"},
            {.name = "value", .type = "any", .description = "New value for the field"},
        },
        .returns = {{.type = "boolean", .description = "true if the field was set"}},
        .example = "bestow.entity.setField(e, \"Transform2D\", \"x\", 200)",
    });

    registry.addSystem(std::move(sys));
}

} // namespace bestow
