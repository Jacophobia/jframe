---
name: level-system
description: Load levels, manage spawn points, and handle level transitions in Bestow. Use when loading game levels, setting up spawn points, or transitioning between levels.
---

# Level System

The level system manages loading, unloading, and transitioning between game levels.

## Level Definition

Levels are defined in Lua files in the `levels/` directory:

```lua
-- levels/level1.lua
return {
    name = "The Beginning",
    description = "Tutorial level",

    -- Spawn points
    spawns = {
        player = Vec3.new(0, 1, 0),
        checkpoint1 = Vec3.new(50, 1, 0),
        checkpoint2 = Vec3.new(100, 1, 20)
    },

    -- Level bounds
    bounds = {
        minX = -10, maxX = 200,
        minY = -100, maxY = 100,
        minZ = -50, maxZ = 50
    },

    -- Camera bounds
    cameraBounds = {
        minX = 0, maxX = 190,
        minY = 5, maxY = 50,
        minZ = -40, maxZ = 40
    },

    -- Lighting
    lighting = {
        ambient = { color = Color.new(0.3, 0.3, 0.4, 1), intensity = 0.4 },
        sun = {
            direction = Vec3.new(-0.5, -1, -0.5),
            color = Color.new(1, 0.95, 0.8, 1),
            intensity = 1.0
        }
    },

    -- Fog
    fog = {
        enabled = true,
        color = Color.new(0.7, 0.8, 0.9, 1),
        startDistance = 50,
        endDistance = 200
    },

    -- Static geometry (loaded once)
    terrain = {
        mesh = "meshes/levels/level1_terrain.obj",
        material = "materials/terrain"
    },

    -- Dynamic objects
    objects = {
        { type = "enemy_spawner", position = Vec3.new(30, 0, 0), params = { enemy = "goblin", count = 3 } },
        { type = "checkpoint", position = Vec3.new(50, 0, 0), params = { id = "checkpoint1" } },
        { type = "collectible", position = Vec3.new(40, 2, 5), params = { item = "coin" } },
        { type = "door", position = Vec3.new(100, 0, 0), params = { to = "level2", spawn = "start" } }
    },

    -- Called when level loads
    load = function()
        local self = app.levels.level1
        local state = app.main.state

        -- Set up lighting
        bestow.graphics3d.setAmbientLight(
            self.lighting.ambient.color,
            self.lighting.ambient.intensity
        )
        bestow.graphics3d.setDirectionalLight(self.lighting.sun)

        -- Set up fog
        bestow.graphics3d.setFog(self.fog)

        -- Create terrain
        local terrain = bestow.entity.create()
        bestow.entity.addComponent(terrain, "Transform3D", {
            position = Vec3.new(0, 0, 0),
            rotation = Quat.identity(),
            scale = Vec3.new(1, 1, 1)
        })
        bestow.entity.addComponent(terrain, "MeshRenderer", {
            mesh = self.terrain.mesh,
            material = self.terrain.material
        })
        state.levelEntities = { terrain }

        -- Spawn objects
        for _, obj in ipairs(self.objects) do
            self.spawnObject(obj)
        end

        -- Store camera bounds for the camera system to enforce
        state.cameraBounds = self.cameraBounds

        -- Spawn player at designated point
        local spawnPoint = state.nextSpawnPoint or "player"
        local spawnPos = self.spawns[spawnPoint] or self.spawns.player
        state.player = app.entities.player.spawnAt(spawnPos)
        state.nextSpawnPoint = nil
    end,

    -- Spawn helper
    spawnObject = function(obj)
        local self = app.levels.level1
        local state = app.main.state

        local spawner = app.spawners[obj.type]
        if spawner then
            local entity = spawner.spawn(obj.position, obj.params)
            table.insert(state.levelEntities, entity)
        else
            print("Unknown object type: " .. obj.type)
        end
    end,

    -- Called when level unloads
    unload = function()
        local state = app.main.state

        -- Destroy all level entities
        if state.levelEntities then
            for _, entity in ipairs(state.levelEntities) do
                if bestow.entity.isValid(entity) then
                    bestow.entity.destroy(entity)
                end
            end
            state.levelEntities = {}
        end

        -- Clear camera bounds
        state.cameraBounds = nil
    end
}
```

