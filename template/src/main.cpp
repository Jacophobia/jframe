// src/main.cpp
// Bestow Game Template - Entry Point with Engine
//
// This is where you wire up your game's systems using the Engine class.
// You CHOOSE which implementations to use for each contract interface.

import std;
import bestow.core;       // Engine class
import bestow.services;   // Contract interfaces

// Import Bestow's default implementations
import bestow.vulkan.impl;     // VulkanGraphics3DSystem
import bestow.input.impl;      // InputSystem
import bestow.entity.impl;     // EntitySystem
import bestow.events.impl;     // EventSystem
import bestow.audio.impl;      // AudioSystem
import bestow.assets.impl;     // AssetSystem
import bestow.config.impl;     // ConfigSystem
import bestow.shader.impl;     // ShaderSystem (OpenGL shader system)

// Import your game
import my.game;

int main() {
    // Create the Engine - the composition root
    bestow::core::Engine engine;

    //=========================================================================
    // Register System Implementations
    //
    // Use engine.use<Contract, Implementation>() to bind implementations.
    // You can use Bestow's implementations or your own.
    //=========================================================================

    // Core systems (no dependencies)
    engine.use<bestow::IEventSystem, bestow::EventSystem>();
    engine.use<bestow::IEntitySystem, bestow::EntitySystem>();

    // Asset system (depends on events)
    engine.use<bestow::IAssetSystem, bestow::AssetSystem>();

    // Config system (depends on assets, events)
    engine.use<bestow::IConfigSystem, bestow::ConfigSystem>();

    // Shader system (depends on assets, events)
    engine.use<bestow::IShaderSystem, bestow::OpenGLShaderSystem>();

    // Graphics system (depends on assets, shaders, config)
    engine.use<bestow::IGraphics3DSystem, bestow::vulkan::VulkanGraphics3DSystem>();

    // Input system (depends on assets)
    engine.use<bestow::IInputSystem, bestow::InputSystem>();

    // Audio system (optional)
    try {
        engine.use<bestow::IAudioSystem, bestow::AudioSystem>();
    } catch (...) {
        std::cerr << "Warning: Audio system not available\n";
    }

    //=========================================================================
    // Run the Game
    //
    // Your game receives Engine& and uses engine.get<Contract>() to
    // retrieve the systems it needs.
    //=========================================================================

    engine.run<mygame::MyGame>();

    return 0;
}
