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

-- CORRECT: Register and play sound by handle
local handle = bestow.assets.registerAsset(AssetType.Sound, "sounds/click.wav")
bestow.assets.loadAsset(handle)
bestow.audio.playOnChannel(bestow.audio.Channel.UI, {
    asset = handle,
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

## Registering and Loading Assets

Assets follow a register → load → use workflow:

```lua
-- Register (creates a handle, doesn't load yet)
local handle = bestow.assets.registerAsset(AssetType.Sound, "sounds/jump.wav")

-- Load (synchronous)
bestow.assets.loadAsset(handle)

-- Or load asynchronously
bestow.assets.loadAssetAsync(handle, function(h, state)
    if state == "Loaded" then
        -- Ready to use
    end
end)

-- Check state
bestow.assets.isLoaded(handle) -> bool
```

**MeshRenderer components** handle loading automatically via path references:
```lua
bestow.entity.addComponent(entity, "MeshRenderer", {
    mesh = "meshes/cube.obj",
    material = "materials/stone"
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
        local typeMap = { meshes = AssetType.Mesh, textures = AssetType.Texture, sounds = AssetType.Sound }
        for assetType, list in pairs(self.assetsToLoad) do
            for _, path in ipairs(list) do
                local handle = bestow.assets.registerAsset(typeMap[assetType], path)
                bestow.assets.loadAssetAsync(handle, function(h, state)
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
-- Register first to get a handle
local handle = bestow.assets.registerAsset(AssetType.Mesh, "meshes/boss.obj")

-- Check if asset is loaded
if bestow.assets.isLoaded(handle) then
    -- Asset is ready to use
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
-- Subscribe using table+method pattern (hot-reload safe)
bestow.events.subscribe("asset_reloaded", {},
    app.systems.renderer, "onAssetReloaded")

-- In the receiving system:
-- onAssetReloaded = function(event)
--     print("Reloaded: " .. event.path)
-- end
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
