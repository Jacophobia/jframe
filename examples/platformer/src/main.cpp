// examples/platformer/src/main.cpp
// Platformer example entry point

// MSVC C++23 module compatibility for EnTT iterators and sol2 globals
#include <jframe/entt_compat.hpp>
#include <jframe/sol2_compat.hpp>

import std;
import jframe;
import jframe.core;

#if defined(JFRAME_DEV_TOOLS)
import jframe.dev;
#endif

#include "Game.h"

int main(int argc, char* argv[]) {
    jframe::core::logInfo("Starting JFrame Platformer Example");

    // Build the engine with all required systems
    auto engineResult = jframe::core::EngineBuilder()
        .withEvents()
        .withEntities()
        .withPhysics()
        .withGraphics(jframe::core::GraphicsConfig{
            .width = 800,
            .height = 600,
            .title = "JFrame Platformer",
            .vsync = true,
            .clearColor = {135, 206, 235, 255}  // Sky blue
        })
        .withInput()
        .withAssets("data")
        .withAudio()
        .withLevel()
        .withSave("saves")
        .withAI()
        .build();

    if (!engineResult) {
        jframe::core::logError("Failed to build engine: " + engineResult.error());
        return 1;
    }

    jframe::core::logInfo("Engine built successfully");

    // Create the game and run it
    platformer::Game game;
    engineResult.value().run(game);

    jframe::core::logInfo("Platformer exiting");
    return 0;
}
