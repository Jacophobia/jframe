---
name: architecture
description: Best practices for architecting Bestow game projects. Use ALWAYS when starting a new game, planning project structure, or making architectural decisions. This is the most important skill for writing correct, idiomatic Bestow code.
---

# Bestow Architecture Best Practices

This document defines **how to architect a Bestow game correctly**. Every pattern here exists because the alternative causes real bugs—crashes, stale references, broken hot reload, or silent failures. Read this before writing any code.

## The Golden Rules

These are non-negotiable. Violating any of them produces broken code.

### 1. Never Cache `app.*` at File Scope

```lua
-- BROKEN: Reference goes stale on hot reload
local player = app.entities.player
local state = app.main.state

return {
    update = function(dt)
        player.update(dt)      -- STALE after any file reload
        state.score = state.score + 1  -- May point to old state table
    end
}

-- CORRECT: Fresh reference every call
return {
    update = function(dt)
        local player = app.entities.player  -- Always current
        local state = app.main.state        -- Always current
        player.update(dt)
        state.score = state.score + 1
    end
}
```

**Why:** When a file reloads, the engine replaces `app.systems.foo` with a new table. Any local variable captured at file scope still points to the OLD table.

### 2. All Mutable State Lives in `app.main.state`

```lua
-- BROKEN: Lost on hot reload
local score = 0
local enemies = {}

-- CORRECT: Survives hot reload
-- In init():
app.main.state = {
    score = 0,
    enemies = {},
    player = nil,
    currentLevel = nil,
    running = true
}
```

File-scope **constants** are fine (`local GRAVITY = -9.81`). Anything that changes must be in `app.main.state`.

### 3. Use Table+Method for All Callbacks

```lua
-- BROKEN: Closure captures stale reference
bestow.events.subscribe("damage", {}, function(event)
    local self = app.systems.combat  -- Looks safe but the CLOSURE itself is stale
    self.onDamage(event)
end)

-- CORRECT: Table+method pattern (engine resolves at call time)
bestow.events.subscribe("damage", {}, app.systems.combat, "onDamage")
```

This applies to: `bestow.events.subscribe()`, `bestow.timer.after()`, `bestow.timer.every()`, and any engine API that takes a callback.

### 4. Action Builder for Input (Not Direct Polling)

```lua
-- DISCOURAGED: Direct polling bypasses phases, rebinding, multi-device
if bestow.input.isKeyDown(KeyCode.W) then ... end

-- CORRECT: Action Builder with Dvorak+QWERTY support
local k = KeyCode
bestow.action.builder():duringPhase("gameplay"):whenActive(k.Comma):emitAction("MoveForward"):continuously()
bestow.action.builder():duringPhase("gameplay"):whenActive(k.W):emitAction("MoveForward"):continuously()

-- Then in update:
if bestow.input.isActionActive("MoveForward") then ... end
```

### 5. Return a Table, Not Execute Code

Every Lua file returns a table. The engine calls functions on the table. Files never execute logic at the top level.

```lua
-- BROKEN: Runs on load, can't hot reload, runs twice on reload
bestow.entity.create()
app.main.state.player = createPlayer()

-- CORRECT: Return a table, engine calls your functions
return {
    create = function(position)
        local entity = bestow.entity.create()
        -- ...
        return entity
    end
}
```

---

## Project Architecture

### The Two Namespaces

| Namespace | What It Is | Hot Reload | Who Writes It |
|-----------|-----------|------------|---------------|
| `bestow.*` | C++ engine API | No | Engine |
| `app.*` | Your game scripts | Yes | You |

**`bestow.*`** is the engine. It provides graphics, physics, audio, input, timers, events, entities, UI, and assets. You call it; you never modify it.

**`app.*`** is your game. The engine auto-discovers files in `entities/`, `systems/`, `levels/`, `data/` and maps them to `app.*` tables. When you save a file, the engine replaces the old table with the new one.

