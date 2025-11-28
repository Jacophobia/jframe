# AssetSystem API

The `AssetSystem` provides asset loading, hot-reload, and management for textures, sounds, fonts, and more.

## Overview

```cpp
auto& assets = sys.assets;

// Register asset
AssetHandle texture = assets->registerAsset(
    AssetType::Texture, "textures/player.png"
);

// Load synchronously
assets->loadAsset(texture);

// Load asynchronously with callback
assets->loadAssetAsync(texture, [](AssetHandle h, AssetState state) {
    if (state == AssetState::Loaded) {
        // Asset ready
    }
});

// Get asset data
if (assets->isLoaded(texture)) {
    auto* texData = assets->getAsset<TextureData>(texture);
}

// Hot-reload (development)
assets->enableHotReload(true);
assets->update();  // Checks for file changes
```

## Lifecycle

### update()

```cpp
void update();
```

Updates the asset system. Processes async loads and hot-reload checks.

**Call once per frame.**

---

## Asset Registration

### registerAsset(AssetType type, const std::filesystem::path& path)

```cpp
AssetHandle registerAsset(AssetType type, const std::filesystem::path& path);
```

Registers an asset for loading.

**Asset Types:**
- `AssetType::Texture` - Images (PNG, JPG, etc.)
- `AssetType::Sound` - Short audio (WAV, OGG)
- `AssetType::Music` - Long audio (streamed)
- `AssetType::Font` - TrueType fonts (TTF, OTF)
- `AssetType::Level` - Level data (Lua)
- `AssetType::Data` - Generic data files
- `AssetType::Shader` - Shader programs
- `AssetType::NavMesh` - Navigation meshes
- `AssetType::BehaviorTree` - AI behavior trees

**Example:**

```cpp
AssetHandle playerTex = assets->registerAsset(
    AssetType::Texture, "textures/player.png"
);
AssetHandle jumpSound = assets->registerAsset(
    AssetType::Sound, "sounds/jump.wav"
);
AssetHandle bgMusic = assets->registerAsset(
    AssetType::Music, "music/level1.ogg"
);
AssetHandle font = assets->registerAsset(
    AssetType::Font, "fonts/arial.ttf"
);
```

---

### unregisterAsset(AssetHandle handle)

```cpp
void unregisterAsset(AssetHandle handle);
```

Unregisters an asset. Asset must be unloaded first.

---

## Loading Assets

### loadAsset(AssetHandle handle)

```cpp
void loadAsset(AssetHandle handle);
```

Loads an asset synchronously (blocks until loaded).

**Example:**

```cpp
AssetHandle texture = assets->registerAsset(
    AssetType::Texture, "textures/player.png"
);
assets->loadAsset(texture);

if (assets->getAssetState(texture) == AssetState::Loaded) {
    // Ready to use
}
```

---

### loadAssetAsync(AssetHandle handle, AssetLoadCallback callback)

```cpp
using AssetLoadCallback = std::function<void(AssetHandle, AssetState)>;
void loadAssetAsync(AssetHandle handle, AssetLoadCallback callback = nullptr);
```

Loads an asset asynchronously (non-blocking).

**Example:**

```cpp
assets->loadAssetAsync(texture, [](AssetHandle h, AssetState state) {
    if (state == AssetState::Loaded) {
        logInfo("Texture loaded");
    } else if (state == AssetState::Failed) {
        logError("Failed to load texture");
    }
});
```

---

### unloadAsset(AssetHandle handle)

```cpp
void unloadAsset(AssetHandle handle);
```

Unloads an asset and frees its memory.

---

## State Queries

### getAssetState(AssetHandle handle)

```cpp
AssetState getAssetState(AssetHandle handle) const;
```

Returns the current state of an asset.

**AssetState Enum:**

```cpp
enum class AssetState : uint8_t {
    Unloaded,
    Loading,
    Loaded,
    Failed
};
```

---

### getAssetMetadata(AssetHandle handle)

```cpp
AssetMetadata getAssetMetadata(AssetHandle handle) const;
```

Returns metadata about an asset.

**AssetMetadata:**

