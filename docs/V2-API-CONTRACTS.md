# V2 API Contracts — Bestow Engine

> Comprehensive low-level (Core) and high-level (System) contract definitions for every Bestow engine system.
> These contracts define **interaction patterns**, not implementation details.

## 1. Design Philosophy

### Guiding Principles

| Level | Principle | Meaning |
|-------|-----------|---------|
| **Low-Level (Core)** | Configurability | Every knob exposed. Full control over every aspect of the system. |
| **High-Level (System)** | Usability | Sensible defaults. Simple methods for common operations. |
| **Both** | Abstraction | Contracts define *interaction*, not *implementation*. |

### Contract Rules

1. **Implementation Agnostic** — Contracts specify WHAT you can do with a system, never HOW it works internally. An implementation may use Vulkan or OpenGL, Box2D or Jolt, FMOD or miniaudio — the contract is unchanged.

2. **Dual API** — Every system exposes two interfaces:
   - `IXxxCore` — Low-level. All capabilities. Fine-grained control. Used by peer systems and advanced users.
   - `IXxx` — High-level. Common operations. Sensible defaults. Used by game code and Lua scripts.

3. **Opaque Handles** — All resources identified by typed `Handle<Tag>` values. No raw pointers, no `void*`, no internal state exposure.

4. **Consistent Error Handling** — All fallible operations return `Result<T>` (`std::expected<T, SystemError>`).

5. **Consistent Subscriptions** — All callbacks return `SubscriptionId` with a corresponding `unsubscribe()`. No raw `std::function` setters without cleanup.

6. **No `std::any`** — All dynamic values use bounded `std::variant` types with known alternatives.

7. **Visibility Classification** — Systems are either **Public** (Lua-exposed, game developer facing) or **Protected** (peer-system only, internal engine plumbing).

### What "Implementation Agnostic" Means

```
CONTRACT says:                          IMPLEMENTATION decides:
─────────────────────                   ──────────────────────
"Create a mesh from this data"    →     GPU buffer upload strategy
"Play this sound at position"     →     FMOD vs miniaudio vs OpenAL
"Render this entity"              →     Vulkan vs OpenGL vs Metal
"Find path from A to B"          →     Recast/Detour vs custom navmesh
"Apply force to body"            →     Box2D vs Jolt vs Rapier
```

The contract defines the **vocabulary of interaction**. The implementation chooses the **engine of execution**.

---

## 2. Foundation Types

All systems share these foundational types. Existing types from `bestow.types` (Vec2, Vec3, Transform2D, etc.) are referenced, not redefined.

### 2.1 Typed Handles

```cpp
/// Type-safe resource identifier. Replaces all raw uint64_t aliases.
/// Tag type provides compile-time type safety — you cannot pass a MeshHandle
/// where a MaterialHandle is expected.
template<typename Tag>
struct Handle {
    uint64_t id = 0;
    constexpr bool isValid() const noexcept { return id != 0; }
    constexpr explicit operator bool() const noexcept { return id != 0; }
    constexpr auto operator<=>(const Handle&) const = default;
    constexpr bool operator==(const Handle&) const = default;
};

// Tag types (empty structs, never instantiated)
struct EntityTag {};
struct MeshTag {};
struct MaterialTag {};
struct TextureTag {};
struct ShaderTag {};
struct SoundTag {};
struct FontTag {};
struct ClipTag {};
struct SkeletonTag {};
struct AnimatorTag {};
struct SocketTag {};
struct Body2DTag {};
struct Body3DTag {};
struct Constraint3DTag {};
struct NavMeshTag {};
struct AssetTag {};
struct UIDocumentTag {};
struct UIElementTag {};
struct InstanceBufferTag {};
struct Font3DTag {};
struct ChannelTag {};
struct LightTag {};
struct AbilityTag {};
struct EffectTag {};
struct BehaviorTreeTag {};

// Handle aliases — the public vocabulary
using Entity             = Handle<EntityTag>;
using MeshHandle         = Handle<MeshTag>;
using MaterialHandle     = Handle<MaterialTag>;
using TextureHandle      = Handle<TextureTag>;
using ShaderHandle       = Handle<ShaderTag>;
using SoundHandle        = Handle<SoundTag>;
using FontHandle         = Handle<FontTag>;
using ClipHandle         = Handle<ClipTag>;
using SkeletonHandle     = Handle<SkeletonTag>;
using AnimatorHandle     = Handle<AnimatorTag>;
using SocketHandle       = Handle<SocketTag>;
using BodyHandle2D       = Handle<Body2DTag>;
using BodyHandle3D       = Handle<Body3DTag>;
using ConstraintHandle   = Handle<Constraint3DTag>;
using NavMeshHandle      = Handle<NavMeshTag>;
using AssetHandle        = Handle<AssetTag>;
using DocumentHandle     = Handle<UIDocumentTag>;
using ElementHandle      = Handle<UIElementTag>;
using InstanceBufferHandle = Handle<InstanceBufferTag>;
using Font3DHandle       = Handle<Font3DTag>;
using Channel            = Handle<ChannelTag>;
using LightHandle        = Handle<LightTag>;
using AbilityHandle      = Handle<AbilityTag>;
using EffectHandle       = Handle<EffectTag>;
using BehaviorTreeHandle = Handle<BehaviorTreeTag>;
```

### 2.2 Error Handling

```cpp
/// Unified error type for all system operations.
enum class SystemError : uint32_t {
    None = 0,
    NotFound,           // Resource/key/entity does not exist
    InvalidHandle,      // Handle is expired, null, or wrong type
    InvalidState,       // Operation not valid in current state
    InvalidArgument,    // Parameter value out of range or malformed
    AlreadyExists,      // Resource with that name/id already exists
    NotSupported,       // Operation not available in this implementation
    IOError,            // File system or network failure
    ParseError,         // Lua, JSON, or data format parse failure
    OutOfMemory,        // Allocation failed
    Timeout,            // Operation exceeded time limit
    NotInitialized,     // System not yet initialized
    LimitExceeded,      // Maximum count/size exceeded
    DependencyMissing,  // Required peer system not available
};

/// Result type — all fallible operations return this.
template<typename T>
using Result = std::expected<T, SystemError>;

/// Void result for operations that succeed or fail without a value.
using VoidResult = std::expected<void, SystemError>;
```

### 2.3 Subscription Pattern

```cpp
/// Opaque subscription identifier. Returned by all subscribe/on* methods.
struct SubscriptionId {
    uint64_t id = 0;
    bool isValid() const noexcept { return id != 0; }
};

/// Every system that accepts callbacks MUST provide:
///   SubscriptionId onSomething(callback) — subscribe
///   void unsubscribe(SubscriptionId id)  — unsubscribe
```

### 2.4 Common Numeric Types

```cpp
using DeltaTime  = float;   // Seconds since last frame
using Timestamp  = double;  // Absolute time in seconds
using Volume     = float;   // 0.0 (silent) to 1.0 (full)
using Opacity    = float;   // 0.0 (transparent) to 1.0 (opaque)
```

### 2.5 Dynamic Value Types (Replacing std::any)

```cpp
/// Configuration and blueprint property values.
using PropertyValue = std::variant<
    bool, int, float, double, std::string,
    Vec2, Vec3, Vec4, Color,
    std::vector<int>, std::vector<float>, std::vector<std::string>
>;

/// Material uniform values.
using UniformValue = std::variant<
    float, int, Vec2, Vec3, Vec4, Mat4, TextureHandle
>;

/// AI blackboard values.
using BlackboardValue = std::variant<
    bool, int, float, std::string, Vec3, Entity
>;

/// Component field values for reflection/Lua access.
using FieldValue = std::variant<
    bool, int, float, double, std::string,
    Vec2, Vec3, Vec4, Color, Entity
>;

/// Generic key-value map using PropertyValue (replaces all std::any maps).
using PropertyMap = std::unordered_map<std::string, PropertyValue>;
```

### 2.6 Update Phases

```cpp
/// Defines when a system's update() runs within the frame.
enum class UpdatePhase : uint8_t {
    EarlyUpdate,   // Input polling, event queue processing
    FixedUpdate,   // Physics, deterministic simulation (fixed timestep)
    Update,        // Game logic, AI, animation, general gameplay
    LateUpdate,    // Camera follow, post-logic adjustments
    PreRender,     // Culling, render preparation, GPU upload
    Render,        // Draw calls
    PostRender,    // Debug overlay, UI, frame stats, swap
};
```

### 2.7 Component Type Identity

```cpp
/// Opaque component type identifier for type-erased ECS operations.
/// Actual values are implementation-defined (could be hash, index, etc.).
using ComponentTypeId = uint64_t;

/// Obtain the ComponentTypeId for a C++ type.
/// Implementation-defined — typically uses typeid hash or compile-time counter.
template<typename T>
ComponentTypeId componentTypeId();
```

---

## 3. System Classification

### Public Systems (Game Developer API + Lua Bindings)

| System | Core API | System API | Lua Namespace | Purpose |
|--------|----------|------------|---------------|---------|
| Entity | `IEntityCore` | `IEntity` | `bestow.entity` | Create/query/modify entities and components |
| Graphics 2D | `IGraphics2DCore` | `IGraphics2D` | `bestow.graphics` | 2D sprite/shape/text rendering |
| Graphics 3D | `IGraphics3DCore` | `IGraphics3D` | `bestow.graphics3d` | 3D mesh/material/light rendering |
| Audio | `IAudioCore` | `IAudio` | `bestow.audio` | Sound, music, positional audio |
| Input | `IInputCore` | `IInput` | `bestow.input` | Keyboard, mouse, gamepad, actions |
| Physics 2D | `IPhysics2DCore` | `IPhysics2D` | `bestow.physics` | 2D rigid body simulation |
| Physics 3D | `IPhysics3DCore` | `IPhysics3D` | `bestow.physics3d` | 3D rigid body simulation |
| Animation | `IAnimationCore` | `IAnimation` | `bestow.animation` | Skeletal animation, state machines |
| Camera | `ICameraCore` | `ICamera` | `bestow.camera` | Camera control, following, effects |
| UI | `IUICore` | `IUI` | `bestow.ui` | HTML/CSS-based user interface |
| Scene | `ISceneCore` | `IScene` | `bestow.scene` | Scene stack management |
| Config | `IConfigCore` | `IConfig` | `bestow.config` | Game configuration values |
| State | `IStateCore` | `IState` | `bestow.state` | Save/load, persistence |
| AI | `IAICore` | `IAI` | `bestow.ai` | Behavior trees, navigation, steering |
| GAS | `IGASCore` | `IGAS` | `bestow.gas` | Gameplay abilities, attributes, effects |
| Blueprint | `IBlueprintCore` | `IBlueprint` | `bestow.blueprint` | Entity templates and prefabs |
| Timer | `ITimerCore` | `ITimer` | `bestow.timer` | Delayed and repeating callbacks |

### Protected Systems (Peer-System Only — No Lua Exposure)

| System | Core API | System API | Purpose |
|--------|----------|------------|---------|
| Events | `IEventCore` | `IEvent` | Internal message bus between systems |
| Assets | `IAssetCore` | `IAsset` | File system gateway, caching, hot reload |
| Graphics Context | `IGraphicsContextCore` | — | Backend-agnostic rendering foundation |
| UI Render | `IUIRenderCore` | — | UI geometry compilation and rendering |
| Shader | `IShaderCore` | — | Shader compilation and management |

### Why These Are Protected

- **Events** — Game code uses typed callbacks on public systems (e.g., `physics.onCollision()`), not raw event IDs. Peer systems use Events for decoupled cross-system communication.
- **Assets** — Game code loads assets declaratively through blueprints, configs, and system APIs (e.g., `audio.playMusic("track")`). Peer systems use Assets as the centralized file gateway.
- **Graphics Context** — Backend-specific rendering plumbing. Game code uses Graphics2D/3D.
- **UI Render** — Geometry pipeline for the UI system. Game code uses the UI system's DOM API.
- **Shader** — Shader compilation internals. Game code uses materials.

---

## 4. Protected System Contracts

### 4.1 Event System [PROTECTED]

**Purpose:** Decoupled publish/subscribe message bus for inter-system communication.
**Dependencies:** None (foundation system).

#### Core API

```cpp
class IEventCore {
public:
    virtual ~IEventCore() = default;

    // Lifecycle
    virtual Result<void> initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update() = 0;

    // Type-erased publish/subscribe
    // EventTypeId is a uint64_t hash of the event struct type.

    /// Publish an event immediately (synchronous dispatch to all subscribers)
    virtual void publish(uint64_t eventTypeId, const void* eventData, size_t dataSize) = 0;

    /// Queue an event for deferred dispatch (processed on next update())
    virtual void queue(uint64_t eventTypeId, const void* eventData, size_t dataSize) = 0;

    /// Subscribe to an event type
    virtual SubscriptionId subscribe(uint64_t eventTypeId,
                                     std::function<void(const void*)> handler) = 0;

    /// Unsubscribe
    virtual void unsubscribe(SubscriptionId id) = 0;

    /// Process all queued events
    virtual void processQueue() = 0;

    /// Discard all queued events without processing
    virtual void clearQueue() = 0;

    /// Get count of pending queued events
    virtual size_t getQueueSize() const = 0;
};
```

#### System API

```cpp
/// Non-virtual typed wrappers over IEventCore.
/// These are free functions or a utility class, not a separate interface,
/// because they use templates (which cannot be virtual).

template<typename E>
void publish(IEventCore& events, const E& event);

template<typename E>
void queue(IEventCore& events, const E& event);

template<typename E>
SubscriptionId subscribe(IEventCore& events, std::function<void(const E&)> handler);
```

---

### 4.2 Asset System [PROTECTED]

**Purpose:** Sole gateway to the file system. Centralized caching, hot reload, and lifecycle management for all game assets.
**Dependencies:** Events.

#### Asset Types

```cpp
enum class AssetType : uint8_t {
    Texture, Font, Sound, Music,
    Shader, Mesh, Model, Cubemap,
    LuaScript, LuaConfig, LuaMaterial,
    NavMesh, RmlDocument, Stylesheet,
    Data,  // Generic binary/text data
};

enum class AssetState : uint8_t {
    Unregistered, Registered, Loading, Loaded, Failed, Unloaded,
};

struct AssetInfo {
    AssetHandle handle;
    AssetType type;
    AssetState state;
    std::string path;
    std::string name;
    size_t sizeBytes = 0;
};
```

#### Core API

