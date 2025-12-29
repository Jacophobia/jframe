# Lua Runtime Architecture

> **Goal:** Game developers should be able to build entire games without ever rebuilding C++. The development experience should feel like React + Vite - instant hot reload, everything defined in scripts, C++ only for the core engine.

## Design Philosophy

### What Lives in C++ (The Engine)
- **Graphics Pipeline** - Vulkan/OpenGL rendering, shaders compilation
- **Physics Engine** - Box2D/Jolt integration, collision detection
- **Audio Engine** - FMOD integration, sound mixing
- **Asset System** - File loading, caching, hot reload infrastructure
- **ECS Core** - EnTT registry, entity management
- **Input Handling** - GLFW/SDL polling, device management
- **Core Components** - Transform2D/3D, Velocity, physics body data

### What Lives in Lua (The Game)
- **Entity Blueprints** - All entity templates
- **Behaviors** - All gameplay logic, abilities, AI
- **Systems** - Game-specific update loops
- **Components** - Game-specific data definitions
- **Levels** - World layouts, spawn points
- **Config** - All game settings
- **Events** - Game event handlers

## Architecture Overview

```
┌────────────────────────────────────────────────────────────────────┐
│                         GAME (100% Lua)                            │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────────┐ │
│  │  Blueprints  │  │   Behaviors  │  │      Lua Systems         │ │
│  │  Player.lua  │  │   dash.lua   │  │   combat_system.lua      │ │
│  │  Enemy.lua   │  │   jump.lua   │  │   inventory_system.lua   │ │
│  │  Bullet.lua  │  │ patrol_ai.lua│  │   dialogue_system.lua    │ │
│  └──────────────┘  └──────────────┘  └──────────────────────────┘ │
│                              ▲                                     │
│                              │ Hot Reload                          │
├──────────────────────────────┼─────────────────────────────────────┤
│                         LUA RUNTIME                                │
│  ┌─────────────────────────────────────────────────────────────┐  │
│  │  LuaRuntime (C++)                                            │  │
│  │  - Manages sol2 state                                        │  │
│  │  - Provides entity API                                       │  │
│  │  - Routes hot reload events                                  │  │
│  │  - Executes Lua systems each frame                           │  │
│  └─────────────────────────────────────────────────────────────┘  │
├────────────────────────────────────────────────────────────────────┤
│                      ENGINE (C++ Only)                             │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌─────────┐ │
│  │ Graphics │ │ Physics  │ │  Audio   │ │  Assets  │ │  Input  │ │
│  │ (Vulkan) │ │ (Box2D)  │ │ (FMOD)   │ │ (+efsw)  │ │ (GLFW)  │ │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘ └─────────┘ │
│                              ▲                                     │
│                              │                                     │
│  ┌─────────────────────────────────────────────────────────────┐  │
│  │  EnTT Registry (C++)                                         │  │
│  │  - Transform2D, Transform3D (C++ components)                 │  │
│  │  - Velocity, PhysicsBody (C++ components)                    │  │
│  │  - LuaEntity marker (C++ component)                          │  │
│  │  - LuaBehaviors attachment (C++ component)                   │  │
│  └─────────────────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────────────────┘
```

## File Structure

```
games/mygame/
├── app.lua                     # Application entry point
├── config/
│   ├── settings.lua           # Application settings
│   ├── input.lua              # Input bindings
│   └── audio.lua              # Audio settings
├── blueprints/
│   ├── player.lua             # Player entity template
│   ├── enemies/
│   │   ├── slime.lua
│   │   └── boss.lua
│   └── items/
│       ├── health_potion.lua
│       └── sword.lua
├── behaviors/
│   ├── player/
│   │   ├── movement.lua       # WASD/,AOE movement
│   │   ├── dash.lua           # Dash ability
│   │   └── attack.lua         # Attack ability
│   ├── ai/
│   │   ├── patrol.lua         # Patrol AI
│   │   ├── chase.lua          # Chase player AI
│   │   └── flee.lua           # Flee AI
│   └── common/
│       ├── health.lua         # Health management
│       └── lifetime.lua       # Auto-destroy after time
├── systems/
│   ├── combat.lua             # Combat resolution
│   ├── inventory.lua          # Item management
│   └── dialogue.lua           # NPC dialogue
├── levels/
│   ├── level01.lua
│   └── level02.lua
└── events/
    ├── on_damage.lua          # Damage event handler
    └── on_level_complete.lua  # Level completion handler
```

