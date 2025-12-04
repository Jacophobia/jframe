// examples/ability-demo/src/main.cpp
// Gameplay Ability System graphical demonstration

// MSVC C++23 module compatibility for EnTT iterators and sol2 globals
#include <jframe/entt_compat.hpp>
#include <jframe/sol2_compat.hpp>

import std;
import jframe;
import jframe.core;

#include "Game.h"

int main(int argc, char* argv[]) {
    jframe::core::logInfo("Starting JFrame GAS Demo");

    // Build the engine with all required systems
    // Note: GAS system is created separately by the Game class
    auto engineResult = jframe::core::EngineBuilder()
        .withEvents()
        .withEntities()
        .withPhysics()
        .withGraphics(jframe::core::GraphicsConfig{
            .width = 800,
            .height = 600,
            .title = "JFrame - Gameplay Ability System Demo",
            .vsync = true,
            .clearColor = {20, 20, 30, 255}  // Dark blue background
        })
        .withInput()
        .withAssets("data")
        .withLevel()  // Enable level system for loading Lua level definitions
        .build();

    if (!engineResult) {
        jframe::core::logError("Failed to build engine: " + engineResult.error());
        return 1;
    }

    jframe::core::logInfo("Engine built successfully");

    // Create the game and run it
    abilitydemo::Game game;
    engineResult.value().run(game);

    jframe::core::logInfo("GAS Demo exiting");
    return 0;
}
