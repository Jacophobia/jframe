# JFrame Platformer Example

A minimal but functional 2D platformer demonstrating the JFrame game framework.

## Features

### Implemented
- **Player Movement** - WASD/Arrow keys for horizontal movement
- **Jump Mechanics** - Space to jump with coyote time (0.1s grace period)
- **Physics Simulation** - Gravity, collision detection via Box2D
- **Camera System** - Smooth camera follow with lerp interpolation
- **Input Mapping** - Configurable action-based input system
- **Fixed Timestep** - Deterministic 60Hz physics updates
- **Interpolated Rendering** - Smooth rendering independent of physics framerate

### Game Components
- `PlayerController` - Movement speed, jump force, air control, ground detection
- `Velocity` - Simple 2D velocity storage
- `Health` - Health points with invincibility frames
- `Camera2D` - Smooth camera following with offset and smoothing factor

## Controls

| Action | Keys |
|--------|------|
| Move Left | A or Left Arrow |
| Move Right | D or Right Arrow |
| Jump | Space |

## Level Design

The example creates a simple test level with:
- Ground platform at the bottom
- 3 floating platforms arranged in a staircase pattern
- Player spawns at position (100, 300)

## Architecture

### File Structure
```
examples/platformer/
├── src/
│   ├── main.cpp           - Entry point, engine setup
│   ├── Game.h             - Game class declaration
│   ├── Game.cpp           - Game class implementation
│   └── Components.h       - Game-specific components
├── data/
│   ├── levels/            - Level definitions (Lua)
│   └── blueprints/        - Entity blueprints (Lua)
└── CMakeLists.txt         - Build configuration
```

### Systems Used
- **Events** - Event dispatching and subscription
- **Entities** - Entity-Component-System via EnTT
- **Physics** - Box2D 3.0 physics simulation
- **Graphics** - OpenGL rendering with primitive shapes
- **Input** - Keyboard and controller input handling
- **Assets** - Asset loading and management
- **Audio** - FMOD audio playback (initialized but not used in this demo)
- **Level** - Lua-based level loading
- **Save** - Save/load game state
- **AI** - AI behaviors and navigation (initialized but not used in this demo)

### Game Loop

```cpp
// Fixed timestep (60 Hz) + variable rendering
while (running) {
    accumulator += deltaTime;

    while (accumulator >= FIXED_DT) {
        updateFixed(FIXED_DT);  // Physics, input, logic
        accumulator -= FIXED_DT;
    }

    render(accumulator / FIXED_DT);  // Interpolation factor
}
```

## Building

```bash
# Configure
cmake --preset macos-debug

# Build
cmake --build --preset macos-debug --target platformer

# Run
./build/macos-debug/examples/platformer/platformer
```

## Rendering

Currently uses debug primitives:
- Green rectangle for the player (24x44 pixels)
- Sky blue background color (135, 206, 235)
- No textures loaded (sprite rendering code in place but no sprites)

## Future Enhancements

The following features are stubbed but not implemented:
- Sprite rendering (texture loading, animation)
- Enemy entities with AI behaviors
- Collectibles (coins, power-ups)
- Level loading from Lua files
- Sound effects and music
- Level transitions
- Checkpoints and respawn system
- Particle effects
- UI rendering (health bar, score)

## Development

### Hot Reload (Debug builds only)
When built with `JFRAME_DEV_TOOLS=ON`:
- Level files auto-reload on save
- Blueprint files auto-reload on save
- Dev overlay with FPS counter

### Code Statistics
- **main.cpp**: 50 lines
- **Game.h**: 44 lines
- **Game.cpp**: 306 lines
- **Components.h**: 48 lines
- **Total**: ~448 lines of game code

## License

Part of the JFrame game framework.
