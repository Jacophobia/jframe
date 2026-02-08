// bestow-luabind/src/docs/physics3d_binding_doc.cpp
// API documentation for bestow.physics3d

module bestow.luabind;

import std;

namespace bestow {

void registerPhysics3DDoc(DocRegistry& registry) {
    SystemDoc sys;
    sys.name = "physics3d";
    sys.qualifiedName = "bestow.physics3d";
    sys.description = "3D physics system providing rigid body simulation, raycasting, shape casting, overlap queries, character controllers, collision callbacks, and debug visualization.";

    // --- Enums ---

    sys.enums.push_back(EnumDoc{
        .name = "Physics3DError",
        .qualifiedName = "Physics3DError",
        .description = "Error codes returned by 3D physics operations.",
        .values = {
            {"Success", "Operation completed successfully"},
            {"InvalidEntity", "The entity is invalid or null"},
            {"BodyNotFound", "No physics body exists for the entity"},
            {"InvalidShape", "The shape definition is invalid"},
            {"ConstraintNotFound", "The constraint was not found"},
            {"CharacterNotFound", "No character controller exists for the entity"},
            {"VehicleNotFound", "No vehicle exists for the entity"},
            {"InvalidConfiguration", "The configuration parameters are invalid"},
            {"OutOfMemory", "Out of memory"},
            {"InternalError", "An internal physics engine error occurred"},
        }
    });

    sys.enums.push_back(EnumDoc{
        .name = "BodyType3D",
        .qualifiedName = "BodyType3D",
        .description = "3D physics body simulation type.",
        .values = {
            {"Static", "Does not move. Used for terrain, buildings, and static geometry."},
            {"Kinematic", "Moves via velocity only, not affected by forces or collisions."},
            {"Dynamic", "Fully simulated: affected by forces, gravity, and collisions."},
        }
    });

    sys.enums.push_back(EnumDoc{
        .name = "ShapeType3D",
        .qualifiedName = "ShapeType3D",
        .description = "Types of 3D collision shapes.",
        .values = {
            {"Box", "Axis-aligned or oriented box"},
            {"Sphere", "Sphere defined by radius"},
            {"Capsule", "Capsule defined by radius and half-height"},
            {"Cylinder", "Cylinder defined by radius and half-height"},
            {"Mesh", "Triangle mesh (static bodies only)"},
            {"ConvexHull", "Convex hull from a set of points"},
            {"HeightField", "Height field for terrain"},
            {"Compound", "Multiple shapes combined into one body"},
        }
    });

    sys.enums.push_back(EnumDoc{
        .name = "MotionQuality",
        .qualifiedName = "MotionQuality",
        .description = "Motion quality settings for physics simulation accuracy.",
        .values = {
            {"Discrete", "Standard discrete collision detection (faster)"},
            {"LinearCast", "Continuous collision detection using linear casts (prevents tunneling)"},
        }
    });

    sys.enums.push_back(EnumDoc{
        .name = "CharacterGroundState",
        .qualifiedName = "CharacterGroundState",
        .description = "Ground state of a character controller.",
        .values = {
            {"OnGround", "Character is on walkable ground"},
            {"OnSteepGround", "Character is on a slope steeper than the max slope angle"},
            {"InAir", "Character is airborne"},
            {"Sliding", "Character is sliding down a steep surface"},
        }
    });

    // --- Types ---

    sys.types.push_back(TypeDoc{
        .name = "PhysicsBodyDef3D",
        .qualifiedName = "PhysicsBodyDef3D",
        .description = "Definition for creating a 3D physics body.",
        .fields = {
            {"type", "BodyType3D", "Body type (Static, Kinematic, Dynamic)"},
            {"transform", "Transform3D", "Initial position, rotation, and scale"},
            {"shapeType", "ShapeType3D", "Type of collision shape"},
            {"shapeHalfExtents", "Vec3", "Half extents for Box shape"},
            {"shapeRadius", "number", "Radius for Sphere/Capsule/Cylinder shapes"},
            {"shapeHalfHeight", "number", "Half height for Capsule/Cylinder shapes"},
            {"density", "number", "Mass density"},
            {"friction", "number", "Surface friction (0.0 to 1.0)"},
            {"restitution", "number", "Bounciness (0.0 to 1.0)"},
            {"linearDamping", "number", "Linear velocity damping"},
            {"angularDamping", "number", "Angular velocity damping"},
            {"gravityFactor", "number", "Gravity multiplier (1.0 = normal, 0.0 = no gravity)"},
            {"allowSleep", "boolean", "Allow the body to sleep when at rest"},
            {"layer", "number", "Collision layer"},
            {"mask", "number", "Collision mask"},
            {"isSensor", "boolean", "If true, detects overlaps without physical collision"},
            {"motionQuality", "MotionQuality", "Discrete or LinearCast motion quality"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "QueryFilter3D",
        .qualifiedName = "QueryFilter3D",
        .description = "Filter for 3D physics queries (raycast, overlap, shape cast).",
        .fields = {
            {"layerMask", "number", "Collision layer mask to filter results"},
            {"ignoreSensors", "boolean", "If true, ignore sensor bodies in results"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "RaycastHit3D",
        .qualifiedName = "RaycastHit3D",
        .description = "Result of a 3D raycast query.",
        .fields = {
            {"entity", "Entity", "The entity that was hit"},
            {"point", "Vec3", "World position of the hit point"},
            {"normal", "Vec3", "Surface normal at the hit point"},
            {"distance", "number", "Distance from ray origin to hit point"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "ShapeCastHit3D",
        .qualifiedName = "ShapeCastHit3D",
        .description = "Result of a 3D shape cast (sphere cast, box cast).",
        .fields = {
            {"entity", "Entity", "The entity that was hit"},
            {"point", "Vec3", "World position of the hit point"},
            {"normal", "Vec3", "Surface normal at the hit point"},
            {"distance", "number", "Distance from cast origin to hit"},
            {"penetrationDepth", "number", "Depth of penetration at contact"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "CharacterControllerDef",
        .qualifiedName = "CharacterControllerDef",
        .description = "Definition for creating a character controller.",
        .fields = {
            {"radius", "number", "Capsule radius"},
            {"height", "number", "Capsule height (total, not half)"},
            {"stepHeight", "number", "Maximum step height the character can climb"},
            {"maxSlopeAngle", "number", "Maximum walkable slope angle in degrees"},
            {"mass", "number", "Character mass"},
            {"layer", "number", "Collision layer"},
            {"mask", "number", "Collision mask"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "CharacterGroundInfo",
        .qualifiedName = "CharacterGroundInfo",
        .description = "Ground information for a character controller.",
        .fields = {
            {"state", "CharacterGroundState", "Current ground state"},
            {"groundEntity", "Entity", "Entity of the ground surface (if on ground)"},
            {"groundNormal", "Vec3", "Normal of the ground surface"},
            {"groundPoint", "Vec3", "Contact point with the ground"},
            {"slopeAngle", "number", "Slope angle in degrees"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "PhysicsStats3D",
        .qualifiedName = "PhysicsStats3D",
        .description = "Performance statistics for the 3D physics simulation.",
        .fields = {
            {"activeBodies", "number", "Number of active (non-sleeping) bodies"},
            {"sleepingBodies", "number", "Number of sleeping bodies"},
            {"constraints", "number", "Number of active constraints"},
            {"characters", "number", "Number of character controllers"},
            {"vehicles", "number", "Number of vehicles"},
            {"updateTimeMs", "number", "Time spent on physics update in milliseconds"},
            {"collisionPairs", "number", "Number of collision pairs detected"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "CollisionEvent3D",
        .qualifiedName = "CollisionEvent3D",
        .description = "Data describing a 3D collision between two physics bodies.",
        .fields = {
            {"entityA", "Entity", "First entity in the collision"},
            {"entityB", "Entity", "Second entity in the collision"},
            {"contactPoint", "Vec3", "World position of the contact"},
            {"contactNormal", "Vec3", "Collision normal"},
            {"impulse", "number", "Collision impulse magnitude"},
            {"penetrationDepth", "number", "Penetration depth at contact"},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "TriggerEvent3D",
        .qualifiedName = "TriggerEvent3D",
        .description = "Data describing a trigger overlap event.",
        .fields = {
            {"entityA", "Entity", "First entity (the trigger or the other body)"},
            {"entityB", "Entity", "Second entity"},
        },
    });

    // --- Body Management ---

    sys.methods.push_back(MethodDoc{
        .name = "createBody",
        .qualifiedName = "bestow.physics3d.createBody",
        .description = "Create a 3D physics body for an entity. Returns true on success, false on failure.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity to attach the body to"},
            {.name = "def", .type = "PhysicsBodyDef3D", .description = "Body definition"},
        },
        .returns = {{.type = "boolean", .description = "true if the body was created successfully"}},
        .seeAlso = {"bestow.physics3d.destroyBody"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "destroyBody",
        .qualifiedName = "bestow.physics3d.destroyBody",
        .description = "Destroy the 3D physics body attached to an entity.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity whose body to destroy"},
        },
        .returns = {{.type = "boolean", .description = "true if the body was destroyed successfully"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "hasBody",
        .qualifiedName = "bestow.physics3d.hasBody",
        .description = "Check if an entity has a 3D physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity to check"},
        },
        .returns = {{.type = "boolean", .description = "true if the entity has a physics body"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getAllBodies",
        .qualifiedName = "bestow.physics3d.getAllBodies",
        .description = "Get all entities that have 3D physics bodies.",
        .returns = {{.type = "Entity[]", .description = "Array of entities with physics bodies"}},
    });

    // --- Body Type ---

    sys.methods.push_back(MethodDoc{
        .name = "setBodyType",
        .qualifiedName = "bestow.physics3d.setBodyType",
        .description = "Change the body type of an entity's 3D physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "type", .type = "BodyType3D", .description = "New body type"},
        },
        .returns = {{.type = "boolean", .description = "true on success"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getBodyType",
        .qualifiedName = "bestow.physics3d.getBodyType",
        .description = "Get the body type of an entity's 3D physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
        },
        .returns = {{.type = "BodyType3D|nil", .description = "Body type, or nil if no body"}},
    });

    // --- Transform ---

    sys.methods.push_back(MethodDoc{
        .name = "setTransform",
        .qualifiedName = "bestow.physics3d.setTransform",
        .description = "Set the full transform (position + rotation) of a 3D physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "transform", .type = "Transform3D", .description = "New transform"},
        },
        .returns = {{.type = "boolean", .description = "true on success"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getTransform",
        .qualifiedName = "bestow.physics3d.getTransform",
        .description = "Get the current transform of a 3D physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
        },
        .returns = {{.type = "Transform3D|nil", .description = "Current transform, or nil if no body"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setPosition",
        .qualifiedName = "bestow.physics3d.setPosition",
        .description = "Teleport a 3D physics body to a new position.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "position", .type = "Vec3", .description = "New world position"},
        },
        .returns = {{.type = "boolean", .description = "true on success"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getPosition",
        .qualifiedName = "bestow.physics3d.getPosition",
        .description = "Get the current position of a 3D physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
        },
        .returns = {{.type = "Vec3|nil", .description = "Current world position, or nil if no body"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setRotation",
        .qualifiedName = "bestow.physics3d.setRotation",
        .description = "Set the rotation of a 3D physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "rotation", .type = "Quat", .description = "New rotation quaternion"},
        },
        .returns = {{.type = "boolean", .description = "true on success"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getRotation",
        .qualifiedName = "bestow.physics3d.getRotation",
        .description = "Get the current rotation of a 3D physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
        },
        .returns = {{.type = "Quat|nil", .description = "Current rotation quaternion, or nil if no body"}},
    });

    // --- Velocity ---

    sys.methods.push_back(MethodDoc{
        .name = "setLinearVelocity",
        .qualifiedName = "bestow.physics3d.setLinearVelocity",
        .description = "Set the linear velocity of a 3D physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "velocity", .type = "Vec3", .description = "New linear velocity"},
        },
        .returns = {{.type = "boolean", .description = "true on success"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getLinearVelocity",
        .qualifiedName = "bestow.physics3d.getLinearVelocity",
        .description = "Get the linear velocity of a 3D physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
        },
        .returns = {{.type = "Vec3|nil", .description = "Current linear velocity, or nil if no body"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setAngularVelocity",
        .qualifiedName = "bestow.physics3d.setAngularVelocity",
        .description = "Set the angular velocity of a 3D physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "velocity", .type = "Vec3", .description = "New angular velocity (axis * angular speed)"},
        },
        .returns = {{.type = "boolean", .description = "true on success"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getAngularVelocity",
        .qualifiedName = "bestow.physics3d.getAngularVelocity",
        .description = "Get the angular velocity of a 3D physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
        },
        .returns = {{.type = "Vec3|nil", .description = "Current angular velocity, or nil if no body"}},
    });

    // --- Forces ---

    sys.methods.push_back(MethodDoc{
        .name = "applyForce",
        .qualifiedName = "bestow.physics3d.applyForce",
        .description = "Apply a continuous force to a 3D physics body at its center of mass.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "force", .type = "Vec3", .description = "Force vector in Newtons"},
        },
        .returns = {{.type = "boolean", .description = "true on success"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "applyForceAtPoint",
        .qualifiedName = "bestow.physics3d.applyForceAtPoint",
        .description = "Apply a continuous force to a 3D physics body at a specific world point.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "force", .type = "Vec3", .description = "Force vector in Newtons"},
            {.name = "point", .type = "Vec3", .description = "World point to apply force at"},
        },
        .returns = {{.type = "boolean", .description = "true on success"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "applyTorque",
        .qualifiedName = "bestow.physics3d.applyTorque",
        .description = "Apply a rotational torque to a 3D physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "torque", .type = "Vec3", .description = "Torque vector"},
        },
        .returns = {{.type = "boolean", .description = "true on success"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "applyImpulse",
        .qualifiedName = "bestow.physics3d.applyImpulse",
        .description = "Apply an instantaneous impulse to a 3D physics body at its center of mass.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "impulse", .type = "Vec3", .description = "Impulse vector"},
        },
        .returns = {{.type = "boolean", .description = "true on success"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "applyImpulseAtPoint",
        .qualifiedName = "bestow.physics3d.applyImpulseAtPoint",
        .description = "Apply an instantaneous impulse at a specific world point.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "impulse", .type = "Vec3", .description = "Impulse vector"},
            {.name = "point", .type = "Vec3", .description = "World point to apply impulse at"},
        },
        .returns = {{.type = "boolean", .description = "true on success"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "applyAngularImpulse",
        .qualifiedName = "bestow.physics3d.applyAngularImpulse",
        .description = "Apply an angular impulse to a 3D physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "impulse", .type = "Vec3", .description = "Angular impulse vector"},
        },
        .returns = {{.type = "boolean", .description = "true on success"}},
    });

    // --- Body Properties ---

    sys.methods.push_back(MethodDoc{
        .name = "setMass",
        .qualifiedName = "bestow.physics3d.setMass",
        .description = "Set the mass of a 3D physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "mass", .type = "number", .description = "New mass in kilograms"},
        },
        .returns = {{.type = "boolean", .description = "true on success"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getMass",
        .qualifiedName = "bestow.physics3d.getMass",
        .description = "Get the mass of a 3D physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
        },
        .returns = {{.type = "number|nil", .description = "Mass in kilograms, or nil if no body"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setLinearDamping",
        .qualifiedName = "bestow.physics3d.setLinearDamping",
        .description = "Set the linear damping of a 3D physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "damping", .type = "number", .description = "Linear damping factor"},
        },
        .returns = {{.type = "boolean", .description = "true on success"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setAngularDamping",
        .qualifiedName = "bestow.physics3d.setAngularDamping",
        .description = "Set the angular damping of a 3D physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "damping", .type = "number", .description = "Angular damping factor"},
        },
        .returns = {{.type = "boolean", .description = "true on success"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setGravityFactor",
        .qualifiedName = "bestow.physics3d.setGravityFactor",
        .description = "Set the gravity multiplier for a 3D physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "factor", .type = "number", .description = "Gravity factor (1.0 = normal, 0.0 = no gravity)"},
        },
        .returns = {{.type = "boolean", .description = "true on success"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setFriction",
        .qualifiedName = "bestow.physics3d.setFriction",
        .description = "Set the friction coefficient of a 3D physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "friction", .type = "number", .description = "Friction coefficient (0.0 to 1.0)"},
        },
        .returns = {{.type = "boolean", .description = "true on success"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setRestitution",
        .qualifiedName = "bestow.physics3d.setRestitution",
        .description = "Set the restitution (bounciness) of a 3D physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "restitution", .type = "number", .description = "Restitution (0.0 = no bounce, 1.0 = perfect bounce)"},
        },
        .returns = {{.type = "boolean", .description = "true on success"}},
    });

    // --- Collision Filtering ---

    sys.methods.push_back(MethodDoc{
        .name = "setCollisionLayer",
        .qualifiedName = "bestow.physics3d.setCollisionLayer",
        .description = "Set the collision layer of a 3D physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "layer", .type = "number", .description = "Collision layer"},
        },
        .returns = {{.type = "boolean", .description = "true on success"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setCollisionMask",
        .qualifiedName = "bestow.physics3d.setCollisionMask",
        .description = "Set the collision mask (which layers this body interacts with).",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "mask", .type = "number", .description = "Collision mask bitmask"},
        },
        .returns = {{.type = "boolean", .description = "true on success"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setSensor",
        .qualifiedName = "bestow.physics3d.setSensor",
        .description = "Set whether a 3D physics body is a sensor.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "isSensor", .type = "boolean", .description = "true to make it a sensor"},
        },
        .returns = {{.type = "boolean", .description = "true on success"}},
    });

    // --- Sleep State ---

    sys.methods.push_back(MethodDoc{
        .name = "isAwake",
        .qualifiedName = "bestow.physics3d.isAwake",
        .description = "Check if a 3D physics body is awake (not sleeping).",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
        },
        .returns = {{.type = "boolean|nil", .description = "true if awake, or nil if no body"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "wakeUp",
        .qualifiedName = "bestow.physics3d.wakeUp",
        .description = "Wake up a sleeping 3D physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
        },
        .returns = {{.type = "boolean", .description = "true on success"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "putToSleep",
        .qualifiedName = "bestow.physics3d.putToSleep",
        .description = "Put a 3D physics body to sleep.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
        },
        .returns = {{.type = "boolean", .description = "true on success"}},
    });

    // --- Bounding Box ---

    sys.methods.push_back(MethodDoc{
        .name = "getBodyBounds",
        .qualifiedName = "bestow.physics3d.getBodyBounds",
        .description = "Get the axis-aligned bounding box of a 3D physics body.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
        },
        .returns = {{.type = "AABB|nil", .description = "Bounding box, or nil if no body"}},
    });

    // --- Raycasting ---

    sys.methods.push_back(MethodDoc{
        .name = "raycast",
        .qualifiedName = "bestow.physics3d.raycast",
        .description = "Cast a 3D ray and return the first hit, or nil if nothing was hit.",
        .params = {
            {.name = "origin", .type = "Vec3", .description = "Ray origin point"},
            {.name = "direction", .type = "Vec3", .description = "Ray direction"},
            {.name = "maxDistance", .type = "number", .description = "Maximum ray distance"},
            {.name = "filter", .type = "QueryFilter3D", .description = "Query filter", .optional = true},
        },
        .returns = {{.type = "RaycastHit3D|nil", .description = "Hit result or nil"}},
        .seeAlso = {"bestow.physics3d.raycastAll"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "raycastAll",
        .qualifiedName = "bestow.physics3d.raycastAll",
        .description = "Cast a 3D ray and return all hits along the ray.",
        .params = {
            {.name = "origin", .type = "Vec3", .description = "Ray origin point"},
            {.name = "direction", .type = "Vec3", .description = "Ray direction"},
            {.name = "maxDistance", .type = "number", .description = "Maximum ray distance"},
            {.name = "filter", .type = "QueryFilter3D", .description = "Query filter", .optional = true},
        },
        .returns = {{.type = "RaycastHit3D[]", .description = "Array of all hits"}},
        .seeAlso = {"bestow.physics3d.raycast"},
    });

    // --- Shape Casting ---

    sys.methods.push_back(MethodDoc{
        .name = "sphereCast",
        .qualifiedName = "bestow.physics3d.sphereCast",
        .description = "Cast a sphere along a direction and return the first hit.",
        .params = {
            {.name = "origin", .type = "Vec3", .description = "Sphere center start position"},
            {.name = "radius", .type = "number", .description = "Sphere radius"},
            {.name = "direction", .type = "Vec3", .description = "Cast direction"},
            {.name = "maxDistance", .type = "number", .description = "Maximum cast distance"},
            {.name = "filter", .type = "QueryFilter3D", .description = "Query filter", .optional = true},
        },
        .returns = {{.type = "ShapeCastHit3D|nil", .description = "Hit result or nil"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "boxCast",
        .qualifiedName = "bestow.physics3d.boxCast",
        .description = "Cast an oriented box along a direction and return the first hit.",
        .params = {
            {.name = "origin", .type = "Vec3", .description = "Box center start position"},
            {.name = "halfExtents", .type = "Vec3", .description = "Box half extents"},
            {.name = "rotation", .type = "Quat", .description = "Box orientation"},
            {.name = "direction", .type = "Vec3", .description = "Cast direction"},
            {.name = "maxDistance", .type = "number", .description = "Maximum cast distance"},
            {.name = "filter", .type = "QueryFilter3D", .description = "Query filter", .optional = true},
        },
        .returns = {{.type = "ShapeCastHit3D|nil", .description = "Hit result or nil"}},
    });

    // --- Overlap Queries ---

    sys.methods.push_back(MethodDoc{
        .name = "overlapSphere",
        .qualifiedName = "bestow.physics3d.overlapSphere",
        .description = "Find all entities overlapping a sphere.",
        .params = {
            {.name = "center", .type = "Vec3", .description = "Sphere center"},
            {.name = "radius", .type = "number", .description = "Sphere radius"},
            {.name = "filter", .type = "QueryFilter3D", .description = "Query filter", .optional = true},
        },
        .returns = {{.type = "Entity[]", .description = "Array of overlapping entities"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "overlapBox",
        .qualifiedName = "bestow.physics3d.overlapBox",
        .description = "Find all entities overlapping an oriented box.",
        .params = {
            {.name = "center", .type = "Vec3", .description = "Box center"},
            {.name = "halfExtents", .type = "Vec3", .description = "Box half extents"},
            {.name = "rotation", .type = "Quat", .description = "Box orientation"},
            {.name = "filter", .type = "QueryFilter3D", .description = "Query filter", .optional = true},
        },
        .returns = {{.type = "Entity[]", .description = "Array of overlapping entities"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "queryAABB",
        .qualifiedName = "bestow.physics3d.queryAABB",
        .description = "Find all entities overlapping an axis-aligned bounding box.",
        .params = {
            {.name = "min", .type = "Vec3", .description = "Minimum corner of the AABB"},
            {.name = "max", .type = "Vec3", .description = "Maximum corner of the AABB"},
            {.name = "filter", .type = "QueryFilter3D", .description = "Query filter", .optional = true},
        },
        .returns = {{.type = "Entity[]", .description = "Array of entities in the region"}},
    });

    // --- Character Controller ---

    sys.methods.push_back(MethodDoc{
        .name = "createCharacter",
        .qualifiedName = "bestow.physics3d.createCharacter",
        .description = "Create a character controller for an entity.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "def", .type = "CharacterControllerDef", .description = "Character controller definition"},
        },
        .returns = {{.type = "boolean", .description = "true if created successfully"}},
        .seeAlso = {"bestow.physics3d.destroyCharacter", "bestow.physics3d.moveCharacter"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "destroyCharacter",
        .qualifiedName = "bestow.physics3d.destroyCharacter",
        .description = "Destroy a character controller.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
        },
        .returns = {{.type = "boolean", .description = "true if destroyed successfully"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "moveCharacter",
        .qualifiedName = "bestow.physics3d.moveCharacter",
        .description = "Move a character controller with a desired velocity. Handles collisions, slopes, and steps automatically.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "velocity", .type = "Vec3", .description = "Desired movement velocity"},
            {.name = "dt", .type = "number", .description = "Delta time for this frame"},
        },
        .returns = {{.type = "boolean", .description = "true on success"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getCharacterPosition",
        .qualifiedName = "bestow.physics3d.getCharacterPosition",
        .description = "Get the position of a character controller.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
        },
        .returns = {{.type = "Vec3|nil", .description = "Character position, or nil if no character"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setCharacterPosition",
        .qualifiedName = "bestow.physics3d.setCharacterPosition",
        .description = "Teleport a character controller to a new position.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
            {.name = "position", .type = "Vec3", .description = "New world position"},
        },
        .returns = {{.type = "boolean", .description = "true on success"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getCharacterGroundInfo",
        .qualifiedName = "bestow.physics3d.getCharacterGroundInfo",
        .description = "Get ground information for a character controller.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
        },
        .returns = {{.type = "CharacterGroundInfo|nil", .description = "Ground info, or nil if no character"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getCharacterVelocity",
        .qualifiedName = "bestow.physics3d.getCharacterVelocity",
        .description = "Get the current velocity of a character controller.",
        .params = {
            {.name = "entity", .type = "Entity", .description = "The entity"},
        },
        .returns = {{.type = "Vec3|nil", .description = "Character velocity, or nil if no character"}},
    });

    // --- World Settings ---

    sys.methods.push_back(MethodDoc{
        .name = "setGravity",
        .qualifiedName = "bestow.physics3d.setGravity",
        .description = "Set the 3D world gravity vector.",
        .params = {
            {.name = "gravity", .type = "Vec3", .description = "Gravity vector (e.g., Vec3(0, -9.81, 0))"},
        },
        .seeAlso = {"bestow.physics3d.getGravity"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "getGravity",
        .qualifiedName = "bestow.physics3d.getGravity",
        .description = "Get the current 3D world gravity vector.",
        .returns = {{.type = "Vec3", .description = "Current gravity vector"}},
        .seeAlso = {"bestow.physics3d.setGravity"},
    });

    // --- Collision Callbacks ---

    sys.methods.push_back(MethodDoc{
        .name = "setCollisionCallback",
        .qualifiedName = "bestow.physics3d.setCollisionCallback",
        .description = "Set a callback function that is called on every collision event.",
        .params = {
            {.name = "callback", .type = "function|nil", .description = "Callback receiving CollisionEvent3D, or nil to clear"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "setTriggerEnterCallback",
        .qualifiedName = "bestow.physics3d.setTriggerEnterCallback",
        .description = "Set a callback for when a body enters a trigger volume.",
        .params = {
            {.name = "callback", .type = "function|nil", .description = "Callback receiving TriggerEvent3D, or nil to clear"},
        },
        .seeAlso = {"bestow.physics3d.setTriggerExitCallback"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "setTriggerExitCallback",
        .qualifiedName = "bestow.physics3d.setTriggerExitCallback",
        .description = "Set a callback for when a body exits a trigger volume.",
        .params = {
            {.name = "callback", .type = "function|nil", .description = "Callback receiving TriggerEvent3D, or nil to clear"},
        },
        .seeAlso = {"bestow.physics3d.setTriggerEnterCallback"},
    });

    // --- Transform Synchronization ---

    sys.methods.push_back(MethodDoc{
        .name = "syncTransforms",
        .qualifiedName = "bestow.physics3d.syncTransforms",
        .description = "Synchronize physics transforms to entity transforms for a list of entities.",
        .params = {
            {.name = "entities", .type = "Entity[]", .description = "Array of entities to synchronize"},
        },
    });

    // --- Debug & Statistics ---

    sys.methods.push_back(MethodDoc{
        .name = "setDebugDraw",
        .qualifiedName = "bestow.physics3d.setDebugDraw",
        .description = "Enable or disable physics debug visualization.",
        .params = {
            {.name = "enabled", .type = "boolean", .description = "true to enable debug drawing"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "getStats",
        .qualifiedName = "bestow.physics3d.getStats",
        .description = "Get performance statistics for the 3D physics simulation.",
        .returns = {{.type = "PhysicsStats3D", .description = "Statistics including body counts, timing, and collision pairs"}},
    });

    // --- Properties ---

    sys.properties.push_back(PropertyDoc{
        .name = "Layer",
        .type = "table",
        .description = "Predefined 3D collision layer constants: Default, Static, Dynamic, Character, Trigger, Debris.",
        .readOnly = true,
    });

    registry.addSystem(std::move(sys));
}

} // namespace bestow
