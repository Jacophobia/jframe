// main.cpp
// Physics System Demo - Entry point

import std;
import bestow.types;
import bestow.physics;
import bestow.physics.impl;
import physics.demo;

int main() {
    try {
        std::println("Bestow Physics System Demo");
        std::println("==========================\n");

        // Create the physics system
        auto physicsSystem = std::make_unique<bestow::Box2DPhysicsSystem>();

        // Run the comprehensive demo
        demo::PhysicsDemo demo(*physicsSystem);
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
