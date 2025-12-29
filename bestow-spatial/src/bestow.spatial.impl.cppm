// bestow-spatial/src/bestow.spatial.impl.cppm
// Spatial systems implementation module

module;

#include <kangaru/kangaru.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/quaternion_double.hpp>

export module bestow.spatial.impl;

import std;
import bestow.services;

export namespace bestow::spatial {

//==========================================================================
// Floating Origin System Implementation
//==========================================================================

class FloatingOriginSystem : public IFloatingOriginSystem {
public:
    // Kangaru service definition
    struct Service : kgr::single_service<FloatingOriginSystem>,
                     kgr::overrides<IFloatingOriginSystemService> {
        static auto construct(kgr::inject_t<IEventSystemService> events)
            -> kgr::inject_result<IEventSystem*> {
            return kgr::inject(&events.service());
        }
    };

    explicit FloatingOriginSystem(IEventSystem* events = nullptr);
    ~FloatingOriginSystem() override;

    // IFloatingOriginSystem interface
    void setCameraWorldPosition(const Vec3d& worldPosition) override;
    [[nodiscard]] Vec3d getOrigin() const override;
    [[nodiscard]] Vec3 getCameraRenderPosition() const override;
    [[nodiscard]] Vec3d getCameraWorldPosition() const override;

    [[nodiscard]] Vec3 toRenderPosition(const Vec3d& worldPosition) const override;
    [[nodiscard]] Vec3d toWorldPosition(const Vec3& renderPosition) const override;
    [[nodiscard]] Transform3D toRenderTransform(const CelestialTransform& celestial) const override;

    [[nodiscard]] std::uint32_t getShiftEpoch() const override;
    [[nodiscard]] bool needsRebase(std::uint32_t lastEpoch) const override;
    void forceOriginShift(const Vec3d& newOrigin) override;

    void setConfig(const FloatingOriginConfig& config) override;
    [[nodiscard]] FloatingOriginConfig getConfig() const override;

    void update(DeltaTime dt) override;

private:
    void performOriginShift(const Vec3d& newOrigin);

    IEventSystem* pIEventSystem_ = nullptr;
    FloatingOriginConfig config_;
    Vec3d currentOrigin_;
    Vec3d cameraWorldPosition_;
    std::uint32_t shiftEpoch_;
    int framesSinceLastShift_;
};

//==========================================================================
// N-Body Gravity System Implementation
//==========================================================================

class GravitySystem : public IGravitySystem {
public:
    // Kangaru service definition
    struct Service : kgr::single_service<GravitySystem>,
                     kgr::overrides<IGravitySystemService> {
        static auto construct(kgr::inject_t<IEventSystemService> events)
            -> kgr::inject_result<IEventSystem*> {
            return kgr::inject(&events.service());
        }
    };

    explicit GravitySystem(IEventSystem* events = nullptr);
    ~GravitySystem() override;

    // IGravitySystem interface
    void registerBody(Entity entity, const CelestialBodyDef& def) override;
    void unregisterBody(Entity entity) override;
    [[nodiscard]] std::optional<OrbitalState> getBodyState(Entity entity) const override;
    void setBodyState(Entity entity, const OrbitalState& state) override;

    [[nodiscard]] Vec3d calculateGravityAt(const Vec3d& position) const override;
    [[nodiscard]] std::optional<Entity> getDominantBody(const Vec3d& position) const override;
    [[nodiscard]] double calculateSOI(Entity body) const override;

    [[nodiscard]] OrbitalElements stateToElements(
        const OrbitalState& state,
        double centralBodyMu) const override;
    [[nodiscard]] OrbitalState elementsToState(
        const OrbitalElements& elements,
        double centralBodyMu) const override;
    [[nodiscard]] OrbitalElements propagateElements(
        const OrbitalElements& elements,
        double centralBodyMu,
        double deltaTime) const override;
    [[nodiscard]] std::vector<Vec3d> predictTrajectory(
        Entity body,
        double duration,
        int numPoints) const override;

    void setIntegrationMethod(IntegrationMethod method) override;
    void setTimeAcceleration(double factor) override;
    [[nodiscard]] double getTimeAcceleration() const override;
    void setPaused(bool paused) override;
    [[nodiscard]] bool isPaused() const override;

    void update(DeltaTime dt) override;

private:
    struct BodyData {
        CelestialBodyDef def;
        OrbitalState state;
    };

    void integrateVelocityVerlet(double dt);
    void integrateRungeKutta4(double dt);
    void propagateKepler(double dt);
    Vec3d calculateAcceleration(const Vec3d& position, Entity excludeBody) const;

