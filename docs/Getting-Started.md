# Getting Started with Bestow

Bestow is a modern C++23 game engine. This guide shows how to create a simple game.

## Prerequisites

- **CMake 3.28+** with C++23 module support
- **LLVM Clang 20+** (macOS) or **MSVC 19.38+** (Windows)
- **Vulkan SDK** installed
- **vcpkg** for dependencies

See `docs/Installation.md` for detailed setup.

## Quick Start

### 1. Create Your Game Class

```cpp
// src/MyGame.cppm
export module my.game;

import bestow.runtime;

export class MyGame : public bestow::Game {
public:
    void onStart() override {
        // Create a cube mesh
        auto result = graphics3d()->createCubeMesh(1.0f);
        if (result) {
            cubeMesh_ = *result;
        }

        // Get default material
        material_ = graphics3d()->getDefaultPBRMaterial();

        // Setup camera
        bestow::Camera3D cam;
        cam.position = {5.0f, 5.0f, 5.0f};
        cam.target = {0.0f, 0.0f, 0.0f};
        cam.up = {0.0f, 1.0f, 0.0f};
        cam.fov = 45.0f;
        graphics3d()->setCamera(cam);

        // Setup lighting
        graphics3d()->setDirectionalLight({
            .direction = {0.5f, -1.0f, 0.3f},
            .color = {1.0f, 1.0f, 1.0f},
            .intensity = 1.0f
        });
    }

    void onUpdate(bestow::DeltaTime dt) override {
        // Handle input (,AOE for Dvorak movement, arrow keys also work)
        if (input()->isKeyDown(GLFW_KEY_ESCAPE)) {
            quit();
        }

        // Rotate cube
        rotation_ += dt;
    }

    void onRender() override {
        // Draw the cube
        bestow::Mat4 transform = glm::rotate(
            glm::identity<glm::mat4>(),
            rotation_,
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        graphics3d()->drawMesh(cubeMesh_, material_, transform);
    }

private:
    bestow::MeshHandle cubeMesh_ = 0;
    bestow::MaterialHandle material_ = 0;
    float rotation_ = 0.0f;
};
```

### 2. Create Entry Point

```cpp
// src/main.cpp
import bestow.runtime;
import my.game;

int main() {
    return bestow::run<MyGame>({
        .title = "My First Game",
        .width = 1280,
        .height = 720,
        .vsync = true
    });
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
        src/MyGame.cppm
)

target_link_libraries(my-game PRIVATE bestow-runtime)
target_compile_features(my-game PRIVATE cxx_std_23)
target_use_std_module(my-game)
```

### 4. Build and Run

```bash
cmake --preset macos-debug  # or windows-debug, linux-debug
cmake --build --preset macos-debug
./build/macos-debug/my-game
```

## Game Lifecycle

Your game class overrides these methods:

| Method | When Called | Use For |
|--------|-------------|---------|
| `onStart()` | Once at startup | Create meshes, load assets, setup camera |
| `onUpdate(dt)` | Fixed 60Hz | Game logic, physics, input handling |
| `onRender()` | Every frame | Drawing meshes, sprites, UI |
| `onShutdown()` | At exit | Save state, cleanup |

## System Access

Access engine systems via convenience methods:

```cpp
graphics3d()  // 3D rendering (Vulkan)
input()       // Keyboard, mouse, gamepad
entities()    // ECS (EnTT-based)
audio()       // Sound (FMOD)
assets()      // Asset loading
events()      // Event bus
camera()      // Camera control
```

Or via `systems()`:

```cpp
systems().graphics3d->drawMesh(...);
systems().input->isKeyDown(...);
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

- Read `docs/api/` for system API reference
- Study `games/game1/` for a complete example
- Check `CLAUDE.md` for development guidelines
