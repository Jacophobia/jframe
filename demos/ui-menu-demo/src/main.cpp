// demos/ui-menu-demo/src/main.cpp
// UI Menu Demo - Entry Point
//
// Demonstrates the Vulkan UI render backend with a clickable menu system.
// Uses RmlUi through the IUISystem interface.

import std;
import bestow.core;       // Engine class
import bestow.services;   // Contract interfaces
import bestow.types;      // PathResolver

// Import Bestow's default implementations
import bestow.vulkan.impl;     // VulkanGraphics3DSystem
import bestow.input.impl;      // InputSystem
import bestow.events.impl;     // EventSystem
import bestow.assets.impl;     // AssetSystem
import bestow.config.impl;     // ConfigSystem

// Import the demo application
import ui.menu.demo;

int main() {
    // Initialize PathResolver with library path
    auto cwd = std::filesystem::current_path();
    bestow::PathResolver::initialize();

    // Look for library assets in order of preference
    std::filesystem::path libraryPath;
    if (std::filesystem::exists(cwd / "library")) {
        libraryPath = cwd / "library";
    } else if (std::filesystem::exists(cwd / "asset-library")) {
        libraryPath = cwd / "asset-library";
    } else if (std::filesystem::exists(cwd.parent_path().parent_path().parent_path() / "asset-library")) {
        libraryPath = cwd.parent_path().parent_path().parent_path() / "asset-library";
    } else {
        std::cerr << "Warning: Could not find library directory from cwd: " << cwd << "\n";
        libraryPath = cwd / "library";
    }
    std::cerr << "Using library path: " << libraryPath << "\n";
    bestow::PathResolver::setLibraryPath(libraryPath.string());

    // Create the Engine - the composition root
    bestow::core::Engine engine;

    // Register system implementations
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

    // Run the demo
    engine.run<demo::UIMenuDemo>();

    return 0;
}
