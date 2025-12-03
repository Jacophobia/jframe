// main.cpp
// Camera System Demo - Entry point

#include <exception>
#include <print>

import jframe.types;
import jframe.camera;
import jframe.camera.impl;
import jframe.entity;
import jframe.entity.impl;
import camera.demo;

int main() {
    try {
        std::println("JFrame Camera System Demo");
        std::println("=========================\n");

        // Create viewport size for camera
        jframe::Size viewport{800, 600};
        std::println("Creating camera system with viewport: {}x{}", viewport.width, viewport.height);

        // Create the camera system
        auto cameraSystem = jframe::createCameraSystem(viewport);
        if (!cameraSystem) {
            std::println("ERROR: Failed to create camera system");
            return 1;
        }

        // Create the entity system (needed for target entities)
        auto entitySystem = jframe::createEntitySystem();
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
