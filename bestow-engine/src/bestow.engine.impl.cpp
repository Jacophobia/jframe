// bestow-engine/src/bestow.engine.impl.cppm
// Engine Implementation with Custom Lightweight Dependency Injection
//
// This module wires all background systems together using a modern C++23
// service container pattern and provides the simplified public API.

module;

#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <spdlog/spdlog.h>

module bestow.engine;

import std;
import bestow.types;
import bestow.entity;
import bestow.entity.impl;
import bestow.graphics3d;
import bestow.graphics3d.impl;
import bestow.shader;
import bestow.shader.impl;
import bestow.assets;
import bestow.assets.impl;
import bestow.audio;
import bestow.audio.impl;
import bestow.input;
import bestow.input.impl;
import bestow.physics3d;
import bestow.physics3d.impl;
import bestow.events;
import bestow.events.impl;
import bestow.gamestate;
import bestow.gamestate.impl;
import bestow.level;
import bestow.level.impl;
import bestow.config;
import bestow.config.impl;
import bestow.ui;
import bestow.ui.impl;

namespace bestow {

//==========================================================================
// Constants
//==========================================================================

// Audio channel IDs for engine-managed channels
constexpr Channel MusicChannel = 0;
constexpr Channel SfxChannelStart = 1;

//==========================================================================
// Service Container - Lightweight C++23 Dependency Injection
//==========================================================================

/// ServiceContainer provides automatic dependency resolution and lifecycle
/// management for all engine systems. Systems are created lazily and wired
/// together using a modern service locator pattern.
class ServiceContainer {
public:
    ServiceContainer() = default;
    ~ServiceContainer() = default;

    // Non-copyable, non-movable (owns all systems)
    ServiceContainer(const ServiceContainer&) = delete;
    ServiceContainer& operator=(const ServiceContainer&) = delete;
    ServiceContainer(ServiceContainer&&) = delete;
    ServiceContainer& operator=(ServiceContainer&&) = delete;

    //======================================================================
    // System Accessors - Lazy initialization with dependency resolution
    //======================================================================

    IAssetSystem& assets() {
        if (!assets_) {
            assets_ = createAssetSystem();
            spdlog::debug("ServiceContainer: Created AssetSystem");
        }
        return *assets_;
    }

    IEventSystem& events() {
        if (!events_) {
            events_ = createEventSystem();
            spdlog::debug("ServiceContainer: Created EventSystem");
        }
        return *events_;
    }

    IEntitySystem& entities() {
        if (!entities_) {
            entities_ = createEntitySystem();
            spdlog::debug("ServiceContainer: Created EntitySystem");
        }
        return *entities_;
    }

    IInputSystem& input() {
        if (!input_) {
            input_ = createInputSystem();
            spdlog::debug("ServiceContainer: Created InputSystem");
        }
        return *input_;
    }

    IShaderSystem& shaders() {
        if (!shaders_) {
            shaders_ = createShaderSystem();
            if (shaders_) {
                shaders_->setAssetSystem(&assets());
            }
            spdlog::debug("ServiceContainer: Created ShaderSystem");
        }
        return *shaders_;
    }

    IGraphics3DSystem& graphics3d() {
        if (!graphics3d_) {
            graphics3d_ = createGraphics3DSystem();
            if (graphics3d_) {
                graphics3d_->setAssetSystem(&assets());
                graphics3d_->setShaderSystem(&shaders());
            }
            spdlog::debug("ServiceContainer: Created Graphics3DSystem");
        }
        return *graphics3d_;
    }

    IAudioSystem& audio() {
        if (!audio_) {
            audio_ = createAudioSystem();
            spdlog::debug("ServiceContainer: Created AudioSystem");
        }
        return *audio_;
    }

    IPhysics3DSystem& physics3d() {
        if (!physics3d_) {
            physics3d_ = createPhysics3DSystem();
            spdlog::debug("ServiceContainer: Created Physics3DSystem");
        }
        return *physics3d_;
    }

    IGameStateSystem& gameStates() {
        if (!gameStates_) {
            gameStates_ = createGameStateSystem();
            spdlog::debug("ServiceContainer: Created GameStateSystem");
        }
        return *gameStates_;
    }

