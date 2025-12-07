// bestow-physics3d/src/bestow.physics3d.impl.cppm
// 3D Physics system implementation using Jolt Physics

module;

// Jolt Physics headers - MUST be in global module fragment
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Constraints/FixedConstraint.h>
#include <Jolt/Physics/Constraints/PointConstraint.h>
#include <Jolt/Physics/Constraints/DistanceConstraint.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <Jolt/Physics/Constraints/SliderConstraint.h>
#include <Jolt/Physics/Constraints/ConeConstraint.h>
#include <Jolt/Physics/Constraints/SixDOFConstraint.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Constraints/FixedConstraint.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <Jolt/Physics/Constraints/SliderConstraint.h>
#include <Jolt/Physics/Constraints/ConeConstraint.h>
#include <Jolt/Physics/Constraints/PointConstraint.h>
#include <Jolt/Physics/Constraints/DistanceConstraint.h>
#include <Jolt/Physics/Constraints/SixDOFConstraint.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Vehicle/VehicleConstraint.h>
#include <Jolt/Physics/Vehicle/WheeledVehicleController.h>

// EnTT for Entity type
#include <entt/entity/entity.hpp>

export module bestow.physics3d.impl;

import std;
import bestow.physics3d;
import bestow.types;

export namespace bestow {

//==========================================================================
// Helper Functions for Type Conversion
//==========================================================================

// Non-exported helper functions
JPH::Vec3 toJolt(Vec3 v) {
    return JPH::Vec3(v.x, v.y, v.z);
}

Vec3 fromJolt(JPH::RVec3 v) {
    return Vec3{static_cast<float>(v.GetX()), static_cast<float>(v.GetY()), static_cast<float>(v.GetZ())};
}

Vec3 fromJoltVec3(JPH::Vec3 v) {
    return Vec3{v.GetX(), v.GetY(), v.GetZ()};
}

JPH::Quat toJolt(Quat q) {
    return JPH::Quat(q.x, q.y, q.z, q.w);
}

Quat fromJolt(JPH::Quat q) {
    return Quat{q.GetW(), q.GetX(), q.GetY(), q.GetZ()};
}

JPH::EMotionType toJoltMotionType(BodyType3D type) {
    switch (type) {
        case BodyType3D::Static: return JPH::EMotionType::Static;
        case BodyType3D::Kinematic: return JPH::EMotionType::Kinematic;
        case BodyType3D::Dynamic: return JPH::EMotionType::Dynamic;
    }
    return JPH::EMotionType::Static;
}

BodyType3D fromJoltMotionType(JPH::EMotionType type) {
    switch (type) {
        case JPH::EMotionType::Static: return BodyType3D::Static;
        case JPH::EMotionType::Kinematic: return BodyType3D::Kinematic;
        case JPH::EMotionType::Dynamic: return BodyType3D::Dynamic;
        default: return BodyType3D::Static;
    }
}

//==========================================================================
// Jolt Physics Layer System (not exported)
//==========================================================================

// Layer definitions - these have internal linkage and cannot be exported
constexpr JPH::ObjectLayer LAYER_NON_MOVING = 0;
constexpr JPH::ObjectLayer LAYER_MOVING = 1;
constexpr JPH::uint NUM_OBJECT_LAYERS = 2;

constexpr JPH::BroadPhaseLayer BROAD_PHASE_LAYER_NON_MOVING(0);
constexpr JPH::BroadPhaseLayer BROAD_PHASE_LAYER_MOVING(1);
constexpr JPH::uint NUM_BROAD_PHASE_LAYERS = 2;

class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
public:
    JPH::uint GetNumBroadPhaseLayers() const override {
        return NUM_BROAD_PHASE_LAYERS;
    }

    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override {
        switch (inLayer) {
            case LAYER_NON_MOVING: return BROAD_PHASE_LAYER_NON_MOVING;
            case LAYER_MOVING: return BROAD_PHASE_LAYER_MOVING;
            default: return BROAD_PHASE_LAYER_MOVING;
        }
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override {
        switch ((JPH::BroadPhaseLayer::Type)inLayer) {
            case (JPH::BroadPhaseLayer::Type)BROAD_PHASE_LAYER_NON_MOVING: return "NON_MOVING";
            case (JPH::BroadPhaseLayer::Type)BROAD_PHASE_LAYER_MOVING: return "MOVING";
            default: return "INVALID";
        }
    }
#endif
};

class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
    bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override {
        switch (inLayer1) {
            case LAYER_NON_MOVING:
                return inLayer2 == BROAD_PHASE_LAYER_MOVING;
            case LAYER_MOVING:
                return true;
            default:
                return false;
        }
    }
};

class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
public:
    bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override {
        switch (inObject1) {
            case LAYER_NON_MOVING:
                return inObject2 == LAYER_MOVING;
            case LAYER_MOVING:
                return true;
            default:
                return false;
        }
    }
};

//==========================================================================
// Contact Listener for Collision Events
//==========================================================================

class ContactListenerImpl : public JPH::ContactListener {
public:
    void setCollisionCallback(Collision3DCallback callback) {
        collisionCallback_ = std::move(callback);
    }

    void setTriggerEnterCallback(Trigger3DEnterCallback callback) {
        triggerEnterCallback_ = std::move(callback);
    }

    void setTriggerExitCallback(Trigger3DExitCallback callback) {
        triggerExitCallback_ = std::move(callback);
    }

    void setEntityLookup(std::function<Entity(JPH::BodyID)> lookup) {
        entityLookup_ = std::move(lookup);
    }

    JPH::ValidateResult OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2,
                                          JPH::RVec3Arg inBaseOffset,
                                          const JPH::CollideShapeResult& inCollisionResult) override {
        return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
    }

    void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2,
                       const JPH::ContactManifold& inManifold,
                       JPH::ContactSettings& ioSettings) override {
        if (!entityLookup_) return;

        Entity entityA = entityLookup_(inBody1.GetID());
        Entity entityB = entityLookup_(inBody2.GetID());

        // Store contact pair
        auto key = makeContactKey(inBody1.GetID(), inBody2.GetID());
        ContactPair3D pair;
        pair.entityA = entityA;
        pair.entityB = entityB;

        // Extract contact points
        for (JPH::uint i = 0; i < inManifold.mRelativeContactPointsOn1.size(); ++i) {
            ContactPoint3D pt;
            pt.worldPositionOnA = fromJolt(inManifold.mBaseOffset + inManifold.mRelativeContactPointsOn1[i]);
            pt.worldPositionOnB = fromJolt(inManifold.mBaseOffset + inManifold.mRelativeContactPointsOn2[i]);
            pt.worldNormalOnB = fromJoltVec3(inManifold.mWorldSpaceNormal);
            pt.penetrationDepth = inManifold.mPenetrationDepth;
            pt.combinedFriction = ioSettings.mCombinedFriction;
            pt.combinedRestitution = ioSettings.mCombinedRestitution;
            pair.contacts.push_back(pt);
        }
        pair.impulse = 0.0f;  // Not available at contact added stage
        pair.isActive = true;
        activeContacts_[key] = pair;

        if (inBody1.IsSensor() || inBody2.IsSensor()) {
            if (triggerEnterCallback_) {
                TriggerEvent3D event{
                    .entityA = entityA,
                    .entityB = entityB
                };
                triggerEnterCallback_(event);
            }
        } else if (collisionCallback_) {
            CollisionEvent3D event{
                .entityA = entityA,
                .entityB = entityB,
                .contactPoint = fromJolt(inManifold.mBaseOffset + inManifold.mRelativeContactPointsOn1[0]),
                .contactNormal = fromJoltVec3(inManifold.mWorldSpaceNormal),
                .impulse = 0.0f,
                .penetrationDepth = inManifold.mPenetrationDepth
            };
            collisionCallback_(event);
        }
    }

    void OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2,
                           const JPH::ContactManifold& inManifold,
                           JPH::ContactSettings& ioSettings) override {
        if (!entityLookup_) return;

        // Update contact pair
        auto key = makeContactKey(inBody1.GetID(), inBody2.GetID());
        auto it = activeContacts_.find(key);
        if (it != activeContacts_.end()) {
            it->second.contacts.clear();
            for (JPH::uint i = 0; i < inManifold.mRelativeContactPointsOn1.size(); ++i) {
                ContactPoint3D pt;
                pt.worldPositionOnA = fromJolt(inManifold.mBaseOffset + inManifold.mRelativeContactPointsOn1[i]);
                pt.worldPositionOnB = fromJolt(inManifold.mBaseOffset + inManifold.mRelativeContactPointsOn2[i]);
                pt.worldNormalOnB = fromJoltVec3(inManifold.mWorldSpaceNormal);
                pt.penetrationDepth = inManifold.mPenetrationDepth;
                pt.combinedFriction = ioSettings.mCombinedFriction;
                pt.combinedRestitution = ioSettings.mCombinedRestitution;
                it->second.contacts.push_back(pt);
            }
            it->second.isActive = true;
        }
    }

    void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override {
        auto key = makeContactKey(inSubShapePair.GetBody1ID(), inSubShapePair.GetBody2ID());
        auto it = activeContacts_.find(key);
        if (it != activeContacts_.end()) {
            // Fire trigger exit callback if applicable
            if (triggerExitCallback_ && entityLookup_) {
                TriggerEvent3D event{
                    .entityA = it->second.entityA,
                    .entityB = it->second.entityB
                };
                triggerExitCallback_(event);
            }
            activeContacts_.erase(it);
        }
    }

    // Contact query methods
    std::vector<ContactPair3D> getContacts(Entity entity) const {
        std::vector<ContactPair3D> result;
        for (const auto& [key, pair] : activeContacts_) {
            if (pair.entityA == entity || pair.entityB == entity) {
                result.push_back(pair);
            }
        }
        return result;
    }

    std::vector<ContactPair3D> getAllContacts() const {
        std::vector<ContactPair3D> result;
        result.reserve(activeContacts_.size());
        for (const auto& [key, pair] : activeContacts_) {
            result.push_back(pair);
        }
        return result;
    }

    bool areInContact(Entity a, Entity b) const {
        for (const auto& [key, pair] : activeContacts_) {
            if ((pair.entityA == a && pair.entityB == b) ||
                (pair.entityA == b && pair.entityB == a)) {
                return true;
            }
        }
        return false;
    }

    std::optional<ContactPair3D> getContactPair(Entity a, Entity b) const {
        for (const auto& [key, pair] : activeContacts_) {
            if ((pair.entityA == a && pair.entityB == b) ||
                (pair.entityA == b && pair.entityB == a)) {
                return pair;
            }
        }
        return std::nullopt;
    }

