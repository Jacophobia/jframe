# Lua Runtime Implementation Guide

> **Vision:** Zero-rebuild game development. Edit Lua, see changes instantly. C++ is infrastructure only.

## Application Entry Point

### Standard Location: `app.lua`

The engine searches for the application entry point in this order:

1. `./app.lua` (current directory)
2. `./main.lua` (alternative name)
3. `./data/app.lua` (data subdirectory)
4. `./game/app.lua` (game subdirectory)

Or specified explicitly:
```cpp
engine.run("path/to/my/app.lua");
```

### Minimal `app.lua`

```lua
-- app.lua - Application entry point
return {
    name = "My Application",
    version = "1.0.0",

    window = {
        width = 1920,
        height = 1080,
        title = "My Application"
    },

    init = function()
        -- Application startup
    end,

    update = function(dt)
        -- Main loop
    end,

    shutdown = function()
        -- Cleanup
    end
}
```

---

## Complete System Inventory

### Systems Requiring Lua Bindings

Based on analysis of `bestow-contract/src/`, here are all systems that need Lua exposure:

| System | Interface | Priority | Complexity | Current Lua Support |
|--------|-----------|----------|------------|---------------------|
| **Entity** | IEntitySystem | Critical | High | None |
| **Events** | IEventSystem | Critical | Medium | None |
| **Assets** | IAssetSystem | Critical | Medium | Partial (paths) |
| **Config** | IConfigSystem | Critical | Low | ✅ Full |
| **Input** | IInputSystem | Critical | Medium | None |
| **Graphics3D** | IGraphics3DSystem | Critical | High | None |
| **Graphics2D** | IGraphicsSystem | High | Medium | None |
| **Physics2D** | IPhysicsSystem | High | Medium | None |
| **Physics3D** | IPhysics3DSystem | High | High | None |
| **Audio** | IAudioSystem | High | Low | None |
| **Level** | ILevelSystem | High | Medium | Partial (loading) |
| **UI** | IUISystem | High | High | None |
| **Animation** | IAnimationSystem | Medium | High | None |
| **AnimStateMachine** | IAnimationStateMachine | Medium | Medium | None |
| **AI** | IAISystem | Medium | Medium | None |
| **Save** | ISaveSystem | Medium | Low | None |
| **Blueprints** | IBlueprintFactory | High | Medium | Partial (loading) |
| **GameState** | IGameStateSystem | Medium | Medium | None |

---

## Lua API Design

### Global Namespaces

```lua
-- Entity creation and queries
spawn(blueprint, x, y, [z], [overrides])    -- Create entity
destroy(entity)                              -- Destroy entity
query                                        -- Query namespace

-- System access
input                                        -- Input system
audio                                        -- Audio system
physics                                      -- Physics system
camera                                       -- Camera control
level                                        -- Level system
save                                         -- Save system
ui                                           -- UI system
animation                                    -- Animation system

-- Utilities
events                                       -- Event system
timer                                        -- Timer utilities
math                                         -- Extended math
debug                                        -- Debug utilities

-- Application state
app                                          -- Application data
time                                         -- Time utilities
```

---

## Detailed API Specifications

### 1. Entity System (`spawn`, `destroy`, `query`)

#### `spawn(blueprint, x, y, [z], [overrides])` → Entity

```lua
-- 2D spawn
local player = spawn(":blueprints:/player", 100, 200)

-- 3D spawn
local enemy = spawn(":blueprints:/enemies/slime", 10, 0, 15)

-- With overrides
local boss = spawn(":blueprints:/enemies/boss", 0, 0, 0, {
    ["data.health.max"] = 500,
    ["data.health.current"] = 500,
    ["data.stats.damage"] = 25
})
```

#### Entity Methods

