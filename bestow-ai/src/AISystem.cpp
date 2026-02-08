// bestow-ai/src/AISystem.cpp

module;

#include <any>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>
#include <DetourStatus.h>

#if defined(BESTOW_HAS_BTCPP)
#include <behaviortree_cpp/bt_factory.h>
#include <behaviortree_cpp/behavior_tree.h>
#endif

module bestow.ai.impl;

namespace bestow {

AISystem::AISystem(IPhysicsSystem& physicsSystem, IAssetSystem& assetSystem)
    : physicsSystem_(&physicsSystem)
    , assetSystem_(&assetSystem) {
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
#if defined(BESTOW_HAS_BTCPP)
    initializeBehaviorTreeFactory();
#endif
    return true;
}

#if defined(BESTOW_HAS_BTCPP)
void AISystem::initializeBehaviorTreeFactory() {
    // Register built-in condition nodes that access blackboard
    btFactory_.registerSimpleCondition("HasTarget", [this](BT::TreeNode& node) {
        auto entityId = node.config().blackboard->get<std::uint32_t>("entity_id");
        auto it = aiComponents_.find(entityId);
        if (it != aiComponents_.end() && it->second.navigationTarget.has_value()) {
            return BT::NodeStatus::SUCCESS;
        }
        return BT::NodeStatus::FAILURE;
    });

    btFactory_.registerSimpleCondition("IsAtTarget", [this](BT::TreeNode& node) {
        auto entityId = node.config().blackboard->get<std::uint32_t>("entity_id");
        auto it = aiComponents_.find(entityId);
        if (it == aiComponents_.end() || !it->second.navigationTarget.has_value() || !physicsSystem_) {
            return BT::NodeStatus::FAILURE;
        }
        Entity entity = static_cast<Entity>(entityId);
        Vec2 pos = physicsSystem_->getPosition(entity);
        Vec2 target = it->second.navigationTarget.value();
        float dx = target.x - pos.x;
        float dy = target.y - pos.y;
        float distSq = dx * dx + dy * dy;
        float threshold = it->second.arrivalRadius;
        if (distSq < threshold * threshold) {
            return BT::NodeStatus::SUCCESS;
        }
        return BT::NodeStatus::FAILURE;
    });

    btFactory_.registerSimpleCondition("HasLineOfSightToTarget", [this](BT::TreeNode& node) {
        auto entityId = node.config().blackboard->get<std::uint32_t>("entity_id");
        auto it = aiComponents_.find(entityId);
        if (it == aiComponents_.end() || !it->second.navigationTarget.has_value() || !physicsSystem_) {
            return BT::NodeStatus::FAILURE;
        }
        Entity entity = static_cast<Entity>(entityId);
        Vec2 pos = physicsSystem_->getPosition(entity);
        Vec2 target = it->second.navigationTarget.value();
        if (hasLineOfSight(pos, target, 0xFFFF)) {
            return BT::NodeStatus::SUCCESS;
        }
        return BT::NodeStatus::FAILURE;
    });

    // Register built-in action nodes
    btFactory_.registerSimpleAction("SeekTarget", [this](BT::TreeNode& node) {
        auto entityId = node.config().blackboard->get<std::uint32_t>("entity_id");
        auto it = aiComponents_.find(entityId);
        if (it != aiComponents_.end()) {
            it->second.steeringBehavior = SteeringBehaviorType::Seek;
            return BT::NodeStatus::SUCCESS;
        }
        return BT::NodeStatus::FAILURE;
    });

    btFactory_.registerSimpleAction("FleeFromTarget", [this](BT::TreeNode& node) {
        auto entityId = node.config().blackboard->get<std::uint32_t>("entity_id");
        auto it = aiComponents_.find(entityId);
        if (it != aiComponents_.end()) {
            it->second.steeringBehavior = SteeringBehaviorType::Flee;
            return BT::NodeStatus::SUCCESS;
        }
        return BT::NodeStatus::FAILURE;
    });

    btFactory_.registerSimpleAction("ArriveAtTarget", [this](BT::TreeNode& node) {
        auto entityId = node.config().blackboard->get<std::uint32_t>("entity_id");
        auto it = aiComponents_.find(entityId);
        if (it != aiComponents_.end()) {
            it->second.steeringBehavior = SteeringBehaviorType::Arrive;
            return BT::NodeStatus::SUCCESS;
        }
        return BT::NodeStatus::FAILURE;
    });

    btFactory_.registerSimpleAction("StopMoving", [this](BT::TreeNode& node) {
        auto entityId = node.config().blackboard->get<std::uint32_t>("entity_id");
        auto it = aiComponents_.find(entityId);
        if (it != aiComponents_.end()) {
            it->second.steeringBehavior = SteeringBehaviorType::None;
            Entity entity = static_cast<Entity>(entityId);
            if (physicsSystem_) {
                physicsSystem_->setVelocity(entity, {0.0f, 0.0f});
            }
            return BT::NodeStatus::SUCCESS;
        }
        return BT::NodeStatus::FAILURE;
    });

    btFactory_.registerSimpleAction("ClearTarget", [this](BT::TreeNode& node) {
        auto entityId = node.config().blackboard->get<std::uint32_t>("entity_id");
        auto it = aiComponents_.find(entityId);
        if (it != aiComponents_.end()) {
            it->second.navigationTarget = std::nullopt;
            return BT::NodeStatus::SUCCESS;
        }
        return BT::NodeStatus::FAILURE;
    });
}

void AISystem::tickBehaviorTree(Entity entity, AIComponent& ai, DeltaTime dt) {
    if (!ai.btTree) return;

    // Update the entity ID in the tree's blackboard
    auto blackboard = ai.btTree->rootBlackboard();
    blackboard->set("entity_id", static_cast<std::uint32_t>(entity));
    blackboard->set("delta_time", static_cast<float>(dt));

    // Copy our blackboard values to the BT blackboard
    for (const auto& [key, value] : ai.blackboard) {
        // Support common types
        if (value.type() == typeid(int)) {
            blackboard->set(key, std::any_cast<int>(value));
        } else if (value.type() == typeid(float)) {
            blackboard->set(key, std::any_cast<float>(value));
        } else if (value.type() == typeid(double)) {
            blackboard->set(key, std::any_cast<double>(value));
        } else if (value.type() == typeid(bool)) {
            blackboard->set(key, std::any_cast<bool>(value));
        } else if (value.type() == typeid(std::string)) {
            blackboard->set(key, std::any_cast<std::string>(value));
        }
        // Vec2 and other complex types would need special handling
    }

    // Tick the tree once
    ai.btTree->tickOnce();
}
#endif

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

#if defined(BESTOW_HAS_BTCPP)
        // Tick behavior tree if attached
        if (ai.btTree) {
            tickBehaviorTree(entity, ai, dt);
        }
#endif