private:
    std::uint64_t makeContactKey(JPH::BodyID a, JPH::BodyID b) const {
        std::uint32_t idA = a.GetIndex();
        std::uint32_t idB = b.GetIndex();
        if (idA > idB) std::swap(idA, idB);
        return (static_cast<std::uint64_t>(idA) << 32) | idB;
    }

    Collision3DCallback collisionCallback_;
    Trigger3DEnterCallback triggerEnterCallback_;
    Trigger3DExitCallback triggerExitCallback_;
    std::function<Entity(JPH::BodyID)> entityLookup_;
    std::unordered_map<std::uint64_t, ContactPair3D> activeContacts_;
};

//==========================================================================
// Jolt Physics 3D System Implementation
//==========================================================================

class JoltPhysics3DSystem : public IPhysics3DSystem {
public:
    JoltPhysics3DSystem();
    ~JoltPhysics3DSystem() override;

    bool initialize();

    //======================================================================
    // Lifecycle
    //======================================================================

    void update(DeltaTime dt, int subSteps = 1) override;
    void syncTransforms(std::span<const Entity> entities) override;

    //======================================================================
    // Body Management
    //======================================================================

    Result<void, Physics3DError> createBody(Entity entity, const PhysicsBodyDef3D& def) override;
    Result<void, Physics3DError> destroyBody(Entity entity) override;
    bool hasBody(Entity entity) const override;
    std::vector<Entity> getAllBodies() const override;

    //======================================================================
    // Body Type
    //======================================================================

    Result<void, Physics3DError> setBodyType(Entity entity, BodyType3D type) override;
    Result<BodyType3D, Physics3DError> getBodyType(Entity entity) const override;

    //======================================================================
    // Transform
    //======================================================================

    Result<void, Physics3DError> setTransform(Entity entity, const Transform3D& transform) override;
    Result<Transform3D, Physics3DError> getTransform(Entity entity) const override;

    Result<void, Physics3DError> setPosition(Entity entity, Vec3 position) override;
    Result<Vec3, Physics3DError> getPosition(Entity entity) const override;

    Result<void, Physics3DError> setRotation(Entity entity, Quat rotation) override;
    Result<Quat, Physics3DError> getRotation(Entity entity) const override;

    //======================================================================
    // Velocity
    //======================================================================

    Result<void, Physics3DError> setLinearVelocity(Entity entity, Vec3 velocity) override;
    Result<Vec3, Physics3DError> getLinearVelocity(Entity entity) const override;

    Result<void, Physics3DError> setAngularVelocity(Entity entity, Vec3 velocity) override;
    Result<Vec3, Physics3DError> getAngularVelocity(Entity entity) const override;

    //======================================================================
    // Forces
    //======================================================================

    Result<void, Physics3DError> applyForce(Entity entity, Vec3 force) override;
    Result<void, Physics3DError> applyForceAtPoint(Entity entity, Vec3 force, Vec3 worldPoint) override;
    Result<void, Physics3DError> applyTorque(Entity entity, Vec3 torque) override;

    Result<void, Physics3DError> applyImpulse(Entity entity, Vec3 impulse) override;
    Result<void, Physics3DError> applyImpulseAtPoint(Entity entity, Vec3 impulse, Vec3 worldPoint) override;
    Result<void, Physics3DError> applyAngularImpulse(Entity entity, Vec3 impulse) override;

    //======================================================================
    // Body Properties
    //======================================================================

    Result<void, Physics3DError> setMass(Entity entity, float mass) override;
    Result<float, Physics3DError> getMass(Entity entity) const override;

    Result<void, Physics3DError> setLinearDamping(Entity entity, float damping) override;
    Result<void, Physics3DError> setAngularDamping(Entity entity, float damping) override;
    Result<void, Physics3DError> setGravityFactor(Entity entity, float factor) override;

    Result<void, Physics3DError> setFriction(Entity entity, float friction) override;
    Result<void, Physics3DError> setRestitution(Entity entity, float restitution) override;

    //======================================================================
    // Collision Filtering
    //======================================================================

    Result<void, Physics3DError> setCollisionLayer(Entity entity, CollisionLayer3D layer) override;
    Result<void, Physics3DError> setCollisionMask(Entity entity, CollisionMask3D mask) override;
    Result<void, Physics3DError> setSensor(Entity entity, bool isSensor) override;

    //======================================================================
    // Compound Shapes
    //======================================================================

    Result<void, Physics3DError> createCompoundBody(
        Entity entity,
        BodyType3D type,
        const Transform3D& transform,
        const CompoundShapeDef& shape) override;
    Result<std::uint32_t, Physics3DError> addShape(Entity entity, const BoxShapeDef& shape) override;
    Result<std::uint32_t, Physics3DError> addShape(Entity entity, const SphereShapeDef& shape) override;
    Result<std::uint32_t, Physics3DError> addShape(Entity entity, const CapsuleShapeDef& shape) override;
    Result<void, Physics3DError> removeShape(Entity entity, std::uint32_t shapeIndex) override;
    Result<std::uint32_t, Physics3DError> getShapeCount(Entity entity) const override;
    Result<void, Physics3DError> createHeightFieldBody(
        Entity entity,
        const Transform3D& transform,
        const HeightFieldShapeDef& shape) override;

    //======================================================================
    // Mass Properties
    //======================================================================

    Result<void, Physics3DError> setMassProperties(Entity entity, const MassProperties& props) override;
    Result<MassProperties, Physics3DError> getMassProperties(Entity entity) const override;
    Result<Vec3, Physics3DError> getCenterOfMass(Entity entity) const override;
    Result<Mat3, Physics3DError> getInertiaTensor(Entity entity) const override;

    //======================================================================
    // Motion Quality (CCD)
    //======================================================================

    Result<void, Physics3DError> setMotionQuality(Entity entity, MotionQuality quality) override;
    Result<MotionQuality, Physics3DError> getMotionQuality(Entity entity) const override;

    //======================================================================
    // Contact Queries
    //======================================================================

    std::vector<ContactPair3D> getContacts(Entity entity) const override;
    std::vector<ContactPair3D> getAllContacts() const override;
    bool areInContact(Entity a, Entity b) const override;
    std::optional<ContactPair3D> getContactPair(Entity a, Entity b) const override;

    //======================================================================
    // Sleep State
    //======================================================================

    Result<bool, Physics3DError> isAwake(Entity entity) const override;
    Result<void, Physics3DError> wakeUp(Entity entity) override;
    Result<void, Physics3DError> putToSleep(Entity entity) override;

    //======================================================================
    // Bounding Box
    //======================================================================

    Result<AABB3D, Physics3DError> getBodyBounds(Entity entity) const override;

    //======================================================================
    // Raycasting
    //======================================================================

    std::optional<RaycastHit3D> raycast(Vec3 origin, Vec3 direction, float maxDistance,
                                        const QueryFilter3D& filter = {}) const override;

    std::vector<RaycastHit3D> raycastAll(Vec3 origin, Vec3 direction, float maxDistance,
                                         const QueryFilter3D& filter = {}) const override;

    //======================================================================
    // Shape Casting
    //======================================================================

    std::optional<ShapeCastHit3D> sphereCast(Vec3 origin, float radius, Vec3 direction,
                                             float maxDistance,
                                             const QueryFilter3D& filter = {}) const override;

    std::optional<ShapeCastHit3D> boxCast(Vec3 origin, Vec3 halfExtents, Quat rotation,
                                          Vec3 direction, float maxDistance,
                                          const QueryFilter3D& filter = {}) const override;

    std::optional<ShapeCastHit3D> capsuleCast(Vec3 origin, float radius, float halfHeight,
                                              Quat rotation, Vec3 direction, float maxDistance,
                                              const QueryFilter3D& filter = {}) const override;

    //======================================================================
    // Overlap Queries
    //======================================================================

    std::vector<Entity> overlapSphere(Vec3 center, float radius,
                                      const QueryFilter3D& filter = {}) const override;

    std::vector<Entity> overlapBox(Vec3 center, Vec3 halfExtents, Quat rotation,
                                   const QueryFilter3D& filter = {}) const override;

    std::vector<Entity> queryAABB(Vec3 min, Vec3 max,
                                  const QueryFilter3D& filter = {}) const override;

    //======================================================================
    // Constraints
    //======================================================================

    Result<UUID, Physics3DError> createConstraint(const ConstraintDef3D& def) override;
    Result<void, Physics3DError> destroyConstraint(UUID constraintId) override;
    Result<void, Physics3DError> setConstraintEnabled(UUID constraintId, bool enabled) override;
    std::vector<UUID> getConstraints(Entity entity) const override;

    Result<void, Physics3DError> setHingeLimits(UUID constraintId, float minAngle, float maxAngle) override;
    Result<void, Physics3DError> setHingeMotor(UUID constraintId, float targetVelocity, float maxTorque) override;
    Result<void, Physics3DError> setSliderLimits(UUID constraintId, float minDistance, float maxDistance) override;
    Result<void, Physics3DError> setSliderMotor(UUID constraintId, float targetVelocity, float maxForce) override;
    Result<float, Physics3DError> getConstraintForce(UUID constraintId) const override;

    //======================================================================
    // Character Controller
    //======================================================================

    Result<void, Physics3DError> createCharacter(Entity entity,
                                                  const CharacterControllerDef& def) override;
    Result<void, Physics3DError> destroyCharacter(Entity entity) override;

    Result<void, Physics3DError> moveCharacter(Entity entity, Vec3 velocity, DeltaTime dt) override;

    Result<Vec3, Physics3DError> getCharacterPosition(Entity entity) const override;
    Result<void, Physics3DError> setCharacterPosition(Entity entity, Vec3 position) override;

    Result<CharacterGroundInfo, Physics3DError> getCharacterGroundInfo(Entity entity) const override;
    Result<Vec3, Physics3DError> getCharacterVelocity(Entity entity) const override;

    //======================================================================
    // Vehicle
    //======================================================================

