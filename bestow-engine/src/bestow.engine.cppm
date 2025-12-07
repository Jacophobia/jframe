// bestow-engine/src/bestow.engine.cppm
// Minimal Public API for Bestow Game Engine
//
// This module provides a simplified, user-friendly interface to the engine.
// Background systems (Graphics, Shader, Assets) are hidden from the user
// and configured through a unified Settings interface.

module;

export module bestow.engine;

import std;
import bestow.types;
import bestow.entity;
import bestow.input;
import bestow.audio;
import bestow.events;
import bestow.gamestate;
import bestow.level;
import bestow.physics3d;

export namespace bestow {

//==========================================================================
// Forward Declarations
//==========================================================================

class Engine;
class World;
class Settings;

//==========================================================================
// Engine Settings - Unified Configuration for Background Systems
//==========================================================================

/// Graphics quality presets
enum class GraphicsQuality : std::uint8_t {
    Low,      // Minimal effects, lower resolution
    Medium,   // Balanced
    High,     // Full effects
    Ultra     // Maximum quality
};

/// Audio quality settings
enum class AudioQuality : std::uint8_t {
    Low,      // Lower sample rate
    Medium,   // CD quality
    High      // Studio quality
};

/// Window settings
struct WindowSettings {
    int width = 1280;
    int height = 720;
    std::string title = "Bestow Game";
    bool fullscreen = false;
    bool vsync = true;
    bool resizable = true;
};

/// Graphics settings (for background Graphics3D/Shader systems)
struct GraphicsSettings {
    GraphicsQuality quality = GraphicsQuality::Medium;
    float renderScale = 1.0f;
    bool shadows = true;
    int shadowResolution = 2048;
    bool bloom = false;
    bool ssao = false;
    float fov = 60.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
};

/// Audio settings (for background Audio system)
struct AudioSettings {
    AudioQuality quality = AudioQuality::Medium;
    float masterVolume = 1.0f;
    float musicVolume = 0.8f;
    float sfxVolume = 1.0f;
    int maxChannels = 32;
};

/// Physics settings (for background Physics3D system)
struct PhysicsSettings {
    float gravity = -9.81f;
    int substeps = 2;
    float fixedTimestep = 1.0f / 60.0f;
    bool debugDraw = false;
};

/// Asset loading settings
struct AssetSettings {
    std::string basePath = "assets/";
    bool hotReload = true;
    int maxConcurrentLoads = 4;
};

/// All engine settings combined
struct EngineSettings {
    WindowSettings window;
    GraphicsSettings graphics;
    AudioSettings audio;
    PhysicsSettings physics;
    AssetSettings assets;

    /// Load settings from a Lua file
    static EngineSettings fromFile(std::string_view path);

    /// Save settings to a Lua file
    void saveToFile(std::string_view path) const;
};

//==========================================================================
// World - The Primary User-Facing System
//==========================================================================

/// The World is the main interface for game logic.
/// It combines Entity management, Physics, and automatic rendering.
/// Users work with entities; physics simulation and rendering happen automatically.
class IWorld {
public:
    virtual ~IWorld() = default;

    //======================================================================
    // Entity Management
    //======================================================================

    /// Create a new entity in the world
    virtual Entity createEntity() = 0;

    /// Create an entity with a Transform3D at the given position
    virtual Entity createEntity(const Vec3& position) = 0;

    /// Create an entity from a blueprint
    virtual Entity createFromBlueprint(std::string_view blueprintPath) = 0;

    /// Destroy an entity
    virtual void destroyEntity(Entity entity) = 0;

    /// Check if an entity is valid
    virtual bool isValid(Entity entity) const = 0;

    //======================================================================
    // Component Access (Type-Safe Templates)
    //======================================================================

    /// Add a component to an entity
    template<typename T, typename... Args>
    T& addComponent(Entity entity, Args&&... args);

    /// Get a component from an entity
    template<typename T>
    T& getComponent(Entity entity);

