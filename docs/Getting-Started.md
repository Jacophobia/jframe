# Getting Started with Bestow

Bestow is a modern C++23 game engine with contract-based dependency injection. This guide shows how to create your first game.

## Prerequisites

- **CMake 3.28+** with C++23 module support
- **LLVM Clang 20+** (macOS) or **MSVC 19.38+** (Windows)
- **Vulkan SDK** installed
- **vcpkg** for dependencies

See `docs/Installation.md` for detailed setup.

## Quick Start

### 1. Create Your Game Class

Your game inherits from `Application<>` and lists its dependencies as template parameters:

```cpp
// src/game.cppm
export module my.game;

import bestow.services;   // Contracts + Application base
import bestow.types;
import bestow.graphics3d;

export class MyGame : public bestow::Application<MyGame,
    bestow::IGraphics3DSystem,
    bestow::IInputSystem>
{
public:
    // Constructor params match template args
    MyGame(bestow::IGraphics3DSystem& graphics, bestow::IInputSystem& input)
        : graphics_(&graphics), input_(&input) {}

    void run() override {
        initialize();
        gameLoop();
        cleanup();
    }

private:
    bool initialize() {
        // Setup graphics
        bestow::Graphics3DConfig config{
            .windowWidth = 1280,
            .windowHeight = 720,
            .windowTitle = "My First Game",
            .vsync = true
        };
        if (!graphics_->initialize(config)) return false;

        // Create a cube mesh
        auto result = graphics_->createCubeMesh(1.0f);
        if (result) cubeMesh_ = *result;

        material_ = graphics_->getDefaultPBRMaterial();

        // Setup camera
        bestow::Camera3D cam;
        cam.fovY = 45.0f;
        cam.transform.position = {5.0f, 5.0f, 5.0f};
        graphics_->setCamera(cam);

        // Setup lighting
        graphics_->setDirectionalLight({
            .direction = {0.5f, -1.0f, 0.3f},
            .color = {1.0f, 1.0f, 1.0f},
            .intensity = 1.0f
        });

        return true;
    }

    void gameLoop() {
        while (!graphics_->shouldClose()) {
            input_->update();

            // Handle input (ESC to quit)
            if (input_->wasKeyJustPressed(GLFW_KEY_ESCAPE)) break;

            // Rotate cube
            rotation_ += 0.016f;

            // Render
            graphics_->beginFrame();
            bestow::Mat4 transform = glm::rotate(
                glm::identity<glm::mat4>(),
                rotation_,
                glm::vec3(0.0f, 1.0f, 0.0f)
            );
            graphics_->drawMesh(cubeMesh_, material_, transform);
            graphics_->endFrame();
        }
    }

    void cleanup() {
        input_->shutdown();
        graphics_->shutdown();
    }

    bestow::IGraphics3DSystem* graphics_;
    bestow::IInputSystem* input_;
    bestow::MeshHandle cubeMesh_ = 0;
    bestow::MaterialHandle material_ = 0;
    float rotation_ = 0.0f;
};
```

### 2. Create Entry Point

```cpp
// src/main.cpp
import std;
import bestow.core;           // Engine class
import bestow.services;       // Contract interfaces

// Import implementations you want to use
import bestow.vulkan.impl;    // VulkanGraphics3DSystem
import bestow.input.impl;     // InputSystem
import bestow.events.impl;    // EventSystem (often needed as dependency)
import bestow.assets.impl;    // AssetSystem (often needed as dependency)
import bestow.config.impl;    // ConfigSystem
import bestow.shader.impl;    // ShaderSystem

import my.game;

int main() {
    bestow::core::Engine engine;

    // Register system implementations
    engine.use<bestow::IEventSystem, bestow::EventSystem>();
    engine.use<bestow::IAssetSystem, bestow::AssetSystem>();
    engine.use<bestow::IConfigSystem, bestow::ConfigSystem>();
    engine.use<bestow::IShaderSystem, bestow::OpenGLShaderSystem>();
    engine.use<bestow::IGraphics3DSystem, bestow::VulkanGraphics3DSystem>();
    engine.use<bestow::IInputSystem, bestow::InputSystem>();

    // Run your game - dependencies auto-injected!
    engine.run<MyGame>();

    return 0;
}
```

### 3. Create CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.28)
project(MyGame CXX)

# Point to Bestow
add_subdirectory(path/to/bestow bestow)

add_executable(my-game src/main.cpp)

target_sources(my-game
    PUBLIC FILE_SET CXX_MODULES FILES
        src/game.cppm
)

target_link_libraries(my-game
    PRIVATE
        bestow-contract
        bestow-core
        bestow-vulkan
        bestow-input
        bestow-events
        bestow-assets
        bestow-config
        bestow-shader
)

target_compile_features(my-game PRIVATE cxx_std_23)
target_use_std_module(my-game)
```

### 4. Build and Run

```bash
cmake --preset macos-debug  # or windows-debug, linux-debug
cmake --build --preset macos-debug
./build/macos-debug/my-game
```

## The Application Pattern

Bestow uses a CRTP base class for automatic dependency injection:

```cpp
// Inherit from Application<YourClass, Dependencies...>
class MyGame : public Application<MyGame, IGraphics3DSystem, IInputSystem, IAudioSystem>
{
public:
    // Constructor params must match template dependencies (in order)
    MyGame(IGraphics3DSystem& g, IInputSystem& i, IAudioSystem& a)
        : graphics_(&g), input_(&i), audio_(&a) {}

    void run() override { /* your game loop */ }
};

// In main.cpp - just call run<>() without listing deps!
engine.run<MyGame>();  // Dependencies auto-detected from base class
```

## Engine API

### Registering Systems

```cpp
// Register implementation for a contract
engine.use<IContract, Implementation>();

// Examples
engine.use<IGraphics3DSystem, VulkanGraphics3DSystem>();
engine.use<IAudioSystem, AudioSystem>();
```

### Checking System Availability

```cpp
// Check if a system is registered
if (engine.has<IAudioSystem>()) {
    auto& audio = engine.get<IAudioSystem>();
}

// Get system or nullptr
auto* audio = engine.tryGet<IAudioSystem>();
if (audio) {
    audio->playSound(...);
}
```

## Controls

Bestow uses **Dvorak-friendly** default controls:

| Action | Dvorak | QWERTY Equivalent |
|--------|--------|-------------------|
| Up | `,` (comma) | W |
| Down | `O` | S |
| Left | `A` | A |
| Right | `E` | D |

Arrow keys also work.

## Example Game

See `games/game1/` for a complete 3D Snake game example demonstrating:
- 3D isometric rendering
- Camera following
- Grid-based movement
- Input handling
- Game state management

## Next Steps

- Read `template/README.md` for detailed template documentation
- Study `games/game1/` for a complete example
- Check `docs/api/` for system API reference
- See `CLAUDE.md` for development guidelines