    Result<void, Physics3DError> createVehicle(Entity entity, const VehicleDef& def) override;
    Result<void, Physics3DError> destroyVehicle(Entity entity) override;

    Result<void, Physics3DError> updateVehicle(Entity entity, float throttle,
                                               float steering, float brake) override;

    Result<WheelState, Physics3DError> getWheelTransform(Entity entity,
                                                          std::uint32_t wheelIndex) const override;
    Result<bool, Physics3DError> isWheelGrounded(Entity entity,
                                                  std::uint32_t wheelIndex) const override;
    Result<float, Physics3DError> getVehicleSpeed(Entity entity) const override;

    //======================================================================
    // World Settings
    //======================================================================

    void setGravity(Vec3 gravity) override;
    Vec3 getGravity() const override;

    //======================================================================
    // Collision Callbacks
    //======================================================================

    void setCollisionCallback(Collision3DCallback callback) override;
    void setTriggerEnterCallback(Trigger3DEnterCallback callback) override;
    void setTriggerExitCallback(Trigger3DExitCallback callback) override;

    //======================================================================
    // Debug Visualization
    //======================================================================

    void setDebugDraw(bool enabled) override;
    std::vector<DebugLine3D> getDebugLines() const override;

    //======================================================================
    // Statistics
    //======================================================================

    PhysicsStats3D getStats() const override;

private:
    JPH::BodyID* getBodyID(Entity entity) const;
    Entity getEntity(JPH::BodyID bodyId) const;
    JPH::Ref<JPH::Shape> createShape(const PhysicsBodyDef3D& def);

    // Jolt Physics System
    std::unique_ptr<JPH::PhysicsSystem> physicsSystem_;
    std::unique_ptr<JPH::TempAllocatorImpl> tempAllocator_;
    std::unique_ptr<JPH::JobSystemThreadPool> jobSystem_;

    // Layer interfaces
    BPLayerInterfaceImpl broadPhaseLayerInterface_;
    ObjectVsBroadPhaseLayerFilterImpl objectVsBroadPhaseLayerFilter_;
    ObjectLayerPairFilterImpl objectLayerPairFilter_;

    // Contact listener
    mutable ContactListenerImpl contactListener_;

    // Entity mappings
    std::unordered_map<std::uint32_t, JPH::BodyID> entityToBody_;
    std::unordered_map<std::uint32_t, Entity> bodyIdToEntity_;

    // Character controllers
    std::unordered_map<std::uint32_t, std::unique_ptr<JPH::CharacterVirtual>> characters_;

    // Constraints
    struct ConstraintData {
        JPH::Ref<JPH::Constraint> constraint;
        ConstraintType3D type;
        Entity entityA;
        Entity entityB;
    };
    std::unordered_map<std::uint64_t, ConstraintData> constraints_;
    std::uint64_t nextConstraintId_ = 1;

    // Vehicles (stubbed for now - VehicleConstraint needs proper setup)
    // struct VehicleData {
    //     JPH::Ref<JPH::VehicleConstraint> constraint;
    //     JPH::WheeledVehicleController* controller;
    // };
    // std::unordered_map<std::uint32_t, VehicleData> vehicles_;

    // Old constraints storage (superseded by ConstraintData above)
    // std::unordered_map<std::uint64_t, JPH::Ref<JPH::Constraint>> constraints_;
    // std::unordered_map<std::uint32_t, std::vector<std::uint64_t>> entityConstraints_;

    // World settings
    Vec3 gravity_{0.0f, -9.81f, 0.0f};

    // Debug
    bool debugDrawEnabled_ = false;
    mutable std::vector<DebugLine3D> debugLines_;

    // Stats
    mutable PhysicsStats3D lastStats_;

    bool initialized_ = false;
};

//==========================================================================
// Implementation
//==========================================================================

JoltPhysics3DSystem::JoltPhysics3DSystem() = default;

JoltPhysics3DSystem::~JoltPhysics3DSystem() {
    // Clean up in reverse order
    characters_.clear();
    // vehicles_.clear();  // Vehicles stubbed out for now
    constraints_.clear();
    entityToBody_.clear();
    bodyIdToEntity_.clear();

    physicsSystem_.reset();
    jobSystem_.reset();
    tempAllocator_.reset();

    // Unregister Jolt types (only if we're the last instance)
    // JPH::UnregisterTypes();
    // delete JPH::Factory::sInstance;
    // JPH::Factory::sInstance = nullptr;
}

bool JoltPhysics3DSystem::initialize() {
    if (initialized_) return true;

    // Register allocation hook (only once globally)
    static bool allocatorRegistered = false;
    if (!allocatorRegistered) {
        JPH::RegisterDefaultAllocator();
        allocatorRegistered = true;
    }

    // Create factory (only once globally)
    if (!JPH::Factory::sInstance) {
        JPH::Factory::sInstance = new JPH::Factory();
    }

    // Register types (only once globally)
    static bool typesRegistered = false;
    if (!typesRegistered) {
        JPH::RegisterTypes();
        typesRegistered = true;
    }

    // Create temp allocator (10 MB)
    tempAllocator_ = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);

    // Create job system
    const int numThreads = std::max(1, static_cast<int>(std::thread::hardware_concurrency()) - 1);
    jobSystem_ = std::make_unique<JPH::JobSystemThreadPool>(
        JPH::cMaxPhysicsJobs,
        JPH::cMaxPhysicsBarriers,
        numThreads
    );

    // Create physics system
    constexpr int cMaxBodies = 10240;
    constexpr int cNumBodyMutexes = 0;  // Auto
    constexpr int cMaxBodyPairs = 65536;
    constexpr int cMaxContactConstraints = 10240;

    physicsSystem_ = std::make_unique<JPH::PhysicsSystem>();
    physicsSystem_->Init(
        cMaxBodies,
        cNumBodyMutexes,
        cMaxBodyPairs,
        cMaxContactConstraints,
        broadPhaseLayerInterface_,
        objectVsBroadPhaseLayerFilter_,
        objectLayerPairFilter_
    );

    // Set gravity
    physicsSystem_->SetGravity(toJolt(gravity_));

    // Setup contact listener
    contactListener_.setEntityLookup([this](JPH::BodyID bodyId) -> Entity {
        return getEntity(bodyId);
    });
    physicsSystem_->SetContactListener(&contactListener_);

    initialized_ = true;
    return true;
}

void JoltPhysics3DSystem::update(DeltaTime dt, int subSteps) {
    if (!initialized_) return;

    const int collisionSteps = std::max(1, subSteps);
    physicsSystem_->Update(dt, collisionSteps, tempAllocator_.get(), jobSystem_.get());
}

void JoltPhysics3DSystem::syncTransforms(std::span<const Entity> entities) {
    // NOTE: This method is intended to copy physics transforms back to entity components.
    // Since we don't have access to the entity system here, this is a no-op.
    // The game layer should call getPosition/getRotation for each entity after update().
}

Result<void, Physics3DError> JoltPhysics3DSystem::createBody(Entity entity, const PhysicsBodyDef3D& def) {
    if (!initialized_) return std::unexpected(Physics3DError::InternalError);
    if (hasBody(entity)) return std::unexpected(Physics3DError::InvalidEntity);

    auto shape = createShape(def);
    if (!shape) return std::unexpected(Physics3DError::InvalidShape);

    JPH::BodyCreationSettings settings(
        shape,
        toJolt(def.transform.position),
        toJolt(def.transform.rotation),
        toJoltMotionType(def.type),
        def.type == BodyType3D::Static ? LAYER_NON_MOVING : LAYER_MOVING
    );

    settings.mFriction = def.friction;
    settings.mRestitution = def.restitution;
    settings.mLinearDamping = def.linearDamping;
    settings.mAngularDamping = def.angularDamping;
    settings.mGravityFactor = def.gravityFactor;
    settings.mAllowSleeping = def.allowSleep;
    settings.mIsSensor = def.isSensor;

    JPH::Body* body = physicsSystem_->GetBodyInterface().CreateBody(settings);
    if (!body) return std::unexpected(Physics3DError::OutOfMemory);

    JPH::BodyID bodyId = body->GetID();
    physicsSystem_->GetBodyInterface().AddBody(bodyId, JPH::EActivation::Activate);

    entityToBody_[static_cast<std::uint32_t>(entity)] = bodyId;
    bodyIdToEntity_[bodyId.GetIndexAndSequenceNumber()] = entity;

    return {};
}

Result<void, Physics3DError> JoltPhysics3DSystem::destroyBody(Entity entity) {
    if (!initialized_) return std::unexpected(Physics3DError::InternalError);

    auto* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    physicsSystem_->GetBodyInterface().RemoveBody(*bodyId);
    physicsSystem_->GetBodyInterface().DestroyBody(*bodyId);

    bodyIdToEntity_.erase(bodyId->GetIndexAndSequenceNumber());
    entityToBody_.erase(static_cast<std::uint32_t>(entity));

    return {};
}

bool JoltPhysics3DSystem::hasBody(Entity entity) const {
    return entityToBody_.contains(static_cast<std::uint32_t>(entity));
}

std::vector<Entity> JoltPhysics3DSystem::getAllBodies() const {
    std::vector<Entity> result;
    result.reserve(entityToBody_.size());
    for (const auto& [entityId, _] : entityToBody_) {
        result.push_back(static_cast<Entity>(entityId));
    }
    return result;
}

Result<void, Physics3DError> JoltPhysics3DSystem::setBodyType(Entity entity, BodyType3D type) {
    auto* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    physicsSystem_->GetBodyInterface().SetMotionType(*bodyId, toJoltMotionType(type), JPH::EActivation::Activate);
    return {};
}

Result<BodyType3D, Physics3DError> JoltPhysics3DSystem::getBodyType(Entity entity) const {
    auto* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    JPH::EMotionType motionType = physicsSystem_->GetBodyInterface().GetMotionType(*bodyId);
    return fromJoltMotionType(motionType);
}

Result<void, Physics3DError> JoltPhysics3DSystem::setTransform(Entity entity, const Transform3D& transform) {
    auto* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    physicsSystem_->GetBodyInterface().SetPositionAndRotation(
        *bodyId,
        toJolt(transform.position),
        toJolt(transform.rotation),
        JPH::EActivation::Activate
    );
    return {};
}

