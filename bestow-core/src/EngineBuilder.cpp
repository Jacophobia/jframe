// bestow-core/src/EngineBuilder.cpp
// Engine builder implementation for system composition

module;

#include <kangaru/kangaru.hpp>

#include <GLFW/glfw3.h>
// MSVC C++23 module compatibility - include full EnTT before import std
#include <bestow/entt_compat.hpp>
#include <spdlog/spdlog.h>

module bestow.core;

// Import interfaces
import bestow.events;
import bestow.entity;
import bestow.physics;
import bestow.graphics;
import bestow.audio;
import bestow.input;
import bestow.assets;
import bestow.save;
import bestow.level;
import bestow.ai;
import bestow.camera;
import bestow.gas;
import bestow.blueprints;
import bestow.services;

// Import implementations for DI registration
import bestow.events.impl;
import bestow.entity.impl;
import bestow.physics.impl;
import bestow.opengl.impl;
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
    // Kangaru DI container
    kgr::container container;
    
    // Core systems
    std::unique_ptr<JobSystem> jobs;

    // Engine aggregate (raw pointers to systems from DI container)
    BestowEngine engineAggregate;

    // Game loop state
    bool running = false;
    Timer runTimer;
};

//==============================================================================
// EngineBuilder::Impl
//==============================================================================

struct EngineBuilder::Impl {
    // Kangaru DI container
    kgr::container container;
    
    // Core systems
    std::unique_ptr<JobSystem> jobs;

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

    // Initialize core systems first
    // Phase 0: JobSystem (no dependencies, required by many systems)
    impl_->jobs = std::make_unique<JobSystem>();
    spdlog::info("JobSystem initialized with {} worker threads", impl_->jobs->workerCount());

    // Resolve systems from DI container (systems are auto-registered by importing .impl modules)
    // Phase 1: Events (no dependencies)
    IEventSystem* eventSystem = nullptr;
    if (impl_->enableEvents) {
        auto& events = impl_->container.service<EventSystemService>();
        eventSystem = &events;
        logInfo("Events system resolved from DI container");
    }

    // Phase 2: Entities (no dependencies)
    IEntitySystem* entitySystem = nullptr;
    if (impl_->enableEntities) {
        auto& entities = impl_->container.service<EntitySystemService>();
        entitySystem = &entities;
        logInfo("Entity system resolved from DI container");
    }

    // Phase 3: Assets (no dependencies)
    IAssetSystem* assetSystem = nullptr;
    if (impl_->enableAssets) {
        auto& assets = impl_->container.service<AssetSystemService>();
        assetSystem = &assets;
        logInfo("Assets system resolved from DI container (base path: " + impl_->assetsBasePath + ")");
    }

    // Phase 4: Graphics (needs configuration and initialization)
    IGraphicsSystem* graphicsSystem = nullptr;
    if (impl_->enableGraphics) {
        auto& graphics = impl_->container.service<GraphicsSystemService>();
        graphicsSystem = &graphics;
        
        // Cast to concrete implementation for initialization
        auto* graphicsImpl = dynamic_cast<OpenGLGraphicsSystem*>(&graphics);
        if (!graphicsImpl) {
            return std::unexpected("Failed to cast graphics system to concrete implementation");
        }
        
        auto& cfg = impl_->graphicsConfig.value();
        if (!graphicsImpl->initialize(cfg.width, cfg.height, cfg.title)) {
            return std::unexpected("Failed to initialize graphics system");
        }
        graphicsImpl->setVSync(cfg.vsync);
        graphicsImpl->setClearColor(cfg.clearColor);
        logInfo("Graphics system resolved and initialized");
    }

    // Phase 5: Input (depends on graphics window)
    IInputSystem* inputSystem = nullptr;
    if (impl_->enableInput) {
        if (!impl_->enableGraphics) {
            return std::unexpected("Input system requires graphics system (for window)");
        }
        auto& input = impl_->container.service<InputSystemService>();
        inputSystem = &input;
        
        // Initialize through interface (no casting needed)
        auto* window = graphicsSystem->getNativeWindowHandle();
        if (!input.initialize(window)) {
            return std::unexpected("Failed to initialize input system");
        }
        logInfo("Input system resolved and initialized");
    }

