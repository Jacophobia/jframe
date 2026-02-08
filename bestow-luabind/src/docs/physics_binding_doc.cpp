// bestow-luabind/src/docs/physics_binding_doc.cpp
// API documentation for bestow.physics (2D)

module bestow.luabind;

import std;

namespace bestow {

void registerPhysicsDoc(DocRegistry& registry) {
    SystemDoc sys;
    sys.name = "physics";
    sys.qualifiedName = "bestow.physics";
    sys.description = "2D physics system powered by Box2D. Provides rigid body creation, force application, collision filtering, raycasting, and ground detection.";

    // --- Enums ---

    sys.enums.push_back(EnumDoc{
        .name = "BodyType",
        .qualifiedName = "BodyType",
        .description = "Physics body simulation type.",
        .values = {
            {"Static", "Does not move. Used for walls, floors, and platforms."},
            {"Kinematic", "Moves via velocity only, not affected by forces or collisions."},
            {"Dynamic", "Fully simulated: affected by forces, gravity, and collisions."},
        }
    });

    // --- Types ---

    sys.types.push_back(TypeDoc{
        .name = "PhysicsBodyDef",
        .qualifiedName = "PhysicsBodyDef",
        .description = "Definition for creating a 2D physics body.",
        .fields = {
            {"type", "BodyType", "Body type (Static, Kinematic, Dynamic)"},
            {"transform", "Transform2D", "Initial position and rotation"},
            {"size", "Vec2", "Size of the body's collision shape"},
            {"fixedRotation", "boolean", "Prevent rotation (useful for character controllers)"},
            {"linearDamping", "number", "Linear velocity damping (0 = no damping)"},
            {"angularDamping", "number", "Angular velocity damping"},
            {"density", "number", "Mass density of the fixture"},
            {"friction", "number", "Surface friction (0.0 to 1.0)"},
            {"restitution", "number", "Bounciness (0.0 = no bounce, 1.0 = perfect bounce)"},
            {"isSensor", "boolean", "If true, detects overlaps but does not collide physically"},
        },
        .example = "local def = PhysicsBodyDef()\ndef.type = BodyType.Dynamic\ndef.transform = { x = 100, y = 200 }\ndef.size = Vec2(32, 64)\ndef.fixedRotation = true\nbestow.physics.createBody(entity, def)",
    });

    sys.types.push_back(TypeDoc{
        .name = "RaycastHit",
        .qualifiedName = "RaycastHit",
        .description = "Result of a 2D raycast query.",
        .fields = {
            {"entity", "Entity", "The entity that was hit"},
            {"point", "Vec2", "World position of the hit point"},
            {"normal", "Vec2", "Surface normal at the hit point"},
            {"distance", "number", "Distance from ray origin to hit point"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "CollisionEvent",
        .qualifiedName = "CollisionEvent",
        .description = "Data describing a collision between two physics bodies.",
        .fields = {
            {"entityA", "Entity", "First entity in the collision"},
            {"entityB", "Entity", "Second entity in the collision"},
            {"contactPoint", "Vec2", "World position of the contact"},
            {"normal", "Vec2", "Collision normal (from A to B)"},
            {"impulse", "number", "Collision impulse magnitude"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "GroundCheckParams",
        .qualifiedName = "GroundCheckParams",
        .description = "Parameters for ground detection queries.",
        .fields = {
            {"rayDistance", "number", "How far below the body to check"},
            {"slopeToleranceDeg", "number", "Maximum slope angle considered 'ground' (degrees)"},
            {"groundMask", "number", "Collision mask for what counts as ground"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "GroundCheckResult",
        .qualifiedName = "GroundCheckResult",
        .description = "Result of a ground detection check.",
        .fields = {
            {"grounded", "boolean", "Whether the entity is on the ground"},
            {"groundEntity", "Entity", "The entity that is the ground (if grounded)"},
            {"contactPoint", "Vec2", "Contact point with the ground"},
            {"surfaceNormal", "Vec2", "Surface normal of the ground"},
            {"slopeAngle", "number", "Angle of the slope in degrees"},
        },
    });

    // --- Body Management ---

    sys.methods.push_back(MethodDoc{
        .name = "createBody",
        .qualifiedName = "bestow.physics.createBody",
        .description = "Create a 2D physics body for an entity.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity to attach the body to"},
            {.name = "def", .type = "PhysicsBodyDef", .description = "Body definition"},
        },
        .seeAlso = {"bestow.physics.destroyBody"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "destroyBody",
        .qualifiedName = "bestow.physics.destroyBody",
        .description = "Destroy the physics body attached to an entity.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity whose body to destroy"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "hasBody",
        .qualifiedName = "bestow.physics.hasBody",
        .description = "Check if an entity has a physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity to check"},
        },
        .returns = {{.type = "boolean", .description = "true if the entity has a physics body"}},
    });

    // --- Body Properties ---

    sys.methods.push_back(MethodDoc{
        .name = "setBodyType",
        .qualifiedName = "bestow.physics.setBodyType",
        .description = "Change the body type of an entity's physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "type", .type = "BodyType", .description = "New body type"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "getBodyType",
        .qualifiedName = "bestow.physics.getBodyType",
        .description = "Get the body type of an entity's physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
        },
        .returns = {{.type = "BodyType", .description = "Current body type"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setPosition",
        .qualifiedName = "bestow.physics.setPosition",
        .description = "Teleport a physics body to a new position.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "position", .type = "Vec2", .description = "New world position"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "getPosition",
        .qualifiedName = "bestow.physics.getPosition",
        .description = "Get the current position of a physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
        },
        .returns = {{.type = "Vec2", .description = "Current world position"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setRotation",
        .qualifiedName = "bestow.physics.setRotation",
        .description = "Set the rotation of a physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "radians", .type = "number", .description = "Rotation angle in radians"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "getRotation",
        .qualifiedName = "bestow.physics.getRotation",
        .description = "Get the current rotation of a physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
        },
        .returns = {{.type = "number", .description = "Rotation in radians"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setVelocity",
        .qualifiedName = "bestow.physics.setVelocity",
        .description = "Set the linear velocity of a physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "velocity", .type = "Vec2", .description = "New linear velocity"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "getVelocity",
        .qualifiedName = "bestow.physics.getVelocity",
        .description = "Get the linear velocity of a physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
        },
        .returns = {{.type = "Vec2", .description = "Current linear velocity"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setAngularVelocity",
        .qualifiedName = "bestow.physics.setAngularVelocity",
        .description = "Set the angular velocity of a physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "velocity", .type = "number", .description = "Angular velocity in radians/second"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "getAngularVelocity",
        .qualifiedName = "bestow.physics.getAngularVelocity",
        .description = "Get the angular velocity of a physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
        },
        .returns = {{.type = "number", .description = "Angular velocity in radians/second"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getBodySize",
        .qualifiedName = "bestow.physics.getBodySize",
        .description = "Get the collision shape size of a physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
        },
        .returns = {{.type = "Vec2", .description = "Body size (width, height)"}},
    });

    // --- Forces ---

    sys.methods.push_back(MethodDoc{
        .name = "applyForce",
        .qualifiedName = "bestow.physics.applyForce",
        .description = "Apply a continuous force to a physics body. Optionally at a specific point.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "force", .type = "Vec2", .description = "Force vector in Newtons"},
            {.name = "point", .type = "Vec2", .description = "World point to apply force at", .optional = true},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "applyImpulse",
        .qualifiedName = "bestow.physics.applyImpulse",
        .description = "Apply an instantaneous impulse to a physics body. Optionally at a specific point.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "impulse", .type = "Vec2", .description = "Impulse vector"},
            {.name = "point", .type = "Vec2", .description = "World point to apply impulse at", .optional = true},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "applyTorque",
        .qualifiedName = "bestow.physics.applyTorque",
        .description = "Apply a rotational torque to a physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "torque", .type = "number", .description = "Torque value"},
        },
    });

    // --- Collision Filtering ---

    sys.methods.push_back(MethodDoc{
        .name = "setCollisionLayer",
        .qualifiedName = "bestow.physics.setCollisionLayer",
        .description = "Set the collision layer of a physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "layer", .type = "number", .description = "Collision layer bitmask"},
        },
        .seeAlso = {"bestow.physics.setCollisionMask"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getCollisionLayer",
        .qualifiedName = "bestow.physics.getCollisionLayer",
        .description = "Get the collision layer of a physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
        },
        .returns = {{.type = "number", .description = "Collision layer bitmask"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setCollisionMask",
        .qualifiedName = "bestow.physics.setCollisionMask",
        .description = "Set the collision mask (which layers this body collides with).",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "mask", .type = "number", .description = "Collision mask bitmask"},
        },
        .seeAlso = {"bestow.physics.setCollisionLayer"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setSensor",
        .qualifiedName = "bestow.physics.setSensor",
        .description = "Set whether a physics body is a sensor (detects overlaps without physical collision).",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "isSensor", .type = "boolean", .description = "true to make it a sensor"},
        },
    });

    // --- Queries ---

    sys.methods.push_back(MethodDoc{
        .name = "queryAABB",
        .qualifiedName = "bestow.physics.queryAABB",
        .description = "Find all entities with physics bodies overlapping an axis-aligned bounding box.",
        .params = {
            {.name = "min", .type = "Vec2", .description = "Minimum corner of the AABB"},
            {.name = "max", .type = "Vec2", .description = "Maximum corner of the AABB"},
        },
        .returns = {{.type = "Entity[]", .description = "Array of entities in the region"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "queryCircle",
        .qualifiedName = "bestow.physics.queryCircle",
        .description = "Find all entities with physics bodies overlapping a circle.",
        .params = {
            {.name = "center", .type = "Vec2", .description = "Center of the circle"},
            {.name = "radius", .type = "number", .description = "Radius of the circle"},
        },
        .returns = {{.type = "Entity[]", .description = "Array of entities in the circle"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "raycast",
        .qualifiedName = "bestow.physics.raycast",
        .description = "Cast a ray and return the first hit, or nil if nothing was hit.",
        .params = {
            {.name = "origin", .type = "Vec2", .description = "Ray origin point"},
            {.name = "direction", .type = "Vec2", .description = "Ray direction (does not need to be normalized)"},
            {.name = "maxDistance", .type = "number", .description = "Maximum ray distance"},
            {.name = "mask", .type = "number", .description = "Collision mask filter", .optional = true},
        },
        .returns = {{.type = "RaycastHit|nil", .description = "Hit result or nil if no hit"}},
        .seeAlso = {"bestow.physics.raycastAll"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "raycastAll",
        .qualifiedName = "bestow.physics.raycastAll",
        .description = "Cast a ray and return all hits along the ray.",
        .params = {
            {.name = "origin", .type = "Vec2", .description = "Ray origin point"},
            {.name = "direction", .type = "Vec2", .description = "Ray direction"},
            {.name = "maxDistance", .type = "number", .description = "Maximum ray distance"},
            {.name = "mask", .type = "number", .description = "Collision mask filter", .optional = true},
        },
        .returns = {{.type = "RaycastHit[]", .description = "Array of all hits along the ray"}},
        .seeAlso = {"bestow.physics.raycast"},
    });

    // --- World Settings ---

    sys.methods.push_back(MethodDoc{
        .name = "setGravity",
        .qualifiedName = "bestow.physics.setGravity",
        .description = "Set the world gravity vector.",
        .params = {
            {.name = "gravity", .type = "Vec2", .description = "Gravity vector (e.g., Vec2(0, -980) for downward gravity)"},
        },
        .seeAlso = {"bestow.physics.getGravity"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getGravity",
        .qualifiedName = "bestow.physics.getGravity",
        .description = "Get the current world gravity vector.",
        .returns = {{.type = "Vec2", .description = "Current gravity vector"}},
        .seeAlso = {"bestow.physics.setGravity"},
    });

    // --- Ground Detection ---

    sys.methods.push_back(MethodDoc{
        .name = "checkGrounded",
        .qualifiedName = "bestow.physics.checkGrounded",
        .description = "Check if an entity is on the ground. Useful for platformer jump mechanics.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity to check"},
            {.name = "params", .type = "GroundCheckParams", .description = "Custom ground check parameters", .optional = true},
        },
        .returns = {{.type = "GroundCheckResult", .description = "Ground check result with grounded state, contact info, and slope angle"}},
        .example = "local result = bestow.physics.checkGrounded(playerEntity)\nif result.grounded then\n    -- Can jump\nend",
    });

    // --- Properties ---

    sys.properties.push_back(PropertyDoc{
        .name = "Layer",
        .type = "table",
        .description = "Predefined collision layer constants: Player, Enemy, Projectile, Terrain, Trigger, Collectible, Ground.",
        .readOnly = true,
    });

    registry.addSystem(std::move(sys));
}

} // namespace bestow
