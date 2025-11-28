# JFrame Assets System

## Overview

The Assets System is JFrame's resource management subsystem responsible for loading, caching, and hot-reloading game assets. It provides a unified interface for managing textures, sounds, music, fonts, levels, data files, shaders, navigation meshes, and behavior trees.

**Key Features:**
- Unified asset registration and loading API
- Synchronous and asynchronous loading
- Thread-safe async operations
- Type-safe asset retrieval
- Automatic hot-reload detection (development builds)
- Memory-efficient unloading
- Bulk operations (load all, unload all)
- Error handling with detailed diagnostics

**Module:** `jframe.assets`
**Implementation Module:** `jframe.assets.impl`
**Interface:** `IAssetSystem`

---

## Asset Types

The system supports nine asset types, each with specialized loading behavior:

| Type | Description | File Formats | Use Case |
|------|-------------|--------------|----------|
| `Texture` | 2D images for sprites and UI | PNG, JPG, BMP, TGA | Graphics rendering |
| `Sound` | Short audio effects | WAV, OGG | Sound effects |
| `Music` | Streaming background music | WAV, OGG, MP3 | Background music |
| `Font` | TrueType/OpenType fonts | TTF, OTF | Text rendering |
| `Level` | Lua level definitions | LUA | Level layouts |
| `Data` | JSON or text configuration | JSON, TXT, CSV | Game config |
| `Shader` | GLSL shader programs | GLSL, VERT, FRAG | Custom rendering |
| `NavMesh` | Navigation mesh data | NAV (binary) | AI pathfinding |
| `BehaviorTree` | AI behavior definitions | JSON, TXT | AI behaviors |

### Asset Type Details

#### Texture
Loaded using `stb_image`. Returns `TextureData`:
```cpp
struct TextureData {
    std::vector<unsigned char> pixels;
    int width = 0;
    int height = 0;
    int channels = 0;  // 1=grayscale, 2=GA, 3=RGB, 4=RGBA
};
```

#### Sound and Music
Loaded as raw file bytes for FMOD consumption. Returns `SoundData`:
```cpp
struct SoundData {
    std::vector<unsigned char> fileData;
    std::string path;
    size_t fileSize = 0;
};
```

#### Font
Loaded as raw TTF/OTF bytes for stb_truetype processing. Returns `FontData`:
```cpp
struct FontData {
    std::vector<unsigned char> fileData;
    std::string path;
    std::size_t fileSize = 0;
};
```

#### Data and Level
Text files loaded as strings. JSON files are automatically parsed. Returns `DataAsset`:
```cpp
struct DataAsset {
    nlohmann::json jsonData;  // Parsed JSON (if isJson is true)
    std::string rawText;      // Original text content
    bool isJson = false;      // True if successfully parsed as JSON
};
```

#### Shader
Shader source code loaded as text. Returns `ShaderData`:
```cpp
struct ShaderData {
    std::string vertexSource;    // Vertex shader (if split files)
    std::string fragmentSource;  // Fragment shader (if split files)
    std::string source;          // Combined source (if single file)
    std::string path;
};
```

#### NavMesh
Binary navigation mesh data. Returns `NavMeshData`:
```cpp
struct NavMeshData {
    std::vector<unsigned char> fileData;
    std::string path;
    size_t fileSize = 0;
};
```

#### BehaviorTree
AI behavior tree definitions. JSON is auto-parsed. Returns `BehaviorTreeData`:
```cpp
struct BehaviorTreeData {
    nlohmann::json treeData;  // Parsed JSON (if isJson is true)
    std::string rawText;      // Original content
    std::string path;
    bool isJson = false;
};
```

---

## Asset Handles and States

### AssetHandle

An asset handle uniquely identifies a registered asset:

```cpp
struct AssetHandle {
    UUID uuid = 0;              // Unique identifier
    AssetType type = AssetType::Data;

    bool isValid() const;
    static AssetHandle invalid();
};
```

**Usage:**
```cpp
AssetHandle playerTexture = assets->registerAsset(AssetType::Texture, "textures/player.png");

if (playerTexture.isValid()) {
    assets->loadAsset(playerTexture);
}
```

### AssetState

Assets transition through states during their lifecycle:

```cpp
enum class AssetState : std::uint8_t {
    Unloaded,  // Registered but not loaded
    Loading,   // Currently being loaded (async)
    Loaded,    // Successfully loaded and ready
    Failed     // Load failed (check error message)
};
```