    // Phase 6: Physics (no dependencies, but needs initialization)
    IPhysicsSystem* physicsSystem = nullptr;
    if (impl_->enablePhysics) {
        auto& physics = impl_->container.service<PhysicsSystemService>();
        physicsSystem = &physics;
        
        // Cast to concrete implementation for initialization
        auto* physicsImpl = dynamic_cast<Box2DPhysicsSystem*>(&physics);
        if (!physicsImpl) {
            return std::unexpected("Failed to cast physics system to concrete implementation");
        }
        
        if (!physicsImpl->initialize()) {
            return std::unexpected("Failed to initialize physics system");
        }
        logInfo("Physics system resolved and initialized");
    }

    // Phase 7: Audio (depends on assets for loading)
    IAudioSystem* audioSystem = nullptr;
    if (impl_->enableAudio) {
        if (!impl_->enableAssets) {
            logWarn("Audio system created without asset system - may have limited functionality");
        }
        auto& audio = impl_->container.service<AudioSystemService>();
        audioSystem = &audio;
        
        // Initialize through interface (no casting needed)
        if (!audio.initialize()) {
            return std::unexpected("Failed to initialize audio system");
        }
        logInfo("Audio system resolved and initialized");
    }

    // Phase 8: Save (no dependencies)
    ISaveSystem* saveSystem = nullptr;
    if (impl_->enableSave) {
        auto& save = impl_->container.service<SaveSystemService>();
        saveSystem = &save;
        logInfo("Save system resolved from DI container (save path: " + impl_->savePath + ")");
    }

    // Phase 9: Level (depends on assets)
    ILevelSystem* levelSystem = nullptr;
    if (impl_->enableLevel) {
        if (!impl_->enableAssets) {
            return std::unexpected("Level system requires asset system");
        }
        auto& level = impl_->container.service<LevelSystemService>();
        levelSystem = &level;
        
        // Cast to concrete implementation for initialization
        auto* levelImpl = dynamic_cast<LevelSystem*>(&level);
        if (!levelImpl) {
            return std::unexpected("Failed to cast level system to concrete implementation");
        }
        
        if (!levelImpl->initialize(assetSystem)) {
            return std::unexpected("Failed to initialize level system");
        }
        logInfo("Level system resolved and initialized");
    }

    // Phase 10: AI (depends on physics for line-of-sight and assets for navmesh)
    IAISystem* aiSystem = nullptr;
    if (impl_->enableAI) {
        if (!impl_->enablePhysics) {
            logWarn("AI system created without physics system - line-of-sight will be disabled");
        }
        if (!impl_->enableAssets) {
            logWarn("AI system created without assets system - navmesh loading will be disabled");
        }

        // AISystem has constructor dependencies - manually emplace then retrieve service
        impl_->container.emplace<AISystemService>(physicsSystem, assetSystem);
        auto& ai = impl_->container.service<IAISystemService>();
        aiSystem = &ai;

        // Cast to concrete implementation for initialization
        auto* aiImpl = dynamic_cast<AISystem*>(&ai);
        if (!aiImpl) {
            return std::unexpected("Failed to cast AI system to concrete implementation");
        }

        if (!aiImpl->initialize()) {
            return std::unexpected("Failed to initialize AI system");
        }
        logInfo("AI system resolved and initialized");
    }

    // Phase 11: Camera
    ICameraSystem* cameraSystem = nullptr;
    if (impl_->enableCamera) {
        if (!impl_->cameraViewportSize.has_value()) {
            return std::unexpected("Camera system requires viewport size");
        }
        // CameraSystem requires viewport size - manually emplace then retrieve service
        impl_->container.emplace<CameraSystemService>(impl_->cameraViewportSize.value());
        auto& camera = impl_->container.service<ICameraSystemService>();
        cameraSystem = &camera;
        logInfo("Camera system resolved from DI container");
    }

