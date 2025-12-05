# Bestow Technical Design Document

> **Version:** 1.0.0  
> **Last Updated:** 2025  
> **Status:** Draft

---

## Table of Contents

1. [Overview](#1-overview)
2. [Development Environment](#2-development-environment)
3. [Third-Party Dependencies](#3-third-party-dependencies)
4. [Project Structure](#4-project-structure)
5. [Core Architecture](#5-core-architecture)
6. [Development Workflow](#6-development-workflow)
7. [Contracts & Interfaces](#7-contracts--interfaces)
8. [System Overviews](#8-system-overviews)
9. [Data Formats](#9-data-formats)
10. [Build Configuration](#10-build-configuration)
11. [Feature Roadmap](#11-feature-roadmap)

---

## 1. Overview

### 1.1 What is Bestow?

Bestow is a modular, interface-driven 2D game framework written in modern C++. It emphasizes:

- **Separation of Concerns** — Each system is independently developed and compiled
- **Data-Driven Design** — Entities, levels, and behaviors defined in JSON
- **Testability** — Interface-based architecture enables easy mocking
- **Production Readiness** — Versioned saves, profiling, hot-reload support
- **Platform Portability** — Designed for future Vulkan migration and Nintendo Switch deployment

### 1.2 Design Philosophy

| Principle | Implementation |
|-----------|----------------|
| Program to interfaces | All systems implement abstract interfaces from `bestow-contract` |
| Composition over inheritance | ECS architecture with EnTT |
| Data-driven configuration | Blueprints, levels, settings in JSON |
| Fail-safe persistence | Checksummed saves with backup recovery |
| Minimal coupling | Dependency injection via Fruit |

### 1.3 Target Platforms

| Platform | Status | Notes |
|----------|--------|-------|
| macOS | Primary Development | Apple Silicon + Intel |
| Windows | Primary Testing/Release | Windows 10/11 |
| Linux | Supported | Ubuntu 22.04+ |
| Nintendo Switch | Planned | Requires Vulkan migration |

---

## 2. Development Environment

### 2.1 Language Standard

```
C++23
```

**Required C++23 Features Used:**
- `std::expected` — Error handling without exceptions
- `std::ranges` — Modern iteration
- `std::format` — String formatting (via fmt fallback)
- `std::span` — Non-owning views
- Deducing `this` — CRTP simplification
- `if constexpr` with lambdas
- `std::unreachable`

### 2.2 Compilers

| Platform | Compiler | Minimum Version |
|----------|----------|-----------------|
| macOS | Apple Clang | 15.0+ (Xcode 15+) |
| macOS | LLVM Clang | 17.0+ |
| Windows | MSVC | 19.38+ (VS 2022 17.8+) |
| Windows | Clang-CL | 17.0+ |
| Linux | GCC | 13.0+ |
| Linux | Clang | 17.0+ |

**Recommended:** Clang 17+ for consistency across platforms.

### 2.3 Build System

```
CMake 3.28+
```

**Why CMake 3.28+:**
- Full C++20/23 module support (`CMAKE_CXX_SCAN_FOR_MODULES`)
- `FILE_SET CXX_MODULES` for declaring module sources
- Automatic module dependency scanning
- Improved presets
- Better cross-platform module handling

### 2.4 Package Manager

```
vcpkg (manifest mode)
```

**Configuration Files:**
- `vcpkg.json` — Dependency manifest
- `vcpkg-configuration.json` — Registry and baseline configuration

**Alternative:** Conan 2.0 is acceptable if preferred.

### 2.5 IDE Recommendations

| Platform | Recommended IDE | Notes |
|----------|-----------------|-------|
| macOS | CLion | Best CMake integration |
| macOS | VS Code + clangd | Lightweight alternative |
| Windows | Visual Studio 2022 | Native debugging |
| Windows | CLion | Cross-platform consistency |

### 2.6 Development Tools

| Tool | Purpose | Required |
|------|---------|----------|
| clang-format | Code formatting | Yes |
| clang-tidy | Static analysis | Yes |
| Tracy | Profiling | Recommended |
| RenderDoc | Graphics debugging | For Vulkan |
| msdf-atlas-gen | Font atlas generation | Build tool |
| tiled-to-lua | Convert Tiled JSON → Lua levels | Planned |

---

## 3. Third-Party Dependencies

### 3.1 Dependency Matrix

| Library | Version | Category | License | Header-Only | Notes |
|---------|---------|----------|---------|-------------|-------|
| **GLFW** | 3.3+ | Windowing/Input | zlib | No | Vulkan-ready, keyboard/mouse |
| **SDL2** | 2.28+ | Game Controllers | zlib | No | Controller database (500+ devices) |
| **glm** | 0.9.9+ | Math | MIT | Yes | GLSL-compatible |
| **spdlog** | 1.12+ | Logging | MIT | Optional | Includes fmt |
| **fmt** | 10.0+ | Formatting | MIT | Optional | Bundled with spdlog |
| **Lua** | 5.4+ | Scripting | MIT | No | Data definitions, config |
| **sol2** | 3.3+ | Lua C++ Bindings | MIT | Yes | Modern C++ Lua wrapper |
| **nlohmann/json** | 3.11+ | JSON | MIT | Yes | Settings, save files only |
| **cereal** | 1.3+ | Binary Serialization | BSD-3 | Yes | Versioning support |
| **EnTT** | 3.12+ | ECS | MIT | Yes | |
| **Box2D** | 3.0+ | Physics | MIT | No | |
| **FMOD Core** | 2.02+ | Audio | Commercial | No | Free < $200k revenue |
| **stb_image** | 2.28+ | Image Loading | Public Domain | Yes | Single header |
| **stb_truetype** | 1.26+ | Font Loading | Public Domain | Yes | Single header |
| **FreeType** | 2.13+ | Font Rendering | FTL | No | For MSDF generation |
| **zstd** | 1.5+ | Compression | BSD | No | |
| **Google Test** | 1.14+ | Testing | BSD-3 | No | |
| **Dear ImGui** | 1.89+ | Debug UI | MIT | Yes | |
| **Tracy** | 0.10+ | Profiling | BSD-3 | No | Optional |
| **taskflow** | 3.6+ | Parallelism | MIT | Yes | |
| **Recast/Detour** | 1.6+ | Navigation | zlib | No | |
| **BehaviorTree.CPP** | 4.0+ | AI | MIT | No | |
| **Fruit** | 3.7+ | DI | Apache-2.0 | No | |
| **efsw** | 1.4+ | File Watching | MIT | No | Hot reload support (dev only) |

### 3.2 Dependency Graph

```
┌─────────────────────────────────────────────────────────────────────────┐
│                              Application                                 │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                            bestow-contract                               │
│                         (Module Interfaces)                              │
│                                                                          │
│  Dependencies: glm, EnTT (types only)                                   │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
            ┌───────────────────────┼───────────────────────┐
            ▼                       ▼                       ▼
    ┌───────────────┐       ┌───────────────┐       ┌───────────────┐
    │ bestow-entity │       │bestow-graphics│       │ bestow-audio  │
    │               │       │               │       │               │
    │ EnTT          │       │ GLFW          │       │ FMOD          │
    └───────────────┘       │ stb_image     │       └───────────────┘
                            │ FreeType      │
            ┌───────────────┤ ImGui         │───────────────┐
            ▼               └───────────────┘               ▼
    ┌───────────────┐                               ┌───────────────┐
    │ bestow-input  │                               │ bestow-physics│
    │               │                               │               │
    │ GLFW (kb/mouse)                               │ Box2D         │
    │ SDL2 (controllers)                            │               │
    └───────────────┘                               └───────────────┘
            │                                               │
            ▼                                               ▼
    ┌───────────────┐       ┌───────────────┐       ┌───────────────┐
    │ bestow-assets │       │ bestow-level  │       │  bestow-ai    │
    │               │       │               │       │               │
    │ stb_image     │       │ Lua           │       │ BT.CPP        │
    │ zstd          │       │ sol2          │       │ Recast/Detour │
    └───────────────┘       └───────────────┘       └───────────────┘
            │                       │
            ▼                       ▼
    ┌───────────────┐       ┌───────────────┐       ┌───────────────┐
    │ bestow-save   │       │ bestow-events │       │  bestow-dev   │
    │               │       │               │       │  (dev only)   │
    │ cereal        │       │ (no external) │       │ efsw          │
    │ zstd          │       │               │       │ ImGui         │
    │ nlohmann/json │       │               │       │               │
    └───────────────┘       └───────────────┘       └───────────────┘

Shared across all systems:
- spdlog/fmt (logging)
- glm (math)
- Tracy (profiling, optional)
- taskflow (parallelism)
- Fruit (DI, in composition root only)

Data Format Usage:
- Lua: Blueprints, Levels, Game Config (programmable, hot-reloadable)
- JSON: Settings, Save files only (simple key-value, no computation needed)
- Binary (cereal): Save game data (fast, compact)
```

### 3.3 FMOD Licensing Notes

FMOD Core is free for:
- Development and prototyping
- Games with total budget < $200k USD

Above $200k budget requires a commercial license. See: https://fmod.com/licensing

**Alternative (if licensing is a concern):** SoLoud (MIT) with reduced features.

### 3.4 SDL2 GameController Database

SDL2 includes a built-in database of controller mappings, but it's recommended to include the community-maintained database for maximum compatibility:

**Repository:** https://github.com/gabomdq/SDL_GameControllerDB

**Setup:**
1. Download `gamecontrollerdb.txt` from the repository
2. Place in `data/config/gamecontrollerdb.txt`
3. Load at startup:

```cpp
SDL_GameControllerAddMappingsFromFile("data/config/gamecontrollerdb.txt");
```

**Supported Controllers (500+):**
- Xbox 360, Xbox One, Xbox Series X|S
- PlayStation 3, 4, 5 (DualShock, DualSense)
- Nintendo Switch Pro Controller, Joy-Cons
- 8BitDo controllers (all models)
- Steam Controller
- Stadia Controller
- Generic DirectInput controllers
- And many more...

**Adding Custom Mappings:**

If a user's controller isn't recognized, they can generate a mapping using tools like:
- [SDL2 Gamepad Tool](https://generalarcade.com/gamepadtool/)
- Steam's controller configuration

### 3.5 vcpkg.json

```json
{
    "$schema": "https://raw.githubusercontent.com/microsoft/vcpkg-tool/main/docs/vcpkg.schema.json",
    "name": "bestow",
    "version": "1.0.0",
    "description": "Bestow Game Framework",
    "homepage": "https://github.com/yourname/bestow",
    "license": "MIT",
    "supports": "!(uwp | arm | android | ios)",
    "dependencies": [
        "glfw3",
        "sdl2",
        "glm",
        "spdlog",
        "fmt",
        "lua",
        "sol2",
        "nlohmann-json",
        "cereal",
        "entt",
        "box2d",
        "freetype",
        "zstd",
        "gtest",
        "imgui",
        "taskflow",
        "recastnavigation",
        "behaviortree-cpp",
        "efsw"
    ],
    "overrides": [
        { "name": "box2d", "version": "3.0.0" }
    ],
    "builtin-baseline": "2024.01.12"
}
```

**Note:** 
- FMOD and Tracy require manual setup (not available in vcpkg)
- Download `gamecontrollerdb.txt` from [SDL_GameControllerDB](https://github.com/gabomdq/SDL_GameControllerDB) for maximum controller compatibility

---

## 4. Project Structure

### 4.1 C++ Modules

Bestow uses C++23 modules for faster compilation and cleaner dependencies.

**Module Naming Convention:**
```
bestow.types          // Core types and aliases
bestow.entity         // IEntitySystem interface
bestow.graphics       // IGraphicsSystem interface
bestow.audio          // IAudioSystem interface
bestow.input          // IInputSystem interface
bestow.assets         // IAssetSystem interface
bestow.save           // ISaveSystem interface
bestow.level          // ILevelSystem interface
bestow.events         // IEventSystem interface
bestow.physics        // IPhysicsSystem interface
bestow.ai             // IAISystem interface
bestow                // Primary module (re-exports all interfaces)
```

**File Extensions:**
| Compiler | Interface Unit | Implementation Unit |
|----------|----------------|---------------------|
| Clang | `.cppm` | `.cpp` |
| MSVC | `.ixx` | `.cpp` |
| GCC | `.cppm` | `.cpp` |

CMake handles the extension mapping automatically.

### 4.2 Repository Layout

```
bestow/
├── .github/
│   └── workflows/
│       ├── ci.yml                    # CI pipeline
│       └── release.yml               # Release automation
│
├── cmake/
│   ├── BestowConfig.cmake           # Find module for consumers
│   ├── CompilerWarnings.cmake       # Warning flags
│   ├── StaticAnalysis.cmake         # clang-tidy setup
│   └── Dependencies.cmake           # Dependency fetching
│
├── external/                         # Non-vcpkg dependencies
│   ├── fmod/
│   │   ├── include/
│   │   └── lib/
│   │       ├── macos/
│   │       ├── windows/
│   │       └── linux/
│   ├── tracy/                        # Git submodule
│   └── stb/
│       ├── stb_image.h
│       └── stb_truetype.h
│
├── bestow-contract/                  # Interface library (C++ modules)
│   ├── CMakeLists.txt
│   └── src/
│       ├── bestow.cppm               # Primary module interface
│       ├── bestow.types.cppm         # Core types
│       ├── bestow.entity.cppm        # IEntitySystem
│       ├── bestow.graphics.cppm      # IGraphicsSystem
│       ├── bestow.audio.cppm         # IAudioSystem
│       ├── bestow.input.cppm         # IInputSystem
│       ├── bestow.assets.cppm        # IAssetSystem
│       ├── bestow.save.cppm          # ISaveSystem
│       ├── bestow.level.cppm         # ILevelSystem
│       ├── bestow.events.cppm        # IEventSystem
│       ├── bestow.physics.cppm       # IPhysicsSystem
│       └── bestow.ai.cppm            # IAISystem
│
├── bestow-entity/                    # Entity system implementation
│   ├── CMakeLists.txt
│   └── src/
│       ├── bestow.entity.impl.cppm   # Module implementation
│       └── EntitySystem.cpp          # Implementation details
│
├── bestow-graphics/                  # Graphics implementation
│   ├── CMakeLists.txt
│   ├── src/
│   │   ├── bestow.graphics.impl.cppm
│   │   ├── GraphicsSystem.cpp
│   │   ├── Renderer.cpp
│   │   ├── SpriteRenderer.cpp
│   │   ├── TextRenderer.cpp
│   │   └── DebugRenderer.cpp
│   └── shaders/
│       ├── sprite.vert
│       ├── sprite.frag
│       ├── msdf_text.vert
│       └── msdf_text.frag
│
├── bestow-audio/                     # Audio implementation
│   ├── CMakeLists.txt
│   └── src/
│       ├── bestow.audio.impl.cppm
│       └── FMODAudioSystem.cpp
│
├── bestow-input/                     # Input implementation
│   ├── CMakeLists.txt
│   └── src/
│       ├── bestow.input.impl.cppm
│       └── InputSystem.cpp
│
├── bestow-assets/                    # Asset management
│   ├── CMakeLists.txt
│   └── src/
│       ├── bestow.assets.impl.cppm
│       ├── AssetSystem.cpp
│       └── loaders/
│           ├── TextureLoader.cpp
│           ├── SoundLoader.cpp
│           └── FontLoader.cpp
│
├── bestow-save/                      # Save system
│   ├── CMakeLists.txt
│   └── src/
│       ├── bestow.save.impl.cppm
│       ├── SaveSystem.cpp
│       ├── SaveSerializer.cpp
│       ├── SaveMigrator.cpp
│       └── ProfileManager.cpp
│
├── bestow-level/                     # Level management
│   ├── CMakeLists.txt
│   └── src/
│       ├── bestow.level.impl.cppm
│       ├── LevelSystem.cpp
│       └── LevelLoader.cpp
│
├── bestow-events/                    # Event system
│   ├── CMakeLists.txt
│   └── src/
│       ├── bestow.events.impl.cppm
│       └── EventSystem.cpp
│
├── bestow-physics/                   # Physics implementation
│   ├── CMakeLists.txt
│   └── src/
│       ├── bestow.physics.impl.cppm
│       └── Box2DPhysicsSystem.cpp
│
├── bestow-ai/                        # AI systems
│   ├── CMakeLists.txt
│   └── src/
│       ├── bestow.ai.impl.cppm
│       ├── AISystem.cpp
│       ├── BehaviorTreeManager.cpp
│       └── NavigationSystem.cpp
│
├── bestow-core/                      # Core utilities
│   ├── CMakeLists.txt
│   └── src/
│       ├── bestow.core.cppm
│       ├── Application.cpp
│       ├── JobSystem.cpp
│       ├── Easing.cpp
│       └── Timer.cpp
│
├── bestow-dev/                       # Development tools (debug only)
│   ├── CMakeLists.txt
│   └── src/
│       ├── bestow.dev.cppm
│       ├── HotReloadManager.cpp
│       ├── DevOverlay.cpp
│       └── EntityInspector.cpp
│
├── tools/                            # Build tools
│   ├── msdf-atlas-gen/              # Font atlas generator
│   ├── asset-packer/                # Asset packaging tool
│   └── tiled-to-lua/                # [TODO] Convert Tiled JSON → Lua levels
│
├── tests/                            # Test suite
│   ├── CMakeLists.txt
│   ├── unit/
│   │   ├── EntitySystemTests.cpp
│   │   ├── SaveSystemTests.cpp
│   │   ├── BlueprintTests.cpp
│   │   └── ...
│   ├── integration/
│   │   └── ...
│   └── mocks/
│       ├── MockAssetSystem.cppm
│       ├── MockAudioSystem.cppm
│       └── ...
│
├── docs/                             # Documentation
│   ├── architecture.md
│   ├── getting-started.md
│   ├── development-workflow.md
│   ├── systems/
│   │   ├── entity.md
│   │   ├── graphics.md
│   │   └── ...
│   └── api/                          # Generated API docs
│
├── examples/                         # Example projects
│   └── platformer/
│       ├── CMakeLists.txt
│       ├── src/
│       │   ├── main.cpp
│       │   ├── Game.cppm
│       │   ├── injection.cppm
│       │   ├── components/
│       │   │   └── Components.cppm
│       │   └── systems/
│       │       ├── PlayerMovementSystem.cppm
│       │       ├── PlayerMovementSystem.cpp
│       │       └── ...
│       └── data/
│           ├── blueprints/
│           │   ├── player.lua
│           │   ├── helpers.lua              # Shared blueprint utilities
│           │   └── enemies/
│           │       └── slime.lua
│           ├── levels/
│           │   ├── helpers.lua              # Shared level utilities
│           │   └── world1/
│           │       └── level1.lua
│           ├── textures/
│           ├── audio/
│           └── fonts/
│
├── .clang-format                     # Code style
├── .clang-tidy                       # Static analysis config
├── .gitignore
├── .gitmodules                       # Tracy submodule
├── CMakeLists.txt                    # Root CMake
├── CMakePresets.json                 # Build presets
├── vcpkg.json                        # vcpkg manifest
├── vcpkg-configuration.json
├── LICENSE
└── README.md
```

### 4.2 Game Project Structure (Consumer)

```
my-game/
├── CMakeLists.txt
├── vcpkg.json
├── src/
│   ├── main.cpp
│   ├── injection.hpp                 # DI wiring
│   ├── Game.hpp
│   ├── Game.cpp
│   ├── components/
│   │   └── Components.hpp            # Game-specific components
│   ├── systems/
│   │   ├── IGameSystem.hpp
│   │   ├── GameSystemManager.hpp
│   │   ├── PlayerMovementSystem.hpp
│   │   ├── PlayerMovementSystem.cpp
│   │   ├── CameraFollowSystem.hpp
│   │   ├── CameraFollowSystem.cpp
│   │   ├── CollectibleSystem.hpp
│   │   ├── CollectibleSystem.cpp
│   │   └── ...
│   ├── data/
│   │   ├── BlueprintRegistry.hpp
│   │   ├── BlueprintRegistry.cpp
│   │   ├── EntityFactory.hpp
│   │   ├── EntityFactory.cpp
│   │   ├── LevelLoader.hpp
│   │   └── LevelLoader.cpp
│   ├── persistence/
│   │   ├── SettingsManager.hpp
│   │   ├── SettingsManager.cpp
│   │   ├── GameStateManager.hpp
│   │   └── GameStateManager.cpp
│   └── ui/
│       └── ...
│
├── data/
│   ├── blueprints/
│   │   ├── helpers.lua               # Shared utilities (component builders, etc.)
│   │   ├── player.lua
│   │   ├── enemies/
│   │   │   ├── base_enemy.lua        # Base enemy template
│   │   │   ├── slime.lua
│   │   │   └── bat.lua
│   │   ├── collectibles/
│   │   │   ├── coin.lua
│   │   │   └── health_pickup.lua
│   │   └── interactables/
│   │       ├── door.lua
│   │       └── chest.lua
│   ├── levels/
│   │   ├── helpers.lua               # Level utilities (patterns, spawners)
│   │   ├── world1/
│   │   │   ├── level1.lua
│   │   │   └── level2.lua
│   │   └── world2/
│   │       └── ...
│   ├── textures/
│   │   ├── player.png
│   │   ├── enemies/
│   │   ├── tilesets/
│   │   └── ui/
│   ├── audio/
│   │   ├── music/
│   │   └── sfx/
│   ├── fonts/
│   │   └── main.ttf
│   ├── ai/
│   │   └── behaviors/
│   │       └── slime_behavior.xml
│   └── config/
│       ├── game.lua                  # Game constants and settings
│       ├── settings.json             # User settings (runtime, saved by game)
│       └── gamecontrollerdb.txt      # SDL2 controller mappings
│
├── saves/                            # Runtime (gitignored)
│   └── profiles/
│       └── ...
│
└── tests/
    └── ...
```

---

## 5. Core Architecture

### 5.1 Architectural Layers

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           GAME LAYER                                     │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐    │
│  │    Game     │  │   Game      │  │  Blueprint  │  │    Game     │    │
│  │   Logic     │  │  Systems    │  │   Factory   │  │   State     │    │
│  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘    │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    │ uses interfaces
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                         CONTRACT LAYER                                   │
│                       (bestow-contract)                                  │
│                                                                          │
│   IEntitySystem  IGraphicsSystem  IAudioSystem  IInputSystem            │
│   IAssetSystem   ISaveSystem      ILevelSystem  IEventSystem            │
│   IPhysicsSystem IAISystem                                              │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    │ implemented by
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                      IMPLEMENTATION LAYER                                │
│                                                                          │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌──────────────┐   │
│  │bestow-entity │ │bestow-graphics│ │ bestow-audio │ │ bestow-input │   │
│  │              │ │              │ │              │ │              │   │
│  │   EnTT       │ │ GLFW/OpenGL  │ │    FMOD      │ │    GLFW      │   │
│  └──────────────┘ └──────────────┘ └──────────────┘ └──────────────┘   │
│                                                                          │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌──────────────┐   │
│  │bestow-assets │ │ bestow-save  │ │ bestow-level │ │bestow-physics│   │
│  │              │ │              │ │              │ │              │   │
│  │stb/zstd      │ │cereal/zstd   │ │nlohmann/json │ │   Box2D      │   │
│  └──────────────┘ └──────────────┘ └──────────────┘ └──────────────┘   │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    │ wired by
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                      COMPOSITION ROOT                                    │
│                        (Fruit DI)                                        │
│                                                                          │
│                   getEngineComponent() → BestowEngine                    │
└─────────────────────────────────────────────────────────────────────────┘
```

### 5.2 Engine Composition

```cpp
// The engine is a simple aggregate of all system interfaces
struct BestowEngine {
    IEventSystem*    events;
    IAssetSystem*    assets;
    IEntitySystem*   entities;
    IGraphicsSystem* graphics;
    IAudioSystem*    audio;
    IInputSystem*    input;
    IPhysicsSystem*  physics;
    ILevelSystem*    levels;
    ISaveSystem*     save;
    IAISystem*       ai;
};
```

### 5.3 System Lifecycle

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         FRAME LIFECYCLE                                  │
└─────────────────────────────────────────────────────────────────────────┘

    ┌──────────────────────────────────────────────────────────────────┐
    │                          FRAME START                              │
    │                     Calculate DeltaTime                          │
    └──────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
    ┌──────────────────────────────────────────────────────────────────┐
    │                         INPUT PHASE                               │
    │                                                                   │
    │   input->update()     Poll GLFW, update action states            │
    └──────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
    ┌──────────────────────────────────────────────────────────────────┐
    │                       GAME LOGIC PHASE                            │
    │                                                                   │
    │   Game systems execute in priority order:                        │
    │   1. PlayerMovement (priority 10)                                │
    │   2. AI (priority 20)                                            │
    │   3. Combat (priority 30)                                        │
    │   4. Collectibles (priority 50)                                  │
    │   5. Animation (priority 80)                                     │
    │   6. Camera (priority 90)                                        │
    └──────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
    ┌──────────────────────────────────────────────────────────────────┐
    │                       ENGINE UPDATE PHASE                         │
    │                                                                   │
    │   physics->update(dt)    Step Box2D simulation                   │
    │   events->processQueue() Handle deferred events                  │
    │   audio->update(dt)      Update FMOD, cleanup finished sounds    │
    │   assets->update()       Process async loads                     │
    │   levels->update(dt)     Handle level transitions                │
    │   save->update(dt)       Auto-save timer                         │
    │   ai->update(dt)         Behavior trees, navigation              │
    └──────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
    ┌──────────────────────────────────────────────────────────────────┐
    │                        RENDER PHASE                               │
    │                                                                   │
    │   graphics->beginFrame()                                         │
    │   Game render systems (sprites, text, particles...)              │
    │   Debug UI (ImGui)                                               │
    │   graphics->endFrame()    Swap buffers                           │
    └──────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
    ┌──────────────────────────────────────────────────────────────────┐
    │                          FRAME END                                │
    │                      Tracy FrameMark                             │
    └──────────────────────────────────────────────────────────────────┘
```

### 5.4 Data Flow

```
┌─────────────────────────────────────────────────────────────────────────┐
│                          DATA FLOW                                       │
└─────────────────────────────────────────────────────────────────────────┘

                        ┌─────────────────┐
                        │   JSON Files    │
                        │                 │
                        │ - Blueprints    │
                        │ - Levels        │
                        │ - Settings      │
                        │ - AI Behaviors  │
                        └────────┬────────┘
                                 │
                    ┌────────────┴────────────┐
                    ▼                         ▼
           ┌────────────────┐       ┌────────────────┐
           │   Asset System │       │ Settings Mgr   │
           │                │       │                │
           │ Load textures, │       │ Controls,      │
           │ sounds, fonts  │       │ Audio, Video   │
           └───────┬────────┘       └───────┬────────┘
                   │                        │
                   ▼                        ▼
           ┌────────────────┐       ┌────────────────┐
           │Blueprint Registry│     │ Input System   │
           │                │       │                │
           │ Parse entity   │       │ Apply bindings │
           │ definitions    │       │                │
           └───────┬────────┘       └────────────────┘
                   │
                   ▼
           ┌────────────────┐
           │ Entity Factory │
           │                │
           │ Instantiate    │
           │ entities from  │
           │ blueprints     │
           └───────┬────────┘
                   │
                   ▼
           ┌────────────────┐
           │ Entity System  │
           │                │
           │ EnTT Registry  │
           │ Components     │
           └───────┬────────┘
                   │
        ┌──────────┴──────────┐
        ▼                     ▼
┌────────────────┐   ┌────────────────┐
│ Game Systems   │   │ Save System    │
│                │   │                │
│ Query & modify │   │ Serialize to   │
│ components     │   │ binary files   │
└────────────────┘   └────────────────┘
```

### 5.5 Event System Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                       EVENT SYSTEM                                       │
└─────────────────────────────────────────────────────────────────────────┘

Publishers                    Event System                    Subscribers
─────────────────────────────────────────────────────────────────────────

┌────────────┐               ┌─────────────────┐              ┌──────────┐
│ Physics    │──collision───▶│                 │──────────────▶│ Combat   │
│ System     │               │                 │              │ System   │
└────────────┘               │                 │              └──────────┘
                             │    Immediate    │
┌────────────┐               │       or        │              ┌──────────┐
│ Level      │──loaded──────▶│    Deferred     │──────────────▶│ Game     │
│ System     │               │     Queue       │              │ Class    │
└────────────┘               │                 │              └──────────┘
                             │                 │
┌────────────┐               │   Subscription  │              ┌──────────┐
│ Combat     │──damage──────▶│     Registry    │──────────────▶│ Audio    │
│ System     │               │                 │              │ System   │
└────────────┘               │                 │              └──────────┘
                             │                 │
┌────────────┐               │                 │              ┌──────────┐
│ Collectible│──collected───▶│                 │──────────────▶│ UI       │
│ System     │               │                 │              │ System   │
└────────────┘               └─────────────────┘              └──────────┘

Event Types:
  - collision     : Physics collision detected
  - level_loaded  : Level finished loading
  - level_unloaded: Level unloaded
  - entity_damaged: Entity took damage
  - entity_died   : Entity health reached zero
  - item_collected: Collectible picked up
  - checkpoint    : Checkpoint activated
  - player_death  : Player died
  - game_saved    : Save completed
  - game_loaded   : Load completed
```

---

## 6. Development Workflow

### 6.1 The "Vite-like" Experience

Bestow provides a hot-reload development workflow similar to modern web development. Edit Lua files in your editor, save, and see changes instantly in the running game.

```
┌─────────────────────────────┐         ┌─────────────────────────────┐
│         VS Code             │         │       Game Window           │
│                             │         │                             │
│  level1.lua                 │         │    ┌───────────────┐        │
│  ───────────────            │  save   │    │   Slime       │        │
│  return {                   │ ──────► │    │     ○         │        │
│    entities = {             │         │    └───────────────┘        │
│      {                      │ detect  │           │                 │
│        x = 100  →  200      │ ──────► │           ▼                 │
│      }                      │         │    ┌───────────────┐        │
│    }                        │ reload  │    │   Slime       │        │
│  }                          │ ──────► │    │         ○     │        │
│                             │         │    └───────────────┘        │
└─────────────────────────────┘         └─────────────────────────────┘
```

**No visual editor required.** Edit Lua → Save → See changes. Just like editing React with Vite.

**Why Lua instead of JSON?**

| Feature | JSON | Lua |
|---------|------|-----|
| Comments | ❌ | `-- comment` |
| Trailing commas | Error | ✅ Allowed |
| Variables | ❌ | `local GROUND_Y = 100` |
| Loops | ❌ Copy-paste | `for i = 1, 10 do ... end` |
| Functions | ❌ | `makeEnemyWave(x, count)` |
| Imports | ❌ | `require("helpers")` |
| Math | ❌ | `math.sin(i) * 100` |
| Conditionals | ❌ | `DEBUG and {...} or {}` |

### 6.2 Hot Reload System

The hot reload system uses **efsw** (Entropia File System Watcher) to monitor the `data/` directory for changes, and **sol2** to execute Lua files.

**What Gets Hot-Reloaded:**

| File Type | Reload Behavior |
|-----------|-----------------|
| `blueprints/*.lua` | Re-execute Lua, update blueprint registry |
| `levels/*.lua` | Re-execute Lua, reload current level if active |
| `textures/*` | Reload texture data, update GPU texture |
| `audio/*` | Reload sound data |
| `config/*.lua` | Re-execute Lua, apply settings immediately |

**Hot Reload Architecture:**

```cpp
// bestow-dev/src/bestow.dev.cppm

module;

#include <efsw/efsw.hpp>
#include <sol/sol.hpp>
#include <filesystem>
#include <functional>
#include <queue>
#include <mutex>

export module bestow.dev;

export namespace bestow::dev {

// File change event
struct FileChange {
    std::filesystem::path path;
    enum class Action { Added, Modified, Deleted } action;
};

// Hot reload manager - watches directories and queues changes
export class HotReloadManager : public efsw::FileWatchListener {
public:
    HotReloadManager() {
        // Set up sandboxed Lua state
        lua_.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table, sol::lib::string);
        
        // Remove dangerous functions for security
        lua_["os"] = sol::nil;
        lua_["io"] = sol::nil;
        lua_["loadfile"] = sol::nil;
        lua_["dofile"] = sol::nil;
        
        // Add helper functions
        registerHelpers();
    }
    
    void watchDirectory(const std::filesystem::path& dir);
    void stopWatching();
    
    // Call each frame to process queued changes
    void update();
    
    // Execute a Lua file and return the result table
    sol::table loadLuaFile(const std::filesystem::path& path);
    
    // Register callbacks for different file types
    std::function<void(const std::filesystem::path&)> onBlueprintChanged;
    std::function<void(const std::filesystem::path&)> onLevelChanged;
    std::function<void(const std::filesystem::path&)> onTextureChanged;
    std::function<void(const std::filesystem::path&)> onAudioChanged;
    std::function<void(const std::filesystem::path&)> onConfigChanged;
    
private:
    void registerHelpers() {
        // range(start, end) -> iterator
        lua_["range"] = [](int start, int end) {
            std::vector<int> result;
            for (int i = start; i <= end; ++i) result.push_back(i);
            return result;
        };
        
        // map(array, fn) -> new array
        lua_["map"] = [this](sol::table arr, sol::function fn) {
            sol::table result = lua_.create_table();
            int idx = 1;
            for (auto& pair : arr) {
                result[idx++] = fn(pair.second);
            }
            return result;
        };
        
        // concat(...) -> merged array
        lua_["concat"] = [this](sol::variadic_args args) {
            sol::table result = lua_.create_table();
            int idx = 1;
            for (auto arg : args) {
                if (arg.get_type() == sol::type::table) {
                    for (auto& pair : arg.as<sol::table>()) {
                        result[idx++] = pair.second;
                    }
                }
            }
            return result;
        };
    }
    
    // efsw callback (called from watcher thread)
    void handleFileAction(efsw::WatchID watchid, 
                          const std::string& dir,
                          const std::string& filename, 
                          efsw::Action action,
                          std::string oldFilename) override;
    
    void processChange(const FileChange& change);
    
    sol::state lua_;
    efsw::FileWatcher watcher_;
    std::queue<FileChange> pendingChanges_;
    std::mutex changeMutex_;
};

}  // namespace bestow::dev
```

**Usage in Game:**

```cpp
import bestow;
import bestow.dev;

class Game {
public:
    void initialize() {
        #if defined(BESTOW_DEV_TOOLS)
        hotReload_.watchDirectory("data/");
        
        hotReload_.onLevelChanged = [this](const auto& path) {
            if (isCurrentLevel(path)) {
                // Re-execute Lua and reload level
                sol::table levelData = hotReload_.loadLuaFile(path);
                reloadCurrentLevel(levelData);
            }
        };
        
        hotReload_.onBlueprintChanged = [this](const auto& path) {
            // Re-execute Lua and update registry
            sol::table blueprintData = hotReload_.loadLuaFile(path);
            blueprintRegistry_.reload(path, blueprintData);
        };
        
        hotReload_.onTextureChanged = [this](const auto& path) {
            engine_.assets->reloadAsset(getAssetHandle(path));
        };
        #endif
    }
    
    void update(DeltaTime dt) {
        #if defined(BESTOW_DEV_TOOLS)
        hotReload_.update();  // Process any pending file changes
        #endif
        
        // Normal game update...
    }
    
private:
    #if defined(BESTOW_DEV_TOOLS)
    bestow::dev::HotReloadManager hotReload_;
    #endif
};
```

### 6.3 Debug vs Release Builds

Development tools are completely compiled out in release builds.

```cpp
// CMakeLists.txt sets BESTOW_DEV_TOOLS for Debug/RelWithDebInfo

#if defined(BESTOW_DEV_TOOLS)
    // ✓ Hot reload enabled
    // ✓ ImGui debug overlay compiled in
    // ✓ Entity inspector available
    // ✓ Position copy-to-clipboard
    // ✓ File watcher running
#else
    // ✗ All dev tools compiled out completely
    // ✗ Zero runtime overhead
    // ✗ Only save system can modify game state
#endif
```

**Build Configurations:**

| Configuration | `BESTOW_DEV_TOOLS` | Use Case |
|---------------|---------------------|----------|
| Debug | Defined | Development with all tools |
| RelWithDebInfo | Defined | Profiling with dev tools |
| Release | Not defined | Final shipping build |

### 6.4 Development Overlay

A minimal ImGui overlay provides entity inspection without requiring visual editing.

```
┌─────────────────────────────────────────────────────────────────────────┐
│ [Game View]                                              [F1: Toggle]   │
│                                                                         │
│    Player ○                                                             │
│                                    ┌──────────────────────────────────┐ │
│         Slime ●  ←── click ───────│ Selected: slime_01               │ │
│                                    │ Level Entity ID: enemy_03        │ │
│                                    │ Blueprint: enemies/slime         │ │
│    Coin ◇                          │ Position: (245.5, 120.0)         │ │
│                                    │ Health: 20/20                    │ │
│                                    │                                  │ │
│                                    │ [📋 Copy Position]               │ │
│                                    │ [📋 Copy Entity Lua]             │ │
│                                    └──────────────────────────────────┘ │
│                                                                         │
│ Hot Reload: ● Watching data/        Last: level1.lua (2s ago)          │
└─────────────────────────────────────────────────────────────────────────┘
```

**Workflow:**

1. Click an entity in the game window
2. See its ID, position, and properties in the inspector
3. Click "Copy Position" → `x = 245.5, y = 120.0` copied to clipboard
4. Paste into your level Lua file
5. Save → Lua re-executes → game updates

**No dragging, no visual positioning.** Just click → copy → paste → save.

### 6.5 Entity Inspector

```cpp
// bestow-dev/src/EntityInspector.cpp

export module bestow.dev:inspector;

import bestow;

export namespace bestow::dev {

class EntityInspector {
public:
    EntityInspector(BestowEngine& engine) : engine_(engine) {}
    
    void update() {
        handleEntitySelection();
        renderInspectorWindow();
    }
    
private:
    void handleEntitySelection() {
        if (engine_.input->wasActionJustPressed("dev_select")) {
            Vec2 mouseWorld = engine_.graphics->screenToWorld(
                engine_.input->getMousePosition()
            );
            selectedEntity_ = findEntityAtPosition(mouseWorld);
        }
    }
    
    void renderInspectorWindow() {
        if (!selectedEntity_) return;
        
        ImGui::Begin("Entity Inspector");
        
        if (auto* levelEntity = engine_.entities->tryGet<LevelEntityId>(*selectedEntity_)) {
            ImGui::Text("Level Entity ID: %s", levelEntity->id.c_str());
        }
        
        if (auto* transform = engine_.entities->tryGet<TransformComponent>(*selectedEntity_)) {
            ImGui::Text("Position: (%.1f, %.1f)", transform->x, transform->y);
            
            if (ImGui::Button("Copy Position (Lua)")) {
                // Output Lua syntax
                std::string lua = std::format(
                    "x = {:.1f}, y = {:.1f}", 
                    transform->x, transform->y
                );
                ImGui::SetClipboardText(lua.c_str());
            }
        }
        
        if (ImGui::Button("Copy Entity (Lua)")) {
            std::string lua = serializeEntityToLua(*selectedEntity_);
            ImGui::SetClipboardText(lua.c_str());
        }
        
        ImGui::End();
    }
    
    std::string serializeEntityToLua(Entity entity) {
        std::string lua = "{\n";
        
        if (auto* levelId = engine_.entities->tryGet<LevelEntityId>(entity)) {
            lua += std::format("  id = \"{}\",\n", levelId->id);
        }
        
        if (auto* blueprint = engine_.entities->tryGet<BlueprintRef>(entity)) {
            lua += std::format("  blueprint = \"{}\",\n", blueprint->id);
        }
        
        if (auto* transform = engine_.entities->tryGet<TransformComponent>(entity)) {
            lua += std::format("  x = {:.1f},\n", transform->x);
            lua += std::format("  y = {:.1f},\n", transform->y);
            if (transform->rotation != 0.0f) {
                lua += std::format("  rotation = {:.3f},\n", transform->rotation);
            }
        }
        
        lua += "}";
        return lua;
    }
    
    std::optional<Entity> selectedEntity_;
    BestowEngine& engine_;
};

}  // namespace bestow::dev
```

### 6.6 Typical Development Session

```
1. Start game in Debug mode
   $ cmake --build --preset macos-debug && ./build/macos-debug/bin/mygame

2. Open level Lua file in VS Code
   $ code data/levels/world1/level1.lua

3. Edit entity position
   entities = {
     {
       id = "coin_01",
       blueprint = "collectibles/coin",
       x = 200,     -- Change this
       y = 150      -- Change this
     },
     
     -- Or use a loop to place many coins!
     unpack(map(range(0, 10), function(i)
       return {
         id = "coin_row_" .. i,
         blueprint = "collectibles/coin",
         x = 300 + (i * 50),
         y = 250
       }
     end))
   }

4. Save file (Cmd+S)
   → Game detects change
   → Lua re-executes
   → Level reloads with new positions

5. Need exact position? Click entity in game
   → Inspector shows current position
   → "Copy Position" → paste into Lua
   → Save → reload

6. Iterate rapidly without recompiling
```

### 6.7 Level Editing Tips

**Finding the Right Position:**

1. **Rough placement:** Edit Lua with estimated coordinates
2. **See result:** Save → Lua re-executes → hot reload
3. **Refine:** Click entity → copy exact position → paste back
4. **Repeat:** Until perfect

**Using Loops (the killer feature):**

Instead of copy-pasting 20 coins:

```lua
-- Old way (tedious):
entities = {
  { id = "coin_0", blueprint = "collectibles/coin", x = 100, y = 200 },
  { id = "coin_1", blueprint = "collectibles/coin", x = 150, y = 200 },
  { id = "coin_2", blueprint = "collectibles/coin", x = 200, y = 200 },
  -- ... 17 more times
}

-- New way (one line):
entities = map(range(0, 19), function(i)
  return {
    id = "coin_" .. i,
    blueprint = "collectibles/coin",
    x = 100 + (i * 50),
    y = 200
  }
end)
```

**Pattern Functions:**

```lua
-- data/levels/helpers.lua

local H = {}

-- Place entities in a grid
function H.grid(blueprint, startX, startY, cols, rows, spacingX, spacingY, idPrefix)
  local entities = {}
  for row = 0, rows - 1 do
    for col = 0, cols - 1 do
      table.insert(entities, {
        id = idPrefix .. "_" .. row .. "_" .. col,
        blueprint = blueprint,
        x = startX + (col * spacingX),
        y = startY + (row * spacingY)
      })
    end
  end
  return entities
end

-- Place entities in an arc
function H.arc(blueprint, centerX, centerY, radius, startAngle, endAngle, count, idPrefix)
  local entities = {}
  local angleStep = (endAngle - startAngle) / (count - 1)
  for i = 0, count - 1 do
    local angle = math.rad(startAngle + (i * angleStep))
    table.insert(entities, {
      id = idPrefix .. "_" .. i,
      blueprint = blueprint,
      x = centerX + math.cos(angle) * radius,
      y = centerY + math.sin(angle) * radius
    })
  end
  return entities
end

-- Place entities along a sine wave
function H.wave(blueprint, startX, y, count, spacing, amplitude, frequency, idPrefix)
  local entities = {}
  for i = 0, count - 1 do
    table.insert(entities, {
      id = idPrefix .. "_" .. i,
      blueprint = blueprint,
      x = startX + (i * spacing),
      y = y + math.sin(i * frequency) * amplitude
    })
  end
  return entities
end

return H
```

**Using Patterns in Levels:**

```lua
-- data/levels/world1/level1.lua

local H = require("levels.helpers")

return {
  id = "world1/level1",
  name = "Green Hills",
  
  entities = concat(
    -- Player spawn
    {{ id = "player", blueprint = "player", x = 100, y = 200 }},
    
    -- 5x3 grid of coins
    H.grid("collectibles/coin", 300, 200, 5, 3, 50, 50, "coin"),
    
    -- Arc of enemies around a point
    H.arc("enemies/slime", 800, 300, 150, 0, 180, 5, "slime"),
    
    -- Wave of floating platforms
    H.wave("platforms/cloud", 1000, 400, 8, 100, 50, 0.5, "cloud")
  )
}
```

**Conditional Content:**

```lua
-- Different content for debug vs release
local DEBUG = os.getenv("BESTOW_DEBUG") == "1"

return {
  -- ...
  entities = concat(
    mainEntities,
    DEBUG and {
      { id = "debug_warp", blueprint = "debug/warp_zone", x = 50, y = 50 }
    } or {}
  )
}
```

**Version Control:**

Lua files are just text—you get:
- Clean diffs
- Easy merging  
- Full history
- Branch-based level variants
- Code review for level changes

### 6.8 Conditional Compilation

```cpp
// In your game code, wrap dev-only features:

void Game::update(DeltaTime dt) {
    // Always runs
    updateGameLogic(dt);
    
    #if defined(BESTOW_DEV_TOOLS)
    // Only in debug builds
    hotReload_.update();
    devOverlay_.update();
    entityInspector_.update();
    #endif
}

void Game::render() {
    renderGame();
    
    #if defined(BESTOW_DEV_TOOLS)
    devOverlay_.render();
    #endif
}
```

---

## 7. Contracts & Interfaces

### 7.1 Core Types (bestow.types.cppm)

```cpp
// Global module fragment - include third-party headers here
module;

#include <entt/entity/fwd.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

// Module declaration
export module bestow.types;

// Standard library imports
import std;

export namespace bestow {

//==========================================================================
// Fundamental Types
//==========================================================================

using UUID = std::uint64_t;
using DeltaTime = float;      // Seconds
using Timestamp = float;      // Seconds from start
using Entity = entt::entity;

template<typename T, typename E = std::error_code>
using Result = std::expected<T, E>;

//==========================================================================
// Math Types (aliases to glm)
//==========================================================================

using Vec2 = glm::vec2;
using Vec3 = glm::vec3;

struct Transform2D {
    float x = 0.0f;
    float y = 0.0f;
    float rotation = 0.0f;    // Radians
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    
    Vec2 position() const { return {x, y}; }
    Vec2 scale() const { return {scaleX, scaleY}; }
};

//==========================================================================
// Graphics Types
//==========================================================================

struct Coordinate {
    int x = 0;
    int y = 0;
};

struct Size {
    int width = 0;
    int height = 0;
};

struct Canvas {
    Coordinate origin;        // Bottom-left corner
    Size size;
};

struct Color {
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
    std::uint8_t a = 255;
    
    static Color white()       { return {255, 255, 255, 255}; }
    static Color black()       { return {0, 0, 0, 255}; }
    static Color red()         { return {255, 0, 0, 255}; }
    static Color green()       { return {0, 255, 0, 255}; }
    static Color blue()        { return {0, 0, 255, 255}; }
    static Color transparent() { return {0, 0, 0, 0}; }
};

using RenderLayer = int32_t;

struct Sprite {
    AssetHandle textureHandle;
    Canvas sourceRect;
    Transform2D transform;
    Color tint = Color::white();
    RenderLayer layer = 0;
    Vec2 anchor = {0.5f, 0.5f};   // Pivot point (0-1)
};

struct Camera {
    Transform2D transform;
    float zoom = 1.0f;
    Size viewportSize;
};

//==========================================================================
// Asset Types
//==========================================================================

enum class AssetType : uint8_t {
    Texture,
    Sound,
    Music,
    Font,
    Level,
    Data,
    Shader,
    NavMesh,
    BehaviorTree
};

enum class AssetState : uint8_t {
    Unloaded,
    Loading,
    Loaded,
    Failed
};

struct AssetHandle {
    UUID uuid = 0;
    AssetType type = AssetType::Data;
    
    bool operator==(const AssetHandle&) const = default;
    auto operator<=>(const AssetHandle&) const = default;
    bool isValid() const { return uuid != 0; }
    
    static AssetHandle invalid() { return {}; }
};

struct AssetHandleHash {
    std::size_t operator()(const AssetHandle& h) const noexcept {
        return std::hash<UUID>{}(h.uuid);
    }
};

//==========================================================================
// Audio Types
//==========================================================================

using Channel = uint32_t;
using Volume = float;          // 0.0 to 1.0
using SoundHandle = uint64_t;  // For positional sounds

struct ChannelSound {
    AssetHandle asset;
    Volume volume = 1.0f;
    float pitch = 1.0f;
    bool looping = false;
    float fadeInTime = 0.0f;
    std::optional<float> startTime;
};

struct PositionalSound {
    AssetHandle asset;
    Vec3 position{0.0f};
    Volume volume = 1.0f;
    float pitch = 1.0f;
    float minDistance = 1.0f;
    float maxDistance = 100.0f;
    std::optional<Vec3> velocity;
    std::function<void()> onComplete;
};

struct AudioListener {
    Vec3 position{0.0f};
    Vec3 forward{0.0f, 0.0f, -1.0f};
    Vec3 up{0.0f, 1.0f, 0.0f};
    Vec3 velocity{0.0f};
};

namespace Channels {
    inline constexpr Channel Music = 0;
    inline constexpr Channel Ambience = 1;
    inline constexpr Channel UI = 2;
    inline constexpr Channel Voice = 3;
}

//==========================================================================
// Input Types
//==========================================================================

enum class InputDeviceType : uint8_t {
    Keyboard,
    Mouse,
    Controller
};

struct InputBinding {
    InputDeviceType deviceType = InputDeviceType::Keyboard;
    int deviceIndex = 0;       // For multiple controllers
    int keyCode = 0;           // Platform key code
    float scale = 1.0f;        // For analog scaling/inversion
    float deadzone = 0.1f;     // For analog inputs
};

using Action = std::string;

struct ActionState {
    Action action;
    bool active = false;       // Currently pressed
    float value = 0.0f;        // -1.0 to 1.0 for analog
    bool justPressed = false;  // Became active this frame
    bool justReleased = false; // Became inactive this frame
};

struct InputMapping {
    InputBinding binding;
    Action action;
};

//==========================================================================
// Physics Types
//==========================================================================

enum class BodyType : uint8_t {
    Static,
    Kinematic,
    Dynamic
};

struct PhysicsBodyDef {
    BodyType type = BodyType::Dynamic;
    Transform2D transform;
    bool fixedRotation = true;
    float linearDamping = 0.0f;
    float angularDamping = 0.0f;
    float density = 1.0f;
    float friction = 0.3f;
    float restitution = 0.0f;
};

using CollisionLayer = uint16_t;
using CollisionMask = uint16_t;

struct CollisionEvent {
    Entity entityA;
    Entity entityB;
    Vec2 contactPoint;
    Vec2 normal;
    float impulse;
};

//==========================================================================
// Level Types
//==========================================================================

using LevelId = UUID;

enum class LevelState : uint8_t {
    Unloaded,
    Loading,
    Loaded,
    Active,
    Unloading
};

enum class LevelEvent : uint8_t {
    LoadStarted,
    LoadCompleted,
    UnloadStarted,
    UnloadCompleted,
    Activated,
    Deactivated
};

struct LevelEventData {
    LevelId levelId;
    LevelEvent event;
};

struct LevelTransition {
    LevelId fromLevel;
    LevelId toLevel;
    std::optional<std::string> spawnPoint;
    bool unloadPrevious = true;
};

//==========================================================================
// Event Types
//==========================================================================

using EventType = std::string;

// Event data variants
struct EntityEventData {
    Entity entity;
    std::optional<Entity> otherEntity;
};

struct DamageEventData {
    Entity target;
    Entity source;
    int amount;
    Vec2 knockback;
};

using EventData = std::variant<
    EntityEventData,
    DamageEventData,
    LevelEventData,
    CollisionEvent,
    std::any
>;

using EventCallback = std::function<void(const EventData&)>;
using SubscriptionId = UUID;

//==========================================================================
// Save Types
//==========================================================================

using SaveSlot = uint32_t;

namespace SaveSlots {
    inline constexpr SaveSlot QuickSave = UINT32_MAX - 1;
    inline constexpr SaveSlot AutoSave = UINT32_MAX;
}

enum class SaveError {
    Success,
    FileNotFound,
    CorruptedFile,
    InvalidChecksum,
    VersionMismatch,
    MigrationFailed,
    IOError,
    SerializationError
};

//==========================================================================
// AI Types
//==========================================================================

enum class BehaviorStatus {
    Success,
    Failure,
    Running
};

}  // namespace bestow
```

### 7.2 IEntitySystem

```cpp
// bestow-contract/src/bestow.entity.cppm

module;

#include <entt/entt.hpp>

export module bestow.entity;

import bestow.types;
import std;

export namespace bestow {

struct EntitySelector {
    std::vector<entt::id_type> requiredComponents;
    std::vector<entt::id_type> excludedComponents;
    std::optional<std::function<bool(Entity)>> predicate;
};

class IEntitySystem {
public:
    virtual ~IEntitySystem() = default;
    
    //======================================================================
    // Entity Lifecycle
    //======================================================================
    
    virtual Entity createEntity() = 0;
    virtual void destroyEntity(Entity entity) = 0;
    virtual bool isValid(Entity entity) const = 0;
    virtual std::size_t entityCount() const = 0;
    
    //======================================================================
    // Component Access (Type-Erased for Interface Boundary)
    //======================================================================
    
    virtual void* addComponent(Entity entity, entt::id_type typeId, 
                               const void* data, std::size_t size) = 0;
    virtual void removeComponent(Entity entity, entt::id_type typeId) = 0;
    virtual void* getComponent(Entity entity, entt::id_type typeId) = 0;
    virtual const void* getComponent(Entity entity, entt::id_type typeId) const = 0;
    virtual bool hasComponent(Entity entity, entt::id_type typeId) const = 0;
    
    //======================================================================
    // Typed Component Helpers
    //======================================================================
    
    template<typename T, typename... Args>
    T& emplace(Entity entity, Args&&... args) {
        auto& registry = getRegistry();
        return registry.emplace<T>(entity, std::forward<Args>(args)...);
    }
    
    template<typename T>
    void remove(Entity entity) {
        getRegistry().remove<T>(entity);
    }
    
    template<typename T>
    T& get(Entity entity) {
        return getRegistry().get<T>(entity);
    }
    
    template<typename T>
    const T& get(Entity entity) const {
        return getRegistry().get<T>(entity);
    }
    
    template<typename T>
    T* tryGet(Entity entity) {
        return getRegistry().try_get<T>(entity);
    }
    
    template<typename... Ts>
    bool allOf(Entity entity) const {
        return getRegistry().all_of<Ts...>(entity);
    }
    
    template<typename... Ts>
    bool anyOf(Entity entity) const {
        return getRegistry().any_of<Ts...>(entity);
    }
    
    //======================================================================
    // Querying
    //======================================================================
    
    virtual std::vector<Entity> query(const EntitySelector& selector) const = 0;
    
    template<typename... Components>
    auto view() {
        return getRegistry().view<Components...>();
    }
    
    template<typename... Components>
    auto view() const {
        return getRegistry().view<Components...>();
    }
    
    //======================================================================
    // Registry Access
    //======================================================================
    
    virtual entt::registry& getRegistry() = 0;
    virtual const entt::registry& getRegistry() const = 0;
    
    //======================================================================
    // Iteration
    //======================================================================
    
    virtual void each(std::function<void(Entity)> callback) = 0;
    virtual void update(DeltaTime dt) = 0;
};

}  // namespace bestow
```

### 7.3 IGraphicsSystem

```cpp
#pragma once

#include "types.hpp"
#include <string>
#include <span>

namespace bestow {

class IGraphicsSystem {
public:
    virtual ~IGraphicsSystem() = default;
    
    //======================================================================
    // Frame Lifecycle
    //======================================================================
    
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;
    
    //======================================================================
    // Sprite Rendering
    //======================================================================
    
    virtual void draw(const Sprite& sprite) = 0;
    virtual void drawBatch(std::span<const Sprite> sprites) = 0;
    
    //======================================================================
    // Primitive Rendering (Debug)
    //======================================================================
    
    virtual void drawRect(const Canvas& rect, const Color& color, 
                          bool filled = true) = 0;
    virtual void drawLine(Vec2 from, Vec2 to, const Color& color, 
                          float thickness = 1.0f) = 0;
    virtual void drawCircle(Vec2 center, float radius, const Color& color,
                            bool filled = true, int segments = 32) = 0;
    virtual void drawPolygon(std::span<const Vec2> vertices, 
                             const Color& color, bool filled = true) = 0;
    
    //======================================================================
    // Text Rendering
    //======================================================================
    
    virtual void drawText(const std::string& text, Vec2 position,
                          AssetHandle fontHandle, float size,
                          const Color& color = Color::white()) = 0;
    virtual Vec2 measureText(const std::string& text, AssetHandle fontHandle,
                             float size) const = 0;
    
    //======================================================================
    // Camera
    //======================================================================
    
    virtual void setCamera(const Camera& camera) = 0;
    virtual Camera getCamera() const = 0;
    
    virtual Vec2 worldToScreen(Vec2 worldPos) const = 0;
    virtual Vec2 screenToWorld(Vec2 screenPos) const = 0;
    
    //======================================================================
    // Window Management
    //======================================================================
    
    virtual Size getWindowSize() const = 0;
    virtual void setWindowSize(Size size) = 0;
    virtual bool isFullscreen() const = 0;
    virtual void setFullscreen(bool fullscreen) = 0;
    virtual bool shouldClose() const = 0;
    virtual void* getNativeWindowHandle() const = 0;
    
    //======================================================================
    // Render State
    //======================================================================
    
    virtual void setClearColor(const Color& color) = 0;
    virtual void setVSync(bool enabled) = 0;
};

}  // namespace bestow
```

### 7.4 IAudioSystem

```cpp
#pragma once

#include "types.hpp"

namespace bestow {

struct ChannelState {
    bool isPlaying = false;
    bool isPaused = false;
    float position = 0.0f;     // Current playback position (seconds)
    float length = 0.0f;       // Total length (seconds)
    Volume volume = 1.0f;
};

class IAudioSystem {
public:
    virtual ~IAudioSystem() = default;
    
    //======================================================================
    // Lifecycle
    //======================================================================
    
    virtual void update(DeltaTime dt) = 0;
    
    //======================================================================
    // Channel-Based Audio (Managed)
    //======================================================================
    
    virtual void playOnChannel(Channel channel, const ChannelSound& sound) = 0;
    virtual void stopChannel(Channel channel, float fadeOutTime = 0.0f) = 0;
    virtual void pauseChannel(Channel channel) = 0;
    virtual void resumeChannel(Channel channel) = 0;
    
    virtual void setChannelVolume(Channel channel, Volume volume) = 0;
    virtual void setChannelPitch(Channel channel, float pitch) = 0;
    virtual void seekChannel(Channel channel, float position) = 0;
    
    virtual ChannelState getChannelState(Channel channel) const = 0;
    virtual bool isChannelPlaying(Channel channel) const = 0;
    
    //======================================================================
    // Positional Audio (Fire & Forget)
    //======================================================================
    
    virtual SoundHandle playPositional(const PositionalSound& sound) = 0;
    virtual void stopPositional(SoundHandle handle) = 0;
    virtual void updatePositionalPosition(SoundHandle handle, Vec3 position) = 0;
    virtual bool isPositionalPlaying(SoundHandle handle) const = 0;
    
    //======================================================================
    // 3D Audio Listener
    //======================================================================
    
    virtual void setListener(const AudioListener& listener) = 0;
    virtual AudioListener getListener() const = 0;
    
    //======================================================================
    // Global Controls
    //======================================================================
    
    virtual void setMasterVolume(Volume volume) = 0;
    virtual Volume getMasterVolume() const = 0;
    
    virtual void pauseAll() = 0;
    virtual void resumeAll() = 0;
    virtual void stopAll() = 0;
    
    //======================================================================
    // Channel Groups
    //======================================================================
    
    virtual void setGroupVolume(const std::string& group, Volume volume) = 0;
    virtual void assignChannelToGroup(Channel channel, const std::string& group) = 0;
};

}  // namespace bestow
```

### 7.5 IInputSystem

```cpp
#pragma once

#include "types.hpp"
#include <vector>
#include <optional>

namespace bestow {

class IInputSystem {
public:
    virtual ~IInputSystem() = default;
    
    //======================================================================
    // Lifecycle
    //======================================================================
    
    virtual void update() = 0;
    
    //======================================================================
    // Mapping Management
    //======================================================================
    
    virtual void registerMapping(const InputMapping& mapping) = 0;
    virtual void removeMapping(const InputBinding& binding) = 0;
    virtual void clearMappings() = 0;
    virtual std::vector<InputMapping> getMappings() const = 0;
    
    //======================================================================
    // Action State Queries
    //======================================================================
    
    virtual ActionState getActionState(const Action& action) const = 0;
    virtual std::vector<ActionState> getAllActionStates() const = 0;
    
    virtual bool isActionActive(const Action& action) const = 0;
    virtual bool wasActionJustPressed(const Action& action) const = 0;
    virtual bool wasActionJustReleased(const Action& action) const = 0;
    virtual float getActionValue(const Action& action) const = 0;
    
    //======================================================================
    // Raw Input (for Rebinding UI)
    //======================================================================
    
    virtual std::optional<InputBinding> getLastInput() const = 0;
    virtual bool isListeningForInput() const = 0;
    virtual void startListeningForInput() = 0;
    virtual void stopListeningForInput() = 0;
    
    //======================================================================
    // Mouse State
    //======================================================================
    
    virtual Vec2 getMousePosition() const = 0;
    virtual Vec2 getMouseDelta() const = 0;
    virtual bool isMouseButtonDown(int button) const = 0;
    
    //======================================================================
    // Controller
    //======================================================================
    
    virtual int getConnectedControllerCount() const = 0;
    virtual bool isControllerConnected(int index) const = 0;
    virtual std::string getControllerName(int index) const = 0;
};

}  // namespace bestow
```

### 7.6 IAssetSystem

```cpp
#pragma once

#include "types.hpp"
#include <filesystem>
#include <functional>
#include <optional>

namespace bestow {

struct AssetMetadata {
    AssetHandle handle;
    std::filesystem::path sourcePath;
    AssetState state = AssetState::Unloaded;
    std::size_t sizeBytes = 0;
    std::optional<std::string> errorMessage;
};

using AssetLoadCallback = std::function<void(AssetHandle, AssetState)>;

class IAssetSystem {
public:
    virtual ~IAssetSystem() = default;
    
    //======================================================================
    // Lifecycle
    //======================================================================
    
    virtual void update() = 0;
    
    //======================================================================
    // Registration
    //======================================================================
    
    virtual AssetHandle registerAsset(AssetType type, 
                                       const std::filesystem::path& path) = 0;
    virtual void unregisterAsset(AssetHandle handle) = 0;
    
    //======================================================================
    // Loading
    //======================================================================
    
    virtual void loadAsset(AssetHandle handle) = 0;
    virtual void loadAssetAsync(AssetHandle handle, 
                                AssetLoadCallback callback = nullptr) = 0;
    virtual void unloadAsset(AssetHandle handle) = 0;
    
    //======================================================================
    // State Queries
    //======================================================================
    
    virtual AssetState getAssetState(AssetHandle handle) const = 0;
    virtual AssetMetadata getAssetMetadata(AssetHandle handle) const = 0;
    virtual bool isLoaded(AssetHandle handle) const = 0;
    
    //======================================================================
    // Raw Data Access
    //======================================================================
    
    virtual void* getRawAsset(AssetHandle handle) = 0;
    virtual const void* getRawAsset(AssetHandle handle) const = 0;
    
    template<typename T>
    T* getAsset(AssetHandle handle) {
        return static_cast<T*>(getRawAsset(handle));
    }
    
    template<typename T>
    const T* getAsset(AssetHandle handle) const {
        return static_cast<const T*>(getRawAsset(handle));
    }
    
    //======================================================================
    // Bulk Operations
    //======================================================================
    
    virtual void loadAll() = 0;
    virtual void unloadAll() = 0;
    virtual std::vector<AssetHandle> getAssetsOfType(AssetType type) const = 0;
    
    //======================================================================
    // Hot Reload (Development)
    //======================================================================
    
    virtual void enableHotReload(bool enable) = 0;
    virtual void checkForReloads() = 0;
};

}  // namespace bestow
```

### 7.7 ISaveSystem

```cpp
#pragma once

#include "types.hpp"
#include <chrono>
#include <vector>
#include <optional>

namespace bestow {

// Interface for objects that can be serialized
class ISaveable {
public:
    virtual ~ISaveable() = default;
    virtual std::string getSaveKey() const = 0;
    virtual void serialize(class SaveArchive& archive) const = 0;
    virtual void deserialize(const class LoadArchive& archive) = 0;
};

struct SaveMetadata {
    SaveSlot slot;
    std::string saveName;
    std::chrono::system_clock::time_point timestamp;
    std::string gameVersion;
    uint64_t playtimeSeconds = 0;
    float completionPercentage = 0.0f;
    std::optional<std::string> levelName;
    bool hasScreenshot = false;
};

class ISaveSystem {
public:
    virtual ~ISaveSystem() = default;
    
    //======================================================================
    // Lifecycle
    //======================================================================
    
    virtual void update(DeltaTime dt) = 0;
    
    //======================================================================
    // Saveable Registration
    //======================================================================
    
    virtual void registerSaveable(ISaveable* saveable) = 0;
    virtual void unregisterSaveable(ISaveable* saveable) = 0;
    
    //======================================================================
    // Save/Load Operations
    //======================================================================
    
    virtual Result<void, SaveError> save(SaveSlot slot, 
                                          const std::string& saveName) = 0;
    virtual Result<void, SaveError> load(SaveSlot slot) = 0;
    virtual bool deleteSave(SaveSlot slot) = 0;
    
    //======================================================================
    // Quick Save/Load
    //======================================================================
    
    virtual void quickSave() = 0;
    virtual void quickLoad() = 0;
    
    //======================================================================
    // Auto-Save
    //======================================================================
    
    virtual void autoSave() = 0;
    virtual void enableAutoSave(std::chrono::seconds interval) = 0;
    virtual void disableAutoSave() = 0;
    
    //======================================================================
    // Metadata Queries
    //======================================================================
    
    virtual std::vector<SaveMetadata> getAllSaveMetadata() const = 0;
    virtual std::optional<SaveMetadata> getSaveMetadata(SaveSlot slot) const = 0;
    virtual bool saveExists(SaveSlot slot) const = 0;
    
    //======================================================================
    // Profile Management
    //======================================================================
    
    virtual void setActiveProfile(const std::string& profileId) = 0;
    virtual std::string getActiveProfile() const = 0;
    virtual std::vector<std::string> getProfiles() const = 0;
};

}  // namespace bestow
```

### 7.8 ILevelSystem

```cpp
#pragma once

#include "types.hpp"
#include <vector>
#include <optional>

namespace bestow {

struct LevelMetadata {
    LevelId id;
    AssetHandle assetHandle;
    std::string levelName;
    LevelState state = LevelState::Unloaded;
    float width = 0.0f;
    float height = 0.0f;
};

class ILevelSystem {
public:
    virtual ~ILevelSystem() = default;
    
    //======================================================================
    // Lifecycle
    //======================================================================
    
    virtual void update(DeltaTime dt) = 0;
    
    //======================================================================
    // Level Management
    //======================================================================
    
    virtual Result<LevelId, std::error_code> loadLevel(AssetHandle levelAsset) = 0;
    virtual void unloadLevel(LevelId levelId) = 0;
    virtual void setActiveLevel(LevelId levelId) = 0;
    
    //======================================================================
    // Level Transitions
    //======================================================================
    
    virtual void transition(const LevelTransition& transition) = 0;
    
    //======================================================================
    // State Queries
    //======================================================================
    
    virtual std::optional<LevelId> getActiveLevel() const = 0;
    virtual LevelState getLevelState(LevelId levelId) const = 0;
    virtual LevelMetadata getLevelMetadata(LevelId levelId) const = 0;
    virtual std::vector<LevelMetadata> getLoadedLevels() const = 0;
    
    //======================================================================
    // Spawn Points
    //======================================================================
    
    virtual std::optional<Transform2D> getSpawnPoint(LevelId levelId,
                                                      const std::string& name) const = 0;
    virtual std::vector<std::string> getSpawnPointNames(LevelId levelId) const = 0;
    
    //======================================================================
    // Level Queries
    //======================================================================
    
    virtual std::vector<Entity> getLevelEntities(LevelId levelId) const = 0;
};

}  // namespace bestow
```

### 7.9 IEventSystem

```cpp
#pragma once

#include "types.hpp"
#include <functional>

namespace bestow {

class IEventSystem {
public:
    virtual ~IEventSystem() = default;
    
    //======================================================================
    // Publishing
    //======================================================================
    
    // Immediate dispatch
    virtual void publish(const EventType& type, const EventData& data) = 0;
    
    // Deferred dispatch (processed during processQueue)
    virtual void queue(const EventType& type, const EventData& data) = 0;
    
    //======================================================================
    // Subscribing
    //======================================================================
    
    virtual SubscriptionId subscribe(const EventType& type, 
                                     EventCallback callback) = 0;
    virtual void unsubscribe(SubscriptionId id) = 0;
    virtual void unsubscribeAll(const EventType& type) = 0;
    
    //======================================================================
    // Processing
    //======================================================================
    
    virtual void processQueue() = 0;
    virtual void clearQueue() = 0;
    virtual std::size_t queueSize() const = 0;
};

// Common event type constants
namespace Events {
    inline constexpr const char* Collision = "collision";
    inline constexpr const char* LevelLoaded = "level_loaded";
    inline constexpr const char* LevelUnloaded = "level_unloaded";
    inline constexpr const char* EntityDamaged = "entity_damaged";
    inline constexpr const char* EntityDied = "entity_died";
    inline constexpr const char* ItemCollected = "item_collected";
    inline constexpr const char* Checkpoint = "checkpoint";
    inline constexpr const char* PlayerDeath = "player_death";
    inline constexpr const char* GameSaved = "game_saved";
    inline constexpr const char* GameLoaded = "game_loaded";
}

}  // namespace bestow
```

### 7.10 IPhysicsSystem

```cpp
#pragma once

#include "types.hpp"
#include <vector>
#include <optional>
#include <functional>

namespace bestow {

struct RaycastHit {
    Entity entity;
    Vec2 point;
    Vec2 normal;
    float distance;
};

class IPhysicsSystem {
public:
    virtual ~IPhysicsSystem() = default;
    
    //======================================================================
    // Lifecycle
    //======================================================================
    
    virtual void update(DeltaTime dt) = 0;
    
    //======================================================================
    // Body Management
    //======================================================================
    
    virtual void createBody(Entity entity, const PhysicsBodyDef& def) = 0;
    virtual void destroyBody(Entity entity) = 0;
    virtual bool hasBody(Entity entity) const = 0;
    
    //======================================================================
    // Body Properties
    //======================================================================
    
    virtual void setBodyType(Entity entity, BodyType type) = 0;
    virtual BodyType getBodyType(Entity entity) const = 0;
    
    virtual void setPosition(Entity entity, Vec2 position) = 0;
    virtual Vec2 getPosition(Entity entity) const = 0;
    
    virtual void setRotation(Entity entity, float radians) = 0;
    virtual float getRotation(Entity entity) const = 0;
    
    virtual void setVelocity(Entity entity, Vec2 velocity) = 0;
    virtual Vec2 getVelocity(Entity entity) const = 0;
    
    virtual void setAngularVelocity(Entity entity, float velocity) = 0;
    virtual float getAngularVelocity(Entity entity) const = 0;
    
    //======================================================================
    // Forces
    //======================================================================
    
    virtual void applyForce(Entity entity, Vec2 force, 
                            Vec2 point = {0, 0}) = 0;
    virtual void applyImpulse(Entity entity, Vec2 impulse,
                              Vec2 point = {0, 0}) = 0;
    virtual void applyTorque(Entity entity, float torque) = 0;
    
    //======================================================================
    // Collision Filtering
    //======================================================================
    
    virtual void setCollisionLayer(Entity entity, CollisionLayer layer) = 0;
    virtual void setCollisionMask(Entity entity, CollisionMask mask) = 0;
    virtual void setSensor(Entity entity, bool isSensor) = 0;
    
    //======================================================================
    // Queries
    //======================================================================
    
    virtual std::vector<Entity> queryAABB(Vec2 min, Vec2 max) const = 0;
    virtual std::vector<Entity> queryCircle(Vec2 center, float radius) const = 0;
    virtual std::optional<RaycastHit> raycast(Vec2 origin, Vec2 direction,
                                               float maxDistance,
                                               CollisionMask mask = 0xFFFF) const = 0;
    virtual std::vector<RaycastHit> raycastAll(Vec2 origin, Vec2 direction,
                                                float maxDistance,
                                                CollisionMask mask = 0xFFFF) const = 0;
    
    //======================================================================
    // World Settings
    //======================================================================
    
    virtual void setGravity(Vec2 gravity) = 0;
    virtual Vec2 getGravity() const = 0;
    
    //======================================================================
    // Collision Callbacks
    //======================================================================
    
    using CollisionCallback = std::function<void(const CollisionEvent&)>;
    virtual void setCollisionCallback(CollisionCallback callback) = 0;
};

}  // namespace bestow
```

### 7.11 IAISystem

```cpp
#pragma once

#include "types.hpp"
#include <string>
#include <vector>
#include <optional>

namespace bestow {

struct NavMeshQuery {
    Vec2 start;
    Vec2 end;
    float agentRadius = 0.5f;
};

struct NavigationPath {
    std::vector<Vec2> waypoints;
    float totalLength = 0.0f;
    bool isComplete = false;    // False if path was truncated
};

class IAISystem {
public:
    virtual ~IAISystem() = default;
    
    //======================================================================
    // Lifecycle
    //======================================================================
    
    virtual void update(DeltaTime dt) = 0;
    
    //======================================================================
    // Behavior Trees
    //======================================================================
    
    virtual void attachBehaviorTree(Entity entity, AssetHandle treeAsset) = 0;
    virtual void detachBehaviorTree(Entity entity) = 0;
    virtual bool hasBehaviorTree(Entity entity) const = 0;
    
    virtual void setBehaviorTreeBlackboard(Entity entity, 
                                            const std::string& key,
                                            const std::any& value) = 0;
    virtual std::any getBehaviorTreeBlackboard(Entity entity,
                                                const std::string& key) const = 0;
    
    //======================================================================
    // Navigation
    //======================================================================
    
    virtual void loadNavMesh(AssetHandle navMeshAsset) = 0;
    virtual void unloadNavMesh() = 0;
    virtual bool hasNavMesh() const = 0;
    
    virtual std::optional<NavigationPath> findPath(const NavMeshQuery& query) const = 0;
    virtual bool isPointOnNavMesh(Vec2 point) const = 0;
    virtual std::optional<Vec2> getClosestPointOnNavMesh(Vec2 point) const = 0;
    
    //======================================================================
    // Steering Behaviors
    //======================================================================
    
    virtual void setNavigationTarget(Entity entity, Vec2 target) = 0;
    virtual void clearNavigationTarget(Entity entity) = 0;
    virtual std::optional<Vec2> getNavigationTarget(Entity entity) const = 0;
    
    virtual void setMaxSpeed(Entity entity, float speed) = 0;
    virtual void setMaxAcceleration(Entity entity, float acceleration) = 0;
    
    //======================================================================
    // Spatial Queries
    //======================================================================
    
    virtual std::vector<Entity> findEntitiesInRadius(Vec2 center, float radius,
                                                      CollisionMask mask = 0xFFFF) const = 0;
    virtual std::optional<Entity> findClosestEntity(Vec2 position,
                                                     CollisionMask mask = 0xFFFF) const = 0;
    virtual bool hasLineOfSight(Vec2 from, Vec2 to, 
                                CollisionMask obstacleMask = 0xFFFF) const = 0;
};

}  // namespace bestow
```

---

## 8. System Overviews

### 8.1 Entity System

**Purpose:** Manages game entities using the ECS (Entity Component System) pattern.

**Implementation:** Wraps EnTT registry with interface-compliant API.

**Key Responsibilities:**
- Entity creation and destruction
- Component attachment and querying
- Archetype-based iteration

**Data Ownership:**
- EnTT registry (sole owner)
- All component data

**Dependencies:**
- EnTT

**Usage Pattern:**
```cpp
// Create entity
Entity player = entities->createEntity();

// Add components via typed helpers
entities->emplace<TransformComponent>(player, Transform2D{100, 200});
entities->emplace<Health>(player, 100, 100);
entities->emplace<PlayerTag>(player);

// Query entities
for (auto [entity, transform, health] : entities->view<TransformComponent, Health>()) {
    // Process entities with both components
}
```

---

### 8.2 Graphics System

**Purpose:** Handles all rendering operations.

**Implementation:** GLFW window + OpenGL 4.1 renderer (Vulkan migration planned).

**Key Responsibilities:**
- Window management
- Sprite batching and rendering
- Sprite animation system
- Text rendering (MSDF fonts)
- Debug drawing (primitives)
- Camera management (see Camera System)

**Data Ownership:**
- Window handle
- OpenGL context
- Render state
- Shader programs
- Vertex buffers
- Animation state tracking

**Dependencies:**
- GLFW
- stb_image (via Asset System)
- FreeType (font loading)
- glm

**Render Pipeline:**
```
1. beginFrame()     - Clear buffers, set up frame
2. draw() calls     - Queue sprites into batches
3. drawText()       - Queue text rendering
4. Debug rendering  - Lines, shapes, etc.
5. ImGui rendering  - Debug UI
6. endFrame()       - Submit batches, swap buffers
```

**Sprite Rendering:**

```cpp
// Load a spritesheet
AssetHandle playerSheet = assets->registerAsset(AssetType::Texture, "sprites/player.png");

// Define spritesheet layout
SpriteSheet sheet{
    .texture = playerSheet,
    .frameWidth = 32,
    .frameHeight = 32,
    .columns = 8,
    .rows = 4
};

// Render a specific frame
graphics->drawSprite(sheet, frameIndex, position, size, rotation, tint);

// Render with automatic batching
graphics->drawSprite(sheet, 0, {100, 200}, {32, 32});
graphics->drawSprite(sheet, 1, {150, 200}, {32, 32});
graphics->drawSprite(sheet, 2, {200, 200}, {32, 32});
// All batched into single draw call
```

**Animation System:**

```cpp
// Define animations
Animation idleAnim{
    .name = "idle",
    .frames = {0, 1, 2, 3},
    .frameDuration = 0.15f,
    .looping = true
};

Animation runAnim{
    .name = "run",
    .frames = {8, 9, 10, 11, 12, 13},
    .frameDuration = 0.08f,
    .looping = true
};

// Component-based approach
struct AnimatedSprite {
    SpriteSheet* sheet;
    std::string currentAnimation;
    int currentFrame = 0;
    float frameTimer = 0.0f;
    std::unordered_map<std::string, Animation> animations;
};

// Game code
auto& sprite = entities->getComponent<AnimatedSprite>(player);
sprite.currentAnimation = "run";

// In render system
graphics->updateAnimations(dt);  // Updates all AnimatedSprite components
graphics->renderAnimatedSprites();  // Batched rendering
```

**Text Rendering:**

```cpp
// Load font
AssetHandle font = assets->registerAsset(AssetType::Font, "fonts/roboto.ttf");

// Render text
graphics->drawText("Score: 1000", {10, 10}, font, 24, Color::white());
graphics->drawTextCentered("GAME OVER", {400, 300}, font, 48, Color::red());

// Measure text for layout
Vec2 size = graphics->measureText("Press SPACE", font, 32);
Vec2 centeredPos = {screenWidth/2 - size.x/2, screenHeight - 50};
graphics->drawText("Press SPACE", centeredPos, font, 32);
```

**Sprite Batching:**

The graphics system automatically batches sprites that share:
- Same texture/spritesheet
- Same shader
- Same blend mode
- Consecutive draw calls

This reduces draw calls from potentially thousands to dozens per frame.

---

### 8.3 Audio System

**Purpose:** Provides channel-based managed audio and fire-and-forget positional audio.

**Implementation:** FMOD Core API wrapper.

**Key Responsibilities:**
- Channel-based audio (music, UI, voice)
- 3D positional audio with attenuation
- Volume groups (music, sfx, voice)
- Fade in/out support
- Listener tracking for 3D audio

**Data Ownership:**
- FMOD system instance
- Channel states
- Loaded sound cache
- Channel group volumes

**Dependencies:**
- FMOD Core

**Two-Tier Audio Model:**

| Tier | Use Case | API | Lifecycle |
|------|----------|-----|-----------|
| Channel-Based | Music, UI, Voice | `playOnChannel()` | Explicit stop |
| Positional | SFX, Combat, Environment | `playPositional()` | Auto-cleanup |

---

### 8.4 Input System

**Purpose:** Maps raw input to game actions with support for multiple input devices.

**Implementation:** GLFW for keyboard/mouse, SDL2 GameController for controllers.

**Key Responsibilities:**
- Keyboard and mouse polling (GLFW)
- Game controller support with automatic mapping (SDL2)
- Action mapping (input → action)
- Edge detection (just pressed/released)
- Input rebinding support
- Controller hot-plug handling
- Support for 500+ controller types via SDL2's controller database

**Data Ownership:**
- Input mappings
- Action states
- Controller connection state (up to 4 controllers)
- Mouse position/delta
- Keyboard key states

**Dependencies:**
- GLFW (keyboard, mouse)
- SDL2 GameController (controllers only, `SDL_INIT_GAMECONTROLLER`)

**Why Two Libraries:**

| Input Type | Library | Reason |
|------------|---------|--------|
| Keyboard | GLFW | Already used for windowing |
| Mouse | GLFW | Already used for windowing |
| Controllers | SDL2 | 500+ device database, automatic button mapping, hot-plug |

SDL2 is initialized with *only* the GameController subsystem—no window, no renderer, no audio. This is a common pattern used by many shipped games.

**Controller Mapping:**

SDL2 GameController provides standardized button names that work across all supported controllers:

| SDL Constant | Xbox | PlayStation | Switch |
|--------------|------|-------------|--------|
| `SDL_CONTROLLER_BUTTON_A` | A | Cross | B |
| `SDL_CONTROLLER_BUTTON_B` | B | Circle | A |
| `SDL_CONTROLLER_BUTTON_X` | X | Square | Y |
| `SDL_CONTROLLER_BUTTON_Y` | Y | Triangle | X |
| `SDL_CONTROLLER_AXIS_LEFTX` | Left Stick X | Left Stick X | Left Stick X |
| `SDL_CONTROLLER_AXIS_TRIGGERLEFT` | LT | L2 | ZL |

**Action Flow:**
```
Keyboard (GLFW) ─────┐
Mouse (GLFW) ────────┼──▶ Input Binding ──▶ Action ──▶ ActionState
Controller (SDL2) ───┘

Example:
  Space Key (GLFW)      ──▶  "jump"  ──▶  {active: true, justPressed: true}
  Button A (SDL2)       ──▶  "jump"  ──▶  {active: true, justPressed: true}
  Left Stick X (SDL2)   ──▶  "move_horizontal"  ──▶  {active: true, value: 0.75}
```

**Hot-Plug Support:**

```cpp
// SDL2 handles controller connect/disconnect events
SDL_Event event;
while (SDL_PollEvent(&event)) {
    switch (event.type) {
        case SDL_CONTROLLERDEVICEADDED:
            // Controller connected
            break;
        case SDL_CONTROLLERDEVICEREMOVED:
            // Controller disconnected
            break;
    }
}
```

---

### 8.5 Asset System

**Purpose:** Manages loading, caching, and lifecycle of all game assets.

**Implementation:** Async loader with type-specific loaders.

**Key Responsibilities:**
- Asset registration and handle generation
- Synchronous and asynchronous loading
- Asset caching and reference counting
- Hot reload support (development)
- Asset packaging (production)

**Data Ownership:**
- Asset registry
- Loaded asset data
- Loading queue

**Dependencies:**
- stb_image (textures)
- stb_truetype / FreeType (fonts)
- FMOD (sounds loaded through Audio System)
- nlohmann/json (data assets)
- zstd (compressed assets)
- taskflow (async loading)

**Supported Asset Types:**
| Type | Extension | Loader |
|------|-----------|--------|
| Texture | .png, .jpg | stb_image |
| Sound | .wav, .ogg | FMOD |
| Music | .ogg, .mp3 | FMOD (streaming) |
| Font | .ttf | FreeType → MSDF |
| Level | .json | nlohmann/json |
| Data | .json | nlohmann/json |
| NavMesh | .navmesh | Recast/Detour |
| BehaviorTree | .xml | BT.CPP |

---

### 8.6 Save System

**Purpose:** Persists game state with versioning, checksums, and backup recovery.

**Implementation:** cereal serialization with zstd compression.

**Key Responsibilities:**
- Save/load orchestration
- Schema versioning and migration
- Checksum validation
- Backup management
- Profile management
- Auto-save timer

**Data Ownership:**
- Save metadata
- Active save state
- Profile data
- Auto-save configuration

**Dependencies:**
- cereal
- zstd
- nlohmann/json (metadata)

**Save File Format:**
```
┌──────────────────┐
│   Header (16B)   │  Magic, Version, Checksum, Flags
├──────────────────┤
│   Metadata       │  Name, Timestamp, Playtime, Screenshot
├──────────────────┤
│   Compressed     │  zstd-compressed cereal archive
│   Game State     │  Player, World, Level states
└──────────────────┘
```

---

### 8.7 Level System

**Purpose:** Manages level loading, unloading, transitions, and entity spawning.

**Implementation:** Lua-based level definitions with sol2 integration and blueprint-based entity spawning.

**Key Responsibilities:**
- Execute Lua level files in sandboxed environment
- Entity spawning from level data (supports loops, patterns, conditionals)
- Blueprint reference resolution
- Level transitions
- Spawn point management
- Level state tracking
- Hot reload support

**Data Ownership:**
- Loaded level metadata
- Level entity references
- Spawn point registry
- Transition queue
- Lua state for level execution
- Entity spawn definitions

**Dependencies:**
- Lua 5.4
- sol2
- Entity System
- Physics System (for entity creation)
- Blueprint System (for entity templates)

**Level Lifecycle:**
```
LoadLevel() → Execute Lua → Parse Result Table → Create Entities → Set Active → Ready
    ↓
Transition() → Queue transition → Unload old → Execute new Lua → Apply spawn
    ↓
UnloadLevel() → Destroy entities → Clear state

Hot Reload:
FileChanged → Re-execute Lua → Diff entities → Update changed → Ready
```

**Entity Spawning:**

The LevelSystem can spawn entities directly from level files using blueprints:

```cpp
// Level file format (Lua)
entities = {
    -- Spawn from blueprint
    { blueprint = "player", spawnPoint = "player_start" },

    -- Spawn with blueprint and custom properties
    {
        blueprint = "enemy_walker",
        spawnPoint = "enemy1",
        properties = { patrolRange = 200, speed = 50 }
    },

    -- Direct entity definition (for simple static objects)
    {
        type = "platform",
        x = 100, y = 500,
        width = 400, height = 32,
        texture = "tiles/grass_platform"
    },

    -- Procedural spawning with loops
    H.map(H.range(0, 10), function(i)
        return {
            blueprint = "coin",
            x = 100 + i * 50,
            y = 200
        }
    end)
}

spawnPoints = {
    player_start = { x = 100, y = 200 },
    enemy1 = { x = 500, y = 200 },
    checkpoint1 = { x = 1000, y = 200 }
}

// C++ API
std::vector<Entity> LevelSystem::spawnEntities(
    LevelId levelId,
    IEntitySystem& entities,
    IPhysicsSystem& physics
);

// Usage
auto spawnedEntities = level->spawnEntities(currentLevel, *entities, *physics);
```

**Blueprint Integration:**

```cpp
// Level references blueprint by ID
{ blueprint = "enemies/slime", x = 300, y = 200 }

// LevelSystem resolves blueprint
auto blueprint = blueprints->getBlueprint("enemies/slime");

// Creates entity with blueprint components + level overrides
Entity enemy = entities->createEntity();
blueprint->applyTo(enemy, entities);  // Apply blueprint
applyLevelOverrides(enemy, levelDef); // Apply level-specific values
```

**Why Lua for Levels:**
- Loops for repetitive placement (coins, enemies, platforms)
- Variables for consistent positioning (`local GROUND_Y = 100`)
- Functions for reusable patterns (`H.grid()`, `H.arc()`, `H.wave()`)
- Conditionals for debug/release content
- Math for procedural placement (`math.sin`, `math.random`)
- Imports for shared helpers (`require("levels.helpers")`)
- Blueprint references for entity templates

---

### 8.8 Event System

**Purpose:** Decoupled inter-system communication via publish/subscribe.

**Implementation:** Type-indexed callback registry with deferred queue.

**Key Responsibilities:**
- Event subscription management
- Immediate event dispatch
- Deferred event queue
- Event type safety

**Data Ownership:**
- Subscription registry
- Event queue

**Dependencies:**
- None (pure C++)

**Event Modes:**

| Mode | Method | When Processed | Use Case |
|------|--------|----------------|----------|
| Immediate | `publish()` | Instantly | Critical events |
| Deferred | `queue()` | `processQueue()` | Frame-safe events |

---

### 8.9 Physics System

**Purpose:** 2D physics simulation and collision detection.

**Implementation:** Box2D 3.0 wrapper.

**Key Responsibilities:**
- Rigid body simulation
- Collision detection and response
- Collision filtering (layers/masks)
- Spatial queries (AABB, circle, raycast)
- Physics-based movement
- Sensor/trigger bodies for overlap detection

**Data Ownership:**
- Box2D world
- Body-to-entity mapping
- Collision callback
- Trigger event callbacks

**Dependencies:**
- Box2D 3.0

**Collision Layers (Example):**
```cpp
namespace CollisionLayers {
    constexpr CollisionLayer Player     = 0x0001;
    constexpr CollisionLayer Enemy      = 0x0002;
    constexpr CollisionLayer Projectile = 0x0004;
    constexpr CollisionLayer Terrain    = 0x0008;
    constexpr CollisionLayer Trigger    = 0x0010;
    constexpr CollisionLayer Collectible= 0x0020;
}
```

**Sensor Bodies (Triggers):**

Sensor bodies detect overlap without physical collision response, ideal for collectibles, triggers, and detection zones.

```cpp
// Create a sensor body
PhysicsBodyDef coinDef{
    .type = BodyType::Static,
    .transform = {.x = 100, .y = 200},
    .isSensor = true,  // No collision response, only overlap detection
    .layers = {
        .category = CollisionLayers::Collectible,
        .mask = CollisionLayers::Player
    }
};
physics->createBody(coinEntity, coinDef);

// Subscribe to trigger events
events->subscribe(Events::TriggerEnter, [this](const EventData& data) {
    auto& trigger = std::get<TriggerEvent>(data);
    if (entities->hasComponent<Coin>(trigger.sensorEntity)) {
        collectCoin(trigger.sensorEntity);
    }
});

events->subscribe(Events::TriggerExit, [this](const EventData& data) {
    auto& trigger = std::get<TriggerEvent>(data);
    // Handle entity leaving trigger zone
});
```

**Trigger Event Types:**
```cpp
namespace Events {
    inline constexpr EventType TriggerEnter = "trigger_enter";
    inline constexpr EventType TriggerExit = "trigger_exit";
}

struct TriggerEvent {
    Entity sensorEntity;   // The sensor body (trigger/collectible)
    Entity otherEntity;    // The entity that entered/exited
};
```

---

### 8.10 AI System

**Purpose:** AI behavior and navigation.

**Implementation:** BehaviorTree.CPP + Recast/Detour.

**Key Responsibilities:**
- Behavior tree execution
- Navigation mesh management
- Pathfinding
- Steering behaviors
- Spatial awareness queries

**Data Ownership:**
- Behavior tree factory
- Entity blackboards
- Navigation mesh
- Active paths

**Dependencies:**
- BehaviorTree.CPP
- Recast/Detour

**Behavior Tree Structure:**
```xml
<BehaviorTree ID="EnemyAI">
  <Fallback>
    <Sequence>
      <Condition ID="PlayerInRange" range="150"/>
      <Action ID="ChasePlayer"/>
      <Condition ID="PlayerInAttackRange" range="30"/>
      <Action ID="Attack"/>
    </Sequence>
    <Action ID="Patrol"/>
  </Fallback>
</BehaviorTree>
```

---

### 8.11 Camera System

**Purpose:** Manages 2D camera behavior and viewport transformations.

**Implementation:** Standalone camera system with common platformer behaviors.

**Key Responsibilities:**
- Camera follow with smoothing
- Deadzone-based following
- Camera bounds enforcement
- Camera shake effects
- Viewport to world coordinate conversion
- Integration with GraphicsSystem

**Data Ownership:**
- Camera transform (position, zoom, rotation)
- Follow target entity
- Deadzone configuration
- Bounds limits
- Shake state

**Dependencies:**
- Entity System (for target tracking)
- glm (for matrix math)

**Camera Behaviors:**

```cpp
// Basic follow
camera->setTarget(playerEntity);
camera->setFollowSmoothing(0.1f);  // Smooth lerp factor

// Deadzone (only moves camera when target leaves deadzone)
camera->setDeadzone({100, 60});  // Center deadzone in pixels

// Camera bounds (prevent showing outside level)
camera->setBounds(0, levelWidth, 0, levelHeight);

// Camera shake
camera->shake(0.5f, 0.3f);  // intensity, duration

// Each frame
camera->update(dt);
Camera currentCam = camera->getCamera();
graphics->setCamera(currentCam);
```

**Camera Structure:**

```cpp
struct Camera {
    Vec2 position;
    float zoom = 1.0f;
    float rotation = 0.0f;  // In radians

    // Conversion utilities
    Vec2 screenToWorld(Vec2 screenPos) const;
    Vec2 worldToScreen(Vec2 worldPos) const;
};
```

**Common Patterns:**

```cpp
// Smooth follow
Vec2 targetPos = entities->getComponent<Transform>(target).position;
Vec2 desired = targetPos - viewport.size / 2.0f;
camera.position = lerp(camera.position, desired, smoothing * dt);

// Deadzone follow
Vec2 targetScreenPos = camera.worldToScreen(targetPos);
Vec2 deadZoneMin = viewport.size / 2.0f - deadzone / 2.0f;
Vec2 deadZoneMax = viewport.size / 2.0f + deadzone / 2.0f;
if (targetScreenPos.x < deadZoneMin.x) {
    camera.position.x -= (deadZoneMin.x - targetScreenPos.x);
}
// ... similar for other edges

// Bounds clamping
camera.position.x = std::clamp(camera.position.x, minX, maxX);
camera.position.y = std::clamp(camera.position.y, minY, maxY);

// Shake
float shakeOffset = sin(shakeTime * 30.0f) * shakeIntensity;
camera.position += {shakeOffset, shakeOffset};
```

---

### 8.12 Components Library

**Purpose:** Provides common, reusable components for typical game mechanics.

**Implementation:** Optional header-only component definitions.

**Key Responsibilities:**
- Standard component definitions
- Common gameplay patterns
- Reduce boilerplate in game code

**Component Categories:**

**Movement Components:**
```cpp
struct Velocity {
    Vec2 linear{0, 0};
    float angular = 0.0f;
};

struct Acceleration {
    Vec2 value{0, 0};
};

struct MaxSpeed {
    float linear = 300.0f;
    float angular = 180.0f;
};
```

**Combat Components:**
```cpp
struct Health {
    int current;
    int maximum;
    float invincibilityTimer = 0.0f;

    bool isDead() const { return current <= 0; }
    void damage(int amount) {
        if (invincibilityTimer <= 0.0f) {
            current = std::max(0, current - amount);
        }
    }
};

struct Damage {
    int amount;
    DamageType type = DamageType::Physical;
    Entity source = Entity::null();
};
```

**Gameplay Components:**
```cpp
struct Timer {
    float remaining;
    float duration;
    bool repeating = false;
    std::function<void()> onComplete;

    void update(float dt) {
        remaining -= dt;
        if (remaining <= 0.0f) {
            if (onComplete) onComplete();
            if (repeating) remaining = duration;
        }
    }
};

struct Lifetime {
    float remaining;

    bool expired() const { return remaining <= 0.0f; }
};
```

**Physics Helper Components:**
```cpp
struct GroundDetector {
    bool isGrounded = false;
    float coyoteTime = 0.0f;  // Remaining coyote time
    static constexpr float MAX_COYOTE_TIME = 0.15f;

    void update(float dt, bool touching) {
        if (touching) {
            isGrounded = true;
            coyoteTime = MAX_COYOTE_TIME;
        } else {
            isGrounded = false;
            coyoteTime = std::max(0.0f, coyoteTime - dt);
        }
    }

    bool canJump() const { return coyoteTime > 0.0f; }
};
```

**Usage Example:**

```cpp
// Create player with common components
Entity player = entities->createEntity();
entities->emplace<Transform>(player, Vec2{100, 200});
entities->emplace<Velocity>(player);
entities->emplace<Health>(player, 100, 100);
entities->emplace<GroundDetector>(player);

// Update system
void updateMovement(DeltaTime dt) {
    auto view = entities->view<Transform, Velocity>();
    for (auto entity : view) {
        auto& transform = view.get<Transform>(entity);
        auto& velocity = view.get<Velocity>(entity);
        transform.position += velocity.linear * dt;
    }
}

void updateTimers(DeltaTime dt) {
    auto view = entities->view<Timer>();
    for (auto entity : view) {
        auto& timer = view.get<Timer>(entity);
        timer.update(dt);
    }
}
```

---

### 8.13 Config System

**Purpose:** Manages game configuration through Lua files with type-safe access, hot-reloading, and change notifications.

**Implementation:** Sandboxed Lua execution with sol2 integration for runtime configuration management.

**Key Responsibilities:**
- Load and parse Lua configuration files
- Provide type-safe value access (float, int, bool, string)
- Support array/list access for collections
- Runtime value modification without file rewrite
- Hot-reload configuration files during development
- Notify subscribers of configuration changes
- Hierarchical key access with dot notation

**Data Ownership:**
- Lua state with configuration tables
- Loaded configuration values
- File modification timestamps
- Change notification subscribers
- Hot-reload watch state

**Dependencies:**
- Lua 5.4
- sol2
- spdlog

**Security (Sandboxing):**

The Config System executes Lua in a restricted sandbox environment:

```cpp
// Allowed libraries: base, math, table, string
lua_.open_libraries(sol::lib::base, sol::lib::math,
                    sol::lib::table, sol::lib::string);

// Removed functions (security):
os, io, loadfile, dofile, load, loadstring,
require, package, debug, rawget, rawset
```

This prevents configuration files from:
- Accessing the file system
- Loading external code
- Modifying Lua internals
- Executing system commands

**Configuration File Format (Lua):**

```lua
-- data/config/game.lua

-- Physics configuration
physics = {
    gravity = 980.0,
    velocityIterations = 8,
    positionIterations = 3
}

-- Player configuration
player = {
    physics = {
        size = { width = 28, height = 52 },
        speed = 200.0,
        jumpForce = 450.0
    },
    jump = {
        maxJumps = 2,
        coyoteTime = 0.15,
        jumpBufferTime = 0.1
    },
    combat = {
        maxHealth = 100,
        invincibilityTime = 1.0
    }
}

-- Animation configuration
animations = {
    player = {
        idle = {
            frameIds = {0, 1, 2, 3},
            frameDuration = 0.15,
            looping = true
        },
        run = {
            frameIds = {8, 9, 10, 11, 12, 13},
            frameDuration = 0.08,
            looping = true
        }
    }
}

-- Computed values (why Lua > JSON)
local BASE_SPEED = 200
debug_mode = {
    enabled = true,
    playerSpeed = BASE_SPEED * 2,  -- Can use variables and math
    showColliders = true
}
```

**Type-Safe Access:**

```cpp
// Load configuration
auto& config = engine.config();
config->loadConfig("data/config/game.lua");

// Access with defaults (recommended)
float gravity = config->getFloatOr("physics.gravity", 980.0f);
int maxJumps = config->getIntOr("player.jump.maxJumps", 2);
bool debugMode = config->getBoolOr("debug_mode.enabled", false);
std::string title = config->getStringOr("game.title", "Untitled Game");

// Access without defaults (returns std::optional)
if (auto speed = config->getFloat("player.physics.speed")) {
    playerSpeed = *speed;
}

// Array access
auto idleFrames = config->getIntArray("animations.player.idle.frameIds");
// Returns: {0, 1, 2, 3}

auto enemyTypes = config->getStringArray("levels.world1.enemyTypes");
// Returns: {"slime", "bat", "skeleton"}
```

**Runtime Modification:**

```cpp
// Modify values at runtime (doesn't change file)
config->setFloat("player.physics.speed", 300.0f);
config->setInt("player.jump.maxJumps", 3);
config->setBool("debug_mode.showColliders", false);

// New value immediately available
float newSpeed = config->getFloatOr("player.physics.speed", 200.0f);
// Returns: 300.0f
```

**Hot Reload (Development):**

```cpp
// Enable hot reload - checks for file changes every update
config->enableHotReload(true);

// Update loop
void Game::update(DeltaTime dt) {
    config->update(dt);  // Checks for file modifications
    // If file changed, automatically reloads
}

// Subscribe to reload notifications
config->onConfigChanged([this](const ConfigKey& key) {
    spdlog::info("Config changed: {}", key);
    applyConfigChanges();
});

// Subscribe to specific key changes
config->onKeyChanged("player.physics", [this](const ConfigKey& key) {
    spdlog::info("Player physics config changed: {}", key);
    updatePlayerPhysics();
});
```

**Change Notifications:**

```cpp
// Global config change notification
auto id = config->onConfigChanged([](const ConfigKey& key) {
    spdlog::info("Config key changed: {}", key);
});

// Prefix-based notifications (only for keys starting with prefix)
auto playerId = config->onKeyChanged("player.", [this](const ConfigKey& key) {
    // Called only when player.* keys change
    reloadPlayerConfig();
});

// Unsubscribe
config->unsubscribe(id);
config->unsubscribe(playerId);
```

**Hierarchical Key Access:**

```cpp
// Dot notation traverses nested tables
config->getFloat("player.physics.speed")        // ✓ player.physics.speed
config->getInt("animations.player.idle.frameIds")  // ✓ animations.player.idle.frameIds
config->getBool("debug_mode.showColliders")     // ✓ debug_mode.showColliders

// Key existence check
if (config->hasKey("player.physics.jumpForce")) {
    // Key exists
}

// Get all keys with prefix
auto playerKeys = config->getKeysWithPrefix("player.");
// Returns: ["player.physics.size.width", "player.physics.size.height",
//           "player.physics.speed", "player.jump.maxJumps", ...]
```

**Multiple Configuration Files:**

```cpp
// Load multiple configs (values merge into same namespace)
config->loadConfig("data/config/game.lua");
config->loadConfig("data/config/player.lua");
config->loadConfig("data/config/enemies.lua");

// Reload specific file
config->reloadConfig("data/config/player.lua");

// Reload all loaded files
config->reloadAll();

// Query loaded files
auto files = config->getLoadedConfigs();
// Returns: ["data/config/game.lua", "data/config/player.lua", ...]

// Get file metadata
auto metadata = config->getMetadata("data/config/game.lua");
// Returns: { sourcePath, loadTime, isDirty }
```

**Common Patterns:**

```cpp
// Pattern 1: Load-once configuration
void Game::initialize() {
    config->loadConfig("data/config/game.lua");

    // Extract commonly-used values into member variables
    physicsGravity_ = config->getFloatOr("physics.gravity", 980.0f);
    playerMaxHealth_ = config->getIntOr("player.combat.maxHealth", 100);
}

// Pattern 2: Development with hot reload
void Game::initializeDev() {
    config->loadConfig("data/config/game.lua");
    config->enableHotReload(true);

    // Re-apply config when changed
    config->onConfigChanged([this](const ConfigKey&) {
        applyGameConfig();
    });
}

// Pattern 3: Per-system config with notifications
class PlayerSystem {
    void initialize(IConfigSystem* config) {
        // Subscribe to player config changes only
        configSub_ = config->onKeyChanged("player.", [this](const ConfigKey& key) {
            loadPlayerConfig();
        });

        loadPlayerConfig();
    }

    void loadPlayerConfig() {
        maxSpeed_ = config_->getFloatOr("player.physics.speed", 200.0f);
        jumpForce_ = config_->getFloatOr("player.physics.jumpForce", 450.0f);
        maxJumps_ = config_->getIntOr("player.jump.maxJumps", 2);
    }

    SubscriptionId configSub_;
};
```

**Integration with Asset System:**

```cpp
// Load config from asset handle (for packaged builds)
AssetHandle configAsset = assets->registerAsset(
    AssetType::Data,
    "configs/game.lua"
);

config->loadConfigAsset(configAsset);
```

**Why Lua for Configuration:**

| Feature | Lua | JSON | Benefit |
|---------|-----|------|---------|
| Comments | ✓ | ✗ | Document config values |
| Trailing commas | ✓ | ✗ | Less merge conflicts |
| Variables | ✓ | ✗ | `local SPEED = 200; fast = SPEED * 2` |
| Math | ✓ | ✗ | `gravity = 9.8 * 100` |
| Conditionals | ✓ | ✗ | `debug and {...} or {...}` |
| Arrays | ✓ | ✓ | Both support lists |
| Nested tables | ✓ | ✓ | Both support hierarchy |

---

## 9. Data Formats

### 9.1 Data Format Strategy

| Data Type | Format | Reason |
|-----------|--------|--------|
| Blueprints | Lua | Inheritance, functions, reusable components |
| Levels | Lua | Loops, patterns, procedural placement |
| Game Config | Lua | Variables, computed values |
| User Settings | JSON | Simple key-value, saved by game at runtime |
| Save Files | Binary (cereal) | Fast, compact, versioned |

### 9.2 Blueprint Format (Lua)

```lua
-- data/blueprints/enemies/slime.lua

local Components = require("blueprints.helpers")

return {
    id = "enemies/slime",
    displayName = "Green Slime",
    tags = { "enemy", "ground", "bouncy" },
    
    -- Inherit from base enemy (optional)
    parent = "enemies/base_enemy",
    
    components = {
        transform = {},  -- Position set at spawn time
        
        sprite = {
            textureAsset = "textures/enemies/slime.png",
            sourceW = 32,
            sourceH = 24,
            layer = 5,
            anchor = { x = 0.5, y = 0.0 }  -- Bottom center
        },
        
        physics = Components.dynamicBody({
            width = 28,
            height = 20,
            categoryBits = 0x0002,  -- ENEMY layer
            maskBits = 0x000D       -- Collides with PLAYER, TERRAIN, PROJECTILE
        }),
        
        health = {
            maxHealth = 20,
            invincibilityTime = 0.5
        },
        
        aiController = {
            behaviorTree = "ai/behaviors/slime.xml",
            detectionRange = 150,
            attackRange = 30
        },
        
        -- Custom component data
        slimeBehavior = {
            bounceForce = 300,
            moveSpeed = 50,
            jumpCooldown = 2.0
        }
    }
}
```

**Blueprint Helpers:**

```lua
-- data/blueprints/helpers.lua

local C = {}

-- Reusable physics body configurations
function C.dynamicBody(opts)
    return {
        bodyType = "dynamic",
        width = opts.width or 32,
        height = opts.height or 32,
        density = opts.density or 1.0,
        friction = opts.friction or 0.3,
        restitution = opts.restitution or 0.0,
        fixedRotation = opts.fixedRotation ~= false,
        categoryBits = opts.categoryBits or 0x0001,
        maskBits = opts.maskBits or 0xFFFF
    }
end

function C.staticBody(opts)
    local body = C.dynamicBody(opts)
    body.bodyType = "static"
    return body
end

function C.sensor(opts)
    local body = C.dynamicBody(opts)
    body.isSensor = true
    return body
end

-- Reusable sprite configurations
function C.animatedSprite(opts)
    return {
        textureAsset = opts.texture,
        frameWidth = opts.frameWidth,
        frameHeight = opts.frameHeight,
        frameCount = opts.frameCount,
        frameRate = opts.frameRate or 12,
        layer = opts.layer or 0,
        looping = opts.looping ~= false
    }
end

-- Collision layer constants
C.Layers = {
    PLAYER     = 0x0001,
    ENEMY      = 0x0002,
    PROJECTILE = 0x0004,
    TERRAIN    = 0x0008,
    TRIGGER    = 0x0010,
    ITEM       = 0x0020
}

return C
```

### 9.3 Level Format (Lua)

```lua
-- data/levels/world1/level1.lua

local H = require("levels.helpers")
local C = require("blueprints.helpers")

-- Local constants for this level
local GROUND_Y = 100
local LEVEL_WIDTH = 3200
local COIN_Y = 250

return {
    id = "world1/level1",
    displayName = "Green Hills",
    
    size = {
        width = LEVEL_WIDTH,
        height = 600
    },
    
    physics = {
        gravity = { x = 0, y = -980 }
    },
    
    background = {
        asset = "textures/backgrounds/hills.png",
        parallax = { x = 0.3, y = 0.1 }
    },
    
    music = {
        asset = "audio/music/hills.ogg",
        volume = 0.7,
        fadeIn = 2.0
    },
    
    -- Tile layers (can also use loops for procedural generation)
    tileLayers = {
        {
            name = "ground",
            tileset = "textures/tilesets/grass.png",
            tileSize = 32,
            collision = true,
            data = H.loadTilemap("levels/world1/level1_ground.csv")
        }
    },
    
    -- Here's where Lua really shines!
    entities = H.concat(
        -- Player spawn point
        {{
            id = "player_spawn",
            blueprint = "player",
            x = 100,
            y = GROUND_Y + 50
        }},
        
        -- Row of coins using a loop
        H.map(H.range(0, 9), function(i)
            return {
                id = "coin_" .. i,
                blueprint = "collectibles/coin",
                x = 300 + (i * 50),
                y = COIN_Y
            }
        end),
        
        -- Coins in a sine wave pattern
        H.wave("collectibles/coin", 800, COIN_Y, 15, 40, 30, 0.3, "wave_coin"),
        
        -- Grid of breakable blocks
        H.grid("interactables/brick", 600, 350, 5, 2, 32, 32, "brick"),
        
        -- Enemies at specific positions
        H.map({
            { x = 500, y = GROUND_Y + 20 },
            { x = 900, y = GROUND_Y + 20 },
            { x = 1400, y = GROUND_Y + 20 },
            { x = 2000, y = 300 }  -- On a platform
        }, function(pos, i)
            return {
                id = "slime_" .. i,
                blueprint = "enemies/slime",
                x = pos.x,
                y = pos.y
            }
        end),
        
        -- Arc of collectibles around a secret area
        H.arc("collectibles/gem", 2500, 400, 100, -45, 225, 7, "secret_gem"),
        
        -- Conditional debug entities
        H.ifDebug({
            { id = "debug_warp", blueprint = "debug/warp", x = 50, y = 300 },
            { id = "debug_god_mode", blueprint = "debug/powerup", x = 100, y = 300 }
        })
    ),
    
    -- Spawn points for level transitions and respawns
    spawnPoints = {
        player_start = { x = 100, y = GROUND_Y + 50 },
        from_level2 = { x = LEVEL_WIDTH - 100, y = GROUND_Y + 50 },
        checkpoint_1 = { x = 1500, y = GROUND_Y + 50 },
        secret_area = { x = 2500, y = 450 }
    },
    
    -- Trigger regions
    regions = {
        {
            name = "exit_right",
            type = "transition",
            bounds = { x = LEVEL_WIDTH - 32, y = 0, width = 32, height = 600 },
            targetLevel = "world1/level2",
            targetSpawn = "from_level1"
        },
        {
            name = "death_zone",
            type = "kill",
            bounds = { x = 0, y = -100, width = LEVEL_WIDTH, height = 100 }
        },
        {
            name = "secret_entrance",
            type = "trigger",
            bounds = { x = 2400, y = 300, width = 64, height = 64 },
            onEnter = "reveal_secret_area"
        }
    },
    
    -- Level connections for the world map
    connections = {
        { direction = "right", targetLevel = "world1/level2" },
        { direction = "secret", targetLevel = "world1/secret1", condition = "flag:found_secret_key" }
    }
}
```

**Level Helpers:**

```lua
-- data/levels/helpers.lua

local H = {}

-- Basic utilities
function H.range(start, stop)
    local t = {}
    for i = start, stop do t[#t + 1] = i end
    return t
end

function H.map(array, fn)
    local result = {}
    for i, v in ipairs(array) do
        result[i] = fn(v, i)
    end
    return result
end

function H.concat(...)
    local result = {}
    for _, arr in ipairs({...}) do
        if arr then
            for _, v in ipairs(arr) do
                result[#result + 1] = v
            end
        end
    end
    return result
end

-- Pattern generators
function H.grid(blueprint, startX, startY, cols, rows, spacingX, spacingY, idPrefix)
    local entities = {}
    for row = 0, rows - 1 do
        for col = 0, cols - 1 do
            entities[#entities + 1] = {
                id = idPrefix .. "_" .. row .. "_" .. col,
                blueprint = blueprint,
                x = startX + (col * spacingX),
                y = startY + (row * spacingY)
            }
        end
    end
    return entities
end

function H.arc(blueprint, cx, cy, radius, startDeg, endDeg, count, idPrefix)
    local entities = {}
    local step = (endDeg - startDeg) / (count - 1)
    for i = 0, count - 1 do
        local angle = math.rad(startDeg + (i * step))
        entities[#entities + 1] = {
            id = idPrefix .. "_" .. i,
            blueprint = blueprint,
            x = cx + math.cos(angle) * radius,
            y = cy + math.sin(angle) * radius
        }
    end
    return entities
end

function H.wave(blueprint, startX, baseY, count, spacing, amplitude, frequency, idPrefix)
    local entities = {}
    for i = 0, count - 1 do
        entities[#entities + 1] = {
            id = idPrefix .. "_" .. i,
            blueprint = blueprint,
            x = startX + (i * spacing),
            y = baseY + math.sin(i * frequency) * amplitude
        }
    end
    return entities
end

function H.line(blueprint, x1, y1, x2, y2, count, idPrefix)
    local entities = {}
    for i = 0, count - 1 do
        local t = count > 1 and (i / (count - 1)) or 0
        entities[#entities + 1] = {
            id = idPrefix .. "_" .. i,
            blueprint = blueprint,
            x = x1 + (x2 - x1) * t,
            y = y1 + (y2 - y1) * t
        }
    end
    return entities
end

-- Conditional helpers
function H.ifDebug(entities)
    if os.getenv("BESTOW_DEBUG") == "1" then
        return entities
    end
    return {}
end

function H.ifFlag(flag, entities)
    -- This will be resolved at runtime by the level loader
    return {
        __conditional = true,
        condition = "flag:" .. flag,
        entities = entities
    }
end

-- CSV tilemap loader
function H.loadTilemap(path)
    -- Implementation reads CSV and returns 2D array
    -- Called at Lua execution time
    return {} -- placeholder
end

return H
```

> **📝 Future Tool: Tiled-to-Lua Converter**
> 
> Create a `tools/tiled-to-lua/` converter that transforms Tiled editor JSON exports into Lua level files. This would allow using Tiled for visual tile placement while still benefiting from Lua's programmability for entity placement and level logic. The converter should preserve tile layers, object layers, and custom properties, outputting clean Lua that can be further edited by hand.

### 9.4 Settings Format (JSON)

Settings remain in JSON since they're machine-generated (saved by the game) and don't need computation:

```json
{
    "version": 1,
    
    "audio": {
        "masterVolume": 1.0,
        "musicVolume": 0.7,
        "sfxVolume": 1.0,
        "voiceVolume": 1.0
    },
    
    "video": {
        "resolutionWidth": 1920,
        "resolutionHeight": 1080,
        "fullscreen": false,
        "vsync": true,
        "targetFps": 60
    },
    
    "controls": {
        "bindings": [
            {
                "action": "jump",
                "primary": { "device": "keyboard", "key": 32 },
                "secondary": { "device": "controller", "button": 0 }
            },
            {
                "action": "move_horizontal",
                "primary": { "device": "keyboard", "keys": [65, 68], "scale": [-1, 1] },
                "secondary": { "device": "controller", "axis": 0 }
            }
        ]
    },
    
    "accessibility": {
        "screenShake": true,
        "flashingEffects": true,
        "subtitles": false,
        "colorblindMode": "none"
    }
}
```

### 9.5 Lua Security (Sandboxing)

When loading Lua files, dangerous functions are removed:

```cpp
// Safe Lua environment setup
void setupSandbox(sol::state& lua) {
    // Allow safe libraries
    lua.open_libraries(
        sol::lib::base,    // print, type, pairs, ipairs, etc.
        sol::lib::math,    // math.sin, math.cos, etc.
        sol::lib::string,  // string manipulation
        sol::lib::table    // table.insert, table.sort, etc.
    );
    
    // Remove dangerous functions
    lua["os"] = sol::nil;           // No OS access
    lua["io"] = sol::nil;           // No file I/O
    lua["loadfile"] = sol::nil;     // No loading arbitrary files
    lua["dofile"] = sol::nil;       // No executing arbitrary files
    lua["load"] = sol::nil;         // No loading arbitrary strings
    lua["rawget"] = sol::nil;       // No bypassing metatables
    lua["rawset"] = sol::nil;
    
    // Custom require that only loads from data/ directory
    lua["require"] = [&lua](const std::string& module) {
        // Convert "levels.helpers" to "data/levels/helpers.lua"
        auto path = resolveModulePath(module);
        if (!isPathSafe(path)) {
            throw std::runtime_error("Cannot require module outside data/");
        }
        return lua.script_file(path.string());
    };
}
```
    
    "inputBindings": [
        {
            "action": "move_horizontal",
            "bindings": [
                {"deviceType": "keyboard", "keyCode": 65, "scale": -1.0},
                {"deviceType": "keyboard", "keyCode": 68, "scale": 1.0},
                {"deviceType": "controller", "keyCode": 0, "deadzone": 0.2}
            ]
        }
    ],
    
    "audio": {
        "masterVolume": 1.0,
        "musicVolume": 0.7,
        "sfxVolume": 1.0
    },
    
    "video": {
        "resolutionWidth": 1920,
        "resolutionHeight": 1080,
        "fullscreen": false,
        "vsync": true
    }
}
```

---

## 10. Build Configuration

### 10.1 Root CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.28)  # Required for good module support

project(bestow 
    VERSION 1.0.0 
    LANGUAGES CXX
    DESCRIPTION "Bestow Game Framework"
)

# C++23 with modules
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)  # Enable module scanning

# Options
option(BESTOW_BUILD_TESTS "Build tests" ON)
option(BESTOW_BUILD_EXAMPLES "Build examples" ON)
option(BESTOW_ENABLE_TRACY "Enable Tracy profiler" ON)
option(BESTOW_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(BESTOW_DEV_TOOLS "Enable development tools (hot reload, inspector)" ON)

# vcpkg toolchain (if using vcpkg)
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
endif()

# Include cmake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerWarnings)
include(StaticAnalysis)
include(Dependencies)

# Find dependencies
find_dependencies()

# Add subdirectories
add_subdirectory(bestow-contract)
add_subdirectory(bestow-core)
add_subdirectory(bestow-entity)
add_subdirectory(bestow-graphics)
add_subdirectory(bestow-audio)
add_subdirectory(bestow-input)
add_subdirectory(bestow-assets)
add_subdirectory(bestow-save)
add_subdirectory(bestow-level)
add_subdirectory(bestow-events)
add_subdirectory(bestow-physics)
add_subdirectory(bestow-ai)

# Development tools (only in debug builds)
if(BESTOW_DEV_TOOLS)
    add_subdirectory(bestow-dev)
endif()

if(BESTOW_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(BESTOW_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
```

### 10.2 Module Library CMake Example (bestow-contract)

```cmake
# bestow-contract/CMakeLists.txt

add_library(bestow-contract)

# Specify module sources
target_sources(bestow-contract
    PUBLIC FILE_SET CXX_MODULES FILES
        src/bestow.types.cppm
        src/bestow.entity.cppm
        src/bestow.graphics.cppm
        src/bestow.audio.cppm
        src/bestow.input.cppm
        src/bestow.assets.cppm
        src/bestow.save.cppm
        src/bestow.level.cppm
        src/bestow.events.cppm
        src/bestow.physics.cppm
        src/bestow.ai.cppm
        src/bestow.cppm  # Primary module that re-exports all
)

# Dependencies
target_link_libraries(bestow-contract
    PUBLIC
        glm::glm
        EnTT::EnTT
)
```

### 10.3 Primary Module Interface (bestow.cppm)

```cpp
// bestow-contract/src/bestow.cppm
// Re-exports all Bestow interfaces for convenient single-import usage

export module bestow;

export import bestow.types;
export import bestow.entity;
export import bestow.graphics;
export import bestow.audio;
export import bestow.input;
export import bestow.assets;
export import bestow.save;
export import bestow.level;
export import bestow.events;
export import bestow.physics;
export import bestow.ai;
```

### 10.4 Implementation Module Example (bestow-entity)

```cmake
# bestow-entity/CMakeLists.txt

add_library(bestow-entity)

target_sources(bestow-entity
    PUBLIC FILE_SET CXX_MODULES FILES
        src/bestow.entity.impl.cppm
    PRIVATE
        src/EntitySystem.cpp
)

target_link_libraries(bestow-entity
    PUBLIC
        bestow-contract
        EnTT::EnTT
)
```

```cpp
// bestow-entity/src/bestow.entity.impl.cppm

module;

#include <entt/entt.hpp>

export module bestow.entity.impl;

import bestow.entity;
import std;

export namespace bestow {

class EntitySystem : public IEntitySystem {
public:
    Entity createEntity() override {
        return registry_.create();
    }
    
    void destroyEntity(Entity entity) override {
        registry_.destroy(entity);
    }
    
    bool isValid(Entity entity) const override {
        return registry_.valid(entity);
    }
    
    // ... rest of implementation
    
private:
    entt::registry registry_;
};

}  // namespace bestow
```

### 10.5 CMakePresets.json

```json
{
    "version": 6,
    "configurePresets": [
        {
            "name": "base",
            "hidden": true,
            "binaryDir": "${sourceDir}/build/${presetName}",
            "installDir": "${sourceDir}/install/${presetName}",
            "cacheVariables": {
                "CMAKE_EXPORT_COMPILE_COMMANDS": "ON"
            }
        },
        {
            "name": "macos-debug",
            "inherits": "base",
            "displayName": "macOS Debug",
            "generator": "Ninja",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Debug",
                "BESTOW_ENABLE_ASAN": "ON"
            },
            "condition": {
                "type": "equals",
                "lhs": "${hostSystemName}",
                "rhs": "Darwin"
            }
        },
        {
            "name": "macos-release",
            "inherits": "base",
            "displayName": "macOS Release",
            "generator": "Ninja",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Release"
            },
            "condition": {
                "type": "equals",
                "lhs": "${hostSystemName}",
                "rhs": "Darwin"
            }
        },
        {
            "name": "windows-debug",
            "inherits": "base",
            "displayName": "Windows Debug",
            "generator": "Visual Studio 17 2022",
            "architecture": "x64",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Debug"
            },
            "condition": {
                "type": "equals",
                "lhs": "${hostSystemName}",
                "rhs": "Windows"
            }
        },
        {
            "name": "windows-release",
            "inherits": "base",
            "displayName": "Windows Release",
            "generator": "Visual Studio 17 2022",
            "architecture": "x64",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Release"
            },
            "condition": {
                "type": "equals",
                "lhs": "${hostSystemName}",
                "rhs": "Windows"
            }
        }
    ],
    "buildPresets": [
        {
            "name": "macos-debug",
            "configurePreset": "macos-debug"
        },
        {
            "name": "macos-release",
            "configurePreset": "macos-release"
        },
        {
            "name": "windows-debug",
            "configurePreset": "windows-debug"
        },
        {
            "name": "windows-release",
            "configurePreset": "windows-release"
        }
    ],
    "testPresets": [
        {
            "name": "macos-debug",
            "configurePreset": "macos-debug",
            "output": {"outputOnFailure": true}
        },
        {
            "name": "windows-debug",
            "configurePreset": "windows-debug",
            "output": {"outputOnFailure": true}
        }
    ]
}
```

### 10.7 .clang-format

```yaml
---
Language: Cpp
BasedOnStyle: LLVM

# Indentation
IndentWidth: 4
TabWidth: 4
UseTab: Never
IndentCaseLabels: false
NamespaceIndentation: None

# Braces
BreakBeforeBraces: Attach
Cpp11BracedListStyle: true

# Line Length
ColumnLimit: 100

# Includes
SortIncludes: CaseSensitive
IncludeBlocks: Regroup
IncludeCategories:
  - Regex: '^<bestow/'
    Priority: 2
  - Regex: '^<(entt|glm|fmt|spdlog|nlohmann)'
    Priority: 3
  - Regex: '^<'
    Priority: 4
  - Regex: '.*'
    Priority: 1

# Alignment
AlignAfterOpenBracket: Align
AlignConsecutiveAssignments: false
AlignConsecutiveDeclarations: false
AlignOperands: true
AlignTrailingComments: true

# Other
AllowShortBlocksOnASingleLine: Empty
AllowShortCaseLabelsOnASingleLine: false
AllowShortFunctionsOnASingleLine: Inline
AllowShortIfStatementsOnASingleLine: Never
AllowShortLoopsOnASingleLine: false
AlwaysBreakTemplateDeclarations: Yes
BinPackArguments: true
BinPackParameters: true
BreakConstructorInitializers: BeforeColon
PointerAlignment: Left
SpaceAfterCStyleCast: false
SpaceBeforeParens: ControlStatements
SpacesInAngles: false
```

---

## Appendix A: Quick Reference

### System Dependencies at a Glance

| System | Depends On |
|--------|------------|
| Entity | EnTT |
| Graphics | GLFW, glm, stb_image, FreeType, ImGui |
| Audio | FMOD |
| Input | GLFW (keyboard/mouse), SDL2 (controllers) |
| Assets | stb_image, zstd, taskflow |
| Save | cereal, zstd, nlohmann/json |
| Level | Lua, sol2 |
| Events | (none) |
| Physics | Box2D |
| AI | BehaviorTree.CPP, Recast/Detour |
| Dev (debug only) | efsw, ImGui, sol2 |

### Build Commands

```bash
# Configure (macOS)
cmake --preset macos-debug

# Build
cmake --build --preset macos-debug

# Test
ctest --preset macos-debug

# Configure (Windows)
cmake --preset windows-debug

# Build
cmake --build --preset windows-debug --config Debug

# Release build (no dev tools)
cmake --preset macos-release
cmake --build --preset macos-release
```

### Key File Locations

| What | Where |
|------|-------|
| Module interfaces | `bestow-contract/src/*.cppm` |
| Primary module | `bestow-contract/src/bestow.cppm` |
| Implementations | `bestow-*/src/*.cpp` |
| Dev tools | `bestow-dev/src/` |
| Tests | `tests/` |
| Examples | `examples/` |
| Build output | `build/<preset>/` |

### Module Import Patterns

```cpp
// Import everything
import bestow;

// Import specific modules
import bestow.entity;
import bestow.graphics;

// In game code
import bestow;
import bestow.dev;  // Only available in debug builds
```

### Hot Reload Quick Reference

| Action | Result |
|--------|--------|
| Edit `blueprints/*.lua` | Lua re-executes, blueprint registry updates |
| Edit `levels/*.lua` | Lua re-executes, current level reloads (if active) |
| Edit `textures/*` | Texture reloads in GPU |
| Edit `config/*.lua` | Lua re-executes, settings apply immediately |
| Press F1 | Toggle dev overlay |
| Click entity | Show inspector |
| "Copy Position" | Lua syntax to clipboard |

### Lua Pattern Cheat Sheet

```lua
-- Place 10 coins in a row
H.map(H.range(0, 9), function(i)
  return { id = "coin_"..i, blueprint = "coin", x = 100 + i*50, y = 200 }
end)

-- Grid of enemies (3 columns, 2 rows)
H.grid("enemies/slime", 500, 200, 3, 2, 100, 80, "slime")

-- Arc of items around a point
H.arc("collectibles/gem", 400, 300, 100, 0, 180, 7, "gem")

-- Sine wave of platforms
H.wave("platforms/cloud", 100, 400, 10, 80, 50, 0.5, "cloud")

-- Conditional debug content
H.ifDebug({ { id = "warp", blueprint = "debug/warp", x = 50, y = 50 } })
```

---

## 11. Feature Roadmap

### 11.1 Current Status

Bestow is in active development with core systems implemented and enhanced features planned.

**Completed Systems:**
- Event System - Fully functional publish/subscribe
- Entity System - EnTT-based ECS
- Physics System - Box2D integration with collision detection
- Asset System - Async loading with hot reload
- Save System - Binary serialization with cereal
- Input System - GLFW keyboard/mouse + SDL2 gamepad
- Audio System - FMOD-based 3D positional audio
- Level System - Lua-based level loading with metadata parsing

**In Progress:**
- Graphics System - Basic rendering functional, sprites/animation/text planned
- Camera System - Planned
- Components Library - Planned
- Dev Tools - Hot reload implemented, inspector/overlay planned

### 11.2 Feature Gap Closure Plan

Based on the platformer example requirements, the following features are being added to make Bestow production-ready.

#### Phase 1: Critical Rendering Features

**Status:** Planned
**Timeline:** Priority 1

- **Sprite Rendering**
  - SpriteSheet support with frame indexing
  - Automatic sprite batching for performance
  - Texture atlas support
  - Z-order/layer sorting

- **Text Rendering**
  - TrueType font loading via FreeType
  - Multi-size font rendering
  - Text alignment (left, center, right)
  - Text measurement for layout

- **Physics Triggers**
  - Sensor body support (`isSensor` flag)
  - TriggerEnter/TriggerExit events
  - Integration with Event System
  - Use cases: collectibles, damage zones, level transitions

**Files Modified:**
- `bestow-contract/src/bestow.types.cppm` - Add SpriteSheet, Animation types
- `bestow-contract/src/bestow.graphics.cppm` - Add sprite/text methods
- `bestow-contract/src/bestow.physics.cppm` - Add trigger callbacks
- `bestow-graphics/src/GraphicsSystem.cpp` - Implement rendering
- `bestow-physics/src/Box2DPhysicsSystem.cpp` - Implement sensors

#### Phase 2: Animation & Camera

**Status:** Planned
**Timeline:** Priority 2

- **Animation System**
  - Animation definition (frame sequences, durations, looping)
  - AnimatedSprite component
  - Automatic frame advancement
  - Animation blending/transitions
  - Event callbacks (onComplete, onFrame)

- **Camera System**
  - Target following with smoothing
  - Deadzone-based follow
  - Camera bounds/constraints
  - Camera shake effects
  - Screen-to-world coordinate conversion
  - Zoom support

**New Files:**
- `bestow-contract/src/bestow.camera.cppm` - ICameraSystem interface
- `bestow-camera/src/CameraSystem.cpp` - Implementation
- `bestow-camera/CMakeLists.txt` - Build config

**Files Modified:**
- `bestow-contract/src/bestow.types.cppm` - Add Animation, Camera types
- `bestow-graphics/src/GraphicsSystem.cpp` - Animation update/rendering

#### Phase 3: Level System Enhancements

**Status:** Planned
**Timeline:** Priority 2

- **Entity Spawning**
  - Parse entity definitions from Lua level files
  - Resolve blueprint references
  - Apply level-specific property overrides
  - `spawnEntities()` method for automatic entity creation
  - Spawn point management

- **Blueprint System Integration**
  - Blueprint registry
  - Component application from blueprints
  - Blueprint inheritance (planned)

**Files Modified:**
- `bestow-contract/src/bestow.level.cppm` - Add spawn methods
- `bestow-level/src/LevelSystem.cpp` - Implement entity spawning
- `bestow-level/src/bestow.level.impl.cppm` - Add EntitySpawnDef

#### Phase 4: Components Library

**Status:** Planned
**Timeline:** Priority 3

- **Common Components**
  - Movement: Velocity, Acceleration, MaxSpeed
  - Combat: Health, Damage, Invincibility
  - Gameplay: Timer, Lifetime
  - Physics: GroundDetector (coyote time support)
  - Visual: Sprite, AnimatedSprite
  - Audio: SoundEmitter

- **Component Utilities**
  - Standard update patterns
  - Common helper functions
  - Documentation and examples

**New Files:**
- `bestow-components/src/bestow.components.cppm` - Component definitions
- `bestow-components/CMakeLists.txt` - Build config

#### Phase 5: Dev Tools Enhancement

**Status:** Partially Complete
**Timeline:** Ongoing

- **Completed:**
  - Hot reload for Lua files
  - Asset hot reload

- **Planned:**
  - Entity inspector overlay
  - Performance profiler display
  - Console/logging overlay
  - Camera debug visualization
  - Physics debug rendering (shapes, contact points)

### 11.3 Example Game Requirements

The platformer example will demonstrate all features:

- Animated player character (idle, run, jump, fall, hurt)
- Animated enemies with AI behaviors
- Parallax scrolling backgrounds
- HUD with health bar, score, timer
- Sound effects (jump, land, hurt, collect, death)
- Background music with transitions
- Multiple levels with save/load
- Checkpoints and respawning
- Collectibles (coins, gems)
- Triggers (level transitions, hazards)
- Camera following with smoothing
- Particle effects

### 11.4 Platform Roadmap

**Current:** macOS, Windows, Linux
**Planned:** Nintendo Switch

**Switch Requirements:**
- Vulkan renderer migration (replace OpenGL)
- Controller-only input support
- Optimized asset loading for limited memory
- Save system Nintendo SDK integration

### 11.5 Success Criteria

Bestow will be considered feature-complete when:

1. All systems have complete implementations (no stubs)
2. Platformer example runs with full visual/audio polish
3. No manual Lua parsing required in game code
4. Common platformer mechanics work out-of-box
5. Game code focuses on game logic (90%+ ratio)
6. All unit tests passing
7. Documentation complete for all systems

---

*End of Technical Design Document*