**State Diagram:**
```
┌──────────┐  registerAsset()  ┌──────────┐
│ (none)   │ ───────────────> │ Unloaded │
└──────────┘                   └──────────┘
                                    │
                    loadAsset()     │ loadAssetAsync()
                           ┌────────┴────────┐
                           ▼                 ▼
                       ┌──────┐        ┌─────────┐
                       │Loaded│ <───── │ Loading │
                       └──────┘        └─────────┘
                           │                 │
                 unload()  │                 │ (on error)
                           ▼                 ▼
                       ┌────────┐       ┌────────┐
                       │Unloaded│       │ Failed │
                       └────────┘       └────────┘
```

---

## Registering Assets

Before loading an asset, you must register it to obtain a handle.

### Basic Registration

```cpp
#include <filesystem>
import jframe;

// Register a texture
AssetHandle playerTexture = assets->registerAsset(
    AssetType::Texture,
    "textures/player.png"
);

// Register with absolute path
AssetHandle bgMusic = assets->registerAsset(
    AssetType::Music,
    std::filesystem::absolute("audio/background.ogg")
);

// Register multiple assets
AssetHandle jumpSound = assets->registerAsset(AssetType::Sound, "sounds/jump.wav");
AssetHandle mainFont = assets->registerAsset(AssetType::Font, "fonts/roboto.ttf");
AssetHandle levelData = assets->registerAsset(AssetType::Level, "levels/level1.lua");
```

### Organizing Asset Handles

```cpp
// Store handles in a struct for easy access
struct GameAssets {
    AssetHandle playerTexture;
    AssetHandle enemyTexture;
    AssetHandle jumpSound;
    AssetHandle levelMusic;
    AssetHandle uiFont;
    AssetHandle level1;
};

GameAssets loadGameAssets(IAssetSystem* assets) {
    GameAssets gameAssets;
    gameAssets.playerTexture = assets->registerAsset(AssetType::Texture, "textures/player.png");
    gameAssets.enemyTexture = assets->registerAsset(AssetType::Texture, "textures/enemy.png");
    gameAssets.jumpSound = assets->registerAsset(AssetType::Sound, "sounds/jump.wav");
    gameAssets.levelMusic = assets->registerAsset(AssetType::Music, "music/level.ogg");
    gameAssets.uiFont = assets->registerAsset(AssetType::Font, "fonts/ui.ttf");
    gameAssets.level1 = assets->registerAsset(AssetType::Level, "levels/level1.lua");
    return gameAssets;
}
```

### Using std::unordered_map

```cpp
// Map asset names to handles
std::unordered_map<std::string, AssetHandle> assetRegistry;

assetRegistry["player"] = assets->registerAsset(AssetType::Texture, "textures/player.png");
assetRegistry["enemy"] = assets->registerAsset(AssetType::Texture, "textures/enemy.png");
assetRegistry["jump"] = assets->registerAsset(AssetType::Sound, "sounds/jump.wav");

// Retrieve by name
AssetHandle playerHandle = assetRegistry["player"];
```

### Unregistering Assets

```cpp
// Unregister when no longer needed
assets->unregisterAsset(playerTexture);

// Unregistering automatically unloads the asset
```

---

## Loading Assets

### Synchronous Loading

Loads immediately on the calling thread. Blocks until complete.

```cpp
// Load a single asset
assets->loadAsset(playerTexture);

// Check if loaded
if (assets->isLoaded(playerTexture)) {
    // Asset is ready to use
}

// Check state
AssetState state = assets->getAssetState(playerTexture);
if (state == AssetState::Loaded) {
    // Success
} else if (state == AssetState::Failed) {
    // Handle error
    AssetMetadata meta = assets->getAssetMetadata(playerTexture);
    if (meta.errorMessage) {
        std::cerr << "Load failed: " << *meta.errorMessage << std::endl;
    }
}
```

**Example: Loading Multiple Assets Sequentially**
```cpp
void loadGameAssets(IAssetSystem* assets, const GameAssets& handles) {
    assets->loadAsset(handles.playerTexture);
    assets->loadAsset(handles.enemyTexture);
    assets->loadAsset(handles.jumpSound);
    assets->loadAsset(handles.levelMusic);
    assets->loadAsset(handles.uiFont);
    assets->loadAsset(handles.level1);
}
```

### Asynchronous Loading

Loads on a background thread. Non-blocking.

```cpp
// Load without callback
assets->loadAssetAsync(playerTexture);

// Poll in game loop
void update() {
    assets->update();  // Process completed async loads

    if (assets->isLoaded(playerTexture)) {
        // Asset is ready
    }
}
```