    ILevelSystem& levels() {
        if (!levels_) {
            levels_ = createLevelSystem();
            spdlog::debug("ServiceContainer: Created LevelSystem");
        }
        return *levels_;
    }

    IConfigSystem& config() {
        if (!config_) {
            config_ = createConfigSystem();
            spdlog::debug("ServiceContainer: Created ConfigSystem");
        }
        return *config_;
    }

    IUISystem& ui() {
        if (!ui_) {
            ui_ = createUISystem();
            spdlog::debug("ServiceContainer: Created UISystem");
        }
        return *ui_;
    }

    //======================================================================
    // Initialization - Pre-create all systems in dependency order
    //======================================================================

    void initializeAll() {
        // Tier 0: Foundation (no deps)
        events();
        config();

        // Tier 1: Core systems
        assets();
        entities();
        input();

        // Tier 2: Systems with dependencies
        shaders();
        audio();
        physics3d();
        gameStates();

        // Tier 3: High-level systems
        graphics3d();
        levels();
        ui();

        spdlog::info("ServiceContainer: All systems initialized");
    }

private:
    // Background systems (hidden from users)
    std::unique_ptr<IAssetSystem> assets_;
    std::unique_ptr<IShaderSystem> shaders_;
    std::unique_ptr<IConfigSystem> config_;

    // User-accessible systems (exposed through facades)
    std::unique_ptr<IEventSystem> events_;
    std::unique_ptr<IEntitySystem> entities_;
    std::unique_ptr<IInputSystem> input_;
    std::unique_ptr<IGraphics3DSystem> graphics3d_;
    std::unique_ptr<IAudioSystem> audio_;
    std::unique_ptr<IPhysics3DSystem> physics3d_;
    std::unique_ptr<IGameStateSystem> gameStates_;
    std::unique_ptr<ILevelSystem> levels_;
    std::unique_ptr<IUISystem> ui_;
};

//==========================================================================
// World Implementation
//==========================================================================

class WorldImpl : public IWorld {
public:
    WorldImpl(ServiceContainer& container)
        : container_(container)
        , entities_(&container.entities())
        , physics_(&container.physics3d())
        , graphics_(&container.graphics3d())
    {}

    // Entity Management
    Entity createEntity() override {
        return entities_->createEntity();
    }

    Entity createEntity(const Vec3& position) override {
        Entity e = entities_->createEntity();
        entities_->emplace<Transform3D>(e, Transform3D{.position = position});
        return e;
    }

    Entity createFromBlueprint(std::string_view blueprintPath) override {
        // Load blueprint from assets and instantiate
        // For now, just create an empty entity
        spdlog::warn("Blueprint loading not yet implemented: {}", blueprintPath);
        return createEntity();
    }

    void destroyEntity(Entity entity) override {
        entities_->destroyEntity(entity);
    }

    bool isValid(Entity entity) const override {
        return entities_->isValid(entity);
    }

    // Physics Helpers
    std::optional<RaycastHit3D> raycast(const Vec3& origin, const Vec3& direction,
                                        float maxDistance) const override {
        // Use default QueryFilter3D (all layers)
        return physics_->raycast(origin, direction, maxDistance, QueryFilter3D{});
    }

    std::vector<RaycastHit3D> raycastAll(const Vec3& origin, const Vec3& direction,
                                         float maxDistance) const override {
        return physics_->raycastAll(origin, direction, maxDistance, QueryFilter3D{});
    }

    std::vector<Entity> overlapSphere(const Vec3& center, float radius) const override {
        return physics_->overlapSphere(center, radius, QueryFilter3D{});
    }

    // Camera Control
    void setCameraPosition(const Vec3& position) override {
        Camera3D cam = graphics_->getCamera();
        cam.transform.position = position;
        graphics_->setCamera(cam);
    }

    void setCameraTarget(const Vec3& target) override {
        Camera3D cam = graphics_->getCamera();
        Vec3 direction = glm::normalize(target - cam.transform.position);
        // Convert direction to quaternion rotation using lookAt
        cam.transform.rotation = glm::quatLookAt(direction, Vec3{0.0f, 1.0f, 0.0f});
        graphics_->setCamera(cam);
    }