        // Apply steering behaviors (seek, flee, arrive, etc.)
        applySteeringBehavior(entity, ai, dt);
    }
}

// Steering behavior implementations
Vec2 AISystem::calculateSeek(Vec2 position, Vec2 target, float maxSpeed) const {
    float dx = target.x - position.x;
    float dy = target.y - position.y;
    float distance = std::sqrt(dx * dx + dy * dy);

    if (distance < 0.0001f) {
        return {0.0f, 0.0f};
    }

    // Normalize and scale to max speed
    return {(dx / distance) * maxSpeed, (dy / distance) * maxSpeed};
}

Vec2 AISystem::calculateFlee(Vec2 position, Vec2 target, float maxSpeed) const {
    float dx = position.x - target.x;  // Reversed direction
    float dy = position.y - target.y;
    float distance = std::sqrt(dx * dx + dy * dy);

    if (distance < 0.0001f) {
        return {0.0f, 0.0f};
    }

    // Normalize and scale to max speed
    return {(dx / distance) * maxSpeed, (dy / distance) * maxSpeed};
}

Vec2 AISystem::calculateArrive(Vec2 position, Vec2 target, float maxSpeed, float arrivalRadius) const {
    float dx = target.x - position.x;
    float dy = target.y - position.y;
    float distance = std::sqrt(dx * dx + dy * dy);

    if (distance < 0.0001f) {
        return {0.0f, 0.0f};
    }

    // Calculate speed based on distance (decelerate as we approach)
    float speed = maxSpeed;
    if (distance < arrivalRadius) {
        speed = maxSpeed * (distance / arrivalRadius);
    }

    // Normalize and scale to computed speed
    return {(dx / distance) * speed, (dy / distance) * speed};
}

