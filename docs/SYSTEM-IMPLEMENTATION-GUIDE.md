# JFrame System Implementation Guide

> **Last Updated:** 2025-11-25
> **Purpose:** Developer reference for implementing and troubleshooting JFrame systems

---

## Table of Contents

1. [Graphics System](#graphics-system)
2. [Audio System](#audio-system)
3. [Save System](#save-system)
4. [Common Issues](#common-issues)

---

## Graphics System

### Overview

The Graphics system uses GLFW for windowing and OpenGL 4.1 Core Profile for rendering. It provides sprite batching with layer-based sorting.

### Key Files

| File | Purpose |
|------|---------|
| `jframe-graphics/src/jframe.graphics.impl.cppm` | Module interface and class definition |
| `jframe-graphics/src/GraphicsSystem.cpp` | Full implementation |

### Dependencies

```cmake
# Required in CMakeLists.txt
target_link_libraries(jframe-graphics
    PUBLIC
        jframe-contract
        glfw
        glm::glm
        glad::glad
)
```

### Initialization Flow

```cpp
bool GraphicsSystem::initialize(int width, int height, const char* title) {
    // 1. Initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 2. Create window
    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
    glfwMakeContextCurrent(window_);

    // 3. Initialize glad (CRITICAL - must be after context creation)
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        return false;
    }

    // 4. Set up OpenGL state
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 5. Compile shaders and create geometry
    // ...
}
```

### Sprite Rendering Pipeline

1. **Collect sprites** - `draw()` adds sprites to batch
2. **Sort by layer** - `endFrame()` sorts by `sprite.layer`
3. **Build matrices** - Calculate projection/view from camera
4. **Render each sprite** - Transform, bind texture, draw quad

### Shader Structure

**Vertex Shader:**
```glsl
#version 410 core
layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aTexCoord;

uniform mat4 uModel;
uniform mat4 uViewProj;

out vec2 vTexCoord;

void main() {
    gl_Position = uViewProj * uModel * vec4(aPosition, 0.0, 1.0);
    vTexCoord = aTexCoord;
}
```

**Fragment Shader:**
```glsl
#version 410 core
in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec4 uTint;

void main() {
    FragColor = texture(uTexture, vTexCoord) * uTint;
}
```

### Troubleshooting

| Issue | Solution |
|-------|----------|
| Black screen | Check if glad loaded successfully |
| No sprites visible | Verify camera viewport size is set |
| Wrong positions | Check model matrix calculation (anchor offset) |
| Textures not showing | Ensure white fallback texture is created |

---

## Audio System

### Overview

The Audio system uses FMOD Core API for sound playback. It supports both channel-based audio (for music/UI) and positional 3D audio (for game sounds).

### Key Files

| File | Purpose |
|------|---------|
| `jframe-audio/src/jframe.audio.impl.cppm` | Module interface and class definition |
| `jframe-audio/src/FMODAudioSystem.cpp` | Full implementation |

### FMOD Installation

FMOD is not included in vcpkg. Install manually:

1. Download FMOD Core API from [fmod.com](https://www.fmod.com/download)
2. Extract to `external/fmod/`:
   ```
   external/fmod/
   ├── include/
   │   └── fmod.h
   └── lib/
       └── macos/
           └── libfmod.dylib
   ```
3. CMake will find it automatically via `cmake/Dependencies.cmake`

### Conditional Compilation

The audio system compiles with or without FMOD:

```cpp
#ifdef JFRAME_HAS_FMOD
    // Real FMOD implementation
    FMOD_System_PlaySound(fmodSystem_, sound, nullptr, false, &channel);
#else
    // Stub mode - tracks state locally but produces no sound
    channels_[channel].state.isPlaying = true;
#endif
```

### Usage Example

```cpp
auto audio = jframe::createAudioSystem();
audio->initialize();

// Play music on dedicated channel
ChannelSound music{
    .asset = musicAsset,
    .volume = 0.8f,
    .looping = true
};
audio->playOnChannel(Channels::Music, music);

// Play 3D positioned sound
PositionalSound explosion{
    .asset = explosionAsset,
    .position = {100.0f, 0.0f, 50.0f},
    .minDistance = 10.0f,
    .maxDistance = 100.0f
};
SoundHandle handle = audio->playPositional(explosion);

// Update listener position (usually camera position)
AudioListener listener{
    .position = cameraPos,
    .forward = cameraForward,
    .up = {0.0f, 1.0f, 0.0f}
};
audio->setListener(listener);

// Call every frame
audio->update(deltaTime);
```

### Troubleshooting

| Issue | Solution |
|-------|----------|
| No sound | Verify FMOD is installed and `JFRAME_HAS_FMOD` is defined |
| CMake "FMOD not found" | Check `external/fmod/` directory structure |
| 3D audio not working | Ensure `setListener()` is called each frame |
| Volume not changing | Check channel group assignments |

---

## Save System

### Overview

The Save system uses cereal for binary serialization with nlohmann_json for metadata files. It supports profiles, auto-save, and versioned save files.

### Key Files

| File | Purpose |
|------|---------|
| `jframe-contract/src/jframe.save.cppm` | Interface and archive types |
| `jframe-save/src/jframe.save.impl.cppm` | Concrete archive implementations |
| `jframe-save/src/SaveSystem.cpp` | Save/load logic |

### Save File Format

```
Binary Save File (.sav):
┌─────────────────────┐
│ Magic: 0x4A465356   │  4 bytes ("JFSV")
│ Version: 1          │  4 bytes
├─────────────────────┤
│ Saveable Count      │  cereal serialized
│ For each saveable:  │
│   - Key (string)    │
│   - Data (binary)   │
└─────────────────────┘

Metadata File (.meta):
{
  "slot": 0,
  "saveName": "Save 1",
  "timestamp": 1732568400,
  "gameVersion": "0.1.0",
  "playtimeSeconds": 3600,
  "completionPercentage": 45.5,
  "levelName": "level_1",
  "hasScreenshot": false
}
```

### Implementing ISaveable

```cpp
class PlayerData : public ISaveable {
public:
    std::string getSaveKey() const override {
        return "player";
    }

    void serialize(ISaveArchive& archive) const override {
        archive.writeInt("health", health_);
        archive.writeFloat("posX", position_.x);
        archive.writeFloat("posY", position_.y);
        archive.writeString("name", name_);
    }

    void deserialize(const ILoadArchive& archive) override {
        health_ = archive.readInt("health");
        position_.x = archive.readFloat("posX");
        position_.y = archive.readFloat("posY");
        name_ = archive.readString("name");
    }

private:
    int health_ = 100;
    Vec2 position_;
    std::string name_;
};
```

### Usage Example

```cpp
auto save = jframe::createSaveSystem();

// Register saveable objects
PlayerData player;
save->registerSaveable(&player);

// Save to slot 0
auto result = save->save(0, "Chapter 1 Complete");
if (!result) {
    // Handle error: result.error()
}

// Load from slot 0
result = save->load(0);
if (!result) {
    switch (result.error()) {
        case SaveError::FileNotFound:
            // No save exists
            break;
        case SaveError::CorruptedFile:
            // Invalid magic number
            break;
        case SaveError::VersionMismatch:
            // Incompatible save version
            break;
    }
}

// Quick save/load
save->quickSave();
save->quickLoad();

// Auto-save every 5 minutes
save->enableAutoSave(std::chrono::minutes(5));
```

### Directory Structure

```
saves/
├── default/           # Default profile
│   ├── save_0.sav
│   ├── save_0.meta
│   ├── save_1.sav
│   ├── save_1.meta
│   ├── save_4294967294.sav  # QuickSave
│   └── save_4294967295.sav  # AutoSave
└── player2/           # Another profile
    └── ...
```

### Troubleshooting

| Issue | Solution |
|-------|----------|
| Save fails | Check directory permissions |
| Load returns CorruptedFile | Wrong magic number - file may be truncated |
| VersionMismatch | Save from different game version |
| SerializationError | Mismatched serialize/deserialize order |

---

## Common Issues

### C++20 Module Issues

**"cannot add 'abi_tag' attribute in a redeclaration"**

This occurs when mixing `#include` with `import std;`. Solution: Use `import std;` consistently in all source files.

**"declaration follows declaration in another module"**

You can't forward-declare a class in one module and define it in another. Either:
- Define the full class in the declaring module, or
- Use abstract interfaces

### Build Issues

**"glad::glad target not found"**

Add glad to vcpkg.json and find it in Dependencies.cmake:
```cmake
find_package(glad CONFIG REQUIRED)
```

**"FMOD not found"**

Install FMOD manually to `external/fmod/`. See Audio System section.

### Runtime Issues

**Assertion failures in EnTT**

Usually caused by accessing destroyed entities. Check entity validity before component access.

**Black screen in graphics**

1. Verify glad loaded: check return value of `gladLoadGLLoader()`
2. Check camera viewport size is non-zero
3. Ensure shaders compiled successfully (check logs)

---

## Adding New Systems

When implementing a new system:

1. **Interface first** - Define in `jframe-contract/src/jframe.yoursystem.cppm`
2. **Impl module** - Create `jframe-yoursystem/src/jframe.yoursystem.impl.cppm`
3. **Implementation** - Create `jframe-yoursystem/src/YourSystem.cpp`
4. **Factory function** - Export `createYourSystem()` from impl module
5. **Tests** - Add to `tests/unit/YourSystemTests.cpp`
6. **Documentation** - Add section to this file

### Module Pattern

```cpp
// jframe-yoursystem/src/jframe.yoursystem.impl.cppm
module;

// Third-party includes go here (global module fragment)
#include <third_party.h>

export module jframe.yoursystem.impl;

import std;
import jframe.yoursystem;  // Your interface
import jframe.types;

export namespace jframe {

class YourSystemImpl : public IYourSystem {
    // ...
};

inline std::unique_ptr<IYourSystem> createYourSystem() {
    return std::make_unique<YourSystemImpl>();
}

}
```

---

## Performance Considerations

### Graphics
- Sort sprites by texture to reduce bind calls
- Use instanced rendering for many similar sprites
- Batch debug primitives into single draw call

### Audio
- Cache FMOD_SOUND objects, don't reload each play
- Use channel groups for efficient volume control
- Clean up finished positional sounds in update()

### Save
- Don't save every frame - use auto-save intervals
- Consider compression (zstd) for large saves
- Profile save/load time in debug builds