    void setCameraRotation(const Quat& rotation) override {
        Camera3D cam = graphics_->getCamera();
        cam.transform.rotation = rotation;
        graphics_->setCamera(cam);
    }

    Vec3 getCameraPosition() const override {
        return graphics_->getCamera().transform.position;
    }

    Quat getCameraRotation() const override {
        return graphics_->getCamera().transform.rotation;
    }

    Vec3 getCameraForward() const override {
        Quat rot = getCameraRotation();
        return rot * Vec3{0.0f, 0.0f, -1.0f};
    }

    Vec3 screenToWorld(Vec2 screenPos, float depth) const override {
        Ray3D ray = graphics_->screenToWorldRay(screenPos);
        return ray.pointAt(depth);
    }

    Vec2 worldToScreen(const Vec3& worldPos) const override {
        auto result = graphics_->worldToScreen(worldPos);
        return result.value_or(Vec2{-1.0f, -1.0f});
    }

    // Debug Visualization
    void debugLine(const Vec3& start, const Vec3& end, const Color& color,
                  float duration) override {
        graphics_->debugDrawLine(start, end, color, duration, true);
    }

    void debugBox(const Vec3& center, const Vec3& halfExtents, const Color& color,
                 float duration) override {
        graphics_->debugDrawBox(center, halfExtents, Quat{1.0f, 0.0f, 0.0f, 0.0f}, color, duration, true);
    }

    void debugSphere(const Vec3& center, float radius, const Color& color,
                    float duration) override {
        graphics_->debugDrawSphere(center, radius, color, duration, true);
    }

    // Internal Access
    IEntitySystem& entities() override { return *entities_; }
    const IEntitySystem& entities() const override { return *entities_; }

    IPhysics3DSystem& physics() override { return *physics_; }
    const IPhysics3DSystem& physics() const override { return *physics_; }

private:
    ServiceContainer& container_;
    IEntitySystem* entities_;
    IPhysics3DSystem* physics_;
    IGraphics3DSystem* graphics_;
};

//==========================================================================
// Audio Implementation (Simplified Facade)
//==========================================================================

class AudioImpl : public IAudio {
public:
    AudioImpl(ServiceContainer& container)
        : audio_(&container.audio())
        , assets_(&container.assets())
    {}

    void playSound(std::string_view path, float volume) override {
        AssetHandle handle = assets_->registerAsset(AssetType::Sound, path);
        if (assets_->getAssetState(handle) != AssetState::Loaded) {
            assets_->loadAsset(handle);
        }
        // Use positional audio for fire-and-forget sounds (non-3D)
        audio_->playPositional(PositionalSound{
            .asset = handle,
            .position = Vec3{0.0f},
            .volume = volume,
            .pitch = 1.0f,
            .minDistance = 1.0f,
            .maxDistance = 1000.0f
        });
    }

    void playSoundAt(std::string_view path, const Vec3& position, float volume) override {
        AssetHandle handle = assets_->registerAsset(AssetType::Sound, path);
        if (assets_->getAssetState(handle) != AssetState::Loaded) {
            assets_->loadAsset(handle);
        }
        audio_->playPositional(PositionalSound{
            .asset = handle,
            .position = position,
            .volume = volume,
            .pitch = 1.0f,
            .minDistance = 1.0f,
            .maxDistance = 100.0f
        });
    }

    void playMusic(std::string_view path, float volume, bool loop) override {
        AssetHandle handle = assets_->registerAsset(AssetType::Music, path);
        if (assets_->getAssetState(handle) != AssetState::Loaded) {
            assets_->loadAsset(handle);
        }
        audio_->playOnChannel(MusicChannel, ChannelSound{
            .asset = handle,
            .volume = volume,
            .pitch = 1.0f,
            .looping = loop,
            .fadeInTime = 0.0f
        });
    }

    void stopMusic(float fadeOutTime) override {
        audio_->stopChannel(MusicChannel, fadeOutTime);
    }

    void pauseAll() override {
        audio_->pauseAll();
    }

    void resumeAll() override {
        audio_->resumeAll();
    }

    void setMasterVolume(float volume) override {
        audio_->setMasterVolume(volume);
    }

    void setMusicVolume(float volume) override {
        audio_->setChannelVolume(MusicChannel, volume);
    }