```cpp
class IAssetCore {
public:
    virtual ~IAssetCore() = default;

    // Lifecycle
    virtual Result<void> initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update() = 0;

    //--- Registration ---

    /// Register an asset path for future loading. Returns a handle.
    virtual Result<AssetHandle> registerAsset(AssetType type, const std::string& path) = 0;

    /// Unregister and release all data for an asset
    virtual void unregisterAsset(AssetHandle handle) = 0;

    /// Check if a path is already registered
    virtual std::optional<AssetHandle> findAsset(const std::string& path) const = 0;

    //--- Loading ---

    /// Load synchronously (blocks until complete)
    virtual Result<void> loadAsset(AssetHandle handle) = 0;

    /// Load asynchronously (completion notified via callback or subscription)
    virtual void loadAssetAsync(AssetHandle handle,
                                std::function<void(AssetHandle, AssetState)> onComplete = nullptr) = 0;

    /// Unload asset data (handle remains valid for re-loading)
    virtual void unloadAsset(AssetHandle handle) = 0;

    /// Reload asset from disk
    virtual Result<void> reloadAsset(AssetHandle handle) = 0;

    //--- Bulk Operations ---

    /// Load all registered assets of a type
    virtual void loadAllOfType(AssetType type) = 0;

    /// Unload all assets of a type
    virtual void unloadAllOfType(AssetType type) = 0;

    //--- Queries ---

    /// Get asset state
    virtual AssetState getAssetState(AssetHandle handle) const = 0;

    /// Get asset info
    virtual std::optional<AssetInfo> getAssetInfo(AssetHandle handle) const = 0;

    /// Get all handles of a type
    virtual std::vector<AssetHandle> getAssetsOfType(AssetType type) const = 0;

    /// Get the raw loaded data (type depends on AssetType)
    /// Returns typed pointer: TextureData*, SoundData*, ShaderData*, etc.
    template<typename T>
    const T* getAsset(AssetHandle handle) const;

    //--- Subscriptions ---

    /// Subscribe to changes for a specific asset
    virtual SubscriptionId subscribe(AssetHandle handle,
                                     std::function<void(AssetHandle, AssetType)> callback) = 0;

    /// Subscribe to ALL assets of a type
    virtual SubscriptionId subscribeToType(AssetType type,
                                           std::function<void(AssetHandle, AssetType)> callback) = 0;

    virtual void unsubscribe(SubscriptionId id) = 0;

    //--- Hot Reload ---

    /// Enable/disable file watching for hot reload
    virtual void enableHotReload(bool enable) = 0;
    virtual bool isHotReloadEnabled() const = 0;

    //--- Library Discovery ---

    /// Discover and register all assets in a directory tree
    virtual Result<void> discoverLibrary(const std::string& rootPath) = 0;

    /// Resolve a path alias (e.g., ":library:/shaders/toon.frag")
    virtual std::string resolvePath(const std::string& aliasedPath) const = 0;
};
```

#### System API

```cpp
/// Peer systems use IAssetCore directly.
/// No separate System API — Assets is a protected system.
/// Game code accesses assets through public system APIs
/// (e.g., audio.playMusic("track"), graphics.loadModel("model.gltf")).
```

---

### 4.3 Graphics Context [PROTECTED]

**Purpose:** Backend-agnostic rendering foundation shared by 2D, 3D, and UI renderers.
**Dependencies:** Assets.

```cpp
class IGraphicsContextCore {
public:
    virtual ~IGraphicsContextCore() = default;

    // Lifecycle
    virtual Result<void> initialize(void* nativeWindow) = 0;
    virtual void shutdown() = 0;

    // Frame management
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;

    // Window queries (implementation-agnostic)
    virtual int getWindowWidth() const = 0;
    virtual int getWindowHeight() const = 0;
    virtual float getAspectRatio() const = 0;

    // Viewport
    virtual void setViewport(int x, int y, int width, int height) = 0;
    virtual void setClearColor(Color color) = 0;

    // UI render backend access (for UI system)
    virtual IUIRenderCore* getUIRenderBackend() = 0;
};
```

---

### 4.4 UI Render Backend [PROTECTED]

**Purpose:** Geometry compilation and rendering pipeline for the UI system.
**Dependencies:** Graphics Context.

```cpp
class IUIRenderCore {
public:
    virtual ~IUIRenderCore() = default;

    // Geometry compilation (called by UI system with vertex/index data)
    virtual Result<uint64_t> compileGeometry(std::span<const std::byte> vertices,
                                              std::span<const uint32_t> indices) = 0;
    virtual void releaseGeometry(uint64_t geometryId) = 0;

    // Texture management
    virtual Result<TextureHandle> createTexture(int width, int height,
                                                 std::span<const std::byte> data) = 0;
    virtual void releaseTexture(TextureHandle handle) = 0;

    // Render pass
    virtual void beginPass(int viewportWidth, int viewportHeight) = 0;
    virtual void renderGeometry(uint64_t geometryId, Vec2 translation,
                                TextureHandle texture) = 0;
    virtual void setScissor(int x, int y, int width, int height) = 0;
    virtual void clearScissor() = 0;
    virtual void endPass() = 0;
};
```

---

### 4.5 Shader System [PROTECTED]

**Purpose:** Shader compilation, caching, and hot reload for rendering backends.
**Dependencies:** Assets, Events.

```cpp
enum class ShaderStage : uint8_t {
    Vertex, Fragment, Geometry, Compute, TessControl, TessEval,
};

class IShaderCore {
public:
    virtual ~IShaderCore() = default;

    virtual Result<void> initialize() = 0;
    virtual void shutdown() = 0;

    /// Compile shader from source code
    virtual Result<ShaderHandle> compileShader(ShaderStage stage,
                                               const std::string& source,
                                               const std::string& name = "") = 0;

    /// Load pre-compiled shader (e.g., SPIR-V)
    virtual Result<ShaderHandle> loadCompiledShader(ShaderStage stage,
                                                     std::span<const std::byte> bytecode,
                                                     const std::string& name = "") = 0;

    /// Load shader from asset system
    virtual Result<ShaderHandle> loadShaderAsset(AssetHandle asset) = 0;

    /// Destroy a shader
    virtual void destroyShader(ShaderHandle handle) = 0;

    /// Create a linked shader program from stages
    virtual Result<ShaderHandle> createProgram(std::span<const ShaderHandle> stages,
                                                const std::string& name = "") = 0;

    /// Hot reload a shader (recompile from asset)
    virtual Result<void> reloadShader(ShaderHandle handle) = 0;

    /// Query shader info
    virtual bool isValid(ShaderHandle handle) const = 0;
};
```

---

## 5. Public System Contracts

### 5.1 Config System [PUBLIC]

**Purpose:** Load and query Lua configuration files. Provides type-safe access to game settings.
**Dependencies:** Assets, Events.
**Lua:** `bestow.config`

#### Core API

```cpp
class IConfigCore {
public:
    virtual ~IConfigCore() = default;

    // Lifecycle
    virtual Result<void> initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update(DeltaTime dt) = 0;

    //--- Loading ---

    /// Load a Lua config file from path (via asset system)
    virtual Result<void> loadConfig(const std::string& filePath) = 0;

    /// Load config from asset handle
    virtual Result<void> loadConfigAsset(AssetHandle configAsset) = 0;

    /// Reload all loaded configs
    virtual Result<void> reloadAll() = 0;

    /// Reload a specific config file
    virtual Result<void> reloadConfig(const std::string& filePath) = 0;

    //--- Type-Safe Getters ---

    virtual std::optional<float> getFloat(const std::string& key) const = 0;
    virtual std::optional<int> getInt(const std::string& key) const = 0;
    virtual std::optional<bool> getBool(const std::string& key) const = 0;
    virtual std::optional<std::string> getString(const std::string& key) const = 0;

    /// Getters with default fallback
    virtual float getFloatOr(const std::string& key, float defaultValue) const = 0;
    virtual int getIntOr(const std::string& key, int defaultValue) const = 0;
    virtual bool getBoolOr(const std::string& key, bool defaultValue) const = 0;
    virtual std::string getStringOr(const std::string& key,
                                     const std::string& defaultValue) const = 0;

    //--- Array Access ---

    virtual std::vector<int> getIntArray(const std::string& key) const = 0;
    virtual std::vector<float> getFloatArray(const std::string& key) const = 0;
    virtual std::vector<std::string> getStringArray(const std::string& key) const = 0;

    //--- Runtime Modification ---

    virtual void setFloat(const std::string& key, float value) = 0;
    virtual void setInt(const std::string& key, int value) = 0;
    virtual void setBool(const std::string& key, bool value) = 0;
    virtual void setString(const std::string& key, const std::string& value) = 0;

    //--- Queries ---

    virtual bool hasKey(const std::string& key) const = 0;
    virtual std::vector<std::string> getKeysWithPrefix(const std::string& prefix) const = 0;
    virtual std::vector<std::string> getLoadedConfigs() const = 0;

    //--- Hot Reload ---

    virtual void enableHotReload(bool enable) = 0;
    virtual bool isHotReloadEnabled() const = 0;

    //--- Change Notifications ---

    virtual SubscriptionId onConfigChanged(std::function<void(const std::string& key)> cb) = 0;
    virtual SubscriptionId onKeyChanged(const std::string& keyPrefix,
                                         std::function<void(const std::string& key)> cb) = 0;
    virtual void unsubscribe(SubscriptionId id) = 0;

    //--- Lua Parsing (for peer systems that need Lua evaluation) ---

    /// Parse a Lua string in the sandboxed environment
    virtual Result<PropertyValue> parseLuaString(const std::string& luaCode,
                                                  const std::string& description = "lua") = 0;

    /// Parse a Lua asset
    virtual Result<PropertyValue> parseLuaAsset(AssetHandle luaAsset,
                                                 const std::string& description = "lua") = 0;

    /// Execute Lua code for side effects
    virtual Result<void> executeLuaString(const std::string& luaCode,
                                           const std::string& description = "lua") = 0;
};
```

#### System API

```cpp
class IConfig {
public:
    virtual ~IConfig() = default;

    /// Get a value with automatic type conversion
    virtual std::optional<PropertyValue> get(const std::string& key) const = 0;

    /// Get with default fallback
    virtual PropertyValue getOr(const std::string& key, const PropertyValue& defaultValue) const = 0;

    /// Set a value at runtime
    virtual void set(const std::string& key, const PropertyValue& value) = 0;

    /// Check if key exists
    virtual bool has(const std::string& key) const = 0;

    /// Subscribe to changes on a key or prefix
    virtual SubscriptionId onChange(const std::string& keyOrPrefix,
                                    std::function<void(const std::string&)> cb) = 0;
    virtual void unsubscribe(SubscriptionId id) = 0;
};
```

---

### 5.2 Entity System [PUBLIC]

**Purpose:** Create, destroy, and query entities. Manage components via ECS.
**Dependencies:** Events.
**Lua:** `bestow.entity`

#### Core API

```cpp
class IEntityCore {
public:
    virtual ~IEntityCore() = default;

    // Lifecycle
    virtual Result<void> initialize() = 0;
    virtual void shutdown() = 0;

    //--- Entity Lifecycle ---

    virtual Entity createEntity() = 0;
    virtual void destroyEntity(Entity entity) = 0;
    virtual bool isValid(Entity entity) const = 0;
    virtual size_t getEntityCount() const = 0;
    virtual void destroyAll() = 0;

    //--- Type-Erased Component Access (virtual interface) ---

    /// Add a component by type ID. Data points to constructed component.
    virtual void* addComponentRaw(Entity entity, ComponentTypeId typeId,
                                   const void* data, size_t dataSize) = 0;

    /// Get component by type ID. Returns nullptr if not present.
    virtual void* getComponentRaw(Entity entity, ComponentTypeId typeId) = 0;
    virtual const void* getComponentRaw(Entity entity, ComponentTypeId typeId) const = 0;

    /// Remove a component by type ID
    virtual bool removeComponent(Entity entity, ComponentTypeId typeId) = 0;

    /// Check if entity has a component
    virtual bool hasComponent(Entity entity, ComponentTypeId typeId) const = 0;

    //--- Reflection-Based Access (for Lua and serialization) ---

    /// Register a component type by name with field descriptors
    virtual Result<void> registerComponentType(const std::string& name,
                                                ComponentTypeId typeId,
                                                size_t size) = 0;

    /// Add component by name
    virtual Result<void> addComponentByName(Entity entity, const std::string& name) = 0;

    /// Get/set component fields by name
    virtual std::optional<FieldValue> getComponentField(Entity entity,
                                                         const std::string& component,
                                                         const std::string& field) const = 0;

    virtual Result<void> setComponentField(Entity entity,
                                            const std::string& component,
                                            const std::string& field,
                                            const FieldValue& value) = 0;

    /// Get all component names on an entity
    virtual std::vector<std::string> getComponentNames(Entity entity) const = 0;

    //--- Entity Queries ---

    /// Find entities that have all specified component types
    virtual std::vector<Entity> query(std::span<const ComponentTypeId> required) const = 0;

    /// Get the first entity with specified components (or invalid handle)
    virtual Entity first(std::span<const ComponentTypeId> required) const = 0;

    /// Iterate entities with callback (type-erased)
    virtual void each(std::span<const ComponentTypeId> required,
                      std::function<void(Entity)> callback) const = 0;

    //--- Tagging ---

    /// Tag entity with a string label
    virtual void tag(Entity entity, const std::string& tag) = 0;
    virtual void untag(Entity entity, const std::string& tag) = 0;
    virtual bool hasTag(Entity entity, const std::string& tag) const = 0;
    virtual std::vector<Entity> getEntitiesWithTag(const std::string& tag) const = 0;

    //--- Lifecycle Notifications ---

    virtual SubscriptionId onEntityCreated(std::function<void(Entity)> cb) = 0;
    virtual SubscriptionId onEntityDestroyed(std::function<void(Entity)> cb) = 0;
    virtual void unsubscribe(SubscriptionId id) = 0;
};

/// Non-virtual typed helpers (free functions or utility template class).
/// These wrap IEntityCore's type-erased methods with compile-time type safety.

template<typename T, typename... Args>
T& emplace(IEntityCore& entities, Entity entity, Args&&... args);

template<typename T>
T& get(IEntityCore& entities, Entity entity);

template<typename T>
const T& get(const IEntityCore& entities, Entity entity);

template<typename T>
bool has(const IEntityCore& entities, Entity entity);

template<typename T>
void remove(IEntityCore& entities, Entity entity);

template<typename... Components>
void each(const IEntityCore& entities, std::function<void(Entity, Components&...)> callback);
```

#### System API

```cpp
class IEntity {
public:
    virtual ~IEntity() = default;

    /// Create a new empty entity
    virtual Entity create() = 0;

    /// Destroy an entity and all its components
    virtual void destroy(Entity entity) = 0;

    /// Check if entity exists
    virtual bool exists(Entity entity) const = 0;

    /// Destroy all entities
    virtual void destroyAll() = 0;

    /// Get total entity count
    virtual size_t count() const = 0;

    //--- String-based component access (Lua-friendly) ---

    /// Add a component by name with optional property overrides
    virtual Result<void> addComponent(Entity entity, const std::string& component,
                                       const PropertyMap& properties = {}) = 0;

    /// Remove a component by name
    virtual Result<void> removeComponent(Entity entity, const std::string& component) = 0;

    /// Get a field value
    virtual std::optional<FieldValue> getField(Entity entity,
                                                const std::string& component,
                                                const std::string& field) const = 0;

    /// Set a field value
    virtual Result<void> setField(Entity entity,
                                   const std::string& component,
                                   const std::string& field,
                                   const FieldValue& value) = 0;

    //--- Simple queries ---

    /// Find entities with a tag
    virtual std::vector<Entity> findByTag(const std::string& tag) const = 0;

    /// Tag/untag
    virtual void tag(Entity entity, const std::string& tag) = 0;
    virtual void untag(Entity entity, const std::string& tag) = 0;
};
```