```cpp
struct AssetMetadata {
    AssetHandle handle;
    std::filesystem::path sourcePath;
    AssetState state = AssetState::Unloaded;
    std::size_t sizeBytes = 0;
    std::optional<std::string> errorMessage;
};
```

---

### isLoaded(AssetHandle handle)

```cpp
bool isLoaded(AssetHandle handle) const;
```

Returns `true` if the asset is fully loaded.

---

## Accessing Asset Data

### getRawAsset(AssetHandle handle)

```cpp
void* getRawAsset(AssetHandle handle);
const void* getRawAsset(AssetHandle handle) const;
```

Returns a raw pointer to the asset data.

---

### getAsset<T>(AssetHandle handle)

```cpp
template<typename T>
T* getAsset(AssetHandle handle);

template<typename T>
const T* getAsset(AssetHandle handle) const;
```

Returns a typed pointer to the asset data.

**Example:**

```cpp
// Get texture data
auto* texData = assets->getAsset<TextureData>(textureHandle);
if (texData) {
    std::cout << "Size: " << texData->width << "x" << texData->height << std::endl;
}

// Get font data
auto* fontData = assets->getAsset<FontData>(fontHandle);

// Get sound data
auto* soundData = assets->getAsset<SoundData>(soundHandle);
```

---

## Asset Data Structures

### TextureData

```cpp
struct TextureData {
    std::vector<unsigned char> pixels;
    int width = 0;
    int height = 0;
    int channels = 0;  // 3 = RGB, 4 = RGBA
};
```

---

### FontData

```cpp
struct FontData {
    std::vector<unsigned char> fileData;  // Raw TTF/OTF bytes
    std::string path;
    std::size_t fileSize = 0;
};
```

---

### SoundData

```cpp
struct SoundData {
    std::vector<unsigned char> fileData;  // Raw WAV/OGG/MP3 bytes
    std::string path;
    std::size_t fileSize = 0;
};
```

---

## Bulk Operations

### loadAll()

```cpp
void loadAll();
```

Loads all registered assets synchronously.

---

### unloadAll()

```cpp
void unloadAll();
```

Unloads all assets.

---

### getAssetsOfType(AssetType type)

```cpp
std::vector<AssetHandle> getAssetsOfType(AssetType type) const;
```

Returns all registered assets of a specific type.

**Example:**

```cpp
// Get all textures
auto textures = assets->getAssetsOfType(AssetType::Texture);

// Load all textures
for (const auto& handle : textures) {
    assets->loadAssetAsync(handle);
}
```

---

## Hot Reload (Development)

### enableHotReload(bool enable)

```cpp
void enableHotReload(bool enable);
```

Enables or disables hot-reload. When enabled, `update()` checks for file changes.

**Note:** Only available in Debug/RelWithDebInfo builds.

---

### checkForReloads()

```cpp
void checkForReloads();
```

Manually checks for file changes and reloads modified assets.

---

### reloadAsset(AssetHandle handle)

```cpp
void reloadAsset(AssetHandle handle);
```

Manually reloads a specific asset.

**Example:**

```cpp
#if defined(JFRAME_DEV_TOOLS)
assets->enableHotReload(true);

// In update loop
assets->update();  // Automatically reloads changed files
#endif
```

---

## Common Patterns

### Preloading Level Assets

```cpp
void preloadLevelAssets() {
    std::vector<AssetHandle> levelAssets;

    // Register all assets
    levelAssets.push_back(assets->registerAsset(
        AssetType::Texture, "textures/tileset.png"
    ));
    levelAssets.push_back(assets->registerAsset(
        AssetType::Sound, "sounds/jump.wav"
    ));
    levelAssets.push_back(assets->registerAsset(
        AssetType::Music, "music/level1.ogg"
    ));

    // Load all async
    for (const auto& handle : levelAssets) {
        assets->loadAssetAsync(handle);
    }

    // Wait for all to load
    bool allLoaded = false;
    while (!allLoaded) {
        assets->update();

        allLoaded = true;
        for (const auto& handle : levelAssets) {
            if (assets->getAssetState(handle) != AssetState::Loaded) {
                allLoaded = false;
                break;
            }
        }
    }
}
```

---

### Loading Screen

