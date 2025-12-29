// bestow-spatial/src/PlanetaryLODSystem.cpp
// Planetary terrain LOD system implementation using CDLOD algorithm

module;

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/quaternion_double.hpp>
#include <spdlog/spdlog.h>
#include <cmath>
#include <algorithm>

module bestow.spatial.impl;

import std;
import bestow.services;

namespace bestow::spatial {

namespace {
    constexpr double PI = 3.14159265358979323846;
    constexpr double TWO_PI = 2.0 * PI;
    constexpr double HALF_PI = PI * 0.5;
}

PlanetaryLODSystem::PlanetaryLODSystem(IEventSystem* events, IFloatingOriginSystem* origin)
    : pIEventSystem_(events)
    , pIFloatingOriginSystem_(origin)
{
    spdlog::info("[PlanetaryLODSystem] Initialized");
}

PlanetaryLODSystem::~PlanetaryLODSystem() = default;

void PlanetaryLODSystem::registerPlanet(Entity entity, const PlanetaryTerrainConfig& config) {
    PlanetData data;
    data.config = config;
    data.transform.position = Vec3d{0.0, 0.0, 0.0};
    data.transform.rotation = Quatd{1.0, 0.0, 0.0, 0.0};

    // Initialize the quadtree with root patches for each cube face
    initializePlanetQuadtree(data);

    planets_[entity] = std::move(data);

    spdlog::info("[PlanetaryLODSystem] Registered planet {} with radius {:.0f}m, {} LOD levels",
                 static_cast<std::uint32_t>(entity), config.radius, config.maxLODLevels);
}

void PlanetaryLODSystem::unregisterPlanet(Entity entity) {
    planets_.erase(entity);
}

void PlanetaryLODSystem::setHeightSampler(Entity entity, HeightSampleCallback sampler) {
    auto it = planets_.find(entity);
    if (it != planets_.end()) {
        it->second.heightSampler = std::move(sampler);
    }
}

void PlanetaryLODSystem::initializePlanetQuadtree(PlanetData& data) {
    // Create root nodes for each cube face
    for (int i = 0; i < 6; ++i) {
        auto node = std::make_unique<QuadNode>();
        node->patch.id = nextPatchId_++;
        node->patch.face = static_cast<CubeFace>(i);
        node->patch.lodLevel = 0;
        node->patch.quadrant = 0;
        node->patch.uvBounds = Vec4{0.0f, 0.0f, 1.0f, 1.0f};
        node->patch.dirty = true;
        node->isLeaf = true;

        // Calculate bounding sphere for this face
        // Center is at the face center on the sphere
        Vec3d faceCenter = cubeToSphere(
            static_cast<CubeFace>(i), 0.5, 0.5, data.config.radius);
        node->patch.boundingCenter = faceCenter;
        // Bounding radius covers the entire face (conservative estimate)
        node->patch.boundingRadius = data.config.radius * 0.8;

        data.faceRoots[i] = std::move(node);
    }
}

void PlanetaryLODSystem::update(const Vec3d& cameraWorldPosition, DeltaTime dt) {
    for (auto& [entity, data] : planets_) {
        updateLOD(data, cameraWorldPosition);
    }
}

void PlanetaryLODSystem::updateLOD(PlanetData& data, const Vec3d& cameraPos) {
    // Calculate distance from camera to planet center
    Vec3d planetCenter = data.transform.position;
    double distance = glm::length(cameraPos - planetCenter);
    data.cameraDistance = distance;

    // Determine base LOD level from distance
    double surfaceDistance = distance - data.config.radius;
    if (surfaceDistance < 0) surfaceDistance = 1.0;  // On or below surface

    // LOD level increases as we get closer
    // Each level doubles the detail
    double lodScale = data.config.radius / surfaceDistance;
    data.currentLODLevel = static_cast<int>(std::log2(lodScale));
    data.currentLODLevel = std::clamp(data.currentLODLevel, 0, data.config.maxLODLevels);

    // Clear visible patches
    data.visiblePatches.clear();
    data.stats.patchesSplit = 0;
    data.stats.patchesMerged = 0;

    // Process each face quadtree
    for (auto& root : data.faceRoots) {
        if (root) {
            // Update LOD for this face
            updateNodeLOD(root.get(), cameraPos, data);

            // Collect visible patches
            collectVisiblePatches(root.get(), cameraPos, data.visiblePatches);
        }
    }

    // Sort by distance (front to back for rendering efficiency)
    std::sort(data.visiblePatches.begin(), data.visiblePatches.end(),
        [](const TerrainPatch& a, const TerrainPatch& b) {
            return a.cameraDistance < b.cameraDistance;
        });

    // Update stats
    data.stats.visiblePatches = static_cast<int>(data.visiblePatches.size());
    data.stats.currentLODLevel = data.currentLODLevel;
}

void PlanetaryLODSystem::updateNodeLOD(QuadNode* node, const Vec3d& cameraPos,
                                        PlanetData& data) {
    if (!node) return;

    // Calculate distance from camera to patch
    double dist = glm::length(cameraPos - node->patch.boundingCenter);
    node->patch.cameraDistance = dist;

    if (node->isLeaf) {
        // Check if we should split this node
        if (shouldSplit(node, cameraPos, data.config)) {
            splitNode(node, data);
            ++data.stats.patchesSplit;
        }
    } else {
        // Check if we should merge children back into this node
        if (shouldMerge(node, cameraPos, data.config)) {
            mergeNode(node);
            ++data.stats.patchesMerged;
        } else {
            // Recurse into children
            for (auto& child : node->children) {
                if (child) {
                    updateNodeLOD(child.get(), cameraPos, data);
                }
            }
        }
    }
}

bool PlanetaryLODSystem::shouldSplit(const QuadNode* node, const Vec3d& cameraPos,
                                      const PlanetaryTerrainConfig& config) const {
    if (node->patch.lodLevel >= config.maxLODLevels) {
        return false;  // Already at max LOD
    }

    // Split based on screen-space error metric
    // Larger patches that are close should be split
    double distance = node->patch.cameraDistance;
    double patchSize = node->patch.boundingRadius * 2.0;

    // Screen-space size estimate (proportional to size/distance)
    double screenSize = patchSize / std::max(distance, 1.0);

    // Split threshold - higher values mean more aggressive splitting
    double threshold = 1.0 / config.lodDistanceMultiplier;

    return screenSize > threshold;
}

bool PlanetaryLODSystem::shouldMerge(const QuadNode* node, const Vec3d& cameraPos,
                                      const PlanetaryTerrainConfig& config) const {
    if (node->isLeaf || node->patch.lodLevel == 0) {
        return false;
    }

    // Check if all children are leaves
    for (const auto& child : node->children) {
        if (child && !child->isLeaf) {
            return false;
        }
    }

    // Merge based on distance (reverse of split logic with hysteresis)
    double distance = node->patch.cameraDistance;
    double patchSize = node->patch.boundingRadius * 2.0;
    double screenSize = patchSize / std::max(distance, 1.0);

    // Merge threshold (with hysteresis to prevent flickering)
    double threshold = 0.5 / config.lodDistanceMultiplier;

    return screenSize < threshold;
}

void PlanetaryLODSystem::splitNode(QuadNode* node, const PlanetData& data) {
    if (!node || !node->isLeaf) return;

    node->isLeaf = false;

    // Create 4 children
    for (int q = 0; q < 4; ++q) {
        auto child = std::make_unique<QuadNode>();
        child->parent = node;
        child->patch.id = nextPatchId_++;
        child->patch.face = node->patch.face;
        child->patch.lodLevel = node->patch.lodLevel + 1;
        child->patch.quadrant = q;
        child->isLeaf = true;

        // Calculate child UV bounds
        float uMin = node->patch.uvBounds.x;
        float vMin = node->patch.uvBounds.y;
        float uMax = node->patch.uvBounds.z;
        float vMax = node->patch.uvBounds.w;
        float uMid = (uMin + uMax) * 0.5f;
        float vMid = (vMin + vMax) * 0.5f;

        switch (q) {
            case 0: child->patch.uvBounds = Vec4{uMin, vMin, uMid, vMid}; break;  // Bottom-left
            case 1: child->patch.uvBounds = Vec4{uMid, vMin, uMax, vMid}; break;  // Bottom-right
            case 2: child->patch.uvBounds = Vec4{uMin, vMid, uMid, vMax}; break;  // Top-left
            case 3: child->patch.uvBounds = Vec4{uMid, vMid, uMax, vMax}; break;  // Top-right
        }

        // Calculate bounding sphere for child
        float cu = (child->patch.uvBounds.x + child->patch.uvBounds.z) * 0.5f;
        float cv = (child->patch.uvBounds.y + child->patch.uvBounds.w) * 0.5f;
        child->patch.boundingCenter = cubeToSphere(child->patch.face, cu, cv, data.config.radius);
        child->patch.boundingRadius = node->patch.boundingRadius * 0.5;
        child->patch.dirty = true;

        node->children[q] = std::move(child);
    }
}

void PlanetaryLODSystem::mergeNode(QuadNode* node) {
    if (!node || node->isLeaf) return;

    // Clear mesh cache for children
    for (auto& child : node->children) {
        if (child) {
            meshCache_.erase(child->patch.id);
            child.reset();
        }
    }

    node->isLeaf = true;
    node->patch.dirty = true;
}

void PlanetaryLODSystem::collectVisiblePatches(QuadNode* node, const Vec3d& cameraPos,
                                                std::vector<TerrainPatch>& outPatches) {
    if (!node) return;

    // Simple frustum culling - check if patch is potentially visible
    // (For now, just check if the patch is on the camera-facing hemisphere)
    Vec3d toCamera = glm::normalize(cameraPos - node->patch.boundingCenter);
    Vec3d patchNormal = glm::normalize(node->patch.boundingCenter);

    // Patch is visible if it faces the camera (with some margin for horizon)
    double facing = glm::dot(toCamera, patchNormal);
    if (facing < -0.2) {
        return;  // Back-facing, not visible
    }

    if (node->isLeaf) {
        node->patch.visible = true;
        outPatches.push_back(node->patch);
    } else {
        for (auto& child : node->children) {
            if (child) {
                collectVisiblePatches(child.get(), cameraPos, outPatches);
            }
        }
    }
}

Vec3d PlanetaryLODSystem::cubeToSphere(CubeFace face, double u, double v, double radius) const {
    // Convert UV coordinates to cube coordinates (-1 to 1)
    double cu = u * 2.0 - 1.0;
    double cv = v * 2.0 - 1.0;

    Vec3d cubePos;
    switch (face) {
        case CubeFace::PositiveX: cubePos = Vec3d{ 1.0,   cv,  -cu}; break;
        case CubeFace::NegativeX: cubePos = Vec3d{-1.0,   cv,   cu}; break;
        case CubeFace::PositiveY: cubePos = Vec3d{  cu,  1.0,  -cv}; break;
        case CubeFace::NegativeY: cubePos = Vec3d{  cu, -1.0,   cv}; break;
        case CubeFace::PositiveZ: cubePos = Vec3d{  cu,   cv,  1.0}; break;
        case CubeFace::NegativeZ: cubePos = Vec3d{ -cu,   cv, -1.0}; break;
    }

    // Normalize to project onto sphere
    return glm::normalize(cubePos) * radius;
}

void PlanetaryLODSystem::sphereToLatLon(const Vec3d& spherePos, double& lat, double& lon) const {
    Vec3d norm = glm::normalize(spherePos);
    lat = std::asin(norm.y);
    lon = std::atan2(norm.z, norm.x);
}

Vec3d PlanetaryLODSystem::latLonToSphere(double lat, double lon, double radius) const {
    double cosLat = std::cos(lat);
    return Vec3d{
        cosLat * std::cos(lon) * radius,
        std::sin(lat) * radius,
        cosLat * std::sin(lon) * radius
    };
}

std::span<const TerrainPatch> PlanetaryLODSystem::getVisiblePatches(Entity planet) const {
    auto it = planets_.find(planet);
    if (it != planets_.end()) {
        return it->second.visiblePatches;
    }
    return {};
}

const TerrainPatchMesh* PlanetaryLODSystem::getPatchMesh(std::uint64_t patchId) const {
    auto it = meshCache_.find(patchId);
    if (it != meshCache_.end()) {
        return &it->second;
    }

    // Find the patch and generate mesh
    for (const auto& [entity, data] : planets_) {
        for (const auto& patch : data.visiblePatches) {
            if (patch.id == patchId) {
                auto mesh = generatePatchMesh(patch, data);
                auto [insertIt, inserted] = meshCache_.emplace(patchId, std::move(mesh));
                return &insertIt->second;
            }
        }
    }

    return nullptr;
}

TerrainPatchMesh PlanetaryLODSystem::generatePatchMesh(const TerrainPatch& patch,
                                                        const PlanetData& data) const {
    TerrainPatchMesh mesh;

    int resolution = data.config.patchResolution;
    float uMin = patch.uvBounds.x;
    float vMin = patch.uvBounds.y;
    float uMax = patch.uvBounds.z;
    float vMax = patch.uvBounds.w;
    float uStep = (uMax - uMin) / static_cast<float>(resolution);
    float vStep = (vMax - vMin) / static_cast<float>(resolution);

    // Generate vertices
    for (int v = 0; v <= resolution; ++v) {
        for (int u = 0; u <= resolution; ++u) {
            float uf = uMin + static_cast<float>(u) * uStep;
            float vf = vMin + static_cast<float>(v) * vStep;

            // Get sphere position
            Vec3d spherePos = cubeToSphere(patch.face, uf, vf, data.config.radius);

            // Sample height if sampler is available
            float height = 0.0f;
            if (data.heightSampler) {
                double lat, lon;
                sphereToLatLon(spherePos, lat, lon);
                height = data.heightSampler(lat, lon, patch.face, Vec2{uf, vf});
            }

            // Displace along normal
            Vec3d normal = glm::normalize(spherePos);
            Vec3d displaced = spherePos + normal * static_cast<double>(height);

            // Convert to render-relative position
            Vec3d renderPos = displaced;
            if (pIFloatingOriginSystem_) {
                renderPos = displaced - pIFloatingOriginSystem_->getOrigin();
            }

            mesh.vertices.push_back(Vec3{
                static_cast<float>(renderPos.x),
                static_cast<float>(renderPos.y),
                static_cast<float>(renderPos.z)
            });

            mesh.normals.push_back(Vec3{
                static_cast<float>(normal.x),
                static_cast<float>(normal.y),
                static_cast<float>(normal.z)
            });

            mesh.texCoords.push_back(Vec2{uf, vf});
        }
    }

    // Generate indices
    for (int v = 0; v < resolution; ++v) {
        for (int u = 0; u < resolution; ++u) {
            std::uint32_t topLeft = static_cast<std::uint32_t>(v * (resolution + 1) + u);
            std::uint32_t topRight = topLeft + 1;
            std::uint32_t bottomLeft = topLeft + static_cast<std::uint32_t>(resolution + 1);
            std::uint32_t bottomRight = bottomLeft + 1;

            // Two triangles per quad
            mesh.indices.push_back(topLeft);
            mesh.indices.push_back(bottomLeft);
            mesh.indices.push_back(topRight);

            mesh.indices.push_back(topRight);
            mesh.indices.push_back(bottomLeft);
            mesh.indices.push_back(bottomRight);
        }
    }

    // Generate skirt vertices (for seamless LOD transitions)
    // Skirts extend downward from the patch edges to hide gaps
    float skirtDepth = static_cast<float>(data.config.radius) * 0.001f;  // 0.1% of radius

    auto addSkirtVertex = [&](int u, int v, const Vec3& pos, const Vec3& normal) {
        Vec3 skirtPos = pos - normal * skirtDepth;
        mesh.skirtVertices.push_back(skirtPos);
    };

    // Add skirt vertices for each edge
    int baseIndex = static_cast<int>(mesh.vertices.size());

    // Bottom edge skirt
    for (int u = 0; u <= resolution; ++u) {
        int idx = u;
        addSkirtVertex(u, 0, mesh.vertices[idx], mesh.normals[idx]);
    }
    // Top edge skirt
    for (int u = 0; u <= resolution; ++u) {
        int idx = resolution * (resolution + 1) + u;
        addSkirtVertex(u, resolution, mesh.vertices[idx], mesh.normals[idx]);
    }
    // Left edge skirt
    for (int v = 0; v <= resolution; ++v) {
        int idx = v * (resolution + 1);
        addSkirtVertex(0, v, mesh.vertices[idx], mesh.normals[idx]);
    }
    // Right edge skirt
    for (int v = 0; v <= resolution; ++v) {
        int idx = v * (resolution + 1) + resolution;
        addSkirtVertex(resolution, v, mesh.vertices[idx], mesh.normals[idx]);
    }

    return mesh;
}

int PlanetaryLODSystem::getLODLevel(Entity planet) const {
    auto it = planets_.find(planet);
    if (it != planets_.end()) {
        return it->second.currentLODLevel;
    }
    return 0;
}

double PlanetaryLODSystem::getHeightAt(Entity planet, const Vec3d& worldPos) const {
    auto it = planets_.find(planet);
    if (it == planets_.end() || !it->second.heightSampler) {
        return 0.0;
    }

    const auto& data = it->second;
    Vec3d localPos = worldPos - data.transform.position;

    double lat, lon;
    sphereToLatLon(localPos, lat, lon);

    // Determine cube face from position
    Vec3d absPos = glm::abs(localPos);
    CubeFace face;
    float u, v;

    if (absPos.x >= absPos.y && absPos.x >= absPos.z) {
        face = localPos.x > 0 ? CubeFace::PositiveX : CubeFace::NegativeX;
        u = static_cast<float>((localPos.z / absPos.x + 1.0) * 0.5);
        v = static_cast<float>((localPos.y / absPos.x + 1.0) * 0.5);
    } else if (absPos.y >= absPos.x && absPos.y >= absPos.z) {
        face = localPos.y > 0 ? CubeFace::PositiveY : CubeFace::NegativeY;
        u = static_cast<float>((localPos.x / absPos.y + 1.0) * 0.5);
        v = static_cast<float>((localPos.z / absPos.y + 1.0) * 0.5);
    } else {
        face = localPos.z > 0 ? CubeFace::PositiveZ : CubeFace::NegativeZ;
        u = static_cast<float>((localPos.x / absPos.z + 1.0) * 0.5);
        v = static_cast<float>((localPos.y / absPos.z + 1.0) * 0.5);
    }

    return data.heightSampler(lat, lon, face, Vec2{u, v});
}

Vec3 PlanetaryLODSystem::getSurfaceNormal(Entity planet, const Vec3d& worldPos) const {
    auto it = planets_.find(planet);
    if (it == planets_.end()) {
        return Vec3{0.0f, 1.0f, 0.0f};
    }

    const auto& data = it->second;
    Vec3d localPos = worldPos - data.transform.position;

    // For now, return the sphere normal (radial direction)
    // A more accurate implementation would sample the height map gradient
    Vec3d normal = glm::normalize(localPos);
    return Vec3{
        static_cast<float>(normal.x),
        static_cast<float>(normal.y),
        static_cast<float>(normal.z)
    };
}

Vec3d PlanetaryLODSystem::latLonToWorld(Entity planet, double latitude, double longitude,
                                         double altitude) const {
    auto it = planets_.find(planet);
    if (it == planets_.end()) {
        return Vec3d{0.0, 0.0, 0.0};
    }

    const auto& data = it->second;
    double radius = data.config.radius + altitude;
    Vec3d localPos = latLonToSphere(latitude, longitude, radius);

    // Apply planet rotation and position
    Vec3d rotated = data.transform.rotation * localPos;
    return rotated + data.transform.position;
}

void PlanetaryLODSystem::worldToLatLon(Entity planet, const Vec3d& worldPos,
                                        double& outLatitude, double& outLongitude,
                                        double& outAltitude) const {
    auto it = planets_.find(planet);
    if (it == planets_.end()) {
        outLatitude = 0.0;
        outLongitude = 0.0;
        outAltitude = 0.0;
        return;
    }

    const auto& data = it->second;
    Vec3d localPos = worldPos - data.transform.position;

    // Inverse rotation
    Quatd invRot = glm::inverse(data.transform.rotation);
    localPos = invRot * localPos;

    outAltitude = glm::length(localPos) - data.config.radius;
    sphereToLatLon(localPos, outLatitude, outLongitude);
}

void PlanetaryLODSystem::setPlanetTransform(Entity planet, const CelestialTransform& transform) {
    auto it = planets_.find(planet);
    if (it != planets_.end()) {
        it->second.transform = transform;
    }
}

PlanetaryTerrainConfig PlanetaryLODSystem::getConfig(Entity planet) const {
    auto it = planets_.find(planet);
    if (it != planets_.end()) {
        return it->second.config;
    }
    return {};
}

void PlanetaryLODSystem::setConfig(Entity planet, const PlanetaryTerrainConfig& config) {
    auto it = planets_.find(planet);
    if (it != planets_.end()) {
        it->second.config = config;
        // Reinitialize quadtree if configuration changed significantly
        initializePlanetQuadtree(it->second);
    }
}

IPlanetaryLODSystem::LODStats PlanetaryLODSystem::getStats(Entity planet) const {
    auto it = planets_.find(planet);
    if (it != planets_.end()) {
        return it->second.stats;
    }
    return {};
}

}  // namespace bestow::spatial
