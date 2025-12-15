# Bestow Game Framework

A modern C++23 game framework built with modularity, data-driven design, and performance in mind.

## Getting Started

**New to Bestow?** Start with the [Your First Application](tutorials/01-your-first-application.md) tutorial to learn how to build your first game in just 30 minutes.

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

Follow this recommended path to master Bestow:

| Step | Resource | Level | Description |
|------|----------|-------|-------------|
| 1 | [Your First Application](tutorials/01-your-first-application.md) | Beginner | Create a simple game with entities, rendering, and basic input |
| 2 | [Using Systems](tutorials/02-using-systems.md) | Intermediate | Deep dive into physics, audio, events, and advanced features |
| 3 | [Custom Systems](tutorials/03-custom-systems.md) | Advanced | Build your own systems and integrate them with Bestow |
| 4 | [Template Project](template/README.md) | Reference | Complete starter project demonstrating all systems |
| 5 | [API Reference](#api-reference) | Reference | Detailed documentation for each system |
| 6 | [Example Games](games/) | Study | Complete game implementations |

### Tutorials

**New to game development?** Start here! These tutorials build on each other and will take you from zero to creating your own games.

#### [01 - Your First Application](tutorials/01-your-first-application.md) (Beginner)
Learn the fundamentals:
- Setting up the EngineBuilder
- Creating and managing entities
- Adding sprites and rendering
- Handling keyboard input
- Building and running your first game

**Time:** 30-45 minutes | **Prerequisites:** Basic C++ knowledge

#### [02 - Using Systems](tutorials/02-using-systems.md) (Intermediate)
Master the built-in systems:
- Physics bodies and collision handling
- Playing sounds and music
- Event pub/sub patterns
- Asset loading and management
- Blueprint-based entity creation
- Camera control and effects

**Time:** 1-2 hours | **Prerequisites:** Complete Tutorial 01

#### [03 - Custom Systems](tutorials/03-custom-systems.md) (Advanced)
Extend Bestow with your own systems:
- Creating custom system interfaces
- Dependency injection with Kangaru
- Integrating third-party libraries
- Contract-based architecture
- System lifecycle management
- Testing and debugging custom systems

**Time:** 2-3 hours | **Prerequisites:** Complete Tutorial 02, solid C++ experience

### Example Games

Real-world examples are the best way to learn! Check out these complete game implementations:

- **[Game1](games/game1/)** - Full-featured platformer demonstrating all 15 systems
  - Complete with blueprints, levels, and gameplay abilities
  - Reference implementation for [engine documentation](games/game1/engine-docs/)
  - Includes AI, physics, audio, GAS, and more

### API Reference

Detailed technical documentation for each system:

- [Entity System](games/game1/engine-docs/ENTITY-SYSTEM.md) - ECS with EnTT
- [Physics System](games/game1/engine-docs/PHYSICS-SYSTEM.md) - Box2D integration
- [Graphics System](games/game1/engine-docs/GRAPHICS-SYSTEM.md) - 2D rendering
- [Input System](games/game1/engine-docs/INPUT-SYSTEM.md) - Keyboard, mouse, gamepad
- [Audio System](games/game1/engine-docs/AUDIO-SYSTEM.md) - FMOD integration
- [Events System](games/game1/engine-docs/EVENTS-SYSTEM.md) - Pub/sub messaging
- [Assets System](games/game1/engine-docs/ASSET-SYSTEM.md) - Resource management
- [Level System](games/game1/engine-docs/LEVEL-SYSTEM.md) - Lua-based levels
- [Blueprints System](games/game1/engine-docs/BLUEPRINTS-SYSTEM.md) - Entity templates
- [Camera System](games/game1/engine-docs/CAMERA-SYSTEM.md) - Camera control
- [GAS System](games/game1/engine-docs/GAS-SYSTEM.md) - Gameplay Ability System
- [AI System](games/game1/engine-docs/AI-SYSTEM.md) - Behavior trees
- [Save System](games/game1/engine-docs/SAVE-SYSTEM.md) - Serialization
- [Config System](games/game1/engine-docs/CONFIG-SYSTEM.md) - Configuration
- [UI System](games/game1/engine-docs/UI-SYSTEM.md) - User interface

### Architecture & Design

For contributors and advanced users:

- [Data-Driven Design Guide](docs/Data-Driven-Design.md) - What goes in Lua vs C++
- [Technical Design](docs/bestow-technical-design.md) - Architecture and design decisions
- [Project Status](docs/PROJECT-STATUS.md) - Implementation progress
- [System Implementation Guide](docs/SYSTEM-IMPLEMENTATION-GUIDE.md) - For contributors
- [Development Guidelines](CLAUDE.md) - Coding standards and workflow

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