```lua
-- Component access (C++ components)
entity:get("Transform3D")                -- Get component
entity:get("Transform3D").position.x     -- Access fields
entity:set("Velocity", {x = 10, y = 0})  -- Set component
entity:has("Health")                     -- Check component

-- Lua data access
entity.data.health.current               -- Direct access
entity.data.stats.speed = 200            -- Direct modification

-- Behavior interaction
entity:getBehavior("Dash")               -- Get specific behavior
entity:callBehaviors("onDamage", 10)     -- Call method on all behaviors
entity:hasBehavior("Movement")           -- Check behavior

-- Events
entity:emit("damaged", {amount = 10})    -- Emit entity event
entity:on("damaged", function(data) end) -- Subscribe to entity event

-- Tags
entity:hasTag("enemy")                   -- Check tag
entity:addTag("poisoned")                -- Add tag
entity:removeTag("poisoned")             -- Remove tag
entity:getTags()                         -- Get all tags

-- Lifecycle
entity:destroy()                         -- Destroy entity
entity:isValid()                         -- Check if valid
entity:id()                              -- Get entity ID
```

#### Query API

```lua
-- By tag
local enemies = query.withTag("enemy")
local players = query.withTag("player")

-- Multiple tags (AND)
local activeEnemies = query.withTags("enemy", "active")

-- Any tag (OR)
local targets = query.withAnyTag("enemy", "destructible")

-- By components
local moving = query.with("Transform3D", "Velocity")

-- Spatial queries
local nearby = query.inRadius(x, y, z, radius)
local inBox = query.inBox(minX, minY, minZ, maxX, maxY, maxZ)
local inCircle = query.inCircle2D(x, y, radius)  -- 2D

-- Single entity
local player = query.single("player")            -- Error if 0 or 2+
local first = query.first("enemy")               -- First match

-- Count
local enemyCount = query.count("enemy")

-- Iteration
query.each("enemy", function(entity)
    entity.data.health.current = entity.data.health.current - 1
end)
```

### 2. Input System (`input`)

```lua
-- Action queries (uses input bindings from app.lua)
input.pressed("jump")              -- Just pressed this frame
input.released("jump")             -- Just released this frame
input.held("move_left")            -- Currently held

-- Axis values (-1 to 1)
input.axis("move_horizontal")      -- Combined left/right
input.axis("move_vertical")        -- Combined up/down

-- Raw key access
input.keyPressed(KEY.SPACE)
input.keyHeld(KEY.SHIFT)
input.keyReleased(KEY.ESCAPE)

-- Mouse
input.mousePosition()              -- Returns x, y
input.mouseDelta()                 -- Returns dx, dy
input.mouseButton(0)               -- Left button
input.mouseButton(1)               -- Right button
input.scroll()                     -- Returns dx, dy

-- Modifiers
input.shift()                      -- Shift held
input.ctrl()                       -- Ctrl held
input.alt()                        -- Alt held

-- Controller
input.controllerConnected(0)       -- Is controller 0 connected
input.controllerAxis(0, "left_x")  -- Controller axis
input.controllerButton(0, "a")     -- Controller button

-- Text input mode (for UI)
input.enableTextInput()
input.disableTextInput()
input.getTextInput()               -- Get typed text
input.clearTextInput()
```

### 3. Audio System (`audio`)

```lua
-- Sound effects (fire and forget)
audio.play(":assets:/sounds/sfx/jump.wav")
audio.play(":assets:/sounds/sfx/hit.wav", {
    volume = 0.8,
    pitch = 1.2,
    pitchVariation = 0.1           -- Random variation
})

-- Positional audio
audio.playAt(":assets:/sounds/sfx/explosion.wav", x, y, z)
audio.playAt(":assets:/sounds/sfx/explosion.wav", x, y, z, {
    volume = 1.0,
    minDistance = 5,
    maxDistance = 50,
    rolloff = "linear"             -- or "inverse", "custom"
})

-- Managed sounds (can control later)
local handle = audio.playSFX(":assets:/sounds/sfx/engine.wav", {loop = true})
audio.stop(handle)
audio.pause(handle)
audio.resume(handle)
audio.setVolume(handle, 0.5)
audio.setPitch(handle, 1.5)

-- Music (single music channel)
audio.playMusic(":assets:/sounds/music/level1.ogg")
audio.playMusic(":assets:/sounds/music/boss.ogg", {
    fadeIn = 2.0,
    loop = true
})
audio.stopMusic()
audio.stopMusic({fadeOut = 1.0})
audio.pauseMusic()
audio.resumeMusic()
audio.setMusicVolume(0.7)

-- Global controls
audio.setMasterVolume(1.0)
audio.setSFXVolume(0.8)
audio.pauseAll()
audio.resumeAll()
audio.stopAll()

-- 3D listener (for positional audio)
audio.setListener(x, y, z, forwardX, forwardY, forwardZ)
audio.setListenerVelocity(vx, vy, vz)  -- For doppler
```

