# Bestow Game Framework

A **Lua-first game engine** with a modern C++23 core. Write your game in Lua, run it instantly with no compilation.

## Quick Start

### Run a Game

```bash
# Run your game
bestow run my-game/main.lua

# Create a new project
bestow new my-game
```

### Minimal Example (main.lua)

```lua
return {
    title = "My Game",
    width = 1280,
    height = 720,

    init = function()
        local player = bestow.entity.create()
        bestow.entity.addComponent(player, "Transform3D", {
            position = Vec3.new(0, 1, 0)
        })
        app.main.state = { player = player }
    end,

    update = function(dt)
        -- Move with ,AOE (Dvorak) or arrow keys
        if bestow.input.isKeyDown(Keys.Comma) then
            local pos = bestow.entity.getField(app.main.state.player, "Transform3D", "position")
            pos.z = pos.z - 5 * dt
            bestow.entity.setField(app.main.state.player, "Transform3D", "position", pos)
        end
        return not bestow.input.wasKeyJustPressed(Keys.Escape)
    end,

    render = function()
        bestow.graphics3d.beginFrame()
        bestow.graphics3d.endFrame()
    end,

    run = function()
        app.main.init()
        while app.main.update(bestow.core.deltaTime()) do
            app.main.render()
        end
    end
}
```

That's it! No compilation, no build system, no CMake. Just Lua.

---

## Two Ways to Use Bestow

| Approach | Best For | Documentation |
|----------|----------|---------------|
| **Lua CLI** (Recommended) | Game developers, designers, rapid prototyping | [Getting Started](docs/Getting-Started.md), [Lua API](docs/Data-Driven-Design.md) |
| **C++ Library** | Engine developers, performance-critical code, custom systems | [C++ Guide](docs/Using-CPP-Library.md), [Architecture](docs/Architecture.md) |

### Lua CLI (Recommended for Most Users)

Write your entire game in Lua. The engine handles rendering, physics, audio, and input.

```bash
bestow run main.lua          # Run your game
bestow new my-game           # Create new project
bestow run --hot-reload      # Enable hot reload
```

**Project structure:**
```
my-game/
├── main.lua                 # Entry point
├── entities/
│   └── player.lua          # → app.entities.player
├── systems/
│   └── movement.lua        # → app.systems.movement
├── levels/
│   └── level1.lua          # → app.levels.level1
└── assets/
    ├── textures/
    └── sounds/
```

**Key benefits:**
- ✅ No compilation - edit and run instantly
- ✅ Hot reload - see changes without restarting
- ✅ Simple project structure - just Lua files
- ✅ Full engine power - graphics, physics, audio, input

### C++ Library (For Engine Developers)

Use Bestow as a C++ library when you need:
- Custom engine systems
- Maximum performance
- Direct hardware access
- Integration with existing C++ codebases

```cpp
// main.cpp
import bestow.core;
import bestow.vulkan.impl;

class MyGame : public bestow::Application<MyGame, bestow::IGraphics3DSystem> {
public:
    MyGame(bestow::IGraphics3DSystem& graphics) : graphics_(&graphics) {}
    void run() override { /* game loop */ }
};

int main() {
    bestow::core::Engine engine;
    engine.use<bestow::IGraphics3DSystem, bestow::VulkanGraphics3DSystem>();
    engine.run<MyGame>();
}
```

See [Using Bestow as a C++ Library](docs/Using-CPP-Library.md) for complete documentation.

---

## Documentation

### For Game Developers (Lua)

| Document | Description |
|----------|-------------|
| [Getting Started](docs/Getting-Started.md) | Quick start with Lua CLI |
| [CLI Reference](docs/CLI.md) | All `bestow` commands and options |
| [Lua API Reference](docs/Data-Driven-Design.md) | Complete `bestow.*` and `app.*` API |
| [Example Games](games/) | Complete game implementations |

### For Engine Developers (C++)

| Document | Description |
|----------|-------------|
| [Using C++ Library](docs/Using-CPP-Library.md) | C++ integration guide |
| [Architecture](docs/Architecture.md) | System design overview |
| [API Reference](docs/api/) | System API documentation |
| [Technical Design](docs/bestow-technical-design.md) | Deep dive into architecture |

### For Contributors

| Document | Description |
|----------|-------------|
| [Development Guidelines](CLAUDE.md) | Coding standards and workflow |
| [System Implementation Guide](docs/SYSTEM-IMPLEMENTATION-GUIDE.md) | Adding new systems |
| [Project Status](docs/PROJECT-STATUS.md) | Implementation progress |

---

## Features

- **Lua-first design** - Write games in Lua, no compilation needed
- **Hot reload** - Edit Lua files and see changes instantly
- **Vulkan rendering** - Modern 3D graphics (OpenGL fallback available)
- **Physics** - Box2D for 2D, Jolt for 3D
- **Audio** - FMOD integration for sound and music
- **Entity Component System** - Powered by EnTT
- **Cross-platform** - macOS, Windows, Linux

## CLI Installation

```bash
# macOS (Homebrew)
brew install bestow

# Build from source
cmake --preset macos-release
cmake --build --preset macos-release
sudo cmake --install build/macos-release
```

See [docs/Installation.md](docs/Installation.md) for detailed setup.

## Requirements

**For Lua game development:**
- `bestow` CLI tool (see installation above)

**For C++ development:**
- C++23 compiler (Clang 20+, GCC 13+, MSVC 19.38+)
- CMake 3.28+
- Vulkan SDK
- vcpkg for dependencies

## License

See LICENSE file for details.
