// main.cpp
// AI System Demo - Entry point

#include <exception>
#include <print>

import jframe.types;
import jframe.ai;
import jframe.ai.impl;
import jframe.entity;
import jframe.entity.impl;
import jframe.physics;
import jframe.physics.impl;
import jframe.assets;
import jframe.assets.impl;
import jframe.events;
import jframe.events.impl;
import ai.demo;

int main() {
    try {
        std::println("JFrame AI System Demo");
        std::println("=====================\n");

        // Create required systems for AI
        std::println("Initializing systems...");

        auto eventSystem = jframe::createEventSystem();
        if (!eventSystem) {
            std::println("ERROR: Failed to create event system");
            return 1;
        }
        std::println("  Event system initialized");

        auto entitySystem = jframe::createEntitySystem();
        if (!entitySystem) {
            std::println("ERROR: Failed to create entity system");
            return 1;
        }
        std::println("  Entity system initialized");

        auto assetSystem = jframe::createAssetSystem();
        if (!assetSystem) {
            std::println("ERROR: Failed to create asset system");
            return 1;
        }
        std::println("  Asset system initialized");

        auto physicsSystem = jframe::createPhysicsSystem();
        if (!physicsSystem) {
            std::println("ERROR: Failed to create physics system");
            return 1;
        }

        physicsSystem->setGravity(jframe::Vec2{0.0f, -980.0f});
        std::println("  Physics system initialized");

        auto aiSystem = jframe::createAISystem(
            physicsSystem.get(), assetSystem.get());
        if (!aiSystem) {
            std::println("ERROR: Failed to create AI system");
            return 1;
        }
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
