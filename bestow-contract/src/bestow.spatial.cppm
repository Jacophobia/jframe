// bestow-contract/src/bestow.spatial.cppm
// Spatial systems interface for large-world rendering (floating origin, LOD, etc.)

module;

#include <functional>
#include <optional>
#include <span>

export module bestow.spatial;

import bestow.types;

export namespace bestow {

//==========================================================================
// Floating Origin System Interface
//==========================================================================
// The floating origin system enables rendering of astronomical-scale worlds
// by keeping the camera near the coordinate origin and rebasing all world
// coordinates when the camera moves too far from the origin.
//
// This is essential for space games where distances can span billions of km,
// far exceeding the precision of 32-bit floats used by GPUs.
//
// Usage:
// 1. Store all world positions using CelestialTransform (double precision)
// 2. Call setCameraWorldPosition() with the camera's absolute position
// 3. Use toRenderPosition() to convert world positions to camera-relative
// 4. Subscribe to origin shift events to update custom systems

/// Interface for floating origin management
class IFloatingOriginSystem {
public:
    virtual ~IFloatingOriginSystem() = default;

    //----------------------------------------------------------------------
    // Core Operations
    //----------------------------------------------------------------------

    /// Set the camera's absolute world position (double precision)
    /// This is the primary input - call every frame with camera position
    /// If the camera moves beyond the shift threshold, an origin shift is triggered
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
    /// This is the primary method for preparing objects for GPU rendering
    /// @param worldPosition Absolute world position in meters (double precision)
    /// @return Position relative to camera origin (single precision for GPU)
    [[nodiscard]] virtual Vec3 toRenderPosition(const Vec3d& worldPosition) const = 0;

    /// Convert a camera-relative render position back to world coordinates
    /// @param renderPosition Position relative to camera origin
    /// @return Absolute world position in meters
    [[nodiscard]] virtual Vec3d toWorldPosition(const Vec3& renderPosition) const = 0;

    /// Convert a CelestialTransform to a render-ready Transform3D
    /// Handles both position and rotation conversion
    [[nodiscard]] virtual Transform3D toRenderTransform(const CelestialTransform& celestial) const = 0;

    //----------------------------------------------------------------------
    // Origin Shift Management
    //----------------------------------------------------------------------

    /// Get the current shift epoch (increments each time origin shifts)
    /// Use this to detect if an entity needs rebasing
    [[nodiscard]] virtual std::uint32_t getShiftEpoch() const = 0;

    /// Check if an entity needs rebasing based on its last known epoch
    [[nodiscard]] virtual bool needsRebase(std::uint32_t lastEpoch) const = 0;

    /// Force an origin shift to a specific position
    /// Normally shifts are automatic, but this allows manual control
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
    /// Checks if origin shift is needed and processes pending operations
    virtual void update(DeltaTime dt) = 0;
};

//==========================================================================
// N-Body Gravity System Interface
//==========================================================================
// Provides realistic gravitational simulation for celestial bodies.
// Uses double-precision throughout for accuracy at solar system scales.

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

    /// Register a celestial body for gravity simulation
    /// @param entity The entity representing this body
    /// @param def Body definition (mass, position, velocity, etc.)
    virtual void registerBody(Entity entity, const CelestialBodyDef& def) = 0;

    /// Unregister a body from simulation
    virtual void unregisterBody(Entity entity) = 0;

    /// Get the current state of a body
    [[nodiscard]] virtual std::optional<OrbitalState> getBodyState(Entity entity) const = 0;

    /// Set the state of a body (for manual adjustments)
    virtual void setBodyState(Entity entity, const OrbitalState& state) = 0;

    //----------------------------------------------------------------------
    // Gravity Calculation
    //----------------------------------------------------------------------

    /// Calculate gravitational acceleration at a point from all bodies
    /// @param position World position to calculate gravity at
    /// @return Gravitational acceleration vector in m/s²
    [[nodiscard]] virtual Vec3d calculateGravityAt(const Vec3d& position) const = 0;

