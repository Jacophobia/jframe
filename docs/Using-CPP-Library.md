# Using Bestow as a C++ Library

This guide is for developers who want to use Bestow as a C++ library rather than the Lua CLI.

> **Looking for Lua documentation?** Most game developers should use the [Lua CLI approach](Getting-Started.md) - it's simpler and provides hot reload.

## When to Use C++

Use Bestow as a C++ library when you need:
- **Custom engine systems** - Add new systems beyond what the Lua API provides
- **Maximum performance** - Bypass Lua overhead for performance-critical code
- **Direct hardware access** - Low-level graphics, audio, or input handling
- **C++ integration** - Use Bestow with existing C++ codebases
- **Custom tools** - Build editors, asset processors, or dev tools

---

## Quick Start

### 1. Create Your Project

```bash
mkdir my-game && cd my-game
```

Create `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.28)
project(my_game LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    bestow
    GIT_REPOSITORY https://github.com/Jacophobia/bestow.git
    GIT_TAG        main
)

set(BESTOW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BESTOW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(bestow)

add_executable(my_game)
target_sources(my_game PRIVATE
    FILE_SET CXX_MODULES FILES
    src/game.cppm
    src/main.cpp
)
target_link_libraries(my_game PRIVATE bestow-core)
```

### 2. Write Your Game

Create `src/game.cppm`:

```cpp
module;

export module my.game;

import std;
import bestow;

export class MyGame : public bestow::Application<MyGame,
    bestow::IGraphics3DSystem,
    bestow::IInputSystem>
{
public:
    MyGame(bestow::IGraphics3DSystem& graphics, bestow::IInputSystem& input)
        : graphics_(&graphics), input_(&input) {}

    void run() override {
        while (!input_->wasKeyJustPressed(bestow::Key::Escape)) {
            input_->update();

            graphics_->beginFrame();
            // Render your scene
            graphics_->endFrame();
        }
    }

private:
    bestow::IGraphics3DSystem* graphics_;
    bestow::IInputSystem* input_;
};
```

Create `src/main.cpp`:

```cpp
import std;
import bestow.core;
import bestow.vulkan.impl;
import bestow.input.impl;
import my.game;

int main() {
    bestow::core::Engine engine;

    // Register system implementations
    engine.use<bestow::IGraphics3DSystem, bestow::VulkanGraphics3DSystem>();
    engine.use<bestow::IInputSystem, bestow::InputSystem>();

    // Run the game (dependencies auto-injected)
    engine.run<MyGame>();

    return 0;
}
```

### 3. Build and Run

```bash
cmake --preset default
cmake --build build
./build/my_game
```

---

## Core Concepts

### Contract-Based Architecture

Bestow uses **contracts** (interfaces) to decouple systems. Your game depends on interfaces, not implementations.

```cpp
// Game depends on contracts (interfaces)
class MyGame : public Application<MyGame, IGraphics3DSystem, IInputSystem> { ... };

// main.cpp chooses implementations
engine.use<IGraphics3DSystem, VulkanGraphics3DSystem>();  // Could be OpenGL!
engine.use<IInputSystem, InputSystem>();
```

**Benefits:**
- Swap implementations without changing game code
- Mock systems for testing
- Multiple renderer backends (Vulkan/OpenGL)

### Application Pattern

Your game inherits from `Application<YourGame, Dependencies...>`:

```cpp
class MyGame : public bestow::Application<MyGame,
    bestow::IGraphics3DSystem,
    bestow::IInputSystem,
    bestow::IAudioSystem,
    bestow::IEntitySystem>
{
public:
    // Constructor params match template args
    MyGame(
        bestow::IGraphics3DSystem& graphics,
        bestow::IInputSystem& input,
        bestow::IAudioSystem& audio,
        bestow::IEntitySystem& entities
    ) : graphics_(&graphics), input_(&input),
        audio_(&audio), entities_(&entities) {}

    void run() override {
        initialize();
        gameLoop();
        shutdown();
    }

private:
    void initialize();
    void gameLoop();
    void shutdown();

    bestow::IGraphics3DSystem* graphics_;
    bestow::IInputSystem* input_;
    bestow::IAudioSystem* audio_;
    bestow::IEntitySystem* entities_;
};
```

