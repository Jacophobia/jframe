// main.cpp
// AI System Demo - Entry point

// MSVC C++23 module compatibility for EnTT iterators
#include <bestow/entt_compat.hpp>

import std;
import bestow;
import bestow.core;
import ai.demo;

int main() {
    try {
        std::println("Bestow AI System Demo");
        std::println("=====================\n");

        // Create engine and get required systems for AI
        std::println("Initializing engine and systems...");
        bestow::core::Engine engine;
        auto& sys = engine.systems();

        std::println("  Event system initialized");
        std::println("  Entity system initialized");
        std::println("  Asset system initialized");

        sys.physics->setGravity(bestow::Vec2{0.0f, -980.0f});
        std::println("  Physics system initialized");

        std::println("  AI system initialized");

        std::println("\nAll systems ready!\n");

        // Run the comprehensive demo
        demo::AIDemo demo(*sys.ai, *sys.entities, *sys.physics, *sys.assets);
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
