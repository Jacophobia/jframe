// src/main.cpp
// Bestow Game Template - Entry Point with Explicit System Registration
//
// This is where you wire up your game's systems.
// You CHOOSE which implementations to use for each contract interface.
//
// Benefits:
// - You understand exactly what systems your game uses
// - Easy to swap implementations (e.g., Vulkan vs OpenGL)
// - Easy to create custom implementations that conform to contracts
// - Easy to mock systems for testing

// Include Kangaru for DI container
#include <kangaru/kangaru.hpp>

import std;
import bestow.services;  // Contract interfaces

// Import Bestow's default implementations
// You can replace any of these with your own implementations!
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

//=============================================================================
// Example: Custom Entity System
//
// Uncomment this to use a custom entity system instead of Bestow's default.
// Your implementation must conform to IEntitySystem contract.
//=============================================================================
// class MyCustomEntitySystem : public bestow::IEntitySystem {
// public:
//     // Implement all IEntitySystem methods...
//     bestow::Entity createEntity() override { ... }
//     void destroyEntity(bestow::Entity e) override { ... }
//     // etc.
// };
//
// struct MyCustomEntitySystemService
//     : kgr::single_service<MyCustomEntitySystem>
//     , kgr::overrides<bestow::IEntitySystemService> {};

int main() {
    // Create Kangaru DI container
    kgr::container container;

    //=========================================================================
    // Step 1: Register System Implementations
    //
    // For each contract interface, register an implementation.
    // You can use Bestow's implementations or your own.
    //
    // Order matters! Systems with dependencies must be registered after
    // their dependencies. The service definitions handle the wiring.
    //=========================================================================

    // Core systems (no dependencies)
    container.service<bestow::EventSystemService>();
    container.service<bestow::EntitySystemService>();
    container.service<bestow::ConfigSystemService>();

    // Asset system
    container.service<bestow::AssetSystemService>();

    // Shader system (depends on assets, events)
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
    // Step 2: Run the Game
    //
    // Resolve the game application from the container.
    // Kangaru injects all dependencies specified in MyGameService::construct()
    //=========================================================================

    auto& game = container.service<mygame::MyGameService>();
    game.run();

    return 0;
}

//=============================================================================
// Alternative: Using Custom Implementations
//
// To use a custom implementation instead of Bestow's default:
//
// 1. Create your implementation class conforming to the contract interface
// 2. Create a Kangaru service struct that overrides the abstract service
// 3. Register your service instead of Bestow's
//
// Example - using a custom entity system:
//
//     // Instead of:
//     container.service<bestow::EntitySystemService>();
//
//     // Use:
//     container.service<MyCustomEntitySystemService>();
//
// The rest of your code remains unchanged because everything uses
// the contract interfaces (IEntitySystem, etc.), not implementations.
//=============================================================================

//=============================================================================
// Testing with Mocks
//
// For unit tests, create mock implementations and register those:
//
//     class MockEntitySystem : public bestow::IEntitySystem {
//         // Mock implementation for testing
//     };
//
//     struct MockEntitySystemService
//         : kgr::single_service<MockEntitySystem>
//         , kgr::overrides<bestow::IEntitySystemService> {};
//
//     // In your test:
//     kgr::container testContainer;
//     testContainer.service<MockEntitySystemService>();
//     testContainer.service<mygame::MyGameService>();
//     // Now MyGame receives the mock instead of real EntitySystem
//=============================================================================
