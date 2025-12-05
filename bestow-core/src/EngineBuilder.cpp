// bestow-core/src/EngineBuilder.cpp
// Engine builder implementation for system composition

module;

#include <GLFW/glfw3.h>
// MSVC C++23 module compatibility - include full EnTT before import std
#include <bestow/entt_compat.hpp>

module bestow.core;

import bestow.events.impl;
import bestow.entity.impl;
import bestow.physics.impl;
import bestow.graphics.impl;
import bestow.audio.impl;
import bestow.input.impl;
import bestow.assets.impl;
import bestow.save.impl;
import bestow.level.impl;
import bestow.ai.impl;
import bestow.camera.impl;
import bestow.gas.impl;
import bestow.blueprints.impl;

namespace bestow::core {

//==============================================================================
// Engine::Impl - Define here after all imports
//==============================================================================

struct Engine::Impl {
    // System ownership (unique_ptr ensures proper cleanup order)
    std::unique_ptr<EventSystem> events;
    std::unique_ptr<EntitySystem> entities;
    std::unique_ptr<Box2DPhysicsSystem> physics;
    std::unique_ptr<GraphicsSystem> graphics;
    std::unique_ptr<FMODAudioSystem> audio;
    std::unique_ptr<InputSystem> input;
    std::unique_ptr<AssetSystem> assets;
    std::unique_ptr<SaveSystem> save;
    std::unique_ptr<LevelSystem> level;
    std::unique_ptr<AISystem> ai;
    std::unique_ptr<CameraSystem> camera;
    std::unique_ptr<GASSystem> gas;
    std::unique_ptr<BlueprintFactory> blueprints;

    // Engine aggregate (raw pointers to systems)
    BestowEngine engineAggregate;

    // Game loop state
    bool running = false;
    Timer runTimer;
};

//==============================================================================
// EngineBuilder::Impl
//==============================================================================

struct EngineBuilder::Impl {
    // System instances (owned by builder during construction)
    std::unique_ptr<EventSystem> events;
    std::unique_ptr<EntitySystem> entities;
    std::unique_ptr<Box2DPhysicsSystem> physics;
    std::unique_ptr<GraphicsSystem> graphics;
    std::unique_ptr<FMODAudioSystem> audio;
    std::unique_ptr<InputSystem> input;
    std::unique_ptr<AssetSystem> assets;
    std::unique_ptr<SaveSystem> save;
    std::unique_ptr<LevelSystem> level;
    std::unique_ptr<AISystem> ai;
    std::unique_ptr<CameraSystem> camera;
    std::unique_ptr<GASSystem> gas;
    std::unique_ptr<BlueprintFactory> blueprints;

    // Configuration
    std::optional<GraphicsConfig> graphicsConfig;
    std::string assetsBasePath = "assets";
    std::string savePath = "saves";
    std::optional<Size> cameraViewportSize;

