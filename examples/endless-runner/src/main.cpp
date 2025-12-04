// examples/endless-runner/src/main.cpp
// Endless runner demo showcasing procedural level generation

// MSVC C++23 module compatibility for EnTT iterators and sol2 globals
#include <bestow/entt_compat.hpp>
#include <bestow/sol2_compat.hpp>

import std;
import bestow;
import bestow.core;

#if defined(BESTOW_DEV_TOOLS)
import bestow.dev;
#endif

#include "Game.h"

int main(int argc, char* argv[]) {
    bestow::core::logInfo("Starting Bestow Endless Runner");

    auto engineResult = bestow::core::EngineBuilder()
        .withEvents()
        .withEntities()
        .withPhysics()
        .withGraphics(bestow::core::GraphicsConfig{
            .width = 1280,
            .height = 720,
            .title = "Bestow Endless Runner - Procedural Generation Demo",
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
        bestow::core::logError("Failed to build engine: " + engineResult.error());
        return 1;
    }

    bestow::core::logInfo("Engine built successfully");

    endless_runner::Game game;
    engineResult.value().run(game);

    bestow::core::logInfo("Endless Runner exiting");
    return 0;
}
