// bestow-runtime/src/bestow.runtime.cppm
// Simple user-facing game API with all dependencies wired up
//
// This module provides the simplest possible API for creating games.
// Game developers should only need to:
//   1. Extend bestow::Game
//   2. Override lifecycle methods (onStart, onUpdate, onRender)
//   3. Call bestow::run<MyGame>(config) in main()
//
// All dependency injection and system initialization is handled internally.

module;

#include <kangaru/kangaru.hpp>
#include <GLFW/glfw3.h>

export module bestow.runtime;

import std;
import bestow.types;
import bestow.services;  // All contracts

// Import default implementations
import bestow.vulkan.impl;     // Vulkan 3D graphics (VulkanGraphics3DSystemService)
import bestow.audio.impl;      // FMOD audio (AudioSystemService)
import bestow.entity.impl;     // EnTT entities (EntitySystemService)
import bestow.input.impl;      // GLFW input (InputSystemService)
import bestow.events.impl;     // Event system (EventSystemService)
import bestow.assets.impl;     // Asset system (AssetSystemService)
import bestow.config.impl;     // Config system (ConfigSystemService)

export namespace bestow {

//==========================================================================
// Game Configuration
//==========================================================================

/// Configuration for initializing a game.
/// Pass this to bestow::run<MyGame>(config).
struct GameConfig {
    std::string title = "Bestow Game";
    int width = 1280;
    int height = 720;
    bool vsync = true;
    bool fullscreen = false;
    Color clearColor = Color{30, 30, 50, 255};  // Dark blue
    std::string dataPath = "data";  // Base path for assets
};

//==========================================================================
// Systems Bundle
//==========================================================================

/// Provides access to all engine systems.
/// Game developers access systems through the Game::systems() method.
struct Systems {
    IEntitySystem* entities = nullptr;
    IGraphics3DSystem* graphics3d = nullptr;
    IAudioSystem* audio = nullptr;
    IInputSystem* input = nullptr;
    IAssetSystem* assets = nullptr;
    IEventSystem* events = nullptr;
    IPhysics3DSystem* physics3d = nullptr;
    ICameraSystem* camera = nullptr;
    ILevelSystem* levels = nullptr;
    ISaveSystem* save = nullptr;
    IConfigSystem* config = nullptr;
    IShaderSystem* shaders = nullptr;
    IAISystem* ai = nullptr;
    IUISystem* ui = nullptr;
    IGASSystem* gas = nullptr;
    IGameStateSystem* gamestate = nullptr;
    IBlueprintFactory* blueprints = nullptr;
};

//==========================================================================
// Game Base Class
//==========================================================================

/// Base class for all games. Extend this and override the lifecycle methods.
///
/// Example:
/// ```cpp
/// import bestow.runtime;
///
/// class MyGame : public bestow::Game {
/// public:
///     void onStart() override {
///         cube_ = graphics3d()->createCubeMesh();
///     }
///
///     void onUpdate(DeltaTime dt) override {
///         if (input()->isKeyDown(GLFW_KEY_SPACE)) {
///             player_.jump();
///         }
///     }
///
///     void onRender() override {
///         graphics3d()->drawMesh(cube_, material_, transform_);
///     }
/// };
///
/// int main() {
///     return bestow::run<MyGame>({.title = "My Game"});
/// }
/// ```
class Game {
public:
    virtual ~Game() = default;

    //======================================================================
    // Lifecycle Methods (Override These)
    //======================================================================

    /// Called once after all systems are initialized.
    /// Use this to create meshes, load assets, spawn initial entities.
    virtual void onStart() {}

    /// Called at fixed timestep (typically 60Hz) for game logic.
    /// Use this for physics, AI, game state updates.
    /// @param dt Time since last update in seconds
    virtual void onUpdate(DeltaTime dt) { (void)dt; }

    /// Called each frame for rendering.
    /// Use this to draw meshes, sprites, UI.
    virtual void onRender() {}

    /// Called when the game is shutting down.
    /// Use this to save state, clean up resources.
    virtual void onShutdown() {}

    //======================================================================
    // System Access
    //======================================================================

    /// Get access to all engine systems.
    /// Example: systems().graphics3d->drawMesh(...)
    [[nodiscard]] Systems& systems() { return systems_; }
    [[nodiscard]] const Systems& systems() const { return systems_; }

    /// Convenience accessors for common systems
    [[nodiscard]] IGraphics3DSystem* graphics3d() { return systems_.graphics3d; }
    [[nodiscard]] IInputSystem* input() { return systems_.input; }
    [[nodiscard]] IEntitySystem* entities() { return systems_.entities; }
    [[nodiscard]] IAudioSystem* audio() { return systems_.audio; }
    [[nodiscard]] IAssetSystem* assets() { return systems_.assets; }
    [[nodiscard]] IEventSystem* events() { return systems_.events; }
    [[nodiscard]] IPhysics3DSystem* physics3d() { return systems_.physics3d; }
    [[nodiscard]] ICameraSystem* camera() { return systems_.camera; }

