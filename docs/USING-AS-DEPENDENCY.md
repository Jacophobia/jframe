# Using Bestow as a Dependency

This guide explains how to use Bestow as a dependency in your own projects.

## Options

There are three ways to use Bestow in your project:

1. **CMake FetchContent** (Recommended) - Automatically downloads and builds Bestow
2. **Prebuilt Binaries** - Download from GitHub Releases
3. **Git Submodule** - Include Bestow source in your repository

---

## Option 1: CMake FetchContent (Recommended)

This is the simplest approach. CMake will download and build Bestow automatically.

### Prerequisites

Your project must use the same compiler requirements as Bestow:
- **LLVM Clang 20+** on all platforms
- **C++23** with module support
- **vcpkg** for dependency management

### CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.28)
project(my_game LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    bestow
    GIT_REPOSITORY https://github.com/Jacophobia/bestow.git
    GIT_TAG        v1.0.0  # Or a specific commit hash
)

# Don't build tests/examples when used as dependency
set(BESTOW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BESTOW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(bestow)

# Your game executable
add_executable(my_game main.cpp)

# Link to Bestow
target_link_libraries(my_game PRIVATE bestow-core)
```

### Using Bestow Modules

```cpp
// main.cpp
import std;
import bestow;

int main() {
    // Use Bestow types and systems
    bestow::Engine engine;
    engine.run();
    return 0;
}
```

---

## Option 2: Prebuilt Binaries

Download prebuilt libraries from [GitHub Releases](https://github.com/Jacophobia/bestow/releases).

### Available Platforms

| Platform | Architecture | Filename |
|----------|--------------|----------|
| macOS | Apple Silicon (arm64) | `bestow-macos-arm64.tar.gz` |
| macOS | Intel (x64) | `bestow-macos-x64.tar.gz` |
| Linux | x64 | `bestow-linux-x64.tar.gz` |
| Windows | x64 | `bestow-windows-x64.zip` |

### Installation

1. Download the appropriate archive for your platform
2. Extract to a known location (e.g., `~/libs/bestow` or `C:\libs\bestow`)
3. Configure CMake to find Bestow:

```cmake
cmake_minimum_required(VERSION 3.28)
project(my_game LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Tell CMake where to find Bestow
set(bestow_DIR "/path/to/bestow/lib/cmake/bestow")

find_package(bestow REQUIRED)

add_executable(my_game main.cpp)
target_link_libraries(my_game PRIVATE bestow::bestow-core)
```

Or pass on the command line:
```bash
cmake -Dbestow_DIR=/path/to/bestow/lib/cmake/bestow ..
```

---

## Option 3: Git Submodule

Include Bestow as a submodule in your repository.

### Setup

```bash
# Add Bestow as a submodule
git submodule add https://github.com/Jacophobia/bestow.git external/bestow
git submodule update --init --recursive
```

### CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.28)
project(my_game LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Don't build tests/examples
set(BESTOW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BESTOW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(external/bestow)

add_executable(my_game main.cpp)
target_link_libraries(my_game PRIVATE bestow-core)
```

### Updating Bestow

```bash
cd external/bestow
git fetch origin
git checkout v1.1.0  # Or desired version
cd ../..
git add external/bestow
git commit -m "Update Bestow to v1.1.0"
```

---

## Available Targets

When linking to Bestow, you can use these targets:

| Target | Description |
|--------|-------------|
| `bestow-core` | Main framework (includes all systems) |
| `bestow-entity` | Entity Component System (EnTT) |
| `bestow-graphics` | Graphics/Rendering (OpenGL) |
| `bestow-audio` | Audio (FMOD) |
| `bestow-input` | Input handling (GLFW, SDL2) |
| `bestow-physics` | Physics (Box2D) |
| `bestow-assets` | Asset management |
| `bestow-events` | Event system |
| `bestow-save` | Save/Load (cereal) |
| `bestow-level` | Level management (Lua) |
| `bestow-ai` | AI/Pathfinding (Recast/Detour) |
| `bestow-camera` | Camera system |
| `bestow-config` | Configuration (Lua) |
| `bestow-gas` | Gameplay Ability System |
| `bestow-blueprints` | Blueprint/Prefab system |
| `bestow-dev` | Development tools (debug only) |

For most games, just link to `bestow-core` - it brings in all dependencies.

---

## vcpkg Integration

Bestow uses vcpkg for its dependencies. Your project should also use vcpkg.

### CMakePresets.json (Example)

```json
{
    "version": 6,
    "configurePresets": [
        {
            "name": "default",
            "toolchainFile": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake",
            "cacheVariables": {
                "CMAKE_CXX_STANDARD": "23"
            }
        }
    ]
}
```

### vcpkg.json (Your Project)

You don't need to duplicate Bestow's dependencies - they're brought in automatically through CMake. But if you want explicit control:

```json
{
    "name": "my-game",
    "version": "1.0.0",
    "dependencies": [
        "glfw3",
        "glad",
        "glm",
        "entt",
        "spdlog"
    ]
}
```

---

## FMOD Note

Bestow's audio system uses FMOD, which requires manual installation:

1. Download FMOD Core API from [fmod.com/download](https://www.fmod.com/download)
2. Place in your project's `external/fmod/core/` directory
3. See [Installation.md](Installation.md) for detailed instructions

If you don't need audio, you can exclude the audio system by not linking to it.

---

## Compiler Requirements

Bestow requires C++23 with `import std;` support:

| Platform | Compiler | Version |
|----------|----------|---------|
| macOS | LLVM Clang | 20+ |
| Linux | LLVM Clang | 20+ |
| Windows | LLVM Clang | 20+ |

Your project must use the same compiler family and version for ABI compatibility.

---

## Example Project Structure

```
my-game/
├── CMakeLists.txt
├── CMakePresets.json
├── vcpkg.json
├── src/
│   └── main.cpp
├── assets/
│   ├── textures/
│   ├── audio/
│   └── levels/
└── external/
    └── fmod/
        └── core/
            ├── inc/
            └── lib/
```

---

## Troubleshooting

### "Cannot find module 'bestow'"

Ensure you're linking to Bestow targets and using C++23 modules:

```cmake
target_link_libraries(my_game PRIVATE bestow-core)
```

### "ABI mismatch" or "undefined symbols"

Your project must use the same compiler (LLVM Clang 20+) as Bestow. Mixing compilers will cause linker errors.

### "vcpkg dependencies not found"

Ensure `VCPKG_ROOT` is set and the toolchain file is configured:

```bash
export VCPKG_ROOT=~/vcpkg
cmake --preset default
```

### Version Compatibility

Bestow follows semantic versioning:
- **Major** versions may have breaking API changes
- **Minor** versions add features, backward compatible
- **Patch** versions are bug fixes only

Use `find_package(bestow 1.0 REQUIRED)` to require a minimum version.
