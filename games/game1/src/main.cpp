// games/game1/src/main.cpp
// 3D Isometric Snake Game - Entry Point
//
// Uses the Engine class for system registration and DI.

import std;
import bestow.core;       // Engine class
import bestow.services;   // Contract interfaces

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
    // Create the Engine
    bestow::core::Engine engine;

    // Register system implementations
    engine.registerSystem<bestow::ConfigSystemService>();
    engine.registerSystem<bestow::AssetSystemService>();
    engine.registerSystem<bestow::ShaderSystemService>();
    engine.registerSystem<bestow::vulkan::VulkanGraphics3DSystemService>();
    engine.registerSystem<bestow::InputSystemService>();

    // Audio (optional)
    try {
        engine.registerSystem<bestow::AudioSystemService>();
    } catch (...) {
        std::cerr << "Warning: Audio system not available\n";
    }

    // Run the game
    engine.run<snake::SnakeGame>();

    return 0;
}
