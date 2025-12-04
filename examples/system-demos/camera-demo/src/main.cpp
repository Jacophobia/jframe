// main.cpp
// Camera System Demo - Entry point

// MSVC C++23 module compatibility for EnTT iterators
#include <bestow/entt_compat.hpp>

import std;
import bestow.types;
import bestow.camera;
import bestow.camera.impl;
import bestow.entity;
import bestow.entity.impl;
import camera.demo;

int main() {
    try {
        std::println("Bestow Camera System Demo");
        std::println("=========================\n");

        // Create viewport size for camera
        bestow::Size viewport{800, 600};
        std::println("Creating camera system with viewport: {}x{}", viewport.width, viewport.height);

        // Create the camera system
        auto cameraSystem = bestow::createCameraSystem(viewport);
        if (!cameraSystem) {
            std::println("ERROR: Failed to create camera system");
            return 1;
        }

        // Create the entity system (needed for target entities)
        auto entitySystem = bestow::createEntitySystem();
        if (!entitySystem) {
            std::println("ERROR: Failed to create entity system");
            return 1;
        }

        std::println("Systems created successfully!\n");

        // Run the comprehensive demo
        demo::CameraDemo cameraDemo(*cameraSystem, *entitySystem);
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