## Core Concepts

### 1. Blueprints (Entity Templates)

Blueprints define what an entity IS - its components and initial values.

```lua
-- blueprints/player.lua
return {
    name = "Player",

    -- C++ components (fast, engine-integrated)
    components = {
        Transform2D = { x = 0, y = 0 },
        Velocity = { x = 0, y = 0 },
        DebugRect = {
            width = 32,
            height = 32,
            fillColor = {100, 200, 255, 255}
        }
    },

    -- Physics body (optional)
    physics = {
        type = "dynamic",
        size = {32, 32},
        fixedRotation = true,
        density = 1.0,
        friction = 0.3
    },

    -- Lua-defined data (flexible, hot-reloadable)
    data = {
        health = { current = 100, max = 100 },
        mana = { current = 50, max = 50 },
        stats = {
            speed = 200,
            jumpForce = 400,
            dashSpeed = 600,
            dashCooldown = 1.0
        },
        inventory = {
            slots = 10,
            items = {}
        }
    },

    -- Behaviors attached to this entity
    behaviors = {
        "behaviors/player/movement.lua",
        "behaviors/player/dash.lua",
        "behaviors/player/attack.lua",
        "behaviors/common/health.lua"
    },

    -- Tags for querying
    tags = { "player", "friendly", "controllable" }
}
```

### 2. Behaviors (Entity Logic)

Behaviors define what an entity DOES - its update logic and event responses.

```lua
-- behaviors/player/dash.lua
return {
    name = "Dash",

    -- State that persists across frames
    -- These fields are preserved during hot reload
    state = {
        cooldownTimer = 0,
        isDashing = false,
        dashTimer = 0
    },

    -- Called when behavior is first attached
    init = function(self, entity)
        -- Access entity data
        self.dashSpeed = entity.data.stats.dashSpeed
        self.dashCooldown = entity.data.stats.dashCooldown
        self.dashDuration = 0.15
    end,

    -- Called every frame
    update = function(self, entity, dt)
        -- Update cooldown
        if self.state.cooldownTimer > 0 then
            self.state.cooldownTimer = self.state.cooldownTimer - dt
        end

        -- Handle active dash
        if self.state.isDashing then
            self.state.dashTimer = self.state.dashTimer - dt
            if self.state.dashTimer <= 0 then
                self.state.isDashing = false
                -- Restore normal velocity control
                entity:emit("dash_ended")
            end
        end
    end,

    -- Called when dash input is triggered
    onDashInput = function(self, entity, direction)
        if self.state.cooldownTimer <= 0 and not self.state.isDashing then
            -- Get velocity component (C++)
            local vel = entity:get("Velocity")

            -- Apply dash velocity
            vel.x = direction.x * self.dashSpeed
            vel.y = direction.y * self.dashSpeed

            -- Set state
            self.state.isDashing = true
            self.state.dashTimer = self.dashDuration
            self.state.cooldownTimer = self.dashCooldown

            -- Emit events for other systems
            entity:emit("dash_started", {
                direction = direction,
                speed = self.dashSpeed
            })

            -- Spawn trail particles
            spawn("DashTrail", entity:get("Transform2D"))
        end
    end,

    -- Called when entity takes damage
    onDamage = function(self, entity, amount, source)
        -- Can't take damage while dashing (i-frames)
        if self.state.isDashing then
            return false  -- Prevent damage
        end
        return true  -- Allow damage
    end
}
```

### 3. Lua Systems (Game Logic)

Systems process groups of entities with specific components/tags.

```lua
-- systems/combat.lua
return {
    name = "CombatSystem",

    -- System priority (lower = earlier)
    priority = 100,

    -- Called once when system is loaded
    init = function(self)
        self.damageQueue = {}

        -- Subscribe to collision events
        events.subscribe("collision", function(a, b, data)
            self:onCollision(a, b, data)
        end)
    end,

    -- Called every frame
    update = function(self, dt)
        -- Process queued damage
        for _, dmg in ipairs(self.damageQueue) do
            self:applyDamage(dmg.target, dmg.amount, dmg.source)
        end
        self.damageQueue = {}
    end,

    -- Handle collision between entities
    onCollision = function(self, entityA, entityB, data)
        -- Check if this is a damage-dealing collision
        local damagerData = entityA.data.damage
        local healthData = entityB.data.health

        if damagerData and healthData then
            table.insert(self.damageQueue, {
                target = entityB,
                amount = damagerData.amount,
                source = entityA
            })
        end
    end,

    -- Apply damage to an entity
    applyDamage = function(self, target, amount, source)
        local health = target.data.health
        if not health then return end

        -- Allow behaviors to modify/block damage
        local allowed = target:callBehaviors("onDamage", amount, source)
        if allowed == false then return end

        -- Apply damage
        health.current = math.max(0, health.current - amount)

        -- Emit damage event
        target:emit("damaged", { amount = amount, source = source })

        -- Check for death
        if health.current <= 0 then
            target:emit("died", { killer = source })

            -- Spawn death effect
            spawn("DeathEffect", target:get("Transform2D"))

            -- Destroy entity
            target:destroy()
        end
    end
}
```