### File → Namespace Mapping

```
entities/player.lua           → app.entities.player
entities/enemies/goblin.lua   → app.entities.enemies.goblin
systems/movement.lua          → app.systems.movement
systems/combat.lua            → app.systems.combat
levels/forest.lua             → app.levels.forest
data/items.lua                → app.data.items
main.lua                      → app.main
```

The path IS the namespace. No registration, no imports, no `require()`.

### The Four Module Types

#### 1. Entity Blueprints (`entities/`)

Pure factories. Create an entity, add components, return it. **No update logic.**

```lua
-- entities/player.lua
return {
    create = function(position)
        local entity = bestow.entity.create()

        bestow.entity.addComponent(entity, "Transform3D", {
            position = position or Vec3.new(0, 1, 0),
            rotation = Quat.identity(),
            scale = Vec3.one()
        })
        bestow.entity.addComponent(entity, "MeshRenderer", {
            mesh = "meshes/player.obj",
            material = "materials/player"
        })
        bestow.entity.addComponent(entity, "Health", { current = 100, max = 100 })
        bestow.entity.addComponent(entity, "PlayerTag", {})

        bestow.physics3d.createBody(entity, {
            type = "Dynamic",
            shapeType = "Capsule",
            radius = 0.4,
            height = 1.8,
            mass = 70.0
        })

        return entity
    end,

    -- Variants use the same pattern with different defaults
    createWithWeapon = function(position, weaponType)
        local entity = app.entities.player.create(position)
        bestow.entity.addComponent(entity, "Weapon", {
            type = weaponType,
            damage = app.data.weapons[weaponType].damage
        })
        return entity
    end
}
```

**Rules for entities:**
- Return a table with `create()` (and optional variants)
- `create()` takes parameters for position, configuration, etc.
- `create()` returns the entity handle
- No `update()`, no `init()`, no subscription to events
- Use `app.data.*` for stats/configuration data
- Access other entity blueprints through `app.entities.*` inside functions

#### 2. Systems (`systems/`)

Game logic. Systems have `init()`, `update()`, `shutdown()` and contain behavior.

```lua
-- systems/movement.lua
local MOVE_SPEED = 8.0
local JUMP_FORCE = 5.0

return {
    -- Subscription IDs for cleanup
    jumpSubId = nil,

    init = function()
        local self = app.systems.movement

        -- Register input actions (Dvorak + QWERTY)
        local k = KeyCode
        for _, key in ipairs({k.Comma, k.W}) do
            bestow.action.builder():duringPhase("gameplay"):whenActive(key):emitAction("MoveForward"):continuously()
        end
        for _, key in ipairs({k.O, k.S}) do
            bestow.action.builder():duringPhase("gameplay"):whenActive(key):emitAction("MoveBack"):continuously()
        end
        bestow.action.builder():duringPhase("gameplay"):whenActive(k.A):emitAction("MoveLeft"):continuously()
        for _, key in ipairs({k.E, k.D}) do
            bestow.action.builder():duringPhase("gameplay"):whenActive(key):emitAction("MoveRight"):continuously()
        end
        bestow.action.builder():duringPhase("gameplay"):whenPressed(k.Space):emitAction("Jump"):discretely()

        -- Push the gameplay input phase
        bestow.input.pushPhase("gameplay")
    end,

    update = function(dt)
        local self = app.systems.movement
        local state = app.main.state
        if not state.player or not bestow.entity.isValid(state.player) then return end

        -- Read actions
        local moveX, moveZ = 0, 0
        if bestow.input.isActionActive("MoveLeft") then moveX = moveX - 1 end
        if bestow.input.isActionActive("MoveRight") then moveX = moveX + 1 end
        if bestow.input.isActionActive("MoveForward") then moveZ = moveZ - 1 end
        if bestow.input.isActionActive("MoveBack") then moveZ = moveZ + 1 end

        local moveDir = Vec3.new(moveX, 0, moveZ)
        if moveDir:lengthSquared() > 1.0 then
            moveDir = moveDir:normalize()
        end

        -- Apply movement via character controller
        local velocity = moveDir * MOVE_SPEED
        local groundInfo = bestow.physics3d.getCharacterGroundInfo(state.player)
        local gravityY = groundInfo.isGrounded and -1 or -20 * dt

        bestow.physics3d.moveCharacter(
            state.player,
            Vec3.new(velocity.x, gravityY, velocity.z),
            dt
        )

        -- Jump
        if bestow.input.isActionActive("Jump") and groundInfo.isGrounded then
            bestow.physics3d.applyImpulse(state.player, Vec3.new(0, JUMP_FORCE, 0))
        end
    end,

    shutdown = function()
        local self = app.systems.movement
        -- Clean up subscriptions, timers, etc.
    end
}
```

