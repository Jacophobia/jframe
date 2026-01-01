// bestow-luabind/src/bindings/physics3d_binding.cpp
// 3D Physics system Lua bindings

module;

#include <bestow/sol2_compat.hpp>
#include <spdlog/spdlog.h>

module bestow.luabind;

import std;

namespace bestow {

// Helper to convert Result<T, Physics3DError> to Lua (value, nil) or (nil, errorString)
template<typename T>
sol::object resultToLua(sol::state& lua, const Result<T, Physics3DError>& result) {
    if (result.has_value()) {
        return sol::make_object(lua, result.value());
    }
    return sol::nil;
}

// Helper for Result<void, Physics3DError>
sol::object voidResultToLua(sol::state& lua, const Result<void, Physics3DError>& result) {
    if (result.has_value()) {
        return sol::make_object(lua, true);
    }
    return sol::make_object(lua, false);
}

void bindPhysics3DSystem(sol::state& lua, IPhysics3DSystem& physics) {
    //=========================================================================
    // Physics3D-related types
    //=========================================================================

    // Physics3DError enum
    lua.new_enum<Physics3DError>("Physics3DError",
        {
            {"Success", Physics3DError::Success},
            {"InvalidEntity", Physics3DError::InvalidEntity},
            {"BodyNotFound", Physics3DError::BodyNotFound},
            {"InvalidShape", Physics3DError::InvalidShape},
            {"ConstraintNotFound", Physics3DError::ConstraintNotFound},
            {"CharacterNotFound", Physics3DError::CharacterNotFound},
            {"VehicleNotFound", Physics3DError::VehicleNotFound},
            {"InvalidConfiguration", Physics3DError::InvalidConfiguration},
            {"OutOfMemory", Physics3DError::OutOfMemory},
            {"InternalError", Physics3DError::InternalError}
        }
    );

    // BodyType3D enum
    lua.new_enum<BodyType3D>("BodyType3D",
        {
            {"Static", BodyType3D::Static},
            {"Kinematic", BodyType3D::Kinematic},
            {"Dynamic", BodyType3D::Dynamic}
        }
    );

    // ShapeType3D enum
    lua.new_enum<ShapeType3D>("ShapeType3D",
        {
            {"Box", ShapeType3D::Box},
            {"Sphere", ShapeType3D::Sphere},
            {"Capsule", ShapeType3D::Capsule},
            {"Cylinder", ShapeType3D::Cylinder},
            {"Mesh", ShapeType3D::Mesh},
            {"ConvexHull", ShapeType3D::ConvexHull},
            {"HeightField", ShapeType3D::HeightField},
            {"Compound", ShapeType3D::Compound}
        }
    );

    // MotionQuality enum
    lua.new_enum<MotionQuality>("MotionQuality",
        {
            {"Discrete", MotionQuality::Discrete},
            {"LinearCast", MotionQuality::LinearCast}
        }
    );

    // PhysicsBodyDef3D struct
    lua.new_usertype<PhysicsBodyDef3D>("PhysicsBodyDef3D",
        sol::constructors<PhysicsBodyDef3D()>(),
        "type", &PhysicsBodyDef3D::type,
        "transform", &PhysicsBodyDef3D::transform,
        "shapeType", &PhysicsBodyDef3D::shapeType,
        "shapeHalfExtents", &PhysicsBodyDef3D::shapeHalfExtents,
        "shapeRadius", &PhysicsBodyDef3D::shapeRadius,
        "shapeHalfHeight", &PhysicsBodyDef3D::shapeHalfHeight,
        "density", &PhysicsBodyDef3D::density,
        "friction", &PhysicsBodyDef3D::friction,
        "restitution", &PhysicsBodyDef3D::restitution,
        "linearDamping", &PhysicsBodyDef3D::linearDamping,
        "angularDamping", &PhysicsBodyDef3D::angularDamping,
        "gravityFactor", &PhysicsBodyDef3D::gravityFactor,
        "allowSleep", &PhysicsBodyDef3D::allowSleep,
        "layer", &PhysicsBodyDef3D::layer,
        "mask", &PhysicsBodyDef3D::mask,
        "isSensor", &PhysicsBodyDef3D::isSensor,
        "motionQuality", &PhysicsBodyDef3D::motionQuality
    );

    // QueryFilter3D struct
    lua.new_usertype<QueryFilter3D>("QueryFilter3D",
        sol::constructors<QueryFilter3D()>(),
        "layerMask", &QueryFilter3D::layerMask,
        "ignoreSensors", &QueryFilter3D::ignoreSensors
    );

    // RaycastHit3D struct
    lua.new_usertype<RaycastHit3D>("RaycastHit3D",
        sol::constructors<RaycastHit3D()>(),
        "entity", &RaycastHit3D::entity,
        "point", &RaycastHit3D::point,
        "normal", &RaycastHit3D::normal,
        "distance", &RaycastHit3D::distance
    );

    // ShapeCastHit3D struct
    lua.new_usertype<ShapeCastHit3D>("ShapeCastHit3D",
        sol::constructors<ShapeCastHit3D()>(),
        "entity", &ShapeCastHit3D::entity,
        "point", &ShapeCastHit3D::point,
        "normal", &ShapeCastHit3D::normal,
        "distance", &ShapeCastHit3D::distance,
        "penetrationDepth", &ShapeCastHit3D::penetrationDepth
    );

    // CharacterControllerDef struct
    lua.new_usertype<CharacterControllerDef>("CharacterControllerDef",
        sol::constructors<CharacterControllerDef()>(),
        "radius", &CharacterControllerDef::radius,
        "height", &CharacterControllerDef::height,
        "stepHeight", &CharacterControllerDef::stepHeight,
        "maxSlopeAngle", &CharacterControllerDef::maxSlopeAngle,
        "mass", &CharacterControllerDef::mass,
        "layer", &CharacterControllerDef::layer,
        "mask", &CharacterControllerDef::mask
    );

    // CharacterGroundState enum
    lua.new_enum<CharacterGroundState>("CharacterGroundState",
        {
            {"OnGround", CharacterGroundState::OnGround},
            {"OnSteepGround", CharacterGroundState::OnSteepGround},
            {"InAir", CharacterGroundState::InAir},
            {"Sliding", CharacterGroundState::Sliding}
        }
    );

    // CharacterGroundInfo struct
    lua.new_usertype<CharacterGroundInfo>("CharacterGroundInfo",
        sol::constructors<CharacterGroundInfo()>(),
        "state", &CharacterGroundInfo::state,
        "groundEntity", &CharacterGroundInfo::groundEntity,
        "groundNormal", &CharacterGroundInfo::groundNormal,
        "groundPoint", &CharacterGroundInfo::groundPoint,
        "slopeAngle", &CharacterGroundInfo::slopeAngle
    );

    // PhysicsStats3D struct
    lua.new_usertype<PhysicsStats3D>("PhysicsStats3D",
        sol::constructors<PhysicsStats3D()>(),
        "activeBodies", &PhysicsStats3D::activeBodies,
        "sleepingBodies", &PhysicsStats3D::sleepingBodies,
        "constraints", &PhysicsStats3D::constraints,
        "characters", &PhysicsStats3D::characters,
        "vehicles", &PhysicsStats3D::vehicles,
        "updateTimeMs", &PhysicsStats3D::updateTimeMs,
        "collisionPairs", &PhysicsStats3D::collisionPairs
    );

    // CollisionEvent3D struct
    lua.new_usertype<CollisionEvent3D>("CollisionEvent3D",
        sol::constructors<CollisionEvent3D()>(),
        "entityA", &CollisionEvent3D::entityA,
        "entityB", &CollisionEvent3D::entityB,
        "contactPoint", &CollisionEvent3D::contactPoint,
        "contactNormal", &CollisionEvent3D::contactNormal,
        "impulse", &CollisionEvent3D::impulse,
        "penetrationDepth", &CollisionEvent3D::penetrationDepth
    );

    // TriggerEvent3D struct
    lua.new_usertype<TriggerEvent3D>("TriggerEvent3D",
        sol::constructors<TriggerEvent3D()>(),
        "entityA", &TriggerEvent3D::entityA,
        "entityB", &TriggerEvent3D::entityB
    );

    //=========================================================================
    // bestow.physics3d table
    //=========================================================================

    sol::table bestow = lua["bestow"];
    sol::table physics3dTable = lua.create_table();

    //-------------------------------------------------------------------------
    // Body Management
    //-------------------------------------------------------------------------

    physics3dTable["createBody"] = [&physics, &lua](Entity entity, const PhysicsBodyDef3D& def) {
        auto result = physics.createBody(entity, def);
        return voidResultToLua(lua, result);
    };

    physics3dTable["destroyBody"] = [&physics, &lua](Entity entity) {
        auto result = physics.destroyBody(entity);
        return voidResultToLua(lua, result);
    };

    physics3dTable["hasBody"] = [&physics](Entity entity) {
        return physics.hasBody(entity);
    };

    physics3dTable["getAllBodies"] = [&physics]() {
        return physics.getAllBodies();
    };

    //-------------------------------------------------------------------------
    // Body Type
    //-------------------------------------------------------------------------

    physics3dTable["setBodyType"] = [&physics, &lua](Entity entity, BodyType3D type) {
        auto result = physics.setBodyType(entity, type);
        return voidResultToLua(lua, result);
    };

    physics3dTable["getBodyType"] = [&physics, &lua](Entity entity) -> sol::object {
        auto result = physics.getBodyType(entity);
        return resultToLua(lua, result);
    };

    //-------------------------------------------------------------------------
    // Transform
    //-------------------------------------------------------------------------

    physics3dTable["setTransform"] = [&physics, &lua](Entity entity, const Transform3D& transform) {
        auto result = physics.setTransform(entity, transform);
        return voidResultToLua(lua, result);
    };

    physics3dTable["getTransform"] = [&physics, &lua](Entity entity) -> sol::object {
        auto result = physics.getTransform(entity);
        return resultToLua(lua, result);
    };

    physics3dTable["setPosition"] = [&physics, &lua](Entity entity, const Vec3& position) {
        auto result = physics.setPosition(entity, position);
        return voidResultToLua(lua, result);
    };

    physics3dTable["getPosition"] = [&physics, &lua](Entity entity) -> sol::object {
        auto result = physics.getPosition(entity);
        return resultToLua(lua, result);
    };

    physics3dTable["setRotation"] = [&physics, &lua](Entity entity, const Quat& rotation) {
        auto result = physics.setRotation(entity, rotation);
        return voidResultToLua(lua, result);
    };

    physics3dTable["getRotation"] = [&physics, &lua](Entity entity) -> sol::object {
        auto result = physics.getRotation(entity);
        return resultToLua(lua, result);
    };

    //-------------------------------------------------------------------------
    // Velocity
    //-------------------------------------------------------------------------

    physics3dTable["setLinearVelocity"] = [&physics, &lua](Entity entity, const Vec3& velocity) {
        auto result = physics.setLinearVelocity(entity, velocity);
        return voidResultToLua(lua, result);
    };

    physics3dTable["getLinearVelocity"] = [&physics, &lua](Entity entity) -> sol::object {
        auto result = physics.getLinearVelocity(entity);
        return resultToLua(lua, result);
    };

    physics3dTable["setAngularVelocity"] = [&physics, &lua](Entity entity, const Vec3& velocity) {
        auto result = physics.setAngularVelocity(entity, velocity);
        return voidResultToLua(lua, result);
    };

    physics3dTable["getAngularVelocity"] = [&physics, &lua](Entity entity) -> sol::object {
        auto result = physics.getAngularVelocity(entity);
        return resultToLua(lua, result);
    };

    //-------------------------------------------------------------------------
    // Forces
    //-------------------------------------------------------------------------

    physics3dTable["applyForce"] = [&physics, &lua](Entity entity, const Vec3& force) {
        auto result = physics.applyForce(entity, force);
        return voidResultToLua(lua, result);
    };

    physics3dTable["applyForceAtPoint"] = [&physics, &lua](Entity entity, const Vec3& force, const Vec3& point) {
        auto result = physics.applyForceAtPoint(entity, force, point);
        return voidResultToLua(lua, result);
    };

    physics3dTable["applyTorque"] = [&physics, &lua](Entity entity, const Vec3& torque) {
        auto result = physics.applyTorque(entity, torque);
        return voidResultToLua(lua, result);
    };

    physics3dTable["applyImpulse"] = [&physics, &lua](Entity entity, const Vec3& impulse) {
        auto result = physics.applyImpulse(entity, impulse);
        return voidResultToLua(lua, result);
    };

    physics3dTable["applyImpulseAtPoint"] = [&physics, &lua](Entity entity, const Vec3& impulse, const Vec3& point) {
        auto result = physics.applyImpulseAtPoint(entity, impulse, point);
        return voidResultToLua(lua, result);
    };

    physics3dTable["applyAngularImpulse"] = [&physics, &lua](Entity entity, const Vec3& impulse) {
        auto result = physics.applyAngularImpulse(entity, impulse);
        return voidResultToLua(lua, result);
    };

    //-------------------------------------------------------------------------
    // Body Properties
    //-------------------------------------------------------------------------

    physics3dTable["setMass"] = [&physics, &lua](Entity entity, float mass) {
        auto result = physics.setMass(entity, mass);
        return voidResultToLua(lua, result);
    };

    physics3dTable["getMass"] = [&physics, &lua](Entity entity) -> sol::object {
        auto result = physics.getMass(entity);
        return resultToLua(lua, result);
    };

    physics3dTable["setLinearDamping"] = [&physics, &lua](Entity entity, float damping) {
        auto result = physics.setLinearDamping(entity, damping);
        return voidResultToLua(lua, result);
    };

    physics3dTable["setAngularDamping"] = [&physics, &lua](Entity entity, float damping) {
        auto result = physics.setAngularDamping(entity, damping);
        return voidResultToLua(lua, result);
    };

    physics3dTable["setGravityFactor"] = [&physics, &lua](Entity entity, float factor) {
        auto result = physics.setGravityFactor(entity, factor);
        return voidResultToLua(lua, result);
    };

    physics3dTable["setFriction"] = [&physics, &lua](Entity entity, float friction) {
        auto result = physics.setFriction(entity, friction);
        return voidResultToLua(lua, result);
    };

    physics3dTable["setRestitution"] = [&physics, &lua](Entity entity, float restitution) {
        auto result = physics.setRestitution(entity, restitution);
        return voidResultToLua(lua, result);
    };

    //-------------------------------------------------------------------------
    // Collision Filtering
    //-------------------------------------------------------------------------

    physics3dTable["setCollisionLayer"] = [&physics, &lua](Entity entity, CollisionLayer3D layer) {
        auto result = physics.setCollisionLayer(entity, layer);
        return voidResultToLua(lua, result);
    };

    physics3dTable["setCollisionMask"] = [&physics, &lua](Entity entity, CollisionMask3D mask) {
        auto result = physics.setCollisionMask(entity, mask);
        return voidResultToLua(lua, result);
    };

    physics3dTable["setSensor"] = [&physics, &lua](Entity entity, bool isSensor) {
        auto result = physics.setSensor(entity, isSensor);
        return voidResultToLua(lua, result);
    };

    //-------------------------------------------------------------------------
    // Sleep State
    //-------------------------------------------------------------------------

    physics3dTable["isAwake"] = [&physics, &lua](Entity entity) -> sol::object {
        auto result = physics.isAwake(entity);
        return resultToLua(lua, result);
    };

    physics3dTable["wakeUp"] = [&physics, &lua](Entity entity) {
        auto result = physics.wakeUp(entity);
        return voidResultToLua(lua, result);
    };

    physics3dTable["putToSleep"] = [&physics, &lua](Entity entity) {
        auto result = physics.putToSleep(entity);
        return voidResultToLua(lua, result);
    };

    //-------------------------------------------------------------------------
    // Bounding Box
    //-------------------------------------------------------------------------

    physics3dTable["getBodyBounds"] = [&physics, &lua](Entity entity) -> sol::object {
        auto result = physics.getBodyBounds(entity);
        return resultToLua(lua, result);
    };

    //-------------------------------------------------------------------------
    // Raycasting
    //-------------------------------------------------------------------------

    physics3dTable["raycast"] = sol::overload(
        [&physics, &lua](const Vec3& origin, const Vec3& direction, float maxDistance) -> sol::object {
            auto result = physics.raycast(origin, direction, maxDistance);
            if (result) {
                return sol::make_object(lua, *result);
            }
            return sol::nil;
        },
        [&physics, &lua](const Vec3& origin, const Vec3& direction, float maxDistance, const QueryFilter3D& filter) -> sol::object {
            auto result = physics.raycast(origin, direction, maxDistance, filter);
            if (result) {
                return sol::make_object(lua, *result);
            }
            return sol::nil;
        }
    );

    physics3dTable["raycastAll"] = sol::overload(
        [&physics](const Vec3& origin, const Vec3& direction, float maxDistance) {
            return physics.raycastAll(origin, direction, maxDistance);
        },
        [&physics](const Vec3& origin, const Vec3& direction, float maxDistance, const QueryFilter3D& filter) {
            return physics.raycastAll(origin, direction, maxDistance, filter);
        }
    );

    //-------------------------------------------------------------------------
    // Shape Casting
    //-------------------------------------------------------------------------

    physics3dTable["sphereCast"] = sol::overload(
        [&physics, &lua](const Vec3& origin, float radius, const Vec3& direction, float maxDistance) -> sol::object {
            auto result = physics.sphereCast(origin, radius, direction, maxDistance);
            if (result) {
                return sol::make_object(lua, *result);
            }
            return sol::nil;
        },
        [&physics, &lua](const Vec3& origin, float radius, const Vec3& direction, float maxDistance, const QueryFilter3D& filter) -> sol::object {
            auto result = physics.sphereCast(origin, radius, direction, maxDistance, filter);
            if (result) {
                return sol::make_object(lua, *result);
            }
            return sol::nil;
        }
    );

    physics3dTable["boxCast"] = sol::overload(
        [&physics, &lua](const Vec3& origin, const Vec3& halfExtents, const Quat& rotation,
                         const Vec3& direction, float maxDistance) -> sol::object {
            auto result = physics.boxCast(origin, halfExtents, rotation, direction, maxDistance);
            if (result) {
                return sol::make_object(lua, *result);
            }
            return sol::nil;
        },
        [&physics, &lua](const Vec3& origin, const Vec3& halfExtents, const Quat& rotation,
                         const Vec3& direction, float maxDistance, const QueryFilter3D& filter) -> sol::object {
            auto result = physics.boxCast(origin, halfExtents, rotation, direction, maxDistance, filter);
            if (result) {
                return sol::make_object(lua, *result);
            }
            return sol::nil;
        }
    );

    //-------------------------------------------------------------------------
    // Overlap Queries
    //-------------------------------------------------------------------------

    physics3dTable["overlapSphere"] = sol::overload(
        [&physics](const Vec3& center, float radius) {
            return physics.overlapSphere(center, radius);
        },
        [&physics](const Vec3& center, float radius, const QueryFilter3D& filter) {
            return physics.overlapSphere(center, radius, filter);
        }
    );

    physics3dTable["overlapBox"] = sol::overload(
        [&physics](const Vec3& center, const Vec3& halfExtents, const Quat& rotation) {
            return physics.overlapBox(center, halfExtents, rotation);
        },
        [&physics](const Vec3& center, const Vec3& halfExtents, const Quat& rotation, const QueryFilter3D& filter) {
            return physics.overlapBox(center, halfExtents, rotation, filter);
        }
    );

    physics3dTable["queryAABB"] = sol::overload(
        [&physics](const Vec3& min, const Vec3& max) {
            return physics.queryAABB(min, max);
        },
        [&physics](const Vec3& min, const Vec3& max, const QueryFilter3D& filter) {
            return physics.queryAABB(min, max, filter);
        }
    );

    //-------------------------------------------------------------------------
    // Character Controller
    //-------------------------------------------------------------------------

    physics3dTable["createCharacter"] = [&physics, &lua](Entity entity, const CharacterControllerDef& def) {
        auto result = physics.createCharacter(entity, def);
        return voidResultToLua(lua, result);
    };

    physics3dTable["destroyCharacter"] = [&physics, &lua](Entity entity) {
        auto result = physics.destroyCharacter(entity);
        return voidResultToLua(lua, result);
    };

    physics3dTable["moveCharacter"] = [&physics, &lua](Entity entity, const Vec3& velocity, DeltaTime dt) {
        auto result = physics.moveCharacter(entity, velocity, dt);
        return voidResultToLua(lua, result);
    };

    physics3dTable["getCharacterPosition"] = [&physics, &lua](Entity entity) -> sol::object {
        auto result = physics.getCharacterPosition(entity);
        return resultToLua(lua, result);
    };

    physics3dTable["setCharacterPosition"] = [&physics, &lua](Entity entity, const Vec3& position) {
        auto result = physics.setCharacterPosition(entity, position);
        return voidResultToLua(lua, result);
    };

    physics3dTable["getCharacterGroundInfo"] = [&physics, &lua](Entity entity) -> sol::object {
        auto result = physics.getCharacterGroundInfo(entity);
        return resultToLua(lua, result);
    };

    physics3dTable["getCharacterVelocity"] = [&physics, &lua](Entity entity) -> sol::object {
        auto result = physics.getCharacterVelocity(entity);
        return resultToLua(lua, result);
    };

    //-------------------------------------------------------------------------
    // World Settings
    //-------------------------------------------------------------------------

    physics3dTable["setGravity"] = [&physics](const Vec3& gravity) {
        physics.setGravity(gravity);
    };

    physics3dTable["getGravity"] = [&physics]() {
        return physics.getGravity();
    };

    //-------------------------------------------------------------------------
    // Collision Callbacks
    //-------------------------------------------------------------------------

    physics3dTable["setCollisionCallback"] = [&physics](sol::function callback) {
        if (callback.valid()) {
            physics.setCollisionCallback([callback](const CollisionEvent3D& event) {
                callback(event);
            });
        } else {
            physics.setCollisionCallback(nullptr);
        }
    };

    physics3dTable["setTriggerEnterCallback"] = [&physics](sol::function callback) {
        if (callback.valid()) {
            physics.setTriggerEnterCallback([callback](const TriggerEvent3D& event) {
                callback(event);
            });
        } else {
            physics.setTriggerEnterCallback(nullptr);
        }
    };

    physics3dTable["setTriggerExitCallback"] = [&physics](sol::function callback) {
        if (callback.valid()) {
            physics.setTriggerExitCallback([callback](const TriggerEvent3D& event) {
                callback(event);
            });
        } else {
            physics.setTriggerExitCallback(nullptr);
        }
    };

    //-------------------------------------------------------------------------
    // Debug & Statistics
    //-------------------------------------------------------------------------

    physics3dTable["setDebugDraw"] = [&physics](bool enabled) {
        physics.setDebugDraw(enabled);
    };

    physics3dTable["getStats"] = [&physics]() {
        return physics.getStats();
    };

    //-------------------------------------------------------------------------
    // Collision Layers Constants
    //-------------------------------------------------------------------------

    sol::table layers = lua.create_table();
    layers["Default"] = CollisionLayers3D::Default;
    layers["Static"] = CollisionLayers3D::Static;
    layers["Dynamic"] = CollisionLayers3D::Dynamic;
    layers["Character"] = CollisionLayers3D::Character;
    layers["Trigger"] = CollisionLayers3D::Trigger;
    layers["Debris"] = CollisionLayers3D::Debris;

    physics3dTable["Layer"] = layers;

    bestow["physics3d"] = physics3dTable;
}

}  // namespace bestow
