// demos/animation-showcase/src/main.cpp
// Animation System Showcase - Entry Point
//
// Demonstrates loading FBX models and playing skeletal animations.
// Uses Vulkan renderer with the Application<> pattern for dependency injection.

import std;
import bestow.core;       // Engine class
import bestow.services;   // Contract interfaces
import bestow.types;      // PathResolver

// Import Bestow's default implementations
import bestow.vulkan.impl;     // VulkanGraphics3DSystem
import bestow.input.impl;      // InputSystem
import bestow.events.impl;     // EventSystem
import bestow.assets.impl;     // AssetSystem
import bestow.lua.impl;        // LuaRuntime (replaces ConfigSystem)
import bestow.animation.impl;  // AnimationSystem

// Import the showcase application
import animation.showcase;

int main() {
    // Initialize PathResolver with library path pointing to asset-library
    auto cwd = std::filesystem::current_path();
    bestow::PathResolver::initialize();

    // Check if asset-library exists relative to current directory (running from project root)
    // or if we're in build directory (need to go up)
    std::filesystem::path libraryPath;
    if (std::filesystem::exists(cwd / "asset-library")) {
        libraryPath = cwd / "asset-library";
    } else if (std::filesystem::exists(cwd.parent_path().parent_path() / "asset-library")) {
        libraryPath = cwd.parent_path().parent_path() / "asset-library";
    } else {
        std::cerr << "Warning: Could not find asset-library directory from cwd: " << cwd << "\n";
        libraryPath = cwd / "asset-library";  // Fallback
    }
    std::cerr << "Using library path: " << libraryPath << "\n";
    bestow::PathResolver::setLibraryPath(libraryPath.string());

    // Create the Engine - the composition root
    bestow::core::Engine engine;

    // Register system implementations
    engine.use<bestow::IEventSystem, bestow::EventSystem>();
    engine.use<bestow::IAssetSystem, bestow::AssetSystem>();
    engine.use<bestow::ILuaRuntime, bestow::LuaRuntime>();
    engine.use<bestow::IGraphics3DSystem, bestow::VulkanGraphics3DSystem>();
    engine.use<bestow::IInputSystem, bestow::InputSystem>();
    engine.use<bestow::IAnimationSystem, bestow::AnimationSystem>();

    // Run the showcase
    engine.run<showcase::AnimationShowcase>();

    return 0;
}