**Rules for systems:**
- `local self = app.systems.NAME` at the top of every function
- `local state = app.main.state` to access game state
- Constants at file scope are fine
- Store subscription/timer IDs on `self` for cleanup
- Always check entity validity before operating on entities
- Never cache references to other systems at file scope

#### 3. Levels (`levels/`)

Declarative descriptions of what exists in a level, plus `load()`/`unload()` functions.

```lua
-- levels/forest.lua
return {
    name = "Forest Clearing",

    spawns = {
        player = Vec3.new(0, 1, 0),
        checkpoint1 = Vec3.new(50, 1, 0),
        exit = Vec3.new(100, 1, 0)
    },

    objects = {
        { type = "enemy", position = Vec3.new(20, 0, 5), params = { kind = "goblin" } },
        { type = "coin", position = Vec3.new(10, 1.5, 0) },
        { type = "coin", position = Vec3.new(12, 1.5, 0) },
    },

    load = function()
        local self = app.levels.forest
        local state = app.main.state

        -- Track level entities for cleanup
        state.levelEntities = {}

        -- Create terrain
        local ground = bestow.entity.create()
        bestow.entity.addComponent(ground, "Transform3D", {
            position = Vec3.zero(),
            rotation = Quat.identity(),
            scale = Vec3.new(200, 1, 100)
        })
        bestow.entity.addComponent(ground, "MeshRenderer", {
            mesh = "meshes/plane.obj",
            material = "materials/grass"
        })
        bestow.physics3d.createBody(ground, {
            type = "Static",
            shapeType = "Box",
            halfExtents = Vec3.new(100, 0.5, 50)
        })
        table.insert(state.levelEntities, ground)

        -- Spawn objects from declaration
        for _, obj in ipairs(self.objects) do
            local entity = app.main.spawnObject(obj)
            if entity then
                table.insert(state.levelEntities, entity)
            end
        end

        -- Spawn player
        state.player = app.entities.player.create(self.spawns.player)
        state.currentLevel = "forest"
    end,

    unload = function()
        local state = app.main.state
        if state.levelEntities then
            for _, entity in ipairs(state.levelEntities) do
                if bestow.entity.isValid(entity) then
                    bestow.entity.destroy(entity)
                end
            end
            state.levelEntities = {}
        end
    end
}
```

**Rules for levels:**
- Declarative data at the top (spawns, objects, lighting, fog)
- `load()` creates all entities and stores handles in `state.levelEntities`
- `unload()` destroys all tracked entities
- Always track entities for cleanup
- Always check `isValid()` before destroying

#### 4. Data (`data/`)

Pure data tables. No functions (except computed values or callbacks like `onUse`).

```lua
-- data/weapons.lua
return {
    sword = {
        name = "Iron Sword",
        damage = 25,
        attackSpeed = 1.0,
        range = 2.0
    },
    bow = {
        name = "Longbow",
        damage = 15,
        attackSpeed = 0.5,
        range = 30.0,
        projectile = "arrow"
    }
}

-- Access: app.data.weapons.sword.damage
```

