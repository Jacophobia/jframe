// main.cpp
// Entity System Demo - Entry point

// MSVC C++23 module compatibility for EnTT iterators
#include <bestow/entt_compat.hpp>

import std;
import bestow.types;
import bestow.entity;
import bestow.entity.impl;
import entity.demo;

int main() {
    try {
        std::println("Bestow Entity System Demo");
        std::println("=========================\n");

        // Create the entity system
        auto entitySystem = std::make_unique<bestow::EntitySystem>();

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
