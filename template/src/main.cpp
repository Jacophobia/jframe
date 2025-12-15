// src/main.cpp
// Bestow Game Template - Entry Point
//
// This is the simplest possible Bestow game entry point.
// Just import your game module and call bestow::run<YourGame>()!

import bestow.runtime;
import bestow.types;
import my.game;

int main() {
    return bestow::run<MyGame>({
        .title = "My Bestow Game",
        .width = 1280,
        .height = 720,
        .vsync = true,
        .clearColor = bestow::Color{30, 30, 50, 255}
    });
}