### 4. Physics System (`physics`)

```lua
-- 2D Physics
physics.raycast2D(startX, startY, endX, endY)
physics.raycast2D(startX, startY, endX, endY, {
    mask = COLLISION.ENEMY | COLLISION.WALL,
    ignoreEntity = player
})

physics.circleQuery2D(x, y, radius)
physics.boxQuery2D(minX, minY, maxX, maxY)

physics.isGrounded(entity)         -- Ground check
physics.applyForce(entity, fx, fy)
physics.applyImpulse(entity, ix, iy)
physics.setVelocity(entity, vx, vy)
physics.getVelocity(entity)        -- Returns vx, vy

-- 3D Physics
physics.raycast(origin, direction, maxDistance)
physics.raycast(origin, direction, maxDistance, {
    mask = COLLISION.ENEMY,
    ignoreEntity = player
})

physics.sphereQuery(x, y, z, radius)
physics.boxQuery(center, halfExtents, rotation)
physics.sphereCast(origin, radius, direction, distance)

physics.applyForce3D(entity, fx, fy, fz)
physics.applyImpulse3D(entity, ix, iy, iz)
physics.applyTorque(entity, tx, ty, tz)

-- Body manipulation
physics.setPosition(entity, x, y, z)
physics.setRotation(entity, qx, qy, qz, qw)
physics.setLinearVelocity(entity, vx, vy, vz)
physics.setAngularVelocity(entity, wx, wy, wz)

-- Character controller
physics.moveCharacter(entity, dx, dy, dz, dt)
physics.getGroundInfo(entity)      -- Returns state, normal, entity

-- World settings
physics.setGravity(0, -9.8, 0)
physics.getGravity()
```

### 5. Camera System (`camera`)

```lua
-- Camera control
camera.setPosition(x, y, z)
camera.getPosition()               -- Returns x, y, z
camera.setRotation(pitch, yaw, roll)
camera.lookAt(x, y, z)
camera.lookAt(targetEntity)

-- Camera following
camera.follow(entity)
camera.follow(entity, {
    offset = {0, 5, -10},
    smoothing = 0.1,
    lookAhead = 2.0
})
camera.stopFollowing()

-- Camera styles (convenience)
camera.setStyle("isometric", {angle = 45, distance = 15})
camera.setStyle("thirdPerson", {distance = 5, height = 2})
camera.setStyle("firstPerson", {offset = {0, 1.7, 0}})
camera.setStyle("topDown", {height = 20, angle = 90})

-- Projection
camera.setPerspective(fov, near, far)
camera.setOrthographic(width, height, near, far)

-- Screen effects
camera.shake(duration, intensity)
camera.shake(duration, {
    intensity = 0.5,
    frequency = 20,
    decay = true
})

-- Coordinate conversion
camera.worldToScreen(x, y, z)      -- Returns screenX, screenY
camera.screenToWorld(screenX, screenY, depth)
camera.screenToRay(screenX, screenY)  -- Returns origin, direction

-- Viewport
camera.getViewportSize()           -- Returns width, height
```

### 6. Level System (`level`)