Result<Transform3D, Physics3DError> JoltPhysics3DSystem::getTransform(Entity entity) const {
    auto* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    JPH::RVec3 position = physicsSystem_->GetBodyInterface().GetPosition(*bodyId);
    JPH::Quat rotation = physicsSystem_->GetBodyInterface().GetRotation(*bodyId);

    return Transform3D{
        .position = fromJolt(position),
        .rotation = fromJolt(rotation),
        .scale = Vec3{1.0f, 1.0f, 1.0f}
    };
}

Result<void, Physics3DError> JoltPhysics3DSystem::setPosition(Entity entity, Vec3 position) {
    auto* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    physicsSystem_->GetBodyInterface().SetPosition(*bodyId, toJolt(position), JPH::EActivation::Activate);
    return {};
}

Result<Vec3, Physics3DError> JoltPhysics3DSystem::getPosition(Entity entity) const {
    auto* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    return fromJolt(physicsSystem_->GetBodyInterface().GetPosition(*bodyId));
}

Result<void, Physics3DError> JoltPhysics3DSystem::setRotation(Entity entity, Quat rotation) {
    auto* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    physicsSystem_->GetBodyInterface().SetRotation(*bodyId, toJolt(rotation), JPH::EActivation::Activate);
    return {};
}

Result<Quat, Physics3DError> JoltPhysics3DSystem::getRotation(Entity entity) const {
    auto* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    return fromJolt(physicsSystem_->GetBodyInterface().GetRotation(*bodyId));
}

Result<void, Physics3DError> JoltPhysics3DSystem::setLinearVelocity(Entity entity, Vec3 velocity) {
    auto* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    physicsSystem_->GetBodyInterface().SetLinearVelocity(*bodyId, toJolt(velocity));
    return {};
}

Result<Vec3, Physics3DError> JoltPhysics3DSystem::getLinearVelocity(Entity entity) const {
    auto* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    return fromJolt(physicsSystem_->GetBodyInterface().GetLinearVelocity(*bodyId));
}

Result<void, Physics3DError> JoltPhysics3DSystem::setAngularVelocity(Entity entity, Vec3 velocity) {
    auto* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    physicsSystem_->GetBodyInterface().SetAngularVelocity(*bodyId, toJolt(velocity));
    return {};
}

Result<Vec3, Physics3DError> JoltPhysics3DSystem::getAngularVelocity(Entity entity) const {
    auto* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    return fromJolt(physicsSystem_->GetBodyInterface().GetAngularVelocity(*bodyId));
}

Result<void, Physics3DError> JoltPhysics3DSystem::applyForce(Entity entity, Vec3 force) {
    auto* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    physicsSystem_->GetBodyInterface().AddForce(*bodyId, toJolt(force));
    return {};
}

Result<void, Physics3DError> JoltPhysics3DSystem::applyForceAtPoint(Entity entity, Vec3 force, Vec3 worldPoint) {
    auto* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    physicsSystem_->GetBodyInterface().AddForce(*bodyId, toJolt(force), toJolt(worldPoint));
    return {};
}

Result<void, Physics3DError> JoltPhysics3DSystem::applyTorque(Entity entity, Vec3 torque) {
    auto* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    physicsSystem_->GetBodyInterface().AddTorque(*bodyId, toJolt(torque));
    return {};
}

Result<void, Physics3DError> JoltPhysics3DSystem::applyImpulse(Entity entity, Vec3 impulse) {
    auto* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    physicsSystem_->GetBodyInterface().AddImpulse(*bodyId, toJolt(impulse));
    return {};
}

Result<void, Physics3DError> JoltPhysics3DSystem::applyImpulseAtPoint(Entity entity, Vec3 impulse, Vec3 worldPoint) {
    auto* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    physicsSystem_->GetBodyInterface().AddImpulse(*bodyId, toJolt(impulse), toJolt(worldPoint));
    return {};
}

Result<void, Physics3DError> JoltPhysics3DSystem::applyAngularImpulse(Entity entity, Vec3 impulse) {
    auto* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    physicsSystem_->GetBodyInterface().AddAngularImpulse(*bodyId, toJolt(impulse));
    return {};
}

Result<void, Physics3DError> JoltPhysics3DSystem::setMass(Entity entity, float mass) {
    // TODO: Implement mass override (Jolt requires mass properties recalculation)
    return std::unexpected(Physics3DError::InvalidConfiguration);
}

Result<float, Physics3DError> JoltPhysics3DSystem::getMass(Entity entity) const {
    auto* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    const JPH::Body* body = physicsSystem_->GetBodyLockInterface().TryGetBody(*bodyId);
    if (!body) return std::unexpected(Physics3DError::BodyNotFound);

    return 1.0f / body->GetMotionProperties()->GetInverseMass();
}

Result<void, Physics3DError> JoltPhysics3DSystem::setLinearDamping(Entity entity, float damping) {
    auto* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    JPH::Body* body = physicsSystem_->GetBodyLockInterface().TryGetBody(*bodyId);
    if (!body) return std::unexpected(Physics3DError::BodyNotFound);

    body->GetMotionProperties()->SetLinearDamping(damping);
    return {};
}

Result<void, Physics3DError> JoltPhysics3DSystem::setAngularDamping(Entity entity, float damping) {
    auto* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    JPH::Body* body = physicsSystem_->GetBodyLockInterface().TryGetBody(*bodyId);
    if (!body) return std::unexpected(Physics3DError::BodyNotFound);

    body->GetMotionProperties()->SetAngularDamping(damping);
    return {};
}

Result<void, Physics3DError> JoltPhysics3DSystem::setGravityFactor(Entity entity, float factor) {
    auto* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    JPH::Body* body = physicsSystem_->GetBodyLockInterface().TryGetBody(*bodyId);
    if (!body) return std::unexpected(Physics3DError::BodyNotFound);

    body->GetMotionProperties()->SetGravityFactor(factor);
    return {};
}

Result<void, Physics3DError> JoltPhysics3DSystem::setFriction(Entity entity, float friction) {
    auto* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    physicsSystem_->GetBodyInterface().SetFriction(*bodyId, friction);
    return {};
}

Result<void, Physics3DError> JoltPhysics3DSystem::setRestitution(Entity entity, float restitution) {
    auto* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    physicsSystem_->GetBodyInterface().SetRestitution(*bodyId, restitution);
    return {};
}

Result<void, Physics3DError> JoltPhysics3DSystem::setCollisionLayer(Entity entity, CollisionLayer3D layer) {
    // TODO: Implement collision layer filtering with custom ObjectLayerFilter
    return std::unexpected(Physics3DError::InvalidConfiguration);
}

Result<void, Physics3DError> JoltPhysics3DSystem::setCollisionMask(Entity entity, CollisionMask3D mask) {
    // TODO: Implement collision mask filtering with custom ObjectLayerFilter
    return std::unexpected(Physics3DError::InvalidConfiguration);
}

Result<void, Physics3DError> JoltPhysics3DSystem::setSensor(Entity entity, bool isSensor) {
    auto* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    physicsSystem_->GetBodyInterface().SetIsSensor(*bodyId, isSensor);
    return {};
}

Result<bool, Physics3DError> JoltPhysics3DSystem::isAwake(Entity entity) const {
    auto* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    return physicsSystem_->GetBodyInterface().IsActive(*bodyId);
}

Result<void, Physics3DError> JoltPhysics3DSystem::wakeUp(Entity entity) {
    auto* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    physicsSystem_->GetBodyInterface().ActivateBody(*bodyId);
    return {};
}

Result<void, Physics3DError> JoltPhysics3DSystem::putToSleep(Entity entity) {
    auto* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    physicsSystem_->GetBodyInterface().DeactivateBody(*bodyId);
    return {};
}

Result<AABB3D, Physics3DError> JoltPhysics3DSystem::getBodyBounds(Entity entity) const {
    auto* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    const JPH::Body* body = physicsSystem_->GetBodyLockInterface().TryGetBody(*bodyId);
    if (!body) return std::unexpected(Physics3DError::BodyNotFound);

    JPH::AABox bounds = body->GetWorldSpaceBounds();
    return AABB3D{
        .min = fromJolt(bounds.mMin),
        .max = fromJolt(bounds.mMax)
    };
}

std::optional<RaycastHit3D> JoltPhysics3DSystem::raycast(Vec3 origin, Vec3 direction, float maxDistance,
                                                         const QueryFilter3D& filter) const {
    JPH::RRayCast ray{toJolt(origin), toJolt(direction) * maxDistance};
    JPH::RayCastResult hit;

    if (physicsSystem_->GetNarrowPhaseQuery().CastRay(ray, hit)) {
        JPH::BodyID hitBodyId = hit.mBodyID;
        Entity hitEntity = getEntity(hitBodyId);

        // Get the actual surface normal
        Vec3 normal{0.0f, 1.0f, 0.0f};
        JPH::BodyLockRead lock(physicsSystem_->GetBodyLockInterface(), hitBodyId);
        if (lock.Succeeded()) {
            const JPH::Body& body = lock.GetBody();
            JPH::Vec3 hitPoint = ray.GetPointOnRay(hit.mFraction);
            normal = fromJoltVec3(body.GetShape()->GetSurfaceNormal(hit.mSubShapeID2, hitPoint));
        }

        return RaycastHit3D{
            .entity = hitEntity,
            .point = fromJolt(ray.GetPointOnRay(hit.mFraction)),
            .normal = normal,
            .distance = maxDistance * hit.mFraction
        };
    }

    return std::nullopt;
}

std::vector<RaycastHit3D> JoltPhysics3DSystem::raycastAll(Vec3 origin, Vec3 direction, float maxDistance,
                                                          const QueryFilter3D& filter) const {
    std::vector<RaycastHit3D> results;

    JPH::RRayCast ray{toJolt(origin), toJolt(direction) * maxDistance};
    JPH::AllHitCollisionCollector<JPH::CastRayCollector> collector;

    physicsSystem_->GetNarrowPhaseQuery().CastRay(ray, JPH::RayCastSettings(), collector);

    for (const JPH::RayCastResult& hit : collector.mHits) {
        Entity hitEntity = getEntity(hit.mBodyID);

        Vec3 normal{0.0f, 1.0f, 0.0f};
        JPH::BodyLockRead lock(physicsSystem_->GetBodyLockInterface(), hit.mBodyID);
        if (lock.Succeeded()) {
            const JPH::Body& body = lock.GetBody();
            JPH::Vec3 hitPoint = ray.GetPointOnRay(hit.mFraction);
            normal = fromJoltVec3(body.GetShape()->GetSurfaceNormal(hit.mSubShapeID2, hitPoint));
        }

        results.push_back(RaycastHit3D{
            .entity = hitEntity,
            .point = fromJolt(ray.GetPointOnRay(hit.mFraction)),
            .normal = normal,
            .distance = maxDistance * hit.mFraction
        });
    }

    // Sort by distance
    std::sort(results.begin(), results.end(),
              [](const RaycastHit3D& a, const RaycastHit3D& b) { return a.distance < b.distance; });

    return results;
}

