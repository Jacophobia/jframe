// bestow-contract/src/bestow.physics3d.cppm
// 3D Physics system interface (Jolt Physics)

module;

#include <functional>
#include <optional>
#include <vector>
#include <span>

export module bestow.physics3d;

import bestow.types;

export namespace bestow {

// All 3D math types (Quat, Transform3D, AABB3D) are imported from bestow.types
// All 3D physics enums (BodyType3D, ShapeType3D, ConstraintType3D) are imported from bestow.types
// All collision layer types and constants are imported from bestow.types
// All event types (CollisionEvent3D, TriggerEvent3D, RaycastHit3D) are imported from bestow.types

//==========================================================================
// Error Types
//==========================================================================

enum class Physics3DError {
    Success,
    InvalidEntity,
    BodyNotFound,
    InvalidShape,
    ConstraintNotFound,
    CharacterNotFound,
    VehicleNotFound,
    InvalidConfiguration,
    OutOfMemory,
    InternalError
};

//==========================================================================
// Shape Definitions
//==========================================================================

struct ShapeDef3D {
    ShapeType3D type = ShapeType3D::Box;
    float density = 1000.0f;
    float friction = 0.5f;
    float restitution = 0.3f;
    Vec3 localPosition{0.0f};
    Quat localRotation{1.0f, 0.0f, 0.0f, 0.0f};
};

struct BoxShapeDef : ShapeDef3D {
    Vec3 halfExtents{0.5f};

    BoxShapeDef() {
        type = ShapeType3D::Box;
    }
};

struct SphereShapeDef : ShapeDef3D {
    float radius = 0.5f;

    SphereShapeDef() {
        type = ShapeType3D::Sphere;
    }
};

struct CapsuleShapeDef : ShapeDef3D {
    float radius = 0.5f;
    float halfHeight = 0.5f;

    CapsuleShapeDef() {
        type = ShapeType3D::Capsule;
    }
};

struct CylinderShapeDef : ShapeDef3D {
    float radius = 0.5f;
    float halfHeight = 0.5f;

    CylinderShapeDef() {
        type = ShapeType3D::Cylinder;
    }
};

struct MeshShapeDef : ShapeDef3D {
    std::vector<Vec3> vertices;
    std::vector<std::uint32_t> indices;

    MeshShapeDef() {
        type = ShapeType3D::Mesh;
    }
};

struct ConvexHullShapeDef : ShapeDef3D {
    std::vector<Vec3> vertices;

    ConvexHullShapeDef() {
        type = ShapeType3D::ConvexHull;
    }
};

struct HeightFieldShapeDef : ShapeDef3D {
    std::vector<float> heights;  // Row-major height values
    std::uint32_t width = 0;     // Number of samples along X
    std::uint32_t length = 0;    // Number of samples along Z
    Vec3 scale{1.0f};            // Scale to apply to the height field

    HeightFieldShapeDef() {
        type = ShapeType3D::HeightField;
    }
};

struct CompoundShapeDef : ShapeDef3D {
    std::vector<BoxShapeDef> boxes;
    std::vector<SphereShapeDef> spheres;
    std::vector<CapsuleShapeDef> capsules;
    std::vector<CylinderShapeDef> cylinders;
    std::vector<ConvexHullShapeDef> convexHulls;

    CompoundShapeDef() {
        type = ShapeType3D::Compound;
    }
};

//==========================================================================
// Mass Properties
//==========================================================================

struct MassProperties {
    float mass = 1.0f;
    Vec3 centerOfMass{0.0f};
    Mat3 inertiaTensor{1.0f};  // 3x3 inertia tensor
    bool autoCompute = true;   // If true, compute from shapes automatically
};

//==========================================================================
// Body Definition
//==========================================================================

enum class MotionQuality : std::uint8_t {
    Discrete,    // Standard discrete collision detection
    LinearCast   // CCD for fast-moving objects (prevents tunneling)
};

struct PhysicsBodyDef3D {
    BodyType3D type = BodyType3D::Dynamic;
    Transform3D transform;
    ShapeType3D shapeType = ShapeType3D::Box;
    Vec3 shapeHalfExtents{0.5f};  // For box
    float shapeRadius = 0.5f;      // For sphere/capsule/cylinder
    float shapeHalfHeight = 0.5f;  // For capsule/cylinder
    float density = 1000.0f;
    float friction = 0.5f;
    float restitution = 0.3f;
    float linearDamping = 0.05f;
    float angularDamping = 0.05f;
    float gravityFactor = 1.0f;
    bool allowSleep = true;
    CollisionLayer3D layer = 0x0001;
    CollisionMask3D mask = 0xFFFF;
    bool isSensor = false;

