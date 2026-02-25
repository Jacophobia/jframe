# Shared Patterns & Foundation Types

Every V2 contract uses the patterns defined here. Read this before reading any individual system doc.

---

## 1. Strong Typed Handles

All handles are distinct types with zero runtime overhead. You cannot accidentally swap a `MeshHandle` for a `MaterialHandle`.

```cpp
template<typename Tag>
struct Handle {
    std::uint64_t id = 0;

    constexpr bool isValid() const noexcept { return id != 0; }
    constexpr explicit operator bool() const noexcept { return id != 0; }
    constexpr auto operator<=>(const Handle&) const = default;

    struct Hash {
        std::size_t operator()(Handle h) const noexcept {
            return std::hash<std::uint64_t>{}(h.id);
        }
    };
};
```

### Handle Type Catalog

| Handle Type | Tag | Used By | Description |
|------------|-----|---------|-------------|
| `AssetHandle` | `AssetTag` | Assets | Reference to a registered asset |
| `MeshHandle` | `MeshTag` | Graphics3D | GPU mesh buffer |
| `MaterialHandle` | `MaterialTag` | Graphics3D | Material instance |
| `TextureHandle` | `TextureTag` | Graphics2D/3D, UI | GPU texture |
| `FontHandle` | `FontTag` | Graphics2D/3D, UI | Loaded font |
| `ShaderHandle` | `ShaderTag` | Shader | Compiled shader/program |
| `SoundHandle` | `SoundTag` | Audio | Active sound instance |
| `ChannelHandle` | `ChannelTag` | Audio | Named audio channel |
| `BodyHandle` | `BodyTag` | Physics2D/3D | Physics body |
| `ConstraintHandle` | `ConstraintTag` | Physics2D/3D | Physics joint/constraint |
| `SkeletonHandle` | `SkeletonTag` | Animation | Bone hierarchy |
| `AnimClipHandle` | `AnimClipTag` | Animation | Animation clip |
| `AnimatorHandle` | `AnimatorTag` | Animation | Per-entity animator instance |
| `EmitterHandle` | `EmitterTag` | Particles | Particle emitter |
| `TweenHandle` | `TweenTag` | Tween | Active tween |
| `UIDocHandle` | `UIDocTag` | UI | Loaded UI document |
| `UIElementHandle` | `UIElementTag` | UI | DOM element reference |
| `NetworkId` | `NetworkTag` | Network | Client/player identity |
| `RPCHandle` | `RPCTag` | Network | Registered RPC function |
| `SubscriptionId` | `SubscriptionTag` | All systems | Event subscription |

---

## 2. Unified Error Pattern

All fallible operations return `Result<T, SystemError>`.

### SystemError

```cpp
enum class ErrorCategory : std::uint8_t {
    None = 0,        // No error
    InvalidHandle,   // Handle is null, expired, or wrong type
    InvalidArgument, // Argument out of range or malformed
    NotFound,        // Resource, entity, or key not found
    AlreadyExists,   // Duplicate registration
    NotInitialized,  // System not yet initialized
    IOError,         // File read/write failure
    ParseError,      // Lua/JSON/config parsing failure
    OutOfMemory,     // Allocation failure
    NotSupported,    // Feature not available on this backend
    Timeout,         // Async operation timed out
    NetworkError,    // Connection lost, packet failure
    InternalError    // Bug in the implementation
};

struct SystemError {
    ErrorCategory category = ErrorCategory::None;
    std::string message;       // Human-readable, always populated
    std::string systemName;    // "Audio", "Physics3D", etc.

    constexpr explicit operator bool() const noexcept {
        return category != ErrorCategory::None;
    }
};
```

### Result Type

```cpp
template<typename T>
using Result = std::expected<T, SystemError>;
```

### Usage Patterns

```cpp
// Fallible creation — returns handle or error
Result<MeshHandle> createMesh(const MeshDef& def);

// Fallible void operation — returns nothing or error
Result<void> destroyMesh(MeshHandle handle);

// Fallible query — returns value or error
Result<Vec3> getPosition(BodyHandle body) const;

// Infallible query — returns value directly (never fails)
Vec3 getGravity() const;

// Infallible check — returns bool directly
bool isValid(Entity entity) const;
```