---

### 5.3 Input System [PUBLIC]

**Purpose:** Handle keyboard, mouse, gamepad input. Action mapping with phase-based contexts.
**Dependencies:** Events.
**Lua:** `bestow.input`

#### Types

```cpp
enum class InputState : uint8_t {
    Idle, Pressed, Held, Released,
};

enum class CursorMode : uint8_t {
    Normal, Hidden, Disabled, // Disabled = captured + hidden
};

/// Describes what physical input triggers an action.
struct InputBinding {
    enum class Type : uint8_t { Key, MouseButton, GamepadButton, GamepadAxis, Scroll };
    Type type;
    int code;            // KeyCode, MouseButton, GamepadButton, or GamepadAxis value
    float threshold = 0.5f; // Axis threshold for digital conversion
};

/// A complete action registration: name, phase, bindings, and behavior.
struct ActionRegistration {
    std::string name;
    std::string phase;
    std::vector<InputBinding> bindings;
    bool consumeInput = true;
    float holdThreshold = 0.0f; // 0 = use system default
};
```

#### Core API

```cpp
class IInputCore {
public:
    virtual ~IInputCore() = default;

    // Lifecycle
    virtual Result<void> initialize(void* nativeWindow) = 0;
    virtual void shutdown() = 0;
    virtual void update() = 0;

    //--- Phase Management ---

    virtual std::string getCurrentPhase() const = 0;
    virtual std::vector<std::string> getPhaseStack() const = 0;
    virtual void pushPhase(const std::string& phase) = 0;
    virtual void popPhase() = 0;
    virtual void changePhase(const std::string& phase) = 0;
    virtual bool isPhaseActive(const std::string& phase) const = 0;

    //--- Action Registration ---

    virtual void registerAction(const ActionRegistration& registration) = 0;
    virtual void unregisterAction(const std::string& actionName) = 0;
    virtual void unregisterPhaseActions(const std::string& phase) = 0;
    virtual void clearActions() = 0;
    virtual std::vector<ActionRegistration> getActions() const = 0;

    //--- Input State Queries ---

    virtual InputState getInputState(const InputBinding& binding) const = 0;
    virtual float getInputHoldDuration(const InputBinding& binding) const = 0;
    virtual void setDefaultHoldThreshold(float seconds) = 0;
    virtual float getDefaultHoldThreshold() const = 0;

    //--- Configuration Loading ---

    virtual Result<void> loadInputConfig(const std::string& path) = 0;
    virtual Result<void> reloadInputConfig() = 0;

    //--- Raw Input for Rebinding ---

    virtual std::optional<InputBinding> getLastInput() const = 0;
    virtual bool isListeningForInput() const = 0;
    virtual void startListeningForInput() = 0;
    virtual void stopListeningForInput() = 0;

    //--- Direct Keyboard ---

    virtual bool isKeyDown(int keyCode) const = 0;
    virtual bool wasKeyJustPressed(int keyCode) const = 0;
    virtual bool wasKeyJustReleased(int keyCode) const = 0;

    //--- Direct Mouse ---

    virtual Vec2 getMousePosition() const = 0;
    virtual Vec2 getMouseDelta() const = 0;
    virtual bool isMouseButtonDown(int button) const = 0;
    virtual bool wasMouseButtonJustPressed(int button) const = 0;
    virtual bool wasMouseButtonJustReleased(int button) const = 0;
    virtual Vec2 getScrollDelta() const = 0;

    //--- Direct Gamepad ---

    virtual bool isGamepadButtonDown(int button, int padIndex = 0) const = 0;
    virtual bool wasGamepadButtonJustPressed(int button, int padIndex = 0) const = 0;
    virtual bool wasGamepadButtonJustReleased(int button, int padIndex = 0) const = 0;
    virtual float getGamepadAxisValue(int axis, int padIndex = 0) const = 0;
    virtual Vec2 getLeftStick(int padIndex = 0) const = 0;
    virtual Vec2 getRightStick(int padIndex = 0) const = 0;

    //--- Modifier Keys ---

    virtual bool isShiftPressed() const = 0;
    virtual bool isCtrlPressed() const = 0;
    virtual bool isAltPressed() const = 0;

    //--- Cursor Control ---

    virtual void showMouseCursor() = 0;
    virtual void hideMouseCursor() = 0;
    virtual bool isMouseCursorVisible() const = 0;
    virtual void setCursorMode(CursorMode mode) = 0;
    virtual CursorMode getCursorMode() const = 0;

    //--- Text Input ---

    virtual void enableTextInput() = 0;
    virtual void disableTextInput() = 0;
    virtual bool isTextInputEnabled() const = 0;
    virtual std::string getTextInput() const = 0;
    virtual void clearTextInput() = 0;

    //--- Controller Info ---

    virtual int getConnectedControllerCount() const = 0;
    virtual bool isControllerConnected(int index) const = 0;
    virtual std::string getControllerName(int index) const = 0;
};
```

#### System API

```cpp
class IInput {
public:
    virtual ~IInput() = default;

    //--- Action Queries (the primary game-developer interface) ---

    /// Check if a named action is currently active (pressed or held)
    virtual bool isActionActive(const std::string& action) const = 0;

    /// Check if action was just pressed this frame
    virtual bool wasActionJustPressed(const std::string& action) const = 0;

    /// Check if action was just released this frame
    virtual bool wasActionJustReleased(const std::string& action) const = 0;

    /// Get analog value for an action (0.0-1.0 for triggers, -1.0-1.0 for axes)
    virtual float getActionValue(const std::string& action) const = 0;

    //--- Quick Access ---

    virtual Vec2 getMousePosition() const = 0;
    virtual Vec2 getMouseDelta() const = 0;

    //--- Phase (context switching) ---

    virtual void pushPhase(const std::string& phase) = 0;
    virtual void popPhase() = 0;
    virtual void changePhase(const std::string& phase) = 0;
    virtual std::string getCurrentPhase() const = 0;
};
```

---

### 5.4 Graphics 2D System [PUBLIC]

**Purpose:** 2D sprite, shape, and text rendering.
**Dependencies:** Assets, Entity, Graphics Context.
**Lua:** `bestow.graphics`

#### Core API

```cpp
class IGraphics2DCore {
public:
    virtual ~IGraphics2DCore() = default;

    // Lifecycle
    virtual Result<void> initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update(DeltaTime dt) = 0;

    //--- Rendering ---

    /// Draw a single sprite
    virtual void drawSprite(const Sprite& sprite, const Transform2D& transform) = 0;

    /// Draw a batch of sprites
    virtual void drawSprites(std::span<const Sprite> sprites,
                              std::span<const Transform2D> transforms) = 0;

    /// Draw sprite sheet frame
    virtual void drawSpriteFrame(TextureHandle sheet, int frameIndex,
                                  const Transform2D& transform, Vec2 frameSize) = 0;

    /// Draw animated sprite (automatically advances frame)
    virtual void drawAnimatedSprite(TextureHandle sheet, int startFrame, int frameCount,
                                     float fps, float elapsed,
                                     const Transform2D& transform, Vec2 frameSize) = 0;

    //--- Primitives ---

    virtual void drawRect(Vec2 position, Vec2 size, Color color, bool filled = true) = 0;
    virtual void drawLine(Vec2 from, Vec2 to, Color color, float thickness = 1.0f) = 0;
    virtual void drawCircle(Vec2 center, float radius, Color color, bool filled = true) = 0;
    virtual void drawPolygon(std::span<const Vec2> points, Color color, bool filled = true) = 0;

    //--- Text ---

    virtual void drawText(const std::string& text, Vec2 position, FontHandle font,
                           float size, Color color = Color{1,1,1,1}) = 0;
    virtual Vec2 measureText(const std::string& text, FontHandle font, float size) const = 0;

    //--- Entity Rendering ---

    /// Render all entities that have Sprite + Transform2D components
    virtual void renderEntities() = 0;

    //--- Camera ---

    virtual void setCamera(const Camera& camera) = 0;
    virtual Camera getCamera() const = 0;

    //--- Render State ---

    virtual void setDrawOrder(int layer) = 0;
    virtual int getWindowWidth() const = 0;
    virtual int getWindowHeight() const = 0;

    //--- Viewport Culling ---

    virtual bool isInViewport(Vec2 position, Vec2 size) const = 0;
};
```

#### System API

```cpp
class IGraphics2D {
public:
    virtual ~IGraphics2D() = default;

    /// Draw a sprite at position
    virtual void drawSprite(const Sprite& sprite, Vec2 position, float rotation = 0.0f) = 0;

    /// Draw text
    virtual void drawText(const std::string& text, Vec2 position,
                           float size = 16.0f, Color color = Color{1,1,1,1}) = 0;

    /// Draw basic shapes
    virtual void drawRect(Vec2 position, Vec2 size, Color color) = 0;
    virtual void drawCircle(Vec2 center, float radius, Color color) = 0;
    virtual void drawLine(Vec2 from, Vec2 to, Color color, float thickness = 1.0f) = 0;

    /// Window queries
    virtual int getWindowWidth() const = 0;
    virtual int getWindowHeight() const = 0;
};
```

---

### 5.5 Graphics 3D System [PUBLIC]

**Purpose:** 3D mesh, material, lighting, and environment rendering.
**Dependencies:** Assets, Entity, Graphics Context, Shader.
**Lua:** `bestow.graphics3d`

> **Note:** The v1 `IGraphics3DSystem` was a ~100+ method god interface. V2 splits it into focused
> sub-concerns while keeping them under a single `IGraphics3DCore` for systems that need the full API.
> Lock-on targeting has been moved to Camera. Debug drawing has been moved to Dev Tools.

#### Types

```cpp
enum class PrimitiveTopology : uint8_t {
    Points, Lines, LineStrip, Triangles, TriangleStrip,
};

enum class MaterialType : uint8_t {
    PBR, Unlit, Custom,
};

enum class LightType : uint8_t {
    Directional, Point, Spot,
};

struct MeshDescriptor {
    std::span<const std::byte> vertexData;
    std::span<const uint32_t> indexData;
    uint32_t vertexStride;
    PrimitiveTopology topology = PrimitiveTopology::Triangles;
    // Vertex layout is implementation-defined based on shader requirements.
    // The contract specifies WHAT data you provide, not HOW the backend interprets it.
};

struct MaterialDescriptor {
    std::string name;
    MaterialType type = MaterialType::PBR;
    std::unordered_map<std::string, UniformValue> properties; // albedo, roughness, etc.
    std::optional<ShaderHandle> customShader;
};

struct LightDescriptor {
    LightType type = LightType::Point;
    Vec3 position = {};
    Vec3 direction = {0, -1, 0};
    Color color = {1, 1, 1, 1};
    float intensity = 1.0f;
    float range = 10.0f;        // Point/Spot
    float innerAngle = 30.0f;   // Spot
    float outerAngle = 45.0f;   // Spot
    bool castShadows = false;
};

struct MeshInfo {
    uint32_t vertexCount;
    uint32_t indexCount;
    Vec3 boundsMin;
    Vec3 boundsMax;
};

struct EnvironmentSettings {
    std::optional<TextureHandle> skyboxCubemap;
    std::optional<TextureHandle> environmentMap;
    Color ambientColor = {0.1f, 0.1f, 0.1f, 1.0f};
    float ambientIntensity = 0.3f;
    bool fogEnabled = false;
    Color fogColor = {0.5f, 0.5f, 0.5f, 1.0f};
    float fogStart = 50.0f;
    float fogEnd = 200.0f;
    float fogDensity = 0.01f;
};

struct ShadowSettings {
    bool enabled = true;
    int resolution = 2048;
    float distance = 100.0f;
    float bias = 0.005f;
    int cascadeCount = 3;
};

struct RenderCommand {
    MeshHandle mesh;
    MaterialHandle material;
    Mat4 transform;
    Entity entity = {};  // Optional entity association
};

struct InstanceData {
    Mat4 transform;
    Color tint = {1, 1, 1, 1};
};

struct RenderStats {
    uint32_t drawCalls = 0;
    uint32_t triangles = 0;
    uint32_t visibleEntities = 0;
    float frameTimeMs = 0.0f;
};
```

#### Core API

