# Bestow Game Template

This is a minimal starting point for a Bestow game.

## Quick Start

1. Copy this entire `template` directory to a new location
2. Rename the project in `CMakeLists.txt`
3. Implement your game in `src/game.cppm`
4. Build and run!

## Building

```bash
# Configure (adjust BESTOW_ROOT to point to your Bestow installation)
cmake -B build -DBESTOW_ROOT=/path/to/bestow -DBESTOW_BUILD_VULKAN=ON

# Build
cmake --build build

# Run
./build/my-game
```

## Structure

```
template/
  CMakeLists.txt    # Build configuration
  src/
    main.cpp        # Entry point - calls bestow::run<MyGame>()
    game.cppm       # Your game class - extend bestow::Game
  data/             # (optional) Game assets
```

## Game Class

Your game extends `bestow::Game` and overrides these methods:

- `onStart()` - Initialize meshes, load assets, setup camera
- `onUpdate(dt)` - Game logic (called at 60Hz)
- `onRender()` - Draw your game

## Systems Access

Access engine systems via convenience methods:

```cpp
graphics3d()  // 3D rendering
input()       // Keyboard, mouse, gamepad
entities()    // ECS system
audio()       // Sound
assets()      // Asset loading
```

## Controls

Default controls are Dvorak-friendly (,AOE = WASD positions):

| Dvorak | QWERTY | Action |
|--------|--------|--------|
| , | W | Up |
| O | S | Down |
| A | A | Left |
| E | D | Right |

Arrow keys also work.

## Example: Rotating Cube

```cpp
class MyGame : public bestow::Game {
    bestow::MeshHandle cube_;
    float angle_ = 0.0f;

    void onStart() override {
        cube_ = *graphics3d()->createCubeMesh(1.0f);
        // Setup camera and lighting...
    }

    void onUpdate(bestow::DeltaTime dt) override {
        angle_ += dt;
    }

    void onRender() override {
        auto transform = glm::rotate(glm::mat4(1.0f), angle_, {0,1,0});
        graphics3d()->drawMesh(cube_, graphics3d()->getDefaultPBRMaterial(), transform);
    }
};
```
