# Platformer Demo Assets

## Overview

All game assets for this platformer demo are sourced from **Kenney.nl** - a collection of high-quality, free game assets created by Kenney Vleugels.

## License

**Creative Commons Zero (CC0)**
https://creativecommons.org/publicdomain/zero/1.0/

These assets are in the public domain. You can use them freely for any purpose, including commercial projects, without attribution. However, we choose to provide attribution as a courtesy to the creator.

## Attribution

Assets created by **Kenney Vleugels** (Kenney.nl)
Website: https://kenney.nl
Support: https://www.patreon.com/kenney

## Asset Sources

### Player Character
- **Source Pack**: Kenney Platformer Characters
- **Files**:
  - `player_spritesheet.png` - Main player character with all animation frames
  - `playerBlue_*.png` - Alternative blue player character (idle, walk, jump, fall, duck, swim, etc.)
- **Description**: Complete platformer character with animations for idle, walking, jumping, falling, ducking, climbing, and more

### Enemies
- **Source Pack**: Abstract Platformer (370 Assets)
- **Files**:
  - `enemyFloating_*.png` (4 frames) - Floating ghost-like enemies
  - `enemyFlying_*.png` (4 frames) - Flying enemies with wings
  - `enemyFlyingAlt_*.png` (4 frames) - Alternative flying enemies
  - `enemySpikey_*.png` (4 frames) - Spiky ground enemies
  - `enemySwimming_*.png` (4 frames) - Swimming/aquatic enemies
  - `enemyWalking_*.png` (4 frames) - Walking ground enemies
- **Description**: Various enemy types with animation frames for movement

### Platform Tiles
- **Source Pack**: Kenney Platformer Kit & Abstract Platformer
- **Files**:
  - `block.png` - Standard platform block
  - `blockSnow.png` - Snow-covered platform
  - `blockCliff.png` - Cliff edge platform
  - `blockHalf.png` - Half-height platform
  - `blockSlope.png` - Sloped platform
  - `tileBlue_*.png` (27 tiles) - Complete blue tile set with corners, edges, and decorations
- **Description**: Various platform tiles for building levels

### Collectibles & Items
- **Source Pack**: Abstract Platformer & Platformer Pack Redux
- **Files**:
  - `coinGold.png`, `coinSilver.png`, `coinBronze.png` - Collectible coins
  - `*Gem.png` (blue, green, red, yellow, outline) - Gem collectibles
  - `*Crystal.png` (blue, green, red, yellow, outline) - Crystal collectibles
  - `*Jewel.png` (blue, green, red, yellow, outline) - Jewel collectibles
  - `keyGreen.png`, `keyRed.png`, `outlineKey.png` - Keys for unlocking
  - `discGreen.png`, `discRed.png`, `outlineDisc*.png` - Disc collectibles
  - `puzzleGreen.png`, `puzzleRed.png`, `outlinePuzzle.png` - Puzzle pieces
- **Description**: Various collectible items for gameplay rewards

### Backgrounds
- **Source Pack**: Abstract Platformer
- **Files**:
  - `set1_background.png` - Sky/background layer
  - `set1_hills.png` - Hills parallax layer
  - `set1_tiles.png` - Tileset for backgrounds
- **Description**: Layered background elements for parallax scrolling

### UI Elements
- **Source Pack**: Pixel UI Pack
- **Files**:
  - `UIpackSheet_transparent.png` - Complete UI spritesheet with transparent background
  - `UIpackSheet_magenta.png` - Complete UI spritesheet with magenta background for keying
- **Description**: Buttons, panels, icons, and UI elements for menus and HUD

## Audio Assets

### Sound Effects
- **Source Packs**:
  - Kenney Digital Audio
  - Kenney Impact Sounds
- **Files**:
  - **Jump/Movement**: `phaseJump1.ogg`
  - **Collectibles**: `pepSound1-5.ogg`, `powerUp1-12.ogg` (17 pickup sound variations)
  - **Footsteps**: `footstep_concrete_000-004.ogg` (5 footstep variations)
  - **Impacts/Damage**: `impactBell_heavy_000-004.ogg` (5 impact variations)
- **Format**: OGG Vorbis
- **Description**: High-quality 8-bit/retro style sound effects suitable for platformer gameplay

### Suggested Sound Mapping
- `phaseJump1.ogg` - Player jump
- `pepSound1-5.ogg` - Small pickups (coins, gems)
- `powerUp1-12.ogg` - Power-ups, level completion, achievements
- `footstep_concrete_*.ogg` - Player footsteps (randomize for variety)
- `impactBell_heavy_*.ogg` - Enemy damage, player hit, collisions

## File Organization

```
platformer-demo/
└── data/
    ├── textures/         (108 PNG files, ~400 KB total)
    ├── audio/
    │   ├── sfx/          (28 OGG files, ~200 KB total)
    │   └── music/        (empty - add your own or use Kenney Music Jingles)
    ├── levels/           (empty - for Lua level definitions)
    └── fonts/            (empty - for text rendering)
```

## Notes

### Missing Assets
- **Music**: No background music included yet. Consider:
  - Kenney Music Jingles (available in the same repository)
  - Other CC0 music sources
- **Fonts**: No fonts included. Consider:
  - Kenney Pixel fonts (if available)
  - Open-source pixel fonts like Press Start 2P or Pixelated

### Animation Frame Rates
- Most enemy animations have 4 frames - suggest 8-12 FPS for smooth animation
- Player spritesheet includes many poses - refer to original pack for frame mapping

### Recommended Additions
If you need more variety, the Kenney assets repository includes:
- `kenney_platformerpack_industrial` - Industrial/mechanical themed tiles
- `platformer-pack-medieval` - Medieval fantasy themed assets
- `kenney_scribbleplatformer` - Hand-drawn style platformer assets
- `platformergraphics-*` - Themed environment packs (candy, ice, mushroom, etc.)

## Integration Notes

### Spritesheet Usage
The `player_spritesheet.png` contains a grid of animation frames. You'll need to:
1. Parse the spritesheet into individual frames
2. Define animation sequences (walk, jump, idle, etc.)
3. Set appropriate frame durations for smooth animation

### UI Spritesheet
The UI spritesheets contain dozens of elements. Consider:
1. Creating a texture atlas definition
2. Mapping sprite names to coordinates
3. Extracting commonly used elements into separate textures if needed

### Audio Implementation
For best results:
- Use positional audio for sound effects
- Add slight pitch variation (±5%) when playing repeated sounds
- Mix multiple footstep sounds randomly for natural walking
- Layer impact sounds for more dramatic effects

## Updates

Assets downloaded: November 26, 2025
Kenney repository: https://github.com/ETdoFresh/kenney.nl
Last verified: November 2025

---

**Thank you, Kenney, for providing these amazing free assets to the game development community!**