    void setSfxVolume(float volume) override {
        audio_->setGroupVolume("sfx", volume);
    }

private:
    IAudioSystem* audio_;
    IAssetSystem* assets_;
};

//==========================================================================
// Settings Implementation
//==========================================================================

class SettingsImpl : public ISettings {
public:
    SettingsImpl(ServiceContainer& container, EngineSettings initialSettings)
        : container_(container)
        , graphics_(&container.graphics3d())
        , audio_(&container.audio())
        , physics_(&container.physics3d())
        , settings_(std::move(initialSettings))
    {}

    const EngineSettings& current() const override {
        return settings_;
    }

    void apply(const EngineSettings& settings) override {
        settings_ = settings;
        applyWindow(settings.window);
        applyGraphics(settings.graphics);
        applyAudio(settings.audio);
        applyPhysics(settings.physics);
    }

    void applyWindow(const WindowSettings& settings) override {
        settings_.window = settings;
        graphics_->setWindowSize({settings.width, settings.height});
        graphics_->setFullscreen(settings.fullscreen);
        graphics_->setVSync(settings.vsync);
    }

    void applyGraphics(const GraphicsSettings& settings) override {
        settings_.graphics = settings;
        graphics_->setRenderScale(settings.renderScale);
        graphics_->setShadowsEnabled(settings.shadows);
        graphics_->setDirectionalShadowResolution(settings.shadowResolution);
        graphics_->setBloom(settings.bloom, 1.0f, 1.0f);
        graphics_->setSSAO(settings.ssao, 0.5f, 1.0f);

        Camera3D cam = graphics_->getCamera();
        cam.fovY = settings.fov;
        cam.nearPlane = settings.nearPlane;
        cam.farPlane = settings.farPlane;
        graphics_->setCamera(cam);
    }

    void applyAudio(const AudioSettings& settings) override {
        settings_.audio = settings;
        audio_->setMasterVolume(settings.masterVolume);
        audio_->setChannelVolume(MusicChannel, settings.musicVolume);
        audio_->setGroupVolume("sfx", settings.sfxVolume);
    }

    void applyPhysics(const PhysicsSettings& settings) override {
        settings_.physics = settings;
        physics_->setGravity({0.0f, settings.gravity, 0.0f});
    }

    void loadFromFile(std::string_view path) override {
        settings_ = EngineSettings::fromFile(path);
        apply(settings_);
    }

    void saveToFile(std::string_view path) const override {
        settings_.saveToFile(path);
    }

private:
    ServiceContainer& container_;
    IGraphics3DSystem* graphics_;
    IAudioSystem* audio_;
    IPhysics3DSystem* physics_;
    EngineSettings settings_;
};

//==========================================================================
// Engine Implementation
//==========================================================================

class EngineImpl : public IEngine {
public:
    explicit EngineImpl(EngineSettings settings = {})
        : container_(std::make_unique<ServiceContainer>())
        , initialSettings_(std::move(settings))
    {
        // Initialize all systems in dependency order
        container_->initializeAll();

        // Create user-facing facades
        world_ = std::make_unique<WorldImpl>(*container_);
        audio_ = std::make_unique<AudioImpl>(*container_);
        settings_ = std::make_unique<SettingsImpl>(*container_, initialSettings_);

        // Apply initial settings
        settings_->apply(initialSettings_);

        startTime_ = std::chrono::high_resolution_clock::now();

        spdlog::info("Engine initialized successfully");
    }

    ~EngineImpl() override {
        spdlog::info("Engine shutting down");
    }

    // User-facing systems
    IWorld& world() override { return *world_; }
    const IWorld& world() const override { return *world_; }

    IInputSystem& input() override { return container_->input(); }
    const IInputSystem& input() const override { return container_->input(); }

    IAudio& audio() override { return *audio_; }

    IGameStateSystem& gameStates() override { return container_->gameStates(); }
    IEventSystem& events() override { return container_->events(); }
    ILevelSystem& levels() override { return container_->levels(); }

    ISettings& settings() override { return *settings_; }

    // Lifecycle
    void quit() override { running_ = false; }
    bool isRunning() const override { return running_; }

    float getTime() const override {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<float>(now - startTime_).count();
    }