    // CCD (Continuous Collision Detection)
    MotionQuality motionQuality = MotionQuality::Discrete;

    // Mass properties (optional, computed from density by default)
    std::optional<MassProperties> massProperties;
};

//==========================================================================
// Contact Information
//==========================================================================

struct ContactPoint3D {
    Vec3 worldPositionOnA;
    Vec3 worldPositionOnB;
    Vec3 worldNormalOnB;
    float penetrationDepth;
    float combinedFriction;
    float combinedRestitution;
};

struct ContactPair3D {
    Entity entityA;
    Entity entityB;
    std::vector<ContactPoint3D> contacts;
    float impulse;
    bool isActive;
};

//==========================================================================
// Constraint Definitions
//==========================================================================

struct ConstraintDef3D {
    ConstraintType3D type = ConstraintType3D::Fixed;
    Entity bodyA;
    Entity bodyB;
    Vec3 pivotA{0.0f};
    Vec3 pivotB{0.0f};
    bool collideConnected = false;
};

struct HingeConstraintDef : ConstraintDef3D {
    Vec3 axisA{0.0f, 1.0f, 0.0f};
    Vec3 axisB{0.0f, 1.0f, 0.0f};
    bool hasLimits = false;
    float minAngle = -3.14159f;
    float maxAngle = 3.14159f;
    bool hasMotor = false;
    float motorTargetVelocity = 0.0f;
    float motorMaxTorque = 0.0f;

    HingeConstraintDef() {
        type = ConstraintType3D::Hinge;
    }
};

struct SliderConstraintDef : ConstraintDef3D {
    Vec3 axisA{1.0f, 0.0f, 0.0f};
    Vec3 axisB{1.0f, 0.0f, 0.0f};
    bool hasLimits = false;
    float minDistance = -1.0f;
    float maxDistance = 1.0f;
    bool hasMotor = false;
    float motorTargetVelocity = 0.0f;
    float motorMaxForce = 0.0f;

    SliderConstraintDef() {
        type = ConstraintType3D::Slider;
    }
};

struct ConeConstraintDef : ConstraintDef3D {
    Vec3 twistAxisA{1.0f, 0.0f, 0.0f};
    Vec3 twistAxisB{1.0f, 0.0f, 0.0f};
    float halfConeAngle = 0.785398f;  // 45 degrees

    ConeConstraintDef() {
        type = ConstraintType3D::Cone;
    }
};

struct PointConstraintDef : ConstraintDef3D {
    PointConstraintDef() {
        type = ConstraintType3D::Point;
    }
};

struct DistanceConstraintDef : ConstraintDef3D {
    float minDistance = 0.0f;
    float maxDistance = 1.0f;

