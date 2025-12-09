// bestow-physics-common/src/bestow.physics.common.cppm
// Shared types and constants for 2D and 3D physics systems

module;

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

export module bestow.physics.common;

import std;

export namespace bestow {

//==========================================================================
// Base Body Type Enum
//==========================================================================

/// Base body type enum shared between 2D and 3D physics
/// Both BodyType and BodyType3D use these same values for consistency
enum class BodyTypeBase : std::uint8_t {
    Static,     // Immovable object (infinite mass)
    Kinematic,  // Animated object (moved by code, affects other bodies)
    Dynamic     // Simulated object (affected by forces and collisions)
};

//==========================================================================
// Ground State Enums
//==========================================================================

/// Ground state for character controllers (2D)
enum class GroundState : std::uint8_t {
    Grounded,   // Standing on valid ground
    Falling,    // In the air
    Sliding     // On steep slope that's sliding
};

/// Ground state for character controllers (3D)
/// Uses different naming to match Jolt Physics conventions
enum class CharacterGroundState : std::uint8_t {
    OnGround,       // Standing on valid ground
    OnSteepGround,  // On slope too steep to stand on
    InAir,          // Not touching ground
    Sliding         // Sliding down a surface
};

//==========================================================================
// Collision Layer Types
//==========================================================================

/// Collision layer is a bitmask category for an object
/// Typically, an object belongs to exactly one layer (single bit set)
using CollisionLayer = std::uint16_t;

/// Collision mask defines which layers an object can collide with
/// This is a bitmask that can have multiple bits set
using CollisionMask = std::uint16_t;

//==========================================================================
// Collision Layer Constants
//==========================================================================

/// Standard collision layers used by both 2D and 3D physics
/// These can be combined with bitwise operations for custom filtering
namespace CollisionLayers {
    inline constexpr CollisionLayer All = 0xFFFF;       // Collide with everything
    inline constexpr CollisionLayer None = 0x0000;      // Collide with nothing
    inline constexpr CollisionLayer Default = 0x0001;   // Default layer
    inline constexpr CollisionLayer Player = 0x0002;    // Player character
    inline constexpr CollisionLayer Enemy = 0x0004;     // Enemy characters
    inline constexpr CollisionLayer Ground = 0x0008;    // Ground/platforms
    inline constexpr CollisionLayer Trigger = 0x0010;   // Trigger volumes
    inline constexpr CollisionLayer Projectile = 0x0020;// Bullets, etc.
    inline constexpr CollisionLayer Static = 0x0040;    // Static environment
    inline constexpr CollisionLayer Dynamic = 0x0080;   // Dynamic props
    inline constexpr CollisionLayer Character = 0x0100; // Character controllers
    inline constexpr CollisionLayer Debris = 0x0200;    // Small debris
    inline constexpr CollisionLayer Vehicle = 0x0400;   // Vehicles
}

//==========================================================================
// Collision Filter Data
//==========================================================================

/// Collision filtering data shared between 2D and 3D physics
/// This structure defines what layers an object belongs to and what it collides with
struct CollisionFilterData {
    CollisionLayer layer = CollisionLayers::Default;  // What layer this object is on
    CollisionMask mask = CollisionLayers::All;         // What layers it collides with
    bool isSensor = false;                             // Sensor = detect overlap but no collision response
};

//==========================================================================
// Ground Check Parameters
//==========================================================================

/// Parameters for ground detection raycasts
/// Used by both 2D and 3D character controllers
struct GroundCheckParams {
    float rayLength = 0.1f;                           // How far to raycast down
    float maxSlopeAngle = 45.0f;                      // Maximum walkable slope (degrees)
    CollisionMask groundMask = CollisionLayers::Ground; // What counts as ground
};

/// Result of a ground check operation (2D version)
struct GroundCheckResult2D {
    bool grounded = false;                            // Are we on the ground?
    float groundAngle = 0.0f;                         // Angle of ground surface (degrees)
    GroundState state = GroundState::Falling;         // Current ground state
    glm::vec2 contactPoint{0.0f};                     // Where we're touching
    glm::vec2 surfaceNormal{0.0f, -1.0f};            // Surface orientation
};

/// Result of a ground check operation (3D version)
struct GroundCheckResult3D {
    bool grounded = false;                            // Are we on the ground?
    float groundAngle = 0.0f;                         // Angle of ground surface (degrees)
    CharacterGroundState state = CharacterGroundState::InAir; // Current ground state
    glm::vec3 contactPoint{0.0f};                     // Where we're touching
    glm::vec3 surfaceNormal{0.0f, 1.0f, 0.0f};       // Surface orientation
};

//==========================================================================
// Physics Callback Types
//==========================================================================

/// Generic callback template for physics events
/// EventType can be CollisionEvent, TriggerEvent, etc.
template<typename EventType>
using PhysicsCallback = std::function<void(const EventType&)>;

//==========================================================================
// Physics Constants
//==========================================================================

/// Shared physics constants for scaling and simulation
namespace PhysicsConstants {
    /// Pixels per meter conversion for 2D physics
    /// 2D physics uses pixels as the unit, but internally converts to meters
    inline constexpr float PixelsPerMeter = 100.0f;

    /// Meters per pixel (inverse of above)
    inline constexpr float MetersPerPixel = 0.01f;

    /// Default number of substeps for physics simulation accuracy
    inline constexpr int DefaultSubsteps = 4;

    /// Default gravity for 2D physics (pixels/s²)
    /// Positive Y = down in screen coordinates
    inline constexpr float DefaultGravityY2D = 980.0f;

    /// Default gravity for 3D physics (m/s²)
    /// Standard earth gravity
    inline constexpr float DefaultGravityY3D = -9.81f;
}

//==========================================================================
// Common Physics Utility Functions
//==========================================================================

/// Convert pixels to meters for 2D physics
inline constexpr float pixelsToMeters(float pixels) {
    return pixels * PhysicsConstants::MetersPerPixel;
}

/// Convert meters to pixels for 2D physics
inline constexpr float metersToPixels(float meters) {
    return meters * PhysicsConstants::PixelsPerMeter;
}

/// Check if two collision layers should collide based on filter data
inline constexpr bool shouldCollide(CollisionLayer layerA, CollisionMask maskA,
                                     CollisionLayer layerB, CollisionMask maskB) {
    return (layerA & maskB) != 0 && (layerB & maskA) != 0;
}

/// Check if a slope angle is walkable
inline constexpr bool isWalkableSlope(float slopeAngleDegrees, float maxSlopeAngle) {
    return std::abs(slopeAngleDegrees) <= maxSlopeAngle;
}

}  // namespace bestow
