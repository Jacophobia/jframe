---
name: project-structure
description: Understand Bestow project organization and file structure. Use when creating new projects, organizing code, or understanding where files should be placed.
---

# Project Structure

A Bestow Lua game project follows a specific structure for auto-discovery and hot reload.

## Standard Project Layout

```
my-game/
├── main.lua                 # Entry point (required)
├── bestow.config.lua        # Engine configuration (optional)
│
├── entities/                # Entity factory modules
│   ├── player.lua
│   ├── enemies/
│   │   ├── basic.lua
│   │   └── boss.lua
│   └── collectibles/
│       ├── coin.lua
│       └── powerup.lua
│
├── systems/                 # Game systems (update logic)
│   ├── movement.lua
│   ├── combat.lua
│   ├── ai.lua
│   └── ui.lua
│
├── levels/                  # Level definitions
│   ├── level1.lua
│   ├── level2.lua
│   └── boss.lua
│
├── data/                    # Static game data
│   ├── items.lua
│   ├── enemies.lua
│   └── dialogue.lua
│
└── assets/                  # Game assets
    ├── textures/
    │   ├── player.png
    │   └── enemies/
    ├── sounds/
    │   ├── sfx/
    │   └── music/
    ├── meshes/
    ├── materials/
    └── fonts/
```

## Auto-Discovery and Namespaces

Bestow automatically discovers and loads Lua files into the `app.*` namespace:

| Directory | Namespace | Example Access |
|-----------|-----------|----------------|
| `entities/` | `app.entities.*` | `app.entities.player.create()` |
| `systems/` | `app.systems.*` | `app.systems.movement.update(dt)` |
| `levels/` | `app.levels.*` | `app.levels.level1` |
| `data/` | `app.data.*` | `app.data.items.sword` |
| `main.lua` | `app.main` | `app.main.state` |

### Nested Directories

Subdirectories become nested namespaces:

```lua
-- entities/enemies/boss.lua
return {
    create = function(pos) ... end
}

-- Accessed as:
app.entities.enemies.boss.create(Vec3.new(0, 0, 0))
```

## main.lua (Entry Point)

The entry point must return a table with lifecycle callbacks:

```lua
-- main.lua
return {
    -- Called once when game starts
    init = function()
        -- Initialize state (persists across hot reloads)
        app.main.state.score = 0
        app.main.state.level = 1

        -- Initialize systems
        app.systems.audio.init()
        app.systems.ui.init()

        -- Create initial entities
        app.main.state.player = app.entities.player.create(Vec3.new(0, 1, 0))

        -- Load first level
        app.systems.level.load("level1")
    end,

    -- Called every frame
    update = function(dt)
        -- Don't update if paused
        if app.main.state.paused then return end

        -- Update all systems
        app.systems.movement.update(dt)
        app.systems.combat.update(dt)
        app.systems.ai.update(dt)
        app.systems.collectibles.update(dt)
    end,

    -- Called every frame after update
    render = function()
        -- Render 3D world (handled by engine mostly)

        -- Render UI on top
        app.systems.hud.render()
        app.systems.menu.render()
    end,

    -- Called when game exits
    shutdown = function()
        app.systems.save.saveGame()
    end
}
```

## Entity Modules

Entity modules are factories that create game objects:

```lua
-- entities/player.lua
return {
    create = function(position)
        local entity = bestow.entity.create()

        bestow.entity.addComponent(entity, "Transform3D", {
            position = position,
            rotation = Quat.identity(),
            scale = Vec3.new(1, 1, 1)
        })

        bestow.entity.addComponent(entity, "MeshRenderer", {
            mesh = "meshes/player.obj",
            material = "materials/player"
        })

        bestow.entity.addComponent(entity, "Health", {
            current = 100,
            max = 100
        })

        return entity
    end
}
```

## System Modules

Systems contain update logic and game behavior. **Always use Action Builder for input** (not direct polling):

```lua
-- systems/movement.lua
local MOVE_SPEED = 8.0

return {
    update = function(dt)
        local self = app.systems.movement
        local state = app.main.state
        if not state.player then return end

        -- Read actions (registered in init via Action Builder)
        local moveX, moveZ = 0, 0
        if bestow.input.isActionActive("MoveLeft") then moveX = moveX - 1 end
        if bestow.input.isActionActive("MoveRight") then moveX = moveX + 1 end
        if bestow.input.isActionActive("MoveForward") then moveZ = moveZ - 1 end
        if bestow.input.isActionActive("MoveBack") then moveZ = moveZ + 1 end

        local moveDir = Vec3.new(moveX, 0, moveZ)
        if moveDir:lengthSquared() > 1.0 then
            moveDir = moveDir:normalize()
        end

        if moveDir:lengthSquared() > 0.01 then
            local velocity = moveDir * MOVE_SPEED
            bestow.physics3d.moveCharacter(state.player, Vec3.new(velocity.x, -1, velocity.z), dt)
        end
    end
}
```

