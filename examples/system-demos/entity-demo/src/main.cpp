// main.cpp
// Entity System Demo - Entry point

import std;
import jframe.types;
import jframe.entity;
import jframe.entity.impl;
import entity.demo;

int main() {
    try {
        std::println("JFrame Entity System Demo");
        std::println("=========================\n");

        // Create the entity system
        auto entitySystem = jframe::createEntitySystem();

        if (!entitySystem) {
            std::println("ERROR: Failed to create entity system");
            return 1;
        }

        // Run the comprehensive demo
        demo::EntityDemo demo(*entitySystem);
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