**With Callback:**
```cpp
assets->loadAssetAsync(playerTexture, [](AssetHandle handle, AssetState state) {
    if (state == AssetState::Loaded) {
        std::cout << "Texture loaded successfully!" << std::endl;
    } else if (state == AssetState::Failed) {
        std::cerr << "Texture load failed!" << std::endl;
    }
});

// Must call update() to invoke callbacks on main thread
assets->update();
```

**Example: Loading Screen with Progress**
```cpp
class LoadingScreen {
public:
    LoadingScreen(IAssetSystem* assets, const std::vector<AssetHandle>& toLoad)
        : assets_(assets), totalAssets_(toLoad.size()), loadedCount_(0) {

        // Start async loading with callbacks
        for (const auto& handle : toLoad) {
            assets_->loadAssetAsync(handle, [this](AssetHandle h, AssetState s) {
                if (s == AssetState::Loaded || s == AssetState::Failed) {
                    loadedCount_++;
                }
            });
        }
    }

    void update() {
        assets_->update();  // Process callbacks
    }

    float getProgress() const {
        return static_cast<float>(loadedCount_) / totalAssets_;
    }

    bool isComplete() const {
        return loadedCount_ >= totalAssets_;
    }

private:
    IAssetSystem* assets_;
    int totalAssets_;
    std::atomic<int> loadedCount_;
};
```

### Bulk Operations

```cpp
// Load all registered assets
assets->loadAll();

// Unload all assets
assets->unloadAll();

// Load all textures
std::vector<AssetHandle> textures = assets->getAssetsOfType(AssetType::Texture);
for (const auto& texture : textures) {
    assets->loadAsset(texture);
}
```

---

## Accessing Asset Data

### Type-Safe Access

```cpp
// Get asset as specific type
const TextureData* texData = assets->getAsset<TextureData>(playerTexture);
if (texData) {
    std::cout << "Texture size: " << texData->width << "x" << texData->height << std::endl;
    std::cout << "Channels: " << texData->channels << std::endl;
}

// Access sound data
const SoundData* soundData = assets->getAsset<SoundData>(jumpSound);
if (soundData) {
    std::cout << "Sound file size: " << soundData->fileSize << " bytes" << std::endl;
}

// Access JSON data
const DataAsset* configData = assets->getAsset<DataAsset>(configHandle);
if (configData && configData->isJson) {
    int windowWidth = configData->jsonData["settings"]["windowWidth"];
    std::string gameName = configData->jsonData["name"];
}
```

### Raw Access

```cpp
// Get raw pointer to std::any
void* rawData = assets->getRawAsset(playerTexture);
if (rawData) {
    auto* anyData = static_cast<std::any*>(rawData);
    if (anyData->type() == typeid(TextureData)) {
        const TextureData& texData = std::any_cast<const TextureData&>(*anyData);
        // Use texData...
    }
}
```

### Complete Example: Using Loaded Assets

```cpp
import jframe;

class Game {
public:
    void init(IAssetSystem* assets) {
        // Register assets
        playerTexture_ = assets->registerAsset(AssetType::Texture, "textures/player.png");
        jumpSound_ = assets->registerAsset(AssetType::Sound, "sounds/jump.wav");
        config_ = assets->registerAsset(AssetType::Data, "config/game.json");

        // Load synchronously
        assets->loadAsset(playerTexture_);
        assets->loadAsset(jumpSound_);
        assets->loadAsset(config_);

        assets_ = assets;
    }

    void render() {
        // Access texture data
        const TextureData* texData = assets_->getAsset<TextureData>(playerTexture_);
        if (texData) {
            // Pass to graphics system for GPU upload
            // graphics->uploadTexture(playerTexture_, texData->pixels.data(),
            //                         texData->width, texData->height, texData->channels);
        }
    }

    void onJump() {
        // Access sound data
        const SoundData* soundData = assets_->getAsset<SoundData>(jumpSound_);
        if (soundData) {
            // Pass to audio system for playback
            // audio->playSound(jumpSound_, soundData->fileData.data(), soundData->fileSize);
        }
    }

    void loadConfig() {
        const DataAsset* configData = assets_->getAsset<DataAsset>(config_);
        if (configData && configData->isJson) {
            difficulty_ = configData->jsonData["difficulty"];
            playerSpeed_ = configData->jsonData["player"]["speed"];
        }
    }

private:
    IAssetSystem* assets_ = nullptr;
    AssetHandle playerTexture_;
    AssetHandle jumpSound_;
    AssetHandle config_;
    std::string difficulty_;
    float playerSpeed_;
};
```