### 4. Game Entry Point

```lua
-- main.lua
return {
    name = "My Awesome Game",
    version = "1.0.0",

    -- Window settings
    window = {
        width = 1920,
        height = 1080,
        title = "My Awesome Game",
        fullscreen = false
    },

    -- Asset preloading
    preload = {
        textures = {
            "textures/player.png",
            "textures/enemies/*.png",
            "textures/items/*.png"
        },
        sounds = {
            "sounds/sfx/*.wav",
            "sounds/music/*.ogg"
        }
    },

    -- Systems to load (in order)
    systems = {
        "systems/input.lua",
        "systems/combat.lua",
        "systems/inventory.lua",
        "systems/dialogue.lua"
    },

    -- Called when game starts
    init = function()
        -- Load first level
        loadLevel("levels/level01.lua")

        -- Spawn player at spawn point
        local spawnPoint = level.spawnPoints.default
        spawn("Player", spawnPoint.x, spawnPoint.y)

        -- Start background music
        audio.playMusic("sounds/music/level1.ogg")
    end,

    -- Called every frame
    update = function(dt)
        -- Global game logic here
    end,

    -- Called when game shuts down
    shutdown = function()
        -- Cleanup
    end
}
```

## Lua Entity API

The C++ LuaRuntime exposes these functions to Lua:

### Entity Functions

```lua
-- Spawn entity from blueprint
local entity = spawn("Player", x, y)
local entity = spawn("Player", x, y, {
    -- Override blueprint values
    ["data.health.current"] = 50,
    ["data.stats.speed"] = 300
})

-- Destroy entity
entity:destroy()

-- Check if entity is valid
if entity:isValid() then ... end

-- Get entity ID (for comparisons)
local id = entity:id()
```

### Component Access

```lua
-- Get C++ component (Transform2D, Velocity, etc.)
local transform = entity:get("Transform2D")
transform.x = 100
transform.y = 200

local vel = entity:get("Velocity")
vel.x = 50

-- Check if entity has component
if entity:has("Velocity") then ... end

-- Get Lua data (from blueprint data section)
local health = entity.data.health
health.current = health.current - 10
```

### Behavior Interaction

```lua
-- Call a method on all behaviors that have it
entity:callBehaviors("onDamage", amount, source)

-- Get a specific behavior
local dashBehavior = entity:getBehavior("Dash")
if dashBehavior then
    dashBehavior:activate(direction)
end

-- Check behavior state
local isDashing = entity:getBehavior("Dash").state.isDashing
```

### Events

```lua
-- Emit event from entity
entity:emit("damaged", { amount = 10, source = attacker })

-- Subscribe to entity events
entity:on("damaged", function(data)
    print("Took " .. data.amount .. " damage!")
end)

-- Global events
events.emit("level_complete", { levelId = 1 })
events.subscribe("level_complete", function(data)
    print("Completed level " .. data.levelId)
end)
```

### Queries

```lua
-- Find entities by tag
local enemies = query.withTag("enemy")
for _, enemy in ipairs(enemies) do
    enemy:destroy()
end

-- Find entities with components
local movingEntities = query.with("Transform2D", "Velocity")

-- Find single entity
local player = query.single("player")

-- Find entities in area
local nearby = query.inRadius(x, y, radius)
local inBox = query.inBox(minX, minY, maxX, maxY)
```

### Timers and Scheduling

```lua
-- One-shot timer
after(1.0, function()
    print("1 second passed!")
end)

-- Repeating timer
every(0.5, function()
    print("Every half second")
    return true  -- Continue, return false to stop
end)

-- Next frame
nextFrame(function()
    -- Runs next frame
end)
```

### Input

