# Using JFrame as a Dependency

This guide explains how to use JFrame as a dependency in your own projects.

## Options

There are three ways to use JFrame in your project:

1. **CMake FetchContent** (Recommended) - Automatically downloads and builds JFrame
2. **Prebuilt Binaries** - Download from GitHub Releases
3. **Git Submodule** - Include JFrame source in your repository

---

## Option 1: CMake FetchContent (Recommended)

This is the simplest approach. CMake will download and build JFrame automatically.

### Prerequisites

Your project must use the same compiler requirements as JFrame:
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
    jframe
    GIT_REPOSITORY https://github.com/Jacophobia/jframe.git
    GIT_TAG        v1.0.0  # Or a specific commit hash
)

# Don't build tests/examples when used as dependency
set(JFRAME_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(JFRAME_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(jframe)

# Your game executable
add_executable(my_game main.cpp)

# Link to JFrame
target_link_libraries(my_game PRIVATE jframe-core)
```

### Using JFrame Modules

```cpp
// main.cpp
import std;
import jframe;

int main() {
    // Use JFrame types and systems
    jframe::Engine engine;
    engine.run();
    return 0;
}
```

---

## Option 2: Prebuilt Binaries

Download prebuilt libraries from [GitHub Releases](https://github.com/Jacophobia/jframe/releases).

### Available Platforms

| Platform | Architecture | Filename |
|----------|--------------|----------|
| macOS | Apple Silicon (arm64) | `jframe-macos-arm64.tar.gz` |
| macOS | Intel (x64) | `jframe-macos-x64.tar.gz` |
| Linux | x64 | `jframe-linux-x64.tar.gz` |
| Windows | x64 | `jframe-windows-x64.zip` |

### Installation

1. Download the appropriate archive for your platform
2. Extract to a known location (e.g., `~/libs/jframe` or `C:\libs\jframe`)
3. Configure CMake to find JFrame:

```cmake
cmake_minimum_required(VERSION 3.28)
project(my_game LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Tell CMake where to find JFrame
set(jframe_DIR "/path/to/jframe/lib/cmake/jframe")

find_package(jframe REQUIRED)

add_executable(my_game main.cpp)
target_link_libraries(my_game PRIVATE jframe::jframe-core)
```

Or pass on the command line:
```bash
cmake -Djframe_DIR=/path/to/jframe/lib/cmake/jframe ..
```

---

## Option 3: Git Submodule

Include JFrame as a submodule in your repository.

### Setup

```bash
# Add JFrame as a submodule
git submodule add https://github.com/Jacophobia/jframe.git external/jframe
git submodule update --init --recursive
```

### CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.28)
project(my_game LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Don't build tests/examples
set(JFRAME_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(JFRAME_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(external/jframe)

add_executable(my_game main.cpp)
target_link_libraries(my_game PRIVATE jframe-core)
```

### Updating JFrame

```bash
cd external/jframe
git fetch origin
git checkout v1.1.0  # Or desired version
cd ../..
git add external/jframe
git commit -m "Update JFrame to v1.1.0"
```

---

## Available Targets

When linking to JFrame, you can use these targets:

| Target | Description |
|--------|-------------|
| `jframe-core` | Main framework (includes all systems) |
| `jframe-entity` | Entity Component System (EnTT) |
| `jframe-graphics` | Graphics/Rendering (OpenGL) |
| `jframe-audio` | Audio (FMOD) |
| `jframe-input` | Input handling (GLFW, SDL2) |
| `jframe-physics` | Physics (Box2D) |
| `jframe-assets` | Asset management |
| `jframe-events` | Event system |
| `jframe-save` | Save/Load (cereal) |
| `jframe-level` | Level management (Lua) |
| `jframe-ai` | AI/Pathfinding (Recast/Detour) |
| `jframe-camera` | Camera system |
| `jframe-config` | Configuration (Lua) |
| `jframe-gas` | Gameplay Ability System |
| `jframe-blueprints` | Blueprint/Prefab system |
| `jframe-dev` | Development tools (debug only) |

For most games, just link to `jframe-core` - it brings in all dependencies.

---

## vcpkg Integration

JFrame uses vcpkg for its dependencies. Your project should also use vcpkg.

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

You don't need to duplicate JFrame's dependencies - they're brought in automatically through CMake. But if you want explicit control:

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

JFrame's audio system uses FMOD, which requires manual installation:

1. Download FMOD Core API from [fmod.com/download](https://www.fmod.com/download)
2. Place in your project's `external/fmod/core/` directory
3. See [Installation.md](Installation.md) for detailed instructions

If you don't need audio, you can exclude the audio system by not linking to it.

---

## Compiler Requirements

JFrame requires C++23 with `import std;` support:

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

### "Cannot find module 'jframe'"

Ensure you're linking to JFrame targets and using C++23 modules:

```cmake
target_link_libraries(my_game PRIVATE jframe-core)
```

### "ABI mismatch" or "undefined symbols"

Your project must use the same compiler (LLVM Clang 20+) as JFrame. Mixing compilers will cause linker errors.

### "vcpkg dependencies not found"

Ensure `VCPKG_ROOT` is set and the toolchain file is configured:

```bash
export VCPKG_ROOT=~/vcpkg
cmake --preset default
```

### Version Compatibility

JFrame follows semantic versioning:
- **Major** versions may have breaking API changes
- **Minor** versions add features, backward compatible
- **Patch** versions are bug fixes only

Use `find_package(jframe 1.0 REQUIRED)` to require a minimum version.
