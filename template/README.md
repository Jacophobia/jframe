# Bestow Game Template

This template demonstrates the proper Bestow architecture with **contract-based dependency injection**.

## Architecture Overview

Bestow follows a **contract-based architecture** where:

1. **Your game** extends `IApplication` and receives systems via constructor injection
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

## The Three Key Files

### 1. `game.cppm` - Your Game Application

Your game extends `IApplication` and declares its dependencies in the constructor:

```cpp
class MyGame : public bestow::IApplication {
public:
    // Dependencies are injected via constructor
    explicit MyGame(
        bestow::IGraphics3DSystem* graphics = nullptr,
        bestow::IInputSystem* input = nullptr,
        bestow::IEntitySystem* entities = nullptr)
        : graphics_(graphics)
        , input_(input)
        , entities_(entities) {}

    void run() override {
        // Your game logic here
    }

private:
    bestow::IGraphics3DSystem* graphics_;
    bestow::IInputSystem* input_;
    bestow::IEntitySystem* entities_;
};

// Kangaru service definition - specifies how to construct MyGame
struct MyGameService : kgr::single_service<MyGame> {
    static auto construct(
        kgr::inject_t<bestow::IGraphics3DSystemService> graphics,
        kgr::inject_t<bestow::IInputSystemService> input,
        kgr::inject_t<bestow::IEntitySystemService> entities)
        -> kgr::inject_result<
            bestow::IGraphics3DSystem*,
            bestow::IInputSystem*,
            bestow::IEntitySystem*>
    {
        return kgr::inject(
            &graphics.service(),
            &input.service(),
            &entities.service()
        );
    }
};
```

### 2. `main.cpp` - System Registration

This is where you wire up your systems. You explicitly choose which implementations to use:

```cpp
#include <kangaru/kangaru.hpp>

import bestow.services;          // Contract interfaces
import bestow.vulkan.impl;       // Vulkan implementation
import bestow.entity.impl;       // Entity implementation
import bestow.input.impl;        // Input implementation
import my.game;                  // Your game

int main() {
    kgr::container container;

    // Register implementations for each contract
    container.service<bestow::EntitySystemService>();
    container.service<bestow::InputSystemService>();
    container.service<bestow::vulkan::VulkanGraphics3DSystemService>();

    // Run your game - dependencies are automatically injected
    auto& game = container.service<mygame::MyGameService>();
    game.run();

    return 0;
}
```

### 3. `CMakeLists.txt` - Library Linking

Link only the implementation libraries you actually use:

```cmake
target_link_libraries(my-game
    PRIVATE
        bestow-contract   # Interfaces
        bestow-core       # Utilities
        bestow-entity     # Your chosen implementations
        bestow-input
        bestow-vulkan
)
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
};

// 2. Create a Kangaru service definition
struct MyCustomEntitySystemService
    : kgr::single_service<MyCustomEntitySystem>
    , kgr::overrides<bestow::IEntitySystemService> {};

// 3. In main.cpp, register your implementation instead
container.service<MyCustomEntitySystemService>();  // Instead of bestow::EntitySystemService
```

Your game code remains unchanged because it only uses `IEntitySystem*`.

## Testing with Mocks

```cpp
class MockEntitySystem : public bestow::IEntitySystem {
    // Mock implementation for testing
};

struct MockEntitySystemService
    : kgr::single_service<MockEntitySystem>
    , kgr::overrides<bestow::IEntitySystemService> {};

// In your test
kgr::container testContainer;
testContainer.service<MockEntitySystemService>();
testContainer.service<mygame::MyGameService>();
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
| `IEntitySystem` | `EntitySystemService` | `bestow-entity` |
| `IEventSystem` | `EventSystemService` | `bestow-events` |
| `IConfigSystem` | `ConfigSystemService` | `bestow-config` |
| `IAssetSystem` | `AssetSystemService` | `bestow-assets` |
| `IInputSystem` | `InputSystemService` | `bestow-input` |
| `IAudioSystem` | `AudioSystemService` | `bestow-audio` |
| `IGraphics3DSystem` | `vulkan::VulkanGraphics3DSystemService` | `bestow-vulkan` |
| `IShaderSystem` | `ShaderSystemService` | `bestow-shader` |

Import the corresponding `.impl` module to access the service:
```cpp
import bestow.entity.impl;   // Gives you bestow::EntitySystemService
import bestow.vulkan.impl;   // Gives you bestow::vulkan::VulkanGraphics3DSystemService
```