```cpp
class IGraphics3DCore {
public:
    virtual ~IGraphics3DCore() = default;

    // Lifecycle
    virtual Result<void> initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update(DeltaTime dt) = 0;

    // Frame management
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;

    //--- Mesh Management ---

    virtual Result<MeshHandle> createMesh(const MeshDescriptor& desc) = 0;
    virtual void destroyMesh(MeshHandle handle) = 0;
    virtual bool isMeshValid(MeshHandle handle) const = 0;
    virtual std::optional<MeshInfo> getMeshInfo(MeshHandle handle) const = 0;

    /// Create mesh from standard primitives
    virtual Result<MeshHandle> createCube(float size = 1.0f) = 0;
    virtual Result<MeshHandle> createSphere(float radius = 0.5f, int segments = 32) = 0;
    virtual Result<MeshHandle> createPlane(float width = 1.0f, float depth = 1.0f) = 0;
    virtual Result<MeshHandle> createCylinder(float radius = 0.5f, float height = 1.0f) = 0;

    //--- Material Management ---

    virtual Result<MaterialHandle> createMaterial(const MaterialDescriptor& desc) = 0;
    virtual void destroyMaterial(MaterialHandle handle) = 0;
    virtual bool isMaterialValid(MaterialHandle handle) const = 0;

    /// Update material properties at runtime
    virtual Result<void> setMaterialProperty(MaterialHandle handle,
                                              const std::string& property,
                                              const UniformValue& value) = 0;
    virtual std::optional<UniformValue> getMaterialProperty(MaterialHandle handle,
                                                             const std::string& property) const = 0;

    /// Set material textures
    virtual Result<void> setMaterialTexture(MaterialHandle handle,
                                             const std::string& slot,
                                             TextureHandle texture) = 0;

    //--- Rendering ---

    /// Submit a single render command
    virtual void submit(const RenderCommand& cmd) = 0;

    /// Submit a batch of render commands
    virtual void submitBatch(std::span<const RenderCommand> commands) = 0;

    /// Render all entities with Mesh + Material + Transform3D components
    virtual void renderEntities() = 0;

    //--- Instanced Rendering ---

    virtual Result<InstanceBufferHandle> createInstanceBuffer(MeshHandle mesh,
                                                               MaterialHandle material,
                                                               size_t maxInstances) = 0;
    virtual void destroyInstanceBuffer(InstanceBufferHandle handle) = 0;
    virtual Result<void> updateInstances(InstanceBufferHandle handle,
                                          std::span<const InstanceData> instances) = 0;
    virtual void renderInstances(InstanceBufferHandle handle) = 0;

    //--- Lighting ---

    virtual Result<LightHandle> createLight(const LightDescriptor& desc) = 0;
    virtual void destroyLight(LightHandle handle) = 0;
    virtual Result<void> updateLight(LightHandle handle, const LightDescriptor& desc) = 0;
    virtual std::optional<LightDescriptor> getLightInfo(LightHandle handle) const = 0;

    //--- Environment ---

    virtual void setEnvironment(const EnvironmentSettings& settings) = 0;
    virtual EnvironmentSettings getEnvironment() const = 0;

    //--- Shadows ---

    virtual void setShadowSettings(const ShadowSettings& settings) = 0;
    virtual ShadowSettings getShadowSettings() const = 0;

    //--- Camera ---

    virtual void setCamera(const Camera3D& camera) = 0;
    virtual Camera3D getCamera() const = 0;

    //--- 3D Text ---

    virtual Result<Font3DHandle> loadFont3D(AssetHandle fontAsset) = 0;
    virtual void destroyFont3D(Font3DHandle handle) = 0;
    virtual void drawText3D(const std::string& text, Font3DHandle font,
                             const Mat4& transform, float size = 1.0f,
                             Color color = {1,1,1,1}) = 0;

    //--- Asset Integration ---

    /// Load mesh from asset system
    virtual Result<MeshHandle> loadMeshAsset(AssetHandle asset) = 0;

    /// Load complete model (mesh + materials + skeleton + animations)
    virtual Result<MeshHandle> loadModel(AssetHandle asset) = 0;

    /// Load material from Lua definition asset
    virtual Result<MaterialHandle> loadMaterialAsset(AssetHandle asset) = 0;

    //--- Window ---

    virtual int getWindowWidth() const = 0;
    virtual int getWindowHeight() const = 0;
    virtual void setWindowTitle(const std::string& title) = 0;
    virtual void setWindowSize(int width, int height) = 0;

    //--- LOD ---

    virtual void setLODDistances(MeshHandle handle, std::span<const float> distances) = 0;

    //--- Stats ---

    virtual RenderStats getStats() const = 0;
};
```

#### System API

```cpp
class IGraphics3D {
public:
    virtual ~IGraphics3D() = default;

    //--- Simple Rendering ---

    /// Load a model and get a handle for rendering
    virtual Result<MeshHandle> loadModel(const std::string& path) = 0;

    /// Draw a mesh with default or specified material
    virtual void draw(MeshHandle mesh, Vec3 position,
                      Quat rotation = Quat{0,0,0,1},
                      Vec3 scale = Vec3{1,1,1}) = 0;

    virtual void draw(MeshHandle mesh, const Mat4& transform) = 0;

    virtual void draw(MeshHandle mesh, MaterialHandle material, const Mat4& transform) = 0;

    //--- Quick Material ---

    virtual Result<MaterialHandle> createPBRMaterial(Color albedo, float roughness = 0.5f,
                                                      float metallic = 0.0f) = 0;

    virtual Result<MaterialHandle> createUnlitMaterial(Color color) = 0;

    //--- Quick Lighting ---

    virtual Result<LightHandle> addDirectionalLight(Vec3 direction, Color color = {1,1,1,1},
                                                     float intensity = 1.0f) = 0;

    virtual Result<LightHandle> addPointLight(Vec3 position, Color color = {1,1,1,1},
                                               float intensity = 1.0f, float range = 10.0f) = 0;

    //--- Environment ---

    virtual void setAmbientLight(Color color, float intensity = 0.3f) = 0;
    virtual void enableFog(Color color, float start, float end) = 0;
    virtual void disableFog() = 0;
    virtual Result<void> setSkybox(const std::string& cubemapPath) = 0;

    //--- Window ---

    virtual int getWindowWidth() const = 0;
    virtual int getWindowHeight() const = 0;
};
```

---

### 5.6 Audio System [PUBLIC]

**Purpose:** Sound effects, music, and positional 3D audio.
**Dependencies:** Assets, Events.
**Lua:** `bestow.audio`

#### Types

```cpp
struct ChannelSound {
    AssetHandle asset;
    Volume volume = 1.0f;
    float pitch = 1.0f;
    bool loop = false;
    float fadeInTime = 0.0f;
};

struct PositionalSound {
    AssetHandle asset;
    Vec3 position;
    Volume volume = 1.0f;
    float pitch = 1.0f;
    float minDistance = 1.0f;
    float maxDistance = 100.0f;
    bool loop = false;
};

struct AudioListener {
    Vec3 position;
    Vec3 forward = {0, 0, -1};
    Vec3 up = {0, 1, 0};
};

struct ChannelState {
    bool isPlaying = false;
    bool isPaused = false;
    float position = 0.0f;
    float length = 0.0f;
    Volume volume = 1.0f;
};
```

#### Core API

```cpp
class IAudioCore {
public:
    virtual ~IAudioCore() = default;

    // Lifecycle
    virtual Result<void> initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update(DeltaTime dt) = 0;

    //--- Channel-Based Audio (Managed) ---

    virtual void playOnChannel(Channel channel, const ChannelSound& sound) = 0;
    virtual void stopChannel(Channel channel, float fadeOutTime = 0.0f) = 0;
    virtual void pauseChannel(Channel channel) = 0;
    virtual void resumeChannel(Channel channel) = 0;

    virtual void setChannelVolume(Channel channel, Volume volume) = 0;
    virtual void setChannelPitch(Channel channel, float pitch) = 0;
    virtual void seekChannel(Channel channel, float position) = 0;

    virtual ChannelState getChannelState(Channel channel) const = 0;
    virtual bool isChannelPlaying(Channel channel) const = 0;

    //--- Positional Audio (Fire & Forget) ---

    virtual Result<SoundHandle> playPositional(const PositionalSound& sound) = 0;
    virtual void stopPositional(SoundHandle handle) = 0;
    virtual void updatePositionalPosition(SoundHandle handle, Vec3 position) = 0;
    virtual bool isPositionalPlaying(SoundHandle handle) const = 0;

    //--- 3D Audio Listener ---

    virtual void setListener(const AudioListener& listener) = 0;
    virtual AudioListener getListener() const = 0;

    //--- Global Controls ---

    virtual void setMasterVolume(Volume volume) = 0;
    virtual Volume getMasterVolume() const = 0;

    virtual void pauseAll() = 0;
    virtual void resumeAll() = 0;
    virtual void stopAll() = 0;

    //--- Channel Groups ---

    virtual void setGroupVolume(const std::string& group, Volume volume) = 0;
    virtual void assignChannelToGroup(Channel channel, const std::string& group) = 0;
};
```

#### System API

```cpp
class IAudio {
public:
    virtual ~IAudio() = default;

    /// Play background music (auto-manages a dedicated channel)
    virtual void playMusic(const std::string& trackPath, bool loop = true,
                            float fadeIn = 1.0f) = 0;

    /// Stop current music
    virtual void stopMusic(float fadeOut = 1.0f) = 0;

    /// Play a one-shot sound effect
    virtual void playSFX(const std::string& soundPath, float volume = 1.0f) = 0;

    /// Play a sound at a 3D position
    virtual void playSFXAt(const std::string& soundPath, Vec3 position,
                            float volume = 1.0f) = 0;

    /// Volume controls
    virtual void setMasterVolume(Volume volume) = 0;
    virtual void setMusicVolume(Volume volume) = 0;
    virtual void setSFXVolume(Volume volume) = 0;

    /// Pause/resume all audio
    virtual void pauseAll() = 0;
    virtual void resumeAll() = 0;
};
```

---

### 5.7 Physics 2D System [PUBLIC]

**Purpose:** 2D rigid body physics, collision detection, and spatial queries.
**Dependencies:** Entity, Events.
**Lua:** `bestow.physics`

#### Types

```cpp
enum class BodyType2D : uint8_t {
    Static, Kinematic, Dynamic,
};

enum class ShapeType2D : uint8_t {
    Box, Circle, Capsule, Polygon, Edge, Chain,
};

struct PhysicsBodyDef2D {
    BodyType2D type = BodyType2D::Dynamic;
    Vec2 position = {};
    float rotation = 0.0f;
    float density = 1.0f;
    float friction = 0.3f;
    float restitution = 0.0f;
    float linearDamping = 0.0f;
    float angularDamping = 0.01f;
    bool fixedRotation = false;
    bool isSensor = false;
    uint16_t categoryBits = 0x0001;
    uint16_t maskBits = 0xFFFF;
};

struct ShapeDef2D {
    ShapeType2D type = ShapeType2D::Box;
    Vec2 size = {1.0f, 1.0f};   // Box: half-extents
    float radius = 0.5f;         // Circle/Capsule
    float height = 1.0f;         // Capsule
    std::vector<Vec2> vertices;  // Polygon
    Vec2 offset = {};            // Shape offset from body center
};

struct RaycastHit2D {
    Entity entity;
    Vec2 point;
    Vec2 normal;
    float fraction;
};

struct CollisionInfo2D {
    Entity entityA;
    Entity entityB;
    Vec2 contactPoint;
    Vec2 contactNormal;
    float impulse;
};
```

#### Core API

```cpp
class IPhysics2DCore {
public:
    virtual ~IPhysics2DCore() = default;

    // Lifecycle
    virtual Result<void> initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update(DeltaTime fixedDt) = 0;

    //--- Body Management ---

    virtual Result<BodyHandle2D> createBody(Entity entity, const PhysicsBodyDef2D& def) = 0;
    virtual void destroyBody(BodyHandle2D handle) = 0;
    virtual bool isBodyValid(BodyHandle2D handle) const = 0;

    /// Get the body associated with an entity
    virtual std::optional<BodyHandle2D> getBody(Entity entity) const = 0;

    //--- Body Properties ---

    virtual void setBodyType(BodyHandle2D handle, BodyType2D type) = 0;
    virtual BodyType2D getBodyType(BodyHandle2D handle) const = 0;

    virtual void setPosition(BodyHandle2D handle, Vec2 position) = 0;
    virtual Vec2 getPosition(BodyHandle2D handle) const = 0;

    virtual void setRotation(BodyHandle2D handle, float radians) = 0;
    virtual float getRotation(BodyHandle2D handle) const = 0;

    virtual void setLinearVelocity(BodyHandle2D handle, Vec2 velocity) = 0;
    virtual Vec2 getLinearVelocity(BodyHandle2D handle) const = 0;

    virtual void setAngularVelocity(BodyHandle2D handle, float omega) = 0;
    virtual float getAngularVelocity(BodyHandle2D handle) const = 0;

    virtual void setLinearDamping(BodyHandle2D handle, float damping) = 0;
    virtual void setAngularDamping(BodyHandle2D handle, float damping) = 0;
    virtual void setFixedRotation(BodyHandle2D handle, bool fixed) = 0;

    //--- Shapes ---

    virtual Result<void> addShape(BodyHandle2D handle, const ShapeDef2D& shape) = 0;

    //--- Forces ---

    virtual void applyForce(BodyHandle2D handle, Vec2 force) = 0;
    virtual void applyForceAtPoint(BodyHandle2D handle, Vec2 force, Vec2 point) = 0;
    virtual void applyImpulse(BodyHandle2D handle, Vec2 impulse) = 0;
    virtual void applyImpulseAtPoint(BodyHandle2D handle, Vec2 impulse, Vec2 point) = 0;
    virtual void applyTorque(BodyHandle2D handle, float torque) = 0;

    //--- Collision Filtering ---

    virtual void setCategoryBits(BodyHandle2D handle, uint16_t bits) = 0;
    virtual void setMaskBits(BodyHandle2D handle, uint16_t bits) = 0;
    virtual void setSensor(BodyHandle2D handle, bool isSensor) = 0;

    //--- Spatial Queries ---

    virtual std::optional<RaycastHit2D> raycast(Vec2 from, Vec2 to,
                                                  uint16_t maskBits = 0xFFFF) const = 0;
    virtual std::vector<RaycastHit2D> raycastAll(Vec2 from, Vec2 to,
                                                   uint16_t maskBits = 0xFFFF) const = 0;

    virtual std::vector<Entity> queryAABB(Vec2 min, Vec2 max,
                                           uint16_t maskBits = 0xFFFF) const = 0;
    virtual std::vector<Entity> queryCircle(Vec2 center, float radius,
                                             uint16_t maskBits = 0xFFFF) const = 0;

    //--- Ground Detection ---

    virtual bool isOnGround(BodyHandle2D handle) const = 0;
    virtual std::optional<Vec2> getGroundNormal(BodyHandle2D handle) const = 0;

    //--- World Settings ---

    virtual void setGravity(Vec2 gravity) = 0;
    virtual Vec2 getGravity() const = 0;

    //--- Collision Callbacks ---

    virtual SubscriptionId onCollisionBegin(std::function<void(const CollisionInfo2D&)> cb) = 0;
    virtual SubscriptionId onCollisionEnd(std::function<void(Entity, Entity)> cb) = 0;
    virtual SubscriptionId onSensorEnter(std::function<void(Entity sensor, Entity other)> cb) = 0;
    virtual SubscriptionId onSensorExit(std::function<void(Entity sensor, Entity other)> cb) = 0;
    virtual void unsubscribe(SubscriptionId id) = 0;
};
```

#### System API

```cpp
class IPhysics2D {
public:
    virtual ~IPhysics2D() = default;

    /// Create a physics body on an entity
    virtual Result<void> createBody(Entity entity, BodyType2D type,
                                     const ShapeDef2D& shape) = 0;

    /// Remove physics from an entity
    virtual void removeBody(Entity entity) = 0;

    /// Apply force to entity
    virtual void applyForce(Entity entity, Vec2 force) = 0;

    /// Apply impulse to entity
    virtual void applyImpulse(Entity entity, Vec2 impulse) = 0;

    /// Set velocity directly
    virtual void setVelocity(Entity entity, Vec2 velocity) = 0;
    virtual Vec2 getVelocity(Entity entity) const = 0;

    /// Raycast
    virtual std::optional<RaycastHit2D> raycast(Vec2 from, Vec2 to) const = 0;

    /// Ground check
    virtual bool isOnGround(Entity entity) const = 0;

    /// Set world gravity
    virtual void setGravity(Vec2 gravity) = 0;

    /// Collision callbacks
    virtual SubscriptionId onCollision(std::function<void(const CollisionInfo2D&)> cb) = 0;
    virtual void unsubscribe(SubscriptionId id) = 0;
};
```

---

### 5.8 Physics 3D System [PUBLIC]

**Purpose:** 3D rigid body physics, constraints, character controller, and vehicle simulation.
**Dependencies:** Entity, Events.
**Lua:** `bestow.physics3d`

#### Types

