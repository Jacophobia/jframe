// jframe-contract/src/jframe.types.cppm
// Core types and aliases for JFrame

module;

// MSVC C++23 module compatibility - use full EnTT header
#include <jframe/entt_compat.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

export module jframe.types;

import std;

export namespace jframe {

//==========================================================================
// Fundamental Types
//==========================================================================

using UUID = std::uint64_t;
using DeltaTime = float;
using Timestamp = float;
using Entity = entt::entity;

template<typename T, typename E = std::error_code>
using Result = std::expected<T, E>;

//==========================================================================
// Math Types
//==========================================================================

using Vec2 = glm::vec2;
using Vec3 = glm::vec3;

struct Transform2D {
    float x = 0.0f;
    float y = 0.0f;
    float rotation = 0.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;

    Vec2 position() const { return {x, y}; }
    Vec2 scale() const { return {scaleX, scaleY}; }
};

//==========================================================================
// Graphics Types
//==========================================================================

struct Coordinate {
    int x = 0;
    int y = 0;
};

struct Size {
    int width = 0;
    int height = 0;
};

struct Canvas {
    Coordinate origin;
    Size size;
};

struct Color {
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
    std::uint8_t a = 255;

    static constexpr Color white() { return {255, 255, 255, 255}; }
    static constexpr Color black() { return {0, 0, 0, 255}; }
    static constexpr Color red() { return {255, 0, 0, 255}; }
    static constexpr Color green() { return {0, 255, 0, 255}; }
    static constexpr Color blue() { return {0, 0, 255, 255}; }
    static constexpr Color transparent() { return {0, 0, 0, 0}; }
};

using RenderLayer = std::int32_t;

// Forward declare AssetHandle for use in Sprite
struct AssetHandle;

struct Sprite {
    AssetHandle* textureHandle = nullptr;
    Canvas sourceRect;
    Transform2D transform;
    Color tint = Color::white();
    RenderLayer layer = 0;
    Vec2 anchor = {0.5f, 0.5f};
};

struct Camera {
    Transform2D transform;
    float zoom = 1.0f;
    Size viewportSize;
};

//==========================================================================
// Render Layer Presets
//==========================================================================

namespace RenderLayers {
    inline constexpr RenderLayer Background = -100;
    inline constexpr RenderLayer BackgroundDecor = -50;
    inline constexpr RenderLayer Platforms = 0;
    inline constexpr RenderLayer Items = 10;
    inline constexpr RenderLayer Enemies = 20;
    inline constexpr RenderLayer Player = 30;
    inline constexpr RenderLayer Effects = 40;
    inline constexpr RenderLayer Foreground = 50;
    inline constexpr RenderLayer UI = 100;
    inline constexpr RenderLayer Debug = 1000;
}

//==========================================================================
// Debug Visual Components (for entities without textures)
//==========================================================================

/// Debug rectangle component for prototyping and visualization
/// Automatically rendered by graphics system when attached to an entity with Transform2D
struct DebugRect {
    Vec2 size{32.0f, 32.0f};             // Width and height in pixels
    Color fillColor{128, 128, 128, 255}; // Interior color
    Color outlineColor{0, 0, 0, 0};      // Border color (transparent = no outline)
    float outlineWidth{0.0f};            // Border thickness (0 = no outline)
    RenderLayer layer{0};                // Render order
    bool filled{true};                   // Fill interior or outline only
};

/// Debug circle component for prototyping and visualization
struct DebugCircle {
    float radius{16.0f};                 // Radius in pixels
    Color fillColor{128, 128, 128, 255}; // Interior color
    Color outlineColor{0, 0, 0, 0};      // Border color
    float outlineWidth{0.0f};            // Border thickness
    RenderLayer layer{0};                // Render order
    bool filled{true};                   // Fill interior or outline only
    int segments{32};                    // Circle smoothness
};

/// Debug line component for visualization
struct DebugLine {
    Vec2 endOffset{32.0f, 0.0f};         // End point relative to transform
    Color color{255, 255, 255, 255};     // Line color
    float thickness{1.0f};               // Line thickness
    RenderLayer layer{0};                // Render order
};

//==========================================================================
// Asset Types
//==========================================================================

enum class AssetType : std::uint8_t {
    Texture,
    Sound,
    Music,
    Font,
    Level,
    Data,
    Shader,
    NavMesh,
    BehaviorTree
};

enum class AssetState : std::uint8_t {
    Unloaded,
    Loading,
    Loaded,
    Failed
};

struct AssetHandle {
    UUID uuid = 0;
    AssetType type = AssetType::Data;

