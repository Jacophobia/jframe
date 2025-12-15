// games/game1/src/main.cpp
// 3D Isometric Snake Game - Entry Point
//
// Demonstrates the Bestow contract-based DI architecture.
// You explicitly register which implementations to use for each contract.

#include <kangaru/kangaru.hpp>

import std;
import bestow.services;  // Contract interfaces

// Import Bestow's default implementations
import bestow.vulkan.impl;     // VulkanGraphics3DSystemService
import bestow.input.impl;      // InputSystemService
import bestow.audio.impl;      // AudioSystemService
import bestow.assets.impl;     // AssetSystemService
import bestow.config.impl;     // ConfigSystemService
import bestow.shader.impl;     // ShaderSystemService

// Import the snake game
import snake.game;

int main() {
    // Create Kangaru DI container
    kgr::container container;

    //=========================================================================
    // Register System Implementations
    //
    // For each contract interface, register an implementation.
    // Order matters - systems with dependencies must be registered after
    // their dependencies.
    //=========================================================================

    // Core systems (no dependencies)
    container.service<bestow::ConfigSystemService>();

    // Asset system
    container.service<bestow::AssetSystemService>();

    // Shader system (depends on assets)
    container.service<bestow::ShaderSystemService>();

    // Graphics system (depends on assets, shaders, config)
    container.service<bestow::vulkan::VulkanGraphics3DSystemService>();

    // Input system
    container.service<bestow::InputSystemService>();

    // Audio system (optional - may fail if FMOD not available)
    try {
        container.service<bestow::AudioSystemService>();
    } catch (...) {
        std::cerr << "Warning: Audio system not available\n";
    }

    //=========================================================================
    // Run the Game
    //
    // Resolve the game from the container. Kangaru injects all dependencies.
    //=========================================================================

    auto& game = container.service<snake::SnakeGameService>();
    game.run();

    return 0;
}
