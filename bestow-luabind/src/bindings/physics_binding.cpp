// bestow-luabind/src/bindings/physics_binding.cpp
// 2D Physics system Lua bindings

module;

#include <bestow/sol2_compat.hpp>
#include <spdlog/spdlog.h>

module bestow.luabind;

import std;

namespace bestow {

void bindPhysicsSystem(sol::state& lua, IPhysicsSystem& physics) {
    //=========================================================================
    // Physics-related types
    //=========================================================================

    // BodyType enum
    lua.new_enum<BodyType>("BodyType",
        {
            {"Static", BodyType::Static},
            {"Kinematic", BodyType::Kinematic},
            {"Dynamic", BodyType::Dynamic}
        }
    );

    // PhysicsBodyDef struct
    lua.new_usertype<PhysicsBodyDef>("PhysicsBodyDef",
        sol::constructors<PhysicsBodyDef()>(),
        "type", &PhysicsBodyDef::type,
        "transform", &PhysicsBodyDef::transform,
        "size", &PhysicsBodyDef::size,
        "fixedRotation", &PhysicsBodyDef::fixedRotation,
        "linearDamping", &PhysicsBodyDef::linearDamping,
        "angularDamping", &PhysicsBodyDef::angularDamping,
        "density", &PhysicsBodyDef::density,
        "friction", &PhysicsBodyDef::friction,
        "restitution", &PhysicsBodyDef::restitution,
        "isSensor", &PhysicsBodyDef::isSensor
    );

    // RaycastHit struct
    lua.new_usertype<RaycastHit>("RaycastHit",
        sol::constructors<RaycastHit()>(),
        "entity", &RaycastHit::entity,
        "point", &RaycastHit::point,
        "normal", &RaycastHit::normal,
        "distance", &RaycastHit::distance
    );

    // CollisionEvent struct
    lua.new_usertype<CollisionEvent>("CollisionEvent",
        sol::constructors<CollisionEvent()>(),
        "entityA", &CollisionEvent::entityA,
        "entityB", &CollisionEvent::entityB,
        "contactPoint", &CollisionEvent::contactPoint,
        "normal", &CollisionEvent::normal,
        "impulse", &CollisionEvent::impulse
    );

    // GroundCheckParams struct
    lua.new_usertype<GroundCheckParams>("GroundCheckParams",
        sol::constructors<GroundCheckParams()>(),
        "rayDistance", &GroundCheckParams::rayDistance,
        "slopeToleranceDeg", &GroundCheckParams::slopeToleranceDeg,
        "groundMask", &GroundCheckParams::groundMask
    );

    // GroundCheckResult struct
    lua.new_usertype<GroundCheckResult>("GroundCheckResult",
        sol::constructors<GroundCheckResult()>(),
        "grounded", &GroundCheckResult::grounded,
        "groundEntity", &GroundCheckResult::groundEntity,
        "contactPoint", &GroundCheckResult::contactPoint,
        "surfaceNormal", &GroundCheckResult::surfaceNormal,
        "slopeAngle", &GroundCheckResult::slopeAngle
    );

    //=========================================================================
    // bestow.physics table
    //=========================================================================

    sol::table bestow = lua["bestow"];
    sol::table physicsTable = lua.create_table();

    //-------------------------------------------------------------------------
    // Body Management
    //-------------------------------------------------------------------------

    physicsTable["createBody"] = [&physics](Entity entity, const PhysicsBodyDef& def) {
        physics.createBody(entity, def);
    };

    physicsTable["destroyBody"] = [&physics](Entity entity) {
        physics.destroyBody(entity);
    };

    physicsTable["hasBody"] = [&physics](Entity entity) {
        return physics.hasBody(entity);
    };

    //-------------------------------------------------------------------------
    // Body Properties
    //-------------------------------------------------------------------------

    physicsTable["setBodyType"] = [&physics](Entity entity, BodyType type) {
        physics.setBodyType(entity, type);
    };

    physicsTable["getBodyType"] = [&physics](Entity entity) {
        return physics.getBodyType(entity);
    };

    physicsTable["setPosition"] = [&physics](Entity entity, const Vec2& position) {
        physics.setPosition(entity, position);
    };

    physicsTable["getPosition"] = [&physics](Entity entity) {
        return physics.getPosition(entity);
    };

    physicsTable["setRotation"] = [&physics](Entity entity, float radians) {
        physics.setRotation(entity, radians);
    };

    physicsTable["getRotation"] = [&physics](Entity entity) {
        return physics.getRotation(entity);
    };

    physicsTable["setVelocity"] = [&physics](Entity entity, const Vec2& velocity) {
        physics.setVelocity(entity, velocity);
    };

    physicsTable["getVelocity"] = [&physics](Entity entity) {
        return physics.getVelocity(entity);
    };

    physicsTable["setAngularVelocity"] = [&physics](Entity entity, float velocity) {
        physics.setAngularVelocity(entity, velocity);
    };

    physicsTable["getAngularVelocity"] = [&physics](Entity entity) {
        return physics.getAngularVelocity(entity);
    };

    physicsTable["getBodySize"] = [&physics](Entity entity) {
        return physics.getBodySize(entity);
    };

    //-------------------------------------------------------------------------
    // Forces
    //-------------------------------------------------------------------------

    physicsTable["applyForce"] = sol::overload(
        [&physics](Entity entity, const Vec2& force) {
            physics.applyForce(entity, force);
        },
        [&physics](Entity entity, const Vec2& force, const Vec2& point) {
            physics.applyForce(entity, force, point);
        }
    );

    physicsTable["applyImpulse"] = sol::overload(
        [&physics](Entity entity, const Vec2& impulse) {
            physics.applyImpulse(entity, impulse);
        },
        [&physics](Entity entity, const Vec2& impulse, const Vec2& point) {
            physics.applyImpulse(entity, impulse, point);
        }
    );

    physicsTable["applyTorque"] = [&physics](Entity entity, float torque) {
        physics.applyTorque(entity, torque);
    };

    //-------------------------------------------------------------------------
    // Collision Filtering
    //-------------------------------------------------------------------------

    physicsTable["setCollisionLayer"] = [&physics](Entity entity, CollisionLayer layer) {
        physics.setCollisionLayer(entity, layer);
    };

    physicsTable["getCollisionLayer"] = [&physics](Entity entity) {
        return physics.getCollisionLayer(entity);
    };

    physicsTable["setCollisionMask"] = [&physics](Entity entity, CollisionMask mask) {
        physics.setCollisionMask(entity, mask);
    };

    physicsTable["setSensor"] = [&physics](Entity entity, bool isSensor) {
        physics.setSensor(entity, isSensor);
    };

    //-------------------------------------------------------------------------
    // Queries
    //-------------------------------------------------------------------------

    physicsTable["queryAABB"] = [&physics](const Vec2& min, const Vec2& max) {
        return physics.queryAABB(min, max);
    };

    physicsTable["queryCircle"] = [&physics](const Vec2& center, float radius) {
        return physics.queryCircle(center, radius);
    };

    physicsTable["raycast"] = sol::overload(
        [&physics, &lua](const Vec2& origin, const Vec2& direction, float maxDistance) -> sol::object {
            auto result = physics.raycast(origin, direction, maxDistance);
            if (result) {
                return sol::make_object(lua, *result);
            }
            return sol::nil;
        },
        [&physics, &lua](const Vec2& origin, const Vec2& direction, float maxDistance, CollisionMask mask) -> sol::object {
            auto result = physics.raycast(origin, direction, maxDistance, mask);
            if (result) {
                return sol::make_object(lua, *result);
            }
            return sol::nil;
        }
    );

    physicsTable["raycastAll"] = sol::overload(
        [&physics](const Vec2& origin, const Vec2& direction, float maxDistance) {
            return physics.raycastAll(origin, direction, maxDistance);
        },
        [&physics](const Vec2& origin, const Vec2& direction, float maxDistance, CollisionMask mask) {
            return physics.raycastAll(origin, direction, maxDistance, mask);
        }
    );

    //-------------------------------------------------------------------------
    // World Settings
    //-------------------------------------------------------------------------

    physicsTable["setGravity"] = [&physics](const Vec2& gravity) {
        physics.setGravity(gravity);
    };

    physicsTable["getGravity"] = [&physics]() {
        return physics.getGravity();
    };

    //-------------------------------------------------------------------------
    // Ground Detection
    //-------------------------------------------------------------------------

    physicsTable["checkGrounded"] = sol::overload(
        [&physics](Entity entity) {
            return physics.checkGrounded(entity);
        },
        [&physics](Entity entity, const GroundCheckParams& params) {
            return physics.checkGrounded(entity, params);
        }
    );

    //-------------------------------------------------------------------------
    // Collision Layers Constants
    //-------------------------------------------------------------------------

    sol::table layers = lua.create_table();
    layers["Player"] = CollisionLayers::Player;
    layers["Enemy"] = CollisionLayers::Enemy;
    layers["Projectile"] = CollisionLayers::Projectile;
    layers["Terrain"] = CollisionLayers::Terrain;
    layers["Trigger"] = CollisionLayers::Trigger;
    layers["Collectible"] = CollisionLayers::Collectible;
    layers["Ground"] = CollisionLayers::Ground;

    physicsTable["Layer"] = layers;

    bestow["physics"] = physicsTable;
}

}  // namespace bestow
