# Bestow V2 — Comprehensive API Contract Specification

> **Status:** Draft — Produced by analysis of competitive gaps, architectural review, and critical flaw remediation.
>
> **Guiding Principles:**
> - **Usability** (High-Level API) — A game developer should accomplish common tasks in 1-3 calls
> - **Configurability** (Low-Level API) — A power user should be able to tune every knob
> - **Abstraction** (Both) — Contracts define *interaction*, not *implementation*
>
> **Prerequisite Reading:**
> - `BESTOW-COMPETITIVE-ANALYSIS.md` — Feature gap audit
> - `SYSTEM-ARCHITECTURE.md` — System inventory and dependency graph
> - `V2-CONTRACT-ARCHITECTURE.md` — Dual API concept and self-updating docs
> - `V2-CRITICAL-REVIEW.md` — 12 architectural flaws and fixes

---

## Table of Contents

1. [Architectural Fixes (Baked Into Every Contract)](#1-architectural-fixes)
2. [System Visibility Model](#2-system-visibility-model)
3. [Foundation Types (`bestow.types` v2)](#3-foundation-types)
4. [Protected Systems (Peer-Only)](#4-protected-systems)
5. [Public Systems — Low-Level Core Contracts](#5-low-level-core-contracts)
6. [Public Systems — High-Level System Contracts](#6-high-level-system-contracts)
7. [SystemContext (Replacing DI)](#7-systemcontext)
8. [Game Loop & Update Phases](#8-game-loop)
9. [Lua API Mapping](#9-lua-api-mapping)
10. [Self-Updating Documentation Infrastructure](#10-documentation-infrastructure)
11. [Migration Path](#11-migration-path)

---

## 1. Architectural Fixes

These fixes from the Critical Review are **not optional enhancements** — they are baked into every contract definition in this document. Every system in V2 must conform to these patterns.

### 1.1 Strong Typed Handles (Flaw #4 Fix)

**Problem:** All handles are `uint64_t` — swapping a MeshHandle and MaterialHandle compiles silently.

**V2 Rule:** Every handle type is a distinct strong type with zero runtime overhead.

```cpp
// bestow.types v2
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

// Each system declares its own handle types:
using MeshHandle      = Handle<struct MeshTag>;
using MaterialHandle  = Handle<struct MaterialTag>;
using TextureHandle   = Handle<struct TextureTag>;
using SoundHandle     = Handle<struct SoundTag>;
using EntityHandle    = Handle<struct EntityTag>;  // Wraps entt::entity
using AssetHandle     = Handle<struct AssetTag>;
using SkeletonHandle  = Handle<struct SkeletonTag>;
using AnimClipHandle  = Handle<struct AnimClipTag>;
using AnimatorHandle  = Handle<struct AnimatorTag>;
using EmitterHandle   = Handle<struct EmitterTag>;
using TweenHandle     = Handle<struct TweenTag>;
using BodyHandle      = Handle<struct BodyTag>;
using ConstraintHandle = Handle<struct ConstraintTag>;
using ChannelHandle   = Handle<struct ChannelTag>;
using FontHandle      = Handle<struct FontTag>;
using ShaderHandle    = Handle<struct ShaderTag>;
using UIDocHandle     = Handle<struct UIDocTag>;
using UIElementHandle = Handle<struct UIElementTag>;
using NetworkId       = Handle<struct NetworkTag>;
using RPCHandle       = Handle<struct RPCTag>;

// Compile-time safety:
// graphics->drawMesh(materialHandle, meshHandle, transform);  // ERROR — types don't match
```

### 1.2 Unified Error Pattern (Flaw #7 Fix)

**Problem:** Five different error patterns across systems (Result, bool, void, Handle-or-zero, nil).

**V2 Rule:** All fallible operations return `Result<T, SystemError>`. Period.

```cpp
// Unified error type
enum class ErrorCategory : std::uint8_t {
    None = 0,
    InvalidHandle,
    InvalidArgument,
    NotFound,
    AlreadyExists,
    NotInitialized,
    IOError,
    ParseError,
    OutOfMemory,
    NotSupported,
    Timeout,
    NetworkError,
    InternalError
};

struct SystemError {
    ErrorCategory category = ErrorCategory::None;
    std::string message;        // Human-readable, always populated
    std::string systemName;     // "Audio", "Physics", etc.

    constexpr explicit operator bool() const noexcept {
        return category != ErrorCategory::None;
    }
};

// Usage in contracts:
template<typename T>
using Result = std::expected<T, SystemError>;

// Examples:
Result<MeshHandle>  createMesh(const MeshDef& def);       // Returns handle or error
Result<void>        destroyMesh(MeshHandle handle);        // Returns nothing or error
Result<Vec3>        getPosition(BodyHandle body) const;    // Returns value or error
```

### 1.3 Unified Callback Pattern (Flaw #7 Fix)

**Problem:** Three different subscription patterns. Some can't unsubscribe.

**V2 Rule:** All subscriptions return `SubscriptionId`. All support `unsubscribe()`. Subscriptions are scoped to the system that created them.

```cpp
using SubscriptionId = Handle<struct SubscriptionTag>;

// Every system that emits events provides:
SubscriptionId subscribe(EventType type, Callback cb);
void unsubscribe(SubscriptionId id);

// RAII guard available:
class ScopedSubscription {
public:
    ScopedSubscription(SubscriptionId id, std::function<void(SubscriptionId)> unsub);
    ~ScopedSubscription(); // calls unsub_
    ScopedSubscription(ScopedSubscription&&) noexcept;
    ScopedSubscription& operator=(ScopedSubscription&&) noexcept;
    ScopedSubscription(const ScopedSubscription&) = delete;
    void release(); // detach without unsubscribing
};
```

### 1.4 No `std::any` (Flaw #5 Fix)

**Problem:** `std::any` used for scene params, material uniforms, asset data, blackboard values. Game devs must guess types.

**V2 Rule:** Replace every `std::any` with a typed variant or the concrete type.

```cpp
// Material uniforms — closed set of GPU-compatible types
using UniformValue = std::variant<
    float, int, bool,
    Vec2, Vec3, Vec4,
    Mat3, Mat4,
    Color,
    TextureHandle
>;

// Scene parameters — use sol::table (naturally heterogeneous in Lua)
// C++ side uses a typed map:
using SceneParams = std::unordered_map<std::string, std::variant<
    float, int, bool, std::string, Vec2, Vec3
>>;

// AI blackboard — typed variant
using BlackboardValue = std::variant<
    float, int, bool, std::string,
    Vec2, Vec3, Entity
>;

// Asset data — use concrete types, never std::any
// getAsset<TextureData>(handle) instead of std::any_cast
```

### 1.5 No Leaky Abstractions (Flaw #8 Fix)

**Problem:** `IGraphicsContext` exposes `void* getRenderContext()` documented as "returns VkContext".

**V2 Rule:** Public contracts contain zero backend-specific types. Vendor extensions live in separate, optional interfaces.

```cpp
// PUBLIC CONTRACT — no backend types
class IGraphicsContext {
public:
    virtual Size getWindowSize() const = 0;
    virtual bool isInFrame() const = 0;
    // NO: void* getRenderContext()
    // NO: void* getCurrentCommandBuffer()
};

// VENDOR EXTENSION — in bestow-vulkan, not bestow-contract
class IVulkanContext {
public:
    virtual VkInstance getInstance() const = 0;
    virtual VkDevice getDevice() const = 0;
    virtual VkCommandBuffer getCurrentCommandBuffer() const = 0;
};

// Consumers that NEED backend access:
if (auto* vk = dynamic_cast<IVulkanContext*>(&graphicsContext)) {
    // Use Vulkan-specific features
}
```

### 1.6 Lua Two-Value Returns (Flaw #12 Fix)

**Problem:** `nil` means both "value is nil" and "operation failed". AI agents can't distinguish.

**V2 Rule:** All Lua-bound operations that can fail return `(value, error)` pairs.

```lua
-- V1 (broken): nil could be "not found" or "system crashed"
local mesh = bestow.graphics3d.createMesh(data)

-- V2 (fixed): explicit error channel
local mesh, err = bestow.graphics3d.createMesh(data)
if err then
    print("Failed: " .. err.message)  -- Always a string
    print("Category: " .. err.category)  -- "InvalidArgument", etc.
end

-- For queries that legitimately return nil:
local target = bestow.ai.findClosest(pos)  -- nil = no target found (not an error)
-- vs
local path, err = bestow.ai.findPath(start, finish)  -- nil+err = pathfinding failed
```

---

## 2. System Visibility Model

Every system falls into exactly one visibility tier:

### Public Systems (Lua API + C++ API)
Exposed to game developers through both `bestow.*` Lua namespace and C++ contracts.

| System | Lua Path | Dual API | Notes |
|--------|----------|----------|-------|
| Entity | `bestow.entity` | System + Core | ECS facade |
| Events | `bestow.events` | Single | Simple enough already |
| Input | `bestow.input` | System + Core | Action system + raw input |
| Audio | `bestow.audio` | System + Core | Play sounds + full mixer |
| Physics 2D | `bestow.physics` | System + Core | Bodies + advanced queries |
| Physics 3D | `bestow.physics3d` | System + Core | Bodies + constraints + vehicles |
| Graphics 2D | `bestow.graphics` | System + Core | Sprites + primitives + text |
| Graphics 3D | `bestow.graphics3d` | System + Core | Meshes + materials + lighting |
| Animation | `bestow.animation` | System + Core | Play anims + blend trees |
| Anim State Machine | `bestow.animation.fsm` | Single | Builder pattern |
| Camera | `bestow.camera` | System + Core | Follow + shake + zoom |
| Scene | `bestow.scene` | System + Core | Stack-based scene manager |
| State | `bestow.state` | System + Core | Save/load + key-value |
| Config | `bestow.config` | System + Core | Lua configs |
| UI | `bestow.ui` | System + Core | Documents + elements + data binding |
| AI | `bestow.ai` | System + Core | Behavior trees + nav + steering |
| GAS | `bestow.gas` | System + Core | Tags + attributes + effects + abilities |
| GameState | `bestow.gamestate` | Single | State machine (C++ heavy) |
| Tween | `bestow.tween` | Single | Value interpolation (new) |
| Particles | `bestow.particles` | System + Core | CPU/GPU particles (new) |
| Network | `bestow.network` | System + Core | Client-server multiplayer (new) |

### Protected Systems (C++ Only — Peer Systems)
Used by other engine systems. Not exposed in Lua. Game developers never touch these.

| System | Purpose | Consumers |
|--------|---------|-----------|
| Assets | File system gateway, caching, hot reload | All systems that load files |
| Shader | GLSL/SPIR-V compilation pipeline | Graphics2D, Graphics3D |
| Graphics Context | Base interface for 2D/3D graphics | UI, DevTools |
| UI Render Backend | GPU rendering commands for UI | UI System |
| Blueprints | Entity template factory | Scene, Level |
| Metrics | Tracy profiling integration | All systems |

### Internal (Same System Only)
Implementation details invisible outside their system. Examples:
- `VulkanPipeline`, `VulkanSwapchain` — internal to bestow-vulkan
- `FMODChannelManager` — internal to bestow-audio
- `Box2DWorldWrapper` — internal to bestow-physics
- `EnTTRegistryWrapper` — internal to bestow-entity

---

## 3. Foundation Types

The `bestow.types` module is Tier 0 — every other module imports it. V2 adds strong handles, unified errors, and the update phase enum.

### 3.1 Core Aliases

```cpp
export module bestow.types;

import std;

export namespace bestow {

// Time
using DeltaTime = float;
using Timestamp = double;
using FrameCount = std::uint64_t;

// Identity
using UUID = std::uint64_t;

// Entity (wraps EnTT but doesn't leak it)
enum class Entity : std::uint32_t { Null = 0 };
constexpr Entity NullEntity = Entity::Null;

// Strong handle template (Section 1.1)
template<typename Tag> struct Handle { /* ... */ };

// All handle type aliases (Section 1.1)
// ...

// Unified error types (Section 1.2)
// ...

// Subscription types (Section 1.3)
// ...
}
```

### 3.2 Math Types

```cpp
export namespace bestow {

struct Vec2 { float x = 0, y = 0; /* operators */ };
struct Vec3 { float x = 0, y = 0, z = 0; /* operators */ };
struct Vec4 { float x = 0, y = 0, z = 0, w = 0; };
struct Quat { float x = 0, y = 0, z = 0, w = 1; };
struct Mat3 { float m[9]; };
struct Mat4 { float m[16]; };

struct Color {
    float r = 1, g = 1, b = 1, a = 1;
    static constexpr Color white() { return {1,1,1,1}; }
    static constexpr Color black() { return {0,0,0,1}; }
    static constexpr Color red()   { return {1,0,0,1}; }
    // ... etc
};

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

}
```

### 3.3 Uniform Value Variant (replacing `std::any`)

```cpp
export namespace bestow {

using UniformValue = std::variant<
    float, int, bool,
    Vec2, Vec3, Vec4,
    Mat3, Mat4,
    Color,
    TextureHandle
>;

using BlackboardValue = std::variant<
    float, int, bool,
    std::string,
    Vec2, Vec3,
    Entity
>;

using SceneParam = std::variant<
    float, int, bool,
    std::string,
    Vec2, Vec3
>;

using SceneParams = std::unordered_map<std::string, SceneParam>;

}
```

### 3.4 Update Phases (Flaw #6 Fix)

```cpp
export namespace bestow {

enum class UpdatePhase : std::uint8_t {
    EarlyUpdate,    // Input polling, network receive, hot reload checks
    FixedUpdate,    // Physics at fixed timestep (deterministic)
    Update,         // Game logic, AI, animation, tweens, Lua app.update()
    LateUpdate,     // Camera, particles, position-dependent systems
    PreRender,      // Culling, render queue building, sorting
    Render,         // Drawing (graphics begin, scene render, UI, graphics end)
    PostRender      // Debug overlay, frame present, network send
};

// Fixed timestep config
struct TimeConfig {
    float fixedTimestep = 1.0f / 60.0f;   // 60 Hz physics
    float maxDeltaTime = 0.25f;            // Clamp spiral-of-death
    int maxFixedStepsPerFrame = 8;         // Safety cap
};

}
```


---

## 4. Protected Systems

These contracts are available to peer engine systems but **not** exposed in the Lua API or to game developers.

### 4.1 IAssetCore (Protected)

The sole gateway to the file system. No other system reads files directly.

```cpp
class IAssetCore {
public:
    virtual ~IAssetCore() = default;

    // Lifecycle
    virtual void update() = 0;

    // Registration & Loading
    virtual AssetHandle registerAsset(AssetType type, std::string_view path) = 0;
    virtual void unregisterAsset(AssetHandle handle) = 0;
    virtual Result<void> loadAsset(AssetHandle handle) = 0;
    virtual void loadAssetAsync(AssetHandle handle,
        std::function<void(AssetHandle, AssetState)> callback = nullptr) = 0;
    virtual Result<void> unloadAsset(AssetHandle handle) = 0;

    // State
    virtual AssetState getState(AssetHandle handle) const = 0;
    virtual bool isLoaded(AssetHandle handle) const = 0;

    // Typed Data Access (no std::any — concrete types)
    virtual const TextureData*  getTextureData(AssetHandle h) const = 0;
    virtual const FontData*     getFontData(AssetHandle h) const = 0;
    virtual const SoundData*    getSoundData(AssetHandle h) const = 0;
    virtual const ShaderData*   getShaderData(AssetHandle h) const = 0;
    virtual const MeshData*     getMeshData(AssetHandle h) const = 0;
    virtual const ModelData*    getModelData(AssetHandle h) const = 0;
    virtual const MaterialData* getMaterialData(AssetHandle h) const = 0;
    virtual const CubemapData*  getCubemapData(AssetHandle h) const = 0;
    virtual const NavMeshData*  getNavMeshData(AssetHandle h) const = 0;
    virtual const std::string*  getTextData(AssetHandle h) const = 0;  // Lua, configs

    // Convenience loaders (register + load in one call)
    virtual Result<AssetHandle> loadTexture(std::string_view path) = 0;
    virtual Result<AssetHandle> loadFont(std::string_view path) = 0;
    virtual Result<AssetHandle> loadSound(std::string_view path) = 0;
    virtual Result<AssetHandle> loadShader(std::string_view path) = 0;
    virtual Result<AssetHandle> loadShaderCompiled(std::string_view path) = 0;
    virtual Result<AssetHandle> loadMesh(std::string_view path) = 0;
    virtual Result<AssetHandle> loadModel(std::string_view path) = 0;
    virtual Result<AssetHandle> loadCubemap(std::string_view path) = 0;
    virtual Result<AssetHandle> loadCubemap(
        std::string_view posX, std::string_view negX,
        std::string_view posY, std::string_view negY,
        std::string_view posZ, std::string_view negZ) = 0;
    virtual Result<AssetHandle> loadMaterial(std::string_view luaPath) = 0;
    virtual Result<AssetHandle> loadNavMesh(std::string_view path) = 0;
    virtual Result<AssetHandle> loadData(std::string_view path) = 0;

    // Bulk operations
    virtual void loadAll() = 0;
    virtual void unloadAll() = 0;
    virtual std::vector<AssetHandle> getAssetsOfType(AssetType type) const = 0;

    // Hot Reload
    virtual void enableHotReload(bool enable) = 0;
    virtual bool isHotReloadEnabled() const = 0;
    virtual void checkForReloads() = 0;
    virtual Result<void> reloadAsset(AssetHandle handle) = 0;

    // Subscriptions
    virtual SubscriptionId subscribe(AssetHandle handle,
        std::function<void(AssetHandle, AssetType)> callback) = 0;
    virtual SubscriptionId subscribeToType(AssetType type,
        std::function<void(AssetHandle, AssetType)> callback) = 0;
    virtual void unsubscribe(SubscriptionId id) = 0;

    // Path Resolution
    virtual std::optional<std::filesystem::path> resolveLibraryPath(
        std::string_view relativePath) const = 0;
    virtual std::optional<std::filesystem::path> resolveAssetPath(
        std::string_view relativePath) const = 0;
    virtual bool assetExists(std::string_view path) const = 0;

    // Library Discovery
    virtual std::vector<std::string> listLibraryCategories() const = 0;
    virtual std::vector<std::string> listLibraryAssets(
        std::string_view category) const = 0;
};
```

### 4.2 IShaderCore (Protected)

Extracted from Graphics3D. Manages shader compilation pipeline.

```cpp
class IShaderCore {
public:
    virtual ~IShaderCore() = default;

    // Compilation
    virtual Result<ShaderHandle> compileGLSL(
        std::string_view source, ShaderStage stage) = 0;
    virtual Result<ShaderHandle> compileToSPIRV(
        std::string_view glslSource, ShaderStage stage) = 0;
    virtual Result<ShaderHandle> loadSPIRV(
        std::span<const std::uint32_t> bytecode, ShaderStage stage) = 0;

    // Program linking
    virtual Result<ShaderHandle> createProgram(
        ShaderHandle vertex, ShaderHandle fragment) = 0;
    virtual Result<ShaderHandle> createComputeProgram(ShaderHandle compute) = 0;
    virtual void destroyShader(ShaderHandle handle) = 0;

    // Reflection (for automatic uniform binding)
    virtual std::vector<UniformInfo> getUniforms(ShaderHandle program) const = 0;
    virtual std::vector<AttributeInfo> getAttributes(ShaderHandle program) const = 0;

    // Hot reload integration
    virtual Result<void> recompile(ShaderHandle handle,
        std::string_view newSource) = 0;

    // Cache
    virtual void clearCache() = 0;
    virtual std::size_t cacheSize() const = 0;
};

enum class ShaderStage : std::uint8_t {
    Vertex, Fragment, Geometry, Compute,
    TessControl, TessEval
};

struct UniformInfo {
    std::string name;
    std::uint32_t location;
    std::uint32_t binding;
    enum class Type { Float, Int, Vec2, Vec3, Vec4, Mat3, Mat4, Sampler2D, SamplerCube };
    Type type;
};

struct AttributeInfo {
    std::string name;
    std::uint32_t location;
    std::uint32_t components; // 1-4
};
```

### 4.3 IGraphicsContextCore (Protected)

Base interface for all rendering backends. Used by UI, DevTools.

```cpp
class IGraphicsContextCore {
public:
    virtual ~IGraphicsContextCore() = default;

    // Window/viewport
    virtual Size getWindowSize() const = 0;
    virtual void* getNativeWindowHandle() const = 0;
    virtual bool isInFrame() const = 0;

    // UI integration
    virtual IUIRenderBackend* getUIRenderBackend() = 0;

    // Backend identification (for vendor extensions)
    enum class Backend { OpenGL, Vulkan, Metal, Null };
    virtual Backend getBackend() const = 0;
};
```

### 4.4 IUIRenderBackend (Protected)

GPU rendering primitives for the UI system. Implemented by each graphics backend.

```cpp
class IUIRenderBackend {
public:
    virtual ~IUIRenderBackend() = default;

    virtual void beginUIPass() = 0;
    virtual void endUIPass() = 0;

    virtual TextureHandle createTexture(
        int width, int height, int channels, const unsigned char* data) = 0;
    virtual void destroyTexture(TextureHandle handle) = 0;

    virtual void setScissor(int x, int y, int w, int h) = 0;
    virtual void clearScissor() = 0;

    virtual void drawTriangles(
        std::span<const UIVertex> vertices,
        std::span<const std::uint32_t> indices,
        TextureHandle texture) = 0;

    virtual void setViewportSize(int width, int height) = 0;
};
```

### 4.5 IBlueprintCore (Protected)

Entity template factory. Used by Scene and Level systems.

```cpp
class IBlueprintCore {
public:
    virtual ~IBlueprintCore() = default;

    // Blueprint registration
    virtual Result<void> registerBlueprint(
        std::string_view name, AssetHandle luaAsset) = 0;
    virtual Result<void> unregisterBlueprint(std::string_view name) = 0;
    virtual bool hasBlueprint(std::string_view name) const = 0;

    // Instantiation
    virtual Result<Entity> instantiate(std::string_view blueprintName) = 0;
    virtual Result<Entity> instantiate(
        std::string_view blueprintName,
        const std::unordered_map<std::string, SceneParam>& overrides) = 0;

    // Queries
    virtual std::vector<std::string> getRegisteredBlueprints() const = 0;

    // Hot reload
    virtual Result<void> reloadBlueprint(std::string_view name) = 0;
};
```

### 4.6 IMetricsCore (Protected)

Tracy profiling integration. Available in debug builds.

```cpp
class IMetricsCore {
public:
    virtual ~IMetricsCore() = default;

    virtual void beginZone(std::string_view name) = 0;
    virtual void endZone() = 0;
    virtual void plotValue(std::string_view name, float value) = 0;
    virtual void logMessage(std::string_view msg) = 0;

    // Frame markers
    virtual void markFrame() = 0;

    // Counters
    virtual void incrementCounter(std::string_view name, std::int64_t delta = 1) = 0;
    virtual std::int64_t getCounter(std::string_view name) const = 0;

    // Memory tracking
    virtual void trackAlloc(const void* ptr, std::size_t size,
        std::string_view pool = "default") = 0;
    virtual void trackFree(const void* ptr,
        std::string_view pool = "default") = 0;
};
```

---

## 5. Low-Level Core Contracts

Each public system has a **Core** contract that exposes full configurability. These map to `bestow.<system>.core.*` in Lua.

Convention: Core contracts are named `I<System>Core`.

### 5.1 IEntityCore

```cpp
class IEntityCore {
public:
    virtual ~IEntityCore() = default;

    // === Lifecycle ===
    virtual Entity createEntity() = 0;
    virtual Result<void> destroyEntity(Entity entity) = 0;
    virtual bool isValid(Entity entity) const = 0;
    virtual std::size_t entityCount() const = 0;
    virtual void clear() = 0;

    // === Typed Component Access (C++ only — templates) ===
    // These are template methods on the interface, dispatching to the
    // underlying EnTT registry. C++ systems use these directly.
    template<typename T, typename... Args>
    T& emplace(Entity entity, Args&&... args);

    template<typename T>
    void remove(Entity entity);

    template<typename T>
    T& get(Entity entity);

    template<typename T>
    T* tryGet(Entity entity);

    template<typename T>
    bool has(Entity entity) const;

    template<typename... Ts>
    auto view();

    template<typename... Ts>
    auto group();

    // === Type-Erased Component Access (Lua bridge) ===
    // For scripting languages that can't use templates.
    virtual Result<void> addComponentByName(
        Entity entity, std::string_view typeName,
        const ComponentData& data) = 0;
    virtual Result<ComponentData> getComponentByName(
        Entity entity, std::string_view typeName) const = 0;
    virtual Result<void> setComponentByName(
        Entity entity, std::string_view typeName,
        const ComponentData& data) = 0;
    virtual Result<void> removeComponentByName(
        Entity entity, std::string_view typeName) = 0;
    virtual bool hasComponentByName(
        Entity entity, std::string_view typeName) const = 0;

    // === Component Type Registration ===
    virtual Result<void> registerComponentType(
        std::string_view name, ComponentTypeInfo info) = 0;
    virtual std::vector<std::string> getRegisteredTypes() const = 0;
    virtual std::optional<ComponentTypeInfo> getTypeInfo(
        std::string_view name) const = 0;

    // === Batch Operations ===
    virtual std::vector<Entity> createEntities(std::size_t count) = 0;
    virtual void destroyEntities(std::span<const Entity> entities) = 0;

    // === Queries ===
    virtual std::vector<Entity> query(
        std::span<const std::string> withComponents,
        std::span<const std::string> withoutComponents = {}) const = 0;
    virtual std::size_t countWith(
        std::span<const std::string> components) const = 0;
    virtual std::optional<Entity> findFirst(
        std::span<const std::string> components) const = 0;

    // === Hierarchy ===
    virtual Result<void> setParent(Entity child, Entity parent) = 0;
    virtual Result<void> removeParent(Entity child) = 0;
    virtual std::optional<Entity> getParent(Entity entity) const = 0;
    virtual std::vector<Entity> getChildren(Entity entity) const = 0;
    virtual bool isDescendantOf(Entity entity, Entity ancestor) const = 0;

    // === Tags ===
    virtual Result<void> addTag(Entity entity, std::string_view tag) = 0;
    virtual Result<void> removeTag(Entity entity, std::string_view tag) = 0;
    virtual bool hasTag(Entity entity, std::string_view tag) const = 0;
    virtual std::vector<Entity> findByTag(std::string_view tag) const = 0;

    // === Names (for debugging and Lua access) ===
    virtual Result<void> setName(Entity entity, std::string_view name) = 0;
    virtual std::optional<std::string_view> getName(Entity entity) const = 0;
    virtual std::optional<Entity> findByName(std::string_view name) const = 0;

    // === Serialization ===
    virtual Result<std::string> serializeEntity(Entity entity) const = 0;
    virtual Result<Entity> deserializeEntity(std::string_view data) = 0;

    // === Change Detection ===
    virtual bool wasModified(Entity entity) const = 0;
    virtual void clearModifiedFlags() = 0;
};

// ComponentData: type-erased component storage for Lua bridge
struct ComponentData {
    std::unordered_map<std::string, std::variant<
        float, int, bool, std::string,
        Vec2, Vec3, Vec4, Quat,
        Color, Entity
    >> fields;
};

struct ComponentTypeInfo {
    std::string name;
    std::vector<std::pair<std::string, std::string>> fields;  // name, type
    std::size_t sizeBytes = 0;
    std::function<void(Entity, const ComponentData&)> emplaceFromData;
    std::function<ComponentData(Entity)> extractToData;
};
```

### 5.2 IEventCore

Events are simple enough that a single contract suffices. No Core/System split.

```cpp
class IEventCore {
public:
    virtual ~IEventCore() = default;

    // Immediate dispatch
    virtual void publish(std::string_view type, const EventData& data) = 0;

    // Deferred dispatch (processed on next processQueue())
    virtual void queue(std::string_view type, const EventData& data) = 0;
    virtual void processQueue() = 0;
    virtual void clearQueue() = 0;
    virtual std::size_t queueSize() const = 0;

    // Subscriptions
    virtual SubscriptionId subscribe(std::string_view type,
        std::function<void(const EventData&)> callback) = 0;
    virtual SubscriptionId subscribePriority(std::string_view type,
        int priority,
        std::function<void(const EventData&)> callback) = 0;
    virtual void unsubscribe(SubscriptionId id) = 0;
    virtual void unsubscribeAll(std::string_view type) = 0;

    // Queries
    virtual std::size_t subscriberCount(std::string_view type) const = 0;
    virtual std::vector<std::string> getRegisteredTypes() const = 0;
};

// EventData: typed variant instead of std::any
using EventData = std::variant<
    // Physics
    CollisionEvent, TriggerEvent,
    CollisionEvent3D, TriggerEvent3D,
    // Input
    ActionEventData, PhaseEventData,
    // Asset
    AssetChangedEvent,
    // Scene
    SceneChangedEvent,
    // Entity
    EntityCreatedEvent, EntityDestroyedEvent,
    // Custom (Lua-originated events use this)
    CustomEvent
>;

struct CustomEvent {
    std::string name;
    std::unordered_map<std::string, SceneParam> data;
};
```

### 5.3 IInputCore

```cpp
class IInputCore {
public:
    virtual ~IInputCore() = default;

    // === Lifecycle ===
    virtual Result<void> initialize(void* nativeWindow) = 0;
    virtual void shutdown() = 0;
    virtual void update() = 0;

    // === Phase Management ===
    virtual std::string getCurrentPhase() const = 0;
    virtual std::vector<std::string> getPhaseStack() const = 0;
    virtual void pushPhase(std::string_view phase) = 0;
    virtual void popPhase() = 0;
    virtual void changePhase(std::string_view phase) = 0;
    virtual bool isPhaseActive(std::string_view phase) const = 0;

    // === Action Registration ===
    virtual void registerAction(const ActionRegistration& reg) = 0;
    virtual void unregisterAction(std::string_view name) = 0;
    virtual void unregisterPhaseActions(std::string_view phase) = 0;
    virtual void clearActions() = 0;
    virtual std::vector<ActionRegistration> getActions() const = 0;

    // === Input State Queries ===
    virtual InputState getInputState(const InputBinding& binding) const = 0;
    virtual float getHoldDuration(const InputBinding& binding) const = 0;
    virtual void setDefaultHoldThreshold(float seconds) = 0;
    virtual float getDefaultHoldThreshold() const = 0;

    // === Configuration ===
    virtual Result<void> loadConfig(std::string_view path) = 0;
    virtual Result<void> reloadConfig() = 0;

    // === Raw Input Capture (for rebinding UI) ===
    virtual std::optional<InputBinding> getLastInput() const = 0;
    virtual bool isListeningForInput() const = 0;
    virtual void startListeningForInput() = 0;
    virtual void stopListeningForInput() = 0;

    // === Keyboard ===
    virtual bool isKeyDown(KeyCode key) const = 0;
    virtual bool wasKeyJustPressed(KeyCode key) const = 0;
    virtual bool wasKeyJustReleased(KeyCode key) const = 0;

    // === Mouse ===
    virtual Vec2 getMousePosition() const = 0;
    virtual Vec2 getMouseDelta() const = 0;
    virtual bool isMouseButtonDown(MouseButton button) const = 0;
    virtual bool wasMouseButtonJustPressed(MouseButton button) const = 0;
    virtual bool wasMouseButtonJustReleased(MouseButton button) const = 0;
    virtual Vec2 getScrollDelta() const = 0;

    // === Modifiers ===
    virtual ModifierKey getModifierState() const = 0;
    virtual bool isShiftPressed() const = 0;
    virtual bool isCtrlPressed() const = 0;
    virtual bool isAltPressed() const = 0;
    virtual bool isSuperPressed() const = 0;

    // === Gamepad ===
    virtual bool isGamepadButtonDown(GamepadButton btn, int index = 0) const = 0;
    virtual bool wasGamepadButtonJustPressed(GamepadButton btn, int index = 0) const = 0;
    virtual bool wasGamepadButtonJustReleased(GamepadButton btn, int index = 0) const = 0;
    virtual float getGamepadAxisValue(GamepadAxis axis, int index = 0) const = 0;
    virtual Vec2 getLeftStick(int index = 0) const = 0;
    virtual Vec2 getRightStick(int index = 0) const = 0;

    // === Controller Info ===
    virtual int getConnectedControllerCount() const = 0;
    virtual bool isControllerConnected(int index) const = 0;
    virtual std::string getControllerName(int index) const = 0;

    // === Text Input ===
    virtual void enableTextInput() = 0;
    virtual void disableTextInput() = 0;
    virtual bool isTextInputEnabled() const = 0;
    virtual std::string getTextInput() const = 0;
    virtual void clearTextInput() = 0;

    // === Cursor ===
    virtual void showMouseCursor() = 0;
    virtual void hideMouseCursor() = 0;
    virtual bool isMouseCursorVisible() const = 0;
    virtual void setCursorMode(CursorMode mode) = 0;
    virtual CursorMode getCursorMode() const = 0;

    // === Deadzone Configuration ===
    virtual void setStickDeadzone(float deadzone) = 0;
    virtual float getStickDeadzone() const = 0;
    virtual void setTriggerDeadzone(float deadzone) = 0;
    virtual float getTriggerDeadzone() const = 0;

    // === Haptics ===
    virtual Result<void> setGamepadVibration(
        int index, float leftMotor, float rightMotor, float duration = 0) = 0;
    virtual void stopGamepadVibration(int index) = 0;
};
```

### 5.4 IAudioCore

```cpp
class IAudioCore {
public:
    virtual ~IAudioCore() = default;

    // === Lifecycle ===
    virtual Result<void> initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update(DeltaTime dt) = 0;

    // === Channel-Based Playback ===
    virtual Result<void> playOnChannel(ChannelHandle channel,
        AssetHandle sound, float volume = 1.0f, bool loop = false) = 0;
    virtual Result<void> stopChannel(ChannelHandle channel, float fadeOut = 0) = 0;
    virtual Result<void> pauseChannel(ChannelHandle channel) = 0;
    virtual Result<void> resumeChannel(ChannelHandle channel) = 0;
    virtual Result<void> setChannelVolume(ChannelHandle channel, float volume) = 0;
    virtual Result<void> setChannelPitch(ChannelHandle channel, float pitch) = 0;
    virtual Result<void> setChannelPan(ChannelHandle channel, float pan) = 0;
    virtual Result<void> seekChannel(ChannelHandle channel, float position) = 0;
    virtual float getChannelPosition(ChannelHandle channel) const = 0;
    virtual float getChannelDuration(ChannelHandle channel) const = 0;
    virtual bool isChannelPlaying(ChannelHandle channel) const = 0;
    virtual bool isChannelPaused(ChannelHandle channel) const = 0;

    // === Fire-and-Forget Playback ===
    virtual Result<SoundHandle> playOneShot(
        AssetHandle sound, float volume = 1.0f) = 0;
    virtual Result<void> stopOneShot(SoundHandle handle) = 0;

    // === Positional Audio ===
    virtual Result<SoundHandle> playPositional(
        AssetHandle sound, Vec3 position,
        float minDist = 1.0f, float maxDist = 100.0f,
        float volume = 1.0f) = 0;
    virtual Result<void> updatePositionalPosition(SoundHandle handle, Vec3 pos) = 0;
    virtual bool isPositionalPlaying(SoundHandle handle) const = 0;

    // === 3D Listener ===
    virtual void setListenerPosition(Vec3 position) = 0;
    virtual void setListenerOrientation(Vec3 forward, Vec3 up) = 0;
    virtual void setListenerVelocity(Vec3 velocity) = 0;
    virtual void setDopplerScale(float scale) = 0;
    virtual void setDistanceModel(DistanceModel model) = 0;

    // === Volume Control ===
    virtual void setMasterVolume(float volume) = 0;
    virtual float getMasterVolume() const = 0;

    // === Channel Groups ===
    virtual Result<void> createGroup(std::string_view name) = 0;
    virtual Result<void> destroyGroup(std::string_view name) = 0;
    virtual Result<void> setGroupVolume(std::string_view name, float volume) = 0;
    virtual float getGroupVolume(std::string_view name) const = 0;
    virtual Result<void> assignChannelToGroup(
        ChannelHandle channel, std::string_view group) = 0;
    virtual Result<void> muteGroup(std::string_view name) = 0;
    virtual Result<void> unmuteGroup(std::string_view name) = 0;

    // === Global Controls ===
    virtual void pauseAll() = 0;
    virtual void resumeAll() = 0;
    virtual void stopAll(float fadeOut = 0) = 0;

    // === DSP Effects ===
    virtual Result<void> addChannelEffect(
        ChannelHandle channel, AudioEffect effect) = 0;
    virtual Result<void> removeChannelEffects(ChannelHandle channel) = 0;
    virtual Result<void> addGroupEffect(
        std::string_view group, AudioEffect effect) = 0;

    // === Bus Routing ===
    virtual Result<void> setChannelOutput(
        ChannelHandle channel, std::string_view busName) = 0;
    virtual Result<void> setGroupOutput(
        std::string_view group, std::string_view busName) = 0;

    // === Fade ===
    virtual Result<void> fadeChannel(
        ChannelHandle channel, float targetVolume, float duration) = 0;
    virtual Result<void> crossfade(
        ChannelHandle from, ChannelHandle to, float duration) = 0;

    // === Queries ===
    virtual int getActiveChannelCount() const = 0;
    virtual int getActiveSoundCount() const = 0;
    virtual float getCPUUsage() const = 0;
};

enum class DistanceModel : std::uint8_t {
    Linear, InverseDistance, InverseDistanceClamped
};

struct AudioEffect {
    enum class Type : std::uint8_t {
        Reverb, Echo, Chorus, Flanger, Distortion,
        LowPass, HighPass, BandPass, Compressor, Limiter
    };
    Type type;
    std::unordered_map<std::string, float> params; // "wetDry", "decay", etc.
};
```

### 5.5 IPhysics2DCore

```cpp
class IPhysics2DCore {
public:
    virtual ~IPhysics2DCore() = default;

    // === Lifecycle ===
    virtual Result<void> initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update(DeltaTime fixedDt) = 0;  // Called in FixedUpdate phase

    // === Body Management ===
    virtual Result<BodyHandle> createBody(Entity entity, const BodyDef2D& def) = 0;
    virtual Result<void> destroyBody(BodyHandle handle) = 0;
    virtual bool bodyExists(BodyHandle handle) const = 0;
    virtual std::optional<BodyHandle> getBody(Entity entity) const = 0;

    // === Body Properties ===
    virtual Result<void> setBodyType(BodyHandle h, BodyType type) = 0;
    virtual BodyType getBodyType(BodyHandle h) const = 0;
    virtual Result<void> setPosition(BodyHandle h, Vec2 pos) = 0;
    virtual Vec2 getPosition(BodyHandle h) const = 0;
    virtual Result<void> setRotation(BodyHandle h, float radians) = 0;
    virtual float getRotation(BodyHandle h) const = 0;
    virtual Result<void> setVelocity(BodyHandle h, Vec2 vel) = 0;
    virtual Vec2 getVelocity(BodyHandle h) const = 0;
    virtual Result<void> setAngularVelocity(BodyHandle h, float omega) = 0;
    virtual float getAngularVelocity(BodyHandle h) const = 0;
    virtual Result<void> setGravityScale(BodyHandle h, float scale) = 0;
    virtual float getMass(BodyHandle h) const = 0;
    virtual Vec2 getBodySize(BodyHandle h) const = 0;

    // === Shape Management ===
    virtual Result<void> addBoxShape(BodyHandle h,
        Vec2 halfExtents, Vec2 offset = {}, float density = 1.0f) = 0;
    virtual Result<void> addCircleShape(BodyHandle h,
        float radius, Vec2 offset = {}, float density = 1.0f) = 0;
    virtual Result<void> addPolygonShape(BodyHandle h,
        std::span<const Vec2> vertices, float density = 1.0f) = 0;
    virtual Result<void> addEdgeShape(BodyHandle h,
        Vec2 start, Vec2 end) = 0;
    virtual Result<void> addChainShape(BodyHandle h,
        std::span<const Vec2> vertices, bool loop = false) = 0;

    // === Material Properties ===
    virtual Result<void> setFriction(BodyHandle h, float friction) = 0;
    virtual Result<void> setRestitution(BodyHandle h, float restitution) = 0;
    virtual Result<void> setDensity(BodyHandle h, float density) = 0;

    // === Forces ===
    virtual Result<void> applyForce(BodyHandle h, Vec2 force, Vec2 point) = 0;
    virtual Result<void> applyForceToCenter(BodyHandle h, Vec2 force) = 0;
    virtual Result<void> applyImpulse(BodyHandle h, Vec2 impulse, Vec2 point) = 0;
    virtual Result<void> applyImpulseToCenter(BodyHandle h, Vec2 impulse) = 0;
    virtual Result<void> applyTorque(BodyHandle h, float torque) = 0;

    // === Collision Filtering ===
    virtual Result<void> setCollisionLayer(BodyHandle h, std::uint16_t layer) = 0;
    virtual Result<void> setCollisionMask(BodyHandle h, std::uint16_t mask) = 0;
    virtual Result<void> setSensor(BodyHandle h, bool isSensor) = 0;
    virtual bool isSensor(BodyHandle h) const = 0;

    // === Constraints ===
    virtual Result<ConstraintHandle> createDistanceJoint(
        BodyHandle a, BodyHandle b, Vec2 anchorA, Vec2 anchorB) = 0;
    virtual Result<ConstraintHandle> createRevoluteJoint(
        BodyHandle a, BodyHandle b, Vec2 anchor,
        bool enableLimits = false, float lower = 0, float upper = 0) = 0;
    virtual Result<ConstraintHandle> createPrismaticJoint(
        BodyHandle a, BodyHandle b, Vec2 anchor, Vec2 axis) = 0;
    virtual Result<ConstraintHandle> createWeldJoint(
        BodyHandle a, BodyHandle b, Vec2 anchor) = 0;
    virtual Result<void> destroyConstraint(ConstraintHandle handle) = 0;

    // === Queries ===
    virtual std::vector<BodyHandle> queryAABB(Vec2 min, Vec2 max) const = 0;
    virtual std::vector<BodyHandle> queryCircle(Vec2 center, float radius) const = 0;
    virtual std::optional<RaycastHit2D> raycast(
        Vec2 origin, Vec2 direction, float maxDist,
        std::uint16_t mask = 0xFFFF) const = 0;
    virtual std::vector<RaycastHit2D> raycastAll(
        Vec2 origin, Vec2 direction, float maxDist,
        std::uint16_t mask = 0xFFFF) const = 0;

    // === Ground Detection ===
    virtual GroundCheckResult checkGrounded(BodyHandle h,
        const GroundCheckParams& params = {}) const = 0;

    // === World Config ===
    virtual void setGravity(Vec2 gravity) = 0;
    virtual Vec2 getGravity() const = 0;
    virtual void setTimeScale(float scale) = 0;
    virtual float getTimeScale() const = 0;

    // === Callbacks ===
    virtual SubscriptionId onCollision(
        std::function<void(const CollisionEvent&)> cb) = 0;
    virtual SubscriptionId onTrigger(
        std::function<void(const TriggerEvent&)> cb) = 0;
    virtual void unsubscribe(SubscriptionId id) = 0;

    // === Debug ===
    virtual void setDebugDraw(bool enabled) = 0;
    virtual bool isDebugDrawEnabled() const = 0;
};

struct BodyDef2D {
    BodyType type = BodyType::Static;
    Vec2 position;
    float rotation = 0;
    bool fixedRotation = false;
    float linearDamping = 0;
    float angularDamping = 0;
    float gravityScale = 1.0f;
    bool bullet = false;  // Continuous collision detection

    // Default shape (convenience — or add shapes after creation)
    struct BoxShape { Vec2 halfExtents; float density = 1.0f; };
    struct CircleShape { float radius; float density = 1.0f; };
    std::optional<std::variant<BoxShape, CircleShape>> shape;
};

struct RaycastHit2D {
    BodyHandle body;
    Entity entity;
    Vec2 point;
    Vec2 normal;
    float fraction;
};
```

### 5.6 IPhysics3DCore

```cpp
class IPhysics3DCore {
public:
    virtual ~IPhysics3DCore() = default;

    // === Lifecycle ===
    virtual Result<void> initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update(DeltaTime fixedDt) = 0;

    // === Body Management ===
    virtual Result<BodyHandle> createBody(Entity entity, const BodyDef3D& def) = 0;
    virtual Result<void> destroyBody(BodyHandle handle) = 0;
    virtual bool bodyExists(BodyHandle handle) const = 0;
    virtual std::optional<BodyHandle> getBody(Entity entity) const = 0;

    // === Transform ===
    virtual Result<void> setPosition(BodyHandle h, Vec3 pos) = 0;
    virtual Vec3 getPosition(BodyHandle h) const = 0;
    virtual Result<void> setRotation(BodyHandle h, Quat rot) = 0;
    virtual Quat getRotation(BodyHandle h) const = 0;
    virtual Result<void> setTransform(BodyHandle h, Vec3 pos, Quat rot) = 0;

    // === Velocity ===
    virtual Result<void> setLinearVelocity(BodyHandle h, Vec3 vel) = 0;
    virtual Vec3 getLinearVelocity(BodyHandle h) const = 0;
    virtual Result<void> setAngularVelocity(BodyHandle h, Vec3 omega) = 0;
    virtual Vec3 getAngularVelocity(BodyHandle h) const = 0;

    // === Forces ===
    virtual Result<void> applyForce(BodyHandle h, Vec3 force) = 0;
    virtual Result<void> applyForceAtPoint(BodyHandle h, Vec3 force, Vec3 point) = 0;
    virtual Result<void> applyImpulse(BodyHandle h, Vec3 impulse) = 0;
    virtual Result<void> applyImpulseAtPoint(BodyHandle h, Vec3 impulse, Vec3 point) = 0;
    virtual Result<void> applyTorque(BodyHandle h, Vec3 torque) = 0;

    // === Shape Types ===
    virtual Result<void> addBoxShape(BodyHandle h, Vec3 halfExtents,
        Vec3 offset = {}, Quat rotation = {}, float density = 1.0f) = 0;
    virtual Result<void> addSphereShape(BodyHandle h, float radius,
        Vec3 offset = {}, float density = 1.0f) = 0;
    virtual Result<void> addCapsuleShape(BodyHandle h,
        float halfHeight, float radius,
        Vec3 offset = {}, float density = 1.0f) = 0;
    virtual Result<void> addCylinderShape(BodyHandle h,
        float halfHeight, float radius, float density = 1.0f) = 0;
    virtual Result<void> addConvexHullShape(BodyHandle h,
        std::span<const Vec3> vertices, float density = 1.0f) = 0;
    virtual Result<void> addMeshShape(BodyHandle h,
        std::span<const Vec3> vertices,
        std::span<const std::uint32_t> indices) = 0;
    virtual Result<void> addHeightFieldShape(BodyHandle h,
        int width, int height, std::span<const float> heights,
        Vec3 scale = {1,1,1}) = 0;

    // === Material Properties ===
    virtual Result<void> setFriction(BodyHandle h, float friction) = 0;
    virtual Result<void> setRestitution(BodyHandle h, float restitution) = 0;

    // === Collision Filtering ===
    virtual Result<void> setCollisionLayer(BodyHandle h, std::uint16_t layer) = 0;
    virtual Result<void> setCollisionMask(BodyHandle h, std::uint16_t mask) = 0;
    virtual Result<void> setSensor(BodyHandle h, bool isSensor) = 0;

    // === Constraints ===
    virtual Result<ConstraintHandle> createFixedConstraint(
        BodyHandle a, BodyHandle b, Vec3 anchor) = 0;
    virtual Result<ConstraintHandle> createHingeConstraint(
        BodyHandle a, BodyHandle b, Vec3 anchor, Vec3 axis,
        float minAngle = 0, float maxAngle = 0) = 0;
    virtual Result<ConstraintHandle> createSliderConstraint(
        BodyHandle a, BodyHandle b, Vec3 axis,
        float minDist = 0, float maxDist = 0) = 0;
    virtual Result<ConstraintHandle> createBallSocketConstraint(
        BodyHandle a, BodyHandle b, Vec3 pivotA, Vec3 pivotB) = 0;
    virtual Result<ConstraintHandle> createConeConstraint(
        BodyHandle a, BodyHandle b, Vec3 anchor, Vec3 axis,
        float halfAngle) = 0;
    virtual Result<ConstraintHandle> createSixDOFConstraint(
        BodyHandle a, BodyHandle b,
        Vec3 linearMin, Vec3 linearMax,
        Vec3 angularMin, Vec3 angularMax) = 0;
    virtual Result<void> setConstraintMotor(
        ConstraintHandle h, float targetVelocity, float maxForce) = 0;
    virtual Result<void> destroyConstraint(ConstraintHandle h) = 0;

    // === Character Controller ===
    virtual Result<BodyHandle> createCharacterController(
        Entity entity, const CharacterDef3D& def) = 0;
    virtual Result<void> moveCharacter(BodyHandle h, Vec3 displacement,
        DeltaTime dt) = 0;
    virtual bool isCharacterGrounded(BodyHandle h) const = 0;
    virtual Vec3 getCharacterGroundNormal(BodyHandle h) const = 0;
    virtual CharacterGroundState getCharacterGroundState(BodyHandle h) const = 0;

    // === Queries ===
    virtual std::optional<RaycastHit3D> raycast(
        Vec3 origin, Vec3 direction, float maxDist,
        std::uint16_t mask = 0xFFFF) const = 0;
    virtual std::vector<RaycastHit3D> raycastAll(
        Vec3 origin, Vec3 direction, float maxDist,
        std::uint16_t mask = 0xFFFF) const = 0;
    virtual std::optional<RaycastHit3D> sphereCast(
        Vec3 origin, Vec3 direction, float radius, float maxDist,
        std::uint16_t mask = 0xFFFF) const = 0;
    virtual std::vector<BodyHandle> overlapSphere(
        Vec3 center, float radius,
        std::uint16_t mask = 0xFFFF) const = 0;
    virtual std::vector<BodyHandle> overlapBox(
        Vec3 center, Vec3 halfExtents, Quat rotation = {},
        std::uint16_t mask = 0xFFFF) const = 0;

    // === World Config ===
    virtual void setGravity(Vec3 gravity) = 0;
    virtual Vec3 getGravity() const = 0;
    virtual void setTimeScale(float scale) = 0;

    // === Callbacks ===
    virtual SubscriptionId onCollision(
        std::function<void(const CollisionEvent3D&)> cb) = 0;
    virtual SubscriptionId onTrigger(
        std::function<void(const TriggerEvent3D&)> cb) = 0;
    virtual void unsubscribe(SubscriptionId id) = 0;

    // === Debug ===
    virtual void setDebugDraw(bool enabled) = 0;
};

struct CharacterDef3D {
    float height = 1.8f;
    float radius = 0.3f;
    float mass = 80.0f;
    float maxSlopeAngle = 50.0f;   // degrees
    float stepHeight = 0.35f;
    Vec3 position;
};

enum class CharacterGroundState : std::uint8_t {
    OnGround, OnSteepGround, InAir, NotSupported
};
```


### 5.7 IGraphics2DCore

```cpp
class IGraphics2DCore : public IGraphicsContextCore {
public:
    virtual ~IGraphics2DCore() = default;

    // === Frame Lifecycle ===
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;

    // === Sprite Rendering ===
    virtual void draw(const Sprite& sprite) = 0;
    virtual void drawBatch(std::span<const Sprite> sprites) = 0;
    virtual void drawSpriteSheet(AssetHandle sheet, int frameIndex,
        const Transform2D& transform, Color tint = Color::white()) = 0;

    // === Primitive Rendering ===
    virtual void drawRect(Vec2 position, Vec2 size, const Color& color,
        bool filled = true) = 0;
    virtual void drawLine(Vec2 from, Vec2 to, const Color& color,
        float thickness = 1.0f) = 0;
    virtual void drawCircle(Vec2 center, float radius, const Color& color,
        bool filled = true, int segments = 32) = 0;
    virtual void drawPolygon(std::span<const Vec2> vertices,
        const Color& color, bool filled = true) = 0;
    virtual void drawArc(Vec2 center, float radius,
        float startAngle, float endAngle, const Color& color,
        int segments = 32) = 0;

    // === Text ===
    virtual void drawText(std::string_view text, Vec2 position,
        FontHandle font, float size,
        const Color& color = Color::white()) = 0;
    virtual void drawTextCentered(std::string_view text, Vec2 position,
        FontHandle font, float size,
        const Color& color = Color::white()) = 0;
    virtual Vec2 measureText(std::string_view text,
        FontHandle font, float size) const = 0;

    // === Camera ===
    virtual void setCamera(const Camera2D& camera) = 0;
    virtual Camera2D getCamera() const = 0;
    virtual Vec2 worldToScreen(Vec2 worldPos) const = 0;
    virtual Vec2 screenToWorld(Vec2 screenPos) const = 0;

    // === Window ===
    virtual void setWindowSize(Size size) = 0;
    virtual WindowMode getWindowMode() const = 0;
    virtual void setWindowMode(WindowMode mode) = 0;
    virtual bool shouldClose() const = 0;
    virtual void setWindowTitle(std::string_view title) = 0;

    // === Render State ===
    virtual void setClearColor(const Color& color) = 0;
    virtual void setVSync(bool enabled) = 0;
    virtual void setBlendMode(BlendMode mode) = 0;

    // === Auto Entity Rendering ===
    virtual void renderEntities(IEntityCore& entities) = 0;
    virtual void renderEntities(IEntityCore& entities,
        int minLayer, int maxLayer) = 0;
    virtual void setViewportCulling(bool enabled) = 0;

    // === Render Targets ===
    virtual Result<TextureHandle> createRenderTarget(
        int width, int height) = 0;
    virtual Result<void> setRenderTarget(TextureHandle target) = 0;
    virtual void resetRenderTarget() = 0;

    // === Screenshots ===
    virtual Result<void> captureScreenshot(std::string_view path) = 0;
};
```

### 5.8 IGraphics3DCore

Split from the current god interface into focused sub-contracts.

```cpp
// === Main 3D Graphics Contract ===
class IGraphics3DCore : public IGraphicsContextCore {
public:
    virtual ~IGraphics3DCore() = default;

    // === Lifecycle ===
    virtual Result<void> initialize(const Graphics3DConfig& config) = 0;
    virtual void shutdown() = 0;
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;

    // === Mesh Management ===
    virtual Result<MeshHandle> createMesh(const MeshDef& def) = 0;
    virtual Result<void> destroyMesh(MeshHandle handle) = 0;
    virtual Result<MeshHandle> createPrimitiveMesh(PrimitiveType type,
        const PrimitiveParams& params = {}) = 0;

    // === Material Management ===
    virtual Result<MaterialHandle> createPBRMaterial(
        const PBRMaterialDef& def) = 0;
    virtual Result<MaterialHandle> createUnlitMaterial(
        const UnlitMaterialDef& def) = 0;
    virtual Result<void> destroyMaterial(MaterialHandle handle) = 0;
    virtual Result<void> setMaterialUniform(
        MaterialHandle handle, std::string_view name,
        const UniformValue& value) = 0;
    virtual Result<void> setMaterialTexture(
        MaterialHandle handle, std::string_view slot,
        TextureHandle texture) = 0;
    virtual MaterialHandle getDefaultMaterial() const = 0;
    virtual MaterialHandle getErrorMaterial() const = 0;

    // === Texture Management ===
    virtual Result<TextureHandle> createTexture(
        const TextureData& data) = 0;
    virtual Result<TextureHandle> createTextureFromAsset(
        AssetHandle asset) = 0;
    virtual Result<void> destroyTexture(TextureHandle handle) = 0;

    // === Rendering ===
    virtual void drawMesh(MeshHandle mesh, MaterialHandle material,
        const Mat4& transform, bool castShadow = true) = 0;
    virtual void drawMeshInstanced(MeshHandle mesh, MaterialHandle material,
        std::span<const Mat4> transforms) = 0;
    virtual void drawSkinnedMesh(MeshHandle mesh, MaterialHandle material,
        const Mat4& transform, std::span<const Mat4> boneMatrices) = 0;
    virtual void renderEntities(IEntityCore& entities) = 0;

    // === Lighting ===
    virtual void setDirectionalLight(const DirectionalLight& light) = 0;
    virtual Result<std::uint32_t> addPointLight(
        const PointLight& light) = 0;
    virtual Result<std::uint32_t> addSpotLight(
        const SpotLight& light) = 0;
    virtual Result<void> removeLight(std::uint32_t lightId) = 0;
    virtual void clearLights() = 0;
    virtual void setAmbientLight(Vec3 color, float intensity = 1.0f) = 0;

    // === Camera ===
    virtual void setCamera(const Camera3D& camera) = 0;
    virtual Camera3D getCamera() const = 0;
    virtual Vec3 screenToWorldRay(Vec2 screenPos) const = 0;
    virtual Vec2 worldToScreen(Vec3 worldPos) const = 0;

    // === Environment ===
    virtual Result<void> setSkybox(TextureHandle cubemap) = 0;
    virtual void clearSkybox() = 0;
    virtual void setFog(const FogDef& fog) = 0;
    virtual void clearFog() = 0;

    // === Window ===
    virtual void setWindowSize(Size size) = 0;
    virtual WindowMode getWindowMode() const = 0;
    virtual void setWindowMode(WindowMode mode) = 0;
    virtual bool shouldClose() const = 0;
    virtual void setWindowTitle(std::string_view title) = 0;
    virtual void setClearColor(const Color& color) = 0;
    virtual void setVSync(bool enabled) = 0;

    // === Render State ===
    virtual void setWireframe(bool enabled) = 0;
    virtual void setFaceCulling(CullMode mode) = 0;

    // === LOD ===
    virtual Result<void> setMeshLODs(MeshHandle baseMesh,
        std::span<const MeshLOD> lods) = 0;
    virtual void setLODBias(float bias) = 0;
};

// === Post-Processing Contract (split from god interface) ===
class IPostProcessingCore {
public:
    virtual ~IPostProcessingCore() = default;

    virtual void setToneMapping(bool enabled) = 0;
    virtual void setToneMappingOperator(ToneMappingOp op) = 0;
    virtual void setExposure(float exposure) = 0;
    virtual void setBloom(bool enabled, float threshold = 1.0f,
        float intensity = 1.0f) = 0;
    virtual void setSSAO(bool enabled, float radius = 0.5f,
        float intensity = 1.0f, int samples = 16) = 0;
    virtual void setDepthOfField(bool enabled, float focusDist = 10.0f,
        float aperture = 5.6f) = 0;
    virtual void setMotionBlur(bool enabled, float scale = 1.0f) = 0;
    virtual void setAntiAliasing(AntiAliasingMode mode) = 0;
    virtual void setColorGrading(const ColorGradingParams& params) = 0;
    virtual void setVignette(bool enabled, float intensity = 0.5f) = 0;
    virtual void setChromaticAberration(bool enabled, float intensity = 0.5f) = 0;
};

// === Shadow Rendering Contract (split from god interface) ===
class IShadowCore {
public:
    virtual ~IShadowCore() = default;

    virtual void setShadowsEnabled(bool enabled) = 0;
    virtual bool areShadowsEnabled() const = 0;
    virtual void setDirectionalShadowResolution(int resolution) = 0;
    virtual void setShadowDistance(float distance) = 0;
    virtual void setShadowCascadeCount(int count) = 0;
    virtual void setShadowBias(float depthBias, float normalBias) = 0;
    virtual void setPointShadowResolution(int resolution) = 0;
    virtual void setSoftShadows(bool enabled) = 0;
};

// === Debug Rendering Contract (split from god interface) ===
class IDebugRendererCore {
public:
    virtual ~IDebugRendererCore() = default;

    virtual void drawLine(Vec3 from, Vec3 to,
        Color color = Color::white(), float duration = 0) = 0;
    virtual void drawBox(Vec3 center, Vec3 halfExtents,
        Quat rotation = {}, Color color = Color::white(),
        float duration = 0) = 0;
    virtual void drawSphere(Vec3 center, float radius,
        Color color = Color::white(), int segments = 16,
        float duration = 0) = 0;
    virtual void drawCapsule(Vec3 center, float halfHeight, float radius,
        Quat rotation = {}, Color color = Color::white(),
        float duration = 0) = 0;
    virtual void drawFrustum(const Mat4& viewProjection,
        Color color = Color::white(), float duration = 0) = 0;
    virtual void drawRay(Vec3 origin, Vec3 direction, float length = 10,
        Color color = Color::white(), float duration = 0) = 0;
    virtual void drawAxes(Vec3 origin, Quat rotation = {},
        float size = 1.0f, float duration = 0) = 0;
    virtual void drawAABB(Vec3 min, Vec3 max,
        Color color = Color::white(), float duration = 0) = 0;
    virtual void drawGrid(Vec3 origin, int gridSize = 10,
        float cellSize = 1.0f, Color color = {0.5f, 0.5f, 0.5f, 0.5f}) = 0;
    virtual void drawText(Vec3 position, std::string_view text,
        Color color = Color::white(), float duration = 0) = 0;
    virtual void setEnabled(bool enabled) = 0;
    virtual bool isEnabled() const = 0;
    virtual void clear() = 0;
};

// === 3D Text Rendering Contract (split from god interface) ===
class IText3DCore {
public:
    virtual ~IText3DCore() = default;

    virtual Result<FontHandle> loadFont(AssetHandle fontAsset) = 0;
    virtual void destroyFont(FontHandle handle) = 0;
    virtual void drawText(Vec3 position, std::string_view text,
        FontHandle font, float size,
        Color color = Color::white(), bool billboard = true) = 0;
    virtual AABB3D measureText(std::string_view text,
        FontHandle font, float size) const = 0;
};

// Supporting types
enum class PrimitiveType : std::uint8_t {
    Cube, Sphere, Cylinder, Capsule, Plane, Cone, Torus, Quad
};

struct PrimitiveParams {
    float size = 1.0f;
    int segments = 32;
    int rings = 16;
};

struct MeshLOD {
    MeshHandle mesh;
    float screenPercentage;  // Switch when mesh covers less than this % of screen
};

enum class ToneMappingOp : std::uint8_t {
    Reinhard, ACES, Filmic, Uncharted2, Linear
};

enum class AntiAliasingMode : std::uint8_t {
    None, FXAA, MSAA2x, MSAA4x, MSAA8x, TAA
};

struct ColorGradingParams {
    float contrast = 1.0f;
    float saturation = 1.0f;
    float brightness = 0.0f;
    Vec3 colorBalance = {1, 1, 1};  // RGB multipliers
    float gamma = 2.2f;
};
```

### 5.9 IAnimationCore

```cpp
class IAnimationCore {
public:
    virtual ~IAnimationCore() = default;

    // === Lifecycle ===
    virtual void update(DeltaTime dt) = 0;

    // === Skeleton Management ===
    virtual Result<SkeletonHandle> createSkeleton(
        const ModelData& model) = 0;
    virtual Result<void> destroySkeleton(SkeletonHandle handle) = 0;
    virtual int getBoneCount(SkeletonHandle skeleton) const = 0;
    virtual std::optional<int> findBone(
        SkeletonHandle skeleton, std::string_view name) const = 0;
    virtual std::string_view getBoneName(
        SkeletonHandle skeleton, int index) const = 0;

    // === Animation Clip Management ===
    virtual Result<AnimClipHandle> createClip(
        std::string_view name, const ModelData::Animation& anim,
        SkeletonHandle skeleton) = 0;
    virtual Result<AnimClipHandle> loadClip(
        std::string_view name, AssetHandle asset,
        SkeletonHandle skeleton) = 0;
    virtual Result<void> destroyClip(AnimClipHandle handle) = 0;
    virtual float getClipDuration(AnimClipHandle clip) const = 0;

    // === Animator Instances ===
    virtual Result<AnimatorHandle> createAnimator(
        Entity entity, SkeletonHandle skeleton) = 0;
    virtual Result<void> destroyAnimator(AnimatorHandle handle) = 0;
    virtual std::optional<AnimatorHandle> getAnimator(Entity entity) const = 0;

    // === Playback Control ===
    virtual Result<void> play(AnimatorHandle anim, AnimClipHandle clip,
        float blendTime = 0.25f, bool loop = true) = 0;
    virtual Result<void> stop(AnimatorHandle anim, float blendTime = 0) = 0;
    virtual Result<void> pause(AnimatorHandle anim) = 0;
    virtual Result<void> resume(AnimatorHandle anim) = 0;
    virtual Result<void> setPlaybackSpeed(AnimatorHandle anim, float speed) = 0;
    virtual Result<void> setTime(AnimatorHandle anim, float time) = 0;
    virtual float getTime(AnimatorHandle anim) const = 0;
    virtual float getNormalizedTime(AnimatorHandle anim) const = 0;
    virtual bool isPlaying(AnimatorHandle anim) const = 0;

    // === Blending ===
    virtual Result<void> crossfade(AnimatorHandle anim,
        AnimClipHandle toClip, float blendTime) = 0;
    virtual Result<void> setLayerWeight(AnimatorHandle anim,
        int layer, float weight) = 0;
    virtual Result<void> setLayerMask(AnimatorHandle anim,
        int layer, std::span<const int> boneIndices) = 0;
    virtual Result<void> setBlendTree(AnimatorHandle anim,
        const BlendTreeDef& tree) = 0;

    // === Sampling ===
    virtual std::vector<Mat4> samplePose(AnimatorHandle anim) const = 0;
    virtual std::vector<Mat4> sampleClip(
        AnimClipHandle clip, float time, bool loop = true) const = 0;
    virtual std::vector<Mat4> blendPoses(
        std::span<const Mat4> a, std::span<const Mat4> b,
        float weight) const = 0;

    // === IK Solvers ===
    virtual Result<void> setIKTarget(AnimatorHandle anim,
        std::string_view chainName, Vec3 target,
        float weight = 1.0f) = 0;
    virtual Result<void> clearIKTarget(AnimatorHandle anim,
        std::string_view chainName) = 0;
    virtual Result<void> addIKChain(AnimatorHandle anim,
        std::string_view name, int startBone, int endBone,
        int maxIterations = 10) = 0;

    // === Socket Attachments ===
    virtual Result<void> createSocket(AnimatorHandle anim,
        std::string_view socketName, int boneIndex,
        Vec3 offset = {}, Quat rotation = {}) = 0;
    virtual std::optional<Mat4> getSocketTransform(
        AnimatorHandle anim, std::string_view socketName) const = 0;

    // === Ragdoll ===
    virtual Result<void> enableRagdoll(AnimatorHandle anim) = 0;
    virtual Result<void> disableRagdoll(AnimatorHandle anim) = 0;
    virtual bool isRagdollActive(AnimatorHandle anim) const = 0;

    // === Events ===
    virtual SubscriptionId onAnimationEvent(
        std::function<void(AnimatorHandle, std::string_view eventName)> cb) = 0;
    virtual SubscriptionId onAnimationComplete(
        std::function<void(AnimatorHandle, AnimClipHandle)> cb) = 0;
    virtual void unsubscribe(SubscriptionId id) = 0;

    // === Animation Root Motion ===
    virtual Result<void> enableRootMotion(AnimatorHandle anim, bool enabled) = 0;
    virtual Vec3 getRootMotionDelta(AnimatorHandle anim) const = 0;
    virtual Quat getRootMotionRotationDelta(AnimatorHandle anim) const = 0;
};
```

### 5.10 ICameraCore

```cpp
class ICameraCore {
public:
    virtual ~ICameraCore() = default;

    // === 2D Camera ===
    virtual void setTarget(Entity target) = 0;
    virtual void clearTarget() = 0;
    virtual Entity getTarget() const = 0;
    virtual void setFollowSmoothing(float smoothing) = 0;
    virtual void setOffset(Vec2 offset) = 0;
    virtual void setDeadzone(Vec2 size) = 0;
    virtual void setBounds(float minX, float maxX, float minY, float maxY) = 0;
    virtual void clearBounds() = 0;
    virtual void shake(float intensity, float duration) = 0;
    virtual void stopShake() = 0;
    virtual void setZoom(float zoom) = 0;
    virtual float getZoom() const = 0;
    virtual void update(DeltaTime dt, Vec2 targetPosition) = 0;
    virtual Vec2 getPosition() const = 0;
    virtual Vec2 screenToWorld(Vec2 screenPos) const = 0;
    virtual Vec2 worldToScreen(Vec2 worldPos) const = 0;

    // === 3D Camera ===
    virtual void setPosition3D(Vec3 position) = 0;
    virtual Vec3 getPosition3D() const = 0;
    virtual void setRotation3D(Quat rotation) = 0;
    virtual Quat getRotation3D() const = 0;
    virtual void lookAt(Vec3 target, Vec3 up = {0,1,0}) = 0;
    virtual void setFOV(float fovDegrees) = 0;
    virtual float getFOV() const = 0;
    virtual void setNearFar(float near, float far) = 0;
    virtual void setProjection(ProjectionType type) = 0;

    // === Camera Modes ===
    virtual void setMode(CameraMode mode) = 0;
    virtual CameraMode getMode() const = 0;

    // === Orbit Camera ===
    virtual void orbit(float yawDelta, float pitchDelta) = 0;
    virtual void setOrbitTarget(Vec3 target) = 0;
    virtual void setOrbitDistance(float distance) = 0;
    virtual void setOrbitLimits(float minPitch, float maxPitch,
        float minDist, float maxDist) = 0;

    // === FPS Camera ===
    virtual void setFPSOffset(Vec3 eyeOffset) = 0;
    virtual void setMouseSensitivity(float sensitivity) = 0;
    virtual void setPitchLimits(float min, float max) = 0;

    // === Third Person Camera ===
    virtual void setThirdPersonOffset(Vec3 offset) = 0;
    virtual void setThirdPersonDistance(float distance) = 0;
    virtual void enableCollisionAvoidance(bool enabled) = 0;

    // === Effects ===
    virtual void shake3D(float intensity, float duration) = 0;
    virtual void flashFOV(float targetFOV, float duration) = 0;

    // === Matrices ===
    virtual Mat4 getViewMatrix() const = 0;
    virtual Mat4 getProjectionMatrix() const = 0;
    virtual Mat4 getViewProjectionMatrix() const = 0;
    virtual Frustum getFrustum() const = 0;
    virtual Ray3D screenToWorldRay(Vec2 screenPos) const = 0;
};

enum class CameraMode : std::uint8_t {
    Free, Follow2D, FPS, ThirdPerson, Orbit, Fixed, Cinematic
};

enum class ProjectionType : std::uint8_t {
    Perspective, Orthographic
};
```

### 5.11 IConfigCore

```cpp
class IConfigCore {
public:
    virtual ~IConfigCore() = default;

    // === Lifecycle ===
    virtual Result<void> initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update(DeltaTime dt) = 0;

    // === Loading ===
    virtual Result<void> loadConfig(std::string_view filePath) = 0;
    virtual Result<void> loadConfigAsset(AssetHandle asset) = 0;
    virtual Result<void> reloadAll() = 0;
    virtual Result<void> reloadConfig(std::string_view filePath) = 0;

    // === Type-Safe Access ===
    virtual std::optional<float> getFloat(std::string_view key) const = 0;
    virtual std::optional<int> getInt(std::string_view key) const = 0;
    virtual std::optional<bool> getBool(std::string_view key) const = 0;
    virtual std::optional<std::string> getString(std::string_view key) const = 0;
    virtual std::optional<Vec2> getVec2(std::string_view key) const = 0;
    virtual std::optional<Vec3> getVec3(std::string_view key) const = 0;
    virtual std::optional<Color> getColor(std::string_view key) const = 0;

    // === With Defaults ===
    virtual float getFloatOr(std::string_view key, float def) const = 0;
    virtual int getIntOr(std::string_view key, int def) const = 0;
    virtual bool getBoolOr(std::string_view key, bool def) const = 0;
    virtual std::string getStringOr(std::string_view key,
        std::string_view def) const = 0;

    // === Arrays ===
    virtual std::vector<int> getIntArray(std::string_view key) const = 0;
    virtual std::vector<float> getFloatArray(std::string_view key) const = 0;
    virtual std::vector<std::string> getStringArray(std::string_view key) const = 0;

    // === Tables (nested config access) ===
    virtual std::vector<std::string> getTableKeys(std::string_view prefix) const = 0;
    virtual bool isTable(std::string_view key) const = 0;

    // === Runtime Modification ===
    virtual void setFloat(std::string_view key, float value) = 0;
    virtual void setInt(std::string_view key, int value) = 0;
    virtual void setBool(std::string_view key, bool value) = 0;
    virtual void setString(std::string_view key, std::string_view value) = 0;

    // === Queries ===
    virtual bool hasKey(std::string_view key) const = 0;
    virtual std::vector<std::string> getKeysWithPrefix(
        std::string_view prefix) const = 0;
    virtual std::vector<std::string> getLoadedConfigs() const = 0;

    // === Hot Reload ===
    virtual void enableHotReload(bool enable) = 0;
    virtual bool isHotReloadEnabled() const = 0;

    // === Notifications ===
    virtual SubscriptionId onConfigChanged(
        std::function<void(std::string_view key)> callback) = 0;
    virtual SubscriptionId onKeyChanged(std::string_view prefix,
        std::function<void(std::string_view key)> callback) = 0;
    virtual void unsubscribe(SubscriptionId id) = 0;

    // === Lua Parsing ===
    virtual Result<void> executeLua(std::string_view code,
        std::string_view description = "lua") = 0;
};
```

### 5.12 IUICore

```cpp
class IUICore {
public:
    virtual ~IUICore() = default;

    // === Lifecycle ===
    virtual Result<void> initialize(const UIConfig& config) = 0;
    virtual void shutdown() = 0;
    virtual void update(DeltaTime dt) = 0;
    virtual void render() = 0;

    // === Documents ===
    virtual Result<UIDocHandle> loadDocument(std::string_view path) = 0;
    virtual Result<UIDocHandle> loadDocumentFromString(
        std::string_view content, std::string_view name = "inline") = 0;
    virtual Result<void> unloadDocument(UIDocHandle doc) = 0;
    virtual Result<void> showDocument(UIDocHandle doc) = 0;
    virtual Result<void> hideDocument(UIDocHandle doc) = 0;
    virtual bool isDocumentVisible(UIDocHandle doc) const = 0;

    // === Stylesheets ===
    virtual Result<void> loadStyleSheet(std::string_view path) = 0;
    virtual Result<void> applyStyleSheet(UIDocHandle doc,
        std::string_view stylePath) = 0;

    // === Element Access ===
    virtual std::optional<UIElementHandle> getElementById(
        UIDocHandle doc, std::string_view id) = 0;
    virtual std::vector<UIElementHandle> getElementsByClass(
        UIDocHandle doc, std::string_view className) = 0;
    virtual std::vector<UIElementHandle> getElementsByTag(
        UIDocHandle doc, std::string_view tagName) = 0;
    virtual std::vector<UIElementHandle> getChildren(UIElementHandle elem) = 0;
    virtual std::optional<UIElementHandle> getParent(UIElementHandle elem) = 0;

    // === Element Properties ===
    virtual Result<void> setText(UIElementHandle elem, std::string_view text) = 0;
    virtual std::string getText(UIElementHandle elem) const = 0;
    virtual Result<void> setVisible(UIElementHandle elem, bool visible) = 0;
    virtual bool isVisible(UIElementHandle elem) const = 0;
    virtual Result<void> addClass(UIElementHandle elem, std::string_view cls) = 0;
    virtual Result<void> removeClass(UIElementHandle elem, std::string_view cls) = 0;
    virtual bool hasClass(UIElementHandle elem, std::string_view cls) const = 0;
    virtual Result<void> setAttribute(UIElementHandle elem,
        std::string_view name, std::string_view value) = 0;
    virtual std::optional<std::string> getAttribute(UIElementHandle elem,
        std::string_view name) const = 0;
    virtual Result<void> setStyle(UIElementHandle elem,
        std::string_view property, std::string_view value) = 0;

    // === Dynamic DOM ===
    virtual Result<UIElementHandle> createElement(
        UIDocHandle doc, std::string_view tag) = 0;
    virtual Result<void> appendChild(UIElementHandle parent, UIElementHandle child) = 0;
    virtual Result<void> removeElement(UIElementHandle elem) = 0;
    virtual Result<void> setInnerRml(UIElementHandle elem,
        std::string_view rml) = 0;

    // === Data Binding ===
    virtual Result<void> bindInt(std::string_view name, int* value) = 0;
    virtual Result<void> bindFloat(std::string_view name, float* value) = 0;
    virtual Result<void> bindBool(std::string_view name, bool* value) = 0;
    virtual Result<void> bindString(std::string_view name, std::string* value) = 0;
    virtual void unbindData(std::string_view name) = 0;
    virtual void syncBindings() = 0;

    // === Events ===
    virtual SubscriptionId onEvent(std::string_view eventType,
        std::function<void(const UIEventData&)> callback) = 0;
    virtual SubscriptionId onElementEvent(UIElementHandle elem,
        std::string_view eventType,
        std::function<void(const UIEventData&)> callback) = 0;
    virtual void unsubscribe(SubscriptionId id) = 0;

    // === Input ===
    virtual bool processInput(const UIInputEvent& event) = 0;
    virtual bool wantsKeyboardInput() const = 0;
    virtual bool wantsMouseInput() const = 0;

    // === Fonts ===
    virtual Result<void> loadFont(std::string_view path,
        std::string_view familyName = "") = 0;

    // === Viewport ===
    virtual void setViewportSize(int width, int height) = 0;
    virtual void setDPIScale(float scale) = 0;

    // === Debug ===
    virtual void setDebugMode(bool enabled) = 0;
    virtual std::size_t getElementCount() const = 0;
};
```

### 5.13 IAICore

```cpp
class IAICore {
public:
    virtual ~IAICore() = default;

    // === Lifecycle ===
    virtual void update(DeltaTime dt) = 0;

    // === Behavior Trees ===
    virtual Result<void> attachBehaviorTree(Entity entity, AssetHandle treeAsset) = 0;
    virtual Result<void> detachBehaviorTree(Entity entity) = 0;
    virtual bool hasBehaviorTree(Entity entity) const = 0;
    virtual Result<void> setBlackboardValue(Entity entity,
        std::string_view key, const BlackboardValue& value) = 0;
    virtual std::optional<BlackboardValue> getBlackboardValue(
        Entity entity, std::string_view key) const = 0;
    virtual Result<void> clearBlackboard(Entity entity) = 0;

    // === Navigation Mesh ===
    virtual Result<void> loadNavMesh(AssetHandle asset) = 0;
    virtual void unloadNavMesh() = 0;
    virtual bool hasNavMesh() const = 0;
    virtual Result<std::vector<Vec3>> findPath3D(
        Vec3 start, Vec3 end, float agentRadius = 0.5f) const = 0;
    virtual Result<std::vector<Vec2>> findPath2D(
        Vec2 start, Vec2 end, float agentRadius = 0.5f) const = 0;
    virtual bool isPointOnNavMesh(Vec3 point) const = 0;
    virtual std::optional<Vec3> getClosestPointOnNavMesh(Vec3 point) const = 0;

    // === Steering Behaviors ===
    virtual Result<void> setNavigationTarget(Entity entity, Vec3 target) = 0;
    virtual Result<void> clearNavigationTarget(Entity entity) = 0;
    virtual std::optional<Vec3> getNavigationTarget(Entity entity) const = 0;
    virtual Result<void> setMaxSpeed(Entity entity, float speed) = 0;
    virtual Result<void> setMaxAcceleration(Entity entity, float accel) = 0;
    virtual Result<void> setAvoidanceRadius(Entity entity, float radius) = 0;

    // === Patrol Behavior ===
    virtual Result<void> setPatrolPath(Entity entity,
        std::span<const Vec3> waypoints, bool loop = true) = 0;
    virtual Result<void> clearPatrolPath(Entity entity) = 0;

    // === Perception ===
    virtual Result<void> setSightRange(Entity entity, float range) = 0;
    virtual Result<void> setSightAngle(Entity entity, float halfAngle) = 0;
    virtual Result<void> setHearingRange(Entity entity, float range) = 0;
    virtual std::vector<Entity> getPerceivedEntities(Entity entity) const = 0;

    // === Spatial Queries ===
    virtual std::vector<Entity> findEntitiesInRadius(
        Vec3 center, float radius, std::uint16_t mask = 0xFFFF) const = 0;
    virtual std::optional<Entity> findClosestEntity(
        Vec3 position, std::uint16_t mask = 0xFFFF) const = 0;
    virtual bool hasLineOfSight(Vec3 from, Vec3 to,
        std::uint16_t obstacleMask = 0xFFFF) const = 0;

    // === Crowd Simulation ===
    virtual Result<void> addToCrowd(Entity entity) = 0;
    virtual Result<void> removeFromCrowd(Entity entity) = 0;
    virtual void setCrowdSeparation(float weight) = 0;
    virtual void setCrowdAlignment(float weight) = 0;
    virtual void setCrowdCohesion(float weight) = 0;
};
```

### 5.14 ISceneCore

```cpp
class ISceneCore {
public:
    virtual ~ISceneCore() = default;

    // === Lifecycle ===
    virtual void update(DeltaTime dt) = 0;
    virtual void render() = 0;

    // === Registration ===
    virtual Result<void> registerScene(std::string_view name,
        AssetHandle sceneAsset) = 0;
    virtual Result<void> unregisterScene(std::string_view name) = 0;

    // === Stack Operations ===
    virtual Result<void> pushScene(std::string_view name,
        const SceneParams& params = {}) = 0;
    virtual Result<void> popScene() = 0;
    virtual Result<void> replaceScene(std::string_view name,
        const SceneParams& params = {}) = 0;
    virtual void clearStack() = 0;

    // === Queries ===
    virtual std::optional<std::string> getActiveSceneName() const = 0;
    virtual std::vector<std::string> getSceneStack() const = 0;
    virtual std::vector<std::string> getRegisteredScenes() const = 0;

    // === Transitions ===
    virtual void setDefaultTransition(SceneTransition transition) = 0;

    // === Events ===
    virtual SubscriptionId onSceneChanged(
        std::function<void(std::string_view from, std::string_view to)> cb) = 0;
    virtual void unsubscribe(SubscriptionId id) = 0;
};

struct SceneTransition {
    enum class Type { None, Fade, Slide, Custom };
    Type type = Type::None;
    float duration = 0.3f;
};
```

### 5.15 IStateCore

```cpp
class IStateCore {
public:
    virtual ~IStateCore() = default;

    // === Lifecycle ===
    virtual void update(DeltaTime dt) = 0;

    // === Key-Value Storage ===
    virtual void setNumber(std::string_view key, double value) = 0;
    virtual double getNumber(std::string_view key, double def = 0) const = 0;
    virtual void setString(std::string_view key, std::string_view value) = 0;
    virtual std::string getString(std::string_view key,
        std::string_view def = "") const = 0;
    virtual void setBool(std::string_view key, bool value) = 0;
    virtual bool getBool(std::string_view key, bool def = false) const = 0;
    virtual void setJson(std::string_view key, std::string_view json) = 0;
    virtual std::string getJson(std::string_view key) const = 0;
    virtual bool hasData(std::string_view key) const = 0;
    virtual void removeData(std::string_view key) = 0;
    virtual void clearData() = 0;

    // === Persistence ===
    using SaveCallback = std::function<void(bool success, SystemError error)>;
    virtual void commit(int slot, std::string_view name, SaveCallback cb = {}) = 0;
    virtual void restore(int slot, SaveCallback cb = {}) = 0;
    virtual Result<void> deleteSlot(int slot) = 0;
    virtual void quickCommit(SaveCallback cb = {}) = 0;
    virtual void quickRestore(SaveCallback cb = {}) = 0;

    // === Auto-Save ===
    virtual void enableAutoCommit(float intervalSeconds) = 0;
    virtual void disableAutoCommit() = 0;

    // === Slot Queries ===
    virtual std::vector<StateMetadata> getAllSlotMetadata() const = 0;
    virtual std::optional<StateMetadata> getSlotMetadata(int slot) const = 0;
    virtual bool slotExists(int slot) const = 0;

    // === Profiles ===
    virtual void setActiveProfile(std::string_view id) = 0;
    virtual std::string getActiveProfile() const = 0;
    virtual std::vector<std::string> getProfiles() const = 0;

    // === Playtime ===
    virtual std::uint64_t getSessionPlaytime() const = 0;
    virtual std::uint64_t getTotalPlaytime() const = 0;

    // === Migration ===
    virtual void setFormatVersion(int version) = 0;
    virtual void registerMigration(int fromVersion, int toVersion,
        std::function<void()> fn) = 0;

    // === Export (dev tools) ===
    virtual Result<void> exportToJson(int slot, std::string_view path) const = 0;
    virtual Result<void> importFromJson(int slot, std::string_view path) = 0;
};
```

### 5.16 IGASCore

```cpp
class IGASCore {
public:
    virtual ~IGASCore() = default;

    // === Lifecycle ===
    virtual void update(DeltaTime dt) = 0;

    // === Tag Registration ===
    virtual GameplayTag registerTag(std::string_view name) = 0;
    virtual std::optional<GameplayTag> findTag(std::string_view name) const = 0;
    virtual bool isChildOf(GameplayTag child, GameplayTag parent) const = 0;

    // === Attribute Registration ===
    virtual AttributeId registerAttribute(const AttributeDef& def) = 0;
    virtual std::optional<AttributeDef> getAttributeDef(AttributeId id) const = 0;
    virtual std::optional<AttributeDef> getAttributeDef(
        std::string_view name) const = 0;

    // === Effect Registration ===
    virtual EffectId registerEffect(const EffectDef& def) = 0;
    virtual std::optional<EffectDef> getEffectDef(EffectId id) const = 0;

    // === Ability Registration ===
    virtual AbilityId registerAbility(const AbilityDef& def) = 0;
    virtual std::optional<AbilityDef> getAbilityDef(AbilityId id) const = 0;

    // === Entity Component ===
    virtual Result<void> initializeComponent(Entity entity) = 0;
    virtual Result<void> removeComponent(Entity entity) = 0;
    virtual bool hasComponent(Entity entity) const = 0;

    // === Tag Operations ===
    virtual Result<void> addTag(Entity entity, GameplayTag tag) = 0;
    virtual Result<void> removeTag(Entity entity, GameplayTag tag) = 0;
    virtual bool hasTag(Entity entity, GameplayTag tag) const = 0;
    virtual std::vector<GameplayTag> getTags(Entity entity) const = 0;

    // === Attribute Operations ===
    virtual Result<void> initializeAttribute(Entity entity,
        AttributeId id, float baseValue) = 0;
    virtual float getAttributeValue(Entity entity, AttributeId id) const = 0;
    virtual float getAttributeBaseValue(Entity entity, AttributeId id) const = 0;
    virtual Result<void> setAttributeBaseValue(Entity entity,
        AttributeId id, float value) = 0;
    virtual Result<void> modifyAttribute(Entity entity,
        AttributeId id, float delta) = 0;

    // === Effect Operations ===
    virtual Result<void> applyEffect(Entity target, EffectId effect,
        Entity source = NullEntity) = 0;
    virtual Result<void> removeEffect(Entity entity, EffectId effect) = 0;
    virtual void removeAllEffects(Entity entity) = 0;
    virtual bool hasEffect(Entity entity, EffectId effect) const = 0;
    virtual std::vector<ActiveEffect> getActiveEffects(Entity entity) const = 0;

    // === Ability Operations ===
    virtual Result<void> grantAbility(Entity entity, AbilityId ability) = 0;
    virtual Result<void> removeAbility(Entity entity, AbilityId ability) = 0;
    virtual bool hasAbility(Entity entity, AbilityId ability) const = 0;
    virtual bool canActivateAbility(Entity entity, AbilityId ability) const = 0;
    virtual Result<bool> tryActivateAbility(Entity entity, AbilityId ability) = 0;
    virtual Result<void> endAbility(Entity entity, AbilityId ability) = 0;
    virtual bool isAbilityActive(Entity entity, AbilityId ability) const = 0;
    virtual float getAbilityCooldown(Entity entity, AbilityId ability) const = 0;

    // === Callbacks ===
    virtual SubscriptionId onAttributeChanged(
        std::function<void(const AttributeChangeEvent&)> cb) = 0;
    virtual SubscriptionId onEffectApplied(
        std::function<void(const EffectAppliedEvent&)> cb) = 0;
    virtual SubscriptionId onAbilityActivated(
        std::function<void(const AbilityActivatedEvent&)> cb) = 0;
    virtual void unsubscribe(SubscriptionId id) = 0;

    // === Lua Definitions ===
    virtual Result<void> loadDefinitionsFromLua(std::string_view source) = 0;
};
```

### 5.17 New Systems — Core Contracts

#### ITweenCore (NEW)

```cpp
class ITweenCore {
public:
    virtual ~ITweenCore() = default;

    // === Lifecycle ===
    virtual void update(DeltaTime dt) = 0;

    // === Tween Creation ===
    virtual TweenHandle tweenFloat(float from, float to, float duration,
        std::function<void(float)> setter) = 0;
    virtual TweenHandle tweenVec2(Vec2 from, Vec2 to, float duration,
        std::function<void(Vec2)> setter) = 0;
    virtual TweenHandle tweenVec3(Vec3 from, Vec3 to, float duration,
        std::function<void(Vec3)> setter) = 0;
    virtual TweenHandle tweenColor(Color from, Color to, float duration,
        std::function<void(Color)> setter) = 0;

    // === Easing ===
    virtual Result<void> setEasing(TweenHandle h, EasingType easing) = 0;
    virtual Result<void> setCustomEasing(TweenHandle h,
        std::function<float(float)> curve) = 0;

    // === Chaining ===
    virtual Result<void> setDelay(TweenHandle h, float delay) = 0;
    virtual Result<void> setRepeat(TweenHandle h, int count) = 0;  // -1 = infinite
    virtual Result<void> setYoyo(TweenHandle h, bool yoyo) = 0;
    virtual Result<void> onComplete(TweenHandle h, std::function<void()> cb) = 0;
    virtual Result<void> onStep(TweenHandle h, std::function<void(float)> cb) = 0;

    // === Control ===
    virtual Result<void> pause(TweenHandle h) = 0;
    virtual Result<void> resume(TweenHandle h) = 0;
    virtual Result<void> cancel(TweenHandle h) = 0;
    virtual Result<void> complete(TweenHandle h) = 0;  // Jump to end
    virtual bool isActive(TweenHandle h) const = 0;
    virtual float getProgress(TweenHandle h) const = 0;

    // === Bulk ===
    virtual void pauseAll() = 0;
    virtual void resumeAll() = 0;
    virtual void cancelAll() = 0;
    virtual int activeCount() const = 0;

    // === Sequences ===
    virtual TweenHandle createSequence() = 0;
    virtual Result<void> appendToSequence(TweenHandle seq, TweenHandle tween) = 0;
    virtual Result<void> insertInSequence(TweenHandle seq,
        float atTime, TweenHandle tween) = 0;
};

enum class EasingType : std::uint8_t {
    Linear,
    InQuad, OutQuad, InOutQuad,
    InCubic, OutCubic, InOutCubic,
    InQuart, OutQuart, InOutQuart,
    InQuint, OutQuint, InOutQuint,
    InSine, OutSine, InOutSine,
    InExpo, OutExpo, InOutExpo,
    InCirc, OutCirc, InOutCirc,
    InElastic, OutElastic, InOutElastic,
    InBack, OutBack, InOutBack,
    InBounce, OutBounce, InOutBounce,
    Spring
};
```

#### IParticleCore (NEW)

```cpp
class IParticleCore {
public:
    virtual ~IParticleCore() = default;

    // === Lifecycle ===
    virtual void update(DeltaTime dt) = 0;
    virtual void render() = 0;

    // === Emitter Management ===
    virtual Result<EmitterHandle> createEmitter(const EmitterDef& def) = 0;
    virtual Result<void> destroyEmitter(EmitterHandle handle) = 0;

    // === Emitter Control ===
    virtual Result<void> play(EmitterHandle h) = 0;
    virtual Result<void> stop(EmitterHandle h) = 0;
    virtual Result<void> pause(EmitterHandle h) = 0;
    virtual Result<void> restart(EmitterHandle h) = 0;
    virtual bool isPlaying(EmitterHandle h) const = 0;

    // === Emitter Transform ===
    virtual Result<void> setPosition(EmitterHandle h, Vec3 position) = 0;
    virtual Result<void> setRotation(EmitterHandle h, Quat rotation) = 0;
    virtual Result<void> setScale(EmitterHandle h, float scale) = 0;
    virtual Result<void> attachToEntity(EmitterHandle h, Entity entity) = 0;
    virtual Result<void> detachFromEntity(EmitterHandle h) = 0;

    // === Emitter Properties (runtime modification) ===
    virtual Result<void> setEmissionRate(EmitterHandle h, float rate) = 0;
    virtual Result<void> setMaxParticles(EmitterHandle h, int max) = 0;
    virtual Result<void> setParticleLifetime(EmitterHandle h,
        float min, float max) = 0;
    virtual Result<void> setParticleSpeed(EmitterHandle h,
        float min, float max) = 0;
    virtual Result<void> setParticleSize(EmitterHandle h,
        float startMin, float startMax, float endMin, float endMax) = 0;
    virtual Result<void> setParticleColor(EmitterHandle h,
        Color start, Color end) = 0;
    virtual Result<void> setGravity(EmitterHandle h, Vec3 gravity) = 0;

    // === Emission Shapes ===
    virtual Result<void> setEmissionShape(EmitterHandle h,
        EmissionShape shape) = 0;

    // === Material ===
    virtual Result<void> setParticleTexture(EmitterHandle h,
        TextureHandle texture) = 0;
    virtual Result<void> setBlendMode(EmitterHandle h, BlendMode mode) = 0;

    // === Sub-Emitters ===
    virtual Result<void> addSubEmitter(EmitterHandle parent,
        const EmitterDef& child, SubEmitterTrigger trigger) = 0;

    // === Queries ===
    virtual int getParticleCount(EmitterHandle h) const = 0;
    virtual int getTotalParticleCount() const = 0;
    virtual AABB3D getBounds(EmitterHandle h) const = 0;

    // === GPU Particles ===
    virtual bool isGPUSimulationSupported() const = 0;
    virtual Result<void> setGPUSimulation(EmitterHandle h, bool enabled) = 0;

    // === Presets ===
    virtual Result<EmitterHandle> loadPreset(AssetHandle luaPreset) = 0;
    virtual Result<void> savePreset(EmitterHandle h,
        std::string_view path) const = 0;
};

struct EmitterDef {
    // Emission
    float emissionRate = 10.0f;
    int maxParticles = 1000;
    float duration = 0;  // 0 = infinite
    bool playOnCreate = true;

    // Particle properties
    float lifetimeMin = 1.0f, lifetimeMax = 2.0f;
    float speedMin = 1.0f, speedMax = 5.0f;
    float sizeStart = 1.0f, sizeEnd = 0.0f;
    Color colorStart = Color::white();
    Color colorEnd = {1, 1, 1, 0};
    float rotationSpeedMin = 0, rotationSpeedMax = 0;

    // Physics
    Vec3 gravity = {0, -9.81f, 0};
    float drag = 0;

    // Shape
    EmissionShape shape;

    // Rendering
    BlendMode blendMode = BlendMode::Additive;
    bool sortByDepth = false;
    bool billboard = true;
};

struct EmissionShape {
    enum class Type : std::uint8_t {
        Point, Sphere, Hemisphere, Cone, Box, Circle, Ring, Edge
    };
    Type type = Type::Point;
    float radius = 1.0f;
    float angle = 45.0f;      // for Cone
    Vec3 extents = {1, 1, 1}; // for Box
};

enum class SubEmitterTrigger : std::uint8_t {
    OnBirth, OnDeath, OnCollision
};
```

#### INetworkCore (NEW)

```cpp
class INetworkCore {
public:
    virtual ~INetworkCore() = default;

    // === Lifecycle ===
    virtual Result<void> initialize(const NetworkConfig& config) = 0;
    virtual void shutdown() = 0;
    virtual void update(DeltaTime dt) = 0;

    // === Connection ===
    virtual Result<void> host(int port, int maxClients = 16) = 0;
    virtual Result<void> connect(std::string_view address, int port) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    virtual bool isHost() const = 0;
    virtual NetworkId getLocalId() const = 0;
    virtual std::vector<NetworkId> getConnectedClients() const = 0;

    // === Entity Replication ===
    virtual Result<void> registerReplicatedEntity(Entity entity) = 0;
    virtual Result<void> unregisterReplicatedEntity(Entity entity) = 0;
    virtual Result<void> setOwner(Entity entity, NetworkId owner) = 0;
    virtual NetworkId getOwner(Entity entity) const = 0;
    virtual bool isLocallyOwned(Entity entity) const = 0;

    // === Replicated Components ===
    virtual Result<void> registerReplicatedComponent(
        std::string_view componentName,
        ReplicationMode mode = ReplicationMode::Reliable) = 0;
    virtual Result<void> markDirty(Entity entity,
        std::string_view componentName) = 0;

    // === RPCs ===
    virtual Result<RPCHandle> registerRPC(std::string_view name,
        std::function<void(NetworkId sender, std::span<const std::byte> data)> handler) = 0;
    virtual Result<void> callRPC(RPCHandle rpc, NetworkId target,
        std::span<const std::byte> data) = 0;
    virtual Result<void> callRPCAll(RPCHandle rpc,
        std::span<const std::byte> data) = 0;
    virtual Result<void> callRPCServer(RPCHandle rpc,
        std::span<const std::byte> data) = 0;
    virtual void unregisterRPC(RPCHandle rpc) = 0;

    // === Lobby ===
    virtual Result<void> setMaxClients(int max) = 0;
    virtual Result<void> kickClient(NetworkId client) = 0;

    // === Latency ===
    virtual float getPing(NetworkId client) const = 0;
    virtual float getAveragePing() const = 0;
    virtual float getPacketLoss() const = 0;

    // === Events ===
    virtual SubscriptionId onClientConnected(
        std::function<void(NetworkId)> cb) = 0;
    virtual SubscriptionId onClientDisconnected(
        std::function<void(NetworkId)> cb) = 0;
    virtual void unsubscribe(SubscriptionId id) = 0;

    // === Serialization Helpers ===
    virtual std::vector<std::byte> serializeEntity(Entity entity) const = 0;
    virtual Result<void> deserializeEntity(Entity entity,
        std::span<const std::byte> data) = 0;
};

struct NetworkConfig {
    enum class Protocol : std::uint8_t { UDP, WebSocket };
    Protocol protocol = Protocol::UDP;
    int tickRate = 30;            // Server updates per second
    float interpolationDelay = 0.1f;  // seconds
    bool enablePrediction = true;
    bool enableReconciliation = true;
};

enum class ReplicationMode : std::uint8_t {
    Reliable,     // TCP-like, ordered
    Unreliable,   // UDP-like, fastest
    UnreliableOrdered  // Dropped if out of order
};
```


---

## 6. High-Level System Contracts

Each public system also has a **System** contract — the simplified, opinionated API that game developers use daily. These map 1:1 to `bestow.<system>.*` in Lua.

Convention: System contracts are named `I<System>System` and contain 8-20 methods. They favor paths over handles, fire-and-forget over callbacks, and sensible defaults.

### Design Rules for System Contracts

1. **Path-based** — Accept `std::string_view path` where Core uses handles
2. **Sensible defaults** — Most parameters are optional with good defaults
3. **Entity-centric** — Operations target entities, not handles
4. **No lifecycle methods** — No `initialize()`/`shutdown()`/`update()` (engine handles these)
5. **Minimal configuration** — Core tunables not exposed here

### 6.1 IEntitySystem

```cpp
class IEntitySystem {
public:
    virtual ~IEntitySystem() = default;

    virtual Entity create() = 0;
    virtual Result<void> destroy(Entity entity) = 0;
    virtual bool isValid(Entity entity) const = 0;
    virtual std::size_t count() const = 0;

    // Name-based component access (Lua-friendly)
    virtual Result<void> addComponent(Entity entity,
        std::string_view type, const ComponentData& data) = 0;
    virtual Result<void> removeComponent(Entity entity,
        std::string_view type) = 0;
    virtual bool hasComponent(Entity entity, std::string_view type) const = 0;
    virtual Result<ComponentData> getComponent(Entity entity,
        std::string_view type) const = 0;
    virtual Result<void> setField(Entity entity, std::string_view type,
        std::string_view field, const SceneParam& value) = 0;

    // Queries
    virtual std::vector<Entity> findByComponent(
        std::string_view type) const = 0;
    virtual std::optional<Entity> findByName(std::string_view name) const = 0;
    virtual std::vector<Entity> findByTag(std::string_view tag) const = 0;

    // Hierarchy
    virtual Result<void> setParent(Entity child, Entity parent) = 0;
    virtual std::vector<Entity> getChildren(Entity entity) const = 0;
};
```

### 6.2 IInputSystem

```cpp
class IInputSystem {
public:
    virtual ~IInputSystem() = default;

    // Phase management
    virtual void pushPhase(std::string_view phase) = 0;
    virtual void popPhase() = 0;
    virtual void changePhase(std::string_view phase) = 0;
    virtual std::string getCurrentPhase() const = 0;

    // Action registration
    virtual void registerAction(const ActionRegistration& reg) = 0;
    virtual void unregisterAction(std::string_view name) = 0;

    // Polling (for cases where events are overkill)
    virtual bool isKeyDown(KeyCode key) const = 0;
    virtual bool wasKeyJustPressed(KeyCode key) const = 0;
    virtual Vec2 getMousePosition() const = 0;
    virtual Vec2 getMouseDelta() const = 0;
    virtual Vec2 getLeftStick(int index = 0) const = 0;
    virtual Vec2 getRightStick(int index = 0) const = 0;

    // Cursor
    virtual void setCursorMode(CursorMode mode) = 0;

    // Config
    virtual Result<void> loadConfig(std::string_view path) = 0;
};
```

### 6.3 IAudioSystem

```cpp
class IAudioSystem {
public:
    virtual ~IAudioSystem() = default;

    // Fire-and-forget
    virtual Result<SoundHandle> play(std::string_view soundPath,
        float volume = 1.0f) = 0;
    virtual Result<SoundHandle> playAt(std::string_view soundPath,
        Vec3 position, float volume = 1.0f) = 0;

    // Music (one active track, with crossfade)
    virtual Result<void> playMusic(std::string_view musicPath,
        float fadeIn = 1.0f, bool loop = true) = 0;
    virtual Result<void> stopMusic(float fadeOut = 1.0f) = 0;

    // Simple controls
    virtual Result<void> stop(SoundHandle handle) = 0;
    virtual void stopAll() = 0;
    virtual bool isPlaying(SoundHandle handle) const = 0;

    // Volume
    virtual void setMasterVolume(float volume) = 0;
    virtual void setMusicVolume(float volume) = 0;
    virtual void setSFXVolume(float volume) = 0;
};
```

### 6.4 IPhysicsSystem (2D)

```cpp
class IPhysicsSystem {
public:
    virtual ~IPhysicsSystem() = default;

    // Body creation (entity-centric)
    virtual Result<void> createBody(Entity entity, const BodyDef2D& def) = 0;
    virtual Result<void> destroyBody(Entity entity) = 0;
    virtual bool hasBody(Entity entity) const = 0;

    // Transform sync
    virtual Vec2 getPosition(Entity entity) const = 0;
    virtual Result<void> setPosition(Entity entity, Vec2 pos) = 0;
    virtual Vec2 getVelocity(Entity entity) const = 0;
    virtual Result<void> setVelocity(Entity entity, Vec2 vel) = 0;

    // Forces
    virtual Result<void> applyForce(Entity entity, Vec2 force) = 0;
    virtual Result<void> applyImpulse(Entity entity, Vec2 impulse) = 0;

    // Queries
    virtual std::optional<RaycastHit2D> raycast(
        Vec2 origin, Vec2 direction, float maxDist) const = 0;
    virtual GroundCheckResult checkGrounded(Entity entity) const = 0;

    // World
    virtual void setGravity(Vec2 gravity) = 0;

    // Callbacks
    virtual SubscriptionId onCollision(
        std::function<void(const CollisionEvent&)> cb) = 0;
    virtual SubscriptionId onTrigger(
        std::function<void(const TriggerEvent&)> cb) = 0;
    virtual void unsubscribe(SubscriptionId id) = 0;
};
```

### 6.5 IPhysics3DSystem

```cpp
class IPhysics3DSystem {
public:
    virtual ~IPhysics3DSystem() = default;

    // Body creation
    virtual Result<void> createBody(Entity entity, const BodyDef3D& def) = 0;
    virtual Result<void> destroyBody(Entity entity) = 0;

    // Transform
    virtual Vec3 getPosition(Entity entity) const = 0;
    virtual Result<void> setPosition(Entity entity, Vec3 pos) = 0;
    virtual Quat getRotation(Entity entity) const = 0;
    virtual Vec3 getVelocity(Entity entity) const = 0;
    virtual Result<void> setVelocity(Entity entity, Vec3 vel) = 0;

    // Forces
    virtual Result<void> applyForce(Entity entity, Vec3 force) = 0;
    virtual Result<void> applyImpulse(Entity entity, Vec3 impulse) = 0;

    // Character controller
    virtual Result<void> createCharacterController(Entity entity,
        const CharacterDef3D& def) = 0;
    virtual Result<void> moveCharacter(Entity entity,
        Vec3 displacement, DeltaTime dt) = 0;
    virtual bool isCharacterGrounded(Entity entity) const = 0;

    // Queries
    virtual std::optional<RaycastHit3D> raycast(
        Vec3 origin, Vec3 direction, float maxDist) const = 0;

    // World
    virtual void setGravity(Vec3 gravity) = 0;

    // Callbacks
    virtual SubscriptionId onCollision(
        std::function<void(const CollisionEvent3D&)> cb) = 0;
    virtual void unsubscribe(SubscriptionId id) = 0;
};
```

### 6.6 IGraphics2DSystem

```cpp
class IGraphics2DSystem {
public:
    virtual ~IGraphics2DSystem() = default;

    // Drawing
    virtual void drawSprite(std::string_view texturePath,
        Vec2 position, Vec2 size = {}, Color tint = Color::white()) = 0;
    virtual void drawRect(Vec2 position, Vec2 size, Color color,
        bool filled = true) = 0;
    virtual void drawCircle(Vec2 center, float radius, Color color,
        bool filled = true) = 0;
    virtual void drawLine(Vec2 from, Vec2 to, Color color,
        float thickness = 1.0f) = 0;
    virtual void drawText(std::string_view text, Vec2 position,
        float size = 16.0f, Color color = Color::white()) = 0;

    // Camera
    virtual void setCameraPosition(Vec2 position) = 0;
    virtual void setCameraZoom(float zoom) = 0;
    virtual Vec2 screenToWorld(Vec2 screenPos) const = 0;
    virtual Vec2 worldToScreen(Vec2 worldPos) const = 0;

    // Window
    virtual Size getWindowSize() const = 0;
    virtual bool shouldClose() const = 0;
    virtual void setClearColor(Color color) = 0;
};
```

### 6.7 IGraphics3DSystem

```cpp
class IGraphics3DSystem {
public:
    virtual ~IGraphics3DSystem() = default;

    // Mesh drawing (path-based — handles created internally)
    virtual Result<void> drawModel(std::string_view modelPath,
        Vec3 position, Quat rotation = {},
        Vec3 scale = {1,1,1}) = 0;
    virtual Result<void> drawPrimitive(PrimitiveType type,
        Vec3 position, Vec3 scale = {1,1,1},
        Color color = Color::white()) = 0;

    // Lighting
    virtual void setDirectionalLight(Vec3 direction, Color color = Color::white(),
        float intensity = 1.0f) = 0;
    virtual void setAmbientLight(Color color, float intensity = 0.3f) = 0;

    // Camera
    virtual void setCameraPosition(Vec3 position) = 0;
    virtual void setCameraTarget(Vec3 target) = 0;
    virtual void setCameraFOV(float degrees) = 0;

    // Environment
    virtual Result<void> setSkybox(std::string_view cubemapPath) = 0;

    // Window
    virtual Size getWindowSize() const = 0;
    virtual bool shouldClose() const = 0;
    virtual void setClearColor(Color color) = 0;
};
```

### 6.8 IAnimationSystem

```cpp
class IAnimationSystem {
public:
    virtual ~IAnimationSystem() = default;

    // Simple playback
    virtual Result<void> play(Entity entity, std::string_view clipName,
        float blendTime = 0.25f, bool loop = true) = 0;
    virtual Result<void> stop(Entity entity, float blendTime = 0) = 0;
    virtual bool isPlaying(Entity entity) const = 0;

    // Speed
    virtual Result<void> setSpeed(Entity entity, float speed) = 0;

    // Queries
    virtual float getTime(Entity entity) const = 0;
    virtual float getNormalizedTime(Entity entity) const = 0;
    virtual std::string_view getCurrentClipName(Entity entity) const = 0;

    // Setup (from model asset)
    virtual Result<void> setupFromModel(Entity entity,
        std::string_view modelPath) = 0;

    // Events
    virtual SubscriptionId onAnimationComplete(
        std::function<void(Entity, std::string_view clipName)> cb) = 0;
    virtual void unsubscribe(SubscriptionId id) = 0;
};
```

### 6.9 ICameraSystem

```cpp
class ICameraSystem {
public:
    virtual ~ICameraSystem() = default;

    // 2D follow
    virtual void follow(Entity target) = 0;
    virtual void setSmoothing(float smoothing) = 0;
    virtual void setOffset(Vec2 offset) = 0;
    virtual void setBounds(float minX, float maxX, float minY, float maxY) = 0;

    // 3D camera
    virtual void lookAt(Vec3 from, Vec3 target) = 0;
    virtual void setFOV(float degrees) = 0;

    // Effects
    virtual void shake(float intensity, float duration) = 0;
    virtual void setZoom(float zoom) = 0;

    // Queries
    virtual Vec2 getPosition2D() const = 0;
    virtual Vec3 getPosition3D() const = 0;
    virtual Vec2 screenToWorld(Vec2 screenPos) const = 0;
};
```

### 6.10 ISceneSystem

```cpp
class ISceneSystem {
public:
    virtual ~ISceneSystem() = default;

    virtual Result<void> registerScene(std::string_view name,
        std::string_view luaPath) = 0;
    virtual Result<void> pushScene(std::string_view name,
        const SceneParams& params = {}) = 0;
    virtual Result<void> popScene() = 0;
    virtual Result<void> replaceScene(std::string_view name,
        const SceneParams& params = {}) = 0;
    virtual std::optional<std::string> getActiveScene() const = 0;
    virtual std::vector<std::string> getSceneStack() const = 0;
};
```

### 6.11 IStateSystem

```cpp
class IStateSystem {
public:
    virtual ~IStateSystem() = default;

    // Key-value store
    virtual void set(std::string_view key, double value) = 0;
    virtual void set(std::string_view key, std::string_view value) = 0;
    virtual void set(std::string_view key, bool value) = 0;
    virtual double getNumber(std::string_view key, double def = 0) const = 0;
    virtual std::string getString(std::string_view key,
        std::string_view def = "") const = 0;
    virtual bool getBool(std::string_view key, bool def = false) const = 0;
    virtual bool has(std::string_view key) const = 0;
    virtual void remove(std::string_view key) = 0;

    // Save/Load
    virtual void save(int slot, std::string_view name) = 0;
    virtual void load(int slot) = 0;
    virtual void quickSave() = 0;
    virtual void quickLoad() = 0;
    virtual bool slotExists(int slot) const = 0;
};
```

### 6.12 IConfigSystem

```cpp
class IConfigSystem {
public:
    virtual ~IConfigSystem() = default;

    virtual Result<void> loadConfig(std::string_view path) = 0;
    virtual std::optional<float> getFloat(std::string_view key) const = 0;
    virtual std::optional<int> getInt(std::string_view key) const = 0;
    virtual std::optional<bool> getBool(std::string_view key) const = 0;
    virtual std::optional<std::string> getString(std::string_view key) const = 0;
    virtual float getFloatOr(std::string_view key, float def) const = 0;
    virtual int getIntOr(std::string_view key, int def) const = 0;
    virtual bool getBoolOr(std::string_view key, bool def) const = 0;
    virtual bool hasKey(std::string_view key) const = 0;
};
```

### 6.13 IUISystem

```cpp
class IUISystem {
public:
    virtual ~IUISystem() = default;

    virtual Result<UIDocHandle> loadDocument(std::string_view path) = 0;
    virtual Result<void> showDocument(UIDocHandle doc) = 0;
    virtual Result<void> hideDocument(UIDocHandle doc) = 0;
    virtual std::optional<UIElementHandle> getElementById(
        UIDocHandle doc, std::string_view id) = 0;
    virtual Result<void> setText(UIElementHandle elem,
        std::string_view text) = 0;
    virtual Result<void> addClass(UIElementHandle elem,
        std::string_view cls) = 0;
    virtual Result<void> removeClass(UIElementHandle elem,
        std::string_view cls) = 0;
    virtual SubscriptionId onEvent(std::string_view eventType,
        std::function<void(const UIEventData&)> callback) = 0;
    virtual void unsubscribe(SubscriptionId id) = 0;
};
```

### 6.14 IAISystem

```cpp
class IAISystem {
public:
    virtual ~IAISystem() = default;

    virtual Result<void> attachBehaviorTree(Entity entity,
        std::string_view treePath) = 0;
    virtual Result<void> detachBehaviorTree(Entity entity) = 0;
    virtual Result<void> setBlackboard(Entity entity,
        std::string_view key, const BlackboardValue& value) = 0;
    virtual Result<void> setNavigationTarget(Entity entity, Vec3 target) = 0;
    virtual Result<void> clearNavigationTarget(Entity entity) = 0;
    virtual std::vector<Entity> findInRadius(
        Vec3 center, float radius) const = 0;
    virtual bool hasLineOfSight(Vec3 from, Vec3 to) const = 0;
};
```

### 6.15 IGASSystem

```cpp
class IGASSystem {
public:
    virtual ~IGASSystem() = default;

    virtual Result<void> initializeComponent(Entity entity) = 0;
    virtual Result<void> initializeAttribute(Entity entity,
        std::string_view name, float value) = 0;
    virtual float getAttribute(Entity entity, std::string_view name) const = 0;
    virtual Result<void> modifyAttribute(Entity entity,
        std::string_view name, float delta) = 0;
    virtual Result<void> applyEffect(Entity target,
        std::string_view effectName, Entity source = NullEntity) = 0;
    virtual Result<void> grantAbility(Entity entity,
        std::string_view abilityName) = 0;
    virtual Result<bool> tryActivateAbility(Entity entity,
        std::string_view abilityName) = 0;
    virtual Result<void> addTag(Entity entity, std::string_view tag) = 0;
    virtual bool hasTag(Entity entity, std::string_view tag) const = 0;
    virtual Result<void> loadDefinitions(std::string_view luaPath) = 0;
};
```

### 6.16 ITweenSystem (NEW)

```cpp
class ITweenSystem {
public:
    virtual ~ITweenSystem() = default;

    virtual TweenHandle to(float from, float to, float duration,
        std::function<void(float)> setter,
        EasingType easing = EasingType::OutQuad) = 0;
    virtual TweenHandle toVec2(Vec2 from, Vec2 to, float duration,
        std::function<void(Vec2)> setter,
        EasingType easing = EasingType::OutQuad) = 0;
    virtual TweenHandle toVec3(Vec3 from, Vec3 to, float duration,
        std::function<void(Vec3)> setter,
        EasingType easing = EasingType::OutQuad) = 0;
    virtual TweenHandle toColor(Color from, Color to, float duration,
        std::function<void(Color)> setter,
        EasingType easing = EasingType::OutQuad) = 0;

    virtual Result<void> cancel(TweenHandle h) = 0;
    virtual bool isActive(TweenHandle h) const = 0;
    virtual void cancelAll() = 0;
};
```

### 6.17 IParticleSystem (NEW)

```cpp
class IParticleSystem {
public:
    virtual ~IParticleSystem() = default;

    virtual Result<EmitterHandle> createEmitter(
        Vec3 position, const EmitterDef& def) = 0;
    virtual Result<EmitterHandle> loadPreset(
        std::string_view presetPath, Vec3 position) = 0;
    virtual Result<void> play(EmitterHandle h) = 0;
    virtual Result<void> stop(EmitterHandle h) = 0;
    virtual Result<void> destroy(EmitterHandle h) = 0;
    virtual Result<void> setPosition(EmitterHandle h, Vec3 position) = 0;
    virtual Result<void> attachToEntity(EmitterHandle h, Entity entity) = 0;
    virtual int getTotalParticleCount() const = 0;
};
```

### 6.18 INetworkSystem (NEW)

```cpp
class INetworkSystem {
public:
    virtual ~INetworkSystem() = default;

    virtual Result<void> host(int port) = 0;
    virtual Result<void> connect(std::string_view address, int port) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    virtual bool isHost() const = 0;
    virtual NetworkId getLocalId() const = 0;

    virtual Result<void> replicate(Entity entity) = 0;
    virtual bool isLocallyOwned(Entity entity) const = 0;

    virtual SubscriptionId onClientConnected(
        std::function<void(NetworkId)> cb) = 0;
    virtual SubscriptionId onClientDisconnected(
        std::function<void(NetworkId)> cb) = 0;
    virtual void unsubscribe(SubscriptionId id) = 0;

    virtual float getPing() const = 0;
};
```

---

## 7. SystemContext (Replacing DI)

Per the Critical Review (Flaw #1), the runtime DI container is replaced with a typed, compile-time checked SystemContext.

```cpp
struct SystemContext {
    // === Protected Systems (peer-only) ===
    std::unique_ptr<IAssetCore>           assets;
    std::unique_ptr<IShaderCore>          shader;
    std::unique_ptr<IGraphicsContextCore> graphicsContext;
    std::unique_ptr<IUIRenderBackend>     uiRenderBackend;
    std::unique_ptr<IBlueprintCore>       blueprints;
    std::unique_ptr<IMetricsCore>         metrics;

    // === Public Systems (Core + System pairs) ===
    // Each system has a Core and System pointer.
    // One implementation satisfies both (e.g., AudioSystemImpl : IAudioCore, IAudioSystem)
    std::unique_ptr<IEntityCore>          entityCore;
    IEntitySystem*                        entity = nullptr;  // Points into entityCore

    std::unique_ptr<IEventCore>           events;

    std::unique_ptr<IInputCore>           inputCore;
    IInputSystem*                         input = nullptr;

    std::unique_ptr<IAudioCore>           audioCore;
    IAudioSystem*                         audio = nullptr;

    std::unique_ptr<IPhysics2DCore>       physics2DCore;
    IPhysicsSystem*                       physics = nullptr;

    std::unique_ptr<IPhysics3DCore>       physics3DCore;
    IPhysics3DSystem*                     physics3D = nullptr;

    std::unique_ptr<IGraphics2DCore>      graphics2DCore;
    IGraphics2DSystem*                    graphics2D = nullptr;

    std::unique_ptr<IGraphics3DCore>      graphics3DCore;
    IGraphics3DSystem*                    graphics3D = nullptr;

    // Split 3D graphics sub-contracts (point into graphics3DCore impl)
    IPostProcessingCore*                  postProcessing = nullptr;
    IShadowCore*                          shadows = nullptr;
    IDebugRendererCore*                   debugRenderer = nullptr;
    IText3DCore*                          text3D = nullptr;

    std::unique_ptr<IAnimationCore>       animationCore;
    IAnimationSystem*                     animation = nullptr;

    std::unique_ptr<ICameraCore>          cameraCore;
    ICameraSystem*                        camera = nullptr;

    std::unique_ptr<ISceneCore>           sceneCore;
    ISceneSystem*                         scene = nullptr;

    std::unique_ptr<IStateCore>           stateCore;
    IStateSystem*                         state = nullptr;

    std::unique_ptr<IConfigCore>          configCore;
    IConfigSystem*                        config = nullptr;

    std::unique_ptr<IUICore>              uiCore;
    IUISystem*                            ui = nullptr;

    std::unique_ptr<IAICore>              aiCore;
    IAISystem*                            ai = nullptr;

    std::unique_ptr<IGASCore>             gasCore;
    IGASSystem*                           gas = nullptr;

    std::unique_ptr<IGameStateSystem>     gameState;

    std::unique_ptr<ITweenCore>           tweenCore;
    ITweenSystem*                         tween = nullptr;

    std::unique_ptr<IParticleCore>        particleCore;
    IParticleSystem*                      particle = nullptr;

    std::unique_ptr<INetworkCore>         networkCore;
    INetworkSystem*                       network = nullptr;
};

// Factory function — creates default implementation set
SystemContext buildDefaultContext();

// Client can override any system:
SystemContext ctx = buildDefaultContext();
ctx.audioCore = std::make_unique<MyCustomAudio>();
ctx.audio = static_cast<IAudioSystem*>(ctx.audioCore.get());
engine.run(std::move(ctx));
```

**Benefits over runtime DI:**
- All dependencies visible and typed at compile time
- No registration order fragility
- No runtime type lookups
- Errors caught by the compiler, not at runtime
- Explicit ownership (unique_ptr)

---

## 8. Game Loop & Update Phases

Per the Critical Review (Flaws #6, #9, #10), the game loop is restructured with phase-based scheduling and fixed timestep for physics.

```
┌─────────────────────────────────────────────────┐
│                   Frame Start                    │
├─────────────────────────────────────────────────┤
│  EarlyUpdate                                     │
│    ├─ Input.update()                             │
│    ├─ Network.receive()                          │
│    ├─ Assets.checkForReloads()                   │
│    └─ Events.processQueue()                      │
├─────────────────────────────────────────────────┤
│  FixedUpdate (accumulator loop at fixed dt)      │
│    ├─ Physics2D.update(fixedDt)                  │
│    ├─ Physics3D.update(fixedDt)                  │
│    └─ Network.tickSimulation(fixedDt)            │
├─────────────────────────────────────────────────┤
│  Update (variable dt)                            │
│    ├─ Lua app.update(dt)                         │
│    ├─ AI.update(dt)                              │
│    ├─ Animation.update(dt)                       │
│    ├─ Tween.update(dt)                           │
│    ├─ GAS.update(dt)                             │
│    ├─ Scene.update(dt)                           │
│    └─ GameState.update(dt)                       │
├─────────────────────────────────────────────────┤
│  LateUpdate (variable dt)                        │
│    ├─ Camera.update(dt)                          │
│    ├─ Particle.update(dt)                        │
│    └─ Lua app.lateUpdate(dt)                     │
├─────────────────────────────────────────────────┤
│  PreRender                                       │
│    ├─ Culling                                    │
│    └─ Render queue sorting                       │
├─────────────────────────────────────────────────┤
│  Render                                          │
│    ├─ Graphics.beginFrame()                      │
│    ├─ Scene.render()                             │
│    ├─ Particle.render()                          │
│    ├─ UI.render()                                │
│    ├─ DebugRenderer.flush()                      │
│    ├─ Lua app.render()                           │
│    └─ Graphics.endFrame()                        │
├─────────────────────────────────────────────────┤
│  PostRender                                      │
│    ├─ Network.send()                             │
│    ├─ Metrics.markFrame()                        │
│    └─ State.update(dt) (async save processing)   │
├─────────────────────────────────────────────────┤
│                    Frame End                     │
└─────────────────────────────────────────────────┘
```

**Fixed Timestep Accumulator:**
```cpp
float accumulator = 0;
while (running) {
    float dt = clamp(timer.elapsed(), 0, maxDeltaTime);

    // EarlyUpdate
    earlyUpdate(dt);

    // FixedUpdate — deterministic physics
    accumulator += dt;
    while (accumulator >= fixedTimestep) {
        fixedUpdate(fixedTimestep);
        accumulator -= fixedTimestep;
    }
    float alpha = accumulator / fixedTimestep;  // For interpolation

    // Update
    update(dt);

    // LateUpdate
    lateUpdate(dt);

    // Render (with physics interpolation using alpha)
    render(alpha);

    // PostRender
    postRender(dt);
}
```

---

## 9. Lua API Mapping

### Path Convention

```lua
-- High-level (System contract) — default, most common
bestow.audio.play("sounds/jump.wav")
bestow.entity.create()
bestow.physics.setGravity(0, -9.81)

-- Low-level (Core contract) — full control
bestow.audio.core.playOnChannel(channel, soundHandle, 0.8, false)
bestow.entity.core.emplace(entity, "Transform2D", {x=100, y=200})
bestow.physics.core.createDistanceJoint(bodyA, bodyB, anchorA, anchorB)
```

### Error Handling Convention

```lua
-- All operations that can fail return (value, err)
local entity, err = bestow.entity.create()
if err then
    print("Failed: " .. err.message)
    return
end

-- Operations that return void on success return (nil, err) on failure
local _, err = bestow.physics.createBody(entity, def)
if err then
    print("Physics error: " .. err.message)
end

-- Pure queries that return nil-means-not-found do NOT use error returns
local target = bestow.ai.findClosest(pos)  -- nil = nothing found
```

### Complete Lua Namespace Map

```
bestow.
├── entity.           -- IEntitySystem (create, destroy, addComponent, ...)
│   └── core.         -- IEntityCore (emplace, view, hierarchy, tags, ...)
├── events.           -- IEventCore (publish, subscribe, queue, ...)
├── input.            -- IInputSystem (pushPhase, isKeyDown, ...)
│   └── core.         -- IInputCore (registerAction, haptics, deadzone, ...)
├── audio.            -- IAudioSystem (play, playMusic, setVolume, ...)
│   └── core.         -- IAudioCore (channels, groups, effects, 3D, ...)
├── physics.          -- IPhysicsSystem (createBody, raycast, ...)
│   └── core.         -- IPhysics2DCore (shapes, joints, debug, ...)
├── physics3d.        -- IPhysics3DSystem (createBody, character, ...)
│   └── core.         -- IPhysics3DCore (constraints, vehicles, ...)
├── graphics.         -- IGraphics2DSystem (drawSprite, drawText, ...)
│   └── core.         -- IGraphics2DCore (batch, renderTargets, ...)
├── graphics3d.       -- IGraphics3DSystem (drawModel, setLight, ...)
│   └── core.         -- IGraphics3DCore (mesh/material handles, LOD, ...)
│   └── postfx.       -- IPostProcessingCore (bloom, SSAO, DOF, ...)
│   └── shadows.      -- IShadowCore (resolution, cascades, ...)
│   └── debug.        -- IDebugRendererCore (lines, boxes, spheres, ...)
├── animation.        -- IAnimationSystem (play, stop, speed, ...)
│   └── core.         -- IAnimationCore (blend trees, IK, ragdoll, ...)
│   └── fsm.          -- IAnimationStateMachine (builder, params, ...)
├── camera.           -- ICameraSystem (follow, shake, zoom, ...)
│   └── core.         -- ICameraCore (orbit, FPS, matrices, ...)
├── scene.            -- ISceneSystem (push, pop, replace, ...)
│   └── core.         -- ISceneCore (transitions, events, ...)
├── state.            -- IStateSystem (set, get, save, load, ...)
│   └── core.         -- IStateCore (profiles, migration, export, ...)
├── config.           -- IConfigSystem (getFloat, getBool, ...)
│   └── core.         -- IConfigCore (arrays, tables, hotReload, ...)
├── ui.               -- IUISystem (loadDocument, setText, ...)
│   └── core.         -- IUICore (DOM, data binding, fonts, ...)
├── ai.               -- IAISystem (attachTree, setTarget, ...)
│   └── core.         -- IAICore (navmesh, perception, crowd, ...)
├── gas.              -- IGASSystem (attributes, effects, abilities, ...)
│   └── core.         -- IGASCore (registration, tags, ...)
├── gamestate.        -- IGameStateSystem (push, pop, replace, ...)
├── tween.            -- ITweenSystem (to, toVec2, cancel, ...)
│   └── core.         -- ITweenCore (sequences, custom easing, ...)
├── particles.        -- IParticleSystem (create, play, loadPreset, ...)
│   └── core.         -- IParticleCore (shapes, sub-emitters, GPU, ...)
└── network.          -- INetworkSystem (host, connect, replicate, ...)
    └── core.         -- INetworkCore (RPCs, ownership, replication, ...)
```

---

## 10. Self-Updating Documentation Infrastructure

From V2-CONTRACT-ARCHITECTURE.md — the single-source-of-truth system that prevents docs from drifting.

### SystemDefinition Files

One file per system contains everything: method signatures, docs, Lua examples, validation.

```
bestow-luabind/src/definitions/
├── audio_definition.cpp
├── entity_definition.cpp
├── input_definition.cpp
├── physics_definition.cpp
├── graphics3d_definition.cpp
├── ... (one per system)
```

### Generated Artifacts

| Artifact | Source | Generation |
|----------|--------|-----------|
| EmmyLua stubs | SystemDefinition | `bestow api --stubs` |
| CLI `--help` | SystemDefinition | Runtime (reads definitions) |
| Markdown API docs | SystemDefinition | `bestow api --docs` |
| Binding boilerplate | SystemDefinition | Macro expansion at compile time |

### Compile-Time Validation

```cpp
// In each contract:
static constexpr int kContractVersion = 2;
static constexpr int kMethodCount = 12;  // IAudioSystem methods

// In definition file:
static_assert(IAudioSystem::kMethodCount == audioDefinition.countMethods(Level::System));
static_assert(IAudioCore::kMethodCount == audioDefinition.countMethods(Level::Core));
```

### Doc Test Integration

```cpp
// In definition file, Lua examples are testable:
{
    .method = "play",
    .example = R"lua(
        local sound, err = bestow.audio.play("sounds/coin.wav")
        assert(sound, "play should return a handle")
    )lua",
}
// Test suite loads each example and runs it against a mock system
```

---

## 11. Migration Path

### Priority Order (from Critical Review)

1. **Strong typed handles** — Find-and-replace `using XHandle = uint64_t` with `Handle<XTag>`
2. **Unified error pattern** — Wrap existing returns in `Result<T, SystemError>`
3. **Eliminate std::any** — Replace with typed variants system-by-system
4. **Visibility tiers** — Mark each contract Public/Protected/Internal
5. **Split god interfaces** — Extract PostProcessing, Shadow, DebugRenderer, Text3D from Graphics3D
6. **SystemContext** — Build the struct, wire up one system at a time alongside old DI
7. **Fixed timestep game loop** — Add accumulator, move physics to FixedUpdate
8. **Standardize callbacks** — All subscriptions return SubscriptionId
9. **ECS: EnTT-native + Lua bridge** — Remove type-erased stubs, add LuaEntityBridge
10. **Two-value Lua returns** — Update all Lua bindings to return `(value, err)`
11. **Add new systems** — Tween, Particles, Network
12. **Self-updating docs** — Build SystemDefinition infrastructure

### Compatibility Strategy

- V2 contracts live alongside V1 during migration
- Systems implement both interfaces initially
- V1 interfaces are `[[deprecated]]` with messages pointing to V2
- Once all consumers migrate, V1 interfaces are removed
- Strong handles are the first change because they're zero-overhead and catch bugs immediately

---

## Appendix A: Method Count Summary

| System | System API | Core API | Total |
|--------|-----------|----------|-------|
| Entity | 13 | 35 | 48 |
| Events | 10 (single) | — | 10 |
| Input | 11 | 45 | 56 |
| Audio | 10 | 48 | 58 |
| Physics 2D | 13 | 55 | 68 |
| Physics 3D | 13 | 60 | 73 |
| Graphics 2D | 12 | 35 | 47 |
| Graphics 3D | 10 | 45 | 55 |
| PostProcessing | — | 12 | 12 |
| Shadows | — | 8 | 8 |
| Debug Renderer | — | 12 | 12 |
| Text 3D | — | 4 | 4 |
| Animation | 9 | 38 | 47 |
| Camera | 10 | 35 | 45 |
| Scene | 7 | 14 | 21 |
| State | 12 | 30 | 42 |
| Config | 9 | 28 | 37 |
| UI | 10 | 40 | 50 |
| AI | 8 | 30 | 38 |
| GAS | 11 | 35 | 46 |
| GameState | 12 (single) | — | 12 |
| Tween | 6 | 18 | 24 |
| Particles | 8 | 30 | 38 |
| Network | 11 | 32 | 43 |
| **Totals** | **~218** | **~694** | **~912** |

### Protected System Method Counts

| System | Methods |
|--------|---------|
| Assets (Core) | 42 |
| Shader (Core) | 12 |
| Graphics Context | 5 |
| UI Render Backend | 7 |
| Blueprints | 8 |
| Metrics | 8 |
| **Total** | **~82** |

**Grand Total: ~994 contract methods across all systems.**

---

## Appendix B: Comparison — V1 vs V2

| Aspect | V1 (Current) | V2 (This Document) |
|--------|-------------|---------------------|
| Handle safety | All `uint64_t` | Strong typed `Handle<Tag>` |
| Error pattern | 5 different patterns | Unified `Result<T, SystemError>` |
| Callbacks | 3 patterns, some can't unsub | All return `SubscriptionId` |
| `std::any` usage | Scene params, materials, blackboard | Zero — typed variants everywhere |
| Graphics3D methods | ~250 in one interface | Split into 5 focused contracts |
| DI container | Runtime, fragile order | Compile-time `SystemContext` |
| Game loop | Hardcoded, variable dt physics | Phase-based, fixed timestep |
| Lua errors | Silent `nil` | Two-value `(value, err)` returns |
| Backend leaks | `void* getRenderContext()` | Vendor extension interfaces |
| Missing systems | No Particles, Network, Tween | All three fully specified |
| API levels | Single contract per system | Dual: System (simple) + Core (full) |
| Documentation | Manual, drifts | Self-updating from SystemDefinition |
| Visibility | Everything public | Public / Protected / Internal tiers |
| Entity hierarchy | None | Parent/child + tags + names |
| Physics timestep | Variable (broken) | Fixed accumulator (deterministic) |