---

## System Communication Patterns

### Event-Driven Communication (Preferred)

Systems talk to each other through events. This avoids tight coupling.

```lua
-- systems/combat.lua - EMITS damage events
return {
    dealDamage = function(target, amount, source)
        local self = app.systems.combat
        local health = bestow.entity.getComponent(target, "Health")
        if not health then return end

        health.current = math.max(0, health.current - amount)
        bestow.entity.setComponent(target, "Health", health)

        bestow.events.emit("entity_damaged", {
            target = target,
            source = source,
            amount = amount,
            remainingHealth = health.current
        })

        if health.current <= 0 then
            bestow.events.emit("entity_died", {
                entity = target,
                killer = source
            })
        end
    end
}

-- systems/score.lua - LISTENS for events
return {
    subId = nil,

    init = function()
        local self = app.systems.score
        if self.subId then bestow.events.unsubscribe(self.subId) end
        self.subId = bestow.events.subscribe("entity_died", {},
            app.systems.score, "onEntityDied")
    end,

    onEntityDied = function(event)
        local self = app.systems.score
        local state = app.main.state
        if event.killer == state.player then
            state.score = state.score + 100
        end
    end,

    shutdown = function()
        local self = app.systems.score
        if self.subId then bestow.events.unsubscribe(self.subId) end
    end
}
```

**Event naming conventions:**
- Past tense for things that happened: `entity_damaged`, `item_collected`, `level_loaded`
- Present tense for requests: `play_sound`, `spawn_effect`

### Direct System Calls (When Appropriate)

For tightly related systems where decoupling adds no value:

```lua
-- In update, calling another system directly is fine
app.systems.camera.update(dt)
app.systems.movement.update(dt)

-- Calling a specific function on another system is fine
app.systems.audio.playSFX("hit")
```

Use events when: multiple systems care about the same thing, or the sender shouldn't know about the receiver.

Use direct calls when: one specific system needs one specific thing from one other system.

---

## Initialization Order

### main.lua init() Sequence

The order matters. Follow this sequence:

```lua
init = function()
    -- 1. Initialize state (FIRST - everything else depends on this)
    app.main.state = {
        player = nil,
        score = 0,
        lives = 3,
        running = true,
        currentLevel = nil,
        levelEntities = {}
    }

    -- 2. Register input actions (before any system that reads input)
    app.systems.movement.init()  -- Registers Action Builder bindings

    -- 3. Set up rendering environment
    bestow.graphics3d.setCamera({
        position = Vec3.new(0, 10, -20),
        rotation = Quat.lookAt(Vec3.new(0, -0.4, 1):normalize(), Vec3.up()),
        fov = 45.0,
        near = 0.1,
        far = 1000.0
    })
    bestow.graphics3d.setAmbientLight(Color.new(0.2, 0.2, 0.3, 1.0), 0.3)
    bestow.graphics3d.setDirectionalLight({
        direction = Vec3.new(-0.5, -1, -0.5):normalize(),
        color = Color.new(1.0, 0.95, 0.8, 1.0),
        intensity = 1.0
    })

    -- 4. Initialize systems that subscribe to events
    app.systems.combat.init()
    app.systems.score.init()
    app.systems.collectibles.init()

    -- 5. Initialize UI
    bestow.ui.initialize()
    app.systems.hud.init()

    -- 6. Load first level (LAST - creates entities that systems operate on)
    bestow.input.pushPhase("gameplay")
    app.levels.level1.load()
end
```

### main.lua update() Sequence

```lua
update = function(dt)
    local state = app.main.state

    -- 1. Update timers (drives delayed callbacks)
    bestow.timer.update(dt)

    -- 2. Process UI input (consumes input events before gameplay)
    bestow.ui.processInput()

    -- 3. Update game systems (order matters for dependencies)
    app.systems.movement.update(dt)     -- Player moves
    app.systems.enemy_ai.update(dt)     -- Enemies react to player position
    app.systems.combat.update(dt)       -- Resolve interactions
    app.systems.collectibles.update(dt) -- Check pickups
    app.systems.camera.update(dt)       -- Camera follows player (AFTER movement)

    return state.running
end
```