    bool operator==(const AssetHandle&) const = default;
    auto operator<=>(const AssetHandle&) const = default;
    bool isValid() const { return uuid != 0; }

    static AssetHandle invalid() { return {}; }
};

struct AssetHandleHash {
    std::size_t operator()(const AssetHandle& h) const noexcept {
        return std::hash<UUID>{}(h.uuid);
    }
};

//==========================================================================
// Sprite Sheet & Animation Types
//==========================================================================

struct SpriteSheet {
    AssetHandle texture;
    int frameWidth = 32;
    int frameHeight = 32;
    int columns = 1;
    int rows = 1;
    int padding = 0;  // Pixels between frames

    Canvas getFrameRect(int frameIndex) const {
        int col = frameIndex % columns;
        int row = frameIndex / columns;
        return Canvas{
            .origin = {col * (frameWidth + padding), row * (frameHeight + padding)},
            .size = {frameWidth, frameHeight}
        };
    }
};

struct AnimationFrame {
    int frameIndex;
    float duration;  // Seconds this frame is shown
};

struct Animation {
    std::string name;
    std::vector<AnimationFrame> frames;
    bool looping = true;
};

struct AnimatedSprite {
    SpriteSheet sheet;
    std::unordered_map<std::string, Animation> animations;
    std::string currentAnimation;
    int currentFrameIndex = 0;
    float frameTimer = 0.0f;
    bool playing = true;

    void play(const std::string& animName) {
        if (currentAnimation != animName) {
            currentAnimation = animName;
            currentFrameIndex = 0;
            frameTimer = 0.0f;
        }
    }

    void update(float dt) {
        if (!playing || currentAnimation.empty()) return;

        auto it = animations.find(currentAnimation);
        if (it == animations.end() || it->second.frames.empty()) return;

        const Animation& anim = it->second;
        frameTimer += dt;

        const AnimationFrame& frame = anim.frames[currentFrameIndex];
        if (frameTimer >= frame.duration) {
            frameTimer -= frame.duration;
            currentFrameIndex++;

            if (currentFrameIndex >= static_cast<int>(anim.frames.size())) {
                if (anim.looping) {
                    currentFrameIndex = 0;
                } else {
                    currentFrameIndex = static_cast<int>(anim.frames.size()) - 1;
                    playing = false;
                }
            }
        }
    }