### Lua Mapping

All `Result<T>` operations map to two-value Lua returns:

```lua
-- Success: (value, nil)
local mesh, err = bestow.graphics3d.core.createMesh(def)

-- Failure: (nil, error_table)
-- err = { category = "InvalidArgument", message = "Empty vertex list", system = "Graphics3D" }

-- Infallible operations return the value directly:
local gravity = bestow.physics.getGravity()  -- always Vec2, never nil
```

---

## 3. Unified Callback Pattern

All systems that emit events follow the same subscription pattern.

### Contract

```cpp
// Subscribe — returns an ID you can use to unsubscribe later
SubscriptionId subscribe(EventType type, Callback callback);

// Unsubscribe — safe to call with invalid/already-unsubscribed IDs
void unsubscribe(SubscriptionId id);
```

### RAII Guard (Optional)

```cpp
class ScopedSubscription {
public:
    ScopedSubscription(SubscriptionId id, std::function<void(SubscriptionId)> unsub);
    ~ScopedSubscription();  // calls unsub_(id_)
    ScopedSubscription(ScopedSubscription&&) noexcept;
    ScopedSubscription& operator=(ScopedSubscription&&) noexcept;
    ScopedSubscription(const ScopedSubscription&) = delete;
    void release();  // detach without unsubscribing
private:
    SubscriptionId id_;
    std::function<void(SubscriptionId)> unsub_;
};
```

### Rules

1. Every `subscribe()` returns a `SubscriptionId`
2. Every system that has `subscribe()` also has `unsubscribe()`
3. Callbacks are never invoked after `unsubscribe()` returns
4. Calling `unsubscribe()` with an invalid ID is a no-op (no error)
5. The system owns the callback — it must not outlive the system

---

## 4. Typed Variants (No `std::any`)

V2 eliminates all `std::any` usage with purpose-specific typed variants.

### UniformValue (Material/Shader Parameters)

```cpp
using UniformValue = std::variant<
    float, int, bool,
    Vec2, Vec3, Vec4,
    Mat3, Mat4,
    Color,
    TextureHandle
>;
```

**Used by:** Graphics2D, Graphics3D, Shader

### SceneParam (Scene/Blueprint Parameters)

```cpp
using SceneParam = std::variant<
    float, int, bool,
    std::string,
    Vec2, Vec3
>;

using SceneParams = std::unordered_map<std::string, SceneParam>;
```

**Used by:** Scene, Blueprints

### BlackboardValue (AI Behavior Trees)

```cpp
using BlackboardValue = std::variant<
    float, int, bool,
    std::string,
    Vec2, Vec3,
    Entity
>;
```

**Used by:** AI

### ComponentFieldValue (ECS Reflection)

```cpp
using ComponentFieldValue = std::variant<
    float, int, bool,
    std::string,
    Vec2, Vec3, Vec4,
    Quat, Color,
    Entity
>;

using ComponentData = std::unordered_map<std::string, ComponentFieldValue>;
```

**Used by:** Entity (Lua bridge)

### EventData (Event Payloads)

```cpp
using EventData = std::variant<
    CollisionEvent, TriggerEvent,
    CollisionEvent3D, TriggerEvent3D,
    ActionEventData, PhaseEventData,
    AssetChangedEvent, SceneChangedEvent,
    EntityLifecycleEvent,
    CustomEvent  // For Lua-originated events
>;

struct CustomEvent {
    std::string name;
    SceneParams data;
};
```

**Used by:** Events

---

## 5. Update Phases

The game loop runs in 7 phases per frame. Each system declares which phase it runs in.

