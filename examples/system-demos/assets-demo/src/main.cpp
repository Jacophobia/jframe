// assets-demo/src/main.cpp
// Comprehensive demonstration of the IAssetSystem interface

#include <filesystem>
#include <format>
#include <optional>
#include <print>
#include <string_view>
#include <vector>

import jframe.types;
import jframe.assets;

using namespace jframe;

// Helper function to print section headers
void printSection(std::string_view title) {
    std::println("\n{:=<80}", "");
    std::println("  {}", title);
    std::println("{:=<80}\n", "");
}

// Helper to convert AssetType to string
std::string_view assetTypeToString(AssetType type) {
    switch (type) {
        case AssetType::Texture: return "Texture";
        case AssetType::Sound: return "Sound";
        case AssetType::Music: return "Music";
        case AssetType::Font: return "Font";
        case AssetType::Level: return "Level";
        case AssetType::Data: return "Data";
        case AssetType::Shader: return "Shader";
        case AssetType::NavMesh: return "NavMesh";
        case AssetType::BehaviorTree: return "BehaviorTree";
    }
    return "Unknown";
}

// Helper to convert AssetState to string
std::string_view assetStateToString(AssetState state) {
    switch (state) {
        case AssetState::Unloaded: return "Unloaded";
        case AssetState::Loading: return "Loading";
        case AssetState::Loaded: return "Loaded";
        case AssetState::Failed: return "Failed";
    }
    return "Unknown";
}

// Async loading callback
void onAssetLoaded(AssetHandle handle, AssetState state) {
    std::println("  Async callback: Asset {} transitioned to state '{}'",
                 handle.uuid, assetStateToString(state));
}

// Demo function: Registration
void demoRegistration(IAssetSystem& assets) {
    printSection("1. Asset Registration");

    std::println("Registering assets of various types...\n");

    // Register different asset types
    auto textureHandle = assets.registerAsset(AssetType::Texture, "textures/player.png");
    std::println("Registered Texture: UUID={}, Path='textures/player.png'", textureHandle.uuid);

    auto soundHandle = assets.registerAsset(AssetType::Sound, "audio/jump.wav");
    std::println("Registered Sound: UUID={}, Path='audio/jump.wav'", soundHandle.uuid);

    auto musicHandle = assets.registerAsset(AssetType::Music, "music/level1.ogg");
    std::println("Registered Music: UUID={}, Path='music/level1.ogg'", musicHandle.uuid);

    auto fontHandle = assets.registerAsset(AssetType::Font, "fonts/arial.ttf");
    std::println("Registered Font: UUID={}, Path='fonts/arial.ttf'", fontHandle.uuid);

    auto levelHandle = assets.registerAsset(AssetType::Level, "levels/tutorial.lua");
    std::println("Registered Level: UUID={}, Path='levels/tutorial.lua'", levelHandle.uuid);

    auto dataHandle = assets.registerAsset(AssetType::Data, "data/config.json");
    std::println("Registered Data: UUID={}, Path='data/config.json'", dataHandle.uuid);

    auto shaderHandle = assets.registerAsset(AssetType::Shader, "shaders/sprite.glsl");
    std::println("Registered Shader: UUID={}, Path='shaders/sprite.glsl'", shaderHandle.uuid);

    auto navmeshHandle = assets.registerAsset(AssetType::NavMesh, "navmesh/level1.nav");
    std::println("Registered NavMesh: UUID={}, Path='navmesh/level1.nav'", navmeshHandle.uuid);

    auto behaviorHandle = assets.registerAsset(AssetType::BehaviorTree, "ai/patrol.xml");
    std::println("Registered BehaviorTree: UUID={}, Path='ai/patrol.xml'", behaviorHandle.uuid);

    std::println("\nAll asset types registered successfully!");
}

// Demo function: State Queries
void demoStateQueries(IAssetSystem& assets, AssetHandle handle) {
    printSection("2. Asset State Queries");

    std::println("Querying state for asset {}...\n", handle.uuid);

    // Get state
    AssetState state = assets.getAssetState(handle);
    std::println("Asset State: {}", assetStateToString(state));

    // Check if loaded
    bool loaded = assets.isLoaded(handle);
    std::println("Is Loaded: {}", loaded ? "true" : "false");

    // Get metadata
    AssetMetadata metadata = assets.getAssetMetadata(handle);
    std::println("\nAsset Metadata:");
    std::println("  UUID: {}", metadata.handle.uuid);
    std::println("  Type: {}", assetTypeToString(metadata.handle.type));
    std::println("  Path: {}", metadata.sourcePath.string());
    std::println("  State: {}", assetStateToString(metadata.state));
    std::println("  Size: {} bytes", metadata.sizeBytes);

    if (metadata.errorMessage) {
        std::println("  Error: {}", *metadata.errorMessage);
    } else {
        std::println("  Error: None");
    }
}

