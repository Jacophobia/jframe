// bestow-contract/src/bestow.types.cppm
// Core types and aliases for Bestow

module;

// MSVC C++23 module compatibility - use full EnTT header
#include <bestow/entt_compat.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>

export module bestow.types;

import std;

export namespace bestow {

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
using Vec4 = glm::vec4;
using Mat3 = glm::mat3;
using Mat4 = glm::mat4;
using Quat = glm::quat;

struct Transform2D {
    float x = 0.0f;
    float y = 0.0f;
    float rotation = 0.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;

    Vec2 position() const { return {x, y}; }
    Vec2 scale() const { return {scaleX, scaleY}; }
};

struct Velocity2D {
    float x = 0.0f;
    float y = 0.0f;

    Vec2 value() const { return {x, y}; }
    float length() const { return std::sqrt(x * x + y * y); }
};

/// String-based entity tag for grouping/querying
struct Tag {
    std::string value;
};

/// Human-readable entity name for debugging/querying
struct Name {
    std::string value;
};

struct Transform3D {
    Vec3 position{0.0f, 0.0f, 0.0f};
    Quat rotation{1.0f, 0.0f, 0.0f, 0.0f};  // Identity quaternion (w, x, y, z)
    Vec3 scale{1.0f, 1.0f, 1.0f};

    static Transform3D identity() { return {}; }
};

struct AABB3D {
    Vec3 min{0.0f};
    Vec3 max{0.0f};

    Vec3 center() const { return (min + max) * 0.5f; }
    Vec3 extents() const { return (max - min) * 0.5f; }
    Vec3 size() const { return max - min; }
};

struct Ray3D {
    Vec3 origin{0.0f};
    Vec3 direction{0.0f, 0.0f, -1.0f};

    Vec3 pointAt(float t) const { return origin + direction * t; }
};

struct Plane {
    Vec3 normal{0.0f, 1.0f, 0.0f};
    float distance{0.0f};
};

struct Frustum {
    Plane planes[6];  // Near, Far, Left, Right, Top, Bottom
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

/// Float-based rectangle for 2D positioning
struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    bool contains(float px, float py) const {
        return px >= x && px <= x + width && py >= y && py <= y + height;
    }

    bool intersects(const Rect& other) const {
        return !(x + width < other.x || other.x + other.width < x ||
                y + height < other.y || other.y + other.height < y);
    }
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