    float getDeltaTime() const override { return deltaTime_; }
    float getFixedDeltaTime() const override { return settings_->current().physics.fixedTimestep; }

    // Internal: Update loop (called by runGame)
    void update(float dt) {
        deltaTime_ = dt;

        // Update background systems
        container_->input().update();
        container_->shaders().update();  // Hot reload

        // Fixed timestep physics
        accumulator_ += dt;
        float fixedDt = getFixedDeltaTime();
        while (accumulator_ >= fixedDt) {
            container_->physics3d().update(fixedDt);
            accumulator_ -= fixedDt;
        }

        // Update audio
        container_->audio().update(dt);

        // Update UI
        container_->ui().update(dt);
    }

    // Internal: Render loop (called by runGame)
    void render() {
        auto& graphics = container_->graphics3d();
        auto& entities = container_->entities();

        graphics.beginFrame();

        // Render all entities with visual components
        graphics.renderEntities(entities);
        graphics.updateEntityLights(entities);

        // Render UI
        container_->ui().render();

        graphics.endFrame();
    }

    // Internal: Check if window should close
    bool shouldClose() const {
        return container_->graphics3d().shouldClose();
    }

    // Internal: Get native window for input
    void* getNativeWindow() const {
        return container_->graphics3d().getNativeWindowHandle();
    }

    // Internal: Access service container for advanced use
    ServiceContainer& services() { return *container_; }

private:
    std::unique_ptr<ServiceContainer> container_;
    EngineSettings initialSettings_;

    // User-facing wrappers
    std::unique_ptr<WorldImpl> world_;
    std::unique_ptr<AudioImpl> audio_;
    std::unique_ptr<SettingsImpl> settings_;

    // State
    bool running_ = true;
    float deltaTime_ = 0.0f;
    float accumulator_ = 0.0f;
    std::chrono::high_resolution_clock::time_point startTime_;
};

//==========================================================================
// Settings File I/O Implementation (definitions for interface declarations)
//==========================================================================

EngineSettings EngineSettings::fromFile(std::string_view path) {
    spdlog::info("Loading engine settings from: {}", path);
    // TODO: Implement Lua/JSON parsing using config system
    return EngineSettings{};
}

void EngineSettings::saveToFile(std::string_view path) const {
    spdlog::info("Saving engine settings to: {}", path);
    // TODO: Implement Lua/JSON serialization
}

//==========================================================================
// Factory Functions Implementation (definitions for interface declarations)
//==========================================================================

std::unique_ptr<IEngine> createEngine() {
    return std::make_unique<EngineImpl>();
}

std::unique_ptr<IEngine> createEngine(const EngineSettings& settings) {
    return std::make_unique<EngineImpl>(settings);
}

std::unique_ptr<IEngine> createEngineFromFile(std::string_view settingsPath) {
    EngineSettings settings = EngineSettings::fromFile(settingsPath);
    return std::make_unique<EngineImpl>(settings);
}

//==========================================================================
// Game Runner Implementation (definitions for interface declarations)
//==========================================================================

int runGame(Game& game, const EngineSettings& settings) {
    auto engine = std::make_unique<EngineImpl>(settings);

    // Get impl for internal access
    auto* engineImpl = engine.get();

    // Initialize
    game.onStart(*engine);

    // Main loop
    auto lastTime = std::chrono::high_resolution_clock::now();

    while (engine->isRunning() && !engineImpl->shouldClose()) {
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        // Cap delta time to prevent spiral of death
        dt = std::min(dt, 0.25f);

        // Update engine systems
        engineImpl->update(dt);

        // Game updates
        game.onUpdate(*engine, dt);

        // Fixed update for physics
        static float accumulator = 0.0f;
        accumulator += dt;
        float fixedDt = engine->getFixedDeltaTime();
        while (accumulator >= fixedDt) {
            game.onFixedUpdate(*engine, fixedDt);
            accumulator -= fixedDt;
        }

        // Render
        engineImpl->render();
    }

    // Cleanup
    game.onShutdown(*engine);

    return 0;
}

int runGame(Game& game, std::string_view settingsPath) {
    EngineSettings settings = EngineSettings::fromFile(settingsPath);
    return runGame(game, settings);
}

}  // namespace bestow
