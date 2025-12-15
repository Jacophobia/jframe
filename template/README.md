# Bestow Game Template

This template demonstrates the proper Bestow architecture with **contract-based dependency injection**.

## Architecture Overview

Bestow follows a **contract-based architecture** where:

1. **Your game** inherits from `Application<YourGame, Dependencies...>` for automatic DI
2. **Systems** are registered explicitly in `main.cpp` - you choose the implementations
3. **All code** interacts through contract interfaces, not implementations
4. **Testing** is easy - just register mock implementations

This design gives you:
- Full control over which implementations to use
- Easy swapping of systems (Vulkan vs OpenGL, custom vs default)
- Simple testing with mocks
- Clear understanding of your game's dependencies

## Quick Start

1. Copy this entire `template` directory to a new location
2. Rename the project in `CMakeLists.txt`
3. Implement your game in `src/game.cppm`
4. Configure systems in `src/main.cpp`
5. Build and run!

## Building

```bash
# Configure (adjust BESTOW_ROOT to point to your Bestow installation)
cmake -B build -DBESTOW_ROOT=/path/to/bestow -DBESTOW_BUILD_VULKAN=ON

# Build
cmake --build build

# Run
./build/my-game
```

## Project Structure

```
template/
  CMakeLists.txt    # Build configuration and library linking
  src/
    main.cpp        # System registration and game launch
    game.cppm       # Your game application class
  data/             # (optional) Game assets
```

## The Two Key Files

### 1. `game.cppm` - Your Game Application

Your game inherits from `Application<>` with your dependencies as template parameters:

```cpp
import bestow.services;   // All contract interfaces + Application base

class MyGame : public bestow::Application<MyGame,
    bestow::IGraphics3DSystem,
    bestow::IInputSystem,
    bestow::IEntitySystem>
{
public:
    // Constructor params match base class template args
    MyGame(bestow::IGraphics3DSystem& graphics,
           bestow::IInputSystem& input,
           bestow::IEntitySystem& entities)
        : graphics_(&graphics)
        , input_(&input)
        , entities_(&entities) {}

    void run() override {
        // Initialize, game loop, cleanup
    }

private:
    bestow::IGraphics3DSystem* graphics_;
    bestow::IInputSystem* input_;
    bestow::IEntitySystem* entities_;
};
```

That's it! No Service boilerplate required.

### 2. `main.cpp` - System Registration

Wire up your systems using the Engine class:

```cpp
import bestow.core;              // Engine class
import bestow.services;          // Contract interfaces
import bestow.vulkan.impl;       // Vulkan implementation
import bestow.entity.impl;       // Entity implementation
import bestow.input.impl;        // Input implementation
import my.game;                  // Your game

int main() {
    bestow::core::Engine engine;

    // Register implementations for each contract
    engine.use<bestow::IEventSystem, bestow::EventSystem>();
    engine.use<bestow::IEntitySystem, bestow::EntitySystem>();
    engine.use<bestow::IAssetSystem, bestow::AssetSystem>();
    engine.use<bestow::IInputSystem, bestow::InputSystem>();
    engine.use<bestow::IGraphics3DSystem, bestow::VulkanGraphics3DSystem>();

    // Run your game - dependencies auto-detected from Application<> base!
    engine.run<MyGame>();

    return 0;
}
```

### 3. `CMakeLists.txt` - Library Linking

Link only the implementation libraries you actually use:

```cmake
target_link_libraries(my-game
    PRIVATE
        bestow-contract   # Interfaces
        bestow-core       # Engine and utilities
        bestow-entity     # Your chosen implementations
        bestow-input
        bestow-vulkan
)
```

## Engine API Reference

### Registration

```cpp
// Register an implementation for a contract
engine.use<IContract, Implementation>();

// Example
engine.use<IGraphics3DSystem, VulkanGraphics3DSystem>();
```

### System Access

```cpp
// Check if a system is registered (useful for optional systems)
if (engine.has<IAudioSystem>()) {
    // Audio is available
}

// Get a system (throws if not registered)
auto& graphics = engine.get<IGraphics3DSystem>();

// Try to get a system (returns nullptr if not registered)
auto* audio = engine.tryGet<IAudioSystem>();
```

### Running Your Application

```cpp
// Option 1: Application<> base class (RECOMMENDED)
// Dependencies auto-detected from base class template params
engine.run<MyGame>();

// Option 2: Explicit dependencies
engine.run<MyGame, IGraphics3DSystem, IInputSystem>();

// Option 3: Engine& pattern (for dynamic access)
class MyGame : public IApplication {
    MyGame(Engine& e) : graphics_(&e.get<IGraphics3DSystem>()) {}
};
engine.run<MyGame>();
```

## Using Custom Implementations

One of the key benefits of this architecture is easy system swapping.

### Example: Custom Entity System

```cpp
// 1. Create your implementation conforming to the contract
class MyCustomEntitySystem : public bestow::IEntitySystem {
public:
    bestow::Entity createEntity() override {
        // Your custom implementation
    }
    // ... implement all interface methods

    // Nested Service type for Engine registration
    struct Service : kgr::single_service<MyCustomEntitySystem>,
                     kgr::overrides<bestow::IEntitySystemService> {};
};

// 2. In main.cpp, register your implementation
engine.use<bestow::IEntitySystem, MyCustomEntitySystem>();
```

Your game code remains unchanged because it only uses `IEntitySystem&`.

## Testing with Mocks

```cpp
class MockEntitySystem : public bestow::IEntitySystem {
    // Mock implementation for testing
    struct Service : kgr::single_service<MockEntitySystem>,
                     kgr::overrides<bestow::IEntitySystemService> {};
};

// In your test
bestow::core::Engine testEngine;
testEngine.use<bestow::IEntitySystem, MockEntitySystem>();
// Register other systems...
testEngine.run<mygame::MyGame>();
// MyGame now receives the mock!
```

## Controls

Default controls are Dvorak-friendly (,AOE = WASD positions):

| Dvorak | QWERTY | Action |
|--------|--------|--------|
| , | W | Up |
| O | S | Down |
| A | A | Left |
| E | D | Right |
| ESC | ESC | Quit |

Arrow keys also work.

## Available Systems

Bestow provides these default implementations:

| Contract | Implementation | Library |
|----------|----------------|---------|
| `IEntitySystem` | `EntitySystem` | `bestow-entity` |
| `IEventSystem` | `EventSystem` | `bestow-events` |
| `IConfigSystem` | `ConfigSystem` | `bestow-config` |
| `IAssetSystem` | `AssetSystem` | `bestow-assets` |
| `IInputSystem` | `InputSystem` | `bestow-input` |
| `IAudioSystem` | `AudioSystem` | `bestow-audio` |
| `IGraphics3DSystem` | `VulkanGraphics3DSystem` | `bestow-vulkan` |
| `IShaderSystem` | `OpenGLShaderSystem` | `bestow-shader` |

Import the corresponding `.impl` module to access the implementation:
```cpp
import bestow.entity.impl;   // Gives you bestow::EntitySystem
import bestow.vulkan.impl;   // Gives you bestow::VulkanGraphics3DSystem
```
