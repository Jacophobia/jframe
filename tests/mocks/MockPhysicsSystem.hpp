// tests/mocks/MockPhysicsSystem.hpp
// Shared mock physics system for testing

#pragma once

#include <kangaru/kangaru.hpp>

import std;
import bestow;
import bestow.types;

namespace bestow::tests {

class MockPhysicsSystem : public IPhysicsSystem {
public:
    // Tracking
    struct BodyRecord {
        Entity entity;
        PhysicsBodyDef def;
        CollisionLayer layer = 0xFFFF;
        CollisionMask mask = 0xFFFF;
    };
    std::vector<BodyRecord> createdBodies;

    // Delegates
    std::function<void(Entity, const PhysicsBodyDef&)> onCreateBody =
        [this](Entity entity, const PhysicsBodyDef& def) {
            createdBodies.push_back({entity, def});
        };

    // IPhysicsSystem overrides
    void update(DeltaTime) override {}

    void createBody(Entity entity, const PhysicsBodyDef& def) override {
        onCreateBody(entity, def);
    }
    void destroyBody(Entity) override {}
    bool hasBody(Entity) const override { return false; }

    void setBodyType(Entity, BodyType) override {}
    BodyType getBodyType(Entity) const override { return BodyType::Dynamic; }

    void setPosition(Entity, Vec2) override {}
    Vec2 getPosition(Entity) const override { return {}; }

    void setRotation(Entity, float) override {}
    float getRotation(Entity) const override { return 0.0f; }

    void setVelocity(Entity, Vec2) override {}
    Vec2 getVelocity(Entity) const override { return {}; }

    void setAngularVelocity(Entity, float) override {}
    float getAngularVelocity(Entity) const override { return 0.0f; }

    Vec2 getBodySize(Entity) const override { return {}; }

    void applyForce(Entity, Vec2, Vec2) override {}
    void applyImpulse(Entity, Vec2, Vec2) override {}
    void applyTorque(Entity, float) override {}

    void setCollisionLayer(Entity entity, CollisionLayer layer) override {
        for (auto& body : createdBodies) {
            if (body.entity == entity) {
                body.layer = layer;
                break;
            }
        }
    }
    void setCollisionMask(Entity entity, CollisionMask mask) override {
        for (auto& body : createdBodies) {
            if (body.entity == entity) {
                body.mask = mask;
                break;
            }
        }
    }
    void setSensor(Entity, bool) override {}

    std::vector<Entity> queryAABB(Vec2, Vec2) const override { return {}; }
    std::vector<Entity> queryCircle(Vec2, float) const override { return {}; }
    std::optional<RaycastHit> raycast(Vec2, Vec2, float, CollisionMask) const override {
        return std::nullopt;
    }
    std::vector<RaycastHit> raycastAll(Vec2, Vec2, float, CollisionMask) const override {
        return {};
    }

    void setGravity(Vec2) override {}
    Vec2 getGravity() const override { return {0, -980}; }

    void setCollisionCallback(CollisionCallback) override {}

    GroundCheckResult checkGrounded(Entity, const GroundCheckParams&) const override { return {}; }
    CollisionLayer getCollisionLayer(Entity) const override { return 0xFFFF; }
};

}  // namespace bestow::tests
