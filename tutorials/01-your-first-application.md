# Tutorial 01: Your First Bestow Application

This tutorial walks you through creating your first Bestow game from scratch. You'll learn the core concepts of the engine's dependency injection architecture.

## What You'll Build

A simple 3D application that displays a rotating cube. This covers:
- Setting up the Engine and registering systems
- Creating an Application class with dependency injection
- Basic 3D rendering with meshes, materials, and lighting
- Handling input

## Prerequisites

- Bestow built and installed (see `docs/Installation.md`)
- Basic C++ knowledge
- Familiarity with CMake

## Step 1: Project Setup

Create a new directory for your project:

```bash
mkdir my-first-game
cd my-first-game
mkdir src
```

## Step 2: Create Your Game Class

Create `src/game.cppm`:

```cpp
// src/game.cppm
// Your game application with automatic dependency injection

module;

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

export module my.game;

import std;
import bestow.services;   // All contracts + Application<> base class
import bestow.types;
import bestow.graphics3d;

export namespace mygame {

//=============================================================================
// MyGame - Your Application
//
// Key concept: Inherit from Application<YourClass, Dependencies...>
// The template parameters list the system contracts your game needs.
// The Engine will automatically inject these when you call engine.run<MyGame>()
//=============================================================================

class MyGame : public bestow::Application<MyGame,
    bestow::IGraphics3DSystem,
    bestow::IInputSystem>
{
public:
    // Constructor: Parameters MUST match the template arguments in order
    MyGame(bestow::IGraphics3DSystem& graphics, bestow::IInputSystem& input)
        : graphics_(&graphics)
        , input_(&input)
    {
        // Dependencies are now available!
    }

    // Required: Implement run() from IApplication
    void run() override {
        if (!initialize()) {
            std::cerr << "Failed to initialize\n";
            return;
        }

        gameLoop();
        cleanup();
    }

private:
    //=========================================================================
    // Initialization
    //=========================================================================

    bool initialize() {
        // Configure the graphics system
        bestow::Graphics3DConfig config{
            .windowWidth = 1280,
            .windowHeight = 720,
            .windowTitle = "My First Bestow Game",
            .vsync = true,
            .fullscreen = false
        };

        if (!graphics_->initialize(config)) {
            return false;
        }

        // Set background color
        graphics_->setClearColor(bestow::Color{30, 30, 50, 255});

        // Initialize input (pass the window handle from graphics)
        input_->initialize(graphics_->getNativeWindowHandle());

        // Create a cube mesh
        auto meshResult = graphics_->createCubeMesh(1.0f);
        if (meshResult) {
            cubeMesh_ = *meshResult;
        } else {
            std::cerr << "Failed to create cube mesh\n";
            return false;
        }

        // Get default material
        material_ = graphics_->getDefaultPBRMaterial();

        // Setup camera - looking at origin from a corner
        bestow::Camera3D camera;
        camera.fovY = 45.0f;
        camera.nearPlane = 0.1f;
        camera.farPlane = 100.0f;
        camera.transform.position = {5.0f, 5.0f, 5.0f};

        // Point camera at origin (simplified - real code would calculate rotation)
        graphics_->setCamera(camera);

        // Setup lighting
        bestow::DirectionalLight light{
            .direction = {0.5f, -1.0f, 0.3f},
            .color = {1.0f, 1.0f, 1.0f},
            .intensity = 1.0f
        };
        graphics_->setDirectionalLight(light);
        graphics_->setAmbientLight({0.2f, 0.2f, 0.3f}, 0.3f);

        return true;
    }

    //=========================================================================
    // Game Loop
    //=========================================================================

    void gameLoop() {
        running_ = true;

        while (running_ && !graphics_->shouldClose()) {
            // Update input state
            input_->update();

            // Handle input
            handleInput();

            // Update game logic
            update();

            // Render
            render();
        }
    }

    void handleInput() {
        // ESC to quit
        if (input_->wasKeyJustPressed(GLFW_KEY_ESCAPE)) {
            running_ = false;
        }

        // Space to reset rotation
        if (input_->wasKeyJustPressed(GLFW_KEY_SPACE)) {
            rotation_ = 0.0f;
        }
    }

    void update() {
        // Rotate the cube
        rotation_ += 0.016f;  // ~1 radian per second at 60fps
    }

    void render() {
        graphics_->beginFrame();

        // Create rotation matrix
        bestow::Mat4 transform = glm::rotate(
            glm::identity<glm::mat4>(),
            rotation_,
            glm::vec3(0.0f, 1.0f, 0.0f)  // Rotate around Y axis
        );

        // Draw the cube
        graphics_->drawMesh(cubeMesh_, material_, transform);

        graphics_->endFrame();
    }

    //=========================================================================
    // Cleanup
    //=========================================================================

    void cleanup() {
        input_->shutdown();
        graphics_->shutdown();
    }

    //=========================================================================
    // Member Variables
    //=========================================================================

    // Injected dependencies (stored as pointers)
    bestow::IGraphics3DSystem* graphics_;
    bestow::IInputSystem* input_;

    // Game state
    bestow::MeshHandle cubeMesh_ = 0;
    bestow::MaterialHandle material_ = 0;
    float rotation_ = 0.0f;
    bool running_ = false;
};

}  // namespace mygame
```