```lua
-- Check action state
if input.isPressed("jump") then
    -- Jump was just pressed this frame
end

if input.isHeld("move_right") then
    -- Move right is being held
end

-- Get axis value (-1 to 1)
local moveX = input.axis("move_horizontal")
local moveY = input.axis("move_vertical")

-- Get mouse position
local mx, my = input.mousePosition()
```

### Audio

```lua
-- Play sound effect
audio.play("sounds/sfx/jump.wav")
audio.play("sounds/sfx/jump.wav", { volume = 0.5, pitch = 1.2 })

-- Play at position (3D audio)
audio.playAt("sounds/sfx/explosion.wav", x, y, z)

-- Music control
audio.playMusic("sounds/music/boss.ogg", { fadeIn = 2.0 })
audio.stopMusic({ fadeOut = 1.0 })

-- Set listener position (for 3D audio)
audio.setListener(x, y, z, forwardX, forwardY, forwardZ)
```

### Physics

```lua
-- Apply force/impulse
physics.applyForce(entity, forceX, forceY)
physics.applyImpulse(entity, impulseX, impulseY)

-- Raycast
local hit = physics.raycast(startX, startY, endX, endY)
if hit then
    print("Hit entity at", hit.point.x, hit.point.y)
end

-- Set velocity directly
local vel = entity:get("Velocity")
vel.x = 100
vel.y = -50

-- Ground check
local grounded = physics.isGrounded(entity)
```

## Hot Reload System

### How It Works

1. **File Watcher (efsw)** detects changes to `.lua` files
2. **AssetSystem** receives notification, queues reload event
3. **LuaRuntime** receives callback on main thread
4. **Script Reload** re-executes the Lua file
5. **State Preservation** copies behavior state to new instances
6. **Binding Update** rebinds function references

### Hot Reload Flow

```
┌─────────────────────────────────────────────────────────────────┐
│                     File Change Detected                         │
│                    (behaviors/dash.lua)                          │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│  efsw::FileWatcher (background thread)                          │
│  → Queue FileChangeEvent to AssetSystem                         │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│  AssetSystem::update() (main thread)                            │
│  → processFileChanges()                                          │
│  → Reload Lua file                                               │
│  → notifySubscribers(handle, AssetType::Script)                  │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│  LuaRuntime::onScriptChanged(handle)                            │
│  1. Find all entities using this behavior                        │
│  2. For each entity:                                             │
│     a. Save behavior.state (deep copy)                           │
│     b. Create new behavior instance from reloaded script         │
│     c. Restore state to new instance                             │
│     d. Call behavior:onHotReload() if exists                     │
│  3. Log reload success/failure                                   │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│  Game continues with new behavior code                           │
│  All state preserved (cooldowns, timers, counters)               │
└─────────────────────────────────────────────────────────────────┘
```

### State Preservation

```lua
-- behaviors/dash.lua
return {
    name = "Dash",

    -- These fields survive hot reload
    state = {
        cooldownTimer = 0,
        isDashing = false,
        dashTimer = 0,
        totalDashes = 0  -- Preserved across reload!
    },

    -- Optional: called after hot reload
    onHotReload = function(self, entity)
        print("Dash behavior reloaded!")
        -- Re-cache any computed values
        self.dashSpeed = entity.data.stats.dashSpeed
    end,

    -- ... rest of behavior
}
```

### What Reloads Instantly

| File Type | Reload Behavior |
|-----------|-----------------|
| `behaviors/*.lua` | Re-execute, preserve state, rebind functions |
| `blueprints/*.lua` | Reload template, affects new spawns only |
| `systems/*.lua` | Re-execute init, preserve system state |
| `levels/*.lua` | Reload level data, optional level restart |
| `config/*.lua` | Re-execute, apply immediately |
| `main.lua` | Hot reload core game config |

### Error Handling

```lua
-- If a script has errors, the old version keeps running
-- Errors are displayed in dev console:

[LuaRuntime] Hot reload failed for behaviors/dash.lua:
  Line 15: attempt to index nil value 'state'

[LuaRuntime] Keeping previous version active
```

## C++ Implementation (LuaRuntime)

### Interface

