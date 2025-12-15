# Bestow Game Framework

A modern C++23 game framework built with modularity, data-driven design, and performance in mind.

## Quick Start

**New to Bestow?** Start with the [Getting Started Guide](docs/Getting-Started.md) or the [Your First Application](tutorials/01-your-first-application.md) tutorial.

### Minimal Example

```cpp
// game.cppm - Your game class
class MyGame : public bestow::Application<MyGame,
    bestow::IGraphics3DSystem,
    bestow::IInputSystem>
{
public:
    MyGame(IGraphics3DSystem& g, IInputSystem& i) : graphics_(&g), input_(&i) {}
    void run() override { /* game loop */ }
};

// main.cpp - Wire up and run
int main() {
    bestow::core::Engine engine;
    engine.use<bestow::IEventSystem, bestow::EventSystem>();
    engine.use<bestow::IGraphics3DSystem, bestow::VulkanGraphics3DSystem>();
    engine.use<bestow::IInputSystem, bestow::InputSystem>();
    engine.run<MyGame>();  // Dependencies auto-injected!
}
```

---

## Start Your Own Game

The easiest way to start is to copy the **template project**:

```bash
# Copy the template
cp -r template/ ~/Projects/my-game
cd ~/Projects/my-game

# Edit CMakeLists.txt to set BESTOW_DIR path
# Then build!
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/my-game
```

### Template Structure

```
template/
├── CMakeLists.txt              # Build config (edit BESTOW_DIR path)
├── README.md                   # Detailed usage instructions
└── src/
    ├── main.cpp                # Engine setup and system registration
    └── game.cppm               # Your game class with DI
```

See [template/README.md](template/README.md) for detailed documentation.

---

## Documentation

### Learning Path

| Step | Resource | Description |
|------|----------|-------------|
| 1 | [Getting Started](docs/Getting-Started.md) | Quick overview and setup |
| 2 | [Your First Application](tutorials/01-your-first-application.md) | Step-by-step tutorial |
| 3 | [Template Project](template/README.md) | Reference implementation |
| 4 | [Example Games](games/) | Complete game implementations |

### Reference

- [Architecture](docs/Architecture.md) - System design overview
- [API Reference](docs/api/) - System API documentation
- [Technical Design](docs/bestow-technical-design.md) - Deep dive into architecture

### For Contributors

- [Development Guidelines](CLAUDE.md) - Coding standards and workflow
- [System Implementation Guide](docs/SYSTEM-IMPLEMENTATION-GUIDE.md) - Adding new systems
- [Project Status](docs/PROJECT-STATUS.md) - Implementation progress

---

## Core Concepts

### Contract-Based Architecture

Bestow uses **contracts** (interfaces) to decouple systems:

```cpp
// Your game only depends on contracts (interfaces)
class MyGame : public Application<MyGame, IGraphics3DSystem, IInputSystem> { ... };

// main.cpp chooses the implementations
engine.use<IGraphics3DSystem, VulkanGraphics3DSystem>();  // Could be OpenGL!
engine.use<IInputSystem, InputSystem>();
```

### Application Pattern

Inherit from `Application<YourGame, Dependencies...>`:

```cpp
class MyGame : public Application<MyGame, IGraphics3DSystem, IInputSystem, IAudioSystem>
{
public:
    // Constructor params match template args
    MyGame(IGraphics3DSystem& g, IInputSystem& i, IAudioSystem& a) { ... }
    void run() override { /* your game */ }
};

engine.run<MyGame>();  // Dependencies auto-detected and injected!
```

### Engine API

```cpp
// Register implementations
engine.use<IContract, Implementation>();

// Check availability (for optional systems)
if (engine.has<IAudioSystem>()) { ... }

// Get systems
auto& graphics = engine.get<IGraphics3DSystem>();
auto* audio = engine.tryGet<IAudioSystem>();  // nullptr if not registered
```

---

## Build

```bash
# Configure
cmake --preset macos-debug  # or windows-debug, linux-debug

# Build
cmake --build --preset macos-debug

# Test
ctest --preset macos-debug
```

## Requirements

- **C++23** compiler (Clang 20+, GCC 13+, MSVC 19.38+)
- **CMake 3.28+** (for C++ module support)
- **Vulkan SDK**
- **vcpkg** for dependencies

See [docs/Installation.md](docs/Installation.md) for detailed setup.

## Features

- Modern C++23 with modules (`import std;`)
- Contract-based dependency injection
- Entity Component System (EnTT)
- Vulkan 3D rendering (OpenGL fallback available)
- Data-driven design with Lua scripting
- 2D physics with Box2D
- Audio with FMOD
- Cross-platform (macOS, Windows, Linux)

## License

See LICENSE file for details.
