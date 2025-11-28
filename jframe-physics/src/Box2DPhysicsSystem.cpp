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
    // Convert from pixels/s² to meters/s² for Box2D
    // JFrame uses Y-down screen coordinates (positive Y = down on screen)
    // Box2D uses Y-up coordinates (negative Y = down in physics)
    // So we pass gravity directly - JFrame's positive Y gravity becomes Box2D's positive Y
    // which we then negate when converting positions back to screen space in syncTransforms()
    worldDef.gravity = {gravity_.x / PIXELS_PER_METER, gravity_.y / PIXELS_PER_METER};

    // Set hit event threshold to 0 to ensure all collisions generate events
    worldDef.hitEventThreshold = 0.0f;

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
    if (!collisionCallback_ && !triggerEnterCallback_ && !triggerExitCallback_) return;

    b2ContactEvents events = b2World_GetContactEvents(worldId_);
    b2SensorEvents sensorEvents = b2World_GetSensorEvents(worldId_);

    // Process begin contact events (for physical collisions)
    if (collisionCallback_) {
        for (int i = 0; i < events.beginCount; ++i) {
            const b2ContactBeginTouchEvent& event = events.beginEvents[i];

            // Validate shapes before accessing - they may have been destroyed
            if (!b2Shape_IsValid(event.shapeIdA) || !b2Shape_IsValid(event.shapeIdB)) {
                continue;
            }

            // Get shape user data to find entities
            b2BodyId bodyA = b2Shape_GetBody(event.shapeIdA);
            b2BodyId bodyB = b2Shape_GetBody(event.shapeIdB);

            auto keyA = bodyIdToKey(bodyA);
            auto keyB = bodyIdToKey(bodyB);

            auto itA = bodyToMeta_.find(keyA);
            auto itB = bodyToMeta_.find(keyB);

            if (itA != bodyToMeta_.end() && itB != bodyToMeta_.end()) {
                // Only report collision if neither body is a sensor
                if (!itA->second.isSensor && !itB->second.isSensor) {
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
        }

        // Process end contact events (using hit events for collision data)
        for (int i = 0; i < events.hitCount; ++i) {
            const b2ContactHitEvent& event = events.hitEvents[i];

            // Validate shapes before accessing - they may have been destroyed
            if (!b2Shape_IsValid(event.shapeIdA) || !b2Shape_IsValid(event.shapeIdB)) {
                continue;
            }

            b2BodyId bodyA = b2Shape_GetBody(event.shapeIdA);
            b2BodyId bodyB = b2Shape_GetBody(event.shapeIdB);

            auto keyA = bodyIdToKey(bodyA);
            auto keyB = bodyIdToKey(bodyB);

            auto itA = bodyToMeta_.find(keyA);
            auto itB = bodyToMeta_.find(keyB);

            if (itA != bodyToMeta_.end() && itB != bodyToMeta_.end()) {
                // Only report collision if neither body is a sensor
                if (!itA->second.isSensor && !itB->second.isSensor) {
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
    }

    // Process sensor begin events (trigger enter)
    if (triggerEnterCallback_) {
        for (int i = 0; i < sensorEvents.beginCount; ++i) {
            const b2SensorBeginTouchEvent& event = sensorEvents.beginEvents[i];

            // Validate shapes before accessing - they may have been destroyed
            if (!b2Shape_IsValid(event.sensorShapeId) || !b2Shape_IsValid(event.visitorShapeId)) {
                continue;
            }

            b2BodyId sensorBodyId = b2Shape_GetBody(event.sensorShapeId);
            b2BodyId visitorBodyId = b2Shape_GetBody(event.visitorShapeId);

            auto keySensor = bodyIdToKey(sensorBodyId);
            auto keyVisitor = bodyIdToKey(visitorBodyId);

            auto itSensor = bodyToMeta_.find(keySensor);
            auto itVisitor = bodyToMeta_.find(keyVisitor);

            if (itSensor != bodyToMeta_.end() && itVisitor != bodyToMeta_.end()) {
                TriggerEvent trigger{
                    .entityA = itSensor->second.entity,
                    .entityB = itVisitor->second.entity,
                    .contactPoint = {0, 0}  // Box2D doesn't provide contact point for sensors
                };
                triggerEnterCallback_(trigger);
            }
        }
    }

    // Process sensor end events (trigger exit)
    if (triggerExitCallback_) {
        for (int i = 0; i < sensorEvents.endCount; ++i) {
            const b2SensorEndTouchEvent& event = sensorEvents.endEvents[i];

            // Validate shapes before accessing - they may have been destroyed
            if (!b2Shape_IsValid(event.sensorShapeId) || !b2Shape_IsValid(event.visitorShapeId)) {
                continue;
            }

            b2BodyId sensorBodyId = b2Shape_GetBody(event.sensorShapeId);
            b2BodyId visitorBodyId = b2Shape_GetBody(event.visitorShapeId);

            auto keySensor = bodyIdToKey(sensorBodyId);
            auto keyVisitor = bodyIdToKey(visitorBodyId);

            auto itSensor = bodyToMeta_.find(keySensor);
            auto itVisitor = bodyToMeta_.find(keyVisitor);

            if (itSensor != bodyToMeta_.end() && itVisitor != bodyToMeta_.end()) {
                TriggerEvent trigger{
                    .entityA = itSensor->second.entity,
                    .entityB = itVisitor->second.entity,
                    .contactPoint = {0, 0}
                };
                triggerExitCallback_(trigger);
            }
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

    // Create box shape using the size from PhysicsBodyDef
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = def.density;

    // Use material for friction and restitution
    shapeDef.material.friction = def.friction;
    shapeDef.material.restitution = def.restitution;

    // Set sensor flag - sensors detect overlap without collision response
    shapeDef.isSensor = def.isSensor;

    // Enable contact events for collision callbacks (begin/end touch)
    shapeDef.enableContactEvents = true;

    // Enable hit events for collision data (contact point, normal, impulse)
    // This is REQUIRED for collision callbacks to receive detailed contact information
    shapeDef.enableHitEvents = true;

    // Enable sensor events for trigger callbacks
    // Note: In Box2D 3.0, sensor events are generated when a sensor overlaps any shape
    shapeDef.enableSensorEvents = true;

    // Convert pixel size to Box2D meters (half-extents)
    float halfWidth = def.size.x / (2.0f * PIXELS_PER_METER);
    float halfHeight = def.size.y / (2.0f * PIXELS_PER_METER);
    b2Polygon box = b2MakeBox(halfWidth, halfHeight);
    b2CreatePolygonShape(bodyId, &shapeDef, &box);

    // Store mappings
    auto entityKey = static_cast<std::uint32_t>(entity);
    entityToBody_[entityKey] = bodyId;

    auto bodyKey = bodyIdToKey(bodyId);
    bodyToMeta_[bodyKey] = BodyMeta{
        .entity = entity,
        .layer = 0x0001,
        .mask = 0xFFFF,
        .isSensor = def.isSensor,
        .size = def.size  // Store original size to avoid Box2D AABB padding
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

    // No Y negation - we use consistent Y-down coordinates (positive Y = down)
    // This matches gravity which also uses positive Y = down
    b2Vec2 vel = {velocity.x / PIXELS_PER_METER, velocity.y / PIXELS_PER_METER};
    b2Body_SetLinearVelocity(it->second, vel);
}

Vec2 Box2DPhysicsSystem::getVelocity(Entity entity) const {
    auto entityKey = static_cast<std::uint32_t>(entity);
    auto it = entityToBody_.find(entityKey);
    if (it == entityToBody_.end()) return {0, 0};

    b2Vec2 vel = b2Body_GetLinearVelocity(it->second);
    // No Y negation - we use consistent Y-down coordinates
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

Vec2 Box2DPhysicsSystem::getBodySize(Entity entity) const {
    auto entityKey = static_cast<std::uint32_t>(entity);
    auto it = entityToBody_.find(entityKey);
    if (it == entityToBody_.end()) return {0, 0};

    // Return the stored original size from BodyMeta
    // This avoids the Box2D AABB padding (skin radius) which adds ~2 pixels per side
    auto bodyKey = bodyIdToKey(it->second);
    auto metaIt = bodyToMeta_.find(bodyKey);
    if (metaIt != bodyToMeta_.end()) {
        return metaIt->second.size;
    }

    // Fallback: should not happen if createBody was called correctly
    return {0, 0};
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
        // Validate shape before accessing - it may have been destroyed
        if (!b2Shape_IsValid(result.shapeId)) {
            return std::nullopt;
        }

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
            // Validate shape before accessing - it may have been destroyed
            if (!b2Shape_IsValid(shapeId)) {
                return 1.0f;  // Skip invalid shape, continue to find more hits
            }

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
        // Convert from pixels/s² to meters/s² for Box2D
        // Since we don't flip Y for positions, we don't flip Y for gravity either
        b2World_SetGravity(worldId_, {gravity.x / PIXELS_PER_METER, gravity.y / PIXELS_PER_METER});
    }
}

Vec2 Box2DPhysicsSystem::getGravity() const {
    return gravity_;
}

void Box2DPhysicsSystem::setCollisionCallback(CollisionCallback callback) {
    collisionCallback_ = std::move(callback);
}

void Box2DPhysicsSystem::setTriggerEnterCallback(TriggerCallback callback) {
    triggerEnterCallback_ = std::move(callback);
}

void Box2DPhysicsSystem::setTriggerExitCallback(TriggerCallback callback) {
    triggerExitCallback_ = std::move(callback);
}

GroundCheckResult Box2DPhysicsSystem::checkGrounded(Entity entity,
                                                     const GroundCheckParams& params) const {
    GroundCheckResult result;
    if (!initialized_) return result;

    auto entityKey = static_cast<std::uint32_t>(entity);
    auto it = entityToBody_.find(entityKey);
    if (it == entityToBody_.end()) return result;

    b2BodyId bodyId = it->second;

    // Get body position and size
    b2Vec2 pos = b2Body_GetPosition(bodyId);

    // Get the shape to determine body size
    constexpr int MAX_SHAPES = 4;
    b2ShapeId shapes[MAX_SHAPES];
    int shapeCount = b2Body_GetShapes(bodyId, shapes, MAX_SHAPES);
    if (shapeCount == 0) return result;

    // Get the AABB of the first shape to determine body size
    b2AABB aabb = b2Shape_GetAABB(shapes[0]);
    float halfHeight = (aabb.upperBound.y - aabb.lowerBound.y) / 2.0f;

    // Raycast from body CENTER going down (positive Y in our Y-down coordinate system)
    // We use positive gravity (downward), so "down" is positive Y in Box2D too
    float rayStartY = pos.y;  // Body center
    // Ray needs to travel: halfHeight (to reach bottom) + rayDistance (to detect ground below)
    float rayLength = halfHeight + params.rayDistance / PIXELS_PER_METER;

    b2Vec2 origin = {pos.x, rayStartY};
    // Cast downward - in our coordinate system with positive gravity, down is positive Y
    b2Vec2 translation = {0.0f, rayLength};  // Positive Y = down

    // Use default filter to hit all shapes - we'll filter by layer ourselves
    // This is more reliable than relying on Box2D's categoryBits matching
    b2QueryFilter filter = b2DefaultQueryFilter();

    b2RayResult rayResult = b2World_CastRayClosest(worldId_, origin, translation, filter);

    if (rayResult.hit) {
        // Validate shape before accessing - it may have been destroyed
        if (!b2Shape_IsValid(rayResult.shapeId)) {
            return result;
        }

        // Check if hit body's layer matches ground mask
        b2BodyId hitBodyId = b2Shape_GetBody(rayResult.shapeId);
        auto hitKey = bodyIdToKey(hitBodyId);
        auto metaIt = bodyToMeta_.find(hitKey);

        if (metaIt != bodyToMeta_.end()) {
            // Check if the hit body's layer is in the ground mask
            bool layerMatches = (metaIt->second.layer & params.groundMask) != 0;

            if (layerMatches) {
                // Check slope angle - with our Y-down coordinate system:
                // - Ray goes positive Y (down)
                // - Flat floor normal points negative Y (up toward ray origin)
                // - So normalY should be ≈ -1 for flat ground
                float normalY = -rayResult.normal.y;  // Negate because up is negative Y in our coords
                float slopeAngle = std::acos(std::clamp(normalY, -1.0f, 1.0f)) * (180.0f / 3.14159265f);

                if (slopeAngle <= params.slopeToleranceDeg) {
                    result.grounded = true;
                    result.groundEntity = metaIt->second.entity;
                    result.contactPoint = {
                        rayResult.point.x * PIXELS_PER_METER,
                        rayResult.point.y * PIXELS_PER_METER
                    };
                    result.surfaceNormal = {rayResult.normal.x, rayResult.normal.y};
                    result.slopeAngle = slopeAngle;
                }
            }
        }
    }

    return result;
}

CollisionLayer Box2DPhysicsSystem::getCollisionLayer(Entity entity) const {
    auto entityKey = static_cast<std::uint32_t>(entity);
    auto bodyIt = entityToBody_.find(entityKey);
    if (bodyIt == entityToBody_.end()) return 0;

    auto bodyKey = bodyIdToKey(bodyIt->second);
    auto metaIt = bodyToMeta_.find(bodyKey);
    if (metaIt != bodyToMeta_.end()) {
        return metaIt->second.layer;
    }
    return 0;
}

}  // namespace jframe
