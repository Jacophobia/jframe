# Lua-First Integration Plan

> **Goal:** Lua is the ONLY way to build games with Bestow. C++ is infrastructure-only. It should be impossible to create a game without Lua.

## Table of Contents

1. [Current State Analysis](#current-state-analysis)
2. [Path Scheme Extensions](#path-scheme-extensions)
3. [Unified Lua Architecture](#unified-lua-architecture)
4. [Developer Experience: Before & After](#developer-experience-before--after)
5. [C++ Infrastructure Role](#c-infrastructure-role)
6. [Integration with Existing Systems](#integration-with-existing-systems)
7. [Migration Plan](#migration-plan)

---

## Current State Analysis

### Current Lua Usage (Fragmented)

| System | Lua Format | Location | Hot Reload |
|--------|------------|----------|------------|
| **Materials** | `return { shader = {...}, uniforms = {...} }` | `materials/*.lua` | ✅ Yes |
| **Levels** | `return { name, entities, spawnPoints }` | `levels/*.lua` | ✅ Yes |
| **Blueprints** | `Blueprints = { Player = {...} }` | `blueprints/*.lua` | ✅ Yes |
| **Config** | Key-value tables | `config/*.lua` | ✅ Yes |
| **World Defs** | `return { levels, nodes, boss }` | `worlds/*.lua` | ✅ Yes |

### Current Problems

1. **Game Logic in C++**: Snake game is 41K tokens of C++ - all game logic hardcoded
2. **No Behavior Scripts**: Entities have no scriptable behaviors
3. **No Lua Systems**: Game update loops are C++ only
4. **Scattered Patterns**: Each system has its own Lua format
5. **Optional Lua**: Games CAN be built in pure C++ (bad!)

### Current File Structure (Fragmented)
```
games/game1/
├── src/
│   ├── main.cpp              # C++ entry point
│   └── snake.game.cppm       # 41K tokens of C++ game logic!
└── data/
    ├── config/sounds.lua     # Just config
    └── worlds/world1/
        ├── world.lua         # World definition
        └── level01.lua       # Level layout only
```

---

## Path Scheme Extensions

### Current Schemes
```cpp
:assets:/   → Game assets directory (textures, sounds)
:library:/  → Engine library (built-in shaders)
```

### New Schemes for Lua-First Architecture

```cpp
namespace PathScheme {
    // Existing
    inline constexpr std::string_view Assets = ":assets:/";
    inline constexpr std::string_view Library = ":library:/";

    // NEW: Game content schemes
    inline constexpr std::string_view Blueprints = ":blueprints:/";   // Entity templates
    inline constexpr std::string_view Behaviors = ":behaviors:/";     // Entity scripts
    inline constexpr std::string_view Systems = ":systems:/";         // Game systems
    inline constexpr std::string_view Levels = ":levels:/";           // Level definitions
    inline constexpr std::string_view Materials = ":materials:/";     // Material definitions
    inline constexpr std::string_view Config = ":config:/";           // Configuration
    inline constexpr std::string_view Events = ":events:/";           // Event handlers
    inline constexpr std::string_view UI = ":ui:/";                   // UI definitions
}
```

### Usage in Lua

```lua
-- Spawn using blueprint scheme
local player = spawn(":blueprints:/player")
local enemy = spawn(":blueprints:/enemies/slime")

-- Load level using level scheme
loadLevel(":levels:/world1/forest_gate")

-- Reference materials
materials = {
    body = ":materials:/toon",
    glow = ":materials:/hologram"
}

-- Attach behaviors
behaviors = {
    ":behaviors:/player/movement",
    ":behaviors:/player/dash",
    ":behaviors:/common/health"
}
```

### Path Resolution

```cpp
class PathResolver {
public:
    // Extended initialization with game root
    static void initializeGame(std::string_view gameRoot) {
        auto& inst = instance();
        inst.gameRoot_ = gameRoot;

        // Set up scheme paths relative to game root
        inst.schemePaths_[":blueprints:/"] = inst.gameRoot_ / "blueprints";
        inst.schemePaths_[":behaviors:/"] = inst.gameRoot_ / "behaviors";
        inst.schemePaths_[":systems:/"] = inst.gameRoot_ / "systems";
        inst.schemePaths_[":levels:/"] = inst.gameRoot_ / "levels";
        inst.schemePaths_[":materials:/"] = inst.gameRoot_ / "materials";
        inst.schemePaths_[":config:/"] = inst.gameRoot_ / "config";
        inst.schemePaths_[":events:/"] = inst.gameRoot_ / "events";
        inst.schemePaths_[":ui:/"] = inst.gameRoot_ / "ui";
    }

    static std::filesystem::path resolve(std::string_view path) {
        for (const auto& [scheme, basePath] : instance().schemePaths_) {
            if (path.starts_with(scheme)) {
                auto remainder = path.substr(scheme.size());
                auto resolved = basePath / remainder;
                // Auto-append .lua if no extension
                if (!resolved.has_extension()) {
                    resolved += ".lua";
                }
                return resolved;
            }
        }
        return instance().doResolve(path);  // Fall back to existing logic
    }
};
```

---

## Unified Lua Architecture

### Application Entry Point: `app.lua`

Every Bestow application starts with a single entry point file. The engine searches in this order:
1. `./app.lua` (current directory)
2. `./main.lua` (alternative name)
3. `./data/app.lua` (data subdirectory)
4. `./game/app.lua` (game subdirectory)

This is the **only** thing C++ loads.

```lua
-- app.lua
-- This is the ONLY entry point. Everything flows from here.

return {
    --================================================================
    -- Game Metadata
    --================================================================
    name = "Snake Odyssey",
    version = "1.0.0",
    author = "Your Name",

    --================================================================
    -- Window Configuration
    --================================================================
    window = {
        width = 1920,
        height = 1080,
        title = "Snake Odyssey",
        fullscreen = false,
        vsync = true
    },

    --================================================================
    -- Input Bindings (Dvorak-friendly!)
    --================================================================
    input = {
        -- Movement: ,AOE for Dvorak, arrows as fallback
        move_up    = { keys = {"comma", "up"} },
        move_down  = { keys = {"o", "down"} },
        move_left  = { keys = {"a", "left"} },
        move_right = { keys = {"e", "right"} },

        -- Actions
        action     = { keys = {"space", "enter"} },
        pause      = { keys = {"escape", "p"} },
        dash       = { keys = {"shift"} }
    },

    --================================================================
    -- Asset Preloading
    --================================================================
    preload = {
        textures = {
            ":assets:/textures/snake.png",
            ":assets:/textures/food.png",
            ":assets:/textures/enemies/*.png"
        },
        sounds = {
            ":assets:/sounds/sfx/*.wav",
            ":assets:/sounds/music/*.ogg"
        },
        materials = {
            ":materials:/toon",
            ":materials:/hologram",
            ":materials:/glow"
        }
    },

    --================================================================
    -- Game Systems (loaded in order)
    --================================================================
    systems = {
        ":systems:/input",           -- Input processing
        ":systems:/physics",         -- Physics integration
        ":systems:/combat",          -- Damage and health
        ":systems:/spawning",        -- Entity spawning
        ":systems:/camera",          -- Camera control
        ":systems:/rendering",       -- Render ordering
        ":systems:/audio",           -- Sound effects
        ":systems:/ui"               -- UI updates
    },

    --================================================================
    -- Game Lifecycle
    --================================================================

    -- Called once when game starts
    init = function()
        -- Load first world
        game.loadWorld(":levels:/world1/world")

        -- Start at first level
        game.loadLevel(":levels:/world1/level01")

        -- Spawn player at spawn point
        local spawnPoint = level.spawnPoints.default
        game.player = spawn(":blueprints:/player", spawnPoint.x, spawnPoint.z)

        -- Set up camera to follow player
        camera.follow(game.player)
        camera.setStyle("isometric", { angle = 45, distance = 15 })

        -- Start music
        audio.playMusic(":assets:/sounds/music/game.ogg", { fadeIn = 2.0 })

        -- Initialize game state
        game.state = {
            score = 0,
            lives = 3,
            currentWorld = 1,
            currentLevel = 1
        }
    end,

    -- Called every frame
    update = function(dt)
        -- Global game logic here
        -- Most logic is in systems and behaviors

        -- Check for pause
        if input.pressed("pause") then
            game.togglePause()
        end
    end,

    -- Called when game shuts down
    shutdown = function()
        -- Save progress
        save.write("progress", game.state)
    end
}
```

### Blueprint Format (Unified)

```lua
-- blueprints/player.lua
return {
    name = "Player",

    --================================================================
    -- Core Components (C++ backed, high performance)
    --================================================================
    components = {
        Transform3D = {
            position = {0, 0, 0},
            rotation = {0, 0, 0, 1},  -- Quaternion
            scale = {1, 1, 1}
        },
        Velocity = {x = 0, y = 0, z = 0},
        Mesh3D = {
            mesh = ":assets:/models/snake_head.obj",
            material = ":materials:/toon"
        }
    },

    -- Physics body (optional, C++ physics engine)
    physics = {
        type = "dynamic",
        shape = "capsule",
        radius = 0.5,
        height = 1.0,
        mass = 1.0,
        friction = 0.3
    },

    --================================================================
    -- Game Data (Lua tables, fully hot-reloadable)
    --================================================================
    data = {
        -- Stats
        stats = {
            moveSpeed = 5.0,
            turnSpeed = 180.0,  -- degrees per second
            dashSpeed = 15.0,
            dashDuration = 0.2,
            dashCooldown = 1.0
        },

        -- Health
        health = {
            current = 100,
            max = 100,
            invincibilityDuration = 1.0
        },

        -- Snake-specific
        snake = {
            segments = {},
            growthPending = 0,
            moveInterval = 0.15,
            moveTimer = 0
        },

        -- State
        state = {
            isAlive = true,
            isDashing = false,
            isInvincible = false,
            facingDirection = {x = 1, z = 0}
        }
    },

    --================================================================
    -- Behaviors (Lua scripts, hot-reloadable)
    --================================================================
    behaviors = {
        ":behaviors:/player/grid_movement",
        ":behaviors:/player/snake_body",
        ":behaviors:/player/dash",
        ":behaviors:/player/attack",
        ":behaviors:/common/health",
        ":behaviors:/common/invincibility"
    },

    --================================================================
    -- Tags (for queries)
    --================================================================
    tags = {"player", "snake", "friendly", "controllable"},

    --================================================================
    -- Event Handlers (inline or referenced)
    --================================================================
    events = {
        onSpawn = function(self)
            audio.play(":assets:/sounds/sfx/spawn.wav")
        end,

        onDestroy = function(self)
            -- Drop segments, spawn death effect
            for _, segment in ipairs(self.data.snake.segments) do
                spawn(":blueprints:/dropped_segment", segment.position)
            end
            spawn(":blueprints:/effects/death_explosion", self:get("Transform3D").position)
        end
    }
}
```

### Behavior Format (Unified)

```lua
-- behaviors/player/grid_movement.lua
return {
    name = "GridMovement",

    --================================================================
    -- State (preserved across hot reload)
    --================================================================
    state = {
        moveTimer = 0,
        currentDirection = {x = 1, z = 0},
        nextDirection = {x = 1, z = 0},
        bufferedDirection = nil
    },

    --================================================================
    -- Lifecycle
    --================================================================

    init = function(self, entity)
        -- Cache frequently accessed data
        self.moveInterval = entity.data.snake.moveInterval
        self.gridSize = 1.0
    end,

    update = function(self, entity, dt)
        -- Read input
        local inputDir = self:readDirectionInput()
        if inputDir then
            -- Buffer the input
            self.state.bufferedDirection = inputDir
        end

        -- Update move timer
        self.state.moveTimer = self.state.moveTimer + dt

        if self.state.moveTimer >= self.moveInterval then
            self.state.moveTimer = self.state.moveTimer - self.moveInterval
            self:executeMove(entity)
        end
    end,

    --================================================================
    -- Methods
    --================================================================

    readDirectionInput = function(self)
        if input.pressed("move_up") then
            return {x = 0, z = -1}
        elseif input.pressed("move_down") then
            return {x = 0, z = 1}
        elseif input.pressed("move_left") then
            return {x = -1, z = 0}
        elseif input.pressed("move_right") then
            return {x = 1, z = 0}
        end
        return nil
    end,

    executeMove = function(self, entity)
        -- Apply buffered direction if valid (can't reverse)
        if self.state.bufferedDirection then
            local bd = self.state.bufferedDirection
            local cd = self.state.currentDirection
            -- Can't reverse direction
            if not (bd.x == -cd.x and bd.z == -cd.z) then
                self.state.nextDirection = bd
            end
            self.state.bufferedDirection = nil
        end

        self.state.currentDirection = self.state.nextDirection

        -- Move the snake
        local transform = entity:get("Transform3D")
        local dir = self.state.currentDirection

        -- Store old position for body segments
        local oldPos = {
            x = transform.position.x,
            y = transform.position.y,
            z = transform.position.z
        }

        -- Move head
        transform.position.x = transform.position.x + dir.x * self.gridSize
        transform.position.z = transform.position.z + dir.z * self.gridSize

        -- Update facing direction
        entity.data.state.facingDirection = dir

        -- Notify body behavior to update segments
        entity:emit("head_moved", {
            oldPosition = oldPos,
            newPosition = transform.position,
            direction = dir
        })

        -- Check collisions
        entity:emit("check_collision")
    end,

    --================================================================
    -- Hot Reload
    --================================================================

    onHotReload = function(self, entity)
        -- Re-cache values that might have changed
        self.moveInterval = entity.data.snake.moveInterval
        print("[GridMovement] Hot reloaded! Move interval:", self.moveInterval)
    end
}
```

### System Format (Unified)

```lua
-- systems/combat.lua
return {
    name = "CombatSystem",
    priority = 100,  -- Lower = earlier

    --================================================================
    -- State
    --================================================================
    state = {
        damageQueue = {},
        hitStopTimer = 0,
        hitStopDuration = 0
    },

    --================================================================
    -- Lifecycle
    --================================================================

    init = function(self)
        -- Subscribe to damage events
        events.on("damage_requested", function(data)
            table.insert(self.state.damageQueue, data)
        end)

        -- Subscribe to collision events
        events.on("collision", function(a, b, contact)
            self:handleCollision(a, b, contact)
        end)
    end,

    update = function(self, dt)
        -- Handle hit stop
        if self.state.hitStopTimer > 0 then
            self.state.hitStopTimer = self.state.hitStopTimer - dt
            if self.state.hitStopTimer <= 0 then
                time.resume()
            end
            return  -- Skip processing during hit stop
        end

        -- Process damage queue
        for _, dmg in ipairs(self.state.damageQueue) do
            self:applyDamage(dmg.target, dmg.amount, dmg.source, dmg.type)
        end
        self.state.damageQueue = {}
    end,

    shutdown = function(self)
        events.off("damage_requested")
        events.off("collision")
    end,

    --================================================================
    -- Methods
    --================================================================

    handleCollision = function(self, entityA, entityB, contact)
        -- Check for damage dealer hitting damageable
        local damagerA = entityA.data.damage
        local healthB = entityB.data.health

        if damagerA and healthB then
            events.emit("damage_requested", {
                target = entityB,
                source = entityA,
                amount = damagerA.amount,
                type = damagerA.type or "physical",
                knockback = damagerA.knockback,
                contactPoint = contact.point
            })
        end

        -- Check reverse
        local damagerB = entityB.data.damage
        local healthA = entityA.data.health

        if damagerB and healthA then
            events.emit("damage_requested", {
                target = entityA,
                source = entityB,
                amount = damagerB.amount,
                type = damagerB.type or "physical",
                knockback = damagerB.knockback,
                contactPoint = contact.point
            })
        end
    end,

    applyDamage = function(self, target, amount, source, damageType)
        local health = target.data.health
        if not health then return end

        -- Check invincibility
        if target.data.state and target.data.state.isInvincible then
            return
        end

        -- Let behaviors modify damage
        local modifiedAmount = target:callBehaviors("onBeforeDamage", amount, source, damageType)
        if modifiedAmount == false then
            return  -- Damage blocked
        end
        amount = modifiedAmount or amount

        -- Apply damage
        health.current = math.max(0, health.current - amount)

        -- Visual feedback
        self:triggerHitStop(0.05)
        camera.shake(0.2, amount * 0.01)

        -- Spawn hit effect
        local pos = target:get("Transform3D").position
        spawn(":blueprints:/effects/hit_spark", pos)

        -- Play sound
        audio.play(":assets:/sounds/sfx/hit.wav", {
            position = pos,
            volume = 0.8,
            pitchVariation = 0.1
        })

        -- Emit damage event
        target:emit("damaged", {
            amount = amount,
            source = source,
            type = damageType,
            newHealth = health.current
        })

        -- Check death
        if health.current <= 0 then
            target:emit("died", { killer = source })
            target:destroy()
        end
    end,

    triggerHitStop = function(self, duration)
        self.state.hitStopDuration = duration
        self.state.hitStopTimer = duration
        time.pause()
    end
}
```

### Level Format (Unified)

```lua
-- levels/world1/level01.lua
return {
    name = "Forest Gate",

    --================================================================
    -- Level Metadata
    --================================================================
    meta = {
        width = 200,
        height = 200,
        theme = "forest",
        music = ":assets:/sounds/music/forest.ogg",
        ambience = ":assets:/sounds/ambience/forest.ogg"
    },

    --================================================================
    -- Spawn Points
    --================================================================
    spawnPoints = {
        default = { x = 100, z = 100 },
        checkpoint1 = { x = 150, z = 100 },
        bossArena = { x = 100, z = 180 }
    },

    --================================================================
    -- Static Entities (spawned once on level load)
    --================================================================
    entities = {
        -- Ground plane
        {
            blueprint = ":blueprints:/environment/ground",
            position = {x = 100, y = 0, z = 100},
            data = {
                size = {width = 200, height = 200},
                material = ":materials:/terrain"
            }
        },

        -- Walls (can use procedural generation!)
        (function()
            local walls = {}
            -- Generate circular wall pattern
            for ring = 1, 5 do
                local radius = ring * 20
                local count = ring * 6
                for i = 1, count do
                    local angle = (i / count) * math.pi * 2
                    table.insert(walls, {
                        blueprint = ":blueprints:/environment/wall",
                        position = {
                            x = 100 + math.cos(angle) * radius,
                            y = 0,
                            z = 100 + math.sin(angle) * radius
                        }
                    })
                end
            end
            return table.unpack(walls)
        end)()
    },

    --================================================================
    -- Dynamic Spawners (spawn during gameplay)
    --================================================================
    spawners = {
        -- Food spawner
        {
            type = "random",
            blueprint = ":blueprints:/items/food",
            interval = 5.0,
            maxActive = 3,
            area = { minX = 10, maxX = 190, minZ = 10, maxZ = 190 },
            avoidTags = {"wall", "player", "enemy"}
        },

        -- Enemy spawner
        {
            type = "wave",
            waves = {
                { delay = 10, blueprint = ":blueprints:/enemies/slime", count = 2 },
                { delay = 30, blueprint = ":blueprints:/enemies/slime", count = 4 },
                { delay = 60, blueprint = ":blueprints:/enemies/fast_slime", count = 3 }
            },
            spawnArea = { minX = 20, maxX = 180, minZ = 20, maxZ = 180 }
        }
    },

    --================================================================
    -- Level Logic (optional)
    --================================================================

    onLoad = function(level)
        -- Level-specific initialization
        game.state.foodCollected = 0
        game.state.foodRequired = 10
    end,

    onUpdate = function(level, dt)
        -- Check win condition
        if game.state.foodCollected >= game.state.foodRequired then
            level:complete()
        end
    end,

    onComplete = function(level)
        audio.play(":assets:/sounds/sfx/level_complete.wav")

        -- Transition to next level
        after(2.0, function()
            game.loadLevel(":levels:/world1/level02")
        end)
    end
}
```

---

## Developer Experience: Before & After

### BEFORE: Building a Game (Current)

```
Step 1: Write C++ game class (snake.game.cppm - 41K tokens)
Step 2: Add member variables for all game state
Step 3: Write update logic in C++
Step 4: Recompile (30-60 seconds)
Step 5: Test
Step 6: Find bug, go to Step 3
Step 7: Want to tweak player speed? Recompile.
Step 8: Want to add new enemy type? Write C++ class, recompile.
Step 9: Want to change level layout? Edit C++ arrays, recompile.
```

**Time to see a change: 30-60 seconds (full rebuild)**

### AFTER: Building a Game (Lua-First)

```
Step 1: Create app.lua (entry point)
Step 2: Create blueprints/ for entity templates
Step 3: Create behaviors/ for entity logic
Step 4: Create levels/ for world layouts
Step 5: Run game ONCE
Step 6: Edit any Lua file, save
Step 7: See change INSTANTLY (< 100ms hot reload)
Step 8: Want to tweak player speed? Edit blueprint, save. Done.
Step 9: Want to add new enemy? Create new .lua file, save. Done.
Step 10: Want to change level? Edit level.lua, save. Done.
```

**Time to see a change: < 100ms (hot reload)**

### Side-by-Side Comparison

| Task | Before (C++) | After (Lua) |
|------|--------------|-------------|
| Change player speed | Edit .cppm, recompile (60s) | Edit .lua, save (instant) |
| Add new enemy type | New C++ class, recompile (60s) | New .lua file, save (instant) |
| Tweak ability cooldown | Edit .cppm, recompile (60s) | Edit .lua, save (instant) |
| Change level layout | Edit C++ arrays, recompile (60s) | Edit .lua, save (instant) |
| Fix gameplay bug | Edit .cppm, recompile (60s) | Edit .lua, save (instant) |
| Add new behavior | New C++ code, recompile (60s) | New .lua file, save (instant) |
| Test different values | Many recompiles | Just edit and save |

### What the Developer's Day Looks Like

**Morning Session (Lua-First):**
```
9:00 - Start game (once)
9:01 - Edit player dash speed in dash.lua → instant update
9:02 - Tweak enemy patrol pattern in patrol.lua → instant update
9:05 - Add new slime variant (copy slime.lua, modify) → instant update
9:10 - Adjust level layout in level01.lua → instant update
9:30 - Add screen shake to combat.lua → instant update
10:00 - New ability: create teleport.lua → instant update
... (never recompiled C++)
```

**The game stays running the entire session. You just edit Lua files and see changes instantly.**

---

## C++ Infrastructure Role

### What C++ Does (And ONLY This)

```cpp
// main.cpp - This is ALL the C++ a game developer ever writes

#include <bestow/engine.hpp>

int main(int argc, char* argv[]) {
    // 1. Initialize path resolver
    bestow::PathResolver::initialize(argv[0]);

    // 2. Create engine
    bestow::Engine engine;

    // 3. Register implementations (use defaults or custom)
    engine.useDefaults();  // Or pick specific implementations:
    // engine.use<IGraphics3DSystem, VulkanGraphics3DSystem>();
    // engine.use<IPhysicsSystem, JoltPhysicsSystem>();
    // engine.use<IAudioSystem, FMODAudioSystem>();

    // 4. Run application from Lua (auto-discovers app.lua)
    engine.run();  // Or: engine.run("path/to/app.lua");

    return 0;
}
```

**That's it. ~15 lines of C++. Everything else is Lua.**

### The C++ Engine Provides

| C++ System | What It Does | Exposed to Lua |
|------------|--------------|----------------|
| **VulkanGraphics3DSystem** | Rendering pipeline, shaders, meshes | `mesh`, `material`, `camera`, `light` |
| **JoltPhysicsSystem** | Physics simulation, collision | `physics`, `raycast`, `collision events` |
| **FMODAudioSystem** | 3D audio, music, mixing | `audio.play()`, `audio.playMusic()` |
| **AssetSystem** | Loading, caching, hot reload | Automatic, transparent |
| **InputSystem** | Keyboard, mouse, gamepad | `input.pressed()`, `input.held()` |
| **LuaRuntime** | Script execution, hot reload | Everything else |

### What You CANNOT Do in C++ Anymore

With this architecture, these are **not possible**:

```cpp
// ❌ IMPOSSIBLE - No direct entity creation
entities->createEntity();  // Not exposed

// ❌ IMPOSSIBLE - No component manipulation
entities->emplace<Health>(entity, 100, 100);  // Not exposed

// ❌ IMPOSSIBLE - No game logic in C++
void update(float dt) {
    // Handle movement...  // Can't write this in C++
}

// ❌ IMPOSSIBLE - No system implementation in C++
class MyCombatSystem { ... };  // Must be Lua
```

### Enforcing Lua-Only

```cpp
// IEntitySystem interface changes:
class IEntitySystem {
public:
    // These are INTERNAL ONLY - not exposed to game code
    virtual Entity createEntity() = 0;  // Called by LuaRuntime only

    // This is the ONLY public API
    virtual ILuaRuntime& getLuaRuntime() = 0;
};

// LuaRuntime is the ONLY way to interact with the game
class ILuaRuntime {
public:
    // Game developers use ONLY this
    virtual bool loadGame(std::string_view gameEntryPoint) = 0;
};
```

---

## Integration with Existing Systems

### AssetSystem Integration

The AssetSystem already supports hot reload. We extend it:

```cpp
// New asset type
enum class AssetType : std::uint8_t {
    // ... existing
    Script,      // Lua scripts (behaviors, systems)
    Blueprint,   // Entity blueprints
    Level,       // Level definitions
    GameConfig   // game.lua and config files
};

// LuaRuntime subscribes to all Lua-related asset types
void LuaRuntime::init() {
    // Subscribe to script changes
    assets_->subscribeToType(AssetType::Script, [this](AssetHandle h, AssetType t) {
        onScriptChanged(h, t);
    });

    // Subscribe to blueprint changes
    assets_->subscribeToType(AssetType::Blueprint, [this](AssetHandle h, AssetType t) {
        onBlueprintChanged(h, t);
    });

    // Subscribe to level changes
    assets_->subscribeToType(AssetType::Level, [this](AssetHandle h, AssetType t) {
        onLevelChanged(h, t);
    });

    // Enable hot reload
    assets_->enableHotReload(true);
}
```

### Shader System Integration

Materials already hot-reload. LuaRuntime reads the same format:

```lua
-- materials/toon.lua (unchanged format)
return {
    shader = {
        vertex = ":library:/shaders/basic.vert",
        fragment = ":library:/shaders/toon.frag"
    },
    uniforms = {
        uBaseColor = {0.8, 0.2, 0.3, 1.0},
        uBands = 3
    },
    hotReload = true
}
```

### BlueprintFactory Integration

BlueprintFactory becomes a thin wrapper around LuaRuntime:

```cpp
// BlueprintFactory now just delegates
Entity BlueprintFactory::create(std::string_view name, float x, float y,
                                const PropertyMap& overrides) {
    // Convert C++ overrides to Lua table
    sol::table luaOverrides = propertyMapToLua(overrides);

    // Delegate to LuaRuntime
    return luaRuntime_->spawn(name, x, y, luaOverrides);
}
```

### Config System Integration

ConfigSystem already loads Lua. It now uses the same LuaRuntime state:

```cpp
// ConfigSystem uses LuaRuntime's sol::state
class ConfigSystem : public IConfigSystem {
    void loadConfig(std::string_view path) {
        // Use shared Lua state for consistency
        luaRuntime_->executeFile(path);
        // Config is now available in Lua global scope
    }
};
```

### Hot Reload Flow (Unified)

```
┌─────────────────────────────────────────────────────────────────┐
│  File Change (any .lua file)                                    │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│  efsw detects change → AssetSystem queues event                 │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│  AssetSystem::update() → notifySubscribers()                    │
└─────────────────────────────────────────────────────────────────┘
                              │
            ┌─────────────────┼─────────────────┐
            ▼                 ▼                 ▼
┌───────────────────┐ ┌───────────────┐ ┌───────────────────────┐
│  Shader changed   │ │ Material .lua │ │ Script/Blueprint/Level│
│  → ShaderSystem   │ │ → ShaderSystem│ │ → LuaRuntime          │
│  recompiles SPIRV │ │ reloads mat   │ │ hot reloads script    │
└───────────────────┘ └───────────────┘ └───────────────────────┘
```

---

## Implementation Phases

> See `LUA-RUNTIME-IMPLEMENTATION.md` for detailed task breakdown and complete API specifications.

### Phase 1: Core LuaRuntime

1. **Create ILuaRuntime interface**
   - Define contract for Lua script execution
   - Define entity wrapper API
   - Define system/behavior interfaces

2. **Implement LuaRuntime**
   - sol2 state management
   - Script loading and execution
   - Hot reload with state preservation

3. **Extend PathResolver**
   - Add new path schemes
   - Auto-append .lua extension
   - Application root configuration

### Phase 2: Entity Integration

1. **Lua Entity API**
   - `spawn()` function
   - `entity:get()`, `entity:set()`
   - `entity:emit()`, `entity:on()`
   - `query.withTag()`, `query.with()`

2. **Behavior System**
   - Behavior attachment
   - Update dispatch
   - State preservation

3. **Blueprint Loading**
   - Unified blueprint format
   - Component instantiation
   - Behavior instantiation

### Phase 3: System Bindings

1. **Core System Bindings**
   - `input.*` bindings (IInputSystem)
   - `audio.*` bindings (IAudioSystem)
   - `camera.*` bindings (camera control)
   - `physics.*` bindings (IPhysicsSystem, IPhysics3DSystem)

2. **UI System Bindings**
   - `ui.*` bindings (IUISystem)
   - Document management
   - Element access and manipulation
   - Data binding

3. **Additional Bindings**
   - `animation.*` bindings (IAnimationSystem)
   - `save.*` bindings (ISaveSystem)
   - `level.*` bindings (ILevelSystem)

### Phase 4: Lua Systems

1. **Lua Systems**
   - System priority ordering
   - System lifecycle (init, update, shutdown)
   - Event subscriptions

2. **Level Loading**
   - Unified level format
   - Entity spawning from level
   - Spawner system

### Phase 5: Example Application Migration

1. **Migrate Snake Game to Lua**
   - Extract snake.game.cppm logic to Lua
   - Create blueprints/player.lua
   - Create behaviors/movement.lua, etc.
   - Create systems/combat.lua, etc.
   - Create levels/ from existing Lua

2. **Documentation**
   - Getting started guide
   - API reference
   - Example projects

3. **Tooling**
   - Dev console for runtime inspection
   - Error display overlay
   - Performance profiler

---

## File Structure (Final)

```
mygame/
├── main.cpp                    # ~6 lines, infrastructure only
├── app.lua                     # Application entry point
├── config/
│   ├── settings.lua           # Game settings
│   └── audio.lua              # Audio settings
├── blueprints/
│   ├── player.lua
│   ├── enemies/
│   │   ├── slime.lua
│   │   └── boss.lua
│   ├── items/
│   │   ├── food.lua
│   │   └── powerup.lua
│   ├── effects/
│   │   ├── hit_spark.lua
│   │   └── death_explosion.lua
│   └── environment/
│       ├── wall.lua
│       └── ground.lua
├── behaviors/
│   ├── player/
│   │   ├── grid_movement.lua
│   │   ├── snake_body.lua
│   │   ├── dash.lua
│   │   └── attack.lua
│   ├── enemies/
│   │   ├── patrol.lua
│   │   ├── chase.lua
│   │   └── wander.lua
│   └── common/
│       ├── health.lua
│       ├── lifetime.lua
│       └── invincibility.lua
├── systems/
│   ├── input.lua
│   ├── combat.lua
│   ├── spawning.lua
│   ├── camera.lua
│   └── ui.lua
├── levels/
│   ├── world1/
│   │   ├── world.lua
│   │   ├── level01.lua
│   │   └── level02.lua
│   └── world2/
│       └── ...
├── materials/
│   ├── toon.lua
│   └── hologram.lua
├── ui/
│   ├── hud.lua
│   ├── pause_menu.lua
│   └── game_over.lua
└── assets/
    ├── textures/
    ├── models/
    ├── sounds/
    └── fonts/
```

---

## Summary

| Aspect | Before | After |
|--------|--------|-------|
| **Game logic location** | C++ (snake.game.cppm) | Lua (behaviors/, systems/) |
| **Entity creation** | C++ `entities->createEntity()` | Lua `spawn()` only |
| **Iteration speed** | 30-60s rebuild | <100ms hot reload |
| **C++ code needed** | 41K+ tokens | ~6 lines |
| **Path references** | Hardcoded strings | Scheme prefixes (:blueprints:/) |
| **Hot reload scope** | Shaders only | Everything |
| **Barrier to entry** | Know C++ + ECS + CMake | Know Lua only |

**The engine is C++. The game is Lua. There is no in-between.**