---

## Asset Metadata

Query detailed information about an asset:

```cpp
AssetMetadata meta = assets->getAssetMetadata(playerTexture);

std::cout << "Handle UUID: " << meta.handle.uuid << std::endl;
std::cout << "Source path: " << meta.sourcePath << std::endl;
std::cout << "State: " << static_cast<int>(meta.state) << std::endl;
std::cout << "Size: " << meta.sizeBytes << " bytes" << std::endl;

if (meta.errorMessage) {
    std::cerr << "Error: " << *meta.errorMessage << std::endl;
}
```

**AssetMetadata Structure:**
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

## Hot Reloading

Hot reload automatically detects when asset files change on disk and reloads them.

**Enabling Hot Reload:**
```cpp
#if defined(JFRAME_DEV_TOOLS)
    // Enable only in debug builds
    assets->enableHotReload(true);
#endif
```

**Checking for Changes:**
```cpp
void gameLoop() {
    while (running) {
        // Check for file changes every frame
        assets->checkForReloads();
        assets->update();

        // Rest of game loop...
    }
}
```

**Manual Reload:**
```cpp
// Force reload an asset
assets->reloadAsset(playerTexture);

// Reloading unloads then loads the asset
```

### Hot Reload Example

```cpp
import jframe;

class Editor {
public:
    void init(IAssetSystem* assets) {
        assets_ = assets;

        // Register assets
        shader_ = assets->registerAsset(AssetType::Shader, "shaders/custom.glsl");
        assets->loadAsset(shader_);

        // Enable hot reload for development
        assets->enableHotReload(true);
    }

    void update() {
        // Check for file changes
        assets_->checkForReloads();
        assets_->update();

        // If shader was modified, it's automatically reloaded
        const ShaderData* shaderData = assets_->getAsset<ShaderData>(shader_);
        if (shaderData) {
            // Recompile shader with new source
            // graphics->recompileShader(shader_, shaderData->source);
        }
    }

private:
    IAssetSystem* assets_;
    AssetHandle shader_;
};
```

### Hot Reload Behavior

| Asset Type | Reload Action | Use Case |
|------------|---------------|----------|
| Texture | Reload pixel data, re-upload to GPU | Texture editing |
| Sound/Music | Reload file bytes, recreate FMOD sound | Audio tweaking |
| Shader | Reload source, recompile shader | Shader development |
| Level | Reload Lua script, rebuild level | Level design |
| Data | Reload and re-parse JSON | Config tuning |
| Font | Reload font bytes, rebuild atlas | Font testing |

**Important Notes:**
- Hot reload only works for assets in `Loaded` state
- Assets being loaded asynchronously are skipped
- File deletion doesn't trigger a reload (asset stays loaded)
- Modification detection uses filesystem timestamps (1-second resolution on most systems)

---

## Error Handling

### Checking for Errors

```cpp
assets->loadAsset(playerTexture);

if (assets->getAssetState(playerTexture) == AssetState::Failed) {
    AssetMetadata meta = assets->getAssetMetadata(playerTexture);
    if (meta.errorMessage) {
        std::cerr << "Failed to load " << meta.sourcePath
                  << ": " << *meta.errorMessage << std::endl;
    }
}
```

### Common Error Scenarios

**File Not Found:**
```cpp
AssetHandle missing = assets->registerAsset(AssetType::Texture, "nonexistent.png");
assets->loadAsset(missing);

// State will be Failed
// errorMessage: "Failed to load texture: nonexistent.png - file not found"
```

**Invalid Format:**
```cpp
AssetHandle corrupt = assets->registerAsset(AssetType::Texture, "corrupt.png");
assets->loadAsset(corrupt);

// State will be Failed
// errorMessage: "Failed to load texture: corrupt.png - stb_image: corrupt header"
```

**Unloaded Asset Access:**
```cpp
AssetHandle unloaded = assets->registerAsset(AssetType::Sound, "sound.wav");
// Don't load it

const SoundData* data = assets->getAsset<SoundData>(unloaded);
// data will be nullptr
```

### Robust Loading Pattern

```cpp
Result<TextureData*, std::string> loadTexture(IAssetSystem* assets,
                                               const std::filesystem::path& path) {
    AssetHandle handle = assets->registerAsset(AssetType::Texture, path);
    assets->loadAsset(handle);

    AssetState state = assets->getAssetState(handle);
    if (state == AssetState::Failed) {
        AssetMetadata meta = assets->getAssetMetadata(handle);
        return std::unexpected(
            meta.errorMessage.value_or("Unknown error")
        );
    }

    const TextureData* data = assets->getAsset<TextureData>(handle);
    if (!data) {
        return std::unexpected("Failed to retrieve texture data");
    }

    return data;
}
```