std::optional<ShapeCastHit3D> JoltPhysics3DSystem::sphereCast(Vec3 origin, float radius, Vec3 direction,
                                                              float maxDistance,
                                                              const QueryFilter3D& filter) const {
    JPH::SphereShape sphere(radius);
    JPH::RShapeCast shapeCast(&sphere, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sTranslation(toJolt(origin)),
                               toJolt(direction) * maxDistance);

    JPH::ClosestHitCollisionCollector<JPH::CastShapeCollector> collector;
    JPH::ShapeCastSettings settings;

    physicsSystem_->GetNarrowPhaseQuery().CastShape(shapeCast, settings, JPH::RVec3::sZero(), collector);

    if (collector.HadHit()) {
        const JPH::ShapeCastResult& hit = collector.mHit;
        Entity hitEntity = getEntity(hit.mBodyID2);

        return ShapeCastHit3D{
            .entity = hitEntity,
            .point = fromJolt(hit.mContactPointOn2),
            .normal = fromJoltVec3(hit.mPenetrationAxis.Normalized()),
            .distance = hit.mFraction * maxDistance,
            .penetrationDepth = Vec3{0.0f}
        };
    }

    return std::nullopt;
}

std::optional<ShapeCastHit3D> JoltPhysics3DSystem::boxCast(Vec3 origin, Vec3 halfExtents, Quat rotation,
                                                           Vec3 direction, float maxDistance,
                                                           const QueryFilter3D& filter) const {
    JPH::BoxShape box(toJolt(halfExtents));
    JPH::RMat44 startTransform = JPH::RMat44::sRotationTranslation(toJolt(rotation), toJolt(origin));
    JPH::RShapeCast shapeCast(&box, JPH::Vec3::sReplicate(1.0f), startTransform,
                               toJolt(direction) * maxDistance);

    JPH::ClosestHitCollisionCollector<JPH::CastShapeCollector> collector;
    JPH::ShapeCastSettings settings;

    physicsSystem_->GetNarrowPhaseQuery().CastShape(shapeCast, settings, JPH::RVec3::sZero(), collector);

    if (collector.HadHit()) {
        const JPH::ShapeCastResult& hit = collector.mHit;
        Entity hitEntity = getEntity(hit.mBodyID2);

        return ShapeCastHit3D{
            .entity = hitEntity,
            .point = fromJolt(hit.mContactPointOn2),
            .normal = fromJoltVec3(hit.mPenetrationAxis.Normalized()),
            .distance = hit.mFraction * maxDistance,
            .penetrationDepth = Vec3{0.0f}
        };
    }

    return std::nullopt;
}

std::optional<ShapeCastHit3D> JoltPhysics3DSystem::capsuleCast(Vec3 origin, float radius, float halfHeight,
                                                               Quat rotation, Vec3 direction, float maxDistance,
                                                               const QueryFilter3D& filter) const {
    JPH::CapsuleShape capsule(halfHeight, radius);
    JPH::RMat44 startTransform = JPH::RMat44::sRotationTranslation(toJolt(rotation), toJolt(origin));
    JPH::RShapeCast shapeCast(&capsule, JPH::Vec3::sReplicate(1.0f), startTransform,
                               toJolt(direction) * maxDistance);

    JPH::ClosestHitCollisionCollector<JPH::CastShapeCollector> collector;
    JPH::ShapeCastSettings settings;

    physicsSystem_->GetNarrowPhaseQuery().CastShape(shapeCast, settings, JPH::RVec3::sZero(), collector);

    if (collector.HadHit()) {
        const JPH::ShapeCastResult& hit = collector.mHit;
        Entity hitEntity = getEntity(hit.mBodyID2);

        return ShapeCastHit3D{
            .entity = hitEntity,
            .point = fromJolt(hit.mContactPointOn2),
            .normal = fromJoltVec3(hit.mPenetrationAxis.Normalized()),
            .distance = hit.mFraction * maxDistance,
            .penetrationDepth = Vec3{0.0f}
        };
    }

    return std::nullopt;
}

std::vector<Entity> JoltPhysics3DSystem::overlapSphere(Vec3 center, float radius,
                                                       const QueryFilter3D& filter) const {
    std::vector<Entity> results;

    JPH::SphereShape sphere(radius);
    JPH::RMat44 transform = JPH::RMat44::sTranslation(toJolt(center));

    JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;
    physicsSystem_->GetNarrowPhaseQuery().CollideShape(&sphere, JPH::Vec3::sReplicate(1.0f), transform,
                                                        JPH::CollideShapeSettings(), JPH::RVec3::sZero(), collector);

    for (const JPH::CollideShapeResult& hit : collector.mHits) {
        Entity entity = getEntity(hit.mBodyID2);
        if (std::find(results.begin(), results.end(), entity) == results.end()) {
            results.push_back(entity);
        }
    }

    return results;
}

std::vector<Entity> JoltPhysics3DSystem::overlapBox(Vec3 center, Vec3 halfExtents, Quat rotation,
                                                    const QueryFilter3D& filter) const {
    std::vector<Entity> results;

    JPH::BoxShape box(toJolt(halfExtents));
    JPH::RMat44 transform = JPH::RMat44::sRotationTranslation(toJolt(rotation), toJolt(center));

    JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;
    physicsSystem_->GetNarrowPhaseQuery().CollideShape(&box, JPH::Vec3::sReplicate(1.0f), transform,
                                                        JPH::CollideShapeSettings(), JPH::RVec3::sZero(), collector);

    for (const JPH::CollideShapeResult& hit : collector.mHits) {
        Entity entity = getEntity(hit.mBodyID2);
        if (std::find(results.begin(), results.end(), entity) == results.end()) {
            results.push_back(entity);
        }
    }

    return results;
}

std::vector<Entity> JoltPhysics3DSystem::queryAABB(Vec3 min, Vec3 max,
                                                   const QueryFilter3D& filter) const {
    std::vector<Entity> results;

    JPH::AABox aabb(toJolt(min), toJolt(max));

    class AABBCollector : public JPH::CollideShapeBodyCollector {
    public:
        std::vector<JPH::BodyID> bodies;
        void AddHit(const JPH::BodyID& inBodyID) override {
            bodies.push_back(inBodyID);
        }
    };

    AABBCollector collector;
    physicsSystem_->GetBroadPhaseQuery().CollideAABox(aabb, collector);

    for (const JPH::BodyID& bodyId : collector.bodies) {
        Entity entity = getEntity(bodyId);
        results.push_back(entity);
    }

    return results;
}

Result<UUID, Physics3DError> JoltPhysics3DSystem::createConstraint(const ConstraintDef3D& def) {
    if (!physicsSystem_) return std::unexpected(Physics3DError::InternalError);

    // Get body IDs for both entities
    JPH::BodyID* bodyIdA = getBodyID(def.bodyA);
    JPH::BodyID* bodyIdB = getBodyID(def.bodyB);
    if (!bodyIdA || !bodyIdB) return std::unexpected(Physics3DError::BodyNotFound);

    JPH::BodyLockWrite lockA(physicsSystem_->GetBodyLockInterface(), *bodyIdA);
    JPH::BodyLockWrite lockB(physicsSystem_->GetBodyLockInterface(), *bodyIdB);
    if (!lockA.Succeeded() || !lockB.Succeeded()) {
        return std::unexpected(Physics3DError::BodyNotFound);
    }

    JPH::Body& bodyA = lockA.GetBody();
    JPH::Body& bodyB = lockB.GetBody();

    JPH::Ref<JPH::Constraint> constraint;

    switch (def.type) {
        case ConstraintType3D::Fixed: {
            JPH::FixedConstraintSettings settings;
            settings.mAutoDetectPoint = false;
            settings.mPoint1 = bodyA.GetPosition() + toJolt(def.pivotA);
            settings.mPoint2 = bodyB.GetPosition() + toJolt(def.pivotB);
            constraint = settings.Create(bodyA, bodyB);
            break;
        }
        case ConstraintType3D::Point: {
            JPH::PointConstraintSettings settings;
            settings.mPoint1 = bodyA.GetPosition() + toJolt(def.pivotA);
            settings.mPoint2 = bodyB.GetPosition() + toJolt(def.pivotB);
            constraint = settings.Create(bodyA, bodyB);
            break;
        }
        case ConstraintType3D::Distance: {
            JPH::DistanceConstraintSettings settings;
            settings.mPoint1 = bodyA.GetPosition() + toJolt(def.pivotA);
            settings.mPoint2 = bodyB.GetPosition() + toJolt(def.pivotB);
            constraint = settings.Create(bodyA, bodyB);
            break;
        }
        case ConstraintType3D::Hinge: {
            JPH::HingeConstraintSettings settings;
            settings.mPoint1 = bodyA.GetPosition() + toJolt(def.pivotA);
            settings.mPoint2 = bodyB.GetPosition() + toJolt(def.pivotB);
            settings.mHingeAxis1 = JPH::Vec3::sAxisY();
            settings.mHingeAxis2 = JPH::Vec3::sAxisY();
            settings.mNormalAxis1 = JPH::Vec3::sAxisX();
            settings.mNormalAxis2 = JPH::Vec3::sAxisX();
            constraint = settings.Create(bodyA, bodyB);
            break;
        }
        case ConstraintType3D::Slider: {
            JPH::SliderConstraintSettings settings;
            settings.mAutoDetectPoint = false;
            settings.mPoint1 = bodyA.GetPosition() + toJolt(def.pivotA);
            settings.mPoint2 = bodyB.GetPosition() + toJolt(def.pivotB);
            settings.mSliderAxis1 = JPH::Vec3::sAxisX();
            settings.mSliderAxis2 = JPH::Vec3::sAxisX();
            settings.mNormalAxis1 = JPH::Vec3::sAxisY();
            settings.mNormalAxis2 = JPH::Vec3::sAxisY();
            constraint = settings.Create(bodyA, bodyB);
            break;
        }
        case ConstraintType3D::Cone: {
            JPH::ConeConstraintSettings settings;
            settings.mPoint1 = bodyA.GetPosition() + toJolt(def.pivotA);
            settings.mPoint2 = bodyB.GetPosition() + toJolt(def.pivotB);
            settings.mTwistAxis1 = JPH::Vec3::sAxisX();
            settings.mTwistAxis2 = JPH::Vec3::sAxisX();
            settings.mHalfConeAngle = 0.785398f;  // 45 degrees
            constraint = settings.Create(bodyA, bodyB);
            break;
        }
        case ConstraintType3D::SixDOF: {
            JPH::SixDOFConstraintSettings settings;
            settings.mPosition1 = bodyA.GetPosition() + toJolt(def.pivotA);
            settings.mPosition2 = bodyB.GetPosition() + toJolt(def.pivotB);
            constraint = settings.Create(bodyA, bodyB);
            break;
        }
        default:
            return std::unexpected(Physics3DError::InvalidConfiguration);
    }

    if (!constraint) return std::unexpected(Physics3DError::InternalError);

    physicsSystem_->AddConstraint(constraint);

    UUID id = nextConstraintId_++;
    constraints_[id] = ConstraintData{
        .constraint = constraint,
        .type = def.type,
        .entityA = def.bodyA,
        .entityB = def.bodyB
    };

    return id;
}