```lua
-- Level loading
level.load(":levels:/world1/level01")
level.load(":levels:/world1/level01", {
    spawnPoint = "checkpoint2"
})

level.loadAsync(":levels:/world1/level02", function(success)
    if success then
        level.activate()
    end
end)

-- Level queries
level.current()                    -- Current level name
level.getSpawnPoint("default")     -- Returns transform
level.getSpawnPoints()             -- All spawn points
level.getMetadata()                -- Level metadata

-- Level events
level.onLoad(function(levelName)
    -- Level just loaded
end)

level.onActivate(function(levelName)
    -- Level is now active
end)

-- Level transitions
level.transition(":levels:/world1/level02", {
    fadeOut = 0.5,
    fadeIn = 0.5,
    spawnPoint = "default"
})
```

### 7. UI System (`ui`)

```lua
-- Document management
local hud = ui.load(":ui:/hud.rml")
local menu = ui.load(":ui:/pause_menu.rml")

ui.show(hud)
ui.hide(menu)
ui.toggle(menu)
ui.isVisible(menu)

-- Element access (DOM-like)
local healthBar = ui.getElementById(hud, "health-bar")
local buttons = ui.getElementsByClass(hud, "menu-button")

-- Element properties
ui.setText(healthBar, "100")
ui.getText(healthBar)
ui.setVisible(healthBar, true)
ui.setClass(healthBar, "critical", health < 20)
ui.addClass(healthBar, "flashing")
ui.removeClass(healthBar, "flashing")
ui.hasClass(healthBar, "critical")
ui.setAttribute(healthBar, "data-value", tostring(health))
ui.setStyle(healthBar, "width", (health / maxHealth * 100) .. "%")

-- Data binding (automatic updates)
ui.bind("playerHealth", function() return player.data.health.current end)
ui.bind("playerScore", function() return app.score end)
ui.bind("enemyCount", function() return query.count("enemy") end)

-- Event handling
ui.onClick(button, function(element, event)
    audio.play(":assets:/sounds/sfx/click.wav")
    startGame()
end)

ui.onHover(button, function(element, entering)
    if entering then
        ui.addClass(element, "hovered")
    else
        ui.removeClass(element, "hovered")
    end
end)

-- Dynamic UI
local newElement = ui.createElement(hud, "div")
ui.setClass(newElement, "notification")
ui.setText(newElement, "Achievement Unlocked!")
ui.appendChild(container, newElement)

after(3.0, function()
    ui.remove(newElement)
end)

-- Input handling
ui.wantsKeyboard()                 -- UI wants keyboard focus
ui.wantsMouse()                    -- UI wants mouse
```

### 8. Animation System (`animation`)

```lua
-- Animator access
local animator = entity:getAnimator()

-- Playback
animation.play(animator, "run")
animation.play(animator, "run", {
    layer = 0,
    transitionTime = 0.2,
    speed = 1.5,
    loop = true
})

animation.stop(animator)
animation.stop(animator, {fadeOut = 0.3})
animation.pause(animator)
animation.resume(animator)

-- State queries
animation.isPlaying(animator, "run")
animation.getCurrentClip(animator)
animation.getTime(animator)
animation.getNormalizedTime(animator)  -- 0 to 1

-- Layer control
animation.setLayerWeight(animator, 1, 0.5)
animation.setLayerMask(animator, 1, {"spine", "arm_l", "arm_r"})

-- Blending
animation.crossfade(animator, "walk", "run", 0.3)
animation.blend(animator, "idle", "alert", blendFactor)

-- Events
animation.onEvent(animator, "footstep", function(boneName)
    audio.playAt(":assets:/sounds/sfx/footstep.wav",
        animation.getBonePosition(animator, boneName))
end)

animation.onComplete(animator, function(clipName)
    if clipName == "attack" then
        animation.play(animator, "idle")
    end
end)

-- IK
animation.setIKTarget(animator, "left_hand", targetPosition)
animation.setIKTarget(animator, "look_at", targetEntity:get("Transform3D").position)
animation.setIKWeight(animator, "left_hand", 0.8)

-- Sockets
local swordTransform = animation.getSocketTransform(animator, "weapon_socket")
local muzzlePos = animation.getSocketPosition(animator, "muzzle")
```

### 9. Save System (`save`)