```cpp
enum class UpdatePhase : std::uint8_t {
    EarlyUpdate,    // Input polling, network receive, hot reload checks
    FixedUpdate,    // Physics at fixed timestep (deterministic)
    Update,         // Game logic, AI, animation, tweens
    LateUpdate,     // Camera, particles, post-logic cleanup
    PreRender,      // Culling, render queue building, sorting
    Render,         // Drawing
    PostRender      // Network send, frame present, async save
};
```

### Fixed Timestep

Physics runs in `FixedUpdate` with a fixed timestep accumulator:

```cpp
struct TimeConfig {
    float fixedTimestep = 1.0f / 60.0f;  // 60 Hz physics
    float maxDeltaTime = 0.25f;           // Clamp spiral-of-death
    int maxFixedStepsPerFrame = 8;        // Safety cap
};
```

See `DEPENDENCY-DIAGRAM.md` for the full phase-to-system assignment table.

---

## 6. SystemContext

The typed composition root that replaces the runtime DI container. See individual system docs for how each system is wired in.

```cpp
struct SystemContext {
    // Protected systems (peer-only)
    std::unique_ptr<IAssetCore>           assets;
    std::unique_ptr<IShaderCore>          shader;
    std::unique_ptr<IGraphicsContextCore> graphicsContext;
    std::unique_ptr<IUIRenderBackend>     uiRenderBackend;
    std::unique_ptr<IBlueprintCore>       blueprints;
    std::unique_ptr<IMetricsCore>         metrics;

    // Public systems — each has Core (owned) + System (view)
    // The implementation object satisfies both interfaces.
    std::unique_ptr<IEntityCore>     entityCore;
    IEntitySystem*                   entity = nullptr;

    std::unique_ptr<IEventCore>      events;
    // ... (one pair per public system)
};
```

**Benefits:**
- All dependencies visible and typed at compile time
- No registration order fragility
- No runtime type lookups
- Errors caught by the compiler

---

## 7. Math Types

```cpp
struct Vec2 { float x = 0, y = 0; };
struct Vec3 { float x = 0, y = 0, z = 0; };
struct Vec4 { float x = 0, y = 0, z = 0, w = 0; };
struct Quat { float x = 0, y = 0, z = 0, w = 1; };
struct Mat3 { float m[9]; };
struct Mat4 { float m[16]; };
struct Color { float r = 1, g = 1, b = 1, a = 1; };
struct Size  { int width = 0, height = 0; };

struct Transform2D {
    Vec2 position;
    float rotation = 0;     // radians
    Vec2 scale = {1, 1};
};

struct Transform3D {
    Vec3 position;
    Quat rotation;
    Vec3 scale = {1, 1, 1};
};

struct AABB2D { Vec2 min, max; };
struct AABB3D { Vec3 min, max; };
struct Ray3D  { Vec3 origin, direction; };
struct Plane  { Vec3 normal; float distance; };
struct Frustum { Plane planes[6]; };
```

---

## 8. Naming Conventions

| Thing | Convention | Example |
|-------|-----------|---------|
| Contract interface | `I<System>Core` / `I<System>System` | `IAudioCore`, `IAudioSystem` |
| Handle type | `<Noun>Handle` | `MeshHandle`, `SoundHandle` |
| Error enum | `ErrorCategory` | (unified across all systems) |
| Lua high-level | `bestow.<system>.<method>` | `bestow.audio.play()` |
| Lua low-level | `bestow.<system>.core.<method>` | `bestow.audio.core.playOnChannel()` |
| Event name | `snake_case` string | `"collision"`, `"asset_loaded"` |
| Config key | `dot.separated.path` | `"graphics.msaa_samples"` |
| Component name | `PascalCase` string | `"Transform2D"`, `"Health"` |

---

## 9. Document Template

Each system doc follows this structure:

1. **Header** — System name, visibility, tier, dependencies
2. **Purpose** — One paragraph on what this system does
3. **High-Level API** — The `I<System>System` contract with field descriptions
4. **Low-Level API** — The `I<System>Core` contract with field descriptions
5. **Types** — All structs/enums specific to this system
6. **Lua Mapping** — How the C++ contract maps to Lua
7. **Examples** — Usage snippets in both C++ and Lua