```cpp
enum class BodyType3D : uint8_t {
    Static, Kinematic, Dynamic,
};

enum class ShapeType3D : uint8_t {
    Box, Sphere, Capsule, Cylinder, ConvexHull, TriangleMesh, HeightField,
};

enum class MotionQuality : uint8_t {
    Discrete,        // Standard collision detection
    LinearCast,      // CCD for fast-moving objects
};

enum class ConstraintType : uint8_t {
    Hinge, Slider, Cone, Point, Distance, Fixed,
};

struct PhysicsBodyDef3D {
    BodyType3D type = BodyType3D::Dynamic;
    Vec3 position = {};
    Quat rotation = {0, 0, 0, 1};
    float mass = 1.0f;
    float friction = 0.5f;
    float restitution = 0.3f;
    float linearDamping = 0.05f;
    float angularDamping = 0.05f;
    MotionQuality motionQuality = MotionQuality::Discrete;
    bool isSensor = false;
    uint16_t layer = 0;
};

struct ShapeDef3D {
    ShapeType3D type = ShapeType3D::Box;
    Vec3 halfExtents = {0.5f, 0.5f, 0.5f}; // Box
    float radius = 0.5f;                      // Sphere/Capsule/Cylinder
    float height = 1.0f;                      // Capsule/Cylinder
    Vec3 offset = {};
    Quat offsetRotation = {0, 0, 0, 1};
};

struct RaycastHit3D {
    Entity entity;
    BodyHandle3D body;
    Vec3 point;
    Vec3 normal;
    float fraction;
};

struct CollisionInfo3D {
    Entity entityA;
    Entity entityB;
    Vec3 contactPoint;
    Vec3 contactNormal;
    float penetrationDepth;
};

struct ConstraintDef {
    ConstraintType type;
    BodyHandle3D bodyA;
    BodyHandle3D bodyB;
    Vec3 pivotA = {};
    Vec3 pivotB = {};
    Vec3 axisA = {1, 0, 0};
    Vec3 axisB = {1, 0, 0};
    float minLimit = 0.0f;
    float maxLimit = 0.0f;
};

struct CharacterControllerDef {
    float height = 1.8f;
    float radius = 0.3f;
    float mass = 80.0f;
    float maxSlopeAngle = 45.0f;
    float stepHeight = 0.3f;
};
```

#### Core API

```cpp
class IPhysics3DCore {
public:
    virtual ~IPhysics3DCore() = default;

    // Lifecycle
    virtual Result<void> initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update(DeltaTime fixedDt) = 0;

    //--- Body Management ---

    virtual Result<BodyHandle3D> createBody(Entity entity, const PhysicsBodyDef3D& def) = 0;
    virtual void destroyBody(BodyHandle3D handle) = 0;
    virtual bool isBodyValid(BodyHandle3D handle) const = 0;
    virtual std::optional<BodyHandle3D> getBody(Entity entity) const = 0;

    //--- Shapes (compound bodies) ---

    virtual Result<void> addShape(BodyHandle3D handle, const ShapeDef3D& shape) = 0;

    //--- Transform ---

    virtual void setPosition(BodyHandle3D handle, Vec3 position) = 0;
    virtual Vec3 getPosition(BodyHandle3D handle) const = 0;
    virtual void setRotation(BodyHandle3D handle, Quat rotation) = 0;
    virtual Quat getRotation(BodyHandle3D handle) const = 0;

    //--- Velocity ---

    virtual void setLinearVelocity(BodyHandle3D handle, Vec3 velocity) = 0;
    virtual Vec3 getLinearVelocity(BodyHandle3D handle) const = 0;
    virtual void setAngularVelocity(BodyHandle3D handle, Vec3 omega) = 0;
    virtual Vec3 getAngularVelocity(BodyHandle3D handle) const = 0;

    //--- Forces ---

    virtual void applyForce(BodyHandle3D handle, Vec3 force) = 0;
    virtual void applyForceAtPoint(BodyHandle3D handle, Vec3 force, Vec3 point) = 0;
    virtual void applyImpulse(BodyHandle3D handle, Vec3 impulse) = 0;
    virtual void applyImpulseAtPoint(BodyHandle3D handle, Vec3 impulse, Vec3 point) = 0;
    virtual void applyTorque(BodyHandle3D handle, Vec3 torque) = 0;
    virtual void applyTorqueImpulse(BodyHandle3D handle, Vec3 torqueImpulse) = 0;

    //--- Body Properties ---

    virtual void setBodyType(BodyHandle3D handle, BodyType3D type) = 0;
    virtual void setMass(BodyHandle3D handle, float mass) = 0;
    virtual void setFriction(BodyHandle3D handle, float friction) = 0;
    virtual void setRestitution(BodyHandle3D handle, float restitution) = 0;
    virtual void setLinearDamping(BodyHandle3D handle, float damping) = 0;
    virtual void setAngularDamping(BodyHandle3D handle, float damping) = 0;
    virtual void setGravityFactor(BodyHandle3D handle, float factor) = 0;
    virtual void setMotionQuality(BodyHandle3D handle, MotionQuality quality) = 0;

    //--- Sleep State ---

    virtual bool isActive(BodyHandle3D handle) const = 0;
    virtual void activate(BodyHandle3D handle) = 0;
    virtual void deactivate(BodyHandle3D handle) = 0;

    //--- Spatial Queries ---

    virtual std::optional<RaycastHit3D> raycast(Vec3 from, Vec3 to,
                                                  uint16_t layerMask = 0xFFFF) const = 0;
    virtual std::vector<RaycastHit3D> raycastAll(Vec3 from, Vec3 to,
                                                   uint16_t layerMask = 0xFFFF) const = 0;

    virtual std::optional<RaycastHit3D> sphereCast(Vec3 from, Vec3 to, float radius,
                                                     uint16_t layerMask = 0xFFFF) const = 0;
    virtual std::optional<RaycastHit3D> boxCast(Vec3 from, Vec3 to, Vec3 halfExtents,
                                                  Quat orientation = {0,0,0,1},
                                                  uint16_t layerMask = 0xFFFF) const = 0;

    virtual std::vector<Entity> overlapSphere(Vec3 center, float radius,
                                               uint16_t layerMask = 0xFFFF) const = 0;
    virtual std::vector<Entity> overlapBox(Vec3 center, Vec3 halfExtents,
                                            Quat orientation = {0,0,0,1},
                                            uint16_t layerMask = 0xFFFF) const = 0;

    //--- Constraints ---

    virtual Result<ConstraintHandle> createConstraint(const ConstraintDef& def) = 0;
    virtual void destroyConstraint(ConstraintHandle handle) = 0;
    virtual Result<void> setConstraintLimits(ConstraintHandle handle,
                                              float min, float max) = 0;
    virtual Result<void> setConstraintMotor(ConstraintHandle handle,
                                             float targetVelocity, float maxForce) = 0;

    //--- Character Controller ---

    virtual Result<BodyHandle3D> createCharacterController(Entity entity,
                                                            const CharacterControllerDef& def) = 0;
    virtual void moveCharacter(BodyHandle3D handle, Vec3 displacement, DeltaTime dt) = 0;
    virtual bool isCharacterGrounded(BodyHandle3D handle) const = 0;
    virtual Vec3 getCharacterGroundNormal(BodyHandle3D handle) const = 0;

    //--- World Settings ---

    virtual void setGravity(Vec3 gravity) = 0;
    virtual Vec3 getGravity() const = 0;

    //--- Collision Callbacks ---

    virtual SubscriptionId onCollisionBegin(std::function<void(const CollisionInfo3D&)> cb) = 0;
    virtual SubscriptionId onCollisionEnd(std::function<void(Entity, Entity)> cb) = 0;
    virtual SubscriptionId onSensorEnter(std::function<void(Entity sensor, Entity other)> cb) = 0;
    virtual SubscriptionId onSensorExit(std::function<void(Entity sensor, Entity other)> cb) = 0;
    virtual void unsubscribe(SubscriptionId id) = 0;

    //--- Debug ---

    virtual void enableDebugVisualization(bool enable) = 0;
};
```

#### System API

```cpp
class IPhysics3D {
public:
    virtual ~IPhysics3D() = default;

    /// Create a physics body on an entity
    virtual Result<void> createBody(Entity entity, BodyType3D type,
                                     const ShapeDef3D& shape, float mass = 1.0f) = 0;

    /// Remove physics from an entity
    virtual void removeBody(Entity entity) = 0;

    /// Apply force/impulse
    virtual void applyForce(Entity entity, Vec3 force) = 0;
    virtual void applyImpulse(Entity entity, Vec3 impulse) = 0;

    /// Velocity
    virtual void setVelocity(Entity entity, Vec3 velocity) = 0;
    virtual Vec3 getVelocity(Entity entity) const = 0;

    /// Raycast
    virtual std::optional<RaycastHit3D> raycast(Vec3 from, Vec3 to) const = 0;

    /// Character controller
    virtual Result<void> createCharacter(Entity entity, float height = 1.8f,
                                          float radius = 0.3f) = 0;
    virtual void moveCharacter(Entity entity, Vec3 displacement, DeltaTime dt) = 0;
    virtual bool isGrounded(Entity entity) const = 0;

    /// World gravity
    virtual void setGravity(Vec3 gravity) = 0;

    /// Collision callbacks
    virtual SubscriptionId onCollision(std::function<void(const CollisionInfo3D&)> cb) = 0;
    virtual void unsubscribe(SubscriptionId id) = 0;
};
```

---

### 5.9 Animation System [PUBLIC]

**Purpose:** Skeletal animation, blending, state machines, IK, sockets, and ragdoll.
**Dependencies:** Entity, Assets, Events.
**Lua:** `bestow.animation`

> **Note:** Skeletal mesh rendering is handled by IGraphics3DCore.
> Animation provides bone transforms; Graphics3D renders the skinned mesh.

#### Types

```cpp
enum class AnimationWrapMode : uint8_t {
    Once, Loop, PingPong, ClampForever,
};

enum class IKType : uint8_t {
    TwoBone, Aim,
};

struct AnimationEventDef {
    std::string name;
    float time;              // Normalized time (0.0 - 1.0)
    PropertyMap data = {};   // Optional event payload
};

struct BoneTransform {
    Vec3 position;
    Quat rotation;
    Vec3 scale = {1, 1, 1};
};

struct BoneInfo {
    std::string name;
    int index;
    int parentIndex;   // -1 for root
};

struct ClipInfo {
    std::string name;
    float duration;       // Seconds
    float sampleRate;
    int boneCount;
};

struct AnimatorState {
    ClipHandle currentClip;
    float normalizedTime;   // 0.0 - 1.0
    float speed;
    bool isPlaying;
    int activeLayerCount;
};

struct IKTarget {
    IKType type;
    std::string chainName;
    Vec3 targetPosition;
    std::optional<Quat> targetRotation;
    float weight = 1.0f;
};
```

#### Core API

```cpp
class IAnimationCore {
public:
    virtual ~IAnimationCore() = default;

    // Lifecycle
    virtual Result<void> initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update(DeltaTime dt) = 0;

    //--- Skeleton Management ---

    virtual Result<SkeletonHandle> createSkeleton(const std::string& name,
                                                    std::span<const BoneInfo> bones) = 0;
    virtual Result<SkeletonHandle> loadSkeleton(AssetHandle asset) = 0;
    virtual void destroySkeleton(SkeletonHandle handle) = 0;
    virtual bool isSkeletonValid(SkeletonHandle handle) const = 0;

    virtual int getBoneCount(SkeletonHandle handle) const = 0;
    virtual std::optional<int> findBone(SkeletonHandle handle,
                                         const std::string& boneName) const = 0;
    virtual std::optional<BoneInfo> getBoneInfo(SkeletonHandle handle, int boneIndex) const = 0;

    //--- Clip Management ---

    virtual Result<ClipHandle> loadClip(AssetHandle asset, const std::string& clipName = "") = 0;
    virtual void destroyClip(ClipHandle handle) = 0;
    virtual bool isClipValid(ClipHandle handle) const = 0;
    virtual std::optional<ClipInfo> getClipInfo(ClipHandle handle) const = 0;

    /// Define animation events on a clip
    virtual Result<void> addClipEvent(ClipHandle handle, const AnimationEventDef& event) = 0;

    //--- Animator (Per-Entity Animation Controller) ---

    virtual Result<AnimatorHandle> createAnimator(Entity entity,
                                                    SkeletonHandle skeleton) = 0;
    virtual void destroyAnimator(AnimatorHandle handle) = 0;
    virtual bool isAnimatorValid(AnimatorHandle handle) const = 0;
    virtual std::optional<AnimatorHandle> getAnimator(Entity entity) const = 0;

    //--- Playback Control ---

    virtual void play(AnimatorHandle handle, ClipHandle clip,
                      AnimationWrapMode wrap = AnimationWrapMode::Loop,
                      float crossfadeTime = 0.0f) = 0;
    virtual void stop(AnimatorHandle handle) = 0;
    virtual void pause(AnimatorHandle handle) = 0;
    virtual void resume(AnimatorHandle handle) = 0;

    virtual void setSpeed(AnimatorHandle handle, float speed) = 0;
    virtual float getSpeed(AnimatorHandle handle) const = 0;

    virtual void setNormalizedTime(AnimatorHandle handle, float t) = 0;
    virtual float getNormalizedTime(AnimatorHandle handle) const = 0;

    virtual AnimatorState getState(AnimatorHandle handle) const = 0;

    //--- Layers ---

    virtual Result<void> playOnLayer(AnimatorHandle handle, int layer, ClipHandle clip,
                                      float weight = 1.0f,
                                      AnimationWrapMode wrap = AnimationWrapMode::Loop) = 0;
    virtual void setLayerWeight(AnimatorHandle handle, int layer, float weight) = 0;
    virtual float getLayerWeight(AnimatorHandle handle, int layer) const = 0;
    virtual void setLayerBoneMask(AnimatorHandle handle, int layer,
                                   std::span<const std::string> boneNames) = 0;

    //--- Bone Transforms (output) ---

    virtual std::optional<BoneTransform> getBoneWorldTransform(AnimatorHandle handle,
                                                                int boneIndex) const = 0;
    virtual std::optional<BoneTransform> getBoneLocalTransform(AnimatorHandle handle,
                                                                int boneIndex) const = 0;

    /// Get all bone matrices for GPU skinning
    virtual std::vector<Mat4> getSkinningMatrices(AnimatorHandle handle) const = 0;

    //--- Sockets (Attachment Points) ---

    virtual Result<SocketHandle> createSocket(AnimatorHandle handle,
                                               const std::string& boneName,
                                               const std::string& socketName,
                                               const BoneTransform& offset = {}) = 0;
    virtual void destroySocket(SocketHandle handle) = 0;

    /// Get the world-space transform of a socket
    virtual std::optional<Mat4> getSocketTransform(SocketHandle handle) const = 0;

    //--- IK ---

    virtual Result<void> createIKChain(AnimatorHandle handle, const std::string& chainName,
                                        IKType type,
                                        std::span<const std::string> boneNames) = 0;
    virtual void destroyIKChain(AnimatorHandle handle, const std::string& chainName) = 0;
    virtual void setIKTarget(AnimatorHandle handle, const IKTarget& target) = 0;
    virtual void clearIKTarget(AnimatorHandle handle, const std::string& chainName) = 0;

    //--- Root Motion ---

    virtual void enableRootMotion(AnimatorHandle handle, bool enable) = 0;
    virtual Vec3 getRootMotionDelta(AnimatorHandle handle) const = 0;
    virtual Quat getRootMotionRotationDelta(AnimatorHandle handle) const = 0;

    //--- Ragdoll ---

    virtual Result<void> createRagdoll(AnimatorHandle handle,
                                        std::span<const std::string> physicsBones) = 0;
    virtual void destroyRagdoll(AnimatorHandle handle) = 0;
    virtual void enableRagdoll(AnimatorHandle handle) = 0;
    virtual void disableRagdoll(AnimatorHandle handle) = 0;
    virtual bool isRagdollActive(AnimatorHandle handle) const = 0;
    virtual void blendToRagdoll(AnimatorHandle handle, float duration) = 0;
    virtual void blendFromRagdoll(AnimatorHandle handle, ClipHandle clip, float duration) = 0;

    //--- Callbacks ---

    virtual SubscriptionId onAnimationEvent(AnimatorHandle handle,
                                             std::function<void(const std::string& eventName,
                                                                const PropertyMap& data)> cb) = 0;
    virtual SubscriptionId onAnimationComplete(AnimatorHandle handle,
                                                std::function<void(ClipHandle)> cb) = 0;
    virtual void unsubscribe(SubscriptionId id) = 0;

    //--- State Machine ---

    /// Create a state machine for an animator from a Lua definition
    virtual Result<void> loadStateMachine(AnimatorHandle handle, AssetHandle luaAsset) = 0;

    /// State machine parameter control
    virtual void setParameter(AnimatorHandle handle, const std::string& name, bool value) = 0;
    virtual void setParameter(AnimatorHandle handle, const std::string& name, int value) = 0;
    virtual void setParameter(AnimatorHandle handle, const std::string& name, float value) = 0;
    virtual void setTrigger(AnimatorHandle handle, const std::string& name) = 0;

    /// Query current state machine state
    virtual std::optional<std::string> getCurrentStateName(AnimatorHandle handle) const = 0;
};
```