```lua
-- Simple save/load
save.set("highScore", app.score)
save.set("unlockedLevels", app.unlockedLevels)
local highScore = save.get("highScore", 0)  -- Default 0

-- Save slots
save.toSlot(1, "My Save Game")
save.loadSlot(1)
save.deleteSlot(1)
save.getSlotInfo(1)                -- Returns metadata

-- Quick save/load
save.quick()
save.loadQuick()

-- Auto-save
save.enableAutoSave(300)           -- Every 5 minutes
save.disableAutoSave()

-- Save existence
save.exists(1)
save.getSlots()                    -- All used slots

-- Profiles
save.setProfile("player1")
save.getProfiles()
```

### 10. Events System (`events`)

```lua
-- Global events
events.emit("game_over", {score = app.score})
events.emit("level_complete", {levelName = level.current()})

-- Subscribe
local subId = events.on("game_over", function(data)
    print("Game over! Score: " .. data.score)
end)

-- Unsubscribe
events.off(subId)
events.off("game_over")            -- Remove all handlers

-- One-shot
events.once("game_start", function()
    audio.playMusic(":assets:/sounds/music/game.ogg")
end)

-- Deferred events (next frame)
events.defer("cleanup", {entity = enemy})
```

### 11. Timer Utilities (`timer`, `after`, `every`)

```lua
-- One-shot timer
after(1.0, function()
    print("One second passed!")
end)

-- Repeating timer
local timerId = every(0.5, function()
    spawnEnemy()
    return true                    -- Continue, false to stop
end)

-- Cancel timer
timer.cancel(timerId)

-- Next frame
nextFrame(function()
    -- Runs next frame
end)

-- Time utilities
time.dt                            -- Delta time
time.total                         -- Total elapsed
time.scale                         -- Time scale (1.0 = normal)
time.setScale(0.5)                 -- Slow motion
time.pause()                       -- Stop time (scale = 0)
time.resume()                      -- Resume time (scale = 1)
```

### 12. Debug Utilities (`debug`)

```lua
-- Logging
debug.log("Player spawned at", x, y)
debug.warn("Low health!")
debug.error("Failed to load level")

-- Draw (persists for duration)
debug.drawLine(start, end_, color, duration)
debug.drawBox(center, size, color, duration)
debug.drawSphere(center, radius, color, duration)
debug.drawRay(origin, direction, length, color, duration)
debug.drawText(position, text, color, duration)

-- Immediate draw (one frame)
debug.line(start, end_, color)
debug.box(center, size, color)
debug.sphere(center, radius, color)

-- Performance
debug.beginProfile("AI Update")
-- ... code ...
debug.endProfile("AI Update")

-- Console
debug.exec("spawn(':blueprints:/enemy', 0, 0, 0)")
```

---

## Path Scheme Reference

| Scheme | Resolves To | Example |
|--------|-------------|---------|
| `:assets:/` | Game assets directory | `:assets:/textures/player.png` |
| `:library:/` | Engine library directory | `:library:/shaders/pbr.frag` |
| `:blueprints:/` | `{app}/blueprints/` | `:blueprints:/player` |
| `:behaviors:/` | `{app}/behaviors/` | `:behaviors:/player/movement` |
| `:systems:/` | `{app}/systems/` | `:systems:/combat` |
| `:levels:/` | `{app}/levels/` | `:levels:/world1/level01` |
| `:materials:/` | `{app}/materials/` | `:materials:/toon` |
| `:config:/` | `{app}/config/` | `:config:/settings` |
| `:ui:/` | `{app}/ui/` | `:ui:/hud` |

All schemes auto-append `.lua` if no extension is provided.

---

## C++ Infrastructure: The Complete Picture

### What Game Developers Write (C++)

```cpp
// main.cpp - This is ALL the C++ needed
#include <bestow/engine.hpp>

int main(int argc, char* argv[]) {
    bestow::Engine engine;
    engine.initialize(argc, argv);
    engine.useDefaults();
    return engine.run();  // Finds app.lua automatically
}
```

**Total: ~6 lines of C++**

### What Engine Developers Write (C++)

All the systems listed above. Game developers never touch this.

---