### main.lua render() Sequence

```lua
render = function()
    bestow.graphics3d.beginFrame()
    -- Entities with MeshRenderer are drawn automatically by the engine
    -- Only add manual draw calls for debug visualization
    bestow.ui.render()
    bestow.graphics3d.endFrame()
end
```

### main.lua shutdown() Sequence

```lua
shutdown = function()
    -- Reverse order of init
    app.systems.hud.shutdown()
    bestow.ui.shutdown()
    app.systems.collectibles.shutdown()
    app.systems.score.shutdown()
    app.systems.combat.shutdown()
end
```

---

## Cleanup Patterns

### Track Everything You Create

Every entity, subscription, and timer must be cleaned up.

```lua
return {
    -- Store IDs for cleanup
    subIds = {},
    timerIds = {},

    init = function()
        local self = app.systems.spawner

        -- Clean up from previous hot reload
        self.cleanupSubscriptions()

        self.subIds.spawn = bestow.events.subscribe("wave_start", {},
            app.systems.spawner, "onWaveStart")

        self.timerIds.spawnLoop = bestow.timer.every(2.0,
            app.systems.spawner, "spawnEnemy")
    end,

    cleanupSubscriptions = function()
        local self = app.systems.spawner
        for _, id in pairs(self.subIds) do
            bestow.events.unsubscribe(id)
        end
        self.subIds = {}
        for _, id in pairs(self.timerIds) do
            bestow.timer.cancel(id)
        end
        self.timerIds = {}
    end,

    shutdown = function()
        local self = app.systems.spawner
        self.cleanupSubscriptions()
    end
}
```

### Level Entity Tracking

```lua
-- Always track entities created during level load
state.levelEntities = state.levelEntities or {}

local entity = bestow.entity.create()
table.insert(state.levelEntities, entity)

-- On unload, destroy all tracked entities
for _, entity in ipairs(state.levelEntities) do
    if bestow.entity.isValid(entity) then
        bestow.entity.destroy(entity)
    end
end
state.levelEntities = {}
```

### Validate Before Operate

```lua
-- Always check entity validity before using it
local state = app.main.state
if state.player and bestow.entity.isValid(state.player) then
    local pos = bestow.entity.getField(state.player, "Transform3D", "position")
    -- ...
end
```

---

## Common Architectural Patterns

### Spawn Helper in main.lua

```lua
-- main.lua
return {
    -- ...

    spawnObject = function(obj)
        if obj.type == "enemy" then
            return app.entities.enemies[obj.params.kind].create(obj.position)
        elseif obj.type == "coin" then
            return app.entities.collectibles.coin.create(obj.position)
        elseif obj.type == "powerup" then
            return app.entities.collectibles.powerup.create(obj.position, obj.params)
        end
        return nil
    end,

    loadLevel = function(levelName)
        local state = app.main.state
        -- Unload current level
        if state.currentLevel then
            app.levels[state.currentLevel].unload()
        end
        -- Load new level
        app.levels[levelName].load()
    end
}
```

### Component Query Pattern

```lua
-- Iterate entities with specific components
bestow.entity.each(function(entity)
    if not bestow.entity.hasComponent(entity, "Enemy") then return end
    if not bestow.entity.hasComponent(entity, "Health") then return end

    local enemy = bestow.entity.getComponent(entity, "Enemy")
    local health = bestow.entity.getComponent(entity, "Health")
    local pos = bestow.entity.getField(entity, "Transform3D", "position")

    -- Process...

    -- Write back modified components
    bestow.entity.setComponent(entity, "Enemy", enemy)
    bestow.entity.setComponent(entity, "Health", health)
end)
```