// Demo function: Synchronous Loading
void demoSyncLoading(IAssetSystem& assets) {
    printSection("3. Synchronous Loading");

    std::println("Loading texture synchronously...\n");

    auto handle = assets.registerAsset(AssetType::Texture, "textures/enemy.png");
    std::println("Before load - State: {}", assetStateToString(assets.getAssetState(handle)));

    // Load synchronously
    assets.loadAsset(handle);

    std::println("After load - State: {}", assetStateToString(assets.getAssetState(handle)));
    std::println("Is Loaded: {}", assets.isLoaded(handle) ? "true" : "false");

    // Try to access raw asset data
    const void* rawData = assets.getRawAsset(handle);
    if (rawData != nullptr) {
        std::println("Raw asset data pointer: {}", rawData);

        // Cast to TextureData (asset system should store this type for textures)
        const TextureData* texData = assets.getAsset<TextureData>(handle);
        if (texData != nullptr) {
            std::println("TextureData details:");
            std::println("  Width: {}", texData->width);
            std::println("  Height: {}", texData->height);
            std::println("  Channels: {}", texData->channels);
            std::println("  Pixel count: {}", texData->pixels.size());
        }
    } else {
        std::println("Note: Asset failed to load (expected if file doesn't exist)");
    }
}

// Demo function: Asynchronous Loading
void demoAsyncLoading(IAssetSystem& assets) {
    printSection("4. Asynchronous Loading");

    std::println("Loading multiple assets asynchronously...\n");

    // Register assets
    auto sound1 = assets.registerAsset(AssetType::Sound, "audio/coin.wav");
    auto sound2 = assets.registerAsset(AssetType::Sound, "audio/damage.wav");
    auto music = assets.registerAsset(AssetType::Music, "music/boss.ogg");

    std::println("Starting async loads...");

    // Load asynchronously with callbacks
    assets.loadAssetAsync(sound1, onAssetLoaded);
    assets.loadAssetAsync(sound2, onAssetLoaded);
    assets.loadAssetAsync(music, [](AssetHandle h, AssetState s) {
        std::println("  Music loaded: UUID={}, State={}", h.uuid, assetStateToString(s));
    });

    std::println("Async loads initiated. Calling update() to process...\n");

    // Simulate update loop to process async loads
    for (int i = 0; i < 5; ++i) {
        std::println("Update cycle {}...", i + 1);
        assets.update();

        // Check states
        std::println("  Sound1: {}", assetStateToString(assets.getAssetState(sound1)));
        std::println("  Sound2: {}", assetStateToString(assets.getAssetState(sound2)));
        std::println("  Music: {}", assetStateToString(assets.getAssetState(music)));
    }
}

// Demo function: Bulk Operations
void demoBulkOperations(IAssetSystem& assets) {
    printSection("5. Bulk Operations");

    std::println("Registering multiple assets for bulk operations...\n");

    // Register several textures
    auto tex1 = assets.registerAsset(AssetType::Texture, "textures/tile1.png");
    auto tex2 = assets.registerAsset(AssetType::Texture, "textures/tile2.png");
    auto tex3 = assets.registerAsset(AssetType::Texture, "textures/tile3.png");

    // Register some sounds
    auto snd1 = assets.registerAsset(AssetType::Sound, "audio/footstep1.wav");
    auto snd2 = assets.registerAsset(AssetType::Sound, "audio/footstep2.wav");

    std::println("Getting all Texture assets...");
    auto textures = assets.getAssetsOfType(AssetType::Texture);
    std::println("Found {} texture assets:", textures.size());
    for (const auto& handle : textures) {
        auto metadata = assets.getAssetMetadata(handle);
        std::println("  - UUID={}, Path={}", handle.uuid, metadata.sourcePath.string());
    }

    std::println("\nGetting all Sound assets...");
    auto sounds = assets.getAssetsOfType(AssetType::Sound);
    std::println("Found {} sound assets:", sounds.size());
    for (const auto& handle : sounds) {
        auto metadata = assets.getAssetMetadata(handle);
        std::println("  - UUID={}, Path={}", handle.uuid, metadata.sourcePath.string());
    }

    std::println("\nLoading all registered assets...");
    assets.loadAll();
    assets.update(); // Process loads

    std::println("All assets loaded. States:");
    for (const auto& handle : textures) {
        std::println("  Texture {}: {}", handle.uuid, assetStateToString(assets.getAssetState(handle)));
    }
    for (const auto& handle : sounds) {
        std::println("  Sound {}: {}", handle.uuid, assetStateToString(assets.getAssetState(handle)));
    }

    std::println("\nUnloading all assets...");
    assets.unloadAll();

    std::println("All assets unloaded. States:");
    for (const auto& handle : textures) {
        std::println("  Texture {}: {}", handle.uuid, assetStateToString(assets.getAssetState(handle)));
    }
}

