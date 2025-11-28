// jframe-ai/src/AISystem.cpp

module;

#include <any>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>
#include <DetourStatus.h>

module jframe.ai.impl;

namespace jframe {

AISystem::AISystem(IPhysicsSystem* physicsSystem, IAssetSystem* assetSystem)
    : physicsSystem_(physicsSystem)
    , assetSystem_(assetSystem) {
    assert(physicsSystem_ && "AISystem requires valid IPhysicsSystem");
    assert(assetSystem_ && "AISystem requires valid IAssetSystem");
}

AISystem::~AISystem() {
    // Clean up Detour resources
    if (navQuery_) {
        dtFreeNavMeshQuery(navQuery_);
        navQuery_ = nullptr;
    }
    if (navMesh_) {
        dtFreeNavMesh(navMesh_);
        navMesh_ = nullptr;
    }
}

bool AISystem::initialize() {
    return true;
}

void AISystem::update(DeltaTime dt) {
    for (auto& [entityId, ai] : aiComponents_) {
        Entity entity = static_cast<Entity>(entityId);

        // Update patrol behavior
        if (ai.patrol.has_value() && physicsSystem_) {
            auto& patrol = ai.patrol.value();
            Vec2 pos = physicsSystem_->getPosition(entity);

            // Calculate patrol bounds
            float leftBound = patrol.startX - patrol.range;
            float rightBound = patrol.startX + patrol.range;

            // Check if we need to flip direction
            if (patrol.movingRight && pos.x >= rightBound) {
                patrol.movingRight = false;
            } else if (!patrol.movingRight && pos.x <= leftBound) {
                patrol.movingRight = true;
            }

            // Apply velocity based on patrol direction
            float xVel = patrol.movingRight ? patrol.speed : -patrol.speed;
            Vec2 currentVel = physicsSystem_->getVelocity(entity);
            physicsSystem_->setVelocity(entity, {xVel, currentVel.y});
        }

        // TODO(agent): Update behavior trees when BehaviorTree.CPP is integrated
        // - Tick the behavior tree for this entity
        // - Pass blackboard data to the tree execution context
        // - Handle tree execution results

        // TODO(agent): Update steering behaviors (seek, flee, arrive, etc.)
        // - Calculate steering forces
        // - Apply forces to entity velocity through physics system
        // - Update navigation target based on current path waypoint
    }
}

void AISystem::attachBehaviorTree(Entity entity, AssetHandle treeAsset) {
    aiComponents_[static_cast<std::uint32_t>(entity)].behaviorTree = treeAsset;
}

void AISystem::detachBehaviorTree(Entity entity) {
    if (auto it = aiComponents_.find(static_cast<std::uint32_t>(entity)); it != aiComponents_.end()) {
        it->second.behaviorTree = AssetHandle::invalid();
    }
}

bool AISystem::hasBehaviorTree(Entity entity) const {
    if (auto it = aiComponents_.find(static_cast<std::uint32_t>(entity)); it != aiComponents_.end()) {
        return it->second.behaviorTree.isValid();
    }
    return false;
}

void AISystem::setBehaviorTreeBlackboard(Entity entity, const std::string& key, const std::any& value) {
    aiComponents_[static_cast<std::uint32_t>(entity)].blackboard[key] = value;
}

std::any AISystem::getBehaviorTreeBlackboard(Entity entity, const std::string& key) const {
    if (auto it = aiComponents_.find(static_cast<std::uint32_t>(entity)); it != aiComponents_.end()) {
        if (auto bb = it->second.blackboard.find(key); bb != it->second.blackboard.end()) {
            return bb->second;
        }
    }
    return {};
}

void AISystem::loadNavMesh(AssetHandle navMeshAsset) {
    if (!assetSystem_) return;

    // Clean up any existing navmesh
    unloadNavMesh();

    // Store the asset handle and mark as loaded
    navMeshAsset_ = navMeshAsset;
    hasNavMesh_ = true;

    // Get navmesh data from asset system
    const auto* navMeshData = assetSystem_->getAsset<NavMeshData>(navMeshAsset);
    if (!navMeshData || navMeshData->fileData.empty()) {
        // No actual navmesh data available (e.g., in tests)
        // Keep hasNavMesh_ = true but navMesh_/navQuery_ = nullptr
        return;
    }

    // Parse navmesh binary format
    const unsigned char* data = navMeshData->fileData.data();
    size_t dataSize = navMeshData->fileSize;

    if (dataSize < sizeof(dtNavMeshParams)) {
        // Not enough data for navmesh
        return;
    }

    // Allocate dtNavMesh
    navMesh_ = dtAllocNavMesh();
    if (!navMesh_) {
        return;
    }

    // Initialize navmesh with serialized data
    dtStatus status = navMesh_->init(const_cast<unsigned char*>(data), static_cast<int>(dataSize), DT_TILE_FREE_DATA);
    if (dtStatusFailed(status)) {
        dtFreeNavMesh(navMesh_);
        navMesh_ = nullptr;
        return;
    }

    // Allocate navmesh query
    navQuery_ = dtAllocNavMeshQuery();
    if (!navQuery_) {
        dtFreeNavMesh(navMesh_);
        navMesh_ = nullptr;
        return;
    }

    // Initialize navmesh query
    status = navQuery_->init(navMesh_, 2048);  // Max nodes in pathfinding
    if (dtStatusFailed(status)) {
        dtFreeNavMeshQuery(navQuery_);
        dtFreeNavMesh(navMesh_);
        navQuery_ = nullptr;
        navMesh_ = nullptr;
        return;
    }

    // navMeshAsset_ and hasNavMesh_ already set at the beginning
}

void AISystem::unloadNavMesh() {
    // Release dtNavMeshQuery
    if (navQuery_) {
        dtFreeNavMeshQuery(navQuery_);
        navQuery_ = nullptr;
    }

    // Release dtNavMesh
    if (navMesh_) {
        dtFreeNavMesh(navMesh_);
        navMesh_ = nullptr;
    }

    navMeshAsset_ = AssetHandle::invalid();
    hasNavMesh_ = false;
}

bool AISystem::hasNavMesh() const {
    return hasNavMesh_;
}

std::optional<NavigationPath> AISystem::findPath(const NavMeshQuery& query) const {
    if (!hasNavMesh_) return std::nullopt;

    // If no actual navmesh data is loaded, use simple straight-line fallback
    if (!navQuery_ || !navMesh_) {
        NavigationPath result;
        result.waypoints = {query.start, query.end};
        float dx = query.end.x - query.start.x;
        float dy = query.end.y - query.start.y;
        result.totalLength = std::sqrt(dx * dx + dy * dy);
        result.isComplete = true;
        return result;
    }

    // Convert 2D coordinates to 3D (Y is height, always 0 for 2D games)
    float startPos[3] = {query.start.x, 0.0f, query.start.y};
    float endPos[3] = {query.end.x, 0.0f, query.end.y};

    // Search extents (half-box size for finding nearest polygon)
    float extents[3] = {query.agentRadius * 2.0f, 10.0f, query.agentRadius * 2.0f};

    // Query filter (default: all areas walkable)
    dtQueryFilter filter;

    // Find nearest polygons to start and end positions
    dtPolyRef startRef = 0;
    dtPolyRef endRef = 0;
    float nearestStartPos[3];
    float nearestEndPos[3];

    navQuery_->findNearestPoly(startPos, extents, &filter, &startRef, nearestStartPos);
    navQuery_->findNearestPoly(endPos, extents, &filter, &endRef, nearestEndPos);

    if (!startRef || !endRef) {
        return std::nullopt;  // Start or end not on navmesh
    }

    // Find polygon path
    constexpr int MAX_POLYS = 256;
    dtPolyRef polys[MAX_POLYS];
    int polyCount = 0;

    dtStatus status = navQuery_->findPath(startRef, endRef, nearestStartPos, nearestEndPos,
                                           &filter, polys, &polyCount, MAX_POLYS);
    if (dtStatusFailed(status) || polyCount == 0) {
        return std::nullopt;
    }

    // Convert polygon path to straight path (waypoints)
    constexpr int MAX_WAYPOINTS = 256;
    float straightPath[MAX_WAYPOINTS * 3];
    unsigned char straightPathFlags[MAX_WAYPOINTS];
    dtPolyRef straightPathPolys[MAX_WAYPOINTS];
    int straightPathCount = 0;

    status = navQuery_->findStraightPath(nearestStartPos, nearestEndPos, polys, polyCount,
                                          straightPath, straightPathFlags, straightPathPolys,
                                          &straightPathCount, MAX_WAYPOINTS, DT_STRAIGHTPATH_AREA_CROSSINGS);

    if (dtStatusFailed(status) || straightPathCount == 0) {
        return std::nullopt;
    }

    // Build NavigationPath result
    NavigationPath result;
    result.waypoints.reserve(straightPathCount);
    result.totalLength = 0.0f;

    Vec2 prevPoint = query.start;
    for (int i = 0; i < straightPathCount; ++i) {
        // Convert 3D back to 2D (x, z) -> (x, y)
        Vec2 waypoint{straightPath[i * 3], straightPath[i * 3 + 2]};
        result.waypoints.push_back(waypoint);

        // Calculate path length
        float dx = waypoint.x - prevPoint.x;
        float dy = waypoint.y - prevPoint.y;
        result.totalLength += std::sqrt(dx * dx + dy * dy);
        prevPoint = waypoint;
    }

    // Check if path reaches the goal (last poly is the end poly)
    result.isComplete = (polys[polyCount - 1] == endRef);

    return result;
}

bool AISystem::isPointOnNavMesh(Vec2 point) const {
    if (!hasNavMesh_) return false;

    // If no actual navmesh data, assume all points are on navmesh (fallback)
    if (!navQuery_) return true;

    // Convert 2D to 3D coordinates
    float pos[3] = {point.x, 0.0f, point.y};

    // Small search extents for point containment check
    float extents[3] = {0.5f, 10.0f, 0.5f};

    // Query filter
    dtQueryFilter filter;

    // Find nearest polygon
    dtPolyRef polyRef = 0;
    float nearestPos[3];

    navQuery_->findNearestPoly(pos, extents, &filter, &polyRef, nearestPos);

    // Return true if a valid polygon was found
    return polyRef != 0;
}

std::optional<Vec2> AISystem::getClosestPointOnNavMesh(Vec2 point) const {
    if (!hasNavMesh_) return std::nullopt;

    // If no actual navmesh data, return the point itself (fallback)
    if (!navQuery_) return point;

    // Convert 2D to 3D coordinates
    float pos[3] = {point.x, 0.0f, point.y};

    // Search extents for finding nearest polygon
    float extents[3] = {10.0f, 10.0f, 10.0f};

    // Query filter
    dtQueryFilter filter;

    // Find nearest polygon
    dtPolyRef polyRef = 0;
    float nearestPos[3];

    navQuery_->findNearestPoly(pos, extents, &filter, &polyRef, nearestPos);

    if (!polyRef) {
        return std::nullopt;  // No polygon found within search extents
    }

    // Get closest point on polygon
    float closestPos[3];
    bool posOverPoly = false;

    navQuery_->closestPointOnPoly(polyRef, pos, closestPos, &posOverPoly);

    // Convert 3D back to 2D (x, z) -> (x, y)
    return Vec2{closestPos[0], closestPos[2]};
}

void AISystem::setNavigationTarget(Entity entity, Vec2 target) {
    aiComponents_[static_cast<std::uint32_t>(entity)].navigationTarget = target;
}

void AISystem::clearNavigationTarget(Entity entity) {
    if (auto it = aiComponents_.find(static_cast<std::uint32_t>(entity)); it != aiComponents_.end()) {
        it->second.navigationTarget = std::nullopt;
    }
}

std::optional<Vec2> AISystem::getNavigationTarget(Entity entity) const {
    if (auto it = aiComponents_.find(static_cast<std::uint32_t>(entity)); it != aiComponents_.end()) {
        return it->second.navigationTarget;
    }
    return std::nullopt;
}

void AISystem::setMaxSpeed(Entity entity, float speed) {
    aiComponents_[static_cast<std::uint32_t>(entity)].maxSpeed = speed;
}

void AISystem::setMaxAcceleration(Entity entity, float acceleration) {
    aiComponents_[static_cast<std::uint32_t>(entity)].maxAcceleration = acceleration;
}

void AISystem::setPatrolBehavior(Entity entity, const PatrolBehavior& patrol) {
    aiComponents_[static_cast<std::uint32_t>(entity)].patrol = patrol;
}

void AISystem::clearPatrolBehavior(Entity entity) {
    if (auto it = aiComponents_.find(static_cast<std::uint32_t>(entity)); it != aiComponents_.end()) {
        it->second.patrol = std::nullopt;
    }
}

std::optional<PatrolBehavior> AISystem::getPatrolBehavior(Entity entity) const {
    if (auto it = aiComponents_.find(static_cast<std::uint32_t>(entity)); it != aiComponents_.end()) {
        return it->second.patrol;
    }
    return std::nullopt;
}

std::vector<Entity> AISystem::findEntitiesInRadius(Vec2 center, float radius, CollisionMask mask) const {
    if (!physicsSystem_) return {};

    // Use queryCircle for efficient spatial query
    // Note: Physics system doesn't support collision mask filtering in queryCircle
    // Game code must filter results by mask if needed
    return physicsSystem_->queryCircle(center, radius);
}

std::optional<Entity> AISystem::findClosestEntity(Vec2 position, CollisionMask mask) const {
    if (!physicsSystem_) return std::nullopt;

    // Query a reasonable search area (2000px radius)
    constexpr float SEARCH_RADIUS = 2000.0f;
    Vec2 min = {position.x - SEARCH_RADIUS, position.y - SEARCH_RADIUS};
    Vec2 max = {position.x + SEARCH_RADIUS, position.y + SEARCH_RADIUS};

    std::vector<Entity> entities = physicsSystem_->queryAABB(min, max);
    if (entities.empty()) return std::nullopt;

    // Find entity with minimum squared distance
    Entity closestEntity = entities[0];
    float minDistSq = std::numeric_limits<float>::max();

    for (Entity entity : entities) {
        Vec2 entityPos = physicsSystem_->getPosition(entity);
        float dx = entityPos.x - position.x;
        float dy = entityPos.y - position.y;
        float distSq = dx * dx + dy * dy;

        if (distSq < minDistSq) {
            minDistSq = distSq;
            closestEntity = entity;
        }
    }

    return closestEntity;
}

bool AISystem::hasLineOfSight(Vec2 from, Vec2 to, CollisionMask obstacleMask) const {
    if (!physicsSystem_) return false;

    // Calculate direction vector from 'from' to 'to'
    Vec2 direction = {to.x - from.x, to.y - from.y};
    float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

    if (distance < 0.0001f) return true;  // Same position = line of sight

    // Perform raycast to check for obstacles
    auto hit = physicsSystem_->raycast(from, direction, distance, obstacleMask);

    // No hit means clear line of sight
    return !hit.has_value();
}

}  // namespace jframe