    DistanceConstraintDef() {
        type = ConstraintType3D::Distance;
    }
};

//==========================================================================
// Character Controller Definition
//==========================================================================

struct CharacterControllerDef {
    float radius = 0.3f;
    float height = 1.8f;
    float stepHeight = 0.35f;
    float maxSlopeAngle = 45.0f;
    float mass = 80.0f;
    CollisionLayer3D layer = CollisionLayers3D::Character;
    CollisionMask3D mask = 0xFFFF;
};

// CharacterGroundInfo is imported from bestow.types

//==========================================================================
// Vehicle Definitions
//==========================================================================

struct WheelDef {
    Vec3 connectionPoint{0.0f};
    Vec3 suspensionDirection{0.0f, -1.0f, 0.0f};
    float suspensionLength = 0.3f;
    float suspensionStiffness = 35.0f;
    float suspensionDamping = 4.4f;
    float radius = 0.4f;
    float friction = 1.0f;
    bool isDriven = false;
    bool isSteered = false;
};

struct VehicleDef {
    std::vector<WheelDef> wheels;
    float maxEngineForce = 10000.0f;
    float maxBrakeForce = 5000.0f;
    float maxSteeringAngle = 0.5f;
};

struct WheelState {
    Transform3D transform;
    bool grounded = false;
    float suspensionLength = 0.0f;
    Vec3 contactPoint{0.0f};
    Vec3 contactNormal{0.0f, 1.0f, 0.0f};
};

//==========================================================================
// Query Structures
//==========================================================================

struct QueryFilter3D {
    CollisionMask3D layerMask = 0xFFFF;
    std::optional<Entity> ignoreEntity;
    bool ignoreSensors = true;
};

struct ShapeCastHit3D {
    Entity entity;
    Vec3 point;
    Vec3 normal;
    float distance;
    Vec3 penetrationDepth{0.0f};
};

//==========================================================================
// Statistics
//==========================================================================

struct PhysicsStats3D {
    std::uint32_t activeBodies = 0;
    std::uint32_t sleepingBodies = 0;
    std::uint32_t constraints = 0;
    std::uint32_t characters = 0;
    std::uint32_t vehicles = 0;
    float updateTimeMs = 0.0f;
    std::uint32_t collisionPairs = 0;
};

// DebugLine3D is imported from bestow.types

//==========================================================================
// Callback Types
//==========================================================================

using Collision3DCallback = std::function<void(const CollisionEvent3D&)>;
using Trigger3DEnterCallback = std::function<void(const TriggerEvent3D&)>;
using Trigger3DExitCallback = std::function<void(const TriggerEvent3D&)>;

//==========================================================================
// IPhysics3DSystem Interface
//==========================================================================

class IPhysics3DSystem {
public:
    virtual ~IPhysics3DSystem() = default;

    //======================================================================
    // Lifecycle
    //======================================================================

    /// Update the physics simulation
    /// @param dt Time step in seconds
    /// @param subSteps Number of substeps for stability (default: 1)
    virtual void update(DeltaTime dt, int subSteps = 1) = 0;

    /// Synchronize entity transforms from physics bodies
    /// Call after update() to apply physics results to entity transforms
    virtual void syncTransforms(std::span<const Entity> entities) = 0;

    //======================================================================
    // Body Management
    //======================================================================

    virtual Result<void, Physics3DError> createBody(Entity entity, const PhysicsBodyDef3D& def) = 0;
    virtual Result<void, Physics3DError> destroyBody(Entity entity) = 0;
    virtual bool hasBody(Entity entity) const = 0;
    virtual std::vector<Entity> getAllBodies() const = 0;

    //======================================================================
    // Multi-Shape Bodies (Compound Shapes)
    //======================================================================

    /// Create body with compound shape (multiple collision shapes)
    virtual Result<void, Physics3DError> createCompoundBody(
        Entity entity,
        BodyType3D type,
        const Transform3D& transform,
        const CompoundShapeDef& shape) = 0;

    /// Add a shape to an existing body
    virtual Result<std::uint32_t, Physics3DError> addShape(Entity entity, const BoxShapeDef& shape) = 0;
    virtual Result<std::uint32_t, Physics3DError> addShape(Entity entity, const SphereShapeDef& shape) = 0;
    virtual Result<std::uint32_t, Physics3DError> addShape(Entity entity, const CapsuleShapeDef& shape) = 0;

    /// Remove a shape from a body by index
    virtual Result<void, Physics3DError> removeShape(Entity entity, std::uint32_t shapeIndex) = 0;

    /// Get number of shapes on a body
    virtual Result<std::uint32_t, Physics3DError> getShapeCount(Entity entity) const = 0;

    /// Create body with height field shape (terrain)
    virtual Result<void, Physics3DError> createHeightFieldBody(
        Entity entity,
        const Transform3D& transform,
        const HeightFieldShapeDef& shape) = 0;

    //======================================================================
    // Body Type
    //======================================================================

    virtual Result<void, Physics3DError> setBodyType(Entity entity, BodyType3D type) = 0;
    virtual Result<BodyType3D, Physics3DError> getBodyType(Entity entity) const = 0;