// Demo function: Asset Lifecycle
void demoAssetLifecycle(IAssetSystem& assets) {
    printSection("6. Asset Lifecycle (Register -> Load -> Unload -> Unregister)");

    std::println("Demonstrating full asset lifecycle...\n");

    // 1. Register
    std::println("Step 1: Register asset");
    auto handle = assets.registerAsset(AssetType::Font, "fonts/courier.ttf");
    std::println("  Registered: UUID={}, State={}",
                 handle.uuid, assetStateToString(assets.getAssetState(handle)));

    // 2. Load
    std::println("\nStep 2: Load asset");
    assets.loadAsset(handle);
    assets.update();
    std::println("  Loaded: State={}, IsLoaded={}",
                 assetStateToString(assets.getAssetState(handle)),
                 assets.isLoaded(handle) ? "true" : "false");

    // 3. Use (access data)
    std::println("\nStep 3: Access asset data");
    const FontData* fontData = assets.getAsset<FontData>(handle);
    if (fontData != nullptr) {
        std::println("  FontData: Path={}, Size={} bytes",
                     fontData->path, fontData->fileSize);
    } else {
        std::println("  Note: Font data unavailable (expected if file doesn't exist)");
    }

    // 4. Unload
    std::println("\nStep 4: Unload asset");
    assets.unloadAsset(handle);
    std::println("  Unloaded: State={}, IsLoaded={}",
                 assetStateToString(assets.getAssetState(handle)),
                 assets.isLoaded(handle) ? "true" : "false");

    // 5. Unregister
    std::println("\nStep 5: Unregister asset");
    assets.unregisterAsset(handle);
    std::println("  Asset completely removed from registry");

    // Trying to query now should fail gracefully
    auto state = assets.getAssetState(handle);
    std::println("  Query after unregister: State={}", assetStateToString(state));
}

// Demo function: Hot Reload
void demoHotReload(IAssetSystem& assets) {
    printSection("7. Hot Reload (Development Feature)");

    std::println("Hot reload allows runtime asset updates during development.\n");

    // Register an asset
    auto handle = assets.registerAsset(AssetType::Texture, "textures/ui_button.png");
    assets.loadAsset(handle);
    assets.update();

    std::println("Asset loaded: UUID={}", handle.uuid);

    // Enable hot reload
    std::println("\nEnabling hot reload...");
    assets.enableHotReload(true);
    std::println("Hot reload enabled. File watching active.");

    std::println("\nSimulating file change detection...");
    std::println("(In real scenario, file watcher would detect texture modification)");

    // Check for reloads (would normally happen in update loop)
    std::println("\nCalling checkForReloads()...");
    assets.checkForReloads();

    // Manually reload an asset
    std::println("\nManually reloading asset {}...", handle.uuid);
    assets.reloadAsset(handle);
    assets.update();
    std::println("Asset reloaded: State={}", assetStateToString(assets.getAssetState(handle)));

    // Disable hot reload
    std::println("\nDisabling hot reload...");
    assets.enableHotReload(false);
    std::println("Hot reload disabled. File watching stopped.");
}

// Demo function: Error Handling
void demoErrorHandling(IAssetSystem& assets) {
    printSection("8. Error Handling & Failed States");

    std::println("Demonstrating error scenarios...\n");

    // Try to load non-existent file
    std::println("Loading non-existent file...");
    auto badHandle = assets.registerAsset(AssetType::Texture, "textures/does_not_exist.png");
    assets.loadAsset(badHandle);
    assets.update();

    auto state = assets.getAssetState(badHandle);
    std::println("Asset state: {}", assetStateToString(state));

    auto metadata = assets.getAssetMetadata(badHandle);
    if (metadata.errorMessage) {
        std::println("Error message: {}", *metadata.errorMessage);
    }

    // Query invalid handle
    std::println("\nQuerying invalid asset handle...");
    AssetHandle invalid = AssetHandle::invalid();
    std::println("Invalid handle valid: {}", invalid.isValid() ? "true" : "false");
    auto invalidState = assets.getAssetState(invalid);
    std::println("Invalid handle state: {}", assetStateToString(invalidState));
}