### Camera Follow Pattern

```lua
-- systems/camera.lua
local FOLLOW_DISTANCE = 15
local FOLLOW_HEIGHT = 10
local SMOOTH_SPEED = 5.0

return {
    update = function(dt)
        local self = app.systems.camera
        local state = app.main.state
        if not state.player or not bestow.entity.isValid(state.player) then return end

        local playerPos = bestow.entity.getField(state.player, "Transform3D", "position")
        local targetPos = playerPos + Vec3.new(0, FOLLOW_HEIGHT, -FOLLOW_DISTANCE)

        -- Smooth follow
        local currentCam = bestow.graphics3d.getCamera()
        local newPos = Vec3.lerp(currentCam.position, targetPos, SMOOTH_SPEED * dt)
        local lookDir = (playerPos - newPos):normalize()

        bestow.graphics3d.setCamera({
            position = newPos,
            rotation = Quat.lookAt(lookDir, Vec3.up()),
            fov = 45.0,
            near = 0.1,
            far = 1000.0
        })
    end
}
```

### Game State Machine

```lua
-- main.lua with phase-based state management
return {
    init = function()
        app.main.state = {
            phase = "menu",  -- "menu", "playing", "paused", "game_over"
            score = 0,
            player = nil,
            running = true
        }
    end,

    update = function(dt)
        local state = app.main.state
        bestow.timer.update(dt)

        if state.phase == "menu" then
            return app.main.updateMenu(dt)
        elseif state.phase == "playing" then
            return app.main.updatePlaying(dt)
        elseif state.phase == "paused" then
            return app.main.updatePaused(dt)
        elseif state.phase == "game_over" then
            return app.main.updateGameOver(dt)
        end

        return state.running
    end,

    updateMenu = function(dt)
        if bestow.input.wasKeyJustPressed(KeyCode.Enter) then
            app.main.state.phase = "playing"
            app.levels.level1.load()
        end
        if bestow.input.wasKeyJustPressed(KeyCode.Escape) then
            return false  -- Quit
        end
        return true
    end,

    updatePlaying = function(dt)
        bestow.ui.processInput()
        app.systems.movement.update(dt)
        app.systems.enemy_ai.update(dt)
        app.systems.combat.update(dt)
        app.systems.camera.update(dt)
        return app.main.state.running
    end,

    -- ...
}
```

---

## Anti-Patterns (Do NOT Do These)

### 1. God System

```lua
-- WRONG: One massive system that does everything
return {
    update = function(dt)
        -- 500 lines of movement, combat, AI, UI, camera, audio...
    end
}

-- CORRECT: Separate systems with single responsibilities
-- systems/movement.lua   → handles player/entity movement
-- systems/combat.lua     → handles damage, health, death
-- systems/enemy_ai.lua   → handles AI behavior
-- systems/camera.lua     → handles camera follow
```

### 2. Entity with Logic

```lua
-- WRONG: Entity blueprint with update logic
return {
    create = function(pos)
        local entity = bestow.entity.create()
        -- ...
        return entity
    end,

    update = function(entity, dt)  -- DON'T put logic in entity files
        -- ...
    end
}

-- CORRECT: Entity creates, system updates
-- entities/enemy.lua → create()
-- systems/enemy_ai.lua → update(dt) iterates all enemies
```

### 3. Require/Import

```lua
-- WRONG: Lua require is disabled
local utils = require("utils")
local json = require("json")

-- CORRECT: Use app.* namespace (auto-discovered)
local utils = app.data.utils    -- if you have data/utils.lua
local items = app.data.items    -- if you have data/items.lua
```

### 4. Direct File I/O

```lua
-- WRONG: io is disabled
local f = io.open("save.dat", "w")
os.execute("something")

-- CORRECT: Use bestow.save for persistence
bestow.save.registerSaveable({ ... })
bestow.save.save(1)
```

### 5. Reimplementing Engine Features