    //======================================================================
    // Transform
    //======================================================================

    virtual Result<void, Physics3DError> setTransform(Entity entity, const Transform3D& transform) = 0;
    virtual Result<Transform3D, Physics3DError> getTransform(Entity entity) const = 0;

    virtual Result<void, Physics3DError> setPosition(Entity entity, Vec3 position) = 0;
    virtual Result<Vec3, Physics3DError> getPosition(Entity entity) const = 0;

    virtual Result<void, Physics3DError> setRotation(Entity entity, Quat rotation) = 0;
    virtual Result<Quat, Physics3DError> getRotation(Entity entity) const = 0;

    //======================================================================
    // Velocity
    //======================================================================

    virtual Result<void, Physics3DError> setLinearVelocity(Entity entity, Vec3 velocity) = 0;
    virtual Result<Vec3, Physics3DError> getLinearVelocity(Entity entity) const = 0;

    virtual Result<void, Physics3DError> setAngularVelocity(Entity entity, Vec3 velocity) = 0;
    virtual Result<Vec3, Physics3DError> getAngularVelocity(Entity entity) const = 0;

    //======================================================================
    // Forces
    //======================================================================

    virtual Result<void, Physics3DError> applyForce(Entity entity, Vec3 force) = 0;
    virtual Result<void, Physics3DError> applyForceAtPoint(Entity entity, Vec3 force, Vec3 worldPoint) = 0;
    virtual Result<void, Physics3DError> applyTorque(Entity entity, Vec3 torque) = 0;

    virtual Result<void, Physics3DError> applyImpulse(Entity entity, Vec3 impulse) = 0;
    virtual Result<void, Physics3DError> applyImpulseAtPoint(Entity entity, Vec3 impulse, Vec3 worldPoint) = 0;
    virtual Result<void, Physics3DError> applyAngularImpulse(Entity entity, Vec3 impulse) = 0;

    //======================================================================
    // Body Properties
    //======================================================================

    virtual Result<void, Physics3DError> setMass(Entity entity, float mass) = 0;
    virtual Result<float, Physics3DError> getMass(Entity entity) const = 0;

    virtual Result<void, Physics3DError> setLinearDamping(Entity entity, float damping) = 0;
    virtual Result<void, Physics3DError> setAngularDamping(Entity entity, float damping) = 0;
    virtual Result<void, Physics3DError> setGravityFactor(Entity entity, float factor) = 0;

    virtual Result<void, Physics3DError> setFriction(Entity entity, float friction) = 0;
    virtual Result<void, Physics3DError> setRestitution(Entity entity, float restitution) = 0;

    //======================================================================
    // Mass Properties
    //======================================================================

    virtual Result<void, Physics3DError> setMassProperties(Entity entity, const MassProperties& props) = 0;
    virtual Result<MassProperties, Physics3DError> getMassProperties(Entity entity) const = 0;

    /// Get center of mass in world space
    virtual Result<Vec3, Physics3DError> getCenterOfMass(Entity entity) const = 0;

    /// Get inertia tensor
    virtual Result<Mat3, Physics3DError> getInertiaTensor(Entity entity) const = 0;

    //======================================================================
    // Motion Quality (CCD)
    //======================================================================

    virtual Result<void, Physics3DError> setMotionQuality(Entity entity, MotionQuality quality) = 0;
    virtual Result<MotionQuality, Physics3DError> getMotionQuality(Entity entity) const = 0;

    //======================================================================
    // Collision Filtering
    //======================================================================

    virtual Result<void, Physics3DError> setCollisionLayer(Entity entity, CollisionLayer3D layer) = 0;
    virtual Result<void, Physics3DError> setCollisionMask(Entity entity, CollisionMask3D mask) = 0;
    virtual Result<void, Physics3DError> setSensor(Entity entity, bool isSensor) = 0;

    //======================================================================
    // Sleep State
    //======================================================================

    virtual Result<bool, Physics3DError> isAwake(Entity entity) const = 0;
    virtual Result<void, Physics3DError> wakeUp(Entity entity) = 0;
    virtual Result<void, Physics3DError> putToSleep(Entity entity) = 0;

    //======================================================================
    // Bounding Box
    //======================================================================