Result<void, Physics3DError> JoltPhysics3DSystem::destroyConstraint(UUID constraintId) {
    auto it = constraints_.find(constraintId);
    if (it == constraints_.end()) {
        return std::unexpected(Physics3DError::ConstraintNotFound);
    }

    physicsSystem_->RemoveConstraint(it->second.constraint);
    constraints_.erase(it);
    return {};
}

Result<void, Physics3DError> JoltPhysics3DSystem::setConstraintEnabled(UUID constraintId, bool enabled) {
    auto it = constraints_.find(constraintId);
    if (it == constraints_.end()) {
        return std::unexpected(Physics3DError::ConstraintNotFound);
    }

    it->second.constraint->SetEnabled(enabled);
    return {};
}

std::vector<UUID> JoltPhysics3DSystem::getConstraints(Entity entity) const {
    std::vector<UUID> result;
    for (const auto& [id, data] : constraints_) {
        if (data.entityA == entity || data.entityB == entity) {
            result.push_back(id);
        }
    }
    return result;
}

Result<void, Physics3DError> JoltPhysics3DSystem::setHingeLimits(UUID constraintId, float minAngle, float maxAngle) {
    auto it = constraints_.find(constraintId);
    if (it == constraints_.end()) {
        return std::unexpected(Physics3DError::ConstraintNotFound);
    }
    if (it->second.type != ConstraintType3D::Hinge) {
        return std::unexpected(Physics3DError::InvalidConfiguration);
    }

    auto* hingeConstraint = static_cast<JPH::HingeConstraint*>(it->second.constraint.GetPtr());
    hingeConstraint->SetLimits(minAngle, maxAngle);
    return {};
}

Result<void, Physics3DError> JoltPhysics3DSystem::setHingeMotor(UUID constraintId, float targetVelocity, float maxTorque) {
    auto it = constraints_.find(constraintId);
    if (it == constraints_.end()) {
        return std::unexpected(Physics3DError::ConstraintNotFound);
    }
    if (it->second.type != ConstraintType3D::Hinge) {
        return std::unexpected(Physics3DError::InvalidConfiguration);
    }

    auto* hingeConstraint = static_cast<JPH::HingeConstraint*>(it->second.constraint.GetPtr());
    JPH::MotorSettings& motorSettings = hingeConstraint->GetMotorSettings();
    motorSettings.mMaxTorqueLimit = maxTorque;
    motorSettings.mMinTorqueLimit = -maxTorque;
    hingeConstraint->SetTargetAngularVelocity(targetVelocity);
    hingeConstraint->SetMotorState(JPH::EMotorState::Velocity);
    return {};
}

Result<void, Physics3DError> JoltPhysics3DSystem::setSliderLimits(UUID constraintId, float minDistance, float maxDistance) {
    auto it = constraints_.find(constraintId);
    if (it == constraints_.end()) {
        return std::unexpected(Physics3DError::ConstraintNotFound);
    }
    if (it->second.type != ConstraintType3D::Slider) {
        return std::unexpected(Physics3DError::InvalidConfiguration);
    }

    auto* sliderConstraint = static_cast<JPH::SliderConstraint*>(it->second.constraint.GetPtr());
    sliderConstraint->SetLimits(minDistance, maxDistance);
    return {};
}

Result<void, Physics3DError> JoltPhysics3DSystem::setSliderMotor(UUID constraintId, float targetVelocity, float maxForce) {
    auto it = constraints_.find(constraintId);
    if (it == constraints_.end()) {
        return std::unexpected(Physics3DError::ConstraintNotFound);
    }
    if (it->second.type != ConstraintType3D::Slider) {
        return std::unexpected(Physics3DError::InvalidConfiguration);
    }

    auto* sliderConstraint = static_cast<JPH::SliderConstraint*>(it->second.constraint.GetPtr());
    JPH::MotorSettings& motorSettings = sliderConstraint->GetMotorSettings();
    motorSettings.mMaxForceLimit = maxForce;
    motorSettings.mMinForceLimit = -maxForce;
    sliderConstraint->SetTargetVelocity(targetVelocity);
    sliderConstraint->SetMotorState(JPH::EMotorState::Velocity);
    return {};
}

Result<float, Physics3DError> JoltPhysics3DSystem::getConstraintForce(UUID constraintId) const {
    auto it = constraints_.find(constraintId);
    if (it == constraints_.end()) {
        return std::unexpected(Physics3DError::ConstraintNotFound);
    }

    // Jolt doesn't expose constraint force directly in the same way
    // We can get approximate force from the constraint's total lambda (Lagrange multiplier)
    // For now, return 0 as this requires more complex computation
    return 0.0f;
}

Result<void, Physics3DError> JoltPhysics3DSystem::createCharacter(Entity entity,
                                                                   const CharacterControllerDef& def) {
    if (!initialized_) return std::unexpected(Physics3DError::InternalError);

    std::uint32_t entityId = static_cast<std::uint32_t>(entity);
    if (characters_.contains(entityId)) {
        return std::unexpected(Physics3DError::InvalidEntity);
    }

    // Create capsule shape for character
    JPH::Ref<JPH::CapsuleShape> capsuleShape = new JPH::CapsuleShape(def.height * 0.5f - def.radius, def.radius);

    // Create character settings
    JPH::CharacterVirtualSettings settings;
    settings.mShape = capsuleShape;
    settings.mMaxSlopeAngle = JPH::DegreesToRadians(def.maxSlopeAngle);
    settings.mMass = def.mass;
    settings.mMaxStrength = 100.0f;
    settings.mPredictiveContactDistance = 0.1f;
    settings.mPenetrationRecoverySpeed = 1.0f;

    // Create the character
    auto character = std::make_unique<JPH::CharacterVirtual>(
        &settings,
        JPH::RVec3::sZero(),  // Initial position will be set later
        JPH::Quat::sIdentity(),
        0,  // User data
        physicsSystem_.get()
    );

    characters_[entityId] = std::move(character);

    return {};
}

Result<void, Physics3DError> JoltPhysics3DSystem::destroyCharacter(Entity entity) {
    std::uint32_t entityId = static_cast<std::uint32_t>(entity);
    auto it = characters_.find(entityId);
    if (it == characters_.end()) {
        return std::unexpected(Physics3DError::CharacterNotFound);
    }

    characters_.erase(it);
    return {};
}

Result<void, Physics3DError> JoltPhysics3DSystem::moveCharacter(Entity entity, Vec3 velocity, DeltaTime dt) {
    std::uint32_t entityId = static_cast<std::uint32_t>(entity);
    auto it = characters_.find(entityId);
    if (it == characters_.end()) {
        return std::unexpected(Physics3DError::CharacterNotFound);
    }

    JPH::CharacterVirtual* character = it->second.get();

    // Use the velocity directly as provided by the caller
    // The caller is responsible for applying gravity and handling jumping
    JPH::Vec3 newVelocity = toJolt(velocity);

    // Check if grounded and zero out downward velocity if so
    if (character->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround) {
        if (newVelocity.GetY() < 0.0f) {
            newVelocity.SetY(0.0f);
        }
    }

    character->SetLinearVelocity(newVelocity);

    // Update character - don't apply gravity here since caller handles it
    JPH::Vec3 gravity = physicsSystem_->GetGravity();
    JPH::CharacterVirtual::ExtendedUpdateSettings updateSettings;
    character->ExtendedUpdate(dt, JPH::Vec3::sZero(), updateSettings,  // No gravity here
                               physicsSystem_->GetDefaultBroadPhaseLayerFilter(LAYER_MOVING),
                               physicsSystem_->GetDefaultLayerFilter(LAYER_MOVING),
                               {},  // Body filter
                               {},  // Shape filter
                               *tempAllocator_);

    return {};
}

Result<Vec3, Physics3DError> JoltPhysics3DSystem::getCharacterPosition(Entity entity) const {
    std::uint32_t entityId = static_cast<std::uint32_t>(entity);
    auto it = characters_.find(entityId);
    if (it == characters_.end()) {
        return std::unexpected(Physics3DError::CharacterNotFound);
    }

    return fromJolt(it->second->GetPosition());
}

Result<void, Physics3DError> JoltPhysics3DSystem::setCharacterPosition(Entity entity, Vec3 position) {
    std::uint32_t entityId = static_cast<std::uint32_t>(entity);
    auto it = characters_.find(entityId);
    if (it == characters_.end()) {
        return std::unexpected(Physics3DError::CharacterNotFound);
    }

    it->second->SetPosition(toJolt(position));
    return {};
}