```lua
-- WRONG: Writing your own timer
local timers = {}
local function addTimer(delay, callback)
    table.insert(timers, { remaining = delay, fn = callback })
end

-- CORRECT: Use bestow.timer
bestow.timer.after(delay, app.systems.game, "onTimer")
bestow.timer.every(interval, app.systems.spawner, "spawn")
```

### 6. Polling in Render

```lua
-- WRONG: Game logic in render function
render = function()
    bestow.graphics3d.beginFrame()
    if bestow.input.wasKeyJustPressed(KeyCode.Space) then  -- DON'T
        app.main.state.score = app.main.state.score + 1
    end
    bestow.graphics3d.endFrame()
end

-- CORRECT: All logic in update, render only draws
update = function(dt)
    if bestow.input.isActionActive("Jump") then
        -- game logic here
    end
end
```

### 7. Nested app.* Caching

```lua
-- WRONG: Even "safe-looking" caching breaks
return {
    update = function(dt)
        local systems = app.systems  -- Looks innocent...
        systems.movement.update(dt)  -- But if movement.lua reloaded,
                                     -- systems.movement is the OLD table
    end
}

-- CORRECT: Full path every time
return {
    update = function(dt)
        app.systems.movement.update(dt)  -- Always resolves fresh
    end
}
```

---

## Complete Project Template

Here is the minimal correct structure for a new Bestow game:

```
my-game/
├── main.lua                    # Entry point
├── entities/
│   └── player.lua              # Player factory
├── systems/
│   ├── movement.lua            # Player movement
│   └── camera.lua              # Camera follow
├── levels/
│   └── level1.lua              # First level
├── data/                       # (add as needed)
└── assets/
    ├── meshes/
    ├── textures/
    ├── sounds/
    └── materials/
```

### Minimal main.lua

```lua
return {
    title = "My Game",
    width = 1280,
    height = 720,

    init = function()
        app.main.state = {
            player = nil,
            running = true
        }

        -- Initialize systems
        app.systems.movement.init()

        -- Set up camera and lighting
        bestow.graphics3d.setCamera({
            position = Vec3.new(0, 10, -15),
            rotation = Quat.lookAt(Vec3.new(0, -0.5, 1):normalize(), Vec3.up()),
            fov = 45.0, near = 0.1, far = 500.0
        })
        bestow.graphics3d.setAmbientLight(Color.new(0.3, 0.3, 0.4, 1), 0.4)
        bestow.graphics3d.setDirectionalLight({
            direction = Vec3.new(-0.5, -1, -0.5):normalize(),
            color = Color.new(1, 0.95, 0.8, 1),
            intensity = 1.0
        })

        -- Load level
        bestow.input.pushPhase("gameplay")
        app.levels.level1.load()
    end,

    update = function(dt)
        bestow.timer.update(dt)
        app.systems.movement.update(dt)
        app.systems.camera.update(dt)
        return app.main.state.running
    end,

    render = function()
        bestow.graphics3d.beginFrame()
        bestow.graphics3d.endFrame()
    end,

    shutdown = function()
        app.systems.movement.shutdown()
    end
}
```

---

## Checklist: Before You Write Code

Use this checklist for every file you create:

- [ ] File returns a table (not bare code)
- [ ] No `app.*` cached at file scope
- [ ] All mutable state in `app.main.state`
- [ ] All callbacks use table+method pattern (no closures to engine APIs)
- [ ] Input uses Action Builder with Dvorak+QWERTY
- [ ] `local self = app.TYPE.NAME` at top of every function
- [ ] Entity validity checked before use (`bestow.entity.isValid()`)
- [ ] Subscriptions stored on `self` for cleanup
- [ ] `shutdown()` cleans up everything `init()` created
- [ ] No `require()`, `io.*`, `os.*`, `load()`, `dofile()`, `loadfile()`
- [ ] Constants at file scope, everything else inside functions