    virtual Result<AABB3D, Physics3DError> getBodyBounds(Entity entity) const = 0;

    //======================================================================
    // Raycasting
    //======================================================================

    virtual std::optional<RaycastHit3D> raycast(Vec3 origin, Vec3 direction, float maxDistance,
                                                 const QueryFilter3D& filter = {}) const = 0;

    virtual std::vector<RaycastHit3D> raycastAll(Vec3 origin, Vec3 direction, float maxDistance,
                                                   const QueryFilter3D& filter = {}) const = 0;

    //======================================================================
    // Shape Casting
    //======================================================================

    virtual std::optional<ShapeCastHit3D> sphereCast(Vec3 origin, float radius, Vec3 direction,
                                                      float maxDistance,
                                                      const QueryFilter3D& filter = {}) const = 0;

    virtual std::optional<ShapeCastHit3D> boxCast(Vec3 origin, Vec3 halfExtents, Quat rotation,
                                                   Vec3 direction, float maxDistance,
                                                   const QueryFilter3D& filter = {}) const = 0;

    virtual std::optional<ShapeCastHit3D> capsuleCast(Vec3 origin, float radius, float halfHeight,
                                                       Quat rotation, Vec3 direction, float maxDistance,
                                                       const QueryFilter3D& filter = {}) const = 0;

    //======================================================================
    // Overlap Queries
    //======================================================================

    virtual std::vector<Entity> overlapSphere(Vec3 center, float radius,
                                               const QueryFilter3D& filter = {}) const = 0;

    virtual std::vector<Entity> overlapBox(Vec3 center, Vec3 halfExtents, Quat rotation,
                                            const QueryFilter3D& filter = {}) const = 0;

    virtual std::vector<Entity> queryAABB(Vec3 min, Vec3 max,
                                          const QueryFilter3D& filter = {}) const = 0;

    //======================================================================
    // Constraints
    //======================================================================

    virtual Result<UUID, Physics3DError> createConstraint(const ConstraintDef3D& def) = 0;
    virtual Result<void, Physics3DError> destroyConstraint(UUID constraintId) = 0;
    virtual Result<void, Physics3DError> setConstraintEnabled(UUID constraintId, bool enabled) = 0;
    virtual std::vector<UUID> getConstraints(Entity entity) const = 0;

    //======================================================================
    // Constraint Modification
    //======================================================================

    /// Set hinge constraint limits
    virtual Result<void, Physics3DError> setHingeLimits(
        UUID constraintId,
        float minAngle,
        float maxAngle) = 0;

    /// Set hinge motor
    virtual Result<void, Physics3DError> setHingeMotor(
        UUID constraintId,
        float targetVelocity,
        float maxTorque) = 0;

    /// Set slider constraint limits
    virtual Result<void, Physics3DError> setSliderLimits(
        UUID constraintId,
        float minDistance,
        float maxDistance) = 0;

    /// Set slider motor
    virtual Result<void, Physics3DError> setSliderMotor(
        UUID constraintId,
        float targetVelocity,
        float maxForce) = 0;

    /// Get constraint force magnitude (for breaking constraints)
    virtual Result<float, Physics3DError> getConstraintForce(UUID constraintId) const = 0;

    //======================================================================
    // Contact Queries
    //======================================================================

    /// Get all active contact pairs for an entity
    virtual std::vector<ContactPair3D> getContacts(Entity entity) const = 0;

    /// Get all active contact pairs in the world
    virtual std::vector<ContactPair3D> getAllContacts() const = 0;

    /// Check if two entities are in contact
    virtual bool areInContact(Entity a, Entity b) const = 0;

    /// Get the contact pair between two entities (if any)
    virtual std::optional<ContactPair3D> getContactPair(Entity a, Entity b) const = 0;

    //======================================================================
    // Character Controller
    //======================================================================

    virtual Result<void, Physics3DError> createCharacter(Entity entity,
                                                          const CharacterControllerDef& def) = 0;
    virtual Result<void, Physics3DError> destroyCharacter(Entity entity) = 0;

    virtual Result<void, Physics3DError> moveCharacter(Entity entity, Vec3 velocity,
                                                        DeltaTime dt) = 0;

