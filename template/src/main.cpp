// src/main.cpp
// Bestow Game Template - Entry Point with Engine
//
// This is where you wire up your game's systems using the Engine class.
// You CHOOSE which implementations to use for each contract interface.

import std;
import bestow.core;       // Engine class
import bestow.services;   // Contract interfaces

// Import Bestow's default implementations
import bestow.vulkan.impl;     // VulkanGraphics3DSystemService
import bestow.input.impl;      // InputSystemService
import bestow.entity.impl;     // EntitySystemService
import bestow.events.impl;     // EventSystemService
import bestow.audio.impl;      // AudioSystemService
import bestow.assets.impl;     // AssetSystemService
import bestow.config.impl;     // ConfigSystemService
import bestow.shader.impl;     // ShaderSystemService

// Import your game
import my.game;

int main() {
    // Create the Engine - the composition root
    bestow::core::Engine engine;

    //=========================================================================
    // Register System Implementations
    //
    // For each contract, register an implementation service.
    // You can use Bestow's implementations or your own.
    //=========================================================================

    // Core systems (no dependencies)
    engine.registerSystem<bestow::EventSystemService>();
    engine.registerSystem<bestow::EntitySystemService>();
    engine.registerSystem<bestow::ConfigSystemService>();

    // Asset system
    engine.registerSystem<bestow::AssetSystemService>();

    // Shader system (depends on assets, events)
    engine.registerSystem<bestow::ShaderSystemService>();

    // Graphics system (depends on assets, shaders, config)
    engine.registerSystem<bestow::vulkan::VulkanGraphics3DSystemService>();

    // Input system
    engine.registerSystem<bestow::InputSystemService>();

    // Audio system (optional)
    try {
        engine.registerSystem<bestow::AudioSystemService>();
    } catch (...) {
        std::cerr << "Warning: Audio system not available\n";
    }

    //=========================================================================
    // Run the Game
    //
    // Engine resolves the game and injects all dependencies automatically.
    //=========================================================================

    engine.run<mygame::MyGame>();

    return 0;
}
