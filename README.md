# JFrame Game Framework

A modern C++23 game framework built with modularity, data-driven design, and performance in mind.

## Getting Started

**New to JFrame?** Start with the [Getting Started Guide](docs/Getting-Started.md) to learn how to build your first game.

## Documentation

### Core Documentation

- [Getting Started Guide](docs/Getting-Started.md) - Your first JFrame game
- [Data-Driven Design Guide](docs/Data-Driven-Design.md) - What goes in Lua vs C++
- [Technical Design](docs/jframe-technical-design.md) - Architecture and design decisions
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
