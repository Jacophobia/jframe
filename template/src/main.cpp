// template/src/main.cpp
// JFrame Template Game - Entry Point
// This file shows how to build a JFrame engine with all systems enabled

import std;
import jframe;
import jframe.core;

#include "Game.h"

int main(int argc, char* argv[]) {
    jframe::core::logInfo("Starting JFrame Template Game");

    // Build the engine with all required systems
    // The EngineBuilder provides a fluent API for configuring systems
    auto engineResult = jframe::core::EngineBuilder()
        // Event System - Pub/sub messaging between systems
        .withEvents()

        // Entity System - EnTT-based ECS for game objects
        .withEntities()

        // Physics System - Box2D integration for 2D physics
        .withPhysics()

        // Graphics System - OpenGL rendering with debug primitives
        .withGraphics(jframe::core::GraphicsConfig{
            .width = 800,
            .height = 600,
            .title = "JFrame Template Game",
            .vsync = true,
            .clearColor = {40, 40, 50, 255}  // Dark blue-gray background
        })

        // Input System - Keyboard, mouse, and gamepad input
        .withInput()

        // Asset System - Loading and managing game assets
        .withAssets("data")

        // Audio System - FMOD-based audio playback
        .withAudio()

        // Level System - Lua-based level loading
        .withLevel()

        // Build the engine (returns std::expected<Engine, std::string>)
        .build();

    // Check if engine build succeeded
    if (!engineResult) {
        jframe::core::logError("Failed to build engine: " + engineResult.error());
        return 1;
    }

    jframe::core::logInfo("Engine built successfully");

    // Create the game application and run the game loop
    // The engine will call initialize(), updateFixed(), render(), and shutdown()
    template_game::Game game;
    engineResult.value().run(game);

    jframe::core::logInfo("Template Game exiting");
    return 0;
}