    //======================================================================
    // Game Control
    //======================================================================

    /// Request the game to exit after the current frame.
    void quit() { shouldQuit_ = true; }

    /// Check if quit has been requested.
    [[nodiscard]] bool shouldQuit() const { return shouldQuit_; }

    /// Get the current game configuration.
    [[nodiscard]] const GameConfig& config() const { return config_; }

    //======================================================================
    // Internal (Used by bestow::run)
    //======================================================================

    void _setConfig(const GameConfig& cfg) { config_ = cfg; }
    void _setSystems(const Systems& sys) { systems_ = sys; }

private:
    Systems systems_;
    GameConfig config_;
    bool shouldQuit_ = false;
};

//==========================================================================
// Game Runner Implementation
//==========================================================================

namespace detail {

/// Internal game runner that handles the game loop.
template<typename GameType>
int runGame(const GameConfig& config) {
    static_assert(std::is_base_of_v<Game, GameType>,
        "GameType must inherit from bestow::Game");

    // Create Kangaru container for dependency injection
    kgr::container container;

    // Create game instance
    GameType game;
    game._setConfig(config);

    // Initialize systems bundle
    Systems systems;

    // Get graphics3d system (required) - using Vulkan backend
    auto& graphics3d = container.service<vulkan::VulkanGraphics3DSystemService>();
    systems.graphics3d = &graphics3d;

    // Initialize graphics
    Graphics3DConfig gfxConfig{
        .windowWidth = config.width,
        .windowHeight = config.height,
        .windowTitle = config.title,
        .vsync = config.vsync,
        .fullscreen = config.fullscreen
    };
    if (!graphics3d.initialize(gfxConfig)) {
        std::cerr << "Failed to initialize graphics system\n";
        return 1;
    }
    graphics3d.setClearColor(config.clearColor);

    // Get core systems (all in bestow namespace)
    auto& inputSys = container.service<InputSystemService>();
    systems.input = &inputSys;
    inputSys.initialize(graphics3d.getNativeWindowHandle());

    auto& entitySys = container.service<EntitySystemService>();
    systems.entities = &entitySys;

    auto& eventSys = container.service<EventSystemService>();
    systems.events = &eventSys;

    auto& assetSys = container.service<AssetSystemService>();
    systems.assets = &assetSys;

    auto& configSys = container.service<ConfigSystemService>();
    systems.config = &configSys;

    // Note: CameraSystem requires Size in constructor, can't use simple service()
    // Users can access camera via graphics3d->setCamera() directly

    // Audio system (optional - may fail if FMOD not available)
    try {
        auto& audioSys = container.service<AudioSystemService>();
        systems.audio = &audioSys;
    } catch (...) {
        // Audio not available - continue without it
    }

    // Inject systems into game
    game._setSystems(systems);

    // Call game's start method
    game.onStart();

    // Fixed timestep game loop
    constexpr DeltaTime fixedDt = 1.0f / 60.0f;  // 60 Hz
    auto previousTime = std::chrono::high_resolution_clock::now();
    DeltaTime accumulator = 0.0f;

    // Main game loop
    while (!graphics3d.shouldClose() && !game.shouldQuit()) {
        // Calculate delta time
        auto currentTime = std::chrono::high_resolution_clock::now();
        DeltaTime frameTime = std::chrono::duration<float>(currentTime - previousTime).count();
        previousTime = currentTime;

        // Clamp frame time to prevent spiral of death
        if (frameTime > 0.25f) {
            frameTime = 0.25f;
        }
        accumulator += frameTime;

        // Update input
        inputSys.update();

        // Update audio
        if (systems.audio) {
            systems.audio->update(frameTime);
        }

        // Fixed timestep updates
        while (accumulator >= fixedDt) {
            game.onUpdate(fixedDt);
            accumulator -= fixedDt;
        }

        // Render
        graphics3d.beginFrame();
        game.onRender();
        graphics3d.endFrame();
    }

    // Shutdown
    game.onShutdown();
    inputSys.shutdown();
    graphics3d.shutdown();

    return 0;
}

}  // namespace detail

//==========================================================================
// Public API
//==========================================================================

/// Run a game with the given configuration.
///
/// Example:
/// ```cpp
/// int main() {
///     return bestow::run<MyGame>({
///         .title = "My Awesome Game",
///         .width = 1920,
///         .height = 1080,
///         .vsync = true
///     });
/// }
/// ```
///
/// @tparam GameType Your game class that extends bestow::Game
/// @param config Configuration for window, graphics, etc.
/// @return Exit code (0 on success)
template<typename GameType>
int run(const GameConfig& config = {}) {
    return detail::runGame<GameType>(config);
}

}  // namespace bestow
