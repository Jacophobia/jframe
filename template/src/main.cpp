// Bestow Game Template - main.cpp
// Minimal entry point for a 2D game

#include <kangaru/kangaru.hpp>
#include <bestow/entt_compat.hpp>
#include <GLFW/glfw3.h>

import std;
import bestow.types;
import bestow.entity.impl;
import bestow.physics.impl;
import bestow.opengl.impl;
import bestow.input.impl;
import bestow.assets.impl;
import bestow.blueprints.impl;

#include "Game.h"

int main() {
    // Create DI container and systems
    kgr::container container;

    auto& entities = container.service<bestow::EntitySystemService>();
    auto& physics = container.service<bestow::PhysicsSystemService>();
    auto& graphics = container.service<bestow::GraphicsSystemService>();
    auto& input = container.service<bestow::InputSystemService>();
    auto& assets = container.service<bestow::AssetSystemService>();

    // Initialize graphics (creates window internally)
    auto* graphicsImpl = dynamic_cast<bestow::OpenGLGraphicsSystem*>(&graphics);
    if (!graphicsImpl || !graphicsImpl->initialize(800, 600, "Bestow Game")) {
        std::println(stderr, "Failed to initialize graphics");
        return 1;
    }

    // Get the window from graphics and pass to input
    auto* window = static_cast<GLFWwindow*>(graphics.getNativeWindowHandle());

    auto* inputImpl = dynamic_cast<bestow::InputSystem*>(&input);
    if (inputImpl) inputImpl->initialize(window);

    auto* physicsImpl = dynamic_cast<bestow::Box2DPhysicsSystem*>(&physics);
    if (physicsImpl) physicsImpl->initialize();

    // Create blueprint factory
    bestow::BlueprintFactory blueprints(entities, &physics);

    // Create and initialize game
    Game game(entities, physics, graphics, input, blueprints);
    game.init();

    // Game loop
    constexpr float fixedDt = 1.0f / 60.0f;
    float accumulator = 0.0f;
    auto lastTime = std::chrono::high_resolution_clock::now();

    while (!glfwWindowShouldClose(window)) {
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;
        accumulator += dt;

        // Note: glfwPollEvents is called by graphics.endFrame()
        input.update();

        // Fixed timestep updates
        while (accumulator >= fixedDt) {
            physics.update(fixedDt);
            game.update(fixedDt);
            accumulator -= fixedDt;
        }

        // Render (endFrame handles buffer swap and event polling)
        graphics.beginFrame();
        game.render();
        graphics.endFrame();
    }

    game.shutdown();
    // GraphicsSystem destructor handles cleanup
    return 0;
}
