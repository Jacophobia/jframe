// games/game1/src/main.cpp
// 3D Isometric Snake Game - Entry Point
//
// This demonstrates the simple bestow::Game API.
// The entire game is just business logic - no infrastructure code needed!

import bestow.runtime;
import bestow.types;  // For Color type
import snake.game;

int main() {
    return bestow::run<snake::SnakeGame>({
        .title = "Snake 3D - Bestow Demo",
        .width = 1280,
        .height = 720,
        .vsync = true,
        .clearColor = bestow::Color{20, 25, 35, 255}  // Dark blue-gray
    });
}
