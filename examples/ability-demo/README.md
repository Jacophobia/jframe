# Bestow Gameplay Ability System Demo

This demo showcases the Bestow Gameplay Ability System (GAS) with a fully graphical, interactive demonstration.

## Features Demonstrated

### 1. Dash Ability
- **Activation**: Press `LEFT SHIFT`
- **Cost**: 25 stamina
- **Cooldown**: 2 seconds
- **Effect**: Doubles move speed for 0.5 seconds
- **Visual Feedback**: Player turns cyan while dashing
- **Blocking**: Cannot dash when stunned

### 2. Jump Ability (Room-Based)
- **Activation**: Press `SPACE`
- **Requirement**: Must be in a yellow jump zone
- **Cooldown**: 0.5 seconds
- **Visual Feedback**: Yellow transparent zones indicate where jumping is allowed
- **Blocking**: Cannot jump when stunned or outside jump zones

### 3. Health Regeneration Effect
- **Activation**: Press `H` (test mode)
- **Duration**: 10 seconds
- **Effect**: Regenerates 5 health per second
- **Visual Feedback**: Player glows bright green during regeneration

### 4. Stun Effect
- **Activation**: Press `S` (test mode)
- **Duration**: 2 seconds
- **Effect**: Blocks all abilities
- **Visual Feedback**: Player turns purple when stunned

## Controls

| Key | Action |
|-----|--------|
| `A` or `Left Arrow` | Move left |
| `D` or `Right Arrow` | Move right |
| `SPACE` | Jump (only in yellow zones) |
| `LEFT SHIFT` | Dash (costs stamina) |
| `H` | Apply health regen effect |
| `S` | Apply stun effect (test) |

## UI Elements

### Top-Left Bars
- **Red Bar**: Health (0-100)
- **Cyan Bar**: Stamina (0-100)

### Cooldown Indicator
- **Green Square**: Dash is ready
- **Red-tinted Square**: Dash is on cooldown (shows remaining time)

### Active Effects
- **Colored Rectangles**: Show currently active effects
  - Cyan: Dash speed boost
  - Green: Health regeneration
  - Purple: Stunned

## Visual Feedback

### Player Colors
- **Green**: Normal state
- **Cyan**: Dashing (speed boost active)
- **Bright Green**: Health regenerating
- **Purple**: Stunned

### Environment
- **Gray Platforms**: Static platforms for standing
- **Yellow Zones**: Areas where jumping is enabled

## How to Build

From the Bestow root directory:

```bash
cmake --preset macos-debug
cmake --build --preset macos-debug --target ability-demo
```

## How to Run

```bash
cd build/macos-debug/examples/ability-demo
./ability-demo
```

## Implementation Details

### Systems Used
1. **Graphics System** - Rendering sprites, UI, and debug visuals
2. **Input System** - Action-based input mapping
3. **Physics System** - Player movement and collision
4. **Entity System** - Component-based entity management
5. **GAS System** - Abilities, effects, attributes, and tags
6. **Events System** - Internal event handling
7. **Assets System** - Loading Lua configuration

### Key Components
- `PlayerController` - Player movement state
- `Camera2D` - Camera follow logic
- `PlayerTag`, `PlatformTag`, `JumpZoneTag` - Entity categorization

### GAS Integration
The demo shows proper integration of the GAS system with:
- **Attributes**: Health, Stamina, MoveSpeed
- **Abilities**: Dash (with cost and cooldown), Jump (with tag requirements)
- **Effects**: Speed boost, health regen, stun
- **Tags**: Dashing, Stunned, InJumpZone

All GAS definitions are loaded from `data/config/abilities.lua`, making them easy to modify without recompiling.

## Testing Features

Walk around the level to experience:
1. Normal movement on platforms
2. Enter yellow zones to enable jumping
3. Use dash to quickly traverse the level
4. Test stamina costs and cooldowns
5. Apply health regen and watch your health bar refill
6. Apply stun and notice all abilities are blocked
7. Watch the visual feedback change based on active effects

## Architecture Notes

This demo follows the Bestow architecture guidelines:
- Uses `import bestow;` for all Bestow modules
- Implements the `Application` interface from `bestow.core`
- Separates game logic into `updateFixed()` and rendering into `render()`
- Uses the `EngineBuilder` to configure all required systems
- Demonstrates proper component-based design with ECS
- Shows data-driven design with Lua configuration files
