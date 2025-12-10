// main.cpp
// Entity System Demo - Entry point

// MSVC C++23 module compatibility for EnTT iterators
#include <bestow/entt_compat.hpp>

import std;
import bestow;
import bestow.core;
import entity.demo;

int main() {
    try {
        std::println("Bestow Entity System Demo");
        std::println("=========================\n");

        // Create engine and get systems
        bestow::core::Engine engine;
        auto& sys = engine.systems();

        // Run the comprehensive demo
        demo::EntityDemo demo(*sys.entities);
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