    /// Create Color from float values (0.0-1.0 range)
    static constexpr Color fromFloat(float r, float g, float b, float a = 1.0f) {
        return {
            static_cast<std::uint8_t>(r * 255.0f),
            static_cast<std::uint8_t>(g * 255.0f),
            static_cast<std::uint8_t>(b * 255.0f),
            static_cast<std::uint8_t>(a * 255.0f)
        };
    }
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
    BehaviorTree,
    // 3D asset types
    Mesh,       // 3D mesh (OBJ, glTF, FBX)
    Model,      // 3D model with materials (glTF, FBX)
    Material,   // Material definition (JSON, glTF embedded)
    Cubemap     // Skybox/environment map
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

/// Keyboard modifier keys (bitmask flags matching GLFW constants)
/// These can be combined with bitwise OR to require multiple modifiers
enum class ModifierKey : std::uint8_t {
    None     = 0,
    Shift    = 1,   // GLFW_MOD_SHIFT
    Ctrl     = 2,   // GLFW_MOD_CONTROL
    Alt      = 4,   // GLFW_MOD_ALT
    Super    = 8,   // GLFW_MOD_SUPER (Windows/Command key)
    CapsLock = 16,  // GLFW_MOD_CAPS_LOCK
    NumLock  = 32   // GLFW_MOD_NUM_LOCK
};

/// Bitwise operators for ModifierKey
constexpr ModifierKey operator|(ModifierKey a, ModifierKey b) {
    return static_cast<ModifierKey>(static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b));
}

constexpr ModifierKey operator&(ModifierKey a, ModifierKey b) {
    return static_cast<ModifierKey>(static_cast<std::uint8_t>(a) & static_cast<std::uint8_t>(b));
}

constexpr ModifierKey& operator|=(ModifierKey& a, ModifierKey b) {
    return a = a | b;
}

constexpr bool hasModifier(ModifierKey mods, ModifierKey test) {
    return (mods & test) == test;
}

struct InputBinding {
    InputDeviceType deviceType = InputDeviceType::Keyboard;
    int deviceIndex = 0;
    int keyCode = 0;
    ModifierKey requiredModifiers = ModifierKey::None;  ///< Modifiers that must be held for this binding
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
/// In Bestow, the Y-axis points downward (positive Y = down, negative Y = up)
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
// 3D Physics Types
//==========================================================================

enum class BodyType3D : std::uint8_t {
    Static,
    Kinematic,
    Dynamic
};

enum class ShapeType3D : std::uint8_t {
    Box,
    Sphere,
    Capsule,
    Cylinder,
    ConvexHull,
    Mesh,
    Compound,
    HeightField
};

using CollisionLayer3D = std::uint16_t;
using CollisionMask3D = std::uint16_t;

namespace CollisionLayers3D {
    inline constexpr CollisionLayer3D Default = 0x0001;
    inline constexpr CollisionLayer3D Static = 0x0002;
    inline constexpr CollisionLayer3D Dynamic = 0x0004;
    inline constexpr CollisionLayer3D Character = 0x0008;
    inline constexpr CollisionLayer3D Projectile = 0x0010;
    inline constexpr CollisionLayer3D Trigger = 0x0020;
    inline constexpr CollisionLayer3D Debris = 0x0040;
    inline constexpr CollisionLayer3D Vehicle = 0x0080;
}

struct RaycastHit3D {
    Entity entity;
    Vec3 point;
    Vec3 normal;
    float distance;
    std::uint32_t shapeIndex;
};

struct ShapecastHit3D {
    Entity entity;
    Vec3 point;
    Vec3 normal;
    float distance;
    std::uint32_t shapeIndex;
    float penetrationDepth;
};

struct CollisionEvent3D {
    Entity entityA;
    Entity entityB;
    Vec3 contactPoint;
    Vec3 contactNormal;
    float impulse;
    float penetrationDepth;
};

struct TriggerEvent3D {
    Entity entityA;
    Entity entityB;
};

// Constraint types
enum class ConstraintType3D : std::uint8_t {
    Fixed,
    Point,
    Distance,
    Hinge,
    Slider,
    Cone,
    SixDOF
};

using ConstraintId3D = std::uint64_t;

// Character controller
enum class CharacterGroundState : std::uint8_t {
    OnGround,
    OnSteepGround,
    InAir,
    Sliding
};

struct CharacterGroundInfo {
    CharacterGroundState state = CharacterGroundState::InAir;
    Entity groundEntity;
    Vec3 groundNormal{0.0f, 1.0f, 0.0f};
    Vec3 groundPoint{0.0f};
    float slopeAngle = 0.0f;
};

// Vehicle input
struct VehicleInput {
    float throttle = 0.0f;
    float brake = 0.0f;
    float steering = 0.0f;
    bool handbrake = false;
};

//==========================================================================
// 3D Graphics Types
//==========================================================================

struct Vertex3D {
    Vec3 position{0.0f};
    Vec3 normal{0.0f, 1.0f, 0.0f};
    Vec2 texCoord{0.0f};
    Vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
    // Bone data for skeletal animation (up to 4 bones per vertex)
    std::uint8_t boneIndices[4]{0, 0, 0, 0};
    float boneWeights[4]{0.0f, 0.0f, 0.0f, 0.0f};
};

using MeshHandle = std::uint64_t;
using MaterialHandle = std::uint64_t;
using ShaderProgramHandle = std::uint64_t;  // Handle to a compiled shader program

enum class BlendMode : std::uint8_t {
    Opaque,
    AlphaTest,
    AlphaBlend,
    Additive,
    Multiply
};

enum class CullMode : std::uint8_t {
    None,
    Front,
    Back
};

enum class LightType : std::uint8_t {
    Directional,
    Point,
    Spot
};

struct Light3D {
    LightType type = LightType::Point;
    Vec3 color{1.0f, 1.0f, 1.0f};
    float intensity = 1.0f;
    float range = 10.0f;
    float innerConeAngle = 0.4f;
    float outerConeAngle = 0.5f;
    bool castShadows = true;
};

enum class ProjectionType : std::uint8_t {
    Perspective,
    Orthographic
};

struct Camera3D {
    Transform3D transform;
    ProjectionType projection = ProjectionType::Perspective;
    float fovY = 60.0f;
    float aspectRatio = 16.0f / 9.0f;
    float orthoWidth = 10.0f;
    float orthoHeight = 10.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
};

struct Fog {
    bool enabled = false;
    Vec3 color{0.5f, 0.5f, 0.6f};
    float density = 0.01f;
    float startDistance = 10.0f;
    float endDistance = 100.0f;
};

// Debug rendering
struct DebugLine3D {
    Vec3 start{0.0f};
    Vec3 end{0.0f};
    Color color = Color::white();
    float duration = 0.0f;
    bool depthTest = true;
};

struct DebugBox3D {
    Vec3 center{0.0f};
    Vec3 halfExtents{0.5f};
    Quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    Color color = Color::white();
    float duration = 0.0f;
    bool depthTest = true;
};

struct DebugSphere3D {
    Vec3 center{0.0f};
    float radius = 0.5f;
    Color color = Color::white();
    float duration = 0.0f;
    bool depthTest = true;
};

// 3D Mesh Rendering Component - attach to entities for 3D rendering
struct Mesh3DComponent {
    MeshHandle mesh = 0;
    MaterialHandle material = 0;
    RenderLayer layer = 0;
    bool visible = true;
    bool castShadow = true;
    bool receiveShadow = true;
};

// 3D Light Component - attach to entities for dynamic lighting
struct Light3DComponent {
    Light3D light;
    bool enabled = true;
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

// Asset event data
struct AssetEventData {
    AssetHandle handle;
    AssetType type;
    AssetState state;
    std::string error;  // Empty if no error
};

// Config event data
struct ConfigEventData {
    std::string key;
    std::string section;
};

// Shader/Material reload event data
struct ShaderReloadEventData {
    std::uint64_t handle;  // ShaderProgramHandle or MaterialHandle
    bool success;
    std::string error;
};

// Game state event data
struct StateChangeEventData {
    std::string oldStateName;
    std::string newStateName;
};

// File change event data (dev tools)
struct FileChangeEventData {
    std::string path;
    std::string fileType;  // "shader", "config", "texture", etc.
};

using EventData = std::variant<
    EntityEventData,
    DamageEventData,
    LevelEventData,
    CollisionEvent,
    TriggerEvent,
    CollisionEvent3D,
    TriggerEvent3D,
    AssetEventData,
    ConfigEventData,
    ShaderReloadEventData,
    StateChangeEventData,
    FileChangeEventData,
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

//==========================================================================
// Path Resolution Utilities
//==========================================================================

/// Path scheme prefixes for asset resolution
/// - `:assets:/path` - Resolves to executable directory + path (game-specific assets)
/// - `:library:/path` - Resolves to Bestow library asset directory (engine shaders, etc.)
/// - Plain paths are resolved relative to current working directory
namespace PathScheme {
    inline constexpr std::string_view Assets = ":assets:/";
    inline constexpr std::string_view Library = ":library:/";
}

/// Global path resolver for handling :assets:/ and :library:/ prefixes
/// This allows assets to be loaded regardless of current working directory
class PathResolver {
public:
    /// Initialize the path resolver with executable directory detection
    /// Call this early in your application, passing argv[0] or empty string for auto-detect
    static void initialize(std::string_view executablePath = "") {
        instance().doInitialize(executablePath);
    }