```cpp
// bestow-contract/src/bestow.lua.cppm
export module bestow.lua;

import std;
import bestow.types;

export namespace bestow {

class ILuaRuntime {
public:
    virtual ~ILuaRuntime() = default;

    // Lifecycle
    virtual void init() = 0;
    virtual void update(float dt) = 0;
    virtual void shutdown() = 0;

    // Script loading
    virtual bool loadGame(std::string_view mainLuaPath) = 0;
    virtual bool loadSystem(std::string_view path) = 0;
    virtual bool loadBehavior(std::string_view path) = 0;

    // Entity management
    virtual Entity spawn(std::string_view blueprintName,
                        float x, float y,
                        const std::unordered_map<std::string, std::any>& overrides = {}) = 0;

    // Hot reload
    virtual void onScriptChanged(AssetHandle handle, AssetType type) = 0;

    // Debugging
    virtual void executeConsole(std::string_view luaCode) = 0;
    virtual std::string dumpEntity(Entity entity) = 0;
};

} // namespace bestow
```

### Key Components

```cpp
// LuaRuntime implementation components

struct LuaEntityData {
    sol::table data;                    // Lua data table
    std::vector<std::string> tags;      // Entity tags
    std::vector<LuaBehavior> behaviors; // Attached behaviors
};

struct LuaBehavior {
    std::string name;
    std::string scriptPath;
    sol::table instance;          // The behavior table instance
    sol::table state;             // State that survives hot reload
    sol::function initFn;         // Cached init function
    sol::function updateFn;       // Cached update function
    sol::function onHotReloadFn;  // Optional hot reload callback
    // ... other cached function refs
};

struct LuaSystem {
    std::string name;
    std::string scriptPath;
    int priority;
    sol::table instance;
    sol::table state;
    sol::function initFn;
    sol::function updateFn;
    sol::function shutdownFn;
};
```

### Entity Wrapper

```cpp
// Exposes entity to Lua with a nice API
class EntityWrapper {
public:
    EntityWrapper(Entity entity, LuaRuntime* runtime)
        : entity_(entity), runtime_(runtime) {}

    // Get C++ component
    sol::object get(const std::string& componentName);

    // Check component
    bool has(const std::string& componentName);

    // Access Lua data
    sol::table getData();

    // Behavior management
    sol::object getBehavior(const std::string& behaviorName);
    sol::object callBehaviors(const std::string& methodName,
                              sol::variadic_args args);

    // Events
    void emit(const std::string& eventName, sol::table data);
    void on(const std::string& eventName, sol::function callback);

    // Lifecycle
    void destroy();
    bool isValid();
    Entity id();

private:
    Entity entity_;
    LuaRuntime* runtime_;
};
```

## Asset Type for Lua Scripts

Add new asset type for Lua scripts:

```cpp
// In bestow.types.cppm
enum class AssetType : std::uint8_t {
    Texture,
    Sound,
    Music,
    Font,
    Level,
    Data,
    Shader,
    NavMesh,
    BehaviorTree,
    Mesh,
    Model,
    Material,
    Cubemap,
    Script  // NEW: Lua scripts (behaviors, systems, blueprints)
};
```

## Integration with Existing Systems

### BlueprintFactory Integration

The existing `BlueprintFactory` becomes a thin wrapper:

```cpp
// BlueprintFactory delegates to LuaRuntime
Entity BlueprintFactory::create(std::string_view name, float x, float y,
                                const PropertyMap& overrides) {
    return luaRuntime_->spawn(name, x, y, toOverrideMap(overrides));
}
```

### EntitySystem Integration

LuaRuntime manages the `LuaEntityData` component:

```cpp
// C++ component that links entity to Lua
struct LuaEntityMarker {
    // Index into LuaRuntime's entity data storage
    std::size_t luaDataIndex;
};

// In LuaRuntime::spawn()
Entity entity = entities_->createEntity();
entities_->emplace<LuaEntityMarker>(entity, dataIndex);
entities_->emplace<Transform2D>(entity, {x, y});
// ... other C++ components from blueprint
```

### AssetSystem Integration

```cpp
// LuaRuntime subscribes to script changes
void LuaRuntime::init() {
    scriptSubscriptionId_ = assets_->subscribeToType(
        AssetType::Script,
        [this](AssetHandle handle, AssetType type) {
            onScriptChanged(handle, type);
        }
    );
    assets_->enableHotReload(true);
}
```

## Performance Considerations

### Optimizations

1. **Cache Function References**
   - Don't look up Lua functions every frame
   - Cache in LuaBehavior struct

2. **Batch Lua Calls**
   - Update all behaviors in single Lua VM call when possible
   - Use coroutines for long-running behaviors

3. **C++ for Hot Paths**
   - Physics stays in C++
   - Rendering stays in C++
   - Only gameplay logic in Lua