#### System API

```cpp
class IAnimation {
public:
    virtual ~IAnimation() = default;

    /// Play a named animation on an entity (with optional crossfade)
    virtual Result<void> play(Entity entity, const std::string& clipName,
                               bool loop = true, float crossfade = 0.2f) = 0;

    /// Stop animation
    virtual void stop(Entity entity) = 0;

    /// Crossfade to a new animation
    virtual Result<void> crossfade(Entity entity, const std::string& clipName,
                                    float duration = 0.3f) = 0;

    /// Set playback speed
    virtual void setSpeed(Entity entity, float speed) = 0;

    /// Check if animation is playing
    virtual bool isPlaying(Entity entity) const = 0;

    /// Get current animation name
    virtual std::optional<std::string> getCurrentAnimation(Entity entity) const = 0;

    /// State machine: set parameter
    virtual void setParam(Entity entity, const std::string& param, float value) = 0;
    virtual void setParam(Entity entity, const std::string& param, bool value) = 0;
    virtual void setTrigger(Entity entity, const std::string& trigger) = 0;

    /// Listen for animation events
    virtual SubscriptionId onEvent(Entity entity, const std::string& eventName,
                                    std::function<void()> cb) = 0;
    virtual void unsubscribe(SubscriptionId id) = 0;
};
```

---

### 5.10 Camera System [PUBLIC]

**Purpose:** Camera following, effects (shake, zoom), bounds, and coordinate conversion.
**Dependencies:** Entity, Events.
**Lua:** `bestow.camera`

> **Note:** Lock-on targeting (previously in Graphics3D) now lives here.

#### Types

```cpp
struct CameraShake {
    float intensity = 1.0f;
    float duration = 0.5f;
    float frequency = 15.0f;
    bool decaying = true;       // Intensity decreases over duration
};

struct FollowSettings {
    Entity target;
    Vec2 offset = {};           // 2D offset from target
    Vec3 offset3D = {};         // 3D offset from target
    float smoothing = 5.0f;     // Higher = faster tracking
    Vec2 deadzone = {};         // No movement within this zone
};

struct CameraBounds {
    Vec2 min;
    Vec2 max;
};

struct LockOnSettings {
    Entity target;
    float maxDistance = 50.0f;
    float transitionSpeed = 5.0f;
    Vec3 offset = {0, 2, 0};   // Offset above target
};
```

#### Core API

```cpp
class ICameraCore {
public:
    virtual ~ICameraCore() = default;

    // Lifecycle
    virtual Result<void> initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update(DeltaTime dt) = 0;

    //--- 2D Camera ---

    virtual void setCamera2D(const Camera& camera) = 0;
    virtual Camera getCamera2D() const = 0;

    //--- 3D Camera ---

    virtual void setCamera3D(const Camera3D& camera) = 0;
    virtual Camera3D getCamera3D() const = 0;

    //--- Following ---

    virtual void follow(const FollowSettings& settings) = 0;
    virtual void stopFollowing() = 0;
    virtual bool isFollowing() const = 0;

    /// Set smoothing factor (higher = snappier, lower = smoother)
    virtual void setSmoothing(float factor) = 0;
    virtual void setOffset(Vec3 offset) = 0;
    virtual void setDeadzone(Vec2 deadzone) = 0;

    //--- Bounds ---

    virtual void setBounds(const CameraBounds& bounds) = 0;
    virtual void clearBounds() = 0;

    //--- Effects ---

    virtual void shake(const CameraShake& shake) = 0;
    virtual void stopShake() = 0;
    virtual bool isShaking() const = 0;

    virtual void setZoom(float zoom) = 0;
    virtual float getZoom() const = 0;
    virtual void zoomTo(float targetZoom, float duration) = 0;

    //--- Lock-On Targeting (3D) ---

    virtual void lockOn(const LockOnSettings& settings) = 0;
    virtual void releaseLockOn() = 0;
    virtual bool isLockedOn() const = 0;
    virtual std::optional<Entity> getLockOnTarget() const = 0;

    /// Cycle to next/previous lock-on target within range
    virtual void cycleLockOnTarget(bool forward = true) = 0;

    //--- Coordinate Conversion ---

    virtual Vec2 worldToScreen(Vec2 worldPos) const = 0;
    virtual Vec2 screenToWorld(Vec2 screenPos) const = 0;
    virtual Vec3 worldToScreen3D(Vec3 worldPos) const = 0;
    virtual Ray3D screenToRay(Vec2 screenPos) const = 0;
};
```

#### System API

```cpp
class ICamera {
public:
    virtual ~ICamera() = default;

    /// Follow an entity
    virtual void follow(Entity target, float smoothing = 5.0f) = 0;
    virtual void stopFollowing() = 0;

    /// Camera shake
    virtual void shake(float intensity = 1.0f, float duration = 0.5f) = 0;

    /// Zoom
    virtual void setZoom(float zoom) = 0;
    virtual float getZoom() const = 0;

    /// Lock-on (3D)
    virtual void lockOn(Entity target) = 0;
    virtual void releaseLockOn() = 0;

    /// Coordinate conversion
    virtual Vec2 worldToScreen(Vec2 worldPos) const = 0;
    virtual Vec2 screenToWorld(Vec2 screenPos) const = 0;
};
```

---

### 5.11 UI System [PUBLIC]

**Purpose:** HTML/CSS-based user interface using RmlUI. Document management, DOM manipulation, data binding.
**Dependencies:** Assets, Input, Graphics Context, UI Render.
**Lua:** `bestow.ui`

#### Core API

```cpp
class IUICore {
public:
    virtual ~IUICore() = default;

    // Lifecycle
    virtual Result<void> initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update(DeltaTime dt) = 0;
    virtual void render() = 0;

    //--- Document Management ---

    virtual Result<DocumentHandle> loadDocument(const std::string& rmlPath) = 0;
    virtual void unloadDocument(DocumentHandle handle) = 0;
    virtual void showDocument(DocumentHandle handle) = 0;
    virtual void hideDocument(DocumentHandle handle) = 0;
    virtual bool isDocumentVisible(DocumentHandle handle) const = 0;

    //--- Stylesheet ---

    virtual Result<void> loadStylesheet(const std::string& rcssPath) = 0;

    //--- Element Queries ---

    virtual std::optional<ElementHandle> getElementById(DocumentHandle doc,
                                                         const std::string& id) = 0;
    virtual std::vector<ElementHandle> getElementsByTag(DocumentHandle doc,
                                                         const std::string& tag) = 0;
    virtual std::vector<ElementHandle> getElementsByClass(DocumentHandle doc,
                                                           const std::string& className) = 0;

    //--- Element Properties ---

    virtual void setElementText(ElementHandle element, const std::string& text) = 0;
    virtual std::string getElementText(ElementHandle element) const = 0;

    virtual void setElementVisible(ElementHandle element, bool visible) = 0;
    virtual bool isElementVisible(ElementHandle element) const = 0;

    virtual void addClass(ElementHandle element, const std::string& className) = 0;
    virtual void removeClass(ElementHandle element, const std::string& className) = 0;
    virtual bool hasClass(ElementHandle element, const std::string& className) const = 0;

    virtual void setAttribute(ElementHandle element, const std::string& name,
                               const std::string& value) = 0;
    virtual std::optional<std::string> getAttribute(ElementHandle element,
                                                      const std::string& name) const = 0;

    virtual void setStyle(ElementHandle element, const std::string& property,
                           const std::string& value) = 0;

    virtual void setFocus(ElementHandle element) = 0;

    //--- Dynamic DOM ---

    virtual Result<ElementHandle> createElement(DocumentHandle doc,
                                                  const std::string& tag) = 0;
    virtual void appendChild(ElementHandle parent, ElementHandle child) = 0;
    virtual void removeElement(ElementHandle element) = 0;
    virtual void setInnerRml(ElementHandle element, const std::string& rml) = 0;

    //--- Data Binding ---

    virtual void bindData(DocumentHandle doc, const std::string& name,
                           const PropertyValue& value) = 0;
    virtual void updateBinding(DocumentHandle doc, const std::string& name,
                                const PropertyValue& value) = 0;

    //--- Events ---

    virtual SubscriptionId addEventListener(ElementHandle element,
                                             const std::string& eventType,
                                             std::function<void()> handler) = 0;
    virtual void unsubscribe(SubscriptionId id) = 0;

    //--- Input ---

    virtual void processKeyEvent(int keyCode, bool pressed) = 0;
    virtual void processTextInput(const std::string& text) = 0;
    virtual void processMouseMove(Vec2 position) = 0;
    virtual void processMouseButton(int button, bool pressed) = 0;
    virtual void processScroll(Vec2 delta) = 0;

    //--- Fonts ---

    virtual Result<void> loadFont(const std::string& fontPath) = 0;

    //--- Viewport ---

    virtual void setViewportSize(int width, int height) = 0;
};
```

#### System API

```cpp
class IUI {
public:
    virtual ~IUI() = default;

    /// Load and show a UI document
    virtual Result<DocumentHandle> show(const std::string& rmlPath) = 0;

    /// Hide a document
    virtual void hide(DocumentHandle doc) = 0;

    /// Set text content of an element by ID
    virtual void setText(DocumentHandle doc, const std::string& elementId,
                          const std::string& text) = 0;

    /// Set visibility of an element by ID
    virtual void setVisible(DocumentHandle doc, const std::string& elementId,
                              bool visible) = 0;

    /// Add/remove CSS class on an element by ID
    virtual void addClass(DocumentHandle doc, const std::string& elementId,
                           const std::string& className) = 0;
    virtual void removeClass(DocumentHandle doc, const std::string& elementId,
                              const std::string& className) = 0;

    /// Listen for events on an element by ID
    virtual SubscriptionId on(DocumentHandle doc, const std::string& elementId,
                               const std::string& event, std::function<void()> handler) = 0;
    virtual void unsubscribe(SubscriptionId id) = 0;

    /// Update a data binding
    virtual void setData(DocumentHandle doc, const std::string& name,
                          const PropertyValue& value) = 0;
};
```

---

### 5.12 Scene System [PUBLIC]

**Purpose:** Stack-based scene management with transitions.
**Dependencies:** Events, Entity.
**Lua:** `bestow.scene`

#### Types

```cpp
/// Scene lifecycle callbacks — implemented by game code.
struct SceneCallbacks {
    std::function<void(const PropertyMap& params)> onEnter;
    std::function<void()> onExit;
    std::function<void()> onPause;   // When a scene is pushed on top
    std::function<void()> onResume;  // When the scene above is popped
    std::function<void(DeltaTime)> onUpdate;
    std::function<void()> onRender;
};

enum class TransitionType : uint8_t {
    None, Fade, SlideLeft, SlideRight, SlideUp, SlideDown, Custom,
};

struct TransitionSettings {
    TransitionType type = TransitionType::Fade;
    float duration = 0.3f;
    Color fadeColor = {0, 0, 0, 1};
};
```

#### Core API

```cpp
class ISceneCore {
public:
    virtual ~ISceneCore() = default;

    // Lifecycle
    virtual Result<void> initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update(DeltaTime dt) = 0;
    virtual void render() = 0;

    //--- Registration ---

    /// Register a scene with a name and lifecycle callbacks
    virtual Result<void> registerScene(const std::string& name,
                                        const SceneCallbacks& callbacks) = 0;
    virtual void unregisterScene(const std::string& name) = 0;

    //--- Stack Operations ---

    /// Push a scene onto the stack (current scene gets onPause)
    virtual Result<void> push(const std::string& name,
                               const PropertyMap& params = {},
                               const TransitionSettings& transition = {}) = 0;

    /// Pop the top scene (scene below gets onResume)
    virtual Result<void> pop(const TransitionSettings& transition = {}) = 0;

    /// Replace the top scene
    virtual Result<void> replace(const std::string& name,
                                  const PropertyMap& params = {},
                                  const TransitionSettings& transition = {}) = 0;

    /// Clear all scenes and push a new one
    virtual Result<void> clear(const std::string& name,
                                const PropertyMap& params = {},
                                const TransitionSettings& transition = {}) = 0;

    //--- Queries ---

    virtual std::string getCurrentScene() const = 0;
    virtual std::vector<std::string> getSceneStack() const = 0;
    virtual bool isSceneRegistered(const std::string& name) const = 0;
    virtual bool isTransitioning() const = 0;
};
```

#### System API