## Implementation Breakdown

### Phase 1: Core Runtime

**Goal:** Basic Lua execution with hot reload

**Tasks:**

1. **ILuaRuntime Interface**
   - Create `bestow-contract/src/bestow.lua.cppm`
   - Define interface for script execution
   - Define entity wrapper interface
   - Define behavior/system interfaces

2. **LuaRuntime Implementation**
   - Create `bestow-lua/` module
   - Implement sol2 state management
   - Implement sandboxed script execution
   - Implement `app.lua` loading and lifecycle

3. **Path Scheme Extensions**
   - Extend PathResolver with new schemes
   - Add auto `.lua` extension
   - Add application root discovery
   - Implement scheme registration

4. **Hot Reload Infrastructure**
   - Add `AssetType::Script`, `AssetType::Blueprint`, etc.
   - Subscribe to file changes
   - Implement script reloading
   - Implement state preservation

### Phase 2: Entity Integration

**Goal:** Entities fully controllable from Lua

**Tasks:**

1. **Entity Wrapper**
   - Wrap Entity for Lua access
   - Expose component get/set
   - Implement data table attachment
   - Implement tag system

2. **`spawn()` Function**
   - Load blueprint from scheme path
   - Create entity with components
   - Attach behaviors
   - Apply overrides

3. **Query API**
   - Implement `query.withTag()`
   - Implement `query.with()`
   - Implement spatial queries
   - Implement iteration helpers

4. **Behavior System**
   - Behavior loading and caching
   - Behavior attachment to entities
   - Update dispatch loop
   - State preservation for hot reload

### Phase 3: System Bindings

**Goal:** All C++ systems accessible from Lua

**Tasks:**

1. **Input Bindings**
   - Bind IInputSystem methods
   - Create `input` namespace
   - Implement action queries
   - Implement raw input access

2. **Audio Bindings**
   - Bind IAudioSystem methods
   - Create `audio` namespace
   - Implement fire-and-forget sounds
   - Implement positional audio

3. **Physics Bindings**
   - Bind IPhysicsSystem methods (2D)
   - Bind IPhysics3DSystem methods (3D)
   - Create `physics` namespace
   - Implement raycasting

4. **Graphics Bindings**
   - Bind camera control
   - Create `camera` namespace
   - Implement screen effects
   - Implement coordinate conversion

5. **UI Bindings**
   - Bind IUISystem methods
   - Create `ui` namespace
   - Implement DOM-like access
   - Implement data binding

6. **Animation Bindings**
   - Bind IAnimationSystem methods
   - Create `animation` namespace
   - Implement playback control
   - Implement IK access

7. **Level Bindings**
   - Bind ILevelSystem methods
   - Create `level` namespace
   - Implement loading/transitions
   - Implement spawn point access

8. **Save Bindings**
   - Bind ISaveSystem methods
   - Create `save` namespace
   - Implement slot management
   - Implement auto-save

### Phase 4: Lua Systems

**Goal:** Game update loops in Lua

**Tasks:**

1. **System Loading**
   - Load systems from `:systems:/`
   - Parse system definition format
   - Manage system lifecycle

2. **System Execution**
   - Priority ordering
   - Update dispatch
   - Event subscriptions

3. **System Hot Reload**
   - Preserve system state
   - Re-execute on file change
   - Re-establish subscriptions

### Phase 5: Example Migration

**Goal:** Migrate snake game to Lua

**Tasks:**

1. **Extract Blueprints**
   - `blueprints/snake_head.lua`
   - `blueprints/snake_segment.lua`
   - `blueprints/food.lua`
   - `blueprints/enemy.lua`
   - `blueprints/wall.lua`

2. **Extract Behaviors**
   - `behaviors/snake/movement.lua`
   - `behaviors/snake/body.lua`
   - `behaviors/enemy/patrol.lua`
   - `behaviors/common/lifetime.lua`

3. **Extract Systems**
   - `systems/grid.lua`
   - `systems/combat.lua`
   - `systems/spawning.lua`
   - `systems/camera.lua`