Result<CharacterGroundInfo, Physics3DError> JoltPhysics3DSystem::getCharacterGroundInfo(Entity entity) const {
    std::uint32_t entityId = static_cast<std::uint32_t>(entity);
    auto it = characters_.find(entityId);
    if (it == characters_.end()) {
        return std::unexpected(Physics3DError::CharacterNotFound);
    }

    JPH::CharacterVirtual* character = it->second.get();
    JPH::CharacterVirtual::EGroundState groundState = character->GetGroundState();

    CharacterGroundState state = CharacterGroundState::InAir;
    switch (groundState) {
        case JPH::CharacterVirtual::EGroundState::OnGround:
            state = CharacterGroundState::OnGround;
            break;
        case JPH::CharacterVirtual::EGroundState::OnSteepGround:
            state = CharacterGroundState::OnSteepGround;
            break;
        case JPH::CharacterVirtual::EGroundState::NotSupported:
            state = CharacterGroundState::Sliding;  // Map NotSupported to Sliding
            break;
        case JPH::CharacterVirtual::EGroundState::InAir:
        default:
            state = CharacterGroundState::InAir;
            break;
    }

    // Calculate slope angle from ground normal
    JPH::Vec3 groundNormal = character->GetGroundNormal();
    float slopeAngle = std::acos(groundNormal.GetY()) * (180.0f / 3.14159265f);

    return CharacterGroundInfo{
        .state = state,
        .groundEntity = static_cast<Entity>(entt::null),  // Would need to look up from BodyID
        .groundNormal = fromJoltVec3(groundNormal),
        .groundPoint = fromJolt(character->GetGroundPosition()),
        .slopeAngle = slopeAngle
    };
}

Result<Vec3, Physics3DError> JoltPhysics3DSystem::getCharacterVelocity(Entity entity) const {
    std::uint32_t entityId = static_cast<std::uint32_t>(entity);
    auto it = characters_.find(entityId);
    if (it == characters_.end()) {
        return std::unexpected(Physics3DError::CharacterNotFound);
    }

    return fromJoltVec3(it->second->GetLinearVelocity());
}

Result<void, Physics3DError> JoltPhysics3DSystem::createVehicle(Entity entity, const VehicleDef& def) {
    // TODO: Implement vehicle creation using VehicleConstraint
    return std::unexpected(Physics3DError::InvalidConfiguration);
}

Result<void, Physics3DError> JoltPhysics3DSystem::destroyVehicle(Entity entity) {
    // TODO: Implement vehicle destruction
    return std::unexpected(Physics3DError::VehicleNotFound);
}

Result<void, Physics3DError> JoltPhysics3DSystem::updateVehicle(Entity entity, float throttle,
                                                                float steering, float brake) {
    // TODO: Implement vehicle control update
    return std::unexpected(Physics3DError::VehicleNotFound);
}

Result<WheelState, Physics3DError> JoltPhysics3DSystem::getWheelTransform(Entity entity,
                                                                           std::uint32_t wheelIndex) const {
    // TODO: Implement wheel transform query
    return std::unexpected(Physics3DError::VehicleNotFound);
}

Result<bool, Physics3DError> JoltPhysics3DSystem::isWheelGrounded(Entity entity,
                                                                   std::uint32_t wheelIndex) const {
    // TODO: Implement wheel grounded check
    return std::unexpected(Physics3DError::VehicleNotFound);
}

Result<float, Physics3DError> JoltPhysics3DSystem::getVehicleSpeed(Entity entity) const {
    // TODO: Implement vehicle speed query
    return std::unexpected(Physics3DError::VehicleNotFound);
}

void JoltPhysics3DSystem::setGravity(Vec3 gravity) {
    gravity_ = gravity;
    if (physicsSystem_) {
        physicsSystem_->SetGravity(toJolt(gravity));
    }
}

Vec3 JoltPhysics3DSystem::getGravity() const {
    return gravity_;
}

void JoltPhysics3DSystem::setCollisionCallback(Collision3DCallback callback) {
    contactListener_.setCollisionCallback(std::move(callback));
}

void JoltPhysics3DSystem::setTriggerEnterCallback(Trigger3DEnterCallback callback) {
    contactListener_.setTriggerEnterCallback(std::move(callback));
}

void JoltPhysics3DSystem::setTriggerExitCallback(Trigger3DExitCallback callback) {
    contactListener_.setTriggerExitCallback(std::move(callback));
}

//==========================================================================
// Compound Shapes
//==========================================================================

Result<void, Physics3DError> JoltPhysics3DSystem::createCompoundBody(
    Entity entity,
    BodyType3D type,
    const Transform3D& transform,
    const CompoundShapeDef& shape) {
    if (!initialized_) return std::unexpected(Physics3DError::InternalError);
    if (hasBody(entity)) return std::unexpected(Physics3DError::InvalidEntity);

    JPH::StaticCompoundShapeSettings compoundSettings;

    // Add all box shapes
    for (const auto& box : shape.boxes) {
        JPH::Ref<JPH::BoxShape> boxShape = new JPH::BoxShape(toJolt(box.halfExtents));
        compoundSettings.AddShape(toJolt(box.localPosition), toJolt(box.localRotation), boxShape);
    }

    // Add all sphere shapes
    for (const auto& sphere : shape.spheres) {
        JPH::Ref<JPH::SphereShape> sphereShape = new JPH::SphereShape(sphere.radius);
        compoundSettings.AddShape(toJolt(sphere.localPosition), toJolt(sphere.localRotation), sphereShape);
    }

    // Add all capsule shapes
    for (const auto& capsule : shape.capsules) {
        JPH::Ref<JPH::CapsuleShape> capsuleShape = new JPH::CapsuleShape(capsule.halfHeight, capsule.radius);
        compoundSettings.AddShape(toJolt(capsule.localPosition), toJolt(capsule.localRotation), capsuleShape);
    }

    // Add all cylinder shapes
    for (const auto& cylinder : shape.cylinders) {
        JPH::Ref<JPH::CylinderShape> cylinderShape = new JPH::CylinderShape(cylinder.halfHeight, cylinder.radius);
        compoundSettings.AddShape(toJolt(cylinder.localPosition), toJolt(cylinder.localRotation), cylinderShape);
    }

    JPH::Shape::ShapeResult shapeResult = compoundSettings.Create();
    if (!shapeResult.IsValid()) {
        return std::unexpected(Physics3DError::InvalidShape);
    }

    JPH::BodyCreationSettings settings(
        shapeResult.Get(),
        toJolt(transform.position),
        toJolt(transform.rotation),
        toJoltMotionType(type),
        type == BodyType3D::Static ? LAYER_NON_MOVING : LAYER_MOVING
    );

    JPH::Body* body = physicsSystem_->GetBodyInterface().CreateBody(settings);
    if (!body) return std::unexpected(Physics3DError::OutOfMemory);

    JPH::BodyID bodyId = body->GetID();
    physicsSystem_->GetBodyInterface().AddBody(bodyId, JPH::EActivation::Activate);

    entityToBody_[static_cast<std::uint32_t>(entity)] = bodyId;
    bodyIdToEntity_[bodyId.GetIndexAndSequenceNumber()] = entity;

    return {};
}

Result<void, Physics3DError> JoltPhysics3DSystem::createHeightFieldBody(
    Entity entity,
    const Transform3D& transform,
    const HeightFieldShapeDef& shape) {
    if (!initialized_) return std::unexpected(Physics3DError::InternalError);
    if (hasBody(entity)) return std::unexpected(Physics3DError::InvalidEntity);
    if (shape.heights.empty() || shape.width == 0 || shape.length == 0) {
        return std::unexpected(Physics3DError::InvalidShape);
    }

    // Create height field shape settings
    JPH::HeightFieldShapeSettings heightFieldSettings(
        shape.heights.data(),
        toJolt(Vec3{0.0f}),  // Offset
        toJolt(shape.scale),
        shape.width
    );

    JPH::Shape::ShapeResult shapeResult = heightFieldSettings.Create();
    if (!shapeResult.IsValid()) {
        return std::unexpected(Physics3DError::InvalidShape);
    }

    JPH::BodyCreationSettings settings(
        shapeResult.Get(),
        toJolt(transform.position),
        toJolt(transform.rotation),
        JPH::EMotionType::Static,
        LAYER_NON_MOVING
    );

    JPH::Body* body = physicsSystem_->GetBodyInterface().CreateBody(settings);
    if (!body) return std::unexpected(Physics3DError::OutOfMemory);

    JPH::BodyID bodyId = body->GetID();
    physicsSystem_->GetBodyInterface().AddBody(bodyId, JPH::EActivation::DontActivate);

    entityToBody_[static_cast<std::uint32_t>(entity)] = bodyId;
    bodyIdToEntity_[bodyId.GetIndexAndSequenceNumber()] = entity;

    return {};
}

Result<std::uint32_t, Physics3DError> JoltPhysics3DSystem::addShape(Entity entity, const BoxShapeDef& shape) {
    if (!physicsSystem_) return std::unexpected(Physics3DError::InternalError);

    JPH::BodyID* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    JPH::BodyInterface& bodyInterface = physicsSystem_->GetBodyInterface();
    const JPH::Shape* currentShape = bodyInterface.GetShape(*bodyId);

    // Create new box shape
    JPH::Ref<JPH::BoxShape> boxShape = new JPH::BoxShape(toJolt(shape.halfExtents));

    // Create a compound shape with existing shape + new shape
    JPH::StaticCompoundShapeSettings compoundSettings;

    // Add existing shape at origin
    compoundSettings.AddShape(JPH::Vec3::sZero(), JPH::Quat::sIdentity(), currentShape);

    // Add new shape with offset
    JPH::Vec3 offset = toJolt(shape.localPosition);
    JPH::Quat rotation = toJolt(shape.localRotation);
    compoundSettings.AddShape(offset, rotation, boxShape);

    JPH::Shape::ShapeResult result = compoundSettings.Create();
    if (!result.IsValid()) {
        return std::unexpected(Physics3DError::InternalError);
    }

    bodyInterface.SetShape(*bodyId, result.Get(), true, JPH::EActivation::Activate);

    // Return shape index (0 = original, 1+ = added shapes)
    return static_cast<std::uint32_t>(result.Get()->GetSubShapeIDBitsRecursive());
}