    virtual Result<Vec3, Physics3DError> getCharacterPosition(Entity entity) const = 0;
    virtual Result<void, Physics3DError> setCharacterPosition(Entity entity, Vec3 position) = 0;

    virtual Result<CharacterGroundInfo, Physics3DError> getCharacterGroundInfo(Entity entity) const = 0;
    virtual Result<Vec3, Physics3DError> getCharacterVelocity(Entity entity) const = 0;

    //======================================================================
    // Vehicle
    //======================================================================

    virtual Result<void, Physics3DError> createVehicle(Entity entity, const VehicleDef& def) = 0;
    virtual Result<void, Physics3DError> destroyVehicle(Entity entity) = 0;

    virtual Result<void, Physics3DError> updateVehicle(Entity entity, float throttle,
                                                        float steering, float brake) = 0;

    virtual Result<WheelState, Physics3DError> getWheelTransform(Entity entity,
                                                                  std::uint32_t wheelIndex) const = 0;
    virtual Result<bool, Physics3DError> isWheelGrounded(Entity entity,
                                                          std::uint32_t wheelIndex) const = 0;
    virtual Result<float, Physics3DError> getVehicleSpeed(Entity entity) const = 0;

    //======================================================================
    // World Settings
    //======================================================================

    virtual void setGravity(Vec3 gravity) = 0;
    virtual Vec3 getGravity() const = 0;

    //======================================================================
    // Collision Callbacks
    //======================================================================

    virtual void setCollisionCallback(Collision3DCallback callback) = 0;
    virtual void setTriggerEnterCallback(Trigger3DEnterCallback callback) = 0;
    virtual void setTriggerExitCallback(Trigger3DExitCallback callback) = 0;

    //======================================================================
    // Debug Visualization
    //======================================================================

    virtual void setDebugDraw(bool enabled) = 0;
    virtual std::vector<DebugLine3D> getDebugLines() const = 0;

    //======================================================================
    // Statistics
    //======================================================================

    virtual PhysicsStats3D getStats() const = 0;
};

//==========================================================================
// Floating Origin System Interface
//==========================================================================
// The floating origin system enables rendering of astronomical-scale worlds
// by keeping the camera near the coordinate origin and rebasing all world
// coordinates when the camera moves too far from the origin.

/// Interface for floating origin management
class IFloatingOriginSystem {
public:
    virtual ~IFloatingOriginSystem() = default;

    //----------------------------------------------------------------------
    // Core Operations
    //----------------------------------------------------------------------

    /// Set the camera's absolute world position (double precision)
    virtual void setCameraWorldPosition(const Vec3d& worldPosition) = 0;

    /// Get the current floating origin in world coordinates
    [[nodiscard]] virtual Vec3d getOrigin() const = 0;

    /// Get the camera position relative to the current origin (for rendering)
    [[nodiscard]] virtual Vec3 getCameraRenderPosition() const = 0;

    /// Get the camera's absolute world position
    [[nodiscard]] virtual Vec3d getCameraWorldPosition() const = 0;

    //----------------------------------------------------------------------
    // Coordinate Conversion
    //----------------------------------------------------------------------

    /// Convert an absolute world position to camera-relative render position
    [[nodiscard]] virtual Vec3 toRenderPosition(const Vec3d& worldPosition) const = 0;

    /// Convert a camera-relative render position back to world coordinates
    [[nodiscard]] virtual Vec3d toWorldPosition(const Vec3& renderPosition) const = 0;

    /// Convert a CelestialTransform to a render-ready Transform3D
    [[nodiscard]] virtual Transform3D toRenderTransform(const CelestialTransform& celestial) const = 0;

    //----------------------------------------------------------------------
    // Origin Shift Management
    //----------------------------------------------------------------------

    /// Get the current shift epoch (increments each time origin shifts)
    [[nodiscard]] virtual std::uint32_t getShiftEpoch() const = 0;

    /// Check if an entity needs rebasing based on its last known epoch
    [[nodiscard]] virtual bool needsRebase(std::uint32_t lastEpoch) const = 0;

    /// Force an origin shift to a specific position
    virtual void forceOriginShift(const Vec3d& newOrigin) = 0;