## Level Modules

Levels define entity placement and configuration:

```lua
-- levels/level1.lua
return {
    name = "Forest Clearing",

    playerSpawn = Vec3.new(0, 1, 0),

    entities = {
        -- Enemies
        { type = "enemies.basic", position = Vec3.new(10, 0, 5) },
        { type = "enemies.basic", position = Vec3.new(-8, 0, 12) },

        -- Collectibles
        { type = "collectibles.coin", position = Vec3.new(5, 1, 0) },
        { type = "collectibles.coin", position = Vec3.new(7, 1, 0) },
        { type = "collectibles.powerup", position = Vec3.new(0, 1, 10), powerupType = "speed" }
    },

    -- Level-specific settings
    ambientLight = Color.new(0.3, 0.35, 0.4, 1),
    fogDensity = 0.02,
    fogColor = Color.new(0.6, 0.7, 0.8, 1),

    -- Music
    music = "sounds/music/forest.ogg",

    -- Called when level loads
    onLoad = function()
        app.systems.audio.playMusic("sounds/music/forest.ogg")
    end,

    -- Called when level unloads
    onUnload = function()
        app.systems.audio.stopMusic()
    end
}
```

## Data Modules

Static data tables for game configuration:

```lua
-- data/items.lua
return {
    health_potion = {
        name = "Health Potion",
        description = "Restores 50 health",
        icon = "textures/ui/health_potion.png",
        maxStack = 10,
        onUse = function()
            local state = app.main.state
            local health = bestow.entity.getComponent(state.player, "Health")
            health.current = math.min(health.max, health.current + 50)
            bestow.entity.setComponent(state.player, "Health", health)
        end
    },

    sword = {
        name = "Iron Sword",
        description = "A sturdy blade",
        icon = "textures/ui/sword.png",
        damage = 25,
        attackSpeed = 1.0
    }
}

-- Access:
local item = app.data.items.health_potion
print(item.name)  -- "Health Potion"
item.onUse()
```

## Configuration File

Optional engine configuration:

```lua
-- bestow.config.lua
return {
    window = {
        title = "My Awesome Game",
        width = 1920,
        height = 1080,
        fullscreen = false,
        vsync = true
    },

    graphics = {
        renderer = "vulkan",  -- or "opengl"
        shadowQuality = "high",
        antialiasing = "msaa4x"
    },

    audio = {
        masterVolume = 1.0,
        musicVolume = 0.7,
        sfxVolume = 1.0
    },

    physics = {
        gravity = Vec3.new(0, -9.81, 0),
        fixedTimestep = 1/60
    },

    debug = {
        showFPS = true,
        showColliders = false
    }
}
```

## Asset Directory Structure

```
assets/
├── textures/
│   ├── characters/
│   │   ├── player_albedo.png
│   │   ├── player_normal.png
│   │   └── enemy.png
│   ├── environment/
│   │   ├── grass.png
│   │   └── stone.png
│   └── ui/
│       ├── button.png
│       └── icons/
│
├── sounds/
│   ├── sfx/
│   │   ├── jump.wav
│   │   ├── hit.wav
│   │   └── pickup.wav
│   └── music/
│       ├── menu.ogg
│       └── gameplay.ogg
│
├── meshes/
│   ├── characters/
│   │   ├── player.obj
│   │   └── enemy.obj
│   └── environment/
│       ├── tree.obj
│       └── rock.obj
│
├── materials/
│   ├── characters/
│   │   └── player.lua
│   └── environment/
│       └── grass.lua
│
└── fonts/
    └── main.ttf
```

## Creating a New Project

```bash
# Create new project
bestow new my-game

# This creates:
# my-game/
# ├── main.lua
# ├── bestow.config.lua
# ├── entities/
# ├── systems/
# ├── levels/
# ├── data/
# └── assets/

# Run the game
cd my-game
bestow run main.lua --hot-reload
```

## File Naming Conventions

| Type | Convention | Example |
|------|------------|---------|
| Lua modules | lowercase, underscores | `player_controller.lua` |
| Directories | lowercase | `enemies/`, `collectibles/` |
| Textures | lowercase, underscores | `player_albedo.png` |
| Sounds | lowercase, underscores | `jump.wav`, `menu_music.ogg` |
| Meshes | lowercase | `player.obj`, `tree.gltf` |
| Materials | lowercase | `player.lua`, `grass.lua` |

## Best Practices

1. **Keep entities pure factories** - Only create/configure, no update logic
2. **Put logic in systems** - All game behavior in systems/
3. **Use data/ for static tables** - Item definitions, enemy stats, dialogue
4. **Organize by feature** - Group related files in subdirectories
5. **Name files descriptively** - `enemy_spawner.lua` not `spawner.lua`
6. **Use consistent asset naming** - `player_albedo.png`, `player_normal.png`
7. **Keep levels declarative** - Describe what's there, not how it works