// Demo function: Data Structure Details
void demoDataStructures(IAssetSystem& assets) {
    printSection("9. Asset Data Structures");

    std::println("Demonstrating asset-specific data structures...\n");

    // Texture Data
    std::println("TextureData structure:");
    std::println("  - pixels: std::vector<unsigned char>");
    std::println("  - width: int");
    std::println("  - height: int");
    std::println("  - channels: int (RGBA = 4, RGB = 3)");

    auto texHandle = assets.registerAsset(AssetType::Texture, "textures/demo.png");
    assets.loadAsset(texHandle);
    assets.update();

    const TextureData* texData = assets.getAsset<TextureData>(texHandle);
    if (texData) {
        std::println("  Example: {}x{}, {} channels, {} bytes",
                     texData->width, texData->height, texData->channels,
                     texData->pixels.size());
    }

    // Font Data
    std::println("\nFontData structure:");
    std::println("  - fileData: std::vector<unsigned char> (raw TTF/OTF bytes)");
    std::println("  - path: std::string");
    std::println("  - fileSize: std::size_t");

    auto fontHandle = assets.registerAsset(AssetType::Font, "fonts/demo.ttf");
    assets.loadAsset(fontHandle);
    assets.update();

    const FontData* fontData = assets.getAsset<FontData>(fontHandle);
    if (fontData) {
        std::println("  Example: path='{}', size={} bytes",
                     fontData->path, fontData->fileSize);
    }

    // Sound Data
    std::println("\nSoundData structure:");
    std::println("  - fileData: std::vector<unsigned char> (raw WAV/OGG/MP3 bytes)");
    std::println("  - path: std::string");
    std::println("  - fileSize: std::size_t");

    auto soundHandle = assets.registerAsset(AssetType::Sound, "audio/demo.wav");
    assets.loadAsset(soundHandle);
    assets.update();

    const SoundData* soundData = assets.getAsset<SoundData>(soundHandle);
    if (soundData) {
        std::println("  Example: path='{}', size={} bytes",
                     soundData->path, soundData->fileSize);
    }

    // AssetMetadata
    std::println("\nAssetMetadata structure:");
    std::println("  - handle: AssetHandle");
    std::println("  - sourcePath: std::filesystem::path");
    std::println("  - state: AssetState");
    std::println("  - sizeBytes: std::size_t");
    std::println("  - errorMessage: std::optional<std::string>");
}

// Demo function: Advanced Patterns
void demoAdvancedPatterns(IAssetSystem& assets) {
    printSection("10. Advanced Usage Patterns");

    std::println("Demonstrating common usage patterns...\n");

    // Pattern 1: Preload essential assets
    std::println("Pattern 1: Preload essential assets at startup");
    std::vector<AssetHandle> essentialAssets;
    essentialAssets.push_back(assets.registerAsset(AssetType::Texture, "ui/loading.png"));
    essentialAssets.push_back(assets.registerAsset(AssetType::Font, "fonts/main.ttf"));
    essentialAssets.push_back(assets.registerAsset(AssetType::Sound, "audio/ui_click.wav"));

    for (const auto& handle : essentialAssets) {
        assets.loadAsset(handle);
    }
    assets.update();
    std::println("  {} essential assets preloaded", essentialAssets.size());

    // Pattern 2: Lazy load level-specific assets
    std::println("\nPattern 2: Lazy load level-specific assets");
    auto levelAsset = assets.registerAsset(AssetType::Level, "levels/level2.lua");
    std::println("  Level registered but not loaded (State: {})",
                 assetStateToString(assets.getAssetState(levelAsset)));

    std::println("  Player enters level - loading now...");
    assets.loadAsset(levelAsset);
    assets.update();
    std::println("  Level loaded (State: {})",
                 assetStateToString(assets.getAssetState(levelAsset)));

    // Pattern 3: Stream large assets asynchronously
    std::println("\nPattern 3: Stream large music files asynchronously");
    auto bgMusic = assets.registerAsset(AssetType::Music, "music/background.ogg");

    bool musicReady = false;
    assets.loadAssetAsync(bgMusic, [&musicReady](AssetHandle h, AssetState s) {
        if (s == AssetState::Loaded) {
            musicReady = true;
            std::println("  Background music ready to play!");
        }
    });

    // Simulate update loop
    int attempts = 0;
    while (!musicReady && attempts < 5) {
        assets.update();
        ++attempts;
    }

    // Pattern 4: Conditional loading based on settings
    std::println("\nPattern 4: Conditional loading (high quality vs low quality)");
    bool highQuality = true; // Would come from settings

    AssetHandle graphicsAsset;
    if (highQuality) {
        graphicsAsset = assets.registerAsset(AssetType::Texture, "textures/highres.png");
        std::println("  Using high quality assets");
    } else {
        graphicsAsset = assets.registerAsset(AssetType::Texture, "textures/lowres.png");
        std::println("  Using low quality assets");
    }
    assets.loadAsset(graphicsAsset);
    assets.update();

    // Pattern 5: Unload unused assets to free memory
    std::println("\nPattern 5: Unload unused level assets when transitioning");
    std::println("  Unloading level2 assets...");
    assets.unloadAsset(levelAsset);
    std::println("  Memory freed for next level");
}