4. **Lazy Evaluation**
   - Don't update off-screen entities
   - Use spatial partitioning for queries

### Benchmarks

Expected overhead per entity per frame:
- Simple behavior (movement): ~0.001ms
- Complex behavior (AI): ~0.01ms
- 1000 entities with simple behaviors: ~1ms

## Migration Path

### Phase 1: LuaRuntime Core
- [ ] Create `ILuaRuntime` interface
- [ ] Implement basic script loading
- [ ] Implement `spawn()` with blueprints
- [ ] Expose entity API to Lua

### Phase 2: Behaviors
- [ ] Implement behavior attachment
- [ ] Implement behavior update loop
- [ ] Add state preservation for hot reload
- [ ] Test hot reload with simple behavior

### Phase 3: Systems
- [ ] Implement Lua system loading
- [ ] Add system priority ordering
- [ ] Integrate with game loop
- [ ] Test system hot reload

### Phase 4: Integration
- [ ] Integrate with AssetSystem for file watching
- [ ] Add error handling and dev console
- [ ] Performance profiling
- [ ] Documentation and examples

## Example: Complete Player Implementation

```lua
-- blueprints/player.lua
return {
    name = "Player",

    components = {
        Transform2D = { x = 0, y = 0 },
        Velocity = { x = 0, y = 0 },
        DebugRect = {
            width = 32,
            height = 48,
            fillColor = {100, 200, 255, 255}
        }
    },

    physics = {
        type = "dynamic",
        size = {32, 48},
        fixedRotation = true
    },

    data = {
        health = { current = 100, max = 100 },
        stats = {
            speed = 200,
            jumpForce = 450,
            dashSpeed = 600
        }
    },

    behaviors = {
        "behaviors/player/movement.lua",
        "behaviors/player/jump.lua",
        "behaviors/player/dash.lua",
        "behaviors/common/health.lua"
    },

    tags = { "player" }
}
```

```lua
-- behaviors/player/movement.lua
return {
    name = "Movement",

    update = function(self, entity, dt)
        local vel = entity:get("Velocity")
        local speed = entity.data.stats.speed

        -- Dvorak-friendly: ,AOE instead of WASD
        local moveX = 0
        if input.isHeld("move_left") then moveX = -1 end
        if input.isHeld("move_right") then moveX = 1 end

        vel.x = moveX * speed

        -- Flip sprite based on direction
        if moveX ~= 0 then
            local transform = entity:get("Transform2D")
            transform.scaleX = moveX > 0 and 1 or -1
        end
    end
}
```

```lua
-- behaviors/player/jump.lua
return {
    name = "Jump",

    state = {
        isJumping = false,
        jumpHoldTime = 0,
        coyoteTime = 0
    },

    init = function(self, entity)
        self.jumpForce = entity.data.stats.jumpForce
        self.maxJumpHold = 0.2
        self.coyoteDuration = 0.1
    end,

    update = function(self, entity, dt)
        local vel = entity:get("Velocity")
        local grounded = physics.isGrounded(entity)

        -- Coyote time
        if grounded then
            self.state.coyoteTime = self.coyoteDuration
            self.state.isJumping = false
        else
            self.state.coyoteTime = self.state.coyoteTime - dt
        end

        -- Jump initiation
        if input.isPressed("jump") and self.state.coyoteTime > 0 then
            vel.y = -self.jumpForce
            self.state.isJumping = true
            self.state.jumpHoldTime = 0
            audio.play("sounds/sfx/jump.wav")
        end

        -- Variable jump height
        if self.state.isJumping and input.isHeld("jump") then
            self.state.jumpHoldTime = self.state.jumpHoldTime + dt
            if self.state.jumpHoldTime < self.maxJumpHold then
                vel.y = vel.y - self.jumpForce * dt * 3
            end
        end

        -- Cut jump short when button released
        if self.state.isJumping and input.isReleased("jump") then
            if vel.y < 0 then
                vel.y = vel.y * 0.5
            end
            self.state.isJumping = false
        end
    end
}
```

---

## Summary

This architecture provides:

1. **Zero rebuild development** - Change any Lua file, see changes instantly
2. **Clean separation** - Engine in C++, game in Lua
3. **Hot reload everything** - Behaviors, systems, blueprints, config
4. **State preservation** - Cooldowns, timers, etc. survive reload
5. **Familiar patterns** - Similar to React component model
6. **Performance where it matters** - Physics, rendering stay in C++
7. **Same asset system** - Follows existing shader hot reload pattern
