---
name: entity-system
description: Create and manage entities with components in Bestow. Use when creating game objects, adding components, querying entities, or working with the ECS (Entity Component System).
---

# Entity System

The entity system is the foundation of all game objects in Bestow. Entities are lightweight identifiers that hold components (data).

## Creating Entities

```lua
-- Create a new entity
local entity = bestow.entity.create()

-- Check if valid
if bestow.entity.isValid(entity) then
    -- Use entity
end

-- Destroy when done
bestow.entity.destroy(entity)
```

## Adding Components

Components are data containers. Add them by name with a data table:

```lua
-- Add a transform component
bestow.entity.addComponent(entity, "Transform3D", {
    position = Vec3.new(0, 1, 0),
    rotation = Quat.identity(),
    scale = Vec3.new(1, 1, 1)
})

-- Add a mesh renderer
bestow.entity.addComponent(entity, "MeshRenderer", {
    mesh = "meshes/player.obj",
    material = "materials/player"
})

-- Add custom game components
bestow.entity.addComponent(entity, "Health", {
    current = 100,
    max = 100
})

bestow.entity.addComponent(entity, "Velocity", {
    linear = Vec3.new(0, 0, 0),
    angular = Vec3.new(0, 0, 0)
})
```

## Removing Components

```lua
bestow.entity.removeComponent(entity, "Health")
```

## Checking Components

```lua
if bestow.entity.hasComponent(entity, "Health") then
    -- Entity has health
end
```

## Reading Components

```lua
-- Get entire component as table
local health = bestow.entity.getComponent(entity, "Health")
if health then
    print("Health: " .. health.current .. "/" .. health.max)
end

-- Get single field (more efficient for one value)
local position = bestow.entity.getField(entity, "Transform3D", "position")
```

## Modifying Components

```lua
-- Set entire component
local health = bestow.entity.getComponent(entity, "Health")
health.current = health.current - 10
bestow.entity.setComponent(entity, "Health", health)

-- Set single field (more efficient for one value)
bestow.entity.setField(entity, "Transform3D", "position", Vec3.new(5, 0, 0))
```

## Iterating Entities

```lua
-- Iterate ALL entities
bestow.entity.each(function(entity)
    -- Called for every entity
end)

-- Filter by component inside the callback
bestow.entity.each(function(entity)
    if bestow.entity.hasComponent(entity, "Enemy") then
        -- Process enemies
    end
end)
```

## Common Components

### Transform3D (3D position/rotation/scale)
```lua
{
    position = Vec3.new(x, y, z),
    rotation = Quat.identity(),  -- or Quat.fromAxisAngle(axis, angle)
    scale = Vec3.new(1, 1, 1)
}
```

### MeshRenderer (3D mesh display)
```lua
{
    mesh = "path/to/mesh",
    material = "path/to/material"
}
```

### Velocity (movement)
```lua
{
    linear = Vec3.new(vx, vy, vz),
    angular = Vec3.new(ax, ay, az)
}
```

### Health (damage system)
```lua
{
    current = 100,
    max = 100,
    invulnerable = false
}
```

## Entity Blueprint Pattern

Create reusable entity factories in `entities/`:

```lua
-- entities/player.lua
return {
    defaults = {
        Transform3D = {
            position = Vec3.new(0, 1, 0),
            rotation = Quat.identity(),
            scale = Vec3.new(1, 1, 1)
        },
        Health = {
            current = 100,
            max = 100
        },
        PlayerController = {
            speed = 5.0,
            jumpForce = 10.0
        }
    },

    create = function(overrides)
        local self = app.entities.player  -- Hot reload safe!
        local entity = bestow.entity.create()

        for compName, defaults in pairs(self.defaults) do
            local data = {}
            -- Copy defaults
            for k, v in pairs(defaults) do data[k] = v end
            -- Apply overrides
            if overrides and overrides[compName] then
                for k, v in pairs(overrides[compName]) do data[k] = v end
            end
            bestow.entity.addComponent(entity, compName, data)
        end

        return entity
    end,

    spawnAt = function(position)
        local self = app.entities.player
        return self.create({
            Transform3D = { position = position }
        })
    end
}
```

Usage:
```lua
local player = app.entities.player.create()
local player2 = app.entities.player.spawnAt(Vec3.new(10, 1, 0))
```

## Tag Components

Use empty tables as tags to mark entity types:

```lua
-- Add tag
bestow.entity.addComponent(entity, "EnemyTag", {})
bestow.entity.addComponent(entity, "PlayerTag", {})

-- Check tag
if bestow.entity.hasComponent(entity, "EnemyTag") then
    -- This is an enemy
end
```

## Best Practices

1. **Create blueprints for reusable entities** - Don't duplicate component setup
2. **Use getField/setField for single values** - More efficient than get/set entire component
3. **Store entity references in app.main.state** - Survives hot reload
4. **Use tag components for filtering** - Empty tables are cheap
5. **Destroy entities when done** - Prevents memory leaks
