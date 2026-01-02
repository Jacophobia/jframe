// bestow-physics3d/src/GravitySystem.cpp
// N-body gravity system implementation

module;

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/ext/vector_double3.hpp>
#include <spdlog/spdlog.h>
#include <cmath>
#include <numbers>

module bestow.physics3d.impl;

import std;
import bestow.services;

namespace bestow {

GravitySystem::GravitySystem(IEventSystem* events)
    : pIEventSystem_(events)
{
    spdlog::info("[GravitySystem] Initialized with Velocity-Verlet integration");
}

GravitySystem::~GravitySystem() = default;

void GravitySystem::registerBody(Entity entity, const CelestialBodyDef& def) {
    BodyData data{
        .def = def,
        .state = OrbitalState{
            .position = def.transform.position,
            .velocity = def.velocity
        }
    };
    bodies_[entity] = data;

    spdlog::debug("[GravitySystem] Registered body '{}' with mass {:.2e} kg",
                  def.name, def.mass);
}

void GravitySystem::unregisterBody(Entity entity) {
    auto it = bodies_.find(entity);
    if (it != bodies_.end()) {
        spdlog::debug("[GravitySystem] Unregistered body '{}'", it->second.def.name);
        bodies_.erase(it);
    }
}

std::optional<OrbitalState> GravitySystem::getBodyState(Entity entity) const {
    auto it = bodies_.find(entity);
    if (it != bodies_.end()) {
        return it->second.state;
    }
    return std::nullopt;
}

void GravitySystem::setBodyState(Entity entity, const OrbitalState& state) {
    auto it = bodies_.find(entity);
    if (it != bodies_.end()) {
        it->second.state = state;
        it->second.def.transform.position = state.position;
        it->second.def.velocity = state.velocity;
    }
}

Vec3d GravitySystem::calculateGravityAt(const Vec3d& position) const {
    Vec3d totalAccel{0.0, 0.0, 0.0};

    for (const auto& [entity, data] : bodies_) {
        Vec3d delta = data.state.position - position;
        double distSq = glm::dot(delta, delta);

        // Avoid singularity at very small distances
        constexpr double MIN_DIST_SQ = 1e6;  // 1 km minimum
        if (distSq < MIN_DIST_SQ) continue;

        double dist = std::sqrt(distSq);

        // a = G * M / r²
        double accelMag = OrbitalConstants::G * data.def.mass / distSq;
        totalAccel += accelMag * (delta / dist);
    }

    return totalAccel;
}

std::optional<Entity> GravitySystem::getDominantBody(const Vec3d& position) const {
    Entity dominant{};
    double maxInfluence = 0.0;
    bool found = false;

    for (const auto& [entity, data] : bodies_) {
        Vec3d delta = data.state.position - position;
        double dist = glm::length(delta);

        if (dist < 1.0) continue;  // Skip if we're at the body's center

        // Check if we're within the SOI
        double soiRadius = data.def.soiRadius;
        if (soiRadius <= 0.0) {
            // Calculate SOI if not set
            soiRadius = calculateSOI(entity);
        }

        if (dist < soiRadius) {
            // Within SOI - calculate influence (proportional to mass / distance^2)
            double influence = data.def.mass / (dist * dist);
            if (influence > maxInfluence) {
                maxInfluence = influence;
                dominant = entity;
                found = true;
            }
        }
    }

    return found ? std::make_optional(dominant) : std::nullopt;
}

double GravitySystem::calculateSOI(Entity body) const {
    auto it = bodies_.find(body);
    if (it == bodies_.end()) return 0.0;

    const auto& data = it->second;

    // Find parent body
    if (!data.def.parent.has_value()) {
        // No parent - this is the primary body (e.g., Sun)
        // SOI is effectively infinite
        return std::numeric_limits<double>::max();
    }

    Entity parentEntity = *data.def.parent;
    auto parentIt = bodies_.find(parentEntity);
    if (parentIt == bodies_.end()) {
        return std::numeric_limits<double>::max();
    }

    const auto& parentData = parentIt->second;

    // Calculate semi-major axis (approximation using current distance)
    double a = glm::length(data.state.position - parentData.state.position);

    // Hill sphere approximation: r_SOI ≈ a * (m / 3M)^(1/3)
    // More accurate SOI: r_SOI ≈ a * (m / M)^(2/5)
    double massRatio = data.def.mass / parentData.def.mass;
    double soiRadius = a * std::pow(massRatio, 2.0 / 5.0);

    return soiRadius;
}

OrbitalElements GravitySystem::stateToElements(
    const OrbitalState& state,
    double mu) const {

    OrbitalElements elements{};

    const Vec3d& r = state.position;
    const Vec3d& v = state.velocity;
    double rMag = glm::length(r);
    double vMag = glm::length(v);

    // Specific orbital energy
    double energy = (vMag * vMag) / 2.0 - mu / rMag;

    // Semi-major axis
    if (std::abs(energy) > 1e-10) {
        elements.semiMajorAxis = -mu / (2.0 * energy);
    } else {
        // Parabolic orbit
        elements.semiMajorAxis = std::numeric_limits<double>::infinity();
    }

    // Angular momentum vector
    Vec3d h = glm::cross(r, v);
    double hMag = glm::length(h);

    // Eccentricity vector
    Vec3d eVec = glm::cross(v, h) / mu - r / rMag;
    elements.eccentricity = glm::length(eVec);

    // Inclination
    elements.inclination = std::acos(std::clamp(h.z / hMag, -1.0, 1.0));

    // Node vector (points to ascending node)
    Vec3d n{-h.y, h.x, 0.0};
    double nMag = glm::length(n);

    // Longitude of ascending node (RAAN)
    if (nMag > 1e-10) {
        elements.longitudeOfAscendingNode = std::atan2(n.y, n.x);
        if (elements.longitudeOfAscendingNode < 0.0) {
            elements.longitudeOfAscendingNode += 2.0 * std::numbers::pi;
        }
    } else {
        elements.longitudeOfAscendingNode = 0.0;
    }

    // Argument of periapsis
    if (elements.eccentricity > 1e-10 && nMag > 1e-10) {
        double cosOmega = glm::dot(n, eVec) / (nMag * elements.eccentricity);
        elements.argumentOfPeriapsis = std::acos(std::clamp(cosOmega, -1.0, 1.0));
        if (eVec.z < 0.0) {
            elements.argumentOfPeriapsis = 2.0 * std::numbers::pi - elements.argumentOfPeriapsis;
        }
    } else {
        elements.argumentOfPeriapsis = 0.0;
    }

    // True anomaly
    if (elements.eccentricity > 1e-10) {
        double cosNu = glm::dot(eVec, r) / (elements.eccentricity * rMag);
        double nu = std::acos(std::clamp(cosNu, -1.0, 1.0));
        if (glm::dot(r, v) < 0.0) {
            nu = 2.0 * std::numbers::pi - nu;
        }

        // True anomaly to eccentric anomaly
        double E = 2.0 * std::atan(std::sqrt((1.0 - elements.eccentricity) /
                                              (1.0 + elements.eccentricity)) *
                                    std::tan(nu / 2.0));

        // Mean anomaly
        elements.meanAnomaly = E - elements.eccentricity * std::sin(E);
        if (elements.meanAnomaly < 0.0) {
            elements.meanAnomaly += 2.0 * std::numbers::pi;
        }
    } else {
        elements.meanAnomaly = 0.0;
    }

    return elements;
}

OrbitalState GravitySystem::elementsToState(
    const OrbitalElements& elements,
    double mu) const {

    // Solve Kepler's equation: M = E - e * sin(E)
    double M = elements.meanAnomaly;
    double e = elements.eccentricity;
    double E = M;  // Initial guess

    // Newton-Raphson iteration
    for (int i = 0; i < 10; ++i) {
        double f = E - e * std::sin(E) - M;
        double fPrime = 1.0 - e * std::cos(E);
        double dE = f / fPrime;
        E -= dE;
        if (std::abs(dE) < 1e-12) break;
    }

    // True anomaly from eccentric anomaly
    double nu = 2.0 * std::atan(std::sqrt((1.0 + e) / (1.0 - e)) * std::tan(E / 2.0));

    // Distance and velocity magnitude
    double a = elements.semiMajorAxis;
    double p = a * (1.0 - e * e);  // Semi-latus rectum
    double rMag = p / (1.0 + e * std::cos(nu));

    // Position and velocity in perifocal frame
    Vec3d rPeri{rMag * std::cos(nu), rMag * std::sin(nu), 0.0};
    double sqrtMuP = std::sqrt(mu / p);
    Vec3d vPeri{-sqrtMuP * std::sin(nu), sqrtMuP * (e + std::cos(nu)), 0.0};

    // Rotation matrix from perifocal to inertial
    double cosOmega = std::cos(elements.argumentOfPeriapsis);
    double sinOmega = std::sin(elements.argumentOfPeriapsis);
    double cosI = std::cos(elements.inclination);
    double sinI = std::sin(elements.inclination);
    double cosRAAN = std::cos(elements.longitudeOfAscendingNode);
    double sinRAAN = std::sin(elements.longitudeOfAscendingNode);

    // Build rotation matrix columns
    Vec3d r1{
        cosRAAN * cosOmega - sinRAAN * sinOmega * cosI,
        sinRAAN * cosOmega + cosRAAN * sinOmega * cosI,
        sinOmega * sinI
    };
    Vec3d r2{
        -cosRAAN * sinOmega - sinRAAN * cosOmega * cosI,
        -sinRAAN * sinOmega + cosRAAN * cosOmega * cosI,
        cosOmega * sinI
    };
    Vec3d r3{
        sinRAAN * sinI,
        -cosRAAN * sinI,
        cosI
    };

    // Transform to inertial frame
    OrbitalState state;
    state.position = r1 * rPeri.x + r2 * rPeri.y + r3 * rPeri.z;
    state.velocity = r1 * vPeri.x + r2 * vPeri.y + r3 * vPeri.z;

    return state;
}

OrbitalElements GravitySystem::propagateElements(
    const OrbitalElements& elements,
    double mu,
    double deltaTime) const {

    OrbitalElements result = elements;

    // Mean motion
    double n = std::sqrt(mu / (elements.semiMajorAxis *
                               elements.semiMajorAxis *
                               elements.semiMajorAxis));

    // Propagate mean anomaly
    result.meanAnomaly = elements.meanAnomaly + n * deltaTime;

    // Normalize to [0, 2π)
    while (result.meanAnomaly >= 2.0 * std::numbers::pi) {
        result.meanAnomaly -= 2.0 * std::numbers::pi;
    }
    while (result.meanAnomaly < 0.0) {
        result.meanAnomaly += 2.0 * std::numbers::pi;
    }

    return result;
}

std::vector<Vec3d> GravitySystem::predictTrajectory(
    Entity body,
    double duration,
    int numPoints) const {

    std::vector<Vec3d> trajectory;
    trajectory.reserve(numPoints);

    auto it = bodies_.find(body);
    if (it == bodies_.end()) return trajectory;

    const auto& data = it->second;

    // Find dominant body for this trajectory
    auto dominantOpt = getDominantBody(data.state.position);
    if (!dominantOpt.has_value()) {
        // No dominant body - return current position only
        trajectory.push_back(data.state.position);
        return trajectory;
    }

    auto dominantIt = bodies_.find(*dominantOpt);
    if (dominantIt == bodies_.end()) {
        trajectory.push_back(data.state.position);
        return trajectory;
    }

    double mu = OrbitalConstants::G * dominantIt->second.def.mass;

    // Get relative state (relative to dominant body)
    OrbitalState relativeState{
        .position = data.state.position - dominantIt->second.state.position,
        .velocity = data.state.velocity - dominantIt->second.state.velocity
    };

    // Convert to orbital elements
    OrbitalElements elements = stateToElements(relativeState, mu);

    // Sample trajectory at intervals
    double dt = duration / static_cast<double>(numPoints - 1);
    for (int i = 0; i < numPoints; ++i) {
        double t = dt * static_cast<double>(i);
        OrbitalElements propagated = propagateElements(elements, mu, t);
        OrbitalState state = elementsToState(propagated, mu);

        // Convert back to absolute position
        Vec3d absolutePos = state.position + dominantIt->second.state.position;
        trajectory.push_back(absolutePos);
    }

    return trajectory;
}

void GravitySystem::setIntegrationMethod(IntegrationMethod method) {
    integrationMethod_ = method;
    const char* methodName = "Unknown";
    switch (method) {
        case IntegrationMethod::VelocityVerlet: methodName = "Velocity-Verlet"; break;
        case IntegrationMethod::RungeKutta4: methodName = "RK4"; break;
        case IntegrationMethod::Euler: methodName = "Euler"; break;
        case IntegrationMethod::Kepler: methodName = "Kepler"; break;
    }
    spdlog::info("[GravitySystem] Integration method set to {}", methodName);
}

void GravitySystem::setTimeAcceleration(double factor) {
    timeAcceleration_ = factor;

    // Auto-switch to Kepler for high time acceleration
    if (factor > 100.0 && integrationMethod_ != IntegrationMethod::Kepler) {
        spdlog::info("[GravitySystem] High time acceleration ({:.0f}x) - switching to Kepler propagation",
                     factor);
        integrationMethod_ = IntegrationMethod::Kepler;
    }
}

double GravitySystem::getTimeAcceleration() const {
    return timeAcceleration_;
}

void GravitySystem::setPaused(bool paused) {
    paused_ = paused;
}

bool GravitySystem::isPaused() const {
    return paused_;
}

void GravitySystem::update(DeltaTime dt) {
    if (paused_) return;

    // Scale delta time by time acceleration
    double scaledDt = static_cast<double>(dt) * timeAcceleration_;

    switch (integrationMethod_) {
        case IntegrationMethod::VelocityVerlet:
            integrateVelocityVerlet(scaledDt);
            break;
        case IntegrationMethod::RungeKutta4:
            integrateRungeKutta4(scaledDt);
            break;
        case IntegrationMethod::Euler:
            // Simple Euler (not recommended for orbits)
            for (auto& [entity, data] : bodies_) {
                Vec3d accel = calculateAcceleration(data.state.position, entity);
                data.state.velocity += accel * scaledDt;
                data.state.position += data.state.velocity * scaledDt;
                data.def.transform.position = data.state.position;
                data.def.velocity = data.state.velocity;
            }
            break;
        case IntegrationMethod::Kepler:
            propagateKepler(scaledDt);
            break;
    }
}

Vec3d GravitySystem::calculateAcceleration(const Vec3d& position, Entity excludeBody) const {
    Vec3d totalAccel{0.0, 0.0, 0.0};

    for (const auto& [entity, data] : bodies_) {
        if (entity == excludeBody) continue;

        Vec3d delta = data.state.position - position;
        double distSq = glm::dot(delta, delta);

        // Softening parameter to avoid singularity
        constexpr double SOFTENING_SQ = 1e10;  // ~100 km squared
        distSq = std::max(distSq, SOFTENING_SQ);

        double dist = std::sqrt(distSq);
        double accelMag = OrbitalConstants::G * data.def.mass / distSq;
        totalAccel += accelMag * (delta / dist);
    }

    return totalAccel;
}

void GravitySystem::integrateVelocityVerlet(double dt) {
    // Velocity Verlet is symplectic - conserves energy over long periods

    // Step 1: Calculate current accelerations
    std::unordered_map<Entity, Vec3d> accelerations;
    for (const auto& [entity, data] : bodies_) {
        accelerations[entity] = calculateAcceleration(data.state.position, entity);
    }

    // Step 2: Update positions using x(t+dt) = x(t) + v(t)*dt + 0.5*a(t)*dt²
    for (auto& [entity, data] : bodies_) {
        const Vec3d& a = accelerations[entity];
        data.state.position += data.state.velocity * dt + 0.5 * a * (dt * dt);
    }

    // Step 3: Calculate new accelerations
    std::unordered_map<Entity, Vec3d> newAccelerations;
    for (const auto& [entity, data] : bodies_) {
        newAccelerations[entity] = calculateAcceleration(data.state.position, entity);
    }

    // Step 4: Update velocities using v(t+dt) = v(t) + 0.5*(a(t) + a(t+dt))*dt
    for (auto& [entity, data] : bodies_) {
        const Vec3d& aOld = accelerations[entity];
        const Vec3d& aNew = newAccelerations[entity];
        data.state.velocity += 0.5 * (aOld + aNew) * dt;

        // Sync back to def
        data.def.transform.position = data.state.position;
        data.def.velocity = data.state.velocity;
    }
}

void GravitySystem::integrateRungeKutta4(double dt) {
    // RK4 is accurate but not symplectic - energy may drift over long periods

    for (auto& [entity, data] : bodies_) {
        Vec3d x = data.state.position;
        Vec3d v = data.state.velocity;

        // k1
        Vec3d k1v = calculateAcceleration(x, entity);
        Vec3d k1x = v;

        // k2
        Vec3d k2v = calculateAcceleration(x + 0.5 * dt * k1x, entity);
        Vec3d k2x = v + 0.5 * dt * k1v;

        // k3
        Vec3d k3v = calculateAcceleration(x + 0.5 * dt * k2x, entity);
        Vec3d k3x = v + 0.5 * dt * k2v;

        // k4
        Vec3d k4v = calculateAcceleration(x + dt * k3x, entity);
        Vec3d k4x = v + dt * k3v;

        // Combine
        data.state.position = x + (dt / 6.0) * (k1x + 2.0 * k2x + 2.0 * k3x + k4x);
        data.state.velocity = v + (dt / 6.0) * (k1v + 2.0 * k2v + 2.0 * k3v + k4v);

        // Sync back to def
        data.def.transform.position = data.state.position;
        data.def.velocity = data.state.velocity;
    }
}

void GravitySystem::propagateKepler(double dt) {
    // Analytical Kepler propagation - no numerical integration error
    // Best for high time acceleration

    for (auto& [entity, data] : bodies_) {
        // Skip bodies without parents (primary body)
        if (!data.def.parent.has_value()) continue;

        auto parentIt = bodies_.find(*data.def.parent);
        if (parentIt == bodies_.end()) continue;

        double mu = OrbitalConstants::G * parentIt->second.def.mass;

        // Get relative state
        OrbitalState relativeState{
            .position = data.state.position - parentIt->second.state.position,
            .velocity = data.state.velocity - parentIt->second.state.velocity
        };

        // Convert to elements
        OrbitalElements elements = stateToElements(relativeState, mu);

        // Propagate
        OrbitalElements propagated = propagateElements(elements, mu, dt);

        // Convert back to state
        OrbitalState newRelativeState = elementsToState(propagated, mu);

        // Update absolute state
        data.state.position = newRelativeState.position + parentIt->second.state.position;
        data.state.velocity = newRelativeState.velocity + parentIt->second.state.velocity;

        // Sync back to def
        data.def.transform.position = data.state.position;
        data.def.velocity = data.state.velocity;
    }
}

}  // namespace bestow