    // Flags for which systems to enable
    bool enableEvents = false;
    bool enableEntities = false;
    bool enablePhysics = false;
    bool enableGraphics = false;
    bool enableAudio = false;
    bool enableInput = false;
    bool enableAssets = false;
    bool enableSave = false;
    bool enableLevel = false;
    bool enableAI = false;
    bool enableCamera = false;
    bool enableGAS = false;
    bool enableBlueprints = false;
};

//==============================================================================
// EngineBuilder
//==============================================================================

EngineBuilder::EngineBuilder() : impl_(std::make_unique<Impl>()) {}

EngineBuilder::~EngineBuilder() = default;

EngineBuilder& EngineBuilder::withEvents() {
    impl_->enableEvents = true;
    return *this;
}

EngineBuilder& EngineBuilder::withEntities() {
    impl_->enableEntities = true;
    return *this;
}

EngineBuilder& EngineBuilder::withPhysics() {
    impl_->enablePhysics = true;
    return *this;
}

EngineBuilder& EngineBuilder::withGraphics(GraphicsConfig config) {
    impl_->enableGraphics = true;
    impl_->graphicsConfig = config;
    return *this;
}

EngineBuilder& EngineBuilder::withAudio() {
    impl_->enableAudio = true;
    return *this;
}

EngineBuilder& EngineBuilder::withInput() {
    impl_->enableInput = true;
    return *this;
}

EngineBuilder& EngineBuilder::withAssets(std::string_view basePath) {
    impl_->enableAssets = true;
    impl_->assetsBasePath = basePath;
    return *this;
}

EngineBuilder& EngineBuilder::withSave(std::string_view savePath) {
    impl_->enableSave = true;
    impl_->savePath = savePath;
    return *this;
}

EngineBuilder& EngineBuilder::withLevel() {
    impl_->enableLevel = true;
    return *this;
}

EngineBuilder& EngineBuilder::withAI() {
    impl_->enableAI = true;
    return *this;
}

EngineBuilder& EngineBuilder::withCamera(Size viewportSize) {
    impl_->enableCamera = true;
    impl_->cameraViewportSize = viewportSize;
    return *this;
}

EngineBuilder& EngineBuilder::withGAS() {
    impl_->enableGAS = true;
    return *this;
}

EngineBuilder& EngineBuilder::withBlueprints() {
    impl_->enableBlueprints = true;
    return *this;
}

std::expected<Engine, std::string> EngineBuilder::build() {
    // Validate dependencies
    if (impl_->enableGraphics && !impl_->graphicsConfig.has_value()) {
        return std::unexpected("Graphics system requires GraphicsConfig");
    }

    // Initialize systems in dependency order
    // Phase 1: Events (no dependencies)
    if (impl_->enableEvents) {
        impl_->events = std::make_unique<EventSystem>();
        logInfo("Events system initialized");
    }

    // Phase 2: Assets (no dependencies)
    if (impl_->enableAssets) {
        impl_->assets = std::make_unique<AssetSystem>();
        // AssetSystem has default constructor, no separate initialization
        logInfo("Assets system initialized (base path: " + impl_->assetsBasePath + ")");
    }

    // Phase 3: Graphics (needs to create window first)
    if (impl_->enableGraphics) {
        impl_->graphics = std::make_unique<GraphicsSystem>();
        auto& cfg = impl_->graphicsConfig.value();
        if (!impl_->graphics->initialize(cfg.width, cfg.height, cfg.title)) {
            return std::unexpected("Failed to initialize graphics system");
        }
        impl_->graphics->setVSync(cfg.vsync);
        impl_->graphics->setClearColor(cfg.clearColor);
        logInfo("Graphics system initialized");
    }

    // Phase 4: Input (depends on graphics window)
    if (impl_->enableInput) {
        if (!impl_->enableGraphics) {
            return std::unexpected("Input system requires graphics system (for window)");
        }
        impl_->input = std::make_unique<InputSystem>();
        auto* window = static_cast<GLFWwindow*>(impl_->graphics->getNativeWindowHandle());
        if (!impl_->input->initialize(window)) {
            return std::unexpected("Failed to initialize input system");
        }
        logInfo("Input system initialized");
    }

    // Phase 5: Entities (no dependencies)
    if (impl_->enableEntities) {
        impl_->entities = std::make_unique<EntitySystem>();
        logInfo("Entity system initialized");
    }

    // Phase 6: Physics (no dependencies, but typically used with entities)
    if (impl_->enablePhysics) {
        impl_->physics = std::make_unique<Box2DPhysicsSystem>();
        if (!impl_->physics->initialize()) {
            return std::unexpected("Failed to initialize physics system");
        }
        logInfo("Physics system initialized");
    }

    // Phase 7: Audio (depends on assets for loading)
    if (impl_->enableAudio) {
        if (!impl_->enableAssets) {
            logWarn("Audio system created without asset system - may have limited functionality");
        }
        impl_->audio = std::make_unique<FMODAudioSystem>();
        if (!impl_->audio->initialize()) {
            return std::unexpected("Failed to initialize audio system");
        }
        logInfo("Audio system initialized");
    }

    // Phase 8: Save (no dependencies)
    if (impl_->enableSave) {
        impl_->save = std::make_unique<SaveSystem>();
        // SaveSystem has default constructor, no setSaveDirectory method
        // Save path is configured via save/load operations
        logInfo("Save system initialized (save path: " + impl_->savePath + ")");
    }

    // Phase 9: Level (depends on assets)
    if (impl_->enableLevel) {
        if (!impl_->enableAssets) {
            return std::unexpected("Level system requires asset system");
        }
        impl_->level = std::make_unique<LevelSystem>();
        if (!impl_->level->initialize(impl_->assets.get())) {
            return std::unexpected("Failed to initialize level system");
        }
        logInfo("Level system initialized");
    }

    // Phase 10: AI (depends on physics for line-of-sight and assets for navmesh)
    if (impl_->enableAI) {
        // AISystem requires physics system for line-of-sight queries
        if (!impl_->enablePhysics) {
            logWarn("AI system created without physics system - line-of-sight will be disabled");
        }
        if (!impl_->enableAssets) {
            logWarn("AI system created without assets system - navmesh loading will be disabled");
        }
        impl_->ai = std::make_unique<AISystem>(impl_->physics.get(), impl_->assets.get());
        if (!impl_->ai->initialize()) {
            return std::unexpected("Failed to initialize AI system");
        }
        logInfo("AI system initialized");
    }

    // Phase 11: Camera (depends on graphics for viewport size)
    if (impl_->enableCamera) {
        if (!impl_->cameraViewportSize.has_value()) {
            return std::unexpected("Camera system requires viewport size");
        }
        impl_->camera = std::make_unique<CameraSystem>(impl_->cameraViewportSize.value());
        logInfo("Camera system initialized");
    }

    // Phase 12: GAS (Gameplay Ability System - no dependencies)
    if (impl_->enableGAS) {
        impl_->gas = std::make_unique<GASSystem>();
        if (!impl_->gas->initialize()) {
            return std::unexpected("Failed to initialize GAS system");
        }
        logInfo("GAS system initialized");
    }

    // Phase 13: Blueprints (depends on entities and optionally physics)
    if (impl_->enableBlueprints) {
        if (!impl_->enableEntities) {
            return std::unexpected("Blueprint system requires entity system");
        }
        impl_->blueprints = std::make_unique<BlueprintFactory>(
            *impl_->entities,
            impl_->physics.get()
        );
        logInfo("Blueprint factory initialized");
    }

    // Transfer ownership to Engine
    Engine engine;
    engine.impl_->events = std::move(impl_->events);
    engine.impl_->entities = std::move(impl_->entities);
    engine.impl_->physics = std::move(impl_->physics);
    engine.impl_->graphics = std::move(impl_->graphics);
    engine.impl_->audio = std::move(impl_->audio);
    engine.impl_->input = std::move(impl_->input);
    engine.impl_->assets = std::move(impl_->assets);
    engine.impl_->save = std::move(impl_->save);
    engine.impl_->level = std::move(impl_->level);
    engine.impl_->ai = std::move(impl_->ai);
    engine.impl_->camera = std::move(impl_->camera);
    engine.impl_->gas = std::move(impl_->gas);
    engine.impl_->blueprints = std::move(impl_->blueprints);

    // Wire graphics to assets for texture loading
    if (engine.impl_->graphics && engine.impl_->assets) {
        engine.impl_->graphics->setAssetSystem(engine.impl_->assets.get());
        logInfo("Graphics-to-Assets wiring complete");
    }

    // Populate BestowEngine aggregate
    engine.impl_->engineAggregate.events = engine.impl_->events.get();
    engine.impl_->engineAggregate.assets = engine.impl_->assets.get();
    engine.impl_->engineAggregate.entities = engine.impl_->entities.get();
    engine.impl_->engineAggregate.graphics = engine.impl_->graphics.get();
    engine.impl_->engineAggregate.audio = engine.impl_->audio.get();
    engine.impl_->engineAggregate.input = engine.impl_->input.get();
    engine.impl_->engineAggregate.physics = engine.impl_->physics.get();
    engine.impl_->engineAggregate.levels = engine.impl_->level.get();
    engine.impl_->engineAggregate.save = engine.impl_->save.get();
    engine.impl_->engineAggregate.ai = engine.impl_->ai.get();
    engine.impl_->engineAggregate.camera = engine.impl_->camera.get();
    engine.impl_->engineAggregate.gas = engine.impl_->gas.get();
    engine.impl_->engineAggregate.blueprints = engine.impl_->blueprints.get();

    // Wire physics callbacks to events system
    if (engine.impl_->physics && engine.impl_->events) {
        auto* events = engine.impl_->events.get();

        // Physical collisions publish Collision events
        engine.impl_->physics->setCollisionCallback([events](const CollisionEvent& collision) {
            events->publish(Events::Collision, collision);
        });

        // Sensor enter events publish TriggerEnter events
        engine.impl_->physics->setTriggerEnterCallback([events](const TriggerEvent& trigger) {
            events->publish(Events::TriggerEnter, trigger);
        });

        // Sensor exit events publish TriggerExit events
        engine.impl_->physics->setTriggerExitCallback([events](const TriggerEvent& trigger) {
            events->publish(Events::TriggerExit, trigger);
        });

        logInfo("Physics-to-Events wiring complete");
    }

    logInfo("Engine built successfully");
    return engine;
}

}  // namespace bestow::core