    /// Get a component from an entity (const)
    template<typename T>
    const T& getComponent(Entity entity) const;

    /// Try to get a component (returns nullptr if not present)
    template<typename T>
    T* tryGetComponent(Entity entity);

    /// Check if entity has a component
    template<typename T>
    bool hasComponent(Entity entity) const;

    /// Remove a component from an entity
    template<typename T>
    void removeComponent(Entity entity);

    //======================================================================
    // Entity Queries
    //======================================================================

    /// Get a view of entities with specific components
    template<typename... Components>
    auto view();

    /// Find first entity with components
    template<typename... Components>
    std::optional<Entity> findFirst() const;

    /// Iterate over all entities with specific components
    template<typename... Components, typename Func>
    void forEach(Func&& func);

    //======================================================================
    // Physics Helpers
    //======================================================================

    /// Cast a ray into the world
    virtual std::optional<RaycastHit3D> raycast(
        const Vec3& origin,
        const Vec3& direction,
        float maxDistance = 1000.0f) const = 0;

    /// Cast a ray and get all hits
    virtual std::vector<RaycastHit3D> raycastAll(
        const Vec3& origin,
        const Vec3& direction,
        float maxDistance = 1000.0f) const = 0;

    /// Check for overlapping entities at a position
    virtual std::vector<Entity> overlapSphere(
        const Vec3& center,
        float radius) const = 0;

    //======================================================================
    // Camera Control
    //======================================================================

    /// Set the main camera position and target
    virtual void setCameraPosition(const Vec3& position) = 0;
    virtual void setCameraTarget(const Vec3& target) = 0;
    virtual void setCameraRotation(const Quat& rotation) = 0;

    /// Get camera properties
    virtual Vec3 getCameraPosition() const = 0;
    virtual Quat getCameraRotation() const = 0;
    virtual Vec3 getCameraForward() const = 0;

    /// Convert screen to world coordinates
    virtual Vec3 screenToWorld(Vec2 screenPos, float depth = 0.0f) const = 0;
    virtual Vec2 worldToScreen(const Vec3& worldPos) const = 0;

    //======================================================================
    // Debug Visualization
    //======================================================================

    /// Draw a debug line (visible for one frame or specified duration)
    virtual void debugLine(const Vec3& start, const Vec3& end,
                          const Color& color = Color::white(),
                          float duration = 0.0f) = 0;

    /// Draw a debug box
    virtual void debugBox(const Vec3& center, const Vec3& halfExtents,
                         const Color& color = Color::white(),
                         float duration = 0.0f) = 0;

    /// Draw a debug sphere
    virtual void debugSphere(const Vec3& center, float radius,
                            const Color& color = Color::white(),
                            float duration = 0.0f) = 0;

    //======================================================================
    // Internal Access (for advanced use)
    //======================================================================

    /// Get the underlying entity system (advanced use)
    virtual IEntitySystem& entities() = 0;
    virtual const IEntitySystem& entities() const = 0;

    /// Get the underlying physics system (advanced use)
    virtual IPhysics3DSystem& physics() = 0;
    virtual const IPhysics3DSystem& physics() const = 0;
};

//==========================================================================
// Simplified Audio Interface
//==========================================================================

/// Simplified audio interface for common use cases
class IAudio {
public:
    virtual ~IAudio() = default;

    /// Play a sound effect (fire and forget)
    virtual void playSound(std::string_view path, float volume = 1.0f) = 0;

    /// Play a sound at a 3D position
    virtual void playSoundAt(std::string_view path, const Vec3& position,
                            float volume = 1.0f) = 0;

    /// Play background music (crossfades if music already playing)
    virtual void playMusic(std::string_view path, float volume = 1.0f,
                          bool loop = true) = 0;

    /// Stop current music
    virtual void stopMusic(float fadeOutTime = 0.5f) = 0;

