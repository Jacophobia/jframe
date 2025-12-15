// games/game1/src/main.cpp
// 3D Isometric Snake Game - Entry Point
//
// Uses .NET-style constructor injection with engine.run<App, Deps...>().

import std;
import bestow.core;       // Engine class
import bestow.services;   // Contract interfaces

// Import Bestow's default implementations
import bestow.vulkan.impl;     // VulkanGraphics3DSystem
import bestow.input.impl;      // InputSystem
import bestow.events.impl;     // EventSystem
import bestow.audio.impl;      // AudioSystem
import bestow.assets.impl;     // AssetSystem
import bestow.config.impl;     // ConfigSystem
import bestow.shader.impl;     // OpenGLShaderSystem

// Import the snake game
import snake.game;

int main() {
    // Create the Engine - the composition root
    bestow::core::Engine engine;

    // Register system implementations with engine.use<Contract, Implementation>()
    engine.use<bestow::IEventSystem, bestow::EventSystem>();
    engine.use<bestow::IAssetSystem, bestow::AssetSystem>();
    engine.use<bestow::IConfigSystem, bestow::ConfigSystem>();
    engine.use<bestow::IShaderSystem, bestow::OpenGLShaderSystem>();
    engine.use<bestow::IGraphics3DSystem, bestow::VulkanGraphics3DSystem>();
    engine.use<bestow::IInputSystem, bestow::InputSystem>();

    // Audio (optional)
    try {
        engine.use<bestow::IAudioSystem, bestow::AudioSystem>();
    } catch (...) {
        std::cerr << "Warning: Audio system not available\n";
    }

    // Run the game with constructor injection
    engine.run<snake::SnakeGame,
        bestow::IGraphics3DSystem,
        bestow::IInputSystem,
        bestow::IAudioSystem>();

    return 0;
}
