---
name: assets-system
description: Load and manage game assets like textures, sounds, meshes, and materials in Bestow. Use when loading assets, checking load status, or understanding the asset pipeline. Important: Assets are handles, not raw data.
---

# Assets System

The asset system manages loading, caching, and hot-reloading of game assets.

## Core Principle: Assets Are Handles

**Never try to read file contents directly.** Lua scripts work with asset paths and handles. The engine loads and manages the actual data.

```lua
-- WRONG: Trying to read files
local data = io.open("textures/player.png")  -- io is disabled!

-- CORRECT: Reference by path, engine handles the rest
bestow.entity.addComponent(entity, "MeshRenderer", {
    mesh = "meshes/player.obj",
    material = "materials/player"
})

-- CORRECT: Play sound by path
bestow.audio.playOnChannel(Channels.UI, {
    path = "sounds/click.wav",
    volume = 1.0
})
```

## Asset Types and Paths

| Type | Directory | Extensions | Usage |
|------|-----------|------------|-------|
| Textures | `assets/textures/` | .png, .jpg | MeshRenderer, UI |
| Sounds | `assets/sounds/` | .wav, .ogg, .mp3 | Audio system |
| Meshes | `assets/meshes/` | .obj, .gltf, .fbx | MeshRenderer |
| Materials | `assets/materials/` | .lua | MeshRenderer |
| Fonts | `assets/fonts/` | .ttf | Text rendering |

### Path Resolution

Paths are relative to your game's root directory:

```lua
-- If your game is in /games/my-game/
-- Then "textures/player.png" refers to /games/my-game/assets/textures/player.png

mesh = "meshes/player.obj"      -- assets/meshes/player.obj
material = "materials/default"  -- assets/materials/default.lua
path = "sounds/jump.wav"        -- assets/sounds/jump.wav
```

## Automatic Asset Loading

Most assets are loaded automatically when first used:

```lua
-- Mesh and material loaded on first render
bestow.entity.addComponent(entity, "MeshRenderer", {
    mesh = "meshes/cube.obj",
    material = "materials/stone"
})

-- Sound loaded on first play
bestow.audio.playOnChannel(Channels.UI, {
    path = "sounds/explosion.wav"
})
```

## Preloading Assets

For smoother gameplay, preload assets during loading screens:

```lua
-- systems/loader.lua
return {
    assetsToLoad = {
        meshes = {
            "meshes/player.obj",
            "meshes/enemy.obj",
            "meshes/environment/tree.obj"
        },
        textures = {
            "textures/player.png",
            "textures/enemy.png"
        },
        sounds = {
            "sounds/jump.wav",
            "sounds/hit.wav",
            "sounds/music/gameplay.ogg"
        }
    },

    loadProgress = 0,
    totalAssets = 0,
    loadedAssets = 0,

    startLoading = function()
        local self = app.systems.loader

        -- Count total assets
        self.totalAssets = 0
        for _, list in pairs(self.assetsToLoad) do
            self.totalAssets = self.totalAssets + #list
        end

        self.loadedAssets = 0
        self.loadProgress = 0

        -- Queue all assets for loading
        for assetType, list in pairs(self.assetsToLoad) do
            for _, path in ipairs(list) do
                bestow.assets.loadAssetAsync(path, function(state)
                    if state == "Loaded" then
                        self.loadedAssets = self.loadedAssets + 1
                        self.loadProgress = self.loadedAssets / self.totalAssets
                    end
                end)
            end
        end
    end,

    isComplete = function()
        local self = app.systems.loader
        return self.loadedAssets >= self.totalAssets
    end,

    getProgress = function()
        local self = app.systems.loader
        return self.loadProgress
    end
}

-- Usage in loading screen
app.systems.loader.startLoading()

-- In update
if app.systems.loader.isComplete() then
    transitionToGameplay()
else
    local progress = app.systems.loader.getProgress()
    renderLoadingBar(progress)
end
```

## Checking Asset State

```lua
-- Check if asset is loaded
if bestow.assets.isLoaded("meshes/boss.obj") then
    -- Asset is ready to use
end

-- Get detailed state
local state = bestow.assets.getAssetState("meshes/player.obj")
-- state is one of: "Unloaded", "Loading", "Loaded", "Failed"

if state == "Failed" then
    print("Failed to load asset!")
end
```

## Materials

Materials are defined in Lua files:

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
        tint = { 1.0, 1.0, 1.0, 1.0 }
    },
    blendMode = "Opaque",  -- "Opaque", "AlphaBlend", "Additive"
    cullMode = "Back"       -- "None", "Front", "Back"
}
```

Reference without extension:
```lua
material = "materials/player"  -- Loads materials/player.lua
```

## Hot Reload

Assets are automatically reloaded when modified during development:

```bash
# Run with hot reload enabled
bestow run main.lua --hot-reload
```

When you save a texture, mesh, or material file, it's reloaded automatically. Your game doesn't need to handle this - it happens transparently.

### Responding to Asset Reloads

If you need to respond to asset changes:

```lua
-- Subscribe to asset reload events
bestow.events.subscribe("asset_reloaded", function(event)
    local assetPath = event.path
    local assetType = event.type

    print("Reloaded: " .. assetPath)

    -- Refresh anything that cached this asset
    if assetType == "Material" then
        -- Material changed, might need to update visuals
    end
end)
```

## Asset Organization

Recommended directory structure:

```
assets/
├── textures/
│   ├── characters/
│   │   ├── player_albedo.png
│   │   ├── player_normal.png
│   │   └── enemy_albedo.png
│   ├── environment/
│   │   ├── grass.png
│   │   └── stone.png
│   └── ui/
│       ├── button.png
│       └── panel.png
├── meshes/
│   ├── characters/
│   │   ├── player.obj
│   │   └── enemy.obj
│   └── environment/
│       ├── tree.obj
│       └── rock.obj
├── materials/
│   ├── characters/
│   │   ├── player.lua
│   │   └── enemy.lua
│   └── environment/
│       ├── grass.lua
│       └── stone.lua
├── sounds/
│   ├── sfx/
│   │   ├── jump.wav
│   │   ├── hit.wav
│   │   └── pickup.wav
│   └── music/
│       ├── menu.ogg
│       └── gameplay.ogg
└── fonts/
    └── main.ttf
```

## Sharing Assets Between Entities

Assets are cached and shared automatically:

```lua
-- These all share the same mesh data in memory
for i = 1, 100 do
    local tree = bestow.entity.create()
    bestow.entity.addComponent(tree, "MeshRenderer", {
        mesh = "meshes/tree.obj",      -- Same mesh, loaded once
        material = "materials/tree"     -- Same material
    })
end
```

## Dynamic Material Properties

Modify material properties at runtime:

```lua
-- Change tint on an entity's material instance
bestow.graphics3d.setMaterialProperty(entity, "tint", { 1.0, 0.5, 0.5, 1.0 })

-- Change roughness
bestow.graphics3d.setMaterialProperty(entity, "roughness", 0.8)
```

## Best Practices

1. **Never read files directly** - Use asset paths, let engine handle data
2. **Preload during loading screens** - Prevents hitches during gameplay
3. **Organize by category** - characters/, environment/, ui/
4. **Use consistent naming** - player_albedo.png, player_normal.png
5. **Share assets between entities** - They're cached automatically
6. **Use material Lua files** - Easier to tweak than raw textures
7. **Trust hot reload** - Just save files, engine handles the rest