---

## Complete Usage Examples

### Example 1: Loading Game Assets

```cpp
import jframe;
import std;

struct GameAssets {
    // Textures
    AssetHandle playerSprite;
    AssetHandle enemySprite;
    AssetHandle tileSheet;

    // Audio
    AssetHandle jumpSound;
    AssetHandle shootSound;
    AssetHandle backgroundMusic;

    // Fonts
    AssetHandle uiFont;

    // Data
    AssetHandle gameConfig;
    AssetHandle level1;
};

class AssetManager {
public:
    AssetManager(IAssetSystem* assets) : assets_(assets) {}

    GameAssets registerAllAssets() {
        GameAssets assets;

        // Register textures
        assets.playerSprite = assets_->registerAsset(AssetType::Texture, "textures/player.png");
        assets.enemySprite = assets_->registerAsset(AssetType::Texture, "textures/enemy.png");
        assets.tileSheet = assets_->registerAsset(AssetType::Texture, "textures/tiles.png");

        // Register audio
        assets.jumpSound = assets_->registerAsset(AssetType::Sound, "sounds/jump.wav");
        assets.shootSound = assets_->registerAsset(AssetType::Sound, "sounds/shoot.wav");
        assets.backgroundMusic = assets_->registerAsset(AssetType::Music, "music/theme.ogg");

        // Register fonts
        assets.uiFont = assets_->registerAsset(AssetType::Font, "fonts/ui.ttf");

        // Register data
        assets.gameConfig = assets_->registerAsset(AssetType::Data, "config/game.json");
        assets.level1 = assets_->registerAsset(AssetType::Level, "levels/level1.lua");

        return assets;
    }

    void loadAllSync(const GameAssets& assets) {
        // Load everything synchronously
        assets_->loadAsset(assets.playerSprite);
        assets_->loadAsset(assets.enemySprite);
        assets_->loadAsset(assets.tileSheet);
        assets_->loadAsset(assets.jumpSound);
        assets_->loadAsset(assets.shootSound);
        assets_->loadAsset(assets.backgroundMusic);
        assets_->loadAsset(assets.uiFont);
        assets_->loadAsset(assets.gameConfig);
        assets_->loadAsset(assets.level1);
    }

    void loadAllAsync(const GameAssets& assets, std::function<void()> onComplete) {
        std::atomic<int> remaining = 9;

        auto callback = [&remaining, onComplete](AssetHandle, AssetState) {
            if (--remaining == 0) {
                onComplete();
            }
        };

        assets_->loadAssetAsync(assets.playerSprite, callback);
        assets_->loadAssetAsync(assets.enemySprite, callback);
        assets_->loadAssetAsync(assets.tileSheet, callback);
        assets_->loadAssetAsync(assets.jumpSound, callback);
        assets_->loadAssetAsync(assets.shootSound, callback);
        assets_->loadAssetAsync(assets.backgroundMusic, callback);
        assets_->loadAssetAsync(assets.uiFont, callback);
        assets_->loadAssetAsync(assets.gameConfig, callback);
        assets_->loadAssetAsync(assets.level1, callback);
    }

private:
    IAssetSystem* assets_;
};
```

### Example 2: Texture Atlas System

```cpp
import jframe;

class TextureAtlas {
public:
    TextureAtlas(IAssetSystem* assets, const std::filesystem::path& path)
        : assets_(assets) {
        atlasHandle_ = assets->registerAsset(AssetType::Texture, path);
        assets->loadAsset(atlasHandle_);
    }

    const TextureData* getAtlasData() const {
        return assets_->getAsset<TextureData>(atlasHandle_);
    }

    // Define sprite regions within the atlas
    void defineSprite(const std::string& name, int x, int y, int width, int height) {
        sprites_[name] = {x, y, width, height};
    }

    struct SpriteRegion {
        int x, y, width, height;
    };

    std::optional<SpriteRegion> getSprite(const std::string& name) const {
        auto it = sprites_.find(name);
        if (it != sprites_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

private:
    IAssetSystem* assets_;
    AssetHandle atlasHandle_;
    std::unordered_map<std::string, SpriteRegion> sprites_;
};
```

### Example 3: Localization System

