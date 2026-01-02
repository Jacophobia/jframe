---
name: graphics-system
description: Render 3D graphics, set up cameras, lighting, and fog in Bestow. Use when drawing meshes, setting up the scene, configuring the camera, or adjusting visual effects.
---

# Graphics System

The 3D graphics system handles rendering, cameras, lighting, and visual effects.

## Frame Lifecycle

Every frame must be wrapped in begin/end:

```lua
render = function()
    bestow.graphics3d.beginFrame()

    -- All rendering happens here
    -- Entities with MeshRenderer are drawn automatically

    bestow.graphics3d.endFrame()
end
```

## Camera Setup

### Setting the Active Camera

```lua
local camera = {
    position = Vec3.new(0, 10, -20),
    rotation = Quat.fromAxisAngle(Vec3.new(1, 0, 0), 0.3),  -- Tilt down
    fov = 45.0,              -- Field of view in degrees
    near = 0.1,              -- Near clip plane
    far = 1000.0             -- Far clip plane
}

bestow.graphics3d.setCamera(camera)
```

### Follow Camera Pattern

```lua
-- systems/camera.lua
return {
    offset = Vec3.new(0, 10, -15),
    lookAhead = Vec3.new(0, 0, -5),

    update = function(dt)
        local self = app.systems.camera
        local state = app.main.state

        if not state.player then return end

        local playerPos = bestow.entity.getField(state.player, "Transform3D", "position")

        -- Camera follows player with offset
        local camPos = playerPos + self.offset

        -- Look at player + look ahead
        local lookAt = playerPos + self.lookAhead
        local forward = (lookAt - camPos):normalize()

        -- Create rotation from forward vector
        local rotation = Quat.lookRotation(forward, Vec3.new(0, 1, 0))

        bestow.graphics3d.setCamera({
            position = camPos,
            rotation = rotation,
            fov = 45.0,
            near = 0.1,
            far = 1000.0
        })
    end
}
```

## Drawing Meshes

### Automatic Rendering (Preferred)

Entities with `MeshRenderer` components are drawn automatically:

```lua
bestow.entity.addComponent(entity, "MeshRenderer", {
    mesh = "meshes/cube.obj",
    material = "materials/default"
})
```

### Manual Drawing

For special cases, draw meshes manually:

```lua
-- Draw at specific transform
bestow.graphics3d.drawMesh("meshes/cube.obj", "materials/default", {
    position = Vec3.new(5, 0, 0),
    rotation = Quat.identity(),
    scale = Vec3.new(1, 1, 1)
})

-- Or with a Mat4 transform
local transform = Mat4.identity()
bestow.graphics3d.drawMesh("meshes/cube.obj", "materials/default", transform)
```

### Instanced Rendering

For many identical objects:

```lua
local transforms = {}
for i = 1, 100 do
    table.insert(transforms, {
        position = Vec3.new(i * 2, 0, 0),
        rotation = Quat.identity(),
        scale = Vec3.new(1, 1, 1)
    })
end

bestow.graphics3d.drawMeshInstanced("meshes/tree.obj", "materials/tree", transforms)
```

## Lighting

### Ambient Light

```lua
-- Set global ambient light
bestow.graphics3d.setAmbientLight(
    Color.new(0.2, 0.2, 0.3, 1.0),  -- Color (slight blue)
    0.3                              -- Intensity
)
```

### Directional Light (Sun)

```lua
bestow.graphics3d.setDirectionalLight({
    direction = Vec3.new(-0.5, -1, -0.5):normalize(),
    color = Color.new(1.0, 0.95, 0.8, 1.0),  -- Warm sunlight
    intensity = 1.0,
    castShadows = true
})
```

### Point Lights

```lua
-- Add a point light
local lightId = bestow.graphics3d.addPointLight({
    position = Vec3.new(5, 3, 0),
    color = Color.new(1.0, 0.5, 0.0, 1.0),  -- Orange
    intensity = 2.0,
    range = 10.0,
    castShadows = false
})

-- Remove light later
bestow.graphics3d.removeLight(lightId)
```

