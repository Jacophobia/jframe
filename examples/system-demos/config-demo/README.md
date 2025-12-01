# Config System Demo

Comprehensive demonstration of the JFrame Config System API (`jframe.config`).

## Overview

This demo exercises **every single method** in the `IConfigSystem` interface, including:

- **Lifecycle**: `initialize()`, `update()`, `shutdown()`
- **Loading**: `loadConfig()`, `loadConfigAsset()`, `reloadAll()`, `reloadConfig()`
- **Type-Safe Access**: `getFloat()`, `getInt()`, `getBool()`, `getString()`
- **Default Values**: `getFloatOr()`, `getIntOr()`, `getBoolOr()`, `getStringOr()`
- **Arrays**: `getIntArray()`, `getFloatArray()`, `getStringArray()`
- **Runtime Modification**: `setFloat()`, `setInt()`, `setBool()`, `setString()`
- **State Queries**: `hasKey()`, `getKeysWithPrefix()`, `getLoadedConfigs()`, `getMetadata()`
- **Hot Reload**: `enableHotReload()`, `isHotReloadEnabled()`
- **Change Notifications**: `onConfigChanged()`, `onKeyChanged()`, `unsubscribe()`

## Building

```bash
# Configure
cmake --preset macos-debug

# Build
cmake --build --preset macos-debug --target config-demo
```

## Running

The demo must be run from its build directory so it can find the data files:

```bash
cd build/macos-debug/examples/system-demos/config-demo
./config-demo
```

## What the Demo Does

The demo runs 13 comprehensive sections:

1. **Lifecycle Management** - Initializes the config system
2. **Loading Configuration Files** - Loads `game.lua` and `player.lua`
3. **Type-Safe Value Access** - Reads floats, ints, bools, and strings
4. **Default Value Fallbacks** - Uses `getXOr()` methods with defaults
5. **Array/List Access** - Reads arrays of different types
6. **Runtime Value Modification** - Modifies values at runtime with `setX()`
7. **State Queries** - Checks for keys, prefixes, and metadata
8. **Hot Reload System** - Enables/disables hot reload
9. **Change Notifications** - Subscribes to config changes
10. **Configuration Reloading** - Reloads specific and all configs
11. **Update Cycle** - Simulates game loop with hot reload checks
12. **Nested Table Access** - Accesses deeply nested Lua tables
13. **Asset System Integration** - Tests `loadConfigAsset()` method

## Configuration Files

### `data/game.lua`

Demonstrates:
- Window settings (width, height, fullscreen, vsync)
- Graphics settings (FPS, quality, brightness, colors)
- Audio settings (volumes, channels)
- Debug settings (log level, hot reload interval)
- Arrays (level IDs, spawn times, level names)
- Gameplay constants
- Computed values using Lua expressions

### `data/player.lua`

Demonstrates:
- Player stats and attributes
- Movement physics (speed, acceleration, jumping)
- Combat properties (damage, armor, special abilities)
- Animation data (frame counts, FPS)
- Ability and inventory systems
- Visual settings
- Progression system
- Arrays of various types

## Key Features Demonstrated

### Lua Configuration Benefits

The demo shows why Lua is superior to JSON for game configuration:

- **Comments** - Inline documentation
- **Trailing Commas** - No syntax errors
- **Variables** - Reusable values (`local GROUND_Y = 100`)
- **Math** - Computed values (`math.pi`, `1280 / 720`)
- **Conditionals** - Environment-specific config
- **Type Flexibility** - Nested tables, arrays, mixed types

### Hot Reload

The demo simulates a game loop calling `update()` which checks for file modifications and automatically reloads changed configs (when hot reload is enabled).

### Change Notifications

Demonstrates subscribing to:
- **Global changes** - Notified of any config change
- **Prefix-based changes** - Only notified when specific keys change (e.g., `"window."`)
- **Unsubscribing** - Proper cleanup of subscriptions

### Nested Access

Shows how to access deeply nested Lua tables using dot notation:
- `graphics.backgroundColor.r`
- `animations.attackFPS`
- `movement.canWallSlide`

## Expected Output

The demo prints detailed output for each section, showing:
- Successfully loaded values from Lua files
- Array contents
- Runtime modifications
- Callback invocations
- Hot reload status
- Final API coverage checklist

At the end, you'll see a complete checklist confirming all API methods were exercised.

## Integration with Other Systems

The demo shows the `loadConfigAsset()` method which integrates with the Asset System. This method is currently stubbed but will be fully functional once `IAssetSystem` is implemented.

## Files

- `CMakeLists.txt` - Build configuration
- `src/main.cpp` - Demo implementation (550+ lines)
- `data/game.lua` - Game configuration example
- `data/player.lua` - Player configuration example
- `README.md` - This file
