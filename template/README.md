# Bestow Template Game

A complete template project demonstrating all Bestow systems. Copy this to start your own game!

## What's Included

This template demonstrates:

- **All Bestow Systems**: Events, Entity, Input, Physics, Audio, Graphics, Assets, Save, Level, AI, Config, Camera, GAS (Gameplay Ability System), Blueprints, Components
- **Data-Driven Design**: All game data defined in Lua files (blueprints, levels, config, input bindings)
- **Minimal but Complete**: Clean code showing how each system integrates without overwhelming complexity
- **Ready to Run**: Compiles and runs immediately showing a simple playable demo

## Project Structure

```
template/
├── CMakeLists.txt          # Build configuration
├── README.md               # This file
├── src/
│   ├── main.cpp            # Entry point - engine setup
│   ├── Game.cpp            # Main game class - demonstrates all systems
│   └── Game.h              # Game class header
└── data/
    ├── blueprints/
    │   └── entities.lua    # Entity blueprint definitions (Player, Platform, etc.)
    ├── levels/
    │   └── main.lua        # Level layout and entities
    ├── config/
    │   ├── game.lua        # Game configuration values
    │   └── audio.lua       # Audio configuration
    └── input/
        └── bindings.lua    # Input action mappings
```

## How to Use This Template

### 1. Copy the Template

```bash
cp -r template my-awesome-game
cd my-awesome-game
```

### 2. Add to CMake

Edit the root `CMakeLists.txt` and add:

```cmake
add_subdirectory(my-awesome-game)
```

### 3. Build and Run

```bash
cmake --preset macos-debug
cmake --build --preset macos-debug --target my-awesome-game
./build/macos-debug/my-awesome-game/my-awesome-game
```

### 4. Customize

- **Change game config**: Edit `data/config/game.lua`
- **Modify input bindings**: Edit `data/input/bindings.lua`
- **Create new entity types**: Add blueprints to `data/blueprints/entities.lua`
- **Design levels**: Edit `data/levels/main.lua` using Lua programming
- **Add game logic**: Extend `src/Game.cpp` and `src/Game.h`

## What the Demo Shows

The template game demonstrates:

- **Player Movement**: WASD/Arrow keys to move, Space to jump
- **Physics**: Gravity, collision, ground detection
- **Camera**: Smooth camera following the player
- **Entity System**: Player and platform entities managed by EnTT
- **Blueprint System**: Entities created from Lua definitions
- **Level System**: Level layout defined in Lua with loops and functions
- **Input System**: Configurable input bindings from Lua
- **Config System**: Game values loaded from Lua configuration
- **Graphics System**: Debug rendering (DebugRect components)
- **Audio System**: Background music and jump sound effects (placeholders)
- **Events System**: Event publishing/subscription
- **GAS System**: Player abilities (jump) with cooldowns/costs
- **Component System**: Reusable component definitions
- **Camera System**: Smooth camera following with offset

## Data-Driven Development

All game content is defined in Lua files, not C++ code:

### Blueprints (`data/blueprints/entities.lua`)

Define reusable entity templates:

```lua
Blueprints = {
    Player = {
        components = {
            PlayerTag = {},
            DebugRect = {
                fillColor = {50, 200, 50, 255},  -- Green
                layer = 40
            }
        },
        physics = {
            type = "dynamic",
            fixedRotation = true,
            collisionLayer = "Player"
        }
    }
}
```

### Levels (`data/levels/main.lua`)

Use Lua programming for level design:

```lua
-- Use loops to create platforms
local entities = {}
for i = 1, 10 do
    table.insert(entities, {
        type = "platform",
        x = i * 100,
        y = 500,
        width = 80,
        height = 20
    })
end

return {
    name = "Main Level",
    entities = entities
}
```

### Configuration (`data/config/game.lua`)

Centralize all tuning values:

```lua
return {
    player = {
        moveSpeed = 200.0,
        jumpForce = 400.0
    }
}
```

### Input Bindings (`data/input/bindings.lua`)

Map actions to keys/buttons:

```lua
return {
    actions = {
        move_left = {
            keys = {"A", "Left"},
            buttons = {"DPadLeft"}
        },
        jump = {
            keys = {"Space", "W"},
            buttons = {"A"}
        }
    }
}
```

## Next Steps

1. **Explore the Code**: Read through `src/Game.cpp` to see how each system is initialized and used
2. **Modify the Level**: Edit `data/levels/main.lua` to add more platforms or change the layout
3. **Tune Parameters**: Adjust values in `data/config/game.lua` to change player speed, jump height, etc.
4. **Add Content**: Create new entity blueprints in `data/blueprints/entities.lua`
5. **Extend Gameplay**: Add new game logic in `src/Game.cpp` (enemies, collectibles, etc.)

## Common Patterns

### Creating Entities from Blueprints

```cpp
// Create entity from blueprint at position (x, y) with size (w, h)
Entity player = blueprints_->create("Player", 400.0f, 300.0f, 30.0f, 50.0f);
```

### Loading Configuration Values

```cpp
float moveSpeed = config_->getFloatOr("player.moveSpeed", 200.0f);
```

### Handling Input

```cpp
if (sys.input->isActionActive("move_left")) {
    velocity.x = -moveSpeed;
}
if (sys.input->wasActionJustPressed("jump")) {
    velocity.y = -jumpForce;
}
```

### Physics Queries

```cpp
// Check if player is on ground
auto groundResult = sys.physics->checkGrounded(player);
if (groundResult.grounded) {
    // Allow jumping
}
```

### Playing Audio

```cpp
// Play sound effect
sys.audio->playOnChannel(SFX_CHANNEL, {
    .asset = soundHandle,
    .volume = 0.8f,
    .looping = false
});

// Play background music
sys.audio->playOnChannel(MUSIC_CHANNEL, {
    .asset = musicHandle,
    .volume = 0.5f,
    .looping = true,
    .fadeInTime = 1.0f
});
```

### Camera Control

```cpp
// Initialize camera
camera_ = bestow::createCameraSystem(bestow::Size{800, 600});
camera_->setTarget(player);
camera_->setFollowSmoothing(0.1f);  // 0.0 = instant, 0.9 = very smooth
camera_->setOffset({0.0f, -50.0f}); // Camera looks slightly ahead

// Update each frame
camera_->update(dt, targetPos);

// Apply to graphics
sys.graphics->setCamera(camera_->getCamera());
```

## Tips

- **Hot Reload**: In debug builds, Lua files reload automatically when modified
- **Console Logging**: Use `bestow::core::logInfo()`, `logWarn()`, `logError()`
- **Entity Queries**: Use `sys.entities->view<ComponentType>()` to iterate entities efficiently
- **Data First**: Always define content in Lua files, not C++ - easier to iterate and balance
- **Test Incrementally**: Build and test after each change to catch errors early

## Documentation

For more detailed information, see:

- `/docs/Getting-Started.md` - Bestow overview and setup
- `/docs/Data-Driven-Design.md` - Data-driven development guide
- `/CLAUDE.md` - Development guidelines and best practices
- `/docs/systems/` - Individual system documentation

## License

Your game code is yours. Bestow is licensed separately (see main project README).
