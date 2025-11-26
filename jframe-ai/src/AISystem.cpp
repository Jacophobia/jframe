// jframe-ai/src/AISystem.cpp

module;

#include <any>
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
        // Update behavior trees
        // Update steering behaviors
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
    hasNavMesh_ = true;
}

void AISystem::unloadNavMesh() {
    hasNavMesh_ = false;
}

bool AISystem::hasNavMesh() const {
    return hasNavMesh_;
}

std::optional<NavigationPath> AISystem::findPath(const NavMeshQuery& query) const {
    if (!hasNavMesh_) return std::nullopt;
    // Use Recast/Detour to find path
    return NavigationPath{};
}

bool AISystem::isPointOnNavMesh(Vec2 point) const {
    return hasNavMesh_;
}

std::optional<Vec2> AISystem::getClosestPointOnNavMesh(Vec2 point) const {
    if (!hasNavMesh_) return std::nullopt;
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
    return {};
}

std::optional<Entity> AISystem::findClosestEntity(Vec2 position, CollisionMask mask) const {
    return std::nullopt;
}

bool AISystem::hasLineOfSight(Vec2 from, Vec2 to, CollisionMask obstacleMask) const {
    return true;
}

}  // namespace jframe
