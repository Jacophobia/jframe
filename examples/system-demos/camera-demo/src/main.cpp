// main.cpp
// Camera System Demo - Entry point

// MSVC C++23 module compatibility for EnTT iterators
#include <bestow/entt_compat.hpp>

import std;
import bestow;
import bestow.core;
import camera.demo;

int main() {
    try {
        std::println("Bestow Camera System Demo");
        std::println("=========================\n");

        // Create engine and get systems
        bestow::core::Engine engine;
        auto& sys = engine.systems();

        std::println("Got camera and entity systems from engine");

        std::println("Systems created successfully!\n");

        // Run the comprehensive demo
        demo::CameraDemo cameraDemo(*sys.camera, *sys.entities);
        cameraDemo.run();

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
