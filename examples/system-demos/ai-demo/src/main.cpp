// main.cpp
// AI System Demo - Entry point

// MSVC C++23 module compatibility for EnTT iterators
#include <bestow/entt_compat.hpp>

import std;
import bestow.types;
import bestow.ai;
import bestow.ai.impl;
import bestow.entity;
import bestow.entity.impl;
import bestow.physics;
import bestow.physics.impl;
import bestow.assets;
import bestow.assets.impl;
import bestow.events;
import bestow.events.impl;
import ai.demo;

int main() {
    try {
        std::println("Bestow AI System Demo");
        std::println("=====================\n");

        // Create required systems for AI
        std::println("Initializing systems...");

        auto eventSystem = std::make_unique<bestow::EventSystem>();
        std::println("  Event system initialized");

        auto entitySystem = std::make_unique<bestow::EntitySystem>();
        std::println("  Entity system initialized");

        auto assetSystem = std::make_unique<bestow::AssetSystem>();
        std::println("  Asset system initialized");

        auto physicsSystem = std::make_unique<bestow::Box2DPhysicsSystem>();
        physicsSystem->setGravity(bestow::Vec2{0.0f, -980.0f});
        std::println("  Physics system initialized");

        auto aiSystem = std::make_unique<bestow::AISystem>(
            physicsSystem.get(), assetSystem.get());
        std::println("  AI system initialized");

        std::println("\nAll systems ready!\n");

        // Run the comprehensive demo
        demo::AIDemo demo(*aiSystem, *entitySystem, *physicsSystem, *assetSystem);
        demo.run();

        std::println("\nDemo completed successfully!");
        return 0;

    } catch (const std::exception& e) {
        std::println("ERROR: {}", e.what());
        return 1;
    } catch (...) {
        std::println("ERROR: Unknown exception occurred");
        return 1;
    }
}