```cpp
class IScene {
public:
    virtual ~IScene() = default;

    /// Go to a scene (replaces current)
    virtual Result<void> goTo(const std::string& name,
                               const PropertyMap& params = {}) = 0;

    /// Push an overlay scene (e.g., pause menu)
    virtual Result<void> push(const std::string& name,
                               const PropertyMap& params = {}) = 0;

    /// Pop the overlay
    virtual Result<void> pop() = 0;

    /// Get current scene name
    virtual std::string current() const = 0;
};
```

---

### 5.13 State System [PUBLIC]

**Purpose:** Save/load game state. Key-value persistence with slots, profiles, and versioning.
**Dependencies:** Events.
**Lua:** `bestow.state`

> **Note:** State (persistence) is an intentional exception to the AssetSystem file-gateway rule.
> Save files are user-generated data requiring write operations that IAssetSystem doesn't support.

#### Types

```cpp
struct SaveSlotInfo {
    int slotIndex;
    std::string name;
    Timestamp lastSaved;
    std::string gameVersion;
    double playtimeSeconds;
    bool isEmpty;
};

struct ProfileInfo {
    std::string name;
    Timestamp created;
    std::vector<SaveSlotInfo> slots;
};
```

#### Core API

```cpp
class IStateCore {
public:
    virtual ~IStateCore() = default;

    // Lifecycle
    virtual Result<void> initialize() = 0;
    virtual void shutdown() = 0;

    //--- Key-Value Data ---

    virtual void setNumber(const std::string& key, double value) = 0;
    virtual void setString(const std::string& key, const std::string& value) = 0;
    virtual void setBool(const std::string& key, bool value) = 0;

    virtual std::optional<double> getNumber(const std::string& key) const = 0;
    virtual std::optional<std::string> getString(const std::string& key) const = 0;
    virtual std::optional<bool> getBool(const std::string& key) const = 0;

    virtual double getNumberOr(const std::string& key, double defaultValue) const = 0;
    virtual std::string getStringOr(const std::string& key,
                                     const std::string& defaultValue) const = 0;
    virtual bool getBoolOr(const std::string& key, bool defaultValue) const = 0;

    virtual bool hasKey(const std::string& key) const = 0;
    virtual void removeKey(const std::string& key) = 0;
    virtual std::vector<std::string> getKeysWithPrefix(const std::string& prefix) const = 0;

    //--- Save Slots ---

    /// Commit all current state to a slot
    virtual Result<void> commitToSlot(int slotIndex, const std::string& name = "") = 0;

    /// Restore state from a slot
    virtual Result<void> restoreFromSlot(int slotIndex) = 0;

    /// Delete a save slot
    virtual Result<void> deleteSlot(int slotIndex) = 0;

    /// Get slot info
    virtual std::optional<SaveSlotInfo> getSlotInfo(int slotIndex) const = 0;
    virtual std::vector<SaveSlotInfo> getAllSlots() const = 0;

    //--- Quick Save/Load ---

    virtual Result<void> quickSave() = 0;
    virtual Result<void> quickLoad() = 0;

    //--- Auto-Save ---

    virtual void enableAutoSave(float intervalSeconds) = 0;
    virtual void disableAutoSave() = 0;

    //--- Profiles ---

    virtual Result<void> createProfile(const std::string& name) = 0;
    virtual Result<void> switchProfile(const std::string& name) = 0;
    virtual Result<void> deleteProfile(const std::string& name) = 0;
    virtual std::optional<std::string> getCurrentProfile() const = 0;
    virtual std::vector<ProfileInfo> getProfiles() const = 0;

    //--- Game Version ---

    virtual void setGameVersion(const std::string& version) = 0;

    //--- Playtime Tracking ---

    virtual double getPlaytime() const = 0;
    virtual void resetPlaytime() = 0;

    //--- Export/Import ---

    virtual Result<std::string> exportToJson() const = 0;
    virtual Result<void> importFromJson(const std::string& json) = 0;

    //--- Notifications ---

    virtual SubscriptionId onSaved(std::function<void(int slotIndex)> cb) = 0;
    virtual SubscriptionId onLoaded(std::function<void(int slotIndex)> cb) = 0;
    virtual void unsubscribe(SubscriptionId id) = 0;
};
```

#### System API

```cpp
class IState {
public:
    virtual ~IState() = default;

    /// Get/set values
    virtual void set(const std::string& key, double value) = 0;
    virtual void set(const std::string& key, const std::string& value) = 0;
    virtual void set(const std::string& key, bool value) = 0;

    virtual std::optional<double> getNumber(const std::string& key) const = 0;
    virtual std::optional<std::string> getString(const std::string& key) const = 0;
    virtual std::optional<bool> getBool(const std::string& key) const = 0;

    /// Save/load
    virtual Result<void> save(int slot = 0) = 0;
    virtual Result<void> load(int slot = 0) = 0;

    /// Quick save/load
    virtual Result<void> quickSave() = 0;
    virtual Result<void> quickLoad() = 0;
};
```

---

### 5.14 AI System [PUBLIC]

**Purpose:** Behavior trees, navigation/pathfinding, steering behaviors, and spatial awareness.
**Dependencies:** Entity, Physics 3D, Events.
**Lua:** `bestow.ai`

#### Types

```cpp
enum class BehaviorStatus : uint8_t {
    Running, Success, Failure,
};

enum class SteeringBehavior : uint8_t {
    Seek, Flee, Arrive, Pursue, Evade, Wander, Avoid,
};

struct PathRequest {
    Vec3 start;
    Vec3 end;
    float agentRadius = 0.5f;
    float agentHeight = 1.8f;
    uint16_t areaMask = 0xFFFF;
};

struct PathResult {
    std::vector<Vec3> waypoints;
    float totalDistance;
    bool isPartial;   // True if only partial path found
};

struct PatrolRoute {
    std::vector<Vec3> waypoints;
    bool loop = true;
    float waitTimeAtWaypoint = 0.0f;
};

struct SteeringParams {
    float maxSpeed = 5.0f;
    float maxAcceleration = 10.0f;
    float arrivalRadius = 1.0f;
    float avoidanceRadius = 3.0f;
};

struct SpatialQuery {
    Vec3 center;
    float radius;
    uint16_t layerMask = 0xFFFF;
};
```

#### Core API

```cpp
class IAICore {
public:
    virtual ~IAICore() = default;

    // Lifecycle
    virtual Result<void> initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update(DeltaTime dt) = 0;

    //--- Behavior Trees ---

    /// Load behavior tree definition from asset
    virtual Result<BehaviorTreeHandle> loadBehaviorTree(AssetHandle luaAsset) = 0;

    /// Attach a behavior tree to an entity
    virtual Result<void> attachBehaviorTree(Entity entity, BehaviorTreeHandle tree) = 0;

    /// Detach behavior tree from entity
    virtual void detachBehaviorTree(Entity entity) = 0;

    /// Get behavior tree status for entity
    virtual std::optional<BehaviorStatus> getBehaviorStatus(Entity entity) const = 0;

    //--- Blackboard (typed, no std::any) ---

    virtual void setBlackboard(Entity entity, const std::string& key,
                                const BlackboardValue& value) = 0;
    virtual std::optional<BlackboardValue> getBlackboard(Entity entity,
                                                          const std::string& key) const = 0;
    virtual void clearBlackboard(Entity entity) = 0;

    //--- Navigation ---

    virtual Result<NavMeshHandle> loadNavMesh(AssetHandle asset) = 0;
    virtual void unloadNavMesh(NavMeshHandle handle) = 0;

    /// Find path between two points
    virtual Result<PathResult> findPath(const PathRequest& request) const = 0;

    /// Query nearest point on navmesh
    virtual std::optional<Vec3> findNearestPoint(Vec3 position, float searchRadius = 5.0f) const = 0;

    /// Check if a point is on the navmesh
    virtual bool isOnNavMesh(Vec3 position) const = 0;

    /// Raycast on navmesh (does the path cross any edges?)
    virtual std::optional<Vec3> navMeshRaycast(Vec3 from, Vec3 to) const = 0;

    //--- Steering ---

    virtual void setSteeringBehavior(Entity entity, SteeringBehavior behavior,
                                      const SteeringParams& params = {}) = 0;
    virtual void setSteeringTarget(Entity entity, Vec3 target) = 0;
    virtual void setSteeringTargetEntity(Entity entity, Entity target) = 0;
    virtual void clearSteering(Entity entity) = 0;

    /// Get computed steering velocity
    virtual Vec3 getSteeringVelocity(Entity entity) const = 0;

    //--- Patrol ---

    virtual void setPatrolRoute(Entity entity, const PatrolRoute& route) = 0;
    virtual void clearPatrolRoute(Entity entity) = 0;
    virtual int getCurrentWaypointIndex(Entity entity) const = 0;

    //--- Spatial Queries ---

    virtual std::vector<Entity> findEntitiesInRadius(Vec3 center, float radius,
                                                       uint16_t layerMask = 0xFFFF) const = 0;
    virtual std::optional<Entity> findClosestEntity(Vec3 position,
                                                      uint16_t layerMask = 0xFFFF) const = 0;
    virtual bool hasLineOfSight(Vec3 from, Vec3 to,
                                 uint16_t layerMask = 0xFFFF) const = 0;
};
```

#### System API

```cpp
class IAI {
public:
    virtual ~IAI() = default;

    /// Attach behavior tree from file
    virtual Result<void> setBehavior(Entity entity, const std::string& treePath) = 0;
    virtual void clearBehavior(Entity entity) = 0;

    /// Blackboard
    virtual void setData(Entity entity, const std::string& key,
                          const BlackboardValue& value) = 0;
    virtual std::optional<BlackboardValue> getData(Entity entity,
                                                     const std::string& key) const = 0;

    /// Find path
    virtual Result<PathResult> findPath(Vec3 from, Vec3 to) const = 0;

    /// Steering shortcuts
    virtual void seekTarget(Entity entity, Vec3 target, float speed = 5.0f) = 0;
    virtual void seekEntity(Entity entity, Entity target, float speed = 5.0f) = 0;
    virtual void patrol(Entity entity, const std::vector<Vec3>& waypoints, bool loop = true) = 0;
    virtual void stopMovement(Entity entity) = 0;

    /// Spatial awareness
    virtual std::vector<Entity> findNearby(Vec3 center, float radius) const = 0;
    virtual bool canSee(Vec3 from, Vec3 to) const = 0;
};
```

---

### 5.15 GAS — Gameplay Ability System [PUBLIC]

**Purpose:** Tags, attributes, gameplay effects, and abilities.
**Dependencies:** Entity, Events.
**Lua:** `bestow.gas`

#### Types

```cpp
using GameplayTag = std::string;  // Hierarchical: "Status.Burning", "Ability.Dash"

struct AttributeDef {
    std::string name;
    float baseValue = 0.0f;
    float minValue = -std::numeric_limits<float>::max();
    float maxValue = std::numeric_limits<float>::max();
};

enum class EffectDurationType : uint8_t {
    Instant, Duration, Infinite,
};

enum class AttributeModOp : uint8_t {
    Add, Multiply, Override,
};

struct AttributeModifier {
    std::string attribute;
    AttributeModOp operation;
    float value;
};

struct EffectDef {
    std::string name;
    EffectDurationType durationType = EffectDurationType::Instant;
    float duration = 0.0f;
    float period = 0.0f;   // For periodic effects (0 = not periodic)
    std::vector<AttributeModifier> modifiers;
    std::vector<GameplayTag> grantedTags;       // Tags added while active
    std::vector<GameplayTag> requiredTags;      // Must have these to apply
    std::vector<GameplayTag> blockedByTags;     // Cannot apply if target has these
    int maxStacks = 1;
};

struct AbilityDef {
    std::string name;
    std::vector<GameplayTag> tags;
    std::vector<GameplayTag> requiredTags;       // Must have to activate
    std::vector<GameplayTag> blockedByTags;      // Cannot activate if present
    std::vector<GameplayTag> cancelAbilitiesWith; // Cancel active abilities with these tags
    float cooldown = 0.0f;
    float cost = 0.0f;        // Resource cost
    std::string costAttribute; // Which attribute to deduct from
};
```

#### Core API

```cpp
class IGASCore {
public:
    virtual ~IGASCore() = default;

    // Lifecycle
    virtual Result<void> initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update(DeltaTime dt) = 0;

    //--- Entity GAS Component ---

    /// Initialize GAS on an entity
    virtual Result<void> initializeEntity(Entity entity) = 0;
    virtual void uninitializeEntity(Entity entity) = 0;
    virtual bool hasGAS(Entity entity) const = 0;

    //--- Tags ---

    virtual void addTag(Entity entity, const GameplayTag& tag) = 0;
    virtual void removeTag(Entity entity, const GameplayTag& tag) = 0;
    virtual bool hasTag(Entity entity, const GameplayTag& tag) const = 0;
    virtual bool hasAnyTag(Entity entity, std::span<const GameplayTag> tags) const = 0;
    virtual bool hasAllTags(Entity entity, std::span<const GameplayTag> tags) const = 0;
    virtual std::vector<GameplayTag> getTags(Entity entity) const = 0;

    //--- Attributes ---

    virtual Result<void> addAttribute(Entity entity, const AttributeDef& def) = 0;
    virtual void removeAttribute(Entity entity, const std::string& name) = 0;

    virtual std::optional<float> getAttributeValue(Entity entity,
                                                     const std::string& name) const = 0;
    virtual std::optional<float> getAttributeBase(Entity entity,
                                                    const std::string& name) const = 0;
    virtual Result<void> setAttributeBase(Entity entity, const std::string& name,
                                           float value) = 0;
    virtual Result<void> modifyAttribute(Entity entity, const std::string& name,
                                          AttributeModOp op, float value) = 0;

    //--- Effects ---

    virtual Result<EffectHandle> registerEffect(const EffectDef& def) = 0;

    virtual Result<void> applyEffect(Entity target, EffectHandle effect,
                                      Entity source = {}) = 0;
    virtual void removeEffect(Entity entity, EffectHandle effect) = 0;
    virtual void removeAllEffects(Entity entity) = 0;

    virtual bool hasEffect(Entity entity, EffectHandle effect) const = 0;
    virtual int getEffectStacks(Entity entity, EffectHandle effect) const = 0;
    virtual std::optional<float> getEffectRemainingDuration(Entity entity,
                                                              EffectHandle effect) const = 0;

    //--- Abilities ---

    virtual Result<AbilityHandle> registerAbility(const AbilityDef& def) = 0;

    virtual Result<void> grantAbility(Entity entity, AbilityHandle ability) = 0;
    virtual void revokeAbility(Entity entity, AbilityHandle ability) = 0;

    virtual Result<void> activateAbility(Entity entity, AbilityHandle ability) = 0;
    virtual void deactivateAbility(Entity entity, AbilityHandle ability) = 0;

    virtual bool canActivateAbility(Entity entity, AbilityHandle ability) const = 0;
    virtual bool isAbilityActive(Entity entity, AbilityHandle ability) const = 0;
    virtual std::optional<float> getAbilityCooldownRemaining(Entity entity,
                                                               AbilityHandle ability) const = 0;

    virtual std::vector<AbilityHandle> getGrantedAbilities(Entity entity) const = 0;

    //--- Lua Definitions ---

    /// Load effect/ability definitions from Lua file
    virtual Result<void> loadDefinitions(const std::string& luaPath) = 0;

    //--- Callbacks ---

    virtual SubscriptionId onAttributeChanged(Entity entity, const std::string& attribute,
                                               std::function<void(float oldVal, float newVal)> cb) = 0;
    virtual SubscriptionId onTagChanged(Entity entity,
                                         std::function<void(const GameplayTag&, bool added)> cb) = 0;
    virtual SubscriptionId onEffectApplied(Entity entity,
                                            std::function<void(EffectHandle)> cb) = 0;
    virtual SubscriptionId onAbilityActivated(Entity entity,
                                               std::function<void(AbilityHandle)> cb) = 0;
    virtual void unsubscribe(SubscriptionId id) = 0;
};
```