### Spot Lights

```lua
local spotId = bestow.graphics3d.addSpotLight({
    position = Vec3.new(0, 5, 0),
    direction = Vec3.new(0, -1, 0),
    color = Color.new(1.0, 1.0, 1.0, 1.0),
    intensity = 3.0,
    range = 20.0,
    innerConeAngle = 15.0,  -- degrees
    outerConeAngle = 30.0,
    castShadows = true
})
```

## Fog

```lua
bestow.graphics3d.setFog({
    enabled = true,
    color = Color.new(0.5, 0.6, 0.7, 1.0),
    density = 0.02,          -- For exponential fog
    startDistance = 10.0,    -- For linear fog
    endDistance = 100.0
})

-- Disable fog
bestow.graphics3d.setFog({ enabled = false })
```

## Skybox

```lua
-- Set skybox from cubemap
bestow.graphics3d.setSkybox("textures/skybox")

-- The path refers to a cubemap with faces:
-- textures/skybox_right.png, textures/skybox_left.png
-- textures/skybox_top.png, textures/skybox_bottom.png
-- textures/skybox_front.png, textures/skybox_back.png
```

## Debug Drawing

For development visualization:

```lua
-- Draw debug line
bestow.graphics3d.drawDebugLine(
    Vec3.new(0, 0, 0),      -- Start
    Vec3.new(10, 0, 0),     -- End
    Color.new(1, 0, 0, 1)   -- Red
)

-- Draw debug box
bestow.graphics3d.drawDebugBox(
    Vec3.new(5, 1, 5),      -- Center
    Vec3.new(1, 2, 1),      -- Half extents
    Color.new(0, 1, 0, 1)   -- Green
)

-- Draw debug sphere
bestow.graphics3d.drawDebugSphere(
    Vec3.new(0, 3, 0),      -- Center
    1.5,                     -- Radius
    Color.new(0, 0, 1, 1)   -- Blue
)
```

## Screen Effects

### Screen Shake

```lua
-- Camera shake for impacts
bestow.camera3d.shake(
    0.5,    -- Intensity (units)
    0.3     -- Duration (seconds)
)
```

### Zoom

```lua
-- Zoom in (values > 1)
bestow.camera3d.setZoom(1.5)

-- Zoom out (values < 1)
bestow.camera3d.setZoom(0.75)

-- Reset
bestow.camera3d.setZoom(1.0)
```

## Coordinate Conversion

```lua
-- Screen position to world
local screenPos = bestow.input.getMousePosition()
local worldPos = bestow.camera3d.screenToWorld(screenPos)

-- World position to screen
local screenPos = bestow.camera3d.worldToScreen(Vec3.new(5, 0, 10))
```

## Material System

Materials are defined in Lua files in `assets/materials/`:

```lua
-- assets/materials/player.lua
return {
    shader = "shaders/pbr",
    textures = {
        albedo = "textures/player_albedo.png",
        normal = "textures/player_normal.png",
        roughness = "textures/player_roughness.png"
    },
    properties = {
        metallic = 0.0,
        roughness = 0.5,
        emissive = Color.new(0, 0, 0, 1)
    }
}
```

Reference by path without extension:
```lua
mesh = "meshes/player",
material = "materials/player"
```

## Render Layers

Control draw order with render layers:

```lua
-- Entities are drawn in layer order (lower first)
bestow.entity.addComponent(entity, "RenderLayer", {
    layer = 10  -- Default is 0
})

-- Common layer values:
-- Background = -100
-- Terrain = 0
-- Entities = 10
-- Effects = 20
-- UI = 100
```

## Best Practices

1. **Use MeshRenderer components** - Let the engine batch and optimize
2. **Minimize manual drawMesh calls** - They're less efficient
3. **Reuse materials** - Don't create unique materials per entity
4. **Set up lighting once in init()** - Unless dynamic
5. **Use debug drawing sparingly** - It's slow, for development only
6. **Consider fog for depth** - Helps sell distance in 3D
