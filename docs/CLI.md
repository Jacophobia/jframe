# Bestow CLI Reference

The `bestow` command-line interface runs Lua games and manages projects.

## Installation

```bash
# macOS (Homebrew)
brew install bestow

# Build from source
cmake --preset macos-release
cmake --build --preset macos-release
sudo cmake --install build/macos-release
```

---

## Commands

### `bestow run`

Run a Lua game.

```bash
bestow run <main.lua> [options]
```

**Arguments:**
| Argument | Description |
|----------|-------------|
| `<main.lua>` | Path to the game's entry point |

**Options:**
| Option | Description |
|--------|-------------|
| `--hot-reload`, `-h` | Enable hot reload for Lua files |
| `--debug`, `-d` | Show debug overlay (FPS, entity count, etc.) |
| `--vsync`, `-v` | Enable vertical sync (default: enabled) |
| `--no-vsync` | Disable vertical sync |
| `--fullscreen`, `-f` | Start in fullscreen mode |
| `--width <n>` | Window width (overrides main.lua) |
| `--height <n>` | Window height (overrides main.lua) |
| `--renderer <name>` | Graphics renderer: `vulkan` (default), `opengl` |

**Examples:**
```bash
# Run a game
bestow run my-game/main.lua

# Run with hot reload and debug overlay
bestow run my-game/main.lua --hot-reload --debug

# Run fullscreen at 1920x1080
bestow run my-game/main.lua --fullscreen --width 1920 --height 1080

# Run with OpenGL (fallback renderer)
bestow run my-game/main.lua --renderer opengl
```

---

### `bestow new`

Create a new game project with starter files.

```bash
bestow new <name> [options]
```

**Arguments:**
| Argument | Description |
|----------|-------------|
| `<name>` | Project directory name |

**Options:**
| Option | Description |
|--------|-------------|
| `--template <type>` | Project template: `minimal` (default), `3d`, `2d` |

**Examples:**
```bash
# Create a minimal project
bestow new my-game

# Create a 3D game template
bestow new my-3d-game --template 3d

# Create a 2D game template
bestow new my-platformer --template 2d
```

**Generated Structure:**
```
my-game/
├── main.lua           # Entry point
├── entities/          # Entity blueprints
│   └── player.lua
├── systems/           # Game systems
│   └── movement.lua
├── levels/            # Level definitions
│   └── level1.lua
└── assets/            # Game assets
    ├── textures/
    ├── sounds/
    └── materials/
```

---

### `bestow init`

Initialize the current directory with Bestow template files.

```bash
bestow init
```

This command copies template files into your current directory:
- `CLAUDE.md` - AI agent guidance for Bestow game development
- `.claude/skills/` - Claude skills covering all Bestow systems

**Use Cases:**
- Add AI assistance to an existing project
- Set up Claude skills for a game you're developing
- Initialize a directory before running `bestow run`

**Example:**
```bash
mkdir my-game
cd my-game
bestow init

# Now create your main.lua and start developing
```

**Notes:**
- Does not overwrite existing files (skips with a warning)
- Safe to run multiple times
- Template files are installed alongside bestow (no network required)

---

### `bestow generate-stubs`

Generate IDE type stubs for Lua autocomplete.

```bash
bestow generate-stubs <output-dir>
```

**Arguments:**
| Argument | Description |
|----------|-------------|
| `<output-dir>` | Directory to write stub files |

**Example:**
```bash
bestow generate-stubs ./stubs

# Add to VS Code settings.json:
# "Lua.workspace.library": ["./stubs"]
```

This generates EmmyLua-annotated stub files for:
- `bestow.*` API (entity, input, graphics3d, audio, etc.)
- Global types (Vec2, Vec3, Quat, Color, Mat4)
- Component definitions

---

### `bestow build`

Package a game for distribution (coming soon).

```bash
bestow build <main.lua> [options]
```

**Options:**
| Option | Description |
|--------|-------------|
| `--output`, `-o` | Output directory (default: `./build`) |
| `--platform` | Target platform: `macos`, `windows`, `linux` |

---

### `bestow version`

Show version information.

```bash
bestow version
```

**Output:**
```
bestow 0.1.0
Lua 5.4.6
Vulkan 1.3.275
```

---

### `bestow help`

Show help for commands.

```bash
bestow help [command]
```

**Examples:**
```bash
# Show general help
bestow help

# Show help for 'run' command
bestow help run
```

---

## Environment Variables

| Variable | Description | Default |
|----------|-------------|---------|
| `BESTOW_RENDERER` | Default renderer (`vulkan`, `opengl`) | `vulkan` |
| `BESTOW_VSYNC` | Enable vsync (`0`, `1`) | `1` |
| `BESTOW_LOG_LEVEL` | Logging level (`debug`, `info`, `warn`, `error`) | `info` |
| `BESTOW_ASSETS_PATH` | Additional asset search paths (colon-separated) | - |
| `BESTOW_LIBRARY_PATH` | Path to engine library assets | Auto-detected |
| `BESTOW_TEMPLATE_PATH` | Path to template files for `bestow init` | Auto-detected |

**Example:**
```bash
export BESTOW_RENDERER=opengl
export BESTOW_LOG_LEVEL=debug
bestow run my-game/main.lua
```

---

## Configuration File

Create `bestow.config.lua` in your project root to set default options:

```lua
-- bestow.config.lua
return {
    renderer = "vulkan",
    vsync = true,
    hotReload = true,
    debug = false,

    window = {
        width = 1280,
        height = 720,
        fullscreen = false
    },

    -- Folders to ignore for hot reload
    ignore = {
        ".git",
        "node_modules",
        "build"
    }
}
```

---

## Exit Codes

| Code | Description |
|------|-------------|
| `0` | Success |
| `1` | General error |
| `2` | File not found |
| `3` | Lua syntax error |
| `4` | Runtime error |
| `5` | Renderer initialization failed |

---

## Debugging

### Debug Overlay

Enable with `--debug` to show:
- FPS and frame time
- Entity count
- Draw calls
- Memory usage
- Lua script reload events

### Logging

Set log level with `BESTOW_LOG_LEVEL`:

```bash
BESTOW_LOG_LEVEL=debug bestow run main.lua
```

Log levels:
- `debug` - All messages including internal state
- `info` - General information (default)
- `warn` - Warnings only
- `error` - Errors only

### Hot Reload Debugging

When hot reload fails, the error is shown in the console:

```
[hot-reload] Reloading: systems/movement.lua
[hot-reload] ERROR: systems/movement.lua:15: attempt to index a nil value
[hot-reload] Keeping previous version
```

The game continues running with the last working version.

---

## Keyboard Shortcuts (Runtime)

| Key | Action |
|-----|--------|
| `F1` | Toggle debug overlay |
| `F2` | Toggle wireframe mode |
| `F3` | Toggle physics debug draw |
| `F5` | Force reload all Lua files |
| `F11` | Toggle fullscreen |
| `Escape` | Quit (if not overridden by game) |

---

## See Also

- [Getting Started](Getting-Started.md) - Quick start guide
- [Data-Driven Design](Data-Driven-Design.md) - Lua API reference
- [Installation](Installation.md) - Detailed installation guide