```cpp
import jframe;

class LocalizationManager {
public:
    LocalizationManager(IAssetSystem* assets) : assets_(assets) {}

    void loadLanguage(const std::string& lang) {
        currentLang_ = lang;

        std::string path = std::format("localization/{}.json", lang);
        langHandle_ = assets_->registerAsset(AssetType::Data, path);
        assets_->loadAsset(langHandle_);

        // Cache strings
        const DataAsset* data = assets_->getAsset<DataAsset>(langHandle_);
        if (data && data->isJson) {
            strings_.clear();
            for (auto& [key, value] : data->jsonData.items()) {
                strings_[key] = value.get<std::string>();
            }
        }
    }

    std::string getString(const std::string& key) const {
        auto it = strings_.find(key);
        return it != strings_.end() ? it->second : key;
    }

private:
    IAssetSystem* assets_;
    AssetHandle langHandle_;
    std::string currentLang_;
    std::unordered_map<std::string, std::string> strings_;
};
```

### Example 4: Shader Manager with Hot Reload

```cpp
import jframe;

class ShaderManager {
public:
    ShaderManager(IAssetSystem* assets, IGraphicsSystem* graphics)
        : assets_(assets), graphics_(graphics) {
        assets_->enableHotReload(true);
    }

    void registerShader(const std::string& name, const std::filesystem::path& path) {
        AssetHandle handle = assets_->registerAsset(AssetType::Shader, path);
        assets_->loadAsset(handle);

        shaders_[name] = handle;
        compileShader(name);
    }

    void update() {
        assets_->checkForReloads();
        assets_->update();

        // Check if any shaders were reloaded
        for (const auto& [name, handle] : shaders_) {
            // If hot reloaded, recompile
            if (needsRecompile_[name]) {
                compileShader(name);
                needsRecompile_[name] = false;
            }
        }
    }

private:
    void compileShader(const std::string& name) {
        AssetHandle handle = shaders_[name];
        const ShaderData* data = assets_->getAsset<ShaderData>(handle);

        if (data) {
            // Compile shader using graphics system
            // bool success = graphics_->compileShader(name, data->source);
            // if (!success) {
            //     std::cerr << "Shader compilation failed: " << name << std::endl;
            // }

            needsRecompile_[name] = true;  // Mark for next hot reload
        }
    }

    IAssetSystem* assets_;
    IGraphicsSystem* graphics_;
    std::unordered_map<std::string, AssetHandle> shaders_;
    std::unordered_map<std::string, bool> needsRecompile_;
};
```

---

## Thread Safety

The Assets System is designed for safe multi-threaded access:

### Thread-Safe Operations
- `loadAssetAsync()` - Launches background threads
- `update()` - Processes completed async loads
- `getAssetState()` - Read state from any thread
- `getAssetMetadata()` - Read metadata from any thread
- `getRawAsset()` - Access loaded data from any thread

### Main Thread Only
- `loadAsset()` - Blocking, should run on main thread
- `unloadAsset()` - Modifies state, main thread only
- `unregisterAsset()` - Modifies registry, main thread only

### Async Loading Pattern
```cpp
// Game thread
void GameState::loadLevel() {
    assets->loadAssetAsync(levelHandle, [this](AssetHandle h, AssetState s) {
        // This callback runs on MAIN THREAD after update()
        if (s == AssetState::Loaded) {
            onLevelLoaded();
        }
    });
}

// Main loop
void mainLoop() {
    while (running) {
        assets->update();  // Process async results and invoke callbacks
        game->update(deltaTime);
        graphics->render();
    }
}
```

---

## Performance Considerations

### Memory Management

**Asset Size:**
- Textures: Width × Height × Channels bytes (e.g., 1024×1024 RGBA = 4 MB)
- Sounds: File size (WAV can be large, OGG compressed)
- Music: File size (stream from disk if possible)
- Fonts: ~50-500 KB
- Data/Levels: Usually < 100 KB

**Recommendations:**
- Unload unused assets: `assets->unloadAsset(handle)`
- Use `getAssetsOfType()` to bulk unload: `for (auto& h : assets->getAssetsOfType(AssetType::Texture)) { assets->unloadAsset(h); }`
- Load levels on-demand, unload when switching
- Keep UI assets loaded throughout the game

### Loading Strategy

**Synchronous vs. Asynchronous:**
- Sync: Simple, blocks until complete, use for small assets or initial load
- Async: Complex, non-blocking, use for large assets or streaming

**Best Practices:**
- Show loading screen during synchronous batch loads
- Use async for background streaming (next level while playing current)
- Combine: Load critical assets sync, non-critical async