    int getCurrentFrame() const {
        if (currentAnimation.empty()) return 0;

        auto it = animations.find(currentAnimation);
        if (it == animations.end() || it->second.frames.empty()) return 0;

        return it->second.frames[currentFrameIndex].frameIndex;
    }
};

//==========================================================================
// Audio Types
//==========================================================================

using Channel = std::uint32_t;
using Volume = float;
using SoundHandle = std::uint64_t;

struct ChannelSound {
    AssetHandle asset;
    Volume volume = 1.0f;
    float pitch = 1.0f;
    bool looping = false;
    float fadeInTime = 0.0f;
    std::optional<float> startTime;
};

struct PositionalSound {
    AssetHandle asset;
    Vec3 position{0.0f};
    Volume volume = 1.0f;
    float pitch = 1.0f;
    float minDistance = 1.0f;
    float maxDistance = 100.0f;
    std::optional<Vec3> velocity;
    std::function<void()> onComplete;
};

struct AudioListener {
    Vec3 position{0.0f};
    Vec3 forward{0.0f, 0.0f, -1.0f};
    Vec3 up{0.0f, 1.0f, 0.0f};
    Vec3 velocity{0.0f};
};

namespace Channels {
    inline constexpr Channel Music = 0;
    inline constexpr Channel Ambience = 1;
    inline constexpr Channel UI = 2;
    inline constexpr Channel Voice = 3;
}

//==========================================================================
// Input Types
//==========================================================================

enum class InputDeviceType : std::uint8_t {
    Keyboard,
    Mouse,
    Controller
};

struct InputBinding {
    InputDeviceType deviceType = InputDeviceType::Keyboard;
    int deviceIndex = 0;
    int keyCode = 0;
    float scale = 1.0f;
    float deadzone = 0.1f;
};

using Action = std::string;

struct ActionState {
    Action action;
    bool active = false;
    float value = 0.0f;
    bool justPressed = false;
    bool justReleased = false;
};

struct InputMapping {
    InputBinding binding;
    Action action;
};

//==========================================================================
// Physics Types
//==========================================================================

/// Direction constants for screen coordinate system
/// In JFrame, the Y-axis points downward (positive Y = down, negative Y = up)
/// This matches typical screen/window coordinates
namespace Direction {
    /// Upward on screen (negative Y)
    inline constexpr Vec2 Up{0.0f, -1.0f};
    /// Downward on screen (positive Y)
    inline constexpr Vec2 Down{0.0f, 1.0f};
    /// Leftward on screen (negative X)
    inline constexpr Vec2 Left{-1.0f, 0.0f};
    /// Rightward on screen (positive X)
    inline constexpr Vec2 Right{1.0f, 0.0f};
    /// No direction (zero vector)
    inline constexpr Vec2 Zero{0.0f, 0.0f};
}

/// Default gravity for 2D platformer physics (980 pixels/s² downward)
/// This is 9.8 m/s² scaled by PIXELS_PER_METER (100)
namespace PhysicsDefaults {
    inline constexpr float GravityMagnitude = 980.0f;
    inline constexpr Vec2 Gravity{0.0f, GravityMagnitude};  // Points down (+Y)
    inline constexpr float PixelsPerMeter = 100.0f;
}

enum class BodyType : std::uint8_t {
    Static,
    Kinematic,
    Dynamic
};

struct PhysicsBodyDef {
    BodyType type = BodyType::Dynamic;
    Transform2D transform;
    Vec2 size = {32.0f, 32.0f};  // Collision box size in pixels
    bool fixedRotation = true;
    float linearDamping = 0.0f;
    float angularDamping = 0.0f;
    float density = 1.0f;
    float friction = 0.3f;
    float restitution = 0.0f;
    bool isSensor = false;  // Detects overlap but no collision response
};

using CollisionLayer = std::uint16_t;
using CollisionMask = std::uint16_t;

struct CollisionEvent {
    Entity entityA;
    Entity entityB;
    Vec2 contactPoint;
    Vec2 normal;
    float impulse;
};

struct TriggerEvent {
    Entity entityA;
    Entity entityB;
    Vec2 contactPoint;
};

//==========================================================================
// Ground Check Types
//==========================================================================

struct GroundCheckParams {
    float rayDistance = 5.0f;          // How far below to check (pixels)
    float slopeToleranceDeg = 60.0f;   // Max slope angle considered "ground"
    CollisionMask groundMask = 0xFFFF; // Which layers count as ground
};

struct GroundCheckResult {
    bool grounded = false;
    Entity groundEntity{};              // What we're standing on (if any)
    Vec2 contactPoint{};                // Where we're touching
    Vec2 surfaceNormal{0.0f, -1.0f};    // Surface orientation (default: pointing up)
    float slopeAngle = 0.0f;            // Angle in degrees from vertical
};

//==========================================================================
// Level Types
//==========================================================================

using LevelId = UUID;

enum class LevelState : std::uint8_t {
    Unloaded,
    Loading,
    Loaded,
    Active,
    Unloading
};

enum class LevelEvent : std::uint8_t {
    LoadStarted,
    LoadCompleted,
    UnloadStarted,
    UnloadCompleted,
    Activated,
    Deactivated
};

struct LevelEventData {
    LevelId levelId;
    LevelEvent event;
};

struct LevelTransition {
    LevelId fromLevel;
    LevelId toLevel;
    std::optional<std::string> spawnPoint;
    bool unloadPrevious = true;
};

struct EntityDef {
    std::string type;  // "platform", "enemy", "collectible", etc.
    Transform2D transform;
    std::unordered_map<std::string, std::any> properties;  // Custom properties
};

//==========================================================================
// Event Types
//==========================================================================

using EventType = std::string;

struct EntityEventData {
    Entity entity;
    std::optional<Entity> otherEntity;
};

struct DamageEventData {
    Entity target;
    Entity source;
    int amount;
    Vec2 knockback;
};

using EventData = std::variant<
    EntityEventData,
    DamageEventData,
    LevelEventData,
    CollisionEvent,
    TriggerEvent,
    std::any
>;

using EventCallback = std::function<void(const EventData&)>;
using SubscriptionId = UUID;

//==========================================================================
// Save Types
//==========================================================================

using SaveSlot = std::uint32_t;

namespace SaveSlots {
    inline constexpr SaveSlot QuickSave = UINT32_MAX - 1;
    inline constexpr SaveSlot AutoSave = UINT32_MAX;
}

enum class SaveError {
    Success,
    FileNotFound,
    CorruptedFile,
    InvalidChecksum,
    VersionMismatch,
    MigrationFailed,
    IOError,
    SerializationError
};

//==========================================================================
// AI Types
//==========================================================================

enum class BehaviorStatus {
    Success,
    Failure,
    Running
};

}  // namespace jframe
