// games/game1/src/main.cpp
// 3D Isometric Snake Game - Entry Point
//
// Uses the Application<> base class pattern for automatic dependency injection.

import std;
import bestow.core;       // Engine class
import bestow.services;   // Contract interfaces
import bestow.types;      // PathResolver

// Import Bestow's default implementations
import bestow.vulkan.impl;     // VulkanGraphics3DSystem
import bestow.input.impl;      // InputSystem
import bestow.events.impl;     // EventSystem
import bestow.audio.impl;      // AudioSystem
import bestow.assets.impl;     // AssetSystem
import bestow.config.impl;     // ConfigSystem

// Import the snake game
import snake.game;

int main(int argc, char* argv[]) {
    // Change working directory to executable directory so shaders can be found
    // Shaders are copied to the output directory by CMake post-build step
    auto exeDir = std::filesystem::path(argv[0]).parent_path();
    if (!exeDir.empty() && std::filesystem::exists(exeDir)) {
        std::filesystem::current_path(exeDir);
    }

    // Initialize PathResolver with the executable path
    bestow::PathResolver::initialize(argv[0]);

    // Create the Engine - the composition root
    bestow::core::Engine engine;

    // Register system implementations with engine.use<Contract, Implementation>()
    engine.use<bestow::IEventSystem, bestow::EventSystem>();
    engine.use<bestow::IAssetSystem, bestow::AssetSystem>(
        [](bestow::di::ServiceProvider& sp) { return new bestow::AssetSystem(sp.get<bestow::IEventSystem>()); });
    engine.use<bestow::IConfigSystem, bestow::ConfigSystem>(
        [](bestow::di::ServiceProvider& sp) {
            return new bestow::ConfigSystem(sp.get<bestow::IAssetSystem>(), sp.get<bestow::IEventSystem>());
        });
    engine.use<bestow::IGraphics3DSystem, bestow::VulkanGraphics3DSystem>(
        [](bestow::di::ServiceProvider& sp) {
            return new bestow::VulkanGraphics3DSystem(sp.get<bestow::IAssetSystem>(), sp.get<bestow::IConfigSystem>());
        });
    engine.use<bestow::IInputSystem, bestow::InputSystem>(
        [](bestow::di::ServiceProvider& sp) {
            return new bestow::InputSystem(sp.get<bestow::IEventSystem>(), sp.get<bestow::IAssetSystem>());
        });

    // Audio (optional)
    try {
        engine.use<bestow::IAudioSystem, bestow::AudioSystem>(
            [](bestow::di::ServiceProvider& sp) { return new bestow::AudioSystem(sp.get<bestow::IAssetSystem>()); });
    } catch (...) {
        std::cerr << "Warning: Audio system not available\n";
    }

    // Run the game - dependencies auto-detected from Application<> base
    engine.run<snake::SnakeGame>();

    return 0;
}
