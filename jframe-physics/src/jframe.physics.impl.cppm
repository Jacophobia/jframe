// jframe-physics/src/jframe.physics.impl.cppm
// Physics system implementation using Box2D 3.0

module;

#include <box2d/box2d.h>

export module jframe.physics.impl;

import std;
import jframe.physics;
import jframe.types;

export namespace jframe {

class Box2DPhysicsSystem : public IPhysicsSystem {
public:
    Box2DPhysicsSystem();
    ~Box2DPhysicsSystem() override;

    bool initialize();

    void update(DeltaTime dt) override;

    // Body management
    void createBody(Entity entity, const PhysicsBodyDef& def) override;
    void destroyBody(Entity entity) override;
    bool hasBody(Entity entity) const override;

    // Body properties
    void setBodyType(Entity entity, BodyType type) override;
    BodyType getBodyType(Entity entity) const override;
    void setPosition(Entity entity, Vec2 position) override;
    Vec2 getPosition(Entity entity) const override;
    void setRotation(Entity entity, float radians) override;
    float getRotation(Entity entity) const override;
    void setVelocity(Entity entity, Vec2 velocity) override;
    Vec2 getVelocity(Entity entity) const override;
    void setAngularVelocity(Entity entity, float velocity) override;
    float getAngularVelocity(Entity entity) const override;
    Vec2 getBodySize(Entity entity) const override;

    // Forces
    void applyForce(Entity entity, Vec2 force, Vec2 point = {0, 0}) override;
    void applyImpulse(Entity entity, Vec2 impulse, Vec2 point = {0, 0}) override;
    void applyTorque(Entity entity, float torque) override;

    // Collision filtering
    void setCollisionLayer(Entity entity, CollisionLayer layer) override;
    void setCollisionMask(Entity entity, CollisionMask mask) override;
    void setSensor(Entity entity, bool isSensor) override;

    // Queries
    std::vector<Entity> queryAABB(Vec2 min, Vec2 max) const override;
    std::vector<Entity> queryCircle(Vec2 center, float radius) const override;
    std::optional<RaycastHit> raycast(Vec2 origin, Vec2 direction,
                                       float maxDistance,
                                       CollisionMask mask = 0xFFFF) const override;
    std::vector<RaycastHit> raycastAll(Vec2 origin, Vec2 direction,
                                        float maxDistance,
                                        CollisionMask mask = 0xFFFF) const override;

    // World settings
    void setGravity(Vec2 gravity) override;
    Vec2 getGravity() const override;

    // Collision callbacks
    void setCollisionCallback(CollisionCallback callback) override;

    // Trigger callbacks
    using TriggerCallback = std::function<void(const TriggerEvent&)>;
    void setTriggerEnterCallback(TriggerCallback callback);
    void setTriggerExitCallback(TriggerCallback callback);

    // Ground detection
    GroundCheckResult checkGrounded(Entity entity,
                                    const GroundCheckParams& params = {}) const override;
    CollisionLayer getCollisionLayer(Entity entity) const override;

private:
    void processContactEvents();

    // Body metadata (for shape/filter info not stored in Box2D)
    struct BodyMeta {
        Entity entity;
        CollisionLayer layer = 0x0001;
        CollisionMask mask = 0xFFFF;
        bool isSensor = false;
    };

    b2WorldId worldId_;
    std::unordered_map<std::uint32_t, b2BodyId> entityToBody_;
    std::unordered_map<std::uint64_t, BodyMeta> bodyToMeta_;  // b2BodyId uses index1 which fits in uint64
    Vec2 gravity_{0.0f, 980.0f};  // Default gravity in pixels/s² (positive Y = down in screen coords)
    CollisionCallback collisionCallback_;
    TriggerCallback triggerEnterCallback_;
    TriggerCallback triggerExitCallback_;
    bool initialized_ = false;

    // Physics scale: pixels per meter (Box2D works in meters)
    static constexpr float PIXELS_PER_METER = 100.0f;

    // Sub-steps for simulation accuracy
    static constexpr int SUB_STEP_COUNT = 4;
};

// Factory function (exported via namespace)
inline std::unique_ptr<IPhysicsSystem> createPhysicsSystem() {
    return std::make_unique<Box2DPhysicsSystem>();
}

}  // namespace jframe