    // Phase 12: GAS (Gameplay Ability System - no dependencies)
    IGASSystem* gasSystem = nullptr;
    if (impl_->enableGAS) {
        auto& gas = impl_->container.service<GASSystemService>();
        gasSystem = &gas;
        
        // Cast to concrete implementation for initialization
        auto* gasImpl = dynamic_cast<GASSystem*>(&gas);
        if (!gasImpl) {
            return std::unexpected("Failed to cast GAS system to concrete implementation");
        }
        
        if (!gasImpl->initialize()) {
            return std::unexpected("Failed to initialize GAS system");
        }
        logInfo("GAS system resolved and initialized");
    }

    // Phase 13: Blueprints (depends on entities and optionally physics)
    // Note: BlueprintFactory is handled separately as it's not a regular system service yet
    if (impl_->enableBlueprints) {
        if (!impl_->enableEntities) {
            return std::unexpected("Blueprint system requires entity system");
        }
        // TODO: Implement BlueprintFactory as a proper service
        logWarn("Blueprint factory DI integration not yet implemented");
    }

    // Transfer ownership to Engine
    Engine engine;
    engine.impl_->jobs = std::move(impl_->jobs);
    engine.impl_->container = std::move(impl_->container);

    // Populate BestowEngine aggregate with interface pointers
    engine.impl_->engineAggregate.events = eventSystem;
    engine.impl_->engineAggregate.assets = assetSystem;
    engine.impl_->engineAggregate.entities = entitySystem;
    engine.impl_->engineAggregate.graphics = graphicsSystem;
    engine.impl_->engineAggregate.audio = audioSystem;
    engine.impl_->engineAggregate.input = inputSystem;
    engine.impl_->engineAggregate.physics = physicsSystem;
    engine.impl_->engineAggregate.levels = levelSystem;
    engine.impl_->engineAggregate.save = saveSystem;
    engine.impl_->engineAggregate.ai = aiSystem;
    engine.impl_->engineAggregate.camera = cameraSystem;
    engine.impl_->engineAggregate.gas = gasSystem;
    engine.impl_->engineAggregate.blueprints = nullptr; // TODO: Implement BlueprintFactory service

    // Wire system dependencies
    if (graphicsSystem && assetSystem) {
        // Cast to concrete type for wiring
        auto* graphicsImpl = dynamic_cast<OpenGLGraphicsSystem*>(graphicsSystem);
        if (graphicsImpl) {
            graphicsImpl->setAssetSystem(assetSystem);
            logInfo("Graphics-to-Assets wiring complete");
        }
    }
    
    if (physicsSystem && eventSystem) {
        // Cast to concrete type for wiring  
        auto* physicsImpl = dynamic_cast<Box2DPhysicsSystem*>(physicsSystem);
        auto* eventsImpl = dynamic_cast<EventSystem*>(eventSystem);
        if (physicsImpl && eventsImpl) {
            // Wire physics callbacks to events system
            physicsImpl->setCollisionCallback([eventsImpl](const CollisionEvent& collision) {
                eventsImpl->publish(Events::Collision, collision);
            });

            physicsImpl->setTriggerEnterCallback([eventsImpl](const TriggerEvent& trigger) {
                eventsImpl->publish(Events::TriggerEnter, trigger);
            });

            physicsImpl->setTriggerExitCallback([eventsImpl](const TriggerEvent& trigger) {
                eventsImpl->publish(Events::TriggerExit, trigger);
            });

            logInfo("Physics-to-Events wiring complete");
        }
    }
    
    if (assetSystem && eventSystem) {
        // Cast to concrete type for wiring
        auto* assetsImpl = dynamic_cast<AssetSystem*>(assetSystem);
        auto* eventsImpl = dynamic_cast<EventSystem*>(eventSystem);
        if (assetsImpl && eventsImpl) {
            assetsImpl->setEventSystem(eventsImpl);
            logInfo("AssetSystem-to-Events wiring complete");
        }
    }
    
    if (assetSystem && impl_->jobs) {
        // Cast to concrete type for wiring
        auto* assetsImpl = dynamic_cast<AssetSystem*>(assetSystem);
        if (assetsImpl) {
            assetsImpl->setJobSystem(impl_->jobs.get());
            logInfo("JobSystem-to-AssetSystem wiring complete");
        }
    }

    logInfo("Engine built successfully with DI container");
    return engine;
}

}  // namespace bestow::core