## Loading Levels

### Basic Level Loading

```lua
-- In main.lua init()
init = function()
    app.main.state = {
        currentLevel = nil,
        levelEntities = {}
    }

    -- Load first level
    app.main.loadLevel("level1")
end

-- Level loading function
loadLevel = function(levelName)
    local state = app.main.state

    -- Unload current level
    if state.currentLevel then
        local oldLevel = app.levels[state.currentLevel]
        if oldLevel.unload then
            oldLevel.unload()
        end
    end

    -- Load new level
    state.currentLevel = levelName
    local level = app.levels[levelName]
    if level.load then
        level.load()
    end

    -- Play level music
    if level.music then
        app.systems.audio.playMusic(level.music)
    end
end
```

### Level Transitions

```lua
-- systems/transitions.lua
return {
    transitionTo = function(levelName, spawnPoint)
        local state = app.main.state

        -- Store spawn point for new level
        state.nextSpawnPoint = spawnPoint

        -- Fade out
        app.main.state.fading = true
        app.main.state.fadeDirection = "out"
        app.main.state.fadeProgress = 0
        app.main.state.pendingLevel = levelName
    end,

    updateFade = function(dt)
        local self = app.systems.transitions
        local state = app.main.state

        if not state.fading then return end

        state.fadeProgress = state.fadeProgress + dt / 0.5  -- 0.5 second fade

        if state.fadeProgress >= 1 then
            if state.fadeDirection == "out" then
                -- Load new level while fully black
                app.main.loadLevel(state.pendingLevel)
                state.fadeDirection = "in"
                state.fadeProgress = 0
            else
                -- Fade complete
                state.fading = false
            end
        end
    end,

    renderFade = function()
        local state = app.main.state
        if not state.fading then return end

        local alpha = state.fadeProgress
        if state.fadeDirection == "in" then
            alpha = 1 - alpha
        end

        -- Draw black overlay
        bestow.graphics3d.drawScreenQuad(Color.new(0, 0, 0, alpha))
    end
}
```

## Spawn Points

### Getting Spawn Points

```lua
local level = app.levels[state.currentLevel]

-- Get specific spawn point
local playerSpawn = level.spawns.player
local checkpoint = level.spawns.checkpoint1

-- List all spawn points
for name, position in pairs(level.spawns) do
    print(name .. ": " .. position)
end
```

### Checkpoint System

```lua
-- systems/checkpoints.lua
return {
    lastCheckpoint = nil,

    activate = function(checkpointId)
        local self = app.systems.checkpoints
        self.lastCheckpoint = checkpointId

        -- Save progress
        app.systems.save.setCheckpoint(checkpointId)

        -- Visual/audio feedback
        app.systems.audio.playSfx("checkpoint")
    end,

    respawnPlayer = function()
        local self = app.systems.checkpoints
        local state = app.main.state
        local level = app.levels[state.currentLevel]

        -- Get spawn position
        local spawnPos = level.spawns.player  -- Default
        if self.lastCheckpoint then
            spawnPos = level.spawns[self.lastCheckpoint]
        end

        -- Reset player
        if state.player and bestow.entity.isValid(state.player) then
            bestow.entity.setField(state.player, "Transform3D", "position", spawnPos)

            local health = bestow.entity.getComponent(state.player, "Health")
            health.current = health.max
            bestow.entity.setComponent(state.player, "Health", health)
        else
            state.player = app.entities.player.spawnAt(spawnPos)
        end
    end
}
```

## Object Spawners

Create spawner modules for level objects:

