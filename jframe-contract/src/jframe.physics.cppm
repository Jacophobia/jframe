// jframe-contract/src/jframe.physics.cppm
// Physics system interface

module;

#include <functional>
#include <optional>
#include <vector>

export module jframe.physics;

import jframe.types;

export namespace jframe {

struct RaycastHit {
    Entity entity;
    Vec2 point;
    Vec2 normal;
    float distance;
};

class IPhysicsSystem {
public:
    virtual ~IPhysicsSystem() = default;

    //======================================================================
    // Lifecycle
    //======================================================================

    virtual void update(DeltaTime dt) = 0;

    //======================================================================
    // Body Management
    //======================================================================

    virtual void createBody(Entity entity, const PhysicsBodyDef& def) = 0;
    virtual void destroyBody(Entity entity) = 0;
    virtual bool hasBody(Entity entity) const = 0;

    //======================================================================
    // Body Properties
    //======================================================================

    virtual void setBodyType(Entity entity, BodyType type) = 0;
    virtual BodyType getBodyType(Entity entity) const = 0;

    virtual void setPosition(Entity entity, Vec2 position) = 0;
    virtual Vec2 getPosition(Entity entity) const = 0;

    virtual void setRotation(Entity entity, float radians) = 0;
    virtual float getRotation(Entity entity) const = 0;

    virtual void setVelocity(Entity entity, Vec2 velocity) = 0;
    virtual Vec2 getVelocity(Entity entity) const = 0;

    virtual void setAngularVelocity(Entity entity, float velocity) = 0;
    virtual float getAngularVelocity(Entity entity) const = 0;

    /// Get the size of a physics body (returns the collision box size in pixels)
    virtual Vec2 getBodySize(Entity entity) const = 0;

    //======================================================================
    // Forces
    //======================================================================

    virtual void applyForce(Entity entity, Vec2 force,
                            Vec2 point = {0, 0}) = 0;
    virtual void applyImpulse(Entity entity, Vec2 impulse,
                              Vec2 point = {0, 0}) = 0;
    virtual void applyTorque(Entity entity, float torque) = 0;

    //======================================================================
    // Collision Filtering
    //======================================================================

    virtual void setCollisionLayer(Entity entity, CollisionLayer layer) = 0;
    virtual void setCollisionMask(Entity entity, CollisionMask mask) = 0;
    virtual void setSensor(Entity entity, bool isSensor) = 0;

    //======================================================================
    // Queries
    //======================================================================

    virtual std::vector<Entity> queryAABB(Vec2 min, Vec2 max) const = 0;
    virtual std::vector<Entity> queryCircle(Vec2 center, float radius) const = 0;
    virtual std::optional<RaycastHit> raycast(Vec2 origin, Vec2 direction,
                                               float maxDistance,
                                               CollisionMask mask = 0xFFFF) const = 0;
    virtual std::vector<RaycastHit> raycastAll(Vec2 origin, Vec2 direction,
                                                float maxDistance,
                                                CollisionMask mask = 0xFFFF) const = 0;

    //======================================================================
    // World Settings
    //======================================================================

    virtual void setGravity(Vec2 gravity) = 0;
    virtual Vec2 getGravity() const = 0;

    //======================================================================
    // Collision Callbacks
    //======================================================================

    using CollisionCallback = std::function<void(const CollisionEvent&)>;
    virtual void setCollisionCallback(CollisionCallback callback) = 0;

    //======================================================================
    // Ground Detection
    //======================================================================

    /// Check if an entity is grounded (standing on a valid surface)
    /// Uses collision layer filtering - set Ground layer on platforms that count as ground
    virtual GroundCheckResult checkGrounded(Entity entity,
                                            const GroundCheckParams& params = {}) const = 0;

    /// Get the collision layer for an entity (for debugging/inspection)
    virtual CollisionLayer getCollisionLayer(Entity entity) const = 0;
};

// Common collision layer constants
namespace CollisionLayers {
    inline constexpr CollisionLayer Player = 0x0001;
    inline constexpr CollisionLayer Enemy = 0x0002;
    inline constexpr CollisionLayer Projectile = 0x0004;
    inline constexpr CollisionLayer Terrain = 0x0008;
    inline constexpr CollisionLayer Trigger = 0x0010;
    inline constexpr CollisionLayer Collectible = 0x0020;
    inline constexpr CollisionLayer Ground = 0x0040;  // Valid surface for grounded check
}

}  // namespace jframe
