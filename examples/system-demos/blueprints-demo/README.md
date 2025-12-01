# Blueprints System Demo

Comprehensive demonstration of the JFrame Blueprints System API.

## Overview

This demo exercises **100% of the IBlueprintFactory interface**, demonstrating all methods, types, and features of the data-driven entity creation system.

## Features Demonstrated

### API Coverage

#### Blueprint Loading
- `loadBlueprints(luaSource)` - Load blueprint definitions from Lua
- `reloadBlueprints()` - Reload previously loaded blueprints (hot-reload)
- `clearBlueprints()` - Clear all loaded blueprints

#### Blueprint Queries
- `hasBlueprint(name)` - Check if a blueprint exists
- `getBlueprintNames()` - Get all blueprint names
- `getBlueprint(name)` - Get a blueprint definition for inspection

#### Entity Creation (All 4 Overloads)
- `create(name, x, y)` - Create entity at position
- `create(name, x, y, width, height)` - Create with custom size
- `create(name, x, y, overrides)` - Create with property overrides
- `create(name, x, y, width, height, overrides)` - Create with size and overrides

#### Component Registration
- `registerComponent(name, creator)` - Register custom component creators
- `isComponentRegistered(name)` - Check if component is registered

### Blueprint Features

#### Simple Blueprints
- `static_platform` - Static physics body
- `coin` - Sensor body (collectible)
- `bullet` - Dynamic projectile

#### Complex Blueprints
- `player` - Multiple components (DebugRect, PlayerTag) with physics and metadata
- Custom components with detailed properties

#### Blueprint Inheritance
- `base_enemy` - Base blueprint with common enemy properties
- `fast_enemy` - Inherits from base, overrides color and metadata
- `flying_enemy` - Inherits from base, replaces DebugRect with DebugCircle
- `tank_enemy` - Inherits from base, overrides size and physics

#### Physics Configuration
- Static bodies (platforms)
- Dynamic bodies (player, enemies, crates)
- Kinematic bodies (flying enemies)
- Sensor bodies (coins, triggers)
- Custom physics properties (density, friction, restitution, damping)
- Collision layers

#### Property Overrides
- Color overrides using array notation: `{0, 255, 0, 255}`
- Nested property overrides: `"DebugRect.fillColor"`
- Metadata overrides: `"metadata.health"`

#### Metadata
- Gameplay properties (health, damage, points)
- AI configuration (ai_type, moveSpeed)
- Custom flags (canFly, doubleJump)
- Asset references (sound files)

## Files

```
blueprints-demo/
├── CMakeLists.txt         # Build configuration
├── src/
│   └── main.cpp          # Comprehensive demo program
└── data/
    └── blueprints.lua    # Example blueprint definitions
```

## Building

```bash
# Configure
cmake --preset macos-debug

# Build
cmake --build --preset macos-debug --target blueprints-demo

# Run
./build/macos-debug/bin/blueprints-demo
```

## Demo Output Structure

The demo is organized into 9 sections:

1. **Component Registration** - Register custom game components
2. **Blueprint Loading** - Load blueprints from Lua file
3. **Blueprint Queries** - Inspect available blueprints
4. **Entity Creation** - Test all 4 create() overloads
5. **Blueprint Inheritance** - Demonstrate inheritance and property merging
6. **Physics Configuration** - Show different physics body types
7. **Metadata Inspection** - Examine blueprint metadata
8. **Blueprint Reloading** - Simulate hot-reload capability
9. **Edge Cases** - Test error handling and minimal blueprints

## Key Concepts Demonstrated

### Data-Driven Design
Blueprints are defined in Lua files, allowing:
- Non-programmers to create entities
- Runtime reloading without recompilation
- Inheritance for reusable patterns
- Comments and variables in data files

### Component Registration
The demo shows how to register custom components:
```cpp
factory.registerComponent("Health", [](Entity e, IEntitySystem& sys, const PropertyMap& props) {
    HealthComponent health{
        .current = getInt(props, "current", 100),
        .maximum = getInt(props, "maximum", 100)
    };
    sys.emplace<HealthComponent>(e, health);
});
```

### Property Overrides
Create variations without defining new blueprints:
```cpp
PropertyMap overrides;
overrides["DebugRect.fillColor"] = std::vector<double>{0, 255, 0, 255}; // Green
Entity greenEnemy = factory.create("enemy", x, y, overrides);
```

### Blueprint Inheritance
Define base templates and specialize:
```lua
base_enemy = {
    components = { ... },
    physics = { ... },
    metadata = { health = 50 }
}

tank_enemy = {
    inherits = "base_enemy",
    metadata = { health = 150, armor = 10 }  -- Override health, add armor
}
```

## Testing

This demo serves as:
- **API Reference** - Shows how to use every method
- **Integration Test** - Verifies all features work together
- **Example Code** - Copy patterns for your own games

## Related Systems

- **Entity System** (`jframe-entity`) - Manages entities and components
- **Physics System** (`jframe-physics`) - Provides physics bodies referenced in blueprints
- **Config System** (`jframe-config`) - Alternative for simple key-value config

## Notes

- Blueprints use Lua for flexibility (variables, loops, math, comments)
- All Lua execution is sandboxed (os, io, loadfile removed)
- Physics integration is optional (pass nullptr if not using physics)
- Custom components can be registered at runtime
- Hot-reload is supported via `reloadBlueprints()`