**Batch Loading:**
```cpp
// Group assets by priority
std::vector<AssetHandle> criticalAssets = {playerSprite, uiFont};
std::vector<AssetHandle> normalAssets = {backgroundTexture, ambientSound};

// Load critical first (sync)
for (auto& h : criticalAssets) {
    assets->loadAsset(h);
}

// Load normal in background (async)
for (auto& h : normalAssets) {
    assets->loadAssetAsync(h);
}
```

---

## Integration with Other Systems

### Graphics System

```cpp
// Load texture
AssetHandle texHandle = assets->registerAsset(AssetType::Texture, "player.png");
assets->loadAsset(texHandle);

// Upload to GPU
const TextureData* texData = assets->getAsset<TextureData>(texHandle);
if (texData) {
    graphics->uploadTexture(texHandle, texData->pixels.data(),
                           texData->width, texData->height, texData->channels);
}

// Use in sprite rendering
Sprite sprite;
sprite.textureHandle = &texHandle;
graphics->drawSprite(sprite);
```

### Audio System

```cpp
// Load sound
AssetHandle soundHandle = assets->registerAsset(AssetType::Sound, "jump.wav");
assets->loadAsset(soundHandle);

// Create FMOD sound
const SoundData* soundData = assets->getAsset<SoundData>(soundHandle);
if (soundData) {
    audio->createSound(soundHandle, soundData->fileData.data(), soundData->fileSize);
}

// Play sound
audio->playSound(soundHandle);
```

### Level System

```cpp
// Load level
AssetHandle levelHandle = assets->registerAsset(AssetType::Level, "levels/level1.lua");
assets->loadAsset(levelHandle);

// Parse level
const DataAsset* levelData = assets->getAsset<DataAsset>(levelHandle);
if (levelData) {
    // Execute Lua script
    levels->loadLevelFromLua(levelData->rawText);
}
```

---

## Best Practices

1. **Always Check Load Status**
   ```cpp
   assets->loadAsset(handle);
   if (assets->getAssetState(handle) == AssetState::Failed) {
       // Handle error
   }
   ```

2. **Use AssetHandle Structs for Organization**
   ```cpp
   struct LevelAssets {
       AssetHandle background;
       AssetHandle tileset;
       AssetHandle music;
   };
   ```

3. **Unload Assets Between Levels**
   ```cpp
   void Level::unload() {
       assets->unloadAsset(backgroundTexture);
       assets->unloadAsset(levelMusic);
   }
   ```

4. **Enable Hot Reload in Development Only**
   ```cpp
   #if defined(JFRAME_DEV_TOOLS)
       assets->enableHotReload(true);
   #endif
   ```

5. **Use Async for Large Assets**
   ```cpp
   // Large music files should load async
   assets->loadAssetAsync(musicHandle, [](AssetHandle h, AssetState s) {
       if (s == AssetState::Loaded) {
           audio->playMusic(h);
       }
   });
   ```

6. **Validate Asset Pointers**
   ```cpp
   const TextureData* data = assets->getAsset<TextureData>(handle);
   if (!data) {
       return;  // Asset not loaded or wrong type
   }
   ```

7. **Use Metadata for Debugging**
   ```cpp
   void printAssetInfo(IAssetSystem* assets, AssetHandle handle) {
       AssetMetadata meta = assets->getAssetMetadata(handle);
       std::cout << "Asset: " << meta.sourcePath << "\n"
                 << "State: " << static_cast<int>(meta.state) << "\n"
                 << "Size: " << meta.sizeBytes << " bytes\n";
       if (meta.errorMessage) {
           std::cout << "Error: " << *meta.errorMessage << "\n";
       }
   }
   ```

---

## Common Pitfalls

### 1. Forgetting to Call update()
```cpp
// WRONG: Async callbacks never invoked
assets->loadAssetAsync(handle, callback);
// Missing: assets->update();

// CORRECT:
assets->loadAssetAsync(handle, callback);
// In main loop:
while (running) {
    assets->update();  // Process async results
    // ...
}
```

### 2. Accessing Unloaded Assets
```cpp
// WRONG: Asset not loaded
AssetHandle handle = assets->registerAsset(AssetType::Texture, "tex.png");
const TextureData* data = assets->getAsset<TextureData>(handle);  // nullptr!

// CORRECT:
AssetHandle handle = assets->registerAsset(AssetType::Texture, "tex.png");
assets->loadAsset(handle);
if (assets->isLoaded(handle)) {
    const TextureData* data = assets->getAsset<TextureData>(handle);
}
```