### Engine API

```cpp
bestow::core::Engine engine;

// Register implementations
engine.use<IContract, Implementation>();

// Check availability (for optional systems)
if (engine.has<IAudioSystem>()) {
    // Audio is available
}

// Get systems directly
auto& graphics = engine.get<IGraphics3DSystem>();
auto* audio = engine.tryGet<IAudioSystem>();  // nullptr if not registered

// Run with auto-injected dependencies
engine.run<MyGame>();
```

---

## Available Systems

### System Interfaces

| Interface | Implementation | Purpose |
|-----------|----------------|---------|
| `IEventSystem` | `EventSystem` | Publish/subscribe events |
| `IEntitySystem` | `EntitySystem` | Entity Component System (EnTT) |
| `IGraphics3DSystem` | `VulkanGraphics3DSystem` | 3D rendering (Vulkan) |
| `IGraphics3DSystem` | `OpenGLGraphics3DSystem` | 3D rendering (OpenGL) |
| `IGraphicsSystem` | `GraphicsSystem` | 2D sprite rendering |
| `IInputSystem` | `InputSystem` | Keyboard, mouse, gamepad |
| `IAudioSystem` | `FmodAudioSystem` | Sound and music (FMOD) |
| `IPhysicsSystem` | `Box2DPhysicsSystem` | 2D physics |
| `IPhysics3DSystem` | `JoltPhysics3DSystem` | 3D physics |
| `IAssetSystem` | `AssetSystem` | Asset loading and caching |
| `ISaveSystem` | `SaveSystem` | Save/load game state |
| `ILevelSystem` | `LevelSystem` | Level loading (Lua) |
| `IConfigSystem` | `ConfigSystem` | Configuration (Lua) |
| `IAISystem` | `AISystem` | Behavior trees, pathfinding |
| `ICameraSystem` | `CameraSystem` | Camera control |
| `IGASSystem` | `GASSystem` | Gameplay Ability System |

### Importing Systems

```cpp
import bestow;                    // All interfaces
import bestow.entity;             // Just entity system
import bestow.graphics;           // Just graphics interface
import bestow.vulkan.impl;        // Vulkan implementation
import bestow.opengl.impl;        // OpenGL implementation
```

---

## Entity Component System

Bestow uses EnTT for ECS:

```cpp
// Create entity
Entity player = entities_->createEntity();

// Add components
entities_->emplace<Transform3D>(player, Transform3D{
    .position = Vec3{0, 1, 0},
    .rotation = Quat::identity(),
    .scale = Vec3{1, 1, 1}
});
entities_->emplace<Health>(player, Health{100, 100});
entities_->emplace<PlayerTag>(player);

// Query entities
auto view = entities_->view<Transform3D, Health>();
for (auto [entity, transform, health] : view.each()) {
    // Process entities with Transform3D AND Health
}

// Get single component
auto& transform = entities_->get<Transform3D>(player);
transform.position.y += 1.0f;

// Check component existence
if (entities_->has<PlayerTag>(player)) {
    // Is a player
}
```

---

## Game Loop

Bestow uses a fixed-timestep game loop:

```cpp
void MyGame::run() {
    initialize();

    constexpr float fixedDt = 1.0f / 60.0f;  // 60 Hz physics
    float accumulator = 0.0f;

    while (running_) {
        float frameDt = timer_.tick();
        accumulator += frameDt;

        // Fixed timestep updates (physics, game logic)
        while (accumulator >= fixedDt) {
            input_->update();
            updateFixed(fixedDt);
            physics_->step(fixedDt);
            accumulator -= fixedDt;
        }

        // Variable timestep rendering
        float alpha = accumulator / fixedDt;
        render(alpha);
    }

    shutdown();
}

void MyGame::updateFixed(float dt) {
    // Game logic at consistent 60 Hz
}

void MyGame::render(float alpha) {
    graphics_->beginFrame();
    // Interpolate positions for smooth rendering
    graphics_->endFrame();
}
```

---

## Adding Systems to the Engine

### Using Built-in Systems