    //----------------------------------------------------------------------
    // Configuration
    //----------------------------------------------------------------------

    /// Set the floating origin configuration
    virtual void setConfig(const FloatingOriginConfig& config) = 0;

    /// Get the current configuration
    [[nodiscard]] virtual FloatingOriginConfig getConfig() const = 0;

    //----------------------------------------------------------------------
    // System Update
    //----------------------------------------------------------------------

    /// Update the system (call once per frame)
    virtual void update(DeltaTime dt) = 0;
};

//==========================================================================
// N-Body Gravity System Interface
//==========================================================================
// Provides realistic gravitational simulation for celestial bodies.

/// Keplerian orbital elements for analytical orbit representation
struct OrbitalElements {
    double semiMajorAxis = 0.0;      // a - meters
    double eccentricity = 0.0;       // e - dimensionless [0, 1) for ellipse
    double inclination = 0.0;        // i - radians
    double longitudeOfAscendingNode = 0.0;  // Ω (RAAN) - radians
    double argumentOfPeriapsis = 0.0;       // ω - radians
    double meanAnomaly = 0.0;        // M - radians
    double epoch = 0.0;              // Reference time for mean anomaly
};

/// State vector (position + velocity) for a body
struct OrbitalState {
    Vec3d position{0.0, 0.0, 0.0};   // Position in meters
    Vec3d velocity{0.0, 0.0, 0.0};   // Velocity in m/s
};

/// Integration method for orbital simulation
enum class IntegrationMethod : std::uint8_t {
    VelocityVerlet,   // Symplectic, energy-conserving, recommended for orbits
    RungeKutta4,      // High accuracy for short periods
    Euler,            // Fast but unstable - only for debugging
    Kepler            // Analytical propagation - for time-accelerated simulation
};

/// Interface for N-body gravitational simulation
class IGravitySystem {
public:
    virtual ~IGravitySystem() = default;

    //----------------------------------------------------------------------
    // Body Management
    //----------------------------------------------------------------------

    virtual void registerBody(Entity entity, const CelestialBodyDef& def) = 0;
    virtual void unregisterBody(Entity entity) = 0;
    [[nodiscard]] virtual std::optional<OrbitalState> getBodyState(Entity entity) const = 0;
    virtual void setBodyState(Entity entity, const OrbitalState& state) = 0;

    //----------------------------------------------------------------------
    // Gravity Calculation
    //----------------------------------------------------------------------

    [[nodiscard]] virtual Vec3d calculateGravityAt(const Vec3d& position) const = 0;
    [[nodiscard]] virtual std::optional<Entity> getDominantBody(const Vec3d& position) const = 0;
    [[nodiscard]] virtual double calculateSOI(Entity body) const = 0;

    //----------------------------------------------------------------------
    // Orbit Prediction
    //----------------------------------------------------------------------

    [[nodiscard]] virtual OrbitalElements stateToElements(
        const OrbitalState& state, double centralBodyMu) const = 0;
    [[nodiscard]] virtual OrbitalState elementsToState(
        const OrbitalElements& elements, double centralBodyMu) const = 0;
    [[nodiscard]] virtual OrbitalElements propagateElements(
        const OrbitalElements& elements, double centralBodyMu, double deltaTime) const = 0;
    [[nodiscard]] virtual std::vector<Vec3d> predictTrajectory(
        Entity body, double duration, int numPoints) const = 0;

    //----------------------------------------------------------------------
    // Simulation Control
    //----------------------------------------------------------------------

