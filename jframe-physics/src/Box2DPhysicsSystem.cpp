// jframe-physics/src/Box2DPhysicsSystem.cpp
// Box2D 3.0 physics system implementation

module;

#include <cmath>
#include <cstdint>
#include <functional>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

#include <box2d/box2d.h>

module jframe.physics.impl;

namespace jframe {

namespace {
// Helper to convert b2BodyId to uint64 key for lookup
inline std::uint64_t bodyIdToKey(b2BodyId id) {
    // b2BodyId has index1 and world0 - combine them
    return (static_cast<std::uint64_t>(id.world0) << 32) | static_cast<std::uint64_t>(id.index1);
}

// Convert jframe BodyType to Box2D body type
inline b2BodyType toBox2DBodyType(BodyType type) {
    switch (type) {
        case BodyType::Static: return b2_staticBody;
        case BodyType::Kinematic: return b2_kinematicBody;
        case BodyType::Dynamic: return b2_dynamicBody;
        default: return b2_dynamicBody;
    }
}

// Convert Box2D body type to jframe BodyType
inline BodyType fromBox2DBodyType(b2BodyType type) {
    switch (type) {
        case b2_staticBody: return BodyType::Static;
        case b2_kinematicBody: return BodyType::Kinematic;
        case b2_dynamicBody: return BodyType::Dynamic;
        default: return BodyType::Dynamic;
    }
}
}  // namespace

Box2DPhysicsSystem::Box2DPhysicsSystem() {
    worldId_ = b2_nullWorldId;
}

Box2DPhysicsSystem::~Box2DPhysicsSystem() {
    if (b2World_IsValid(worldId_)) {
        b2DestroyWorld(worldId_);
    }
}

bool Box2DPhysicsSystem::initialize() {
    if (initialized_) return true;

    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = {gravity_.x, gravity_.y};

    worldId_ = b2CreateWorld(&worldDef);
    initialized_ = b2World_IsValid(worldId_);
    return initialized_;
}

void Box2DPhysicsSystem::update(DeltaTime dt) {
    if (!initialized_) return;

    // Step the physics simulation
    b2World_Step(worldId_, dt, SUB_STEP_COUNT);

    // Process collision events
    processContactEvents();
}

void Box2DPhysicsSystem::processContactEvents() {
    if (!collisionCallback_) return;

    b2ContactEvents events = b2World_GetContactEvents(worldId_);

    // Process begin contact events
    for (int i = 0; i < events.beginCount; ++i) {
        const b2ContactBeginTouchEvent& event = events.beginEvents[i];

        // Get shape user data to find entities
        b2BodyId bodyA = b2Shape_GetBody(event.shapeIdA);
        b2BodyId bodyB = b2Shape_GetBody(event.shapeIdB);

        auto keyA = bodyIdToKey(bodyA);
        auto keyB = bodyIdToKey(bodyB);

        auto itA = bodyToMeta_.find(keyA);
        auto itB = bodyToMeta_.find(keyB);

        if (itA != bodyToMeta_.end() && itB != bodyToMeta_.end()) {
            CollisionEvent collision{
                .entityA = itA->second.entity,
                .entityB = itB->second.entity,
                .contactPoint = {0, 0},  // Could extract from manifold
                .normal = {0, 0},
                .impulse = 0.0f
            };
            collisionCallback_(collision);
        }
    }

    // Process end contact events (using hit events for collision data)
    for (int i = 0; i < events.hitCount; ++i) {
        const b2ContactHitEvent& event = events.hitEvents[i];

        b2BodyId bodyA = b2Shape_GetBody(event.shapeIdA);
        b2BodyId bodyB = b2Shape_GetBody(event.shapeIdB);

        auto keyA = bodyIdToKey(bodyA);
        auto keyB = bodyIdToKey(bodyB);

        auto itA = bodyToMeta_.find(keyA);
        auto itB = bodyToMeta_.find(keyB);

        if (itA != bodyToMeta_.end() && itB != bodyToMeta_.end()) {
            CollisionEvent collision{
                .entityA = itA->second.entity,
                .entityB = itB->second.entity,
                .contactPoint = {event.point.x * PIXELS_PER_METER, event.point.y * PIXELS_PER_METER},
                .normal = {event.normal.x, event.normal.y},
                .impulse = event.approachSpeed
            };
            collisionCallback_(collision);
        }
    }
}

void Box2DPhysicsSystem::createBody(Entity entity, const PhysicsBodyDef& def) {
    if (!initialized_) return;

    // Create body definition
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = toBox2DBodyType(def.type);
    bodyDef.position = {
        def.transform.position().x / PIXELS_PER_METER,
        def.transform.position().y / PIXELS_PER_METER
    };
    bodyDef.rotation = b2MakeRot(def.transform.rotation);
    bodyDef.linearDamping = def.linearDamping;
    bodyDef.angularDamping = def.angularDamping;
    bodyDef.fixedRotation = def.fixedRotation;

    b2BodyId bodyId = b2CreateBody(worldId_, &bodyDef);

    // Create a default box shape (32x32 pixels default size)
    constexpr float DEFAULT_SIZE = 32.0f;
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = def.density;

    // Use material for friction and restitution
    shapeDef.material.friction = def.friction;
    shapeDef.material.restitution = def.restitution;

    b2Polygon box = b2MakeBox(DEFAULT_SIZE / (2.0f * PIXELS_PER_METER), DEFAULT_SIZE / (2.0f * PIXELS_PER_METER));
    b2CreatePolygonShape(bodyId, &shapeDef, &box);

    // Store mappings
    auto entityKey = static_cast<std::uint32_t>(entity);
    entityToBody_[entityKey] = bodyId;

    auto bodyKey = bodyIdToKey(bodyId);
    bodyToMeta_[bodyKey] = BodyMeta{
        .entity = entity,
        .layer = 0x0001,
        .mask = 0xFFFF,
        .isSensor = false
    };
}

void Box2DPhysicsSystem::destroyBody(Entity entity) {
    auto entityKey = static_cast<std::uint32_t>(entity);
    auto it = entityToBody_.find(entityKey);
    if (it == entityToBody_.end()) return;

    b2BodyId bodyId = it->second;
    auto bodyKey = bodyIdToKey(bodyId);

    // Destroy the body
    b2DestroyBody(bodyId);

    // Remove mappings
    entityToBody_.erase(it);
    bodyToMeta_.erase(bodyKey);
}

bool Box2DPhysicsSystem::hasBody(Entity entity) const {
    return entityToBody_.contains(static_cast<std::uint32_t>(entity));
}

void Box2DPhysicsSystem::setBodyType(Entity entity, BodyType type) {
    auto entityKey = static_cast<std::uint32_t>(entity);
    auto it = entityToBody_.find(entityKey);
    if (it == entityToBody_.end()) return;

    b2Body_SetType(it->second, toBox2DBodyType(type));
}

BodyType Box2DPhysicsSystem::getBodyType(Entity entity) const {
    auto entityKey = static_cast<std::uint32_t>(entity);
    auto it = entityToBody_.find(entityKey);
    if (it == entityToBody_.end()) return BodyType::Static;

    return fromBox2DBodyType(b2Body_GetType(it->second));
}

void Box2DPhysicsSystem::setPosition(Entity entity, Vec2 position) {
    auto entityKey = static_cast<std::uint32_t>(entity);
    auto it = entityToBody_.find(entityKey);
    if (it == entityToBody_.end()) return;

    b2Vec2 pos = {position.x / PIXELS_PER_METER, position.y / PIXELS_PER_METER};
    b2Rot rot = b2Body_GetRotation(it->second);
    b2Body_SetTransform(it->second, pos, rot);
}

Vec2 Box2DPhysicsSystem::getPosition(Entity entity) const {
    auto entityKey = static_cast<std::uint32_t>(entity);
    auto it = entityToBody_.find(entityKey);
    if (it == entityToBody_.end()) return {0, 0};

    b2Vec2 pos = b2Body_GetPosition(it->second);
    return {pos.x * PIXELS_PER_METER, pos.y * PIXELS_PER_METER};
}

void Box2DPhysicsSystem::setRotation(Entity entity, float radians) {
    auto entityKey = static_cast<std::uint32_t>(entity);
    auto it = entityToBody_.find(entityKey);
    if (it == entityToBody_.end()) return;

    b2Vec2 pos = b2Body_GetPosition(it->second);
    b2Body_SetTransform(it->second, pos, b2MakeRot(radians));
}

float Box2DPhysicsSystem::getRotation(Entity entity) const {
    auto entityKey = static_cast<std::uint32_t>(entity);
    auto it = entityToBody_.find(entityKey);
    if (it == entityToBody_.end()) return 0.0f;

    b2Rot rot = b2Body_GetRotation(it->second);
    return b2Rot_GetAngle(rot);
}

void Box2DPhysicsSystem::setVelocity(Entity entity, Vec2 velocity) {
    auto entityKey = static_cast<std::uint32_t>(entity);
    auto it = entityToBody_.find(entityKey);
    if (it == entityToBody_.end()) return;

    b2Vec2 vel = {velocity.x / PIXELS_PER_METER, velocity.y / PIXELS_PER_METER};
    b2Body_SetLinearVelocity(it->second, vel);
}

Vec2 Box2DPhysicsSystem::getVelocity(Entity entity) const {
    auto entityKey = static_cast<std::uint32_t>(entity);
    auto it = entityToBody_.find(entityKey);
    if (it == entityToBody_.end()) return {0, 0};

    b2Vec2 vel = b2Body_GetLinearVelocity(it->second);
    return {vel.x * PIXELS_PER_METER, vel.y * PIXELS_PER_METER};
}

void Box2DPhysicsSystem::setAngularVelocity(Entity entity, float velocity) {
    auto entityKey = static_cast<std::uint32_t>(entity);
    auto it = entityToBody_.find(entityKey);
    if (it == entityToBody_.end()) return;

    b2Body_SetAngularVelocity(it->second, velocity);
}

float Box2DPhysicsSystem::getAngularVelocity(Entity entity) const {
    auto entityKey = static_cast<std::uint32_t>(entity);
    auto it = entityToBody_.find(entityKey);
    if (it == entityToBody_.end()) return 0.0f;

    return b2Body_GetAngularVelocity(it->second);
}

void Box2DPhysicsSystem::applyForce(Entity entity, Vec2 force, Vec2 point) {
    auto entityKey = static_cast<std::uint32_t>(entity);
    auto it = entityToBody_.find(entityKey);
    if (it == entityToBody_.end()) return;

    b2Vec2 f = {force.x, force.y};
    b2Vec2 p = {point.x / PIXELS_PER_METER, point.y / PIXELS_PER_METER};

    if (point.x == 0 && point.y == 0) {
        b2Body_ApplyForceToCenter(it->second, f, true);
    } else {
        b2Body_ApplyForce(it->second, f, p, true);
    }
}

void Box2DPhysicsSystem::applyImpulse(Entity entity, Vec2 impulse, Vec2 point) {
    auto entityKey = static_cast<std::uint32_t>(entity);
    auto it = entityToBody_.find(entityKey);
    if (it == entityToBody_.end()) return;

    b2Vec2 imp = {impulse.x, impulse.y};
    b2Vec2 p = {point.x / PIXELS_PER_METER, point.y / PIXELS_PER_METER};

    if (point.x == 0 && point.y == 0) {
        b2Body_ApplyLinearImpulseToCenter(it->second, imp, true);
    } else {
        b2Body_ApplyLinearImpulse(it->second, imp, p, true);
    }
}

void Box2DPhysicsSystem::applyTorque(Entity entity, float torque) {
    auto entityKey = static_cast<std::uint32_t>(entity);
    auto it = entityToBody_.find(entityKey);
    if (it == entityToBody_.end()) return;

    b2Body_ApplyTorque(it->second, torque, true);
}

void Box2DPhysicsSystem::setCollisionLayer(Entity entity, CollisionLayer layer) {
    auto entityKey = static_cast<std::uint32_t>(entity);
    auto bodyIt = entityToBody_.find(entityKey);
    if (bodyIt == entityToBody_.end()) return;

    auto bodyKey = bodyIdToKey(bodyIt->second);
    auto metaIt = bodyToMeta_.find(bodyKey);
    if (metaIt != bodyToMeta_.end()) {
        metaIt->second.layer = layer;
    }

    // Get all shapes on the body and update their filters
    constexpr int MAX_SHAPES = 16;
    b2ShapeId shapes[MAX_SHAPES];
    int shapeCount = b2Body_GetShapes(bodyIt->second, shapes, MAX_SHAPES);
    for (int i = 0; i < shapeCount; ++i) {
        b2Filter filter = b2Shape_GetFilter(shapes[i]);
        filter.categoryBits = static_cast<uint32_t>(layer);
        b2Shape_SetFilter(shapes[i], filter);
    }
}

void Box2DPhysicsSystem::setCollisionMask(Entity entity, CollisionMask mask) {
    auto entityKey = static_cast<std::uint32_t>(entity);
    auto bodyIt = entityToBody_.find(entityKey);
    if (bodyIt == entityToBody_.end()) return;

    auto bodyKey = bodyIdToKey(bodyIt->second);
    auto metaIt = bodyToMeta_.find(bodyKey);
    if (metaIt != bodyToMeta_.end()) {
        metaIt->second.mask = mask;
    }

    // Get all shapes on the body and update their filters
    constexpr int MAX_SHAPES = 16;
    b2ShapeId shapes[MAX_SHAPES];
    int shapeCount = b2Body_GetShapes(bodyIt->second, shapes, MAX_SHAPES);
    for (int i = 0; i < shapeCount; ++i) {
        b2Filter filter = b2Shape_GetFilter(shapes[i]);
        filter.maskBits = static_cast<uint32_t>(mask);
        b2Shape_SetFilter(shapes[i], filter);
    }
}

void Box2DPhysicsSystem::setSensor(Entity entity, bool isSensor) {
    auto entityKey = static_cast<std::uint32_t>(entity);
    auto bodyIt = entityToBody_.find(entityKey);
    if (bodyIt == entityToBody_.end()) return;

    auto bodyKey = bodyIdToKey(bodyIt->second);
    auto metaIt = bodyToMeta_.find(bodyKey);
    if (metaIt != bodyToMeta_.end()) {
        metaIt->second.isSensor = isSensor;
    }

    // In Box2D 3.x, sensor status can only enable/disable sensor events
    // The actual sensor flag is set at shape creation time
    constexpr int MAX_SHAPES = 16;
    b2ShapeId shapes[MAX_SHAPES];
    int shapeCount = b2Body_GetShapes(bodyIt->second, shapes, MAX_SHAPES);
    for (int i = 0; i < shapeCount; ++i) {
        b2Shape_EnableSensorEvents(shapes[i], isSensor);
    }
}

std::vector<Entity> Box2DPhysicsSystem::queryAABB(Vec2 min, Vec2 max) const {
    std::vector<Entity> results;
    if (!initialized_) return results;

    b2AABB aabb = {
        {min.x / PIXELS_PER_METER, min.y / PIXELS_PER_METER},
        {max.x / PIXELS_PER_METER, max.y / PIXELS_PER_METER}
    };

    b2QueryFilter filter = b2DefaultQueryFilter();

    // Use overlap callback
    struct Context {
        std::vector<Entity>* results;
        const std::unordered_map<std::uint64_t, BodyMeta>* bodyToMeta;
    };
    Context ctx{&results, &bodyToMeta_};

    b2World_OverlapAABB(worldId_, aabb, filter,
        [](b2ShapeId shapeId, void* context) -> bool {
            auto* ctx = static_cast<Context*>(context);
            b2BodyId bodyId = b2Shape_GetBody(shapeId);
            auto key = bodyIdToKey(bodyId);
            auto it = ctx->bodyToMeta->find(key);
            if (it != ctx->bodyToMeta->end()) {
                ctx->results->push_back(it->second.entity);
            }
            return true;  // Continue query
        }, &ctx);

    return results;
}

std::vector<Entity> Box2DPhysicsSystem::queryCircle(Vec2 center, float radius) const {
    std::vector<Entity> results;
    if (!initialized_) return results;

    // Create a circle shape proxy for the query
    b2Circle circle = {
        {center.x / PIXELS_PER_METER, center.y / PIXELS_PER_METER},
        radius / PIXELS_PER_METER
    };

    b2ShapeProxy proxy;
    proxy.points[0] = circle.center;
    proxy.count = 1;
    proxy.radius = circle.radius;

    b2QueryFilter filter = b2DefaultQueryFilter();

    struct Context {
        std::vector<Entity>* results;
        const std::unordered_map<std::uint64_t, BodyMeta>* bodyToMeta;
    };
    Context ctx{&results, &bodyToMeta_};

    b2World_OverlapShape(worldId_, &proxy, filter,
        [](b2ShapeId shapeId, void* context) -> bool {
            auto* ctx = static_cast<Context*>(context);
            b2BodyId bodyId = b2Shape_GetBody(shapeId);
            auto key = bodyIdToKey(bodyId);
            auto it = ctx->bodyToMeta->find(key);
            if (it != ctx->bodyToMeta->end()) {
                ctx->results->push_back(it->second.entity);
            }
            return true;
        }, &ctx);

    return results;
}

std::optional<RaycastHit> Box2DPhysicsSystem::raycast(Vec2 origin, Vec2 direction,
                                                       float maxDistance,
                                                       CollisionMask mask) const {
    if (!initialized_) return std::nullopt;

    // Normalize direction and scale by distance
    float len = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (len < 0.0001f) return std::nullopt;

    b2Vec2 translation = {
        (direction.x / len) * maxDistance / PIXELS_PER_METER,
        (direction.y / len) * maxDistance / PIXELS_PER_METER
    };

    b2Vec2 orig = {origin.x / PIXELS_PER_METER, origin.y / PIXELS_PER_METER};

    b2QueryFilter filter = b2DefaultQueryFilter();
    filter.maskBits = static_cast<uint32_t>(mask);

    b2RayResult result = b2World_CastRayClosest(worldId_, orig, translation, filter);

    if (result.hit) {
        b2BodyId bodyId = b2Shape_GetBody(result.shapeId);
        auto key = bodyIdToKey(bodyId);
        auto it = bodyToMeta_.find(key);

        if (it != bodyToMeta_.end()) {
            return RaycastHit{
                .entity = it->second.entity,
                .point = {result.point.x * PIXELS_PER_METER, result.point.y * PIXELS_PER_METER},
                .normal = {result.normal.x, result.normal.y},
                .distance = result.fraction * maxDistance
            };
        }
    }

    return std::nullopt;
}

std::vector<RaycastHit> Box2DPhysicsSystem::raycastAll(Vec2 origin, Vec2 direction,
                                                        float maxDistance,
                                                        CollisionMask mask) const {
    std::vector<RaycastHit> hits;
    if (!initialized_) return hits;

    float len = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (len < 0.0001f) return hits;

    b2Vec2 translation = {
        (direction.x / len) * maxDistance / PIXELS_PER_METER,
        (direction.y / len) * maxDistance / PIXELS_PER_METER
    };

    b2Vec2 orig = {origin.x / PIXELS_PER_METER, origin.y / PIXELS_PER_METER};

    b2QueryFilter filter = b2DefaultQueryFilter();
    filter.maskBits = static_cast<uint32_t>(mask);

    struct RayContextWithDist {
        std::vector<RaycastHit>* hits;
        const std::unordered_map<std::uint64_t, BodyMeta>* bodyToMeta;
        float maxDist;
    };
    RayContextWithDist ctxDist{&hits, &bodyToMeta_, maxDistance};

    b2World_CastRay(worldId_, orig, translation, filter,
        [](b2ShapeId shapeId, b2Vec2 point, b2Vec2 normal, float fraction, void* context) -> float {
            auto* ctx = static_cast<RayContextWithDist*>(context);
            b2BodyId bodyId = b2Shape_GetBody(shapeId);
            auto key = bodyIdToKey(bodyId);
            auto it = ctx->bodyToMeta->find(key);

            if (it != ctx->bodyToMeta->end()) {
                ctx->hits->push_back(RaycastHit{
                    .entity = it->second.entity,
                    .point = {point.x * 100.0f, point.y * 100.0f},  // PIXELS_PER_METER
                    .normal = {normal.x, normal.y},
                    .distance = fraction * ctx->maxDist
                });
            }
            return 1.0f;  // Continue to find all hits
        }, &ctxDist);

    return hits;
}

void Box2DPhysicsSystem::setGravity(Vec2 gravity) {
    gravity_ = gravity;
    if (initialized_) {
        b2World_SetGravity(worldId_, {gravity.x, gravity.y});
    }
}

Vec2 Box2DPhysicsSystem::getGravity() const {
    return gravity_;
}

void Box2DPhysicsSystem::setCollisionCallback(CollisionCallback callback) {
    collisionCallback_ = std::move(callback);
}

}  // namespace jframe