```lua
-- spawners/enemy_spawner.lua
return {
    spawn = function(position, params)
        local enemy = params.enemy or "goblin"
        local count = params.count or 1

        local entities = {}
        for i = 1, count do
            local offset = Vec3.new(
                math.random() * 4 - 2,
                0,
                math.random() * 4 - 2
            )
            local entity = app.entities.enemies[enemy].spawnAt(position + offset)
            table.insert(entities, entity)
        end

        -- Return container entity or first entity
        return entities[1]
    end
}

-- spawners/checkpoint.lua
return {
    spawn = function(position, params)
        local checkpoint = bestow.entity.create()

        bestow.entity.addComponent(checkpoint, "Transform3D", {
            position = position,
            rotation = Quat.identity(),
            scale = Vec3.new(1, 1, 1)
        })

        bestow.entity.addComponent(checkpoint, "Checkpoint", {
            id = params.id,
            activated = false
        })

        -- Trigger volume for activation
        bestow.physics3d.createBody(checkpoint, {
            type = "Static",
            shapeType = "Box",
            halfExtents = Vec3.new(1, 2, 1),
            isSensor = true,
            layer = Layers.Trigger
        })

        return checkpoint
    end
}

-- spawners/door.lua
return {
    spawn = function(position, params)
        local door = bestow.entity.create()

        bestow.entity.addComponent(door, "Transform3D", {
            position = position,
            rotation = Quat.identity(),
            scale = Vec3.new(2, 3, 0.5)
        })

        bestow.entity.addComponent(door, "MeshRenderer", {
            mesh = "meshes/door.obj",
            material = "materials/door"
        })

        bestow.entity.addComponent(door, "LevelDoor", {
            targetLevel = params.to,
            targetSpawn = params.spawn
        })

        -- Trigger for entering
        bestow.physics3d.createBody(door, {
            type = "Static",
            shapeType = "Box",
            halfExtents = Vec3.new(1, 1.5, 0.5),
            isSensor = true
        })

        return door
    end
}
```

## Level Events

Use table+method pattern for event subscriptions (hot-reload safe):

```lua
-- systems/level_triggers.lua
return {
    init = function()
        local self = app.systems.level_triggers
        self.subId = bestow.events.subscribe("trigger_enter_3d", {},
            app.systems.level_triggers, "onTriggerEnter")
    end,

    onTriggerEnter = function(event)
        local state = app.main.state

        -- Check if player entered a door
        local door = nil
        if event.entityA == state.player and bestow.entity.hasComponent(event.entityB, "LevelDoor") then
            door = event.entityB
        elseif event.entityB == state.player and bestow.entity.hasComponent(event.entityA, "LevelDoor") then
            door = event.entityA
        end

        if door then
            local doorData = bestow.entity.getComponent(door, "LevelDoor")
            app.systems.transitions.transitionTo(doorData.targetLevel, doorData.targetSpawn)
        end

        -- Check checkpoint
        local checkpoint = nil
        if event.entityA == state.player and bestow.entity.hasComponent(event.entityB, "Checkpoint") then
            checkpoint = event.entityB
        elseif event.entityB == state.player and bestow.entity.hasComponent(event.entityA, "Checkpoint") then
            checkpoint = event.entityA
        end

        if checkpoint then
            local checkpointData = bestow.entity.getComponent(checkpoint, "Checkpoint")
            if not checkpointData.activated then
                checkpointData.activated = true
                bestow.entity.setComponent(checkpoint, "Checkpoint", checkpointData)
                app.systems.checkpoints.activate(checkpointData.id)
            end
        end
    end,

    shutdown = function()
        local self = app.systems.level_triggers
        if self.subId then bestow.events.unsubscribe(self.subId) end
    end
}
```

## Best Practices

1. **Define levels as data** - Positions, spawns, lighting in the level file
2. **Use load/unload functions** - Clean setup and teardown
3. **Track level entities** - Destroy them all on unload
4. **Fade transitions** - Hide loading with fade to black
5. **Use spawn points** - Named positions for flexibility
6. **Create spawner modules** - Reusable object creation
7. **Subscribe to trigger events** - For doors, checkpoints, etc.