### 3. Type Mismatch
```cpp
// WRONG: Requesting wrong type
AssetHandle sound = assets->registerAsset(AssetType::Sound, "jump.wav");
assets->loadAsset(sound);
const TextureData* data = assets->getAsset<TextureData>(sound);  // nullptr!

// CORRECT:
const SoundData* data = assets->getAsset<SoundData>(sound);
```

### 4. Memory Leaks (Handles Never Unregistered)
```cpp
// WRONG: Temporary handles leak
void tempFunction() {
    AssetHandle temp = assets->registerAsset(AssetType::Texture, "temp.png");
    assets->loadAsset(temp);
    // temp never unregistered, stays in memory forever
}

// CORRECT:
void tempFunction() {
    AssetHandle temp = assets->registerAsset(AssetType::Texture, "temp.png");
    assets->loadAsset(temp);
    // Use it...
    assets->unregisterAsset(temp);  // Clean up
}
```

---

## Debugging

### Print All Assets
```cpp
void debugPrintAssets(IAssetSystem* assets) {
    std::vector<AssetType> types = {
        AssetType::Texture, AssetType::Sound, AssetType::Music,
        AssetType::Font, AssetType::Level, AssetType::Data,
        AssetType::Shader, AssetType::NavMesh, AssetType::BehaviorTree
    };

    for (AssetType type : types) {
        std::vector<AssetHandle> assetHandles = assets->getAssetsOfType(type);
        if (!assetHandles.empty()) {
            std::cout << "=== " << static_cast<int>(type) << " ===" << std::endl;
            for (const auto& handle : assetHandles) {
                AssetMetadata meta = assets->getAssetMetadata(handle);
                std::cout << "  " << meta.sourcePath
                          << " [" << static_cast<int>(meta.state) << "]"
                          << " (" << meta.sizeBytes << " bytes)" << std::endl;
            }
        }
    }
}
```

### Asset Memory Usage
```cpp
size_t calculateTotalMemoryUsage(IAssetSystem* assets) {
    size_t total = 0;

    std::vector<AssetType> types = {
        AssetType::Texture, AssetType::Sound, AssetType::Music,
        AssetType::Font, AssetType::Level, AssetType::Data,
        AssetType::Shader, AssetType::NavMesh, AssetType::BehaviorTree
    };

    for (AssetType type : types) {
        for (const auto& handle : assets->getAssetsOfType(type)) {
            if (assets->isLoaded(handle)) {
                AssetMetadata meta = assets->getAssetMetadata(handle);
                total += meta.sizeBytes;
            }
        }
    }

    return total;
}
```

---

## FAQ

**Q: Can I register the same file multiple times?**
A: Yes. Each registration returns a unique handle. Both handles reference the same file but load independently.

**Q: What happens if I unregister an asset that's still loaded?**
A: The asset is automatically unloaded, then removed from the registry. The handle becomes invalid.

**Q: Can I load assets from memory instead of files?**
A: Not currently. The system requires filesystem paths. Consider writing memory to a temp file if needed.

**Q: How do I detect when an async load completes without a callback?**
A: Poll `getAssetState()` or `isLoaded()` after calling `update()` each frame.

**Q: Why is my callback not being invoked?**
A: You must call `assets->update()` in your main loop to process completed async loads and invoke callbacks.

**Q: Can I access assets from multiple threads?**
A: Yes, `getAssetState()`, `getAssetMetadata()`, and `getRawAsset()` are thread-safe. Loading/unloading should be done on the main thread.

**Q: How do I reload an asset?**
A: Call `assets->reloadAsset(handle)` or enable hot reload with `assets->checkForReloads()`.

**Q: What's the difference between Data and Level asset types?**
A: Both load text files. Level is semantically intended for Lua scripts, while Data is for JSON/config. Functionally identical.

**Q: Can I load compressed textures (DDS, KTX)?**
A: Not directly. `stb_image` supports PNG/JPG/BMP/TGA. Extend `loadAssetImpl()` to support additional formats.

**Q: How do I stream large audio files?**
A: Load as Music type. The AudioSystem should stream from disk rather than loading the entire file into memory.

---

## See Also

- [Graphics System Documentation](./Graphics-System.md) - Texture upload and rendering
- [Audio System Documentation](./Audio-System.md) - Sound playback
- [Level System Documentation](./Level-System.md) - Level loading from Lua
- [JFrame Technical Design](../jframe-technical-design.md) - Overall architecture
- [Project Status](../PROJECT-STATUS.md) - Implementation progress