    virtual void setIntegrationMethod(IntegrationMethod method) = 0;
    virtual void setTimeAcceleration(double factor) = 0;
    [[nodiscard]] virtual double getTimeAcceleration() const = 0;
    virtual void setPaused(bool paused) = 0;
    [[nodiscard]] virtual bool isPaused() const = 0;
    virtual void update(DeltaTime dt) = 0;
};

//==========================================================================
// Planetary LOD System Interface
//==========================================================================
// Implements CDLOD (Chunked LOD) for seamless planetary terrain rendering.

/// Cube face for cube-to-sphere projection
enum class CubeFace : std::uint8_t {
    PositiveX = 0, NegativeX = 1,
    PositiveY = 2, NegativeY = 3,
    PositiveZ = 4, NegativeZ = 5
};

/// Configuration for a planet's terrain
struct PlanetaryTerrainConfig {
    double radius = 6371000.0;       // Planet radius in meters
    double maxHeight = 10000.0;      // Maximum terrain height
    double atmosphereRadius = 0.0;   // Atmosphere outer radius
    int maxLODLevels = 14;           // Number of LOD levels
    int patchResolution = 32;        // Vertices per side
    float lodDistanceMultiplier = 2.0f;
    bool useTessellation = true;
    bool useProceduralDetail = true;
    float heightMapScale = 1.0f;
};

/// A terrain patch in the LOD quadtree
struct TerrainPatch {
    std::uint64_t id = 0;
    CubeFace face = CubeFace::PositiveZ;
    int lodLevel = 0;
    int quadrant = 0;
    Vec4 uvBounds{0.0f, 0.0f, 1.0f, 1.0f};
    Vec3d boundingCenter{0.0, 0.0, 0.0};
    double boundingRadius = 0.0;
    double cameraDistance = 0.0;
    bool visible = true;
    bool dirty = true;
};

/// Mesh data for a terrain patch
struct TerrainPatchMesh {
    std::vector<Vec3> vertices;
    std::vector<Vec3> normals;
    std::vector<Vec2> texCoords;
    std::vector<std::uint32_t> indices;
    std::vector<Vec3> skirtVertices;
    std::vector<std::uint32_t> skirtIndices;
};

/// Height sample callback for terrain generation
using HeightSampleCallback = std::function<float(double lat, double lon, CubeFace face, Vec2 uv)>;

/// Interface for planetary terrain LOD management
class IPlanetaryLODSystem {
public:
    virtual ~IPlanetaryLODSystem() = default;

    //----------------------------------------------------------------------
    // Planet Registration
    //----------------------------------------------------------------------

    virtual void registerPlanet(Entity entity, const PlanetaryTerrainConfig& config) = 0;
    virtual void unregisterPlanet(Entity entity) = 0;
    virtual void setHeightSampler(Entity entity, HeightSampleCallback sampler) = 0;

    //----------------------------------------------------------------------
    // LOD Updates
    //----------------------------------------------------------------------

    virtual void update(const Vec3d& cameraWorldPosition, DeltaTime dt) = 0;
    [[nodiscard]] virtual std::span<const TerrainPatch> getVisiblePatches(Entity planet) const = 0;
    [[nodiscard]] virtual const TerrainPatchMesh* getPatchMesh(std::uint64_t patchId) const = 0;

    //----------------------------------------------------------------------
    // Queries
    //----------------------------------------------------------------------

    [[nodiscard]] virtual int getLODLevel(Entity planet) const = 0;
    [[nodiscard]] virtual double getHeightAt(Entity planet, const Vec3d& worldPos) const = 0;
    [[nodiscard]] virtual Vec3 getSurfaceNormal(Entity planet, const Vec3d& worldPos) const = 0;
    [[nodiscard]] virtual Vec3d latLonToWorld(Entity planet, double latitude, double longitude,
                                               double altitude = 0.0) const = 0;
    [[nodiscard]] virtual void worldToLatLon(Entity planet, const Vec3d& worldPos,
                                              double& outLatitude, double& outLongitude,
                                              double& outAltitude) const = 0;

    //----------------------------------------------------------------------
    // Configuration
    //----------------------------------------------------------------------

    virtual void setPlanetTransform(Entity planet, const CelestialTransform& transform) = 0;
    [[nodiscard]] virtual PlanetaryTerrainConfig getConfig(Entity planet) const = 0;
    virtual void setConfig(Entity planet, const PlanetaryTerrainConfig& config) = 0;

    //----------------------------------------------------------------------
    // Debug
    //----------------------------------------------------------------------

    struct LODStats {
        int visiblePatches = 0;
        int totalPatches = 0;
        int currentLODLevel = 0;
        std::size_t meshMemoryUsed = 0;
        int patchesSplit = 0;
        int patchesMerged = 0;
    };

    [[nodiscard]] virtual LODStats getStats(Entity planet) const = 0;
};

}  // namespace bestow
