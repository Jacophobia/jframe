// jframe-core/src/Engine.cpp
// Engine implementation with fixed-timestep game loop

module;

// MSVC C++23 module compatibility - include full EnTT before import std
#include <jframe/entt_compat.hpp>

module jframe.core;

import jframe.events.impl;
import jframe.entity.impl;
import jframe.physics.impl;
import jframe.graphics.impl;
import jframe.audio.impl;
import jframe.input.impl;
import jframe.assets.impl;
import jframe.save.impl;
import jframe.level.impl;
import jframe.ai.impl;
import jframe.camera.impl;
import jframe.gas.impl;
import jframe.blueprints.impl;

namespace jframe::core {

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
    JFrameEngine engineAggregate;

    // Game loop state
    bool running = false;
    Timer runTimer;
};

//==============================================================================
// Engine
//==============================================================================

Engine::Engine() : impl_(std::make_unique<Impl>()) {}

Engine::~Engine() {
    // Systems are destroyed in reverse order of construction
    // This is handled automatically by the unique_ptr members
    logInfo("Engine shutting down");
}

Engine::Engine(Engine&&) noexcept = default;
Engine& Engine::operator=(Engine&&) noexcept = default;

JFrameEngine& Engine::systems() {
    return impl_->engineAggregate;
}

const JFrameEngine& Engine::systems() const {
    return impl_->engineAggregate;
}

void Engine::run(Application& app) {
    // Initialize the application
    if (!app.initialize(*this)) {
        logError("Application initialization failed");
        return;
    }

    logInfo("Starting game loop");
    impl_->running = true;
    impl_->runTimer.reset();

    // Fixed timestep configuration (Gaffer on Games pattern)
    constexpr float FIXED_DT = 1.0f / 60.0f;  // 60 Hz physics/logic
    constexpr float MAX_FRAME_TIME = 0.25f;    // Prevent spiral of death

    Timer frameTimer;
    float accumulator = 0.0f;

    while (impl_->running) {
        // Measure frame time
        float frameTime = frameTimer.elapsedSeconds();
        frameTimer.reset();

        // Clamp frame time to prevent spiral of death
        // (if frame takes too long, don't try to catch up indefinitely)
        if (frameTime > MAX_FRAME_TIME) {
            frameTime = MAX_FRAME_TIME;
        }

        accumulator += frameTime;

        // Process input events (once per frame)
        if (impl_->input) {
            impl_->input->update();
        }

        // Check if window should close
        if (impl_->graphics && impl_->graphics->shouldClose()) {
            impl_->running = false;
            break;
        }

        // Fixed timestep updates (deterministic simulation)
        while (accumulator >= FIXED_DT) {
            // Update physics
            if (impl_->physics) {
                impl_->physics->update(FIXED_DT);
            }

            // Update game logic
            app.updateFixed(FIXED_DT);

            // Process queued events
            if (impl_->events) {
                impl_->events->processQueue();
            }

            accumulator -= FIXED_DT;
        }

        // Render with interpolation alpha
        // alpha = how far between current and next physics state (0.0 - 1.0)
        float alpha = accumulator / FIXED_DT;

        if (impl_->graphics) {
            impl_->graphics->beginFrame();
            app.render(alpha);
            impl_->graphics->endFrame();
        } else {
            // No graphics system, just render without frame management
            app.render(alpha);
        }
    }

    // Cleanup
    app.shutdown();
    logInfo("Game loop ended");
}

void Engine::quit() {
    impl_->running = false;
}

bool Engine::isRunning() const {
    return impl_->running;
}

}  // namespace jframe::core
