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
        local rotation = Quat.lookAt(forward, Vec3.new(0, 1, 0))

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

For special cases, draw meshes manually. `drawMesh()` requires MeshHandle and MaterialHandle -- not string paths. Use primitive mesh generators or `bestow.assets` to obtain handles first:

```lua
-- Create mesh and material handles first
local cubeMesh = bestow.graphics3d.createCubeMesh(1.0)  -- returns MeshHandle
local mat = PBRMaterial.new()
mat.baseColorFactor = Vec4(0.8, 0.2, 0.2, 1.0)
local redMaterial = bestow.graphics3d.createMaterial(mat)  -- returns MaterialHandle

-- Draw with a Transform3D
bestow.graphics3d.drawMesh(cubeMesh, redMaterial, {
    position = Vec3.new(5, 0, 0),
    rotation = Quat.identity(),
    scale = Vec3.new(1, 1, 1)
})

-- Or draw with a Mat4 transform
local transform = Mat4.identity()
bestow.graphics3d.drawMesh(cubeMesh, redMaterial, transform)

-- Optional: control shadow casting/receiving (both default to true)
bestow.graphics3d.drawMesh(cubeMesh, redMaterial, transform, true, true)
```

### Primitive Mesh Generators

```lua
local cube = bestow.graphics3d.createCubeMesh(1.0)           -- size
local sphere = bestow.graphics3d.createSphereMesh(0.5, 32, 16) -- radius, segments, rings
local cylinder = bestow.graphics3d.createCylinderMesh(0.5, 1.0, 32) -- radius, height, segments
local capsule = bestow.graphics3d.createCapsuleMesh(0.5, 1.0, 32, 8) -- radius, height, segments, rings
local plane = bestow.graphics3d.createPlaneMesh(1.0, 1.0, 1, 1) -- width, height, wSegments, hSegments

-- Clean up when no longer needed
bestow.graphics3d.destroyMesh(cube)
```

### Material Creation

```lua
-- PBR material
local pbr = PBRMaterial.new()
pbr.baseColorFactor = Vec4(1.0, 0.5, 0.0, 1.0)
pbr.metallicFactor = 0.8
pbr.roughnessFactor = 0.2
local metalMat = bestow.graphics3d.createMaterial(pbr)

-- Unlit material (no lighting calculations)
local unlit = UnlitMaterial.new()
unlit.color = Color.new(1.0, 0.0, 1.0, 1.0)
local unlitMat = bestow.graphics3d.createUnlitMaterial(unlit)

-- Get built-in defaults
local defaultPBR = bestow.graphics3d.getDefaultPBRMaterial()
local defaultUnlit = bestow.graphics3d.getDefaultUnlitMaterial()

-- Clean up when no longer needed
bestow.graphics3d.destroyMaterial(metalMat)
```

## Lighting

### Ambient Light

```lua
-- Set global ambient light (color is Vec3, not Color)
bestow.graphics3d.setAmbientLight(
    Vec3.new(0.2, 0.2, 0.3),  -- Color as Vec3 (slight blue)
    0.3                        -- Intensity (optional, defaults to 1.0)
)
```

### Directional Light (Sun)

```lua
bestow.graphics3d.setDirectionalLight({
    direction = Vec3.new(-0.5, -1, -0.5):normalize(),
    color = Vec3.new(1.0, 0.95, 0.8),  -- Warm sunlight (Vec3)
    intensity = 1.0,
    castShadows = true,
    shadowMapResolution = 2048  -- optional
})
```

### Point Lights

```lua
-- Add a point light: light struct + position as separate args
local light = PointLight.new()
light.color = Vec3.new(1.0, 0.5, 0.0)  -- Orange (Vec3)
light.intensity = 2.0
light.range = 10.0
light.castShadows = false

local lightId = bestow.graphics3d.addPointLight(light, Vec3.new(5, 3, 0))

-- Move a light
bestow.graphics3d.setLightPosition(lightId, Vec3.new(10, 3, 0))

-- Remove light later
bestow.graphics3d.removeLight(lightId)

-- Remove all lights
bestow.graphics3d.clearLights()
```

### Spot Lights

```lua
-- Spot light: light struct + position as separate args
local spot = SpotLight.new()
spot.direction = Vec3.new(0, -1, 0)
spot.color = Vec3.new(1.0, 1.0, 1.0)
spot.intensity = 3.0
spot.range = 20.0
spot.innerConeAngle = 15.0  -- degrees
spot.outerConeAngle = 30.0
spot.castShadows = true

local spotId = bestow.graphics3d.addSpotLight(spot, Vec3.new(0, 5, 0))
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
-- Set skybox from cubemap texture handle
local skybox = Skybox.new()
skybox.cubemapTexture = cubemapHandle  -- TextureHandle from bestow.assets
skybox.rotation = 0.0                  -- Rotation in radians
skybox.exposure = 1.0                  -- Exposure multiplier
bestow.graphics3d.setSkybox(skybox)

-- Clear skybox
bestow.graphics3d.clearSkybox()
```

## Debug Drawing

For development visualization:

```lua
-- Draw debug line (color, duration, depthTest are optional)
bestow.graphics3d.debugDrawLine(
    Vec3.new(0, 0, 0),      -- Start
    Vec3.new(10, 0, 0),     -- End
    Color.new(1, 0, 0, 1)   -- Red (optional, defaults to white)
)

-- Draw debug box (rotation, color, duration, depthTest are optional)
bestow.graphics3d.debugDrawBox(
    Vec3.new(5, 1, 5),      -- Center
    Vec3.new(1, 2, 1),      -- Half extents
    Quat.identity(),         -- Rotation (optional)
    Color.new(0, 1, 0, 1)   -- Green (optional)
)

-- Draw debug sphere (color, duration, depthTest are optional)
bestow.graphics3d.debugDrawSphere(
    Vec3.new(0, 3, 0),      -- Center
    1.5,                     -- Radius
    Color.new(0, 0, 1, 1)   -- Blue (optional)
)

-- Draw debug ray
bestow.graphics3d.debugDrawRay(
    Vec3.new(0, 0, 0),      -- Origin
    Vec3.new(0, 1, 0),      -- Direction
    5.0                      -- Length
)
```

## Screen Effects

### Screen Shake

Screen shake is implemented as a Lua camera system (see camera-system skill):

```lua
-- Trigger shake from camera system
app.systems.camera.shake(0.5, 0.3)  -- intensity, duration
```

### Zoom

Zoom is implemented by adjusting the camera FOV (see camera-system skill):

```lua
-- Zoom via FOV change (getCamera returns Camera3D struct)
local cam = bestow.graphics3d.getCamera()
bestow.graphics3d.setCamera({
    position = cam.transform.position,
    rotation = cam.transform.rotation,
    fov = cam.fovY / 1.5,  -- Zoom in
    near = cam.nearPlane,
    far = cam.farPlane
})
```

## Coordinate Conversion

```lua
-- Screen position to world (via raycast)
local screenPos = bestow.input.getMousePosition()
local ray = bestow.graphics3d.screenToWorldRay(screenPos)
local hit = bestow.physics3d.raycast(ray.origin, ray.direction, 1000)

-- World position to screen
local screenPos = bestow.graphics3d.worldToScreen(Vec3.new(5, 0, 10))
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