    /// Set the library path explicitly (for engine assets like built-in shaders)
    /// If not set, defaults to "library" subdirectory next to executable
    static void setLibraryPath(std::string_view path) {
        auto& inst = instance();
        inst.libraryPath_ = std::filesystem::path(path);
        if (!inst.libraryPath_.is_absolute()) {
            inst.libraryPath_ = inst.assetsPath_ / inst.libraryPath_;
        }
    }

    /// Set the assets path explicitly (for game assets like models, textures)
    /// If not set, defaults to executable directory or current working directory
    static void setAssetsPath(std::string_view path) {
        auto& inst = instance();
        inst.assetsPath_ = std::filesystem::path(path);
        if (!inst.assetsPath_.is_absolute()) {
            inst.assetsPath_ = std::filesystem::current_path() / inst.assetsPath_;
        }
    }

    /// Get the resolved assets path (executable directory)
    static std::filesystem::path getAssetsPath() {
        return instance().assetsPath_;
    }

    /// Get the resolved library path
    static std::filesystem::path getLibraryPath() {
        return instance().libraryPath_;
    }

    /// Resolve a path that may contain :assets:/ or :library:/ prefixes
    /// Returns an absolute path suitable for file operations
    static std::filesystem::path resolve(std::string_view path) {
        return instance().doResolve(path);
    }

    /// Resolve and return as string (convenience method)
    static std::string resolveString(std::string_view path) {
        return resolve(path).string();
    }

    /// Check if a path uses a scheme prefix
    static bool hasScheme(std::string_view path) {
        return path.starts_with(PathScheme::Assets) || path.starts_with(PathScheme::Library);
    }

private:
    PathResolver() = default;

    static PathResolver& instance() {
        static PathResolver inst;
        return inst;
    }

    void doInitialize(std::string_view executablePath) {
        initialized_ = true;

        // Try to determine the executable directory
        std::filesystem::path execPath;

        if (!executablePath.empty()) {
            execPath = std::filesystem::path(executablePath);
        }

        // Get the directory containing the executable
        if (!execPath.empty() && std::filesystem::exists(execPath)) {
            assetsPath_ = std::filesystem::absolute(execPath).parent_path();
        } else {
            // Fall back to current path
            assetsPath_ = std::filesystem::current_path();
        }

        // Default library path to a "library" subdirectory (can be overridden)
        libraryPath_ = assetsPath_ / "library";
    }

    std::filesystem::path doResolve(std::string_view path) const {
        if (path.empty()) {
            return {};
        }

        // Handle :assets:/ prefix
        if (path.starts_with(PathScheme::Assets)) {
            std::string_view remainder = path.substr(PathScheme::Assets.size());
            return assetsPath_ / std::filesystem::path(remainder);
        }

        // Handle :library:/ prefix
        if (path.starts_with(PathScheme::Library)) {
            std::string_view remainder = path.substr(PathScheme::Library.size());
            return libraryPath_ / std::filesystem::path(remainder);
        }

        // Check if already absolute
        std::filesystem::path p(path);
        if (p.is_absolute()) {
            return p;
        }

        // Relative path - resolve against assets path
        return assetsPath_ / p;
    }

    std::filesystem::path assetsPath_;
    std::filesystem::path libraryPath_;
    bool initialized_ = false;
};

}  // namespace bestow
