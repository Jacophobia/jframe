// examples/ability-demo/src/main.cpp
// Gameplay Ability System graphical demonstration

// MSVC C++23 module compatibility for EnTT iterators and sol2 globals
#include <bestow/entt_compat.hpp>
#include <bestow/sol2_compat.hpp>

import std;
import bestow;
import bestow.core;

#include "Game.h"

int main(int argc, char* argv[]) {
    bestow::core::logInfo("Starting Bestow GAS Demo");

    // Build the engine with all required systems
    // Note: GAS system is created separately by the Game class
    auto engineResult = bestow::core::EngineBuilder()
        .withEvents()
        .withEntities()
        .withPhysics()
        .withGraphics(bestow::core::GraphicsConfig{
            .width = 800,
            .height = 600,
            .title = "Bestow - Gameplay Ability System Demo",
            .vsync = true,
            .clearColor = {20, 20, 30, 255}  // Dark blue background
        })
        .withInput()
        .withAssets("data")
        .withLevel()  // Enable level system for loading Lua level definitions
        .build();

    if (!engineResult) {
        bestow::core::logError("Failed to build engine: " + engineResult.error());
        return 1;
    }

    bestow::core::logInfo("Engine built successfully");

    // Create the game and run it
    abilitydemo::Game game;
    engineResult.value().run(game);

    bestow::core::logInfo("GAS Demo exiting");
    return 0;
}
