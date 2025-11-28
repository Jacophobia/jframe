# Tutorial 1: Your First Game

Welcome to JFrame! This tutorial will guide you through creating your first game from scratch. You'll learn the basic structure of a JFrame game, how to build and run it, and make simple modifications.

## Prerequisites

Before starting, make sure you have:

- LLVM Clang 20+ installed (see `docs/LLVM20-SETUP.md`)
- CMake 3.28+
- All dependencies installed via vcpkg

## Step 1: Create Your Game Project

JFrame games follow a simple structure. Let's create a new game called "MyFirstGame".

```bash
cd examples
mkdir my-first-game
cd my-first-game
```

Create the following directory structure:

```
my-first-game/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── Game.h
│   └── Game.cpp
└── data/
    └── levels/
        └── level1.lua
```

## Step 2: Create CMakeLists.txt

Create `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.28)
project(my-first-game CXX)

add_executable(my-first-game
    src/main.cpp
    src/Game.cpp
)

target_link_libraries(my-first-game PRIVATE
    jframe-contract
    jframe-core
)

target_compile_features(my-first-game PRIVATE cxx_std_23)

# Copy data directory to build output
add_custom_command(TARGET my-first-game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/data
    $<TARGET_FILE_DIR:my-first-game>/data
)
```

## Step 3: Create main.cpp

Create `src/main.cpp`:

```cpp
// src/main.cpp
import std;
import jframe;
import jframe.core;

#include "Game.h"

int main(int argc, char* argv[]) {
    jframe::core::logInfo("Starting My First Game!");

    // Build the engine
    auto engineResult = jframe::core::EngineBuilder()
        .withEvents()
        .withEntities()
        .withPhysics()
        .withGraphics(jframe::core::GraphicsConfig{
            .width = 800,
            .height = 600,
            .title = "My First Game",
            .vsync = true,
            .clearColor = {100, 149, 237, 255}  // Cornflower blue
        })
        .withInput()
        .build();

    if (!engineResult) {
        jframe::core::logError("Failed to build engine: " + engineResult.error());
        return 1;
    }

    // Run the game
    MyFirstGame game;
    engineResult.value().run(game);

    return 0;
}
```

## Step 4: Create Game.h

Create `src/Game.h`:

```cpp
// src/Game.h
#pragma once

import jframe;
import jframe.core;

class MyFirstGame : public jframe::core::Application {
public:
    MyFirstGame() = default;
    ~MyFirstGame() override = default;

    bool initialize(jframe::core::Engine& engine) override;
    void updateFixed(jframe::DeltaTime dt) override;
    void render(float alpha) override;
    void shutdown() override;

private:
    jframe::core::Engine* engine_ = nullptr;
    jframe::Entity player_;
};
```

## Step 5: Create Game.cpp

Create `src/Game.cpp`:

```cpp
// src/Game.cpp
import std;
import jframe;
import jframe.core;

#include "Game.h"

bool MyFirstGame::initialize(jframe::core::Engine& engine) {
    jframe::core::logInfo("Initializing game...");

    engine_ = &engine;
    auto& sys = engine.systems();

    // Create the player entity
    player_ = sys.entities->createEntity();

    // Create a physics body for the player
    jframe::PhysicsBodyDef playerDef{
        .type = jframe::BodyType::Dynamic,
        .transform = {.x = 400.0f, .y = 300.0f},
        .size = {32.0f, 32.0f},
        .fixedRotation = true
    };
    sys.physics->createBody(player_, playerDef);

    // Create a ground platform
    jframe::Entity ground = sys.entities->createEntity();
    jframe::PhysicsBodyDef groundDef{
        .type = jframe::BodyType::Static,
        .transform = {.x = 400.0f, .y = 50.0f},
        .size = {800.0f, 20.0f},
        .fixedRotation = true
    };
    sys.physics->createBody(ground, groundDef);

    // Set up input mappings
    sys.input->registerMapping(jframe::InputMapping{
        .binding = jframe::InputBinding{
            .deviceType = jframe::InputDeviceType::Keyboard,
            .keyCode = 65,  // A key
            .scale = -1.0f
        },
        .action = "move_left"
    });

    sys.input->registerMapping(jframe::InputMapping{
        .binding = jframe::InputBinding{
            .deviceType = jframe::InputDeviceType::Keyboard,
            .keyCode = 68,  // D key
            .scale = 1.0f
        },
        .action = "move_right"
    });

    sys.input->registerMapping(jframe::InputMapping{
        .binding = jframe::InputBinding{
            .deviceType = jframe::InputDeviceType::Keyboard,
            .keyCode = 32  // Space
        },
        .action = "jump"
    });

    jframe::core::logInfo("Game initialized!");
    return true;
}

void MyFirstGame::updateFixed(jframe::DeltaTime dt) {
    auto& sys = engine_->systems();

    if (!sys.physics->hasBody(player_)) return;

    // Get input
    float moveLeft = sys.input->getActionValue("move_left");
    float moveRight = sys.input->getActionValue("move_right");
    float horizontal = moveLeft + moveRight;

    // Apply movement
    jframe::Vec2 velocity = sys.physics->getVelocity(player_);
    velocity.x = horizontal * 200.0f;  // Move speed

    // Jump input
    if (sys.input->wasActionJustPressed("jump")) {
        velocity.y = 400.0f;  // Jump force
    }

    sys.physics->setVelocity(player_, velocity);
}

void MyFirstGame::render(float alpha) {
    auto& sys = engine_->systems();

    // Draw the player as a green square
    if (sys.physics->hasBody(player_)) {
        jframe::Vec2 pos = sys.physics->getPosition(player_);
        sys.graphics->drawRect(
            {static_cast<int>(pos.x - 16), static_cast<int>(pos.y - 16), 32, 32},
            jframe::Color::green()
        );
    }
}

void MyFirstGame::shutdown() {
    jframe::core::logInfo("Shutting down game");
}
```

