// examples/platformer-demo/src/main.cpp
// Platformer demo showcasing all JFrame engine features

import std;
import jframe;
import jframe.core;

#if defined(JFRAME_DEV_TOOLS)
import jframe.dev;
#endif

#include "Game.h"

int main(int argc, char* argv[]) {
    jframe::core::logInfo("Starting JFrame Platformer Demo");

    // Build the engine with all required systems
    auto engineResult = jframe::core::EngineBuilder()
        .withEvents()
        .withEntities()
        .withPhysics()
        .withGraphics(jframe::core::GraphicsConfig{
            .width = 1280,
            .height = 720,
            .title = "JFrame Platformer Demo - Sprites, Animations, Physics, Camera",
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
    platformer_demo::Game game;
    engineResult.value().run(game);

    jframe::core::logInfo("Platformer Demo exiting");
    return 0;
}
