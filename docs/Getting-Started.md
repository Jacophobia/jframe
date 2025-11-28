# Getting Started with JFrame

Welcome to JFrame, a modern C++23 game engine built with clean architecture, data-driven design, and developer productivity in mind.

This guide will help you set up JFrame and create your first game in minutes.

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Installation](#installation)
3. [Building JFrame](#building-jframe)
4. [Project Structure Overview](#project-structure-overview)
5. [Your First Game](#your-first-game)
6. [Understanding the Game Loop](#understanding-the-game-loop)
7. [Working with Systems](#working-with-systems)
8. [Next Steps](#next-steps)

---

## Prerequisites

### Required Tools

| Tool | Minimum Version | Purpose |
|------|-----------------|---------|
| CMake | 3.28+ | Build system with C++23 module support |
| vcpkg | Latest | Dependency management |
| Git | Any | Version control |

### Compiler Requirements

JFrame requires a C++23-compliant compiler with full module support, including `import std;`.

| Platform | Compiler | Version | Installation |
|----------|----------|---------|--------------|
| **macOS** | LLVM Clang | 20.0+ | `brew install llvm@20` (Apple Clang NOT supported) |
| **Windows** | MSVC | 19.38+ | Visual Studio 2022 17.8+ with `/std:c++latest` |
| **Windows** | Clang-CL | 17.0+ | LLVM for Windows |
| **Linux** | GCC | 13.0+ | Your package manager |
| **Linux** | Clang | 17.0+ | Your package manager |

**Important for macOS users:** Apple's bundled Clang (Xcode) does NOT support `import std;`. You must install LLVM Clang 20+ via Homebrew. See `docs/LLVM20-SETUP.md` for detailed setup instructions.

### Additional Requirements

- **FMOD Core API** (manual download required)
  - Download from [fmod.com](https://www.fmod.com/download)
  - Place in `external/fmod/` directory
  - Free for indie developers and evaluation

---

## Installation

### 1. Install vcpkg

```bash
# Clone vcpkg to your home directory
cd ~
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg

# Bootstrap vcpkg
./bootstrap-vcpkg.sh  # macOS/Linux
# OR
./bootstrap-vcpkg.bat  # Windows
```

Add vcpkg to your PATH or note its location for CMake configuration.

### 2. Install LLVM 20 (macOS only)

```bash
# Install LLVM 20 via Homebrew
brew install llvm@20

# Verify installation
/opt/homebrew/opt/llvm@20/bin/clang++ --version
```

See `docs/LLVM20-SETUP.md` for troubleshooting and detailed setup.

### 3. Clone JFrame

```bash
git clone https://github.com/yourusername/jframe.git
cd jframe
```

### 4. Install FMOD

1. Download FMOD Core API from [fmod.com](https://www.fmod.com/download)
2. Extract the archive
3. Copy the FMOD directory to `jframe/external/fmod/`

Your directory structure should look like:
```
jframe/
├── external/
│   └── fmod/
│       ├── api/
│       ├── core/
│       └── ...
```

---

## Building JFrame

JFrame uses CMake presets for platform-specific builds. All dependencies (except FMOD) are managed by vcpkg automatically.

### macOS

```bash
# Configure
cmake --preset macos-debug

# Build
cmake --build --preset macos-debug

# Run tests (optional)
ctest --preset macos-debug
```

### Windows

```bash
# Configure
cmake --preset windows-debug

# Build
cmake --build --preset windows-debug

# Run tests (optional)
ctest --preset windows-debug
```

### Linux

```bash
# Configure
cmake --preset linux-debug

# Build
cmake --build --preset linux-debug

# Run tests (optional)
ctest --preset linux-debug
```

### Build Modes

- **Debug**: Development builds with dev tools, hot reload, and Address Sanitizer
- **Release**: Optimized builds for distribution (no dev tools)

To build in Release mode, use the `-release` preset variant (e.g., `macos-release`).

### Verify Your Build

After building, run the platformer demo to verify everything works:

```bash
# macOS
./build/macos-debug/examples/platformer-demo/platformer-demo

# Windows
./build/windows-debug/examples/platformer-demo/Debug/platformer-demo.exe

# Linux
./build/linux-debug/examples/platformer-demo/platformer-demo
```

You should see a window with a platformer game featuring sprites, animations, physics, and collectibles.

---

## Project Structure Overview

```
jframe/
├── jframe-contract/        # System interfaces (API contracts)
│   └── src/
│       ├── jframe.cppm                # Main module (re-exports all)
│       ├── jframe.types.cppm          # Core types (Entity, Transform2D, etc.)
│       ├── jframe.entity.cppm         # Entity/Component system
│       ├── jframe.graphics.cppm       # Rendering system
│       ├── jframe.physics.cppm        # 2D physics (Box2D)
│       ├── jframe.input.cppm          # Input handling
│       ├── jframe.audio.cppm          # Audio system (FMOD)
│       ├── jframe.assets.cppm         # Asset loading
│       ├── jframe.level.cppm          # Level management
│       ├── jframe.events.cppm         # Event system
│       ├── jframe.ai.cppm             # AI/Pathfinding
│       └── ...
│
├── jframe-*/               # System implementations
│   ├── jframe-entity/      # Entity system (EnTT-based)
│   ├── jframe-graphics/    # Graphics system (OpenGL)
│   ├── jframe-physics/     # Physics system (Box2D)
│   └── ...
│
├── jframe-core/            # Core utilities
│   └── src/
│       ├── Engine.cpp      # Main engine runtime
│       ├── EngineBuilder.cpp  # Fluent API for engine setup
│       ├── Logging.cpp     # spdlog wrapper
│       └── ...
│
├── examples/               # Example games
│   ├── platformer-demo/    # Full-featured platformer
│   ├── ability-demo/       # Gameplay Ability System demo
│   └── endless-runner/     # Procedural generation example
│
├── docs/                   # Documentation
│   ├── Getting-Started.md  # This file
│   ├── CLAUDE.md           # Development guidelines
│   ├── jframe-technical-design.md  # Architecture details
│   └── systems/            # Per-system documentation
│
└── data/                   # Game data (in examples)
    ├── blueprints/         # Entity blueprints (Lua)
    ├── levels/             # Level definitions (Lua)
    ├── config/             # Configuration files (Lua)
    ├── textures/           # Image assets
    ├── audio/              # Sound files
    └── fonts/              # Font files
```

### Key Directories

- **jframe-contract**: Defines the public API for all systems (interfaces only)
- **jframe-{system}**: Implementation of each system (e.g., `jframe-graphics`)
- **jframe-core**: Engine runtime, utilities, logging, job system
- **examples**: Playable games showcasing JFrame features
- **docs**: All documentation, guides, and tutorials

---

## Your First Game

Let's create a minimal game that displays a window, renders a sprite, and handles input.

### Step 1: Project Setup

Create a new directory for your game:

```bash
mkdir my-first-game
cd my-first-game
```

Create a `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.28)
project(MyFirstGame CXX)

# Point to JFrame (adjust path as needed)
set(JFRAME_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../jframe")
add_subdirectory(${JFRAME_DIR} jframe)

# Create your game executable
add_executable(my-game
    src/main.cpp
    src/Game.cpp
    src/Game.h
)

# Link against JFrame core (brings in all systems)
target_link_libraries(my-game PRIVATE jframe-core)

# Copy data directory to build output
add_custom_command(TARGET my-game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/data
        $<TARGET_FILE_DIR:my-game>/data
)
```

### Step 2: Create Your Game Class

**src/Game.h**

```cpp
#pragma once

import std;
import jframe;
import jframe.core;

class Game {
public:
    // Engine lifecycle
    bool initialize(jframe::core::Engine& engine);
    void updateFixed(jframe::DeltaTime dt);
    void render(float alpha);
    void shutdown();

private:
    jframe::core::Engine* engine_ = nullptr;
    jframe::Entity player_;
    jframe::AssetHandle playerTexture_;
};
```

**src/Game.cpp**

```cpp
import std;
import jframe;
import jframe.core;

#include "Game.h"

bool Game::initialize(jframe::core::Engine& engine) {
    engine_ = &engine;
    auto& sys = engine.systems();

    jframe::core::logInfo("Initializing My First Game");

    // Load player texture
    playerTexture_ = sys.assets->registerAsset(
        jframe::AssetType::Texture,
        "data/textures/player.png"
    );
    sys.assets->loadAsset(playerTexture_);

    // Create player entity
    player_ = sys.entities->createEntity();

    // Add transform component (position at center of 1280x720 window)
    sys.entities->emplace<jframe::Transform2D>(player_, jframe::Transform2D{
        .x = 640.0f,
        .y = 360.0f,
        .rotation = 0.0f,
        .scaleX = 1.0f,
        .scaleY = 1.0f
    });

    // Setup input
    sys.input->registerMapping(jframe::InputMapping{
        .binding = jframe::InputBinding{
            .deviceType = jframe::InputDeviceType::Keyboard,
            .keyCode = 65  // 'A' key
        },
        .action = "move_left"
    });
    sys.input->registerMapping(jframe::InputMapping{
        .binding = jframe::InputBinding{
            .deviceType = jframe::InputDeviceType::Keyboard,
            .keyCode = 68  // 'D' key
        },
        .action = "move_right"
    });

    jframe::core::logInfo("Game initialized successfully");
    return true;
}

void Game::updateFixed(jframe::DeltaTime dt) {
    auto& sys = engine_->systems();
    auto& transform = sys.entities->get<jframe::Transform2D>(player_);

    // Handle input - move player left/right
    float moveSpeed = 200.0f;  // pixels per second

    if (sys.input->isActionActive("move_left")) {
        transform.x -= moveSpeed * dt;
    }
    if (sys.input->isActionActive("move_right")) {
        transform.x += moveSpeed * dt;
    }

    // Wrap around screen edges
    if (transform.x < 0.0f) transform.x = 1280.0f;
    if (transform.x > 1280.0f) transform.x = 0.0f;
}

void Game::render(float alpha) {
    auto& sys = engine_->systems();
    auto& transform = sys.entities->get<jframe::Transform2D>(player_);

    // Create sprite sheet (single frame)
    jframe::SpriteSheet sheet{
        .texture = playerTexture_,
        .frameWidth = 64,
        .frameHeight = 64,
        .columns = 1,
        .rows = 1
    };

    // Draw the player sprite
    sys.graphics->drawSprite(sheet, 0, transform);
}

void Game::shutdown() {
    jframe::core::logInfo("Shutting down game");
}
```

**src/main.cpp**

```cpp
import std;
import jframe;
import jframe.core;

#include "Game.h"

int main(int argc, char* argv[]) {
    jframe::core::logInfo("Starting My First Game");

    // Build the engine with required systems
    auto engineResult = jframe::core::EngineBuilder()
        .withEvents()
        .withEntities()
        .withGraphics(jframe::core::GraphicsConfig{
            .width = 1280,
            .height = 720,
            .title = "My First JFrame Game",
            .vsync = true,
            .clearColor = {30, 30, 50, 255}  // Dark blue background
        })
        .withInput()
        .withAssets("data")
        .build();

    if (!engineResult) {
        jframe::core::logError("Failed to build engine: " + engineResult.error());
        return 1;
    }

    jframe::core::logInfo("Engine built successfully");

    // Create the game and run it
    Game game;
    engineResult.value().run(game);

    return 0;
}
```

### Step 3: Add Assets

Create a `data/textures/` directory and add a 64x64 PNG image called `player.png`.

```bash
mkdir -p data/textures
# Add your player.png image here (64x64 pixels recommended)
```

### Step 4: Build and Run

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build

# Run
./build/my-game
```

You should see a window with your sprite! Use 'A' and 'D' keys to move it left and right.

---

## Understanding the Game Loop

JFrame uses a fixed-timestep game loop for physics and logic, with variable-timestep rendering for smooth visuals.

```cpp
class Game {
public:
    // Called once at startup
    bool initialize(jframe::core::Engine& engine);

    // Called at fixed intervals (e.g., 60 FPS)
    // Use for physics, game logic, AI
    void updateFixed(jframe::DeltaTime dt);

    // Called every frame with interpolation alpha
    // Use for rendering only
    void render(float alpha);

    // Called once at shutdown
    void shutdown();
};
```

### Typical Responsibilities

| Method | Purpose | Example Code |
|--------|---------|--------------|
| `initialize()` | Setup game state, load assets, create entities | Load textures, create player entity |
| `updateFixed()` | Update game logic at fixed timestep | Move entities, check collisions, update AI |
| `render()` | Draw the current frame | Render sprites, UI, effects |
| `shutdown()` | Cleanup, save data | Unsubscribe events, save progress |

**Important:** Never update game state in `render()`. It's called at variable rates and is only for drawing.

---

## Working with Systems

JFrame is built around the concept of **systems**. Each system provides a specific capability (graphics, physics, audio, etc.). You access systems through the `engine.systems()` object.

### Available Systems

```cpp
auto& sys = engine.systems();

// Entity/Component System (EnTT-based)
sys.entities->createEntity();
sys.entities->emplace<Component>(entity, ...);
sys.entities->get<Component>(entity);

// Graphics System
sys.graphics->drawSprite(sheet, frame, transform);
sys.graphics->drawText(text, position, font, size, color);
sys.graphics->setCamera(camera);

// Physics System (Box2D)
sys.physics->createBody(entity, bodyDef);
sys.physics->setVelocity(entity, velocity);
sys.physics->applyForce(entity, force);

// Input System
sys.input->registerMapping(mapping);
sys.input->isActionActive(action);
sys.input->wasActionJustPressed(action);

// Audio System (FMOD)
sys.audio->playPositional(sound);
sys.audio->playMusic(music);
sys.audio->setVolume(handle, volume);

// Asset System
sys.assets->registerAsset(type, path);
sys.assets->loadAsset(handle);
sys.assets->loadAssetAsync(handle, callback);

// Event System
sys.events->publish(eventType, data);
sys.events->subscribe(eventType, callback);

// Level System
sys.levels->loadLevel(assetHandle);
sys.levels->setActiveLevel(levelId);

// AI System
sys.ai->setPatrolBehavior(entity, patrol);
sys.ai->findPath(start, end);
```

### System Initialization Order

The `EngineBuilder` handles system initialization for you. Just chain the systems you need:

```cpp
auto engineResult = jframe::core::EngineBuilder()
    .withEvents()        // Event system (no dependencies)
    .withEntities()      // Entity system (requires Events)
    .withPhysics()       // Physics (requires Entities, Events)
    .withGraphics(config)  // Graphics (requires Entities)
    .withInput()         // Input (requires Events)
    .withAssets(dataDir)   // Assets (requires Events)
    .withAudio()         // Audio (requires Assets)
    .withLevel()         // Level (requires all above)
    .withAI()            // AI (requires Physics)
    .build();
```

**Common System Combinations:**

- **Minimal 2D Game**: Events, Entities, Graphics, Input, Assets
- **Physics Game**: Add Physics
- **With Sound**: Add Audio
- **Data-Driven**: Add Level (for Lua-based levels and blueprints)
- **With AI**: Add AI (for pathfinding and behavior trees)

---

## Next Steps

Now that you have a basic game running, explore these topics to level up:

### Beginner Topics

1. **Sprites and Animation**
   - Learn about sprite sheets, frame-based animation, and the `AnimatedSprite` component
   - See: `examples/platformer-demo/src/Game.cpp` (animation setup)

2. **Physics Bodies**
   - Add dynamic/kinematic/static physics bodies with Box2D
   - Handle collisions and triggers
   - See: `docs/systems/Physics-System.md`

3. **Input Mapping**
   - Map keyboard, mouse, and gamepad inputs to actions
   - Support multiple bindings per action
   - See: `docs/systems/Input-System.md`

4. **Audio**
   - Play positional sounds and background music with FMOD
   - Control volume, pitch, and 3D audio
   - See: `docs/systems/Audio-System.md`

### Intermediate Topics

5. **Entity Component System (ECS)**
   - Understand components, entities, and systems pattern
   - Use EnTT views for efficient iteration
   - See: `docs/systems/Entity-System.md`

6. **Data-Driven Design**
   - Define entities in Lua blueprints
   - Create levels in Lua with loops and logic
   - See: `examples/platformer-demo/data/blueprints/` and `data/levels/`

7. **Event System**
   - Publish and subscribe to game events
   - Handle physics collisions, input events, and custom events
   - See: `docs/systems/Event-System.md`

8. **Camera System**
   - Implement camera follow, smoothing, and deadzone
   - Add screen shake and zoom effects
   - See: `jframe-camera/` implementation

### Advanced Topics

9. **AI and Pathfinding**
   - Use behavior trees for AI logic
   - Navigate with Recast/Detour pathfinding
   - See: `docs/systems/AI-System.md`

10. **Save System**
    - Serialize game state with cereal
    - Compress saves with zstd
    - See: `docs/systems/Save-System.md`

11. **Gameplay Ability System (GAS)**
    - Create reusable abilities with cooldowns and costs
    - Apply gameplay effects and modifiers
    - See: `docs/GAS-Guide.md` and `examples/ability-demo/`

12. **Hot Reload (Debug Only)**
    - Reload Lua scripts, textures, and audio at runtime
    - Iterate quickly without recompiling
    - See: `jframe-dev/` implementation

### Example Projects

Study these complete examples to see JFrame in action:

- **platformer-demo**: Full 2D platformer with sprites, physics, AI, and audio
  - Location: `examples/platformer-demo/`
  - Features: Player movement, jumping, collectibles, enemies, health, HUD

- **ability-demo**: Gameplay Ability System showcase
  - Location: `examples/ability-demo/`
  - Features: Abilities, cooldowns, mana, effects, attributes

- **endless-runner**: Procedural generation example
  - Location: `examples/endless-runner/`
  - Features: Infinite scrolling, obstacle spawning, score system

### Documentation

- **CLAUDE.md**: Development guidelines, coding standards, architecture
- **jframe-technical-design.md**: Deep dive into engine architecture
- **docs/systems/**: Per-system documentation with API reference
- **docs/GAS-Guide.md**: Gameplay Ability System tutorial

### Community and Support

- GitHub Issues: Report bugs or request features
- GitHub Discussions: Ask questions and share projects
- Discord: Join the JFrame community (link TBD)

---

## Common Pitfalls

### 1. Mixing `#include` and `import std;`

**DON'T:**
```cpp
#include <iostream>  // Old-style header
import std;          // Module import
```

**DO:**
```cpp
import std;  // Use ONLY module imports for standard library
```

### 2. Forgetting to Load Assets

```cpp
// Register the asset
auto texture = sys.assets->registerAsset(AssetType::Texture, "player.png");

// DON'T forget to load it!
sys.assets->loadAsset(texture);  // Or loadAssetAsync()
```

### 3. Updating Game State in `render()`

```cpp
// WRONG - don't modify entities in render()
void Game::render(float alpha) {
    player.x += 10;  // NO! This runs at variable rate!
}

// RIGHT - update in updateFixed()
void Game::updateFixed(DeltaTime dt) {
    player.x += speed * dt;  // YES! Fixed timestep
}
```

### 4. Not Checking System Dependencies

Some systems depend on others. The `EngineBuilder` enforces this, but be aware:

```cpp
// WRONG - Graphics needs Entities
auto engine = EngineBuilder()
    .withGraphics(config)  // Error! Entities not initialized
    .withEntities()
    .build();

// RIGHT - Entities before Graphics
auto engine = EngineBuilder()
    .withEntities()
    .withGraphics(config)
    .build();
```

### 5. Using Apple Clang on macOS

**JFrame requires LLVM Clang 20+** on macOS. Apple Clang does NOT support `import std;`.

```bash
# Check your compiler
which clang++

# Should be: /opt/homebrew/opt/llvm@20/bin/clang++
# NOT: /usr/bin/clang++ (Apple Clang)
```

See `docs/LLVM20-SETUP.md` for setup instructions.

---

## Quick Reference

### Engine Initialization

```cpp
auto engineResult = jframe::core::EngineBuilder()
    .withEvents()
    .withEntities()
    .withGraphics(jframe::core::GraphicsConfig{
        .width = 1280,
        .height = 720,
        .title = "My Game",
        .vsync = true,
        .clearColor = {0, 0, 0, 255}
    })
    .withInput()
    .withAssets("data")
    .build();

if (!engineResult) {
    jframe::core::logError("Build failed: " + engineResult.error());
    return 1;
}

Game game;
engineResult.value().run(game);
```

### Entity Creation

```cpp
auto entity = sys.entities->createEntity();
sys.entities->emplace<jframe::Transform2D>(entity, jframe::Transform2D{
    .x = 100.0f, .y = 200.0f
});
```

### Physics Body

```cpp
jframe::PhysicsBodyDef def{
    .type = jframe::BodyType::Dynamic,
    .transform = {.x = 100, .y = 200},
    .size = {50, 50},
    .fixedRotation = true,
    .density = 1.0f
};
sys.physics->createBody(entity, def);
```

### Input Handling

```cpp
sys.input->registerMapping(jframe::InputMapping{
    .binding = jframe::InputBinding{
        .deviceType = jframe::InputDeviceType::Keyboard,
        .keyCode = 32  // Spacebar
    },
    .action = "jump"
});

// In update
if (sys.input->wasActionJustPressed("jump")) {
    // Jump!
}
```

### Event Subscription

```cpp
auto subId = sys.events->subscribe(
    jframe::Events::Collision,
    [](const jframe::EventData& data) {
        auto& collision = std::get<jframe::CollisionEvent>(data);
        // Handle collision
    }
);

// Don't forget to unsubscribe in shutdown()
sys.events->unsubscribe(subId);
```

---

## Congratulations!

You now have everything you need to start building games with JFrame. The platformer demo in `examples/platformer-demo/` is a great reference for a complete game implementation.

Happy game development!

---

**Need Help?**

- Read the docs: `docs/`
- Study examples: `examples/`
- Check issues: GitHub Issues
- Ask questions: GitHub Discussions
