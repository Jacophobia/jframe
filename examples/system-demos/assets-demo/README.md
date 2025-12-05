# Assets System Demo

Comprehensive demonstration of the Bestow Assets System API (`bestow.assets`).

## Overview

This demo exercises **every method** in the `IAssetSystem` interface, demonstrating:

- Asset lifecycle management (register, load, unload, unregister)
- Synchronous and asynchronous loading patterns
- State queries and metadata inspection
- Type-safe data access
- Bulk operations
- Hot reload capabilities
- Error handling

## Building

```bash
# From project root
cmake --build --preset macos-debug --target assets-demo

# Run the demo
./build/macos-debug/bin/assets-demo
```

## Demonstrated API

### 1. Lifecycle Management
- `update()` - Process async loading operations

### 2. Asset Registration
- `registerAsset(type, path)` - Register assets of all types:
  - `AssetType::Texture` - Images (PNG, JPG, etc.)
  - `AssetType::Sound` - Sound effects (WAV, OGG, MP3)
  - `AssetType::Music` - Background music
  - `AssetType::Font` - TTF/OTF fonts
  - `AssetType::Level` - Lua level definitions
  - `AssetType::Data` - JSON data files
  - `AssetType::Shader` - GLSL shaders
  - `AssetType::NavMesh` - Navigation meshes
  - `AssetType::BehaviorTree` - AI behavior trees
- `unregisterAsset(handle)` - Remove from registry

### 3. Loading Operations
- `loadAsset(handle)` - Synchronous loading
- `loadAssetAsync(handle, callback)` - Asynchronous loading with callbacks
- `unloadAsset(handle)` - Unload to free memory

### 4. State Queries
- `getAssetState(handle)` - Returns `AssetState` enum:
  - `Unloaded` - Not yet loaded
  - `Loading` - Currently loading (async)
  - `Loaded` - Ready for use
  - `Failed` - Load error occurred
- `isLoaded(handle)` - Boolean check
- `getAssetMetadata(handle)` - Full metadata including:
  - UUID
  - Source path
  - Size in bytes
  - Error messages (if failed)

### 5. Data Access
- `getRawAsset(handle)` - Get `void*` pointer to raw data
- `getAsset<T>(handle)` - Type-safe templated access:
  - `TextureData` - Pixel data, dimensions, channels
  - `FontData` - Raw TTF/OTF bytes
  - `SoundData` - Raw audio file bytes

### 6. Bulk Operations
- `loadAll()` - Load all registered assets
- `unloadAll()` - Unload all assets to free memory
- `getAssetsOfType(type)` - Get all assets of a specific type

### 7. Hot Reload (Development)
- `enableHotReload(bool)` - Enable/disable file watching
- `checkForReloads()` - Check for modified files
- `reloadAsset(handle)` - Manually reload an asset

## Usage Patterns

The demo showcases common patterns:

1. **Preloading** - Load essential assets at startup
2. **Lazy loading** - Load level-specific assets on demand
3. **Streaming** - Async load large files (music) in background
4. **Conditional loading** - Quality settings (high-res vs low-res)
5. **Memory management** - Unload unused assets during transitions

## Code Structure

- `src/main.cpp` - Entry point with all demo functions:
  - `demoRegistration()` - Register all asset types
  - `demoStateQueries()` - Query asset states and metadata
  - `demoSyncLoading()` - Synchronous loading
  - `demoAsyncLoading()` - Asynchronous loading with callbacks
  - `demoBulkOperations()` - Batch operations
  - `demoAssetLifecycle()` - Full lifecycle walkthrough
  - `demoHotReload()` - Hot reload API
  - `demoErrorHandling()` - Error scenarios
  - `demoDataStructures()` - Asset data structures
  - `demoAdvancedPatterns()` - Real-world usage patterns

## Notes

- The demo is **API-focused** - asset files don't need to exist
- Demonstrates the interface contract, not file I/O details
- Shows proper error handling and state transitions
- Includes callbacks and async patterns
- Documents all data structures (TextureData, FontData, SoundData)

## Dependencies

- **bestow-contract** - Interface definitions
- **bestow-assets** - Asset system implementation
- **bestow-core** - Core utilities
- **C++23** with `import std;` support

## See Also

- `/docs/bestow-technical-design.md` - Full system architecture
- `/bestow-contract/src/bestow.assets.cppm` - Interface definition
- `/bestow-assets/src/AssetSystem.cpp` - Implementation