## Step 6: Build and Run

From the JFrame root directory:

```bash
# Configure
cmake --preset macos-debug

# Build
cmake --build --preset macos-debug

# Run
./build/macos-debug/examples/my-first-game/my-first-game
```

You should see a window with a green square that:
- Falls due to gravity
- Lands on a platform
- Moves left/right with A/D keys
- Jumps with Space

## Step 7: Modify Player Speed

Let's make the player move faster. In `Game.cpp`, change the move speed:

```cpp
// Change this line in updateFixed()
velocity.x = horizontal * 200.0f;  // Move speed

// To this:
velocity.x = horizontal * 300.0f;  // Faster!
```

Rebuild and run to see the difference.

## Step 8: Add a Second Entity

Let's add a box that the player can push around. In `Game.h`, add a new member:

```cpp
private:
    jframe::core::Engine* engine_ = nullptr;
    jframe::Entity player_;
    jframe::Entity box_;  // Add this
```

In `Game.cpp`, add this to `initialize()` after creating the player:

```cpp
// Create a pushable box
box_ = sys.entities->createEntity();
jframe::PhysicsBodyDef boxDef{
    .type = jframe::BodyType::Dynamic,
    .transform = {.x = 500.0f, .y = 200.0f},
    .size = {40.0f, 40.0f},
    .fixedRotation = true
};
sys.physics->createBody(box_, boxDef);
```

Add this to `render()` to draw the box:

```cpp
// Draw the box as a blue square
if (sys.physics->hasBody(box_)) {
    jframe::Vec2 pos = sys.physics->getPosition(box_);
    sys.graphics->drawRect(
        {static_cast<int>(pos.x - 20), static_cast<int>(pos.y - 20), 40, 40},
        jframe::Color::blue()
    );
}
```

Rebuild and run. You can now push the blue box around!

## Understanding the Code

### Application Lifecycle

JFrame games implement the `Application` interface with four methods:

1. **initialize()** - Called once at startup. Create entities, load assets, set up input.
2. **updateFixed()** - Called at a fixed timestep (60 FPS). Handle game logic, physics, AI.
3. **render()** - Called every frame. Draw graphics.
4. **shutdown()** - Called once at exit. Clean up resources.

### The Engine Builder

The `EngineBuilder` lets you configure which systems to use:

```cpp
EngineBuilder()
    .withEvents()      // Event system
    .withEntities()    // Entity-Component-System
    .withPhysics()     // Box2D physics
    .withGraphics(config)  // OpenGL rendering
    .withInput()       // Keyboard/mouse/controller
    .build();
```

### Entity-Component-System

JFrame uses an ECS architecture:

- **Entities** are just IDs (like `player_`)
- **Components** are data (like `PhysicsBodyDef`)
- **Systems** process entities with specific components

### Physics Bodies

Every physical object needs a `PhysicsBodyDef`:

- **Dynamic** - Affected by gravity and forces (player, boxes)
- **Static** - Never moves (ground, walls)
- **Kinematic** - Moves but isn't affected by forces (moving platforms)

## Next Steps

You've created your first JFrame game! Next tutorials:

- **Tutorial 2: Blueprints and Levels** - Learn data-driven entity creation with Lua
- **Tutorial 3: Physics and Collision** - Understand collision layers, callbacks, and ground detection
- **Tutorial 4: Input and Controls** - Advanced input handling and rebinding
- **Tutorial 5: Audio** - Add sounds and music to your game

## Troubleshooting

**Build fails with "import std not supported"**
- Make sure you're using LLVM Clang 20+, not Apple Clang
- See `docs/LLVM20-SETUP.md`

**Player falls through the ground**
- Check that the ground body is created before the physics system updates
- Verify the ground has `BodyType::Static`

**Input doesn't work**
- Make sure you called `withInput()` in the EngineBuilder
- Check that action names match exactly (case-sensitive)

**Window doesn't appear**
- Verify you called `withGraphics()` with a valid config
- Check console logs for errors