#### System API

```cpp
class IGAS {
public:
    virtual ~IGAS() = default;

    /// Initialize GAS on entity
    virtual Result<void> init(Entity entity) = 0;

    /// Tags
    virtual void addTag(Entity entity, const std::string& tag) = 0;
    virtual void removeTag(Entity entity, const std::string& tag) = 0;
    virtual bool hasTag(Entity entity, const std::string& tag) const = 0;

    /// Attributes
    virtual Result<void> setAttribute(Entity entity, const std::string& attr, float value) = 0;
    virtual std::optional<float> getAttribute(Entity entity, const std::string& attr) const = 0;
    virtual Result<void> modifyAttribute(Entity entity, const std::string& attr, float delta) = 0;

    /// Effects (by name)
    virtual Result<void> applyEffect(Entity target, const std::string& effectName,
                                      Entity source = {}) = 0;
    virtual void removeEffect(Entity target, const std::string& effectName) = 0;

    /// Abilities (by name)
    virtual Result<void> grantAbility(Entity entity, const std::string& abilityName) = 0;
    virtual Result<void> activateAbility(Entity entity, const std::string& abilityName) = 0;
    virtual bool canActivate(Entity entity, const std::string& abilityName) const = 0;
};
```

---

### 5.16 Blueprint System [PUBLIC]

**Purpose:** Entity templates. Load component configurations from Lua files and stamp out entities.
**Dependencies:** Entity, Assets, Config, Events.
**Lua:** `bestow.blueprint`

#### Core API

```cpp
class IBlueprintCore {
public:
    virtual ~IBlueprintCore() = default;

    // Lifecycle
    virtual Result<void> initialize() = 0;
    virtual void shutdown() = 0;

    //--- Blueprint Registration ---

    /// Load blueprint definitions from a Lua file
    virtual Result<void> loadBlueprints(const std::string& luaPath) = 0;

    /// Reload all blueprint definitions
    virtual Result<void> reloadAll() = 0;

    /// Clear all loaded blueprints
    virtual void clearAll() = 0;

    /// Check if a blueprint is registered
    virtual bool hasBlueprint(const std::string& name) const = 0;

    /// Get all registered blueprint names
    virtual std::vector<std::string> getBlueprintNames() const = 0;

    //--- Entity Creation ---

    /// Create entity from blueprint
    virtual Result<Entity> createEntity(const std::string& blueprintName) = 0;

    /// Create entity with position override
    virtual Result<Entity> createEntity(const std::string& blueprintName, Vec2 position) = 0;

    /// Create entity with position and size override
    virtual Result<Entity> createEntity(const std::string& blueprintName,
                                         Vec2 position, Vec2 size) = 0;

    /// Create entity with arbitrary property overrides
    virtual Result<Entity> createEntity(const std::string& blueprintName,
                                         const PropertyMap& overrides) = 0;

    //--- Component Type Registration ---

    /// Register a C++ component type so blueprints can reference it by name
    virtual Result<void> registerComponentType(const std::string& name,
                                                ComponentTypeId typeId,
                                                std::function<void(Entity, const PropertyMap&)> factory) = 0;
};
```

#### System API

```cpp
class IBlueprint {
public:
    virtual ~IBlueprint() = default;

    /// Create entity from blueprint name
    virtual Result<Entity> create(const std::string& name) = 0;

    /// Create entity at position
    virtual Result<Entity> create(const std::string& name, Vec2 position) = 0;
    virtual Result<Entity> create(const std::string& name, Vec3 position) = 0;

    /// Create entity with property overrides
    virtual Result<Entity> create(const std::string& name,
                                   const PropertyMap& overrides) = 0;

    /// Check if blueprint exists
    virtual bool exists(const std::string& name) const = 0;
};
```

---

### 5.17 Timer System [PUBLIC]

**Purpose:** Schedule delayed and repeating callbacks.
**Dependencies:** Events.
**Lua:** `bestow.timer`

#### Core API

```cpp
class ITimerCore {
public:
    virtual ~ITimerCore() = default;

    // Lifecycle
    virtual void update(DeltaTime dt) = 0;

    /// Schedule a one-shot callback after a delay
    virtual SubscriptionId after(float delaySeconds, std::function<void()> callback) = 0;

    /// Schedule a repeating callback
    virtual SubscriptionId every(float intervalSeconds, std::function<void()> callback) = 0;

    /// Schedule a repeating callback with a max repeat count
    virtual SubscriptionId every(float intervalSeconds, int maxRepeats,
                                  std::function<void()> callback) = 0;

    /// Cancel a scheduled timer
    virtual void cancel(SubscriptionId id) = 0;

    /// Cancel all timers
    virtual void cancelAll() = 0;

    /// Pause/resume a timer
    virtual void pause(SubscriptionId id) = 0;
    virtual void resume(SubscriptionId id) = 0;

    /// Check if a timer is active
    virtual bool isActive(SubscriptionId id) const = 0;

    /// Get remaining time
    virtual std::optional<float> getRemaining(SubscriptionId id) const = 0;

    /// Get total active timer count
    virtual size_t getActiveCount() const = 0;
};
```

#### System API

```cpp
class ITimer {
public:
    virtual ~ITimer() = default;

    /// Call function after delay
    virtual SubscriptionId after(float seconds, std::function<void()> cb) = 0;

    /// Call function repeatedly
    virtual SubscriptionId every(float seconds, std::function<void()> cb) = 0;

    /// Cancel a timer
    virtual void cancel(SubscriptionId id) = 0;
};
```

---

## 6. Cross-Cutting Concerns

### 6.1 Lifecycle Contract

All system Core APIs follow a consistent lifecycle pattern:

```
initialize() → [update(dt) | render()]* → shutdown()
```

- `initialize()` — Allocate resources, subscribe to events, load initial data. Returns `Result<void>`.
- `update(dt)` — Per-frame logic. Called in the appropriate `UpdatePhase`.
- `render()` — Drawing operations (only for rendering systems).
- `shutdown()` — Release all resources, unsubscribe from events. Must be idempotent.

### 6.2 Error Propagation

Contracts use `Result<T>` (`std::expected<T, SystemError>`) for all operations that can fail. The error does NOT propagate automatically — callers must check results explicitly.

**Convention for System (high-level) APIs:** Errors are logged internally. Game code can check results but errors are non-fatal by default. The engine doesn't crash on a failed asset load — it uses a fallback.

**Convention for Core (low-level) APIs:** Errors are returned without logging. The caller decides what to do.

### 6.3 Subscription Lifecycle

All subscriptions follow this protocol:

1. **Subscribe:** `SubscriptionId id = system.onSomething(callback);`
2. **Active:** Callback will be invoked when the event occurs.
3. **Unsubscribe:** `system.unsubscribe(id);`
4. **Implicit cleanup:** When the system shuts down, all subscriptions are automatically invalidated. Calling `unsubscribe()` with an invalid ID is a no-op.

### 6.4 Hot Reload Protocol

Systems that support hot reload follow this pattern:

1. Asset system detects file change via file watcher
2. Asset system reloads the asset data
3. Asset system notifies subscribers via `SubscriptionId` callbacks
4. Subscribing systems receive notification and update their internal state
5. Game code subscriptions (if any) are called last

**Which systems support hot reload:**
- Config (Lua configs)
- Blueprint (Lua blueprints)
- Graphics (shaders, textures, materials)
- Animation (clips, state machines)
- Audio (sound files)
- UI (RML documents, stylesheets)
- AI (behavior tree definitions)
- Input (input config)

### 6.5 Thread Safety

- **All System (high-level) APIs are single-threaded** — called only from the main game thread.
- **Core (low-level) APIs may document thread-safety per method** — e.g., asset loading callbacks may be invoked from a background thread.
- **Event callbacks are always dispatched on the main thread** — even for events queued from background threads.

### 6.6 Handle Validity

- Handles become invalid when the associated resource is destroyed.
- Using an invalid handle is safe (returns error/no-op) — it never crashes.
- Handles are NOT reused within a session — a destroyed mesh's handle will never refer to a new mesh.
- Handle comparison is cheap (uint64_t comparison).

### 6.7 Lua Integration Contract

For all **Public** systems, the Lua bindings follow these rules:

1. **Two-value returns for fallible operations:**
   ```lua
   local entity, err = bestow.blueprint.create("player")
   if err then print("Failed: " .. err) end
   ```

2. **Method names match System API** (not Core API):
   ```lua
   bestow.audio.playMusic("music/theme.ogg")
   bestow.input.isActionActive("jump")
   bestow.physics.raycast(from, to)
   ```

3. **Handles are opaque in Lua** — no access to `.id` field:
   ```lua
   local entity = bestow.entity.create()
   bestow.entity.destroy(entity)  -- entity is opaque
   ```

4. **PropertyMap maps to Lua tables:**
   ```lua
   bestow.blueprint.create("enemy", { x = 100, y = 200, health = 50 })
   ```

5. **Callbacks use Lua functions:**
   ```lua
   local sub = bestow.physics.onCollision(function(info)
       print("Collision between " .. info.entityA .. " and " .. info.entityB)
   end)
   bestow.physics.unsubscribe(sub)
   ```

---

## 7. Migration Guide from V1

### Handle Migration

| V1 | V2 | Change |
|----|-----|--------|
| `using MeshHandle = uint64_t` | `using MeshHandle = Handle<MeshTag>` | Type-safe, no cross-type assignment |
| `using Entity = entt::entity` | `using Entity = Handle<EntityTag>` | Abstracted from EnTT |
| `using Channel = int` | `using Channel = Handle<ChannelTag>` | Type-safe |

### Error Handling Migration

| V1 | V2 | Change |
|----|-----|--------|
| `bool loadConfig(...)` | `Result<void> loadConfig(...)` | Carries error info |
| `void playOnChannel(...)` | `void playOnChannel(...)` | Fire-and-forget stays void |
| `Entity createEntity()` | `Entity createEntity()` | Cannot fail, stays direct |

### Callback Migration

| V1 | V2 | Change |
|----|-----|--------|
| `void setCollisionCallback(fn)` | `SubscriptionId onCollision(fn)` | Returnable, unsubscribable |
| `void onConfigChanged(fn)` → `SubscriptionId` | Same | Already correct in v1 |

### Interface Split Migration

| V1 Interface | V2 Interfaces | Reason |
|-------------|---------------|--------|
| `IGraphics3DSystem` (100+ methods) | `IGraphics3DCore` + `IGraphics3D` | Dual-level API |
| Lock-on in Graphics3D | `ICameraCore` | Wrong system |
| Debug draw in Graphics3D | Dev Tools (debug-only) | Conditional compilation |
| Skeleton/Animation in Graphics3D | `IAnimationCore` | Separate concern |

### std::any Elimination

| V1 Usage | V2 Replacement |
|----------|----------------|
| `ConfigValue = std::any` | `PropertyValue = std::variant<...>` |
| `PropertyMap = map<string, any>` | `PropertyMap = map<string, PropertyValue>` |
| AI blackboard `std::any` | `BlackboardValue = std::variant<...>` |
| Material uniforms `std::any` | `UniformValue = std::variant<...>` |
| Scene params `map<string, any>` | `PropertyMap` |
| `DataAsset { std::any data }` | Typed asset access via `getAsset<T>()` |

### void* Elimination

| V1 Usage | V2 Replacement |
|----------|----------------|
| `IGraphicsContext::getRenderContext() → void*` | Removed — backend-specific extension point |
| `IGraphicsContext::getCommandBuffer() → void*` | Removed — submit commands through abstract API |
| `IEntitySystem::getComponentRaw() → void*` | `IEntityCore::getComponentRaw() → void*` (kept — fundamental to type-erased ECS) |

---

## Appendix A: Complete System Dependency Graph

```
Tier 0 (No Dependencies):
  └── Foundation Types

Tier 1 (Foundation Only):
  ├── Events
  └── Timer

Tier 2 (Events):
  ├── Config
  ├── Assets
  ├── Input
  └── State

Tier 3 (Events + Entity):
  ├── Entity
  ├── Camera
  ├── Scene
  └── GAS

Tier 4 (Entity + Assets):
  ├── Graphics 2D
  ├── Graphics 3D
  ├── Audio
  ├── Animation
  ├── Blueprint
  └── Physics 2D / 3D

Tier 5 (Entity + Physics):
  └── AI

Tier 6 (All):
  └── Dev Tools (Debug only)
```

## Appendix B: Method Count Summary

| System | Core API Methods | System API Methods | Total |
|--------|----------------:|-------------------:|------:|
| Events | 8 | 3 (templates) | 11 |
| Assets | 22 | — | 22 |
| Graphics Context | 8 | — | 8 |
| UI Render | 8 | — | 8 |
| Shader | 7 | — | 7 |
| Config | 28 | 6 | 34 |
| Entity | 24 | 12 | 36 |
| Input | 42 | 9 | 51 |
| Graphics 2D | 16 | 7 | 23 |
| Graphics 3D | 38 | 11 | 49 |
| Audio | 22 | 8 | 30 |
| Physics 2D | 34 | 11 | 45 |
| Physics 3D | 44 | 12 | 56 |
| Animation | 40 | 12 | 52 |
| Camera | 22 | 8 | 30 |
| UI | 30 | 8 | 38 |
| Scene | 10 | 4 | 14 |
| State | 28 | 8 | 36 |
| AI | 22 | 10 | 32 |
| GAS | 32 | 10 | 42 |
| Blueprint | 10 | 5 | 15 |
| Timer | 9 | 3 | 12 |
| **Total** | **~518** | **~147** | **~665** |

> **Reduction from v1:** The v1 contracts have ~1000+ methods across 31 interfaces.
> V2 consolidates to ~665 methods across 44 interfaces (22 Core + 22 System),
> while adding missing functionality (AI Lua bindings, Camera lock-on, Timer system, 3D character controller).
> The reduction comes from eliminating deprecated methods, removing duplicated accessor patterns,
> and splitting god interfaces into focused APIs where the System API handles common cases with fewer methods.