    /// Find the dominant gravitational body at a position (for SOI)
    /// @param position World position
    /// @return Entity of the dominant body, or nullopt if none
    [[nodiscard]] virtual std::optional<Entity> getDominantBody(const Vec3d& position) const = 0;

    /// Calculate the Sphere of Influence radius for a body relative to its parent
    [[nodiscard]] virtual double calculateSOI(Entity body) const = 0;

    //----------------------------------------------------------------------
    // Orbit Prediction
    //----------------------------------------------------------------------

    /// Convert state vector to Keplerian elements
    [[nodiscard]] virtual OrbitalElements stateToElements(
        const OrbitalState& state,
        double centralBodyMu) const = 0;

    /// Convert Keplerian elements to state vector
    [[nodiscard]] virtual OrbitalState elementsToState(
        const OrbitalElements& elements,
        double centralBodyMu) const = 0;

    /// Propagate orbital elements forward in time
    [[nodiscard]] virtual OrbitalElements propagateElements(
        const OrbitalElements& elements,
        double centralBodyMu,
        double deltaTime) const = 0;

    /// Predict trajectory (returns positions at intervals)
    [[nodiscard]] virtual std::vector<Vec3d> predictTrajectory(
        Entity body,
        double duration,
        int numPoints) const = 0;

    //----------------------------------------------------------------------
    // Simulation Control
    //----------------------------------------------------------------------

    /// Set the integration method
    virtual void setIntegrationMethod(IntegrationMethod method) = 0;

    /// Set time acceleration factor (1.0 = real-time)
    /// High values automatically switch to Kepler propagation
    virtual void setTimeAcceleration(double factor) = 0;

    /// Get current time acceleration
    [[nodiscard]] virtual double getTimeAcceleration() const = 0;

    /// Pause/resume gravity simulation
    virtual void setPaused(bool paused) = 0;
    [[nodiscard]] virtual bool isPaused() const = 0;

    /// Update the simulation (call once per frame)
    virtual void update(DeltaTime dt) = 0;
};

//==========================================================================
// Planetary LOD System Interface
//==========================================================================
// Implements CDLOD (Chunked LOD) for seamless planetary terrain rendering.
// Uses cube-to-sphere projection for minimal distortion.
//
// LOD Levels:
// 0 = Orbital view (icosphere or low-poly sphere)
// 1-6 = Orbital descent (increasing terrain detail)
// 7-12 = Surface view (high-detail patches with GPU tessellation)
// 13+ = Ground level (procedural detail, rocks, vegetation)

/// Cube face for cube-to-sphere projection
enum class CubeFace : std::uint8_t {
    PositiveX = 0,
    NegativeX = 1,
    PositiveY = 2,
    NegativeY = 3,
    PositiveZ = 4,
    NegativeZ = 5
};

/// Configuration for a planet's terrain
struct PlanetaryTerrainConfig {
    /// Planet radius in meters
    double radius = 6371000.0;  // Earth radius default

    /// Maximum terrain height above radius
    double maxHeight = 10000.0;

    /// Atmosphere outer radius (for rendering transitions)
    double atmosphereRadius = 0.0;

    /// Number of LOD levels (typically 12-16)
    int maxLODLevels = 14;

    /// Base patch resolution (vertices per side, typically 32 or 64)
    int patchResolution = 32;

    /// LOD distance multiplier (higher = more detail at distance)
    float lodDistanceMultiplier = 2.0f;

    /// Enable GPU tessellation for smooth LOD transitions
    bool useTessellation = true;

    /// Enable procedural detail on closest LODs
    bool useProceduralDetail = true;

    /// Height map scaling factor
    float heightMapScale = 1.0f;
};

/// A terrain patch in the LOD quadtree
struct TerrainPatch {
    /// Unique identifier for this patch
    std::uint64_t id = 0;

    /// Cube face this patch belongs to
    CubeFace face = CubeFace::PositiveZ;

    /// LOD level (0 = largest/coarsest, higher = smaller/finer)
    int lodLevel = 0;

    /// Position in quadtree (0-3 for children of parent)
    int quadrant = 0;

    /// UV bounds on the cube face [uMin, vMin, uMax, vMax]
    Vec4 uvBounds{0.0f, 0.0f, 1.0f, 1.0f};

