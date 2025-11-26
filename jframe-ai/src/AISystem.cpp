// jframe-ai/src/AISystem.cpp

module;

#include <any>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

module jframe.ai.impl;

namespace jframe {

bool AISystem::initialize() {
    return true;
}

void AISystem::update(DeltaTime dt) {
    for (auto& [entityId, ai] : aiComponents_) {
        // TODO(agent): Update behavior trees when BehaviorTree.CPP is integrated
        // - Tick the behavior tree for this entity
        // - Pass blackboard data to the tree execution context
        // - Handle tree execution results

        // TODO(agent): Update steering behaviors
        // - Calculate steering forces (seek, flee, arrive, etc.)
        // - Apply forces to entity velocity through physics system
        // - Update navigation target based on current path waypoint
        // For now, game code can use navigationTarget and maxSpeed directly
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
    // TODO(agent): Integrate Recast/Detour for navmesh loading
    // - Load navmesh data from asset system
    // - Parse navmesh geometry and tiles
    // - Initialize dtNavMesh and dtNavMeshQuery objects
    // - Store navmesh handle for future queries
    navMeshAsset_ = navMeshAsset;
    hasNavMesh_ = true;
}

void AISystem::unloadNavMesh() {
    // TODO(agent): Clean up Recast/Detour resources
    // - Release dtNavMesh and dtNavMeshQuery objects
    // - Clear cached navmesh data
    navMeshAsset_ = AssetHandle::invalid();
    hasNavMesh_ = false;
}

bool AISystem::hasNavMesh() const {
    return hasNavMesh_;
}

std::optional<NavigationPath> AISystem::findPath(const NavMeshQuery& query) const {
    if (!hasNavMesh_) return std::nullopt;

    // TODO(agent): Implement pathfinding with Recast/Detour
    // - Query dtNavMeshQuery::findPath() with start/end positions
    // - Convert Detour path to NavigationPath waypoints
    // - Calculate total path length
    // - Set isComplete based on whether path reaches the goal
    // For now, return empty path structure as placeholder
    return NavigationPath{};
}

bool AISystem::isPointOnNavMesh(Vec2 point) const {
    if (!hasNavMesh_) return false;

    // TODO(agent): Query Detour for actual point containment
    // - Use dtNavMeshQuery::findNearestPoly() with small search extents
    // - Return true if a valid polygon is found near the point
    // For now, return true if we have a navmesh (simplified check)
    return true;
}

std::optional<Vec2> AISystem::getClosestPointOnNavMesh(Vec2 point) const {
    if (!hasNavMesh_) return std::nullopt;

    // TODO(agent): Query Detour for closest point on navmesh
    // - Use dtNavMeshQuery::findNearestPoly() to get closest polygon
    // - Use dtNavMeshQuery::closestPointOnPoly() to get exact point
    // For now, return input point as placeholder (assumes point is on navmesh)
    return point;
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

std::vector<Entity> AISystem::findEntitiesInRadius(Vec2 center, float radius, CollisionMask mask) const {
    // TODO(agent): Integrate with physics system for spatial queries
    // - Use IPhysicsSystem::queryAABB() to find entities in bounding box
    // - Filter results by actual distance (circle check)
    // - Filter by collision mask
    // For now, return empty vector as placeholder
    return {};
}

std::optional<Entity> AISystem::findClosestEntity(Vec2 position, CollisionMask mask) const {
    // TODO(agent): Integrate with physics system for spatial queries
    // - Use IPhysicsSystem::queryAABB() with large search area
    // - Find entity with minimum distance to position
    // - Filter by collision mask
    // For now, return nullopt as placeholder
    return std::nullopt;
}

bool AISystem::hasLineOfSight(Vec2 from, Vec2 to, CollisionMask obstacleMask) const {
    // TODO(agent): Integrate with physics system for raycast queries
    // - Use IPhysicsSystem::raycast() from 'from' to 'to'
    // - Check if raycast hits any bodies matching obstacleMask
    // - Return false if obstacle found, true if clear path

    // WAVE 1 Implementation: Distance-based heuristic
    // Calculate distance between points
    float dx = to.x - from.x;
    float dy = to.y - from.y;
    float distance = std::sqrt(dx * dx + dy * dy);

    // Simple heuristic: line of sight exists if distance < 1000 units
    // This prevents unrealistic long-distance visibility
    return distance < 1000.0f;
}

}  // namespace jframe
