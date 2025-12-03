// examples/endless-runner/src/main.cpp
// Endless runner demo showcasing procedural level generation

#include <string>

import jframe;
import jframe.core;

#if defined(JFRAME_DEV_TOOLS)
import jframe.dev;
#endif

#include "Game.h"

int main(int argc, char* argv[]) {
    jframe::core::logInfo("Starting JFrame Endless Runner");

    auto engineResult = jframe::core::EngineBuilder()
        .withEvents()
        .withEntities()
        .withPhysics()
        .withGraphics(jframe::core::GraphicsConfig{
            .width = 1280,
            .height = 720,
            .title = "JFrame Endless Runner - Procedural Generation Demo",
            .vsync = true,
            .clearColor = {135, 206, 235, 255}
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

    endless_runner::Game game;
    engineResult.value().run(game);

    jframe::core::logInfo("Endless Runner exiting");
    return 0;
}
