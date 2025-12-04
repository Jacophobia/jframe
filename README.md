# Bestow Game Framework

A modern C++23 game framework built with modularity, data-driven design, and performance in mind.

## Getting Started

**New to Bestow?** Start with the [Getting Started Guide](docs/Getting-Started.md) to learn how to build your first game.

---

## Start Your Own Game

The easiest way to start a new Bestow game is to copy the **template project**:

```bash
# Copy the template to your projects directory
cp -r template/ ~/Projects/my-game
cd ~/Projects/my-game

# Edit CMakeLists.txt to point BESTOW_DIR to your Bestow installation
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
├── src/
│   ├── main.cpp                # Entry point with EngineBuilder
│   ├── Game.cpp                # Complete game using ALL 15 systems
│   └── Game.h                  # Game class header
└── data/
    ├── blueprints/entities.lua # Entity definitions (player, platforms)
    ├── levels/main.lua         # Level layout with Lua scripting
    ├── config/game.lua         # Game settings
    ├── config/audio.lua        # Audio settings
    └── input/bindings.lua      # Input mappings reference
```

The template demonstrates every Bestow system: entities, physics, graphics, input, audio, events, assets, levels, camera, GAS (abilities), and blueprints. Delete what you don't need!

---

## Documentation

### Learning Path

| Step | Resource | Description |
|------|----------|-------------|
| 1 | [Getting Started](docs/Getting-Started.md) | Installation, build setup, your first game |
| 2 | [Template README](template/README.md) | How to use the starter template |
| 3 | [Tutorials](docs/tutorials/) | Step-by-step guides for each system |
| 4 | [API Reference](docs/api/) | Detailed API documentation |
| 5 | [Examples](examples/) | Complete example games to study |

### Tutorials

| Tutorial | Topics Covered |
|----------|----------------|
| [01 - Your First Game](docs/tutorials/01-Your-First-Game.md) | Engine setup, entities, basic rendering |
| [02 - Blueprints & Levels](docs/tutorials/02-Blueprints-And-Levels.md) | Lua data files, entity templates, level design |
| [03 - Physics & Collision](docs/tutorials/03-Physics-And-Collision.md) | Box2D bodies, collision events, raycasting |
| [04 - Input & Controls](docs/tutorials/04-Input-And-Controls.md) | Keyboard, mouse, gamepad, action mapping |
| [05 - Audio](docs/tutorials/05-Audio.md) | Sound effects, music, positional audio |

### API Reference

Full API documentation is available in [`docs/api/`](docs/api/):

- [EngineBuilder](docs/api/EngineBuilder.md) - Engine initialization
- [Entity System](docs/api/Entity.md) - ECS operations
- [Physics](docs/api/Physics.md) - Box2D integration
- [Graphics](docs/api/Graphics.md) - Rendering
- [Input](docs/api/Input.md) - Input handling
- [Audio](docs/api/Audio.md) - Sound playback
- [And more...](docs/api/README.md)

### Architecture & Design

- [Data-Driven Design Guide](docs/Data-Driven-Design.md) - What goes in Lua vs C++
- [Technical Design](docs/bestow-technical-design.md) - Architecture and design decisions
- [Project Status](docs/PROJECT-STATUS.md) - Implementation progress
- [System Implementation Guide](docs/SYSTEM-IMPLEMENTATION-GUIDE.md) - For contributors

### System Guides

| System | Description |
|--------|-------------|
| [Entity System](docs/systems/Entity-System.md) | ECS-based entity management with EnTT |
| [Events System](docs/systems/Events-System.md) | Type-safe publish/subscribe events |
| [Input System](docs/systems/Input-System.md) | Keyboard, mouse, and gamepad input |
| [Graphics System](docs/systems/Graphics-System.md) | 2D rendering with sprite batching |
| [Physics System](docs/systems/Physics-System.md) | Box2D integration for 2D physics |
| [Audio System](docs/systems/Audio-System.md) | FMOD-powered audio playback |
| [Assets System](docs/systems/Assets-System.md) | Async asset loading and management |
| [Save System](docs/systems/Save-System.md) | Profile and save file management |
| [Level System](docs/systems/Level-System.md) | Lua-based level definitions |
| [AI System](docs/systems/AI-System.md) | Behavior trees and pathfinding |
| [Camera System](docs/systems/Camera-System.md) | Camera management and effects |
| [GAS System](docs/systems/GAS-System.md) | Gameplay Ability System for abilities, effects, attributes |

## Quick Build

```bash
# Configure
cmake --preset macos-debug  # or windows-debug, linux-debug

# Build
cmake --build --preset macos-debug

# Test
ctest --preset macos-debug
```

## Requirements

- **C++23** compiler (Clang 17+, GCC 13+, MSVC 19.38+)
- **CMake 3.28+** (for C++ module support)
- vcpkg for dependencies

## Features

- Modern C++23 with modules
- Entity Component System (EnTT)
- Data-driven design with Lua scripting
- 2D physics with Box2D
- Audio with FMOD
- Cross-platform (macOS, Windows, Linux)

## License

See LICENSE file for details.