4. **Create `app.lua`**
   - Window configuration
   - Input bindings
   - Asset preloading
   - Game initialization

---

## File Structure: Complete Example

```
my_game/
├── main.cpp                      # ~6 lines C++
├── app.lua                       # Application entry
├── config/
│   ├── settings.lua
│   ├── input.lua
│   └── audio.lua
├── blueprints/
│   ├── player.lua
│   ├── enemies/
│   │   ├── slime.lua
│   │   ├── skeleton.lua
│   │   └── boss.lua
│   ├── items/
│   │   ├── health_potion.lua
│   │   ├── mana_potion.lua
│   │   └── coin.lua
│   ├── projectiles/
│   │   ├── arrow.lua
│   │   └── fireball.lua
│   ├── effects/
│   │   ├── explosion.lua
│   │   ├── hit_spark.lua
│   │   └── death.lua
│   └── environment/
│       ├── wall.lua
│       ├── door.lua
│       └── chest.lua
├── behaviors/
│   ├── player/
│   │   ├── movement.lua
│   │   ├── combat.lua
│   │   ├── inventory.lua
│   │   └── abilities/
│   │       ├── dash.lua
│   │       ├── fireball.lua
│   │       └── shield.lua
│   ├── enemies/
│   │   ├── patrol.lua
│   │   ├── chase.lua
│   │   ├── attack.lua
│   │   └── flee.lua
│   └── common/
│       ├── health.lua
│       ├── damage.lua
│       ├── knockback.lua
│       ├── lifetime.lua
│       └── collectible.lua
├── systems/
│   ├── input.lua
│   ├── combat.lua
│   ├── spawning.lua
│   ├── camera.lua
│   ├── inventory.lua
│   ├── dialogue.lua
│   └── progression.lua
├── levels/
│   ├── world1/
│   │   ├── world.lua
│   │   ├── level01.lua
│   │   ├── level02.lua
│   │   └── boss.lua
│   └── world2/
│       └── ...
├── materials/
│   ├── player.lua
│   ├── enemy.lua
│   └── environment.lua
├── ui/
│   ├── hud.lua
│   ├── inventory.lua
│   ├── dialogue.lua
│   ├── pause_menu.lua
│   ├── main_menu.lua
│   └── game_over.lua
└── assets/
    ├── textures/
    ├── models/
    ├── sounds/
    │   ├── sfx/
    │   └── music/
    └── fonts/
```

---

## Hot Reload Behavior Summary

| File Type | Hot Reload Behavior | State Preserved |
|-----------|--------------------|--------------------|
| `behaviors/*.lua` | Re-execute, swap functions | ✅ `state` table |
| `systems/*.lua` | Re-execute, re-init | ✅ `state` table |
| `blueprints/*.lua` | Reload template | N/A (affects new spawns) |
| `levels/*.lua` | Reload definition | N/A (optionally respawn) |
| `config/*.lua` | Re-execute, apply | N/A |
| `materials/*.lua` | Reload material | ✅ GPU resources |
| `ui/*.lua` | Reload UI | Partial |
| `app.lua` | Reload config | Partial |
| `*.frag/*.vert` | Recompile shader | ✅ GPU resources |

---

## Error Handling

```lua
-- Lua errors display in dev overlay:
[LuaRuntime] Error in behaviors/player/dash.lua:42
  attempt to index nil value 'cooldown'

Stack trace:
  behaviors/player/dash.lua:42 in update()
  [LuaRuntime] updateBehaviors()

Entity: Player (id: 42)
Behavior: Dash

Previous version remains active.
```

**Errors don't crash the game. The previous working version continues running.**

---

## Summary

| Aspect | Implementation |
|--------|----------------|
| Entry point | `app.lua` (auto-discovered) |
| Path schemes | 8 new schemes (`:blueprints:/`, etc.) |
| Systems bound | 18 systems with Lua APIs |
| API functions | ~200+ Lua functions |
| Game C++ | ~6 lines |
| Hot reload | Everything except C++ engine |

**The engine is C++. The game is Lua. There is no in-between.**