    /// Pause/resume all audio
    virtual void pauseAll() = 0;
    virtual void resumeAll() = 0;

    /// Set master volume (0.0 - 1.0)
    virtual void setMasterVolume(float volume) = 0;
    virtual void setMusicVolume(float volume) = 0;
    virtual void setSfxVolume(float volume) = 0;
};

//==========================================================================
// Settings Interface - Modify Background System Config at Runtime
//==========================================================================

/// Runtime settings interface for modifying background system configurations
class ISettings {
public:
    virtual ~ISettings() = default;

    /// Get current settings
    virtual const EngineSettings& current() const = 0;

    /// Apply new settings (may trigger system reinitialization)
    virtual void apply(const EngineSettings& settings) = 0;

    /// Apply just window settings
    virtual void applyWindow(const WindowSettings& settings) = 0;

    /// Apply just graphics settings
    virtual void applyGraphics(const GraphicsSettings& settings) = 0;

    /// Apply just audio settings
    virtual void applyAudio(const AudioSettings& settings) = 0;

    /// Apply just physics settings
    virtual void applyPhysics(const PhysicsSettings& settings) = 0;

    /// Load settings from file
    virtual void loadFromFile(std::string_view path) = 0;

    /// Save current settings to file
    virtual void saveToFile(std::string_view path) const = 0;
};

//==========================================================================
// Engine - The Main Entry Point
//==========================================================================

/// The Engine is the main entry point for creating a Bestow game.
/// It owns all systems and provides access to the user-facing API.
class IEngine {
public:
    virtual ~IEngine() = default;

    //======================================================================
    // User-Facing Systems
    //======================================================================

    /// The game world (entities, physics, rendering)
    virtual IWorld& world() = 0;
    virtual const IWorld& world() const = 0;

    /// Input handling
    virtual IInputSystem& input() = 0;
    virtual const IInputSystem& input() const = 0;

    /// Audio playback
    virtual IAudio& audio() = 0;

    /// Game state machine
    virtual IGameStateSystem& gameStates() = 0;

    /// Event system for custom events
    virtual IEventSystem& events() = 0;

    /// Level/scene management
    virtual ILevelSystem& levels() = 0;

    /// Runtime settings
    virtual ISettings& settings() = 0;

    //======================================================================
    // Engine Lifecycle
    //======================================================================

    /// Request the engine to quit
    virtual void quit() = 0;

    /// Check if engine should continue running
    virtual bool isRunning() const = 0;

    /// Get time since engine started
    virtual float getTime() const = 0;

    /// Get delta time for current frame
    virtual float getDeltaTime() const = 0;

    /// Get fixed timestep for physics
    virtual float getFixedDeltaTime() const = 0;
};

//==========================================================================
// Engine Factory - Creates Engine with Fruit DI
//==========================================================================

/// Create an engine with default settings
std::unique_ptr<IEngine> createEngine();

/// Create an engine with custom settings
std::unique_ptr<IEngine> createEngine(const EngineSettings& settings);

/// Create an engine from a settings file
std::unique_ptr<IEngine> createEngineFromFile(std::string_view settingsPath);

//==========================================================================
// Game Application Interface
//==========================================================================

/// Base class for game applications
class Game {
public:
    Game() = default;
    virtual ~Game() = default;

    // Non-copyable
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    /// Called once when the game starts
    virtual void onStart(IEngine& engine) = 0;

    /// Called every frame for game logic
    virtual void onUpdate(IEngine& engine, float dt) = 0;

    /// Called at fixed intervals for physics-dependent logic
    virtual void onFixedUpdate(IEngine& engine, float dt) = 0;

    /// Called when the game is shutting down
    virtual void onShutdown(IEngine& engine) = 0;
};

/// Run a game with the engine
int runGame(Game& game, const EngineSettings& settings = {});

/// Run a game with settings from a file
int runGame(Game& game, std::string_view settingsPath);

}  // namespace bestow