    IEventSystem* pIEventSystem_ = nullptr;
    std::unordered_map<Entity, BodyData> bodies_;
    IntegrationMethod integrationMethod_ = IntegrationMethod::VelocityVerlet;
    double timeAcceleration_ = 1.0;
    bool paused_ = false;
};

//==========================================================================
// Planetary LOD System Implementation
//==========================================================================

class PlanetaryLODSystem : public IPlanetaryLODSystem {
public:
    // Kangaru service definition
    struct Service : kgr::single_service<PlanetaryLODSystem>,
                     kgr::overrides<IPlanetaryLODSystemService> {
        static auto construct(kgr::inject_t<IEventSystemService> events,
                              kgr::inject_t<IFloatingOriginSystemService> origin)
            -> kgr::inject_result<IEventSystem*, IFloatingOriginSystem*> {
            return kgr::inject(&events.service(), &origin.service());
        }
    };

    explicit PlanetaryLODSystem(IEventSystem* events = nullptr,
                                 IFloatingOriginSystem* origin = nullptr);
    ~PlanetaryLODSystem() override;

    // IPlanetaryLODSystem interface
    void registerPlanet(Entity entity, const PlanetaryTerrainConfig& config) override;
    void unregisterPlanet(Entity entity) override;
    void setHeightSampler(Entity entity, HeightSampleCallback sampler) override;

    void update(const Vec3d& cameraWorldPosition, DeltaTime dt) override;

    [[nodiscard]] std::span<const TerrainPatch> getVisiblePatches(Entity planet) const override;
    [[nodiscard]] const TerrainPatchMesh* getPatchMesh(std::uint64_t patchId) const override;

    [[nodiscard]] int getLODLevel(Entity planet) const override;
    [[nodiscard]] double getHeightAt(Entity planet, const Vec3d& worldPos) const override;
    [[nodiscard]] Vec3 getSurfaceNormal(Entity planet, const Vec3d& worldPos) const override;

    [[nodiscard]] Vec3d latLonToWorld(Entity planet, double latitude, double longitude,
                                       double altitude = 0.0) const override;
    [[nodiscard]] void worldToLatLon(Entity planet, const Vec3d& worldPos,
                                      double& outLatitude, double& outLongitude,
                                      double& outAltitude) const override;

    void setPlanetTransform(Entity planet, const CelestialTransform& transform) override;
    [[nodiscard]] PlanetaryTerrainConfig getConfig(Entity planet) const override;
    void setConfig(Entity planet, const PlanetaryTerrainConfig& config) override;

    [[nodiscard]] LODStats getStats(Entity planet) const override;

private:
    // Quadtree node for LOD management
    struct QuadNode {
        TerrainPatch patch;
        std::array<std::unique_ptr<QuadNode>, 4> children;
        QuadNode* parent = nullptr;
        bool isLeaf = true;
    };

    // Planet data
    struct PlanetData {
        PlanetaryTerrainConfig config;
        CelestialTransform transform;
        HeightSampleCallback heightSampler;

        // Quadtree roots (one per cube face)
        std::array<std::unique_ptr<QuadNode>, 6> faceRoots;

        // Visible patches (updated each frame)
        std::vector<TerrainPatch> visiblePatches;

        // Current camera distance for LOD selection
        double cameraDistance = 0.0;
        int currentLODLevel = 0;

        // Stats
        LODStats stats;
    };

    // Helper methods
    void initializePlanetQuadtree(PlanetData& data);
    void updateLOD(PlanetData& data, const Vec3d& cameraPos);
    void updateNodeLOD(QuadNode* node, const Vec3d& cameraPos, PlanetData& data);
    void collectVisiblePatches(QuadNode* node, const Vec3d& cameraPos,
                                std::vector<TerrainPatch>& outPatches);
    bool shouldSplit(const QuadNode* node, const Vec3d& cameraPos,
                     const PlanetaryTerrainConfig& config) const;
    bool shouldMerge(const QuadNode* node, const Vec3d& cameraPos,
                     const PlanetaryTerrainConfig& config) const;
    void splitNode(QuadNode* node, const PlanetData& data);
    void mergeNode(QuadNode* node);

    Vec3d cubeToSphere(CubeFace face, double u, double v, double radius) const;
    void sphereToLatLon(const Vec3d& spherePos, double& lat, double& lon) const;
    Vec3d latLonToSphere(double lat, double lon, double radius) const;

    TerrainPatchMesh generatePatchMesh(const TerrainPatch& patch,
                                        const PlanetData& data) const;

    IEventSystem* pIEventSystem_ = nullptr;
    IFloatingOriginSystem* pIFloatingOriginSystem_ = nullptr;
    std::unordered_map<Entity, PlanetData> planets_;
    mutable std::unordered_map<std::uint64_t, TerrainPatchMesh> meshCache_;
    std::uint64_t nextPatchId_ = 1;
};

}  // namespace bestow::spatial
