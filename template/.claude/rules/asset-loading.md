---
paths: "**/*.lua"
---

# Asset Loading Rules

## Never Use io.* or Direct File Access

The `io` library is disabled. All assets must go through `bestow.assets`:

```lua
-- WRONG: io is disabled
local file = io.open("sounds/music.ogg")

-- CORRECT: Use bestow.assets
local handle = bestow.assets.registerAsset(AssetType.Sound, "sounds/music.ogg")
bestow.assets.loadAsset(handle)
```

## Asset Loading Workflow

1. **Register** the asset to get a handle
2. **Load** the asset (sync or async)
3. **Use** the handle with appropriate system
4. **Unload** when done (optional, automatic on shutdown)

```lua
-- 1. Register
local musicHandle = bestow.assets.registerAsset(AssetType.Music, "music/theme.ogg")

-- 2. Load (sync)
bestow.assets.loadAsset(musicHandle)

-- 3. Use
bestow.audio.playOnChannel(bestow.audio.Channel.Music, {
    asset = musicHandle,
    volume = 0.8,
    looping = true
})

-- 4. Unload (when no longer needed)
bestow.assets.unloadAsset(musicHandle)
```

## Async Loading for Large Assets

For textures, meshes, and other large assets, prefer async loading:

```lua
local textureHandle = bestow.assets.registerAsset(AssetType.Texture, "textures/large.png")

-- Async load - continues without blocking
bestow.assets.loadAssetAsync(textureHandle)

-- Check state later
if bestow.assets.isLoaded(textureHandle) then
    -- Ready to use
end
```

## Asset Types

```lua
AssetType.Texture   -- Images (png, jpg, etc.)
AssetType.Sound     -- Sound effects (wav, ogg)
AssetType.Music     -- Streaming music (ogg, mp3)
AssetType.Font      -- Fonts (ttf, otf)
AssetType.Mesh      -- 3D meshes (obj, gltf)
AssetType.Model     -- 3D models with materials
AssetType.Material  -- Material definitions
AssetType.Shader    -- Shader programs
AssetType.Cubemap   -- Skybox textures
```