    /// World-space bounding sphere center
    Vec3d boundingCenter{0.0, 0.0, 0.0};

    /// World-space bounding sphere radius
    double boundingRadius = 0.0;

    /// Distance to camera (for LOD selection)
    double cameraDistance = 0.0;

    /// Is this patch currently visible?
    bool visible = true;

    /// Does this patch need mesh regeneration?
    bool dirty = true;
};

/// Mesh data for a terrain patch
struct TerrainPatchMesh {
    std::vector<Vec3> vertices;
    std::vector<Vec3> normals;
    std::vector<Vec2> texCoords;
    std::vector<std::uint32_t> indices;

    /// Skirt vertices for seamless LOD transitions
    std::vector<Vec3> skirtVertices;
    std::vector<std::uint32_t> skirtIndices;
};

/// Height sample callback (for terrain generation)
/// Parameters: latitude (radians), longitude (radians), face, uv
/// Returns: height above radius in meters
using HeightSampleCallback = std::function<float(double lat, double lon, CubeFace face, Vec2 uv)>;

/// Interface for planetary terrain LOD management
class IPlanetaryLODSystem {
public:
    virtual ~IPlanetaryLODSystem() = default;

    //----------------------------------------------------------------------
    // Planet Registration
    //----------------------------------------------------------------------

    /// Register a planet for LOD management
    /// @param entity The entity representing this planet
    /// @param config Terrain configuration
    virtual void registerPlanet(Entity entity, const PlanetaryTerrainConfig& config) = 0;

    /// Unregister a planet from LOD management
    virtual void unregisterPlanet(Entity entity) = 0;

    /// Set the height sampling function for a planet
    virtual void setHeightSampler(Entity entity, HeightSampleCallback sampler) = 0;

    //----------------------------------------------------------------------
    // LOD Updates
    //----------------------------------------------------------------------

    /// Update LOD based on camera position
    /// This will split/merge patches as needed based on distance
    virtual void update(const Vec3d& cameraWorldPosition, DeltaTime dt) = 0;

    /// Get visible patches for rendering (sorted by distance)
    [[nodiscard]] virtual std::span<const TerrainPatch> getVisiblePatches(Entity planet) const = 0;

    /// Get the mesh for a specific patch (generates if needed)
    [[nodiscard]] virtual const TerrainPatchMesh* getPatchMesh(std::uint64_t patchId) const = 0;

    //----------------------------------------------------------------------
    // Queries
    //----------------------------------------------------------------------

    /// Get current LOD level for a planet at camera position
    [[nodiscard]] virtual int getLODLevel(Entity planet) const = 0;

    /// Get the effective terrain height at a position (samples height map)
    [[nodiscard]] virtual double getHeightAt(Entity planet, const Vec3d& worldPos) const = 0;

    /// Get the surface normal at a position
    [[nodiscard]] virtual Vec3 getSurfaceNormal(Entity planet, const Vec3d& worldPos) const = 0;

    /// Convert lat/lon to world position on planet surface
    [[nodiscard]] virtual Vec3d latLonToWorld(
        Entity planet,
        double latitude,
        double longitude,
        double altitude = 0.0) const = 0;

    /// Convert world position to lat/lon/altitude
    [[nodiscard]] virtual void worldToLatLon(
        Entity planet,
        const Vec3d& worldPos,
        double& outLatitude,
        double& outLongitude,
        double& outAltitude) const = 0;

    //----------------------------------------------------------------------
    // Configuration
    //----------------------------------------------------------------------

    /// Set planet transform (for moving planets)
    virtual void setPlanetTransform(Entity planet, const CelestialTransform& transform) = 0;

    /// Get current terrain configuration
    [[nodiscard]] virtual PlanetaryTerrainConfig getConfig(Entity planet) const = 0;

    /// Update terrain configuration
    virtual void setConfig(Entity planet, const PlanetaryTerrainConfig& config) = 0;

    //----------------------------------------------------------------------
    // Debug
    //----------------------------------------------------------------------

    /// Get statistics for debugging
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