```cpp
bestow::core::Engine engine;

// Core systems
engine.use<bestow::IEventSystem, bestow::EventSystem>();
engine.use<bestow::IEntitySystem, bestow::EntitySystem>();

// Graphics (choose one)
engine.use<bestow::IGraphics3DSystem, bestow::VulkanGraphics3DSystem>();
// OR: engine.use<bestow::IGraphics3DSystem, bestow::OpenGLGraphics3DSystem>();

// Input
engine.use<bestow::IInputSystem, bestow::InputSystem>();

// Audio
engine.use<bestow::IAudioSystem, bestow::FmodAudioSystem>();

// Physics
engine.use<bestow::IPhysicsSystem, bestow::Box2DPhysicsSystem>();

// Assets
engine.use<bestow::IAssetSystem, bestow::AssetSystem>();

engine.run<MyGame>();
```

### Creating Custom Systems

Implement a contract interface:

```cpp
// my_audio_system.cppm
export module my.audio;

import bestow.audio;

export class MyAudioSystem : public bestow::IAudioSystem {
public:
    SoundHandle loadSound(std::string_view path) override {
        // Your implementation
    }

    void playSound(SoundHandle handle, float volume, float pitch) override {
        // Your implementation
    }

    void stopSound(SoundHandle handle) override {
        // Your implementation
    }

    void setMasterVolume(float volume) override {
        masterVolume_ = volume;
    }

private:
    float masterVolume_ = 1.0f;
};
```

Register your custom implementation:

```cpp
engine.use<bestow::IAudioSystem, MyAudioSystem>();
```

---

## Project Setup Options

### Option 1: CMake FetchContent (Recommended)

```cmake
include(FetchContent)

FetchContent_Declare(
    bestow
    GIT_REPOSITORY https://github.com/Jacophobia/bestow.git
    GIT_TAG        v1.0.0
)

set(BESTOW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BESTOW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(bestow)

target_link_libraries(my_game PRIVATE bestow-core)
```

### Option 2: Git Submodule

```bash
git submodule add https://github.com/Jacophobia/bestow.git external/bestow
```

```cmake
add_subdirectory(external/bestow)
target_link_libraries(my_game PRIVATE bestow-core)
```

### Option 3: System Install

```bash
cmake --preset release
cmake --build --preset release
sudo cmake --install build/release
```

```cmake
find_package(bestow REQUIRED)
target_link_libraries(my_game PRIVATE bestow::bestow-core)
```

---

## Compiler Requirements

Bestow requires C++23 with `import std;` support:

| Platform | Compiler | Minimum Version |
|----------|----------|-----------------|
| macOS | LLVM Clang | 20+ |
| Linux | LLVM Clang | 20+ |
| Linux | GCC | 13+ |
| Windows | MSVC | 19.38+ (VS 2022 17.8+) |
| Windows | Clang-CL | 17+ |

**macOS Note:** Apple Clang does not support `import std;`. You must use LLVM Clang 20+ from Homebrew. See [LLVM20-SETUP.md](LLVM20-SETUP.md).

---

## Available CMake Targets

| Target | Description |
|--------|-------------|
| `bestow-core` | All systems (use this for most projects) |
| `bestow-entity` | Entity Component System only |
| `bestow-graphics3d` | 3D graphics (OpenGL) |
| `bestow-vulkan` | Vulkan renderer |
| `bestow-audio` | Audio (FMOD) |
| `bestow-input` | Input handling |
| `bestow-physics` | 2D physics (Box2D) |
| `bestow-physics3d` | 3D physics (Jolt) |
| `bestow-assets` | Asset management |
| `bestow-events` | Event system |
| `bestow-save` | Serialization |
| `bestow-level` | Level loading |
| `bestow-ai` | AI/Pathfinding |
| `bestow-camera` | Camera system |
| `bestow-config` | Configuration |
| `bestow-gas` | Gameplay Ability System |
| `bestow-dev` | Dev tools (debug builds only) |

---

## See Also

- [Architecture Guide](Architecture.md) - Detailed system architecture
- [API Reference](api/) - System API documentation
- [Technical Design](bestow-technical-design.md) - Deep dive into internals
- [System Implementation Guide](SYSTEM-IMPLEMENTATION-GUIDE.md) - Creating custom systems
- [Using as Dependency](USING-AS-DEPENDENCY.md) - Advanced integration options