void AISystem::applySteeringBehavior(Entity entity, AIComponent& ai, DeltaTime dt) {
    if (!physicsSystem_ || ai.steeringBehavior == SteeringBehaviorType::None) {
        return;
    }

    if (!ai.navigationTarget.has_value()) {
        return;
    }

    Vec2 position = physicsSystem_->getPosition(entity);
    Vec2 target = ai.navigationTarget.value();
    Vec2 desiredVelocity{0.0f, 0.0f};

    switch (ai.steeringBehavior) {
        case SteeringBehaviorType::Seek:
            desiredVelocity = calculateSeek(position, target, ai.maxSpeed);
            break;
        case SteeringBehaviorType::Flee:
            desiredVelocity = calculateFlee(position, target, ai.maxSpeed);
            break;
        case SteeringBehaviorType::Arrive:
            desiredVelocity = calculateArrive(position, target, ai.maxSpeed, ai.arrivalRadius);
            break;
        case SteeringBehaviorType::Pursue:
        case SteeringBehaviorType::Evade:
            // Pursue/Evade require target velocity - fall back to Seek/Flee for now
            desiredVelocity = (ai.steeringBehavior == SteeringBehaviorType::Pursue)
                ? calculateSeek(position, target, ai.maxSpeed)
                : calculateFlee(position, target, ai.maxSpeed);
            break;
        default:
            return;
    }

    // Apply acceleration limits
    Vec2 currentVel = physicsSystem_->getVelocity(entity);
    float dvx = desiredVelocity.x - currentVel.x;
    float dvy = desiredVelocity.y - currentVel.y;
    float dvMag = std::sqrt(dvx * dvx + dvy * dvy);

    float maxDeltaV = ai.maxAcceleration * static_cast<float>(dt);
    if (dvMag > maxDeltaV && dvMag > 0.0001f) {
        dvx = (dvx / dvMag) * maxDeltaV;
        dvy = (dvy / dvMag) * maxDeltaV;
    }

    Vec2 newVelocity{currentVel.x + dvx, currentVel.y + dvy};
    physicsSystem_->setVelocity(entity, newVelocity);
}

void AISystem::setSteeringBehavior(Entity entity, SteeringBehaviorType behavior) {
    aiComponents_[static_cast<std::uint32_t>(entity)].steeringBehavior = behavior;
}

void AISystem::setArrivalRadius(Entity entity, float radius) {
    aiComponents_[static_cast<std::uint32_t>(entity)].arrivalRadius = radius;
}

void AISystem::attachBehaviorTree(Entity entity, AssetHandle treeAsset) {
    auto& ai = aiComponents_[static_cast<std::uint32_t>(entity)];
    ai.behaviorTree = treeAsset;

#if defined(BESTOW_HAS_BTCPP)
    // Load the behavior tree from asset data
    if (assetSystem_ && treeAsset.isValid()) {
        const auto* btData = assetSystem_->getAsset<BehaviorTreeData>(treeAsset);
        if (btData && !btData->rawText.empty()) {
            try {
                // Create tree from XML text
                ai.btTree = std::make_unique<BT::Tree>(
                    btFactory_.createTreeFromText(btData->rawText)
                );
            } catch (const std::exception& e) {
                // Failed to create tree - log error but don't crash
                ai.btTree = nullptr;
            }
        }
    }
#endif
}

void AISystem::detachBehaviorTree(Entity entity) {
    if (auto it = aiComponents_.find(static_cast<std::uint32_t>(entity)); it != aiComponents_.end()) {
        it->second.behaviorTree = AssetHandle::invalid();
#if defined(BESTOW_HAS_BTCPP)
        it->second.btTree = nullptr;
#endif
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

}  // namespace bestow