```cpp
struct LoadingProgress {
    int total = 0;
    int loaded = 0;

    float getProgress() const {
        return total > 0 ? (float)loaded / (float)total : 0.0f;
    }
};

LoadingProgress loadAssetsWithProgress(const std::vector<AssetHandle>& handles) {
    LoadingProgress progress;
    progress.total = handles.size();

    for (const auto& handle : handles) {
        assets->loadAssetAsync(handle, [&progress](AssetHandle h, AssetState state) {
            if (state == AssetState::Loaded || state == AssetState::Failed) {
                progress.loaded++;
            }
        });
    }

    // Update loop
    while (progress.loaded < progress.total) {
        assets->update();

        // Render loading screen
        graphics->beginFrame();
        drawLoadingBar(progress.getProgress());
        graphics->endFrame();
    }

    return progress;
}
```

---

### Asset Manager Utility

```cpp
class AssetManager {
    IAssetSystem* assets_;
    std::unordered_map<std::string, AssetHandle> handles_;

public:
    AssetManager(IAssetSystem* assets) : assets_(assets) {}

    AssetHandle load(const std::string& name, AssetType type, const std::string& path) {
        auto handle = assets_->registerAsset(type, path);
        handles_[name] = handle;
        assets_->loadAsset(handle);
        return handle;
    }

    AssetHandle get(const std::string& name) const {
        return handles_.at(name);
    }

    template<typename T>
    T* getData(const std::string& name) {
        return assets_->getAsset<T>(handles_.at(name));
    }
};

// Usage
AssetManager assetMgr(sys.assets);
assetMgr.load("player", AssetType::Texture, "textures/player.png");
assetMgr.load("jump", AssetType::Sound, "sounds/jump.wav");

// Later
AssetHandle playerTex = assetMgr.get("player");
```

---

### Lazy Loading

```cpp
class LazyAsset {
    IAssetSystem* assets_;
    AssetHandle handle_;
    bool loaded_ = false;

public:
    LazyAsset(IAssetSystem* assets, AssetType type, const std::string& path)
        : assets_(assets)
    {
        handle_ = assets_->registerAsset(type, path);
    }

    AssetHandle get() {
        if (!loaded_) {
            assets_->loadAsset(handle_);
            loaded_ = true;
        }
        return handle_;
    }
};

// Usage
LazyAsset bossTexture(sys.assets, AssetType::Texture, "textures/boss.png");

// Only loaded when boss spawns
void spawnBoss() {
    AssetHandle tex = bossTexture.get();  // Loads on first use
    // Create boss entity...
}
```

---

### Resource Pools

```cpp
struct TexturePool {
    std::vector<AssetHandle> textures;

    void add(IAssetSystem* assets, const std::string& path) {
        auto handle = assets->registerAsset(AssetType::Texture, path);
        assets->loadAsset(handle);
        textures.push_back(handle);
    }

    AssetHandle getRandom() const {
        return textures[rand() % textures.size()];
    }
};

// Usage
TexturePool coinTextures;
coinTextures.add(sys.assets, "textures/coin1.png");
coinTextures.add(sys.assets, "textures/coin2.png");
coinTextures.add(sys.assets, "textures/coin3.png");

// Random coin
AssetHandle randomCoin = coinTextures.getRandom();
```

---

## File Formats

JFrame supports these file formats out of the box:

**Textures:**
- PNG (recommended)
- JPEG
- BMP
- TGA

**Audio:**
- WAV (uncompressed)
- OGG Vorbis (compressed, recommended for music)
- MP3 (compressed)

**Fonts:**
- TTF (TrueType)
- OTF (OpenType)

**Levels:**
- Lua scripts

---

## Performance Tips

1. **Async load during transitions** - Load next level while showing transition
2. **Unload unused assets** - Free memory when switching levels
3. **Use compressed formats** - OGG for music, PNG for textures
4. **Batch loads** - Load all level assets at once for better throughput
5. **Disable hot-reload in release** - Only enable in development builds

## See Also

- [GraphicsSystem](GraphicsSystem.md) - Using texture assets
- [AudioSystem](AudioSystem.md) - Using sound assets
- [LevelSystem](LevelSystem.md) - Loading level assets
- [BlueprintFactory](BlueprintFactory.md) - Asset references in blueprints