int main() {
    std::println("\n");
    std::println("{:=<80}", "");
    std::println("  JFrame Assets System - Comprehensive Demo");
    std::println("{:=<80}", "");
    std::println("\nThis demo exercises ALL IAssetSystem interface methods.");
    std::println("Note: Asset files don't need to exist - demo shows API usage.\n");

    // NOTE: In a real application, you would inject the IAssetSystem
    // via dependency injection. For this demo, we assume it's provided.
    // Example: auto assets = injector.get<IAssetSystem*>();

    // Since we're demonstrating the API without a real implementation here,
    // we'll create a mock note
    std::println("NOTE: This demo requires linking against jframe-assets implementation.");
    std::println("      Build the full project to see the system in action.\n");

    // The following code shows how you would use the asset system:
    /*
    IAssetSystem* assets = // ... obtain from DI container

    demoRegistration(*assets);

    // Demonstrate state queries on the first registered asset
    auto firstAsset = assets->getAssetsOfType(AssetType::Texture)[0];
    demoStateQueries(*assets, firstAsset);

    demoSyncLoading(*assets);
    demoAsyncLoading(*assets);
    demoBulkOperations(*assets);
    demoAssetLifecycle(*assets);
    demoHotReload(*assets);
    demoErrorHandling(*assets);
    demoDataStructures(*assets);
    demoAdvancedPatterns(*assets);
    */

    // Summary of demonstrated features
    printSection("Demo Summary");

    std::println("This demo demonstrated:");
    std::println("\n1. LIFECYCLE");
    std::println("   - update() for processing async operations");

    std::println("\n2. REGISTRATION");
    std::println("   - registerAsset() for all AssetType variants:");
    std::println("     * Texture, Sound, Music, Font, Level, Data");
    std::println("     * Shader, NavMesh, BehaviorTree");
    std::println("   - unregisterAsset() to remove from registry");

    std::println("\n3. LOADING");
    std::println("   - loadAsset() for synchronous loading");
    std::println("   - loadAssetAsync() with callbacks");
    std::println("   - unloadAsset() to free memory");

    std::println("\n4. STATE QUERIES");
    std::println("   - getAssetState() returns AssetState enum");
    std::println("   - isLoaded() boolean check");
    std::println("   - getAssetMetadata() for detailed info");

    std::println("\n5. DATA ACCESS");
    std::println("   - getRawAsset() for void* pointer");
    std::println("   - getAsset<T>() templated type-safe access");
    std::println("   - TextureData, FontData, SoundData structures");

    std::println("\n6. BULK OPERATIONS");
    std::println("   - loadAll() to load everything");
    std::println("   - unloadAll() to free all memory");
    std::println("   - getAssetsOfType() to filter by type");

    std::println("\n7. HOT RELOAD");
    std::println("   - enableHotReload() to toggle file watching");
    std::println("   - checkForReloads() to detect changes");
    std::println("   - reloadAsset() to manually reload");

    std::println("\n8. ASSET STATES");
    std::println("   - Unloaded -> Loading -> Loaded");
    std::println("   - Unloaded -> Loading -> Failed");

    std::println("\n9. USAGE PATTERNS");
    std::println("   - Preloading essential assets");
    std::println("   - Lazy loading level-specific content");
    std::println("   - Async streaming for large files");
    std::println("   - Conditional loading based on settings");
    std::println("   - Memory management via unloading");

    std::println("\n{:=<80}", "");
    std::println("  Demo Complete!");
    std::println("{:=<80}\n", "");

    return 0;
}