## Step 3: Create the Entry Point

Create `src/main.cpp`:

```cpp
// src/main.cpp
// Entry point - configure the Engine and run your game

import std;
import bestow.core;       // Engine class
import bestow.services;   // Contract interfaces

// Import the implementations you want to use
// Each .impl module provides concrete implementations of contracts
import bestow.vulkan.impl;    // VulkanGraphics3DSystem
import bestow.input.impl;     // InputSystem
import bestow.events.impl;    // EventSystem (dependency of other systems)
import bestow.assets.impl;    // AssetSystem (dependency of other systems)
import bestow.config.impl;    // ConfigSystem
import bestow.shader.impl;    // OpenGLShaderSystem

// Import your game
import my.game;

int main() {
    //=========================================================================
    // Create the Engine
    //
    // The Engine is the "composition root" - it wires together all your
    // system implementations and manages their lifecycles.
    //=========================================================================

    bestow::core::Engine engine;

    //=========================================================================
    // Register System Implementations
    //
    // Use engine.use<IContract, Implementation>() to bind an implementation
    // to a contract interface. Order matters for dependencies!
    //=========================================================================

    // Core systems (no dependencies on other Bestow systems)
    engine.use<bestow::IEventSystem, bestow::EventSystem>();

    // Asset system (depends on EventSystem)
    engine.use<bestow::IAssetSystem, bestow::AssetSystem>();

    // Config system (depends on AssetSystem, EventSystem)
    engine.use<bestow::IConfigSystem, bestow::ConfigSystem>();

    // Shader system (depends on AssetSystem, EventSystem)
    engine.use<bestow::IShaderSystem, bestow::OpenGLShaderSystem>();

    // Graphics system (depends on AssetSystem, ShaderSystem, ConfigSystem)
    engine.use<bestow::IGraphics3DSystem, bestow::VulkanGraphics3DSystem>();

    // Input system (depends on AssetSystem)
    engine.use<bestow::IInputSystem, bestow::InputSystem>();

    //=========================================================================
    // Run Your Game
    //
    // The Engine reads the template parameters from your Application<> base
    // class and automatically injects the right systems into your constructor.
    //=========================================================================

    engine.run<mygame::MyGame>();

    return 0;
}
```

## Step 4: Create CMakeLists.txt

Create `CMakeLists.txt` in your project root:

```cmake
cmake_minimum_required(VERSION 3.28)
project(MyFirstGame CXX)

# Point to your Bestow installation
set(BESTOW_DIR "/path/to/bestow" CACHE PATH "Path to Bestow")
add_subdirectory(${BESTOW_DIR} bestow)

# Create executable
add_executable(my-first-game src/main.cpp)

# Add C++ module sources
target_sources(my-first-game
    PUBLIC FILE_SET CXX_MODULES FILES
        src/game.cppm
)

# Link required Bestow libraries
target_link_libraries(my-first-game
    PRIVATE
        bestow-contract   # Interface definitions
        bestow-core       # Engine class
        bestow-vulkan     # Vulkan graphics implementation
        bestow-input      # Input implementation
        bestow-events     # Event system implementation
        bestow-assets     # Asset system implementation
        bestow-config     # Config system implementation
        bestow-shader     # Shader system implementation
)

# Require C++23
target_compile_features(my-first-game PRIVATE cxx_std_23)

# Enable std module (required for import std;)
target_use_std_module(my-first-game)
```

## Step 5: Build and Run

```bash
# Configure (update BESTOW_DIR path!)
cmake -B build -DBESTOW_DIR=/path/to/bestow -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build

# Run
./build/my-first-game
```

You should see a window with a rotating cube!

## Understanding the Architecture

### The Application Pattern

```cpp
class MyGame : public Application<MyGame, IGraphics3DSystem, IInputSystem>
```

This line does several things:
1. Declares `MyGame` as an application
2. Lists dependencies (`IGraphics3DSystem`, `IInputSystem`)
3. Creates a `Dependencies` type alias that the Engine reads

### Dependency Injection Flow

```
1. You call engine.run<MyGame>()
2. Engine checks if MyGame has a Dependencies type (it does, from Application<>)
3. Engine resolves each dependency from the container
4. Engine calls MyGame constructor with resolved dependencies
5. Engine calls app.run()
```

### Why This Pattern?

- **Testability**: Pass mock implementations in tests
- **Flexibility**: Swap implementations (e.g., OpenGL vs Vulkan)
- **Clarity**: Dependencies are explicit in the class declaration
- **No globals**: Systems are injected, not accessed via singletons

## Next Steps

- Add more systems (Audio, Entity) to your Application
- Create entities with components
- Load assets (textures, models)
- See `games/game1/` for a complete example

## Common Issues

### "System not registered"
Make sure you call `engine.use<>()` for all systems your game depends on, including transitive dependencies.

### Constructor parameter mismatch
The constructor parameters must match the `Application<>` template arguments in the same order.

### Missing imports
Make sure to `import bestow.XYZ.impl` for each system implementation you use.