Result<std::uint32_t, Physics3DError> JoltPhysics3DSystem::addShape(Entity entity, const SphereShapeDef& shape) {
    if (!physicsSystem_) return std::unexpected(Physics3DError::InternalError);

    JPH::BodyID* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    JPH::BodyInterface& bodyInterface = physicsSystem_->GetBodyInterface();
    const JPH::Shape* currentShape = bodyInterface.GetShape(*bodyId);

    // Create new sphere shape
    JPH::Ref<JPH::SphereShape> sphereShape = new JPH::SphereShape(shape.radius);

    // Create a compound shape with existing shape + new shape
    JPH::StaticCompoundShapeSettings compoundSettings;
    compoundSettings.AddShape(JPH::Vec3::sZero(), JPH::Quat::sIdentity(), currentShape);
    compoundSettings.AddShape(toJolt(shape.localPosition), toJolt(shape.localRotation), sphereShape);

    JPH::Shape::ShapeResult result = compoundSettings.Create();
    if (!result.IsValid()) {
        return std::unexpected(Physics3DError::InternalError);
    }

    bodyInterface.SetShape(*bodyId, result.Get(), true, JPH::EActivation::Activate);

    return static_cast<std::uint32_t>(result.Get()->GetSubShapeIDBitsRecursive());
}

Result<std::uint32_t, Physics3DError> JoltPhysics3DSystem::addShape(Entity entity, const CapsuleShapeDef& shape) {
    if (!physicsSystem_) return std::unexpected(Physics3DError::InternalError);

    JPH::BodyID* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    JPH::BodyInterface& bodyInterface = physicsSystem_->GetBodyInterface();
    const JPH::Shape* currentShape = bodyInterface.GetShape(*bodyId);

    // Create new capsule shape
    JPH::Ref<JPH::CapsuleShape> capsuleShape = new JPH::CapsuleShape(shape.halfHeight, shape.radius);

    // Create a compound shape with existing shape + new shape
    JPH::StaticCompoundShapeSettings compoundSettings;
    compoundSettings.AddShape(JPH::Vec3::sZero(), JPH::Quat::sIdentity(), currentShape);
    compoundSettings.AddShape(toJolt(shape.localPosition), toJolt(shape.localRotation), capsuleShape);

    JPH::Shape::ShapeResult result = compoundSettings.Create();
    if (!result.IsValid()) {
        return std::unexpected(Physics3DError::InternalError);
    }

    bodyInterface.SetShape(*bodyId, result.Get(), true, JPH::EActivation::Activate);

    return static_cast<std::uint32_t>(result.Get()->GetSubShapeIDBitsRecursive());
}

Result<void, Physics3DError> JoltPhysics3DSystem::removeShape(Entity entity, std::uint32_t shapeIndex) {
    // Removing shapes from a compound requires recreating the compound without that shape
    // This is a complex operation that would need to track all shapes
    // For now, return an error as this requires more sophisticated tracking
    return std::unexpected(Physics3DError::InternalError);
}

Result<std::uint32_t, Physics3DError> JoltPhysics3DSystem::getShapeCount(Entity entity) const {
    if (!physicsSystem_) return std::unexpected(Physics3DError::InternalError);

    JPH::BodyID* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    const JPH::BodyInterface& bodyInterface = physicsSystem_->GetBodyInterface();
    const JPH::Shape* shape = bodyInterface.GetShape(*bodyId);

    if (shape->GetType() == JPH::EShapeType::Compound) {
        const JPH::CompoundShape* compound = static_cast<const JPH::CompoundShape*>(shape);
        return compound->GetNumSubShapes();
    }

    return 1;  // Single shape
}

//==========================================================================
// Mass Properties
//==========================================================================

Result<void, Physics3DError> JoltPhysics3DSystem::setMassProperties(Entity entity, const MassProperties& props) {
    auto* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    JPH::Body* body = physicsSystem_->GetBodyLockInterface().TryGetBody(*bodyId);
    if (!body) return std::unexpected(Physics3DError::BodyNotFound);

    if (body->GetMotionType() == JPH::EMotionType::Dynamic) {
        JPH::MassProperties massProps;
        massProps.mMass = props.mass;
        // Convert Mat3 to JPH::Mat44 for inertia
        // Note: Jolt expects a 4x4 matrix but only uses the 3x3 portion for inertia
        body->GetMotionProperties()->SetMassProperties(JPH::EAllowedDOFs::All, massProps);
    }

    return {};
}

Result<MassProperties, Physics3DError> JoltPhysics3DSystem::getMassProperties(Entity entity) const {
    auto* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    const JPH::Body* body = physicsSystem_->GetBodyLockInterface().TryGetBody(*bodyId);
    if (!body) return std::unexpected(Physics3DError::BodyNotFound);

    MassProperties props;
    props.mass = 1.0f / body->GetMotionProperties()->GetInverseMass();
    props.centerOfMass = Vec3{0.0f};  // Jolt uses local center by default
    props.autoCompute = false;

    return props;
}

Result<Vec3, Physics3DError> JoltPhysics3DSystem::getCenterOfMass(Entity entity) const {
    auto* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    return fromJolt(physicsSystem_->GetBodyInterface().GetCenterOfMassPosition(*bodyId));
}

Result<Mat3, Physics3DError> JoltPhysics3DSystem::getInertiaTensor(Entity entity) const {
    auto* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    const JPH::Body* body = physicsSystem_->GetBodyLockInterface().TryGetBody(*bodyId);
    if (!body) return std::unexpected(Physics3DError::BodyNotFound);

    // Return identity matrix for now - proper inertia tensor extraction requires more work
    return Mat3{1.0f};
}

//==========================================================================
// Motion Quality (CCD)
//==========================================================================

Result<void, Physics3DError> JoltPhysics3DSystem::setMotionQuality(Entity entity, MotionQuality quality) {
    if (!physicsSystem_) return std::unexpected(Physics3DError::InternalError);

    JPH::BodyID* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    JPH::BodyInterface& bodyInterface = physicsSystem_->GetBodyInterface();

    JPH::EMotionQuality joltQuality = (quality == MotionQuality::LinearCast)
        ? JPH::EMotionQuality::LinearCast
        : JPH::EMotionQuality::Discrete;

    bodyInterface.SetMotionQuality(*bodyId, joltQuality);

    return {};
}

Result<MotionQuality, Physics3DError> JoltPhysics3DSystem::getMotionQuality(Entity entity) const {
    if (!physicsSystem_) return std::unexpected(Physics3DError::InternalError);

    JPH::BodyID* bodyId = getBodyID(entity);
    if (!bodyId) return std::unexpected(Physics3DError::BodyNotFound);

    const JPH::BodyInterface& bodyInterface = physicsSystem_->GetBodyInterface();
    JPH::EMotionQuality joltQuality = bodyInterface.GetMotionQuality(*bodyId);

    return (joltQuality == JPH::EMotionQuality::LinearCast)
        ? MotionQuality::LinearCast
        : MotionQuality::Discrete;
}

//==========================================================================
// Contact Queries
//==========================================================================

std::vector<ContactPair3D> JoltPhysics3DSystem::getContacts(Entity entity) const {
    return contactListener_.getContacts(entity);
}

std::vector<ContactPair3D> JoltPhysics3DSystem::getAllContacts() const {
    return contactListener_.getAllContacts();
}

bool JoltPhysics3DSystem::areInContact(Entity a, Entity b) const {
    return contactListener_.areInContact(a, b);
}

std::optional<ContactPair3D> JoltPhysics3DSystem::getContactPair(Entity a, Entity b) const {
    return contactListener_.getContactPair(a, b);
}

void JoltPhysics3DSystem::setDebugDraw(bool enabled) {
    debugDrawEnabled_ = enabled;
}

std::vector<DebugLine3D> JoltPhysics3DSystem::getDebugLines() const {
    // TODO: Implement debug line extraction from Jolt DebugRenderer
    return {};
}

PhysicsStats3D JoltPhysics3DSystem::getStats() const {
    if (!physicsSystem_) return {};

    // NOTE: Jolt doesn't expose GetPhysicsStats() directly - would need to track stats ourselves
    // For now, return approximate stats based on what we can query
    return PhysicsStats3D{
        .activeBodies = static_cast<std::uint32_t>(entityToBody_.size()),  // Approximate
        .sleepingBodies = 0,  // TODO: Track sleeping bodies
        .constraints = static_cast<std::uint32_t>(constraints_.size()),
        .characters = static_cast<std::uint32_t>(characters_.size()),
        .vehicles = 0,  // TODO: Re-enable vehicles when properly implemented
        .updateTimeMs = 0.0f,  // TODO: Track update time
        .collisionPairs = 0  // TODO: Track collision pairs
    };
}

JPH::BodyID* JoltPhysics3DSystem::getBodyID(Entity entity) const {
    auto it = entityToBody_.find(static_cast<std::uint32_t>(entity));
    if (it == entityToBody_.end()) return nullptr;
    return const_cast<JPH::BodyID*>(&it->second);
}

Entity JoltPhysics3DSystem::getEntity(JPH::BodyID bodyId) const {
    auto it = bodyIdToEntity_.find(bodyId.GetIndexAndSequenceNumber());
    if (it == bodyIdToEntity_.end()) return static_cast<Entity>(entt::null);
    return it->second;
}

JPH::Ref<JPH::Shape> JoltPhysics3DSystem::createShape(const PhysicsBodyDef3D& def) {
    switch (def.shapeType) {
        case ShapeType3D::Box:
            return new JPH::BoxShape(toJolt(def.shapeHalfExtents));

        case ShapeType3D::Sphere:
            return new JPH::SphereShape(def.shapeRadius);

        case ShapeType3D::Capsule:
            return new JPH::CapsuleShape(def.shapeHalfHeight, def.shapeRadius);

        case ShapeType3D::Cylinder:
            return new JPH::CylinderShape(def.shapeHalfHeight, def.shapeRadius);

        case ShapeType3D::Mesh:
            // TODO: Implement mesh shape creation from vertices/indices
            return nullptr;

        case ShapeType3D::ConvexHull:
            // TODO: Implement convex hull shape creation
            return nullptr;

        case ShapeType3D::HeightField:
            // TODO: Implement height field shape creation
            return nullptr;

        default:
            return nullptr;
    }
}

//==========================================================================
// Factory Function
//==========================================================================

inline std::unique_ptr<IPhysics3DSystem> createPhysics3DSystem() {
    auto system = std::make_unique<JoltPhysics3DSystem>();
    system->initialize();
    return std::unique_ptr<IPhysics3DSystem>(system.release());
}

}  // namespace bestow
