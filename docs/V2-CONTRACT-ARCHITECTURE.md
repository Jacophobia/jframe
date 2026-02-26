# Bestow V2 Contract Architecture

> **Date:** 2026-02-25
> **Status:** Draft
> **Companions:** `SYSTEM-ARCHITECTURE.md` (tier layout), `BESTOW-COMPETITIVE-ANALYSIS.md` (gap analysis)

---

## Table of Contents

1. [Design Goals](#design-goals)
2. [Part 1: Dual API Architecture](#part-1-dual-api-architecture)
3. [Part 2: Self-Updating Documentation Infrastructure](#part-2-self-updating-documentation-infrastructure)
4. [Part 3: Complete V2 Contract Definitions](#part-3-complete-v2-contract-definitions)
   - [Fully Detailed Systems (4 examples)](#fully-detailed-systems)
   - [All Remaining Systems](#all-remaining-systems)

---

## Design Goals

### 1. Two APIs, One Implementation
Every system exposes a **high-level API** (what game developers use 90% of the time) and a **low-level API** (full engine control). Both are available from Lua. One C++ implementation satisfies both contracts.

### 2. Documentation That Cannot Go Stale
A single source of truth drives Lua bindings, IDE stubs, CLI help, and API docs. Compile-time and test-time validation catches drift the moment it occurs.

### 3. Complete Feature Coverage
Every feature identified in the competitive analysis is placed in a contract. No feature exists only as an implementation detail.

---

## Part 1: Dual API Architecture

### The Problem With a Single API Surface

Currently, `bestow.audio` exposes 24 methods at one level. A game developer who wants to play a sound must understand channels, handles, and groups. A developer who needs spatial audio occlusion must use the same flat API. This creates two problems:

1. **Beginners are overwhelmed.** 24 methods when they need 3.
2. **Power users are constrained.** The "simplified" API hides capabilities they need.

### The Solution: System + Core

Every system provides two contracts:

```
bestow.audio.*        ← IAudioSystem      (high-level, ~8 methods)
bestow.audio.core.*   ← IAudioCore        (low-level, ~30 methods)
```

| Layer | Contract | Lua Path | Audience | Purpose |
|-------|----------|----------|----------|---------|
| **System** | `IAudioSystem` | `bestow.audio.*` | Game developers | Simple, opinionated, handles common cases |
| **Core** | `IAudioCore` | `bestow.audio.core.*` | Power users, engine systems | Full control, every parameter exposed |

### Architectural Pattern

```cpp
// Core: the full interface. Implementations satisfy this.
class IAudioCore {
public:
    virtual ~IAudioCore() = default;

    // Lifecycle (called by engine, not game code)
    virtual void initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update(DeltaTime dt) = 0;

    // Channel management
    virtual Channel playOnChannel(SoundHandle, int channel) = 0;
    virtual void stopChannel(int channel) = 0;
    virtual void pauseChannel(int channel) = 0;
    virtual void resumeChannel(int channel) = 0;
    virtual void setChannelVolume(int channel, float vol) = 0;
    virtual void setChannelPitch(int channel, float pitch) = 0;
    virtual void seekChannel(int channel, float positionMs) = 0;
    virtual bool isChannelPlaying(int channel) const = 0;
    virtual ChannelState getChannelState(int channel) const = 0;

    // Positional audio
    virtual void playPositional(SoundHandle, const PositionalSound&) = 0;
    virtual void updatePositionalPosition(int id, Vec3 pos) = 0;
    virtual void stopPositional(int id) = 0;

    // Listener
    virtual void setListener(const AudioListener&) = 0;
    virtual AudioListener getListener() const = 0;

    // Groups
    virtual void setGroupVolume(const std::string& group, float vol) = 0;
    virtual void assignChannelToGroup(int channel, const std::string& group) = 0;

    // Global
    virtual void setMasterVolume(float vol) = 0;
    virtual float getMasterVolume() const = 0;
    virtual void pauseAll() = 0;
    virtual void resumeAll() = 0;
    virtual void stopAll() = 0;

    // === NEW: Audio effects (missing in v1) ===
    virtual void setChannelReverb(int channel, float wet) = 0;
    virtual void setChannelEQ(int channel, float low, float mid, float high) = 0;
    virtual void setChannelLowPass(int channel, float cutoff) = 0;

    // === NEW: Bus/submix routing ===
    virtual int createBus(const std::string& name) = 0;
    virtual void routeChannelToBus(int channel, int bus) = 0;
    virtual void setBusVolume(int bus, float vol) = 0;
    virtual void setBusEffect(int bus, const std::string& effect, float wet) = 0;
};

// System: simplified facade. Default implementation wraps Core.
class IAudioSystem {
public:
    virtual ~IAudioSystem() = default;

    // The ~8 methods game developers actually need
    virtual Channel play(const std::string& soundPath) = 0;
    virtual Channel playAt(const std::string& soundPath, Vec3 position) = 0;
    virtual void playMusic(const std::string& musicPath) = 0;
    virtual void stopMusic() = 0;
    virtual void setVolume(float volume) = 0;
    virtual void setMusicVolume(float volume) = 0;
    virtual void stop(Channel channel) = 0;
    virtual void stopAll() = 0;
};
```

### How They Wire Together

```
┌─────────────────────────────────────────────────────┐
│  Lua Game Code                                       │
│                                                     │
│  bestow.audio.play("boom.wav")        ← Simple     │
│  bestow.audio.core.setChannelEQ(...)  ← Full power │
└────────────┬──────────────┬─────────────────────────┘
             │              │
             ▼              ▼
    ┌────────────┐  ┌───────────────┐
    │IAudioSystem│  │  IAudioCore   │
    │  (facade)  │  │  (full API)   │
    └──────┬─────┘  └──────┬────────┘
           │               │
           └───────┬───────┘
                   │
                   ▼
         ┌──────────────────┐
         │ FMODAudioSystem  │
         │ (implements both)│
         └──────────────────┘
```

### DI Registration

```cpp
// Engine registers one implementation that satisfies both contracts
engine.use<IAudioCore, FMODAudioSystem>([](di::ServiceProvider& sp) {
    return new FMODAudioSystem(sp.get<IAssetCore>());
});

// The facade can be the same object or a wrapper
engine.use<IAudioSystem, FMODAudioSystem>([](di::ServiceProvider& sp) {
    return &static_cast<FMODAudioSystem&>(sp.get<IAudioCore>());
});
```

### When a System Doesn't Need Two Levels

Some systems are already simple enough that one contract suffices. In these cases, the **System** contract IS the only contract. There is no `.core` path in Lua. However, the infrastructure still exists if we need to add a Core layer later.

Systems with **one contract** (System only):
- Events (13 methods -- already simple)
- Camera (12 methods -- already simple)
- Tween (~20 methods -- new, designed simply)
- Scene (11 methods -- already simple)
- Game State (22 methods -- already simple)
- Blueprints (12 methods -- already simple)

Systems with **two contracts** (System + Core):
- Entity, Input, Audio, Physics 2D, Physics 3D, Graphics 2D, Graphics 3D, Animation, Anim State Machine, State, Config, UI, AI, GAS, Particles, Network

### Naming Convention

| C++ Contract | Lua Path | Purpose |
|-------------|----------|---------|
| `IAudioSystem` | `bestow.audio.*` | High-level facade |
| `IAudioCore` | `bestow.audio.core.*` | Low-level full API |
| `IPhysicsSystem` | `bestow.physics.*` | High-level facade |
| `IPhysicsCore` | `bestow.physics.core.*` | Low-level full API |

---

## Part 2: Self-Updating Documentation Infrastructure

### The Current Problem (3 Sources of Truth)

```
bestow.assets.cppm        ← Informal comments (Source 1)
assets_binding_doc.cpp    ← Structured doc objects (Source 2)
StubGenerator.cpp         ← Hardcoded EmmyLua strings (Source 3)
```

When someone adds a method to `IAssetSystem`, they must update **three separate files**. If they forget one, documentation silently drifts. No compiler error, no test failure.

### The V2 Solution: Single Source + Compile-Time Enforcement

```
┌──────────────────────────────────────────────────┐
│  System Definition File (ONE PER SYSTEM)          │
│                                                  │
│  - Contract version stamp                        │
│  - Method roster with docs, params, returns      │
│  - Lua examples (testable)                       │
│  - Both System + Core methods                    │
│                                                  │
│  static_assert(kContractVersion == 7)            │
│  static_assert(methods.size() == kMethodCount)   │
└──────────────┬───────────────────────────────────┘
               │
    ┌──────────┼──────────┬──────────┬──────────┐
    ▼          ▼          ▼          ▼          ▼
┌────────┐┌────────┐┌────────┐┌────────┐┌────────┐
│  sol2  ││ CLI    ││ Stubs  ││ Doc    ││  Doc   │
│Bindings││ --help ││EmmyLua ││ Tests  ││Markdown│
└────────┘└────────┘└────────┘└────────┘└────────┘
```

### Component 1: Contract Version Stamps

Every contract interface declares a version number and method count:

```cpp
// In bestow.audio.cppm
class IAudioCore {
public:
    static constexpr int kContractVersion = 1;
    static constexpr int kMethodCount = 28;

    virtual ~IAudioCore() = default;
    virtual Channel playOnChannel(SoundHandle, int) = 0;
    // ...
};

class IAudioSystem {
public:
    static constexpr int kContractVersion = 1;
    static constexpr int kMethodCount = 8;

    virtual ~IAudioSystem() = default;
    virtual Channel play(const std::string&) = 0;
    // ...
};
```

When a developer adds a method, they bump `kMethodCount`. The system definition file immediately fails to compile if its method list doesn't match.

### Component 2: System Definition Files

One file per system replaces BOTH `*_binding_doc.cpp` AND `StubGenerator` hardcoded strings:

```cpp
// bestow-luabind/src/definitions/audio_definition.cpp

import bestow.audio;
import bestow.luabind; // for SystemDefinition, MethodDef, etc.

namespace bestow::definitions {

const SystemDefinition kAudioSystem = {
    .name = "audio",
    .qualifiedName = "bestow.audio",
    .description = "Play sounds, music, and spatial audio.",
    .contractVersion = IAudioSystem::kContractVersion,

    .methods = {
        {
            .name = "play",
            .level = ApiLevel::System,   // exposed on bestow.audio.*
            .description = "Play a sound effect by file path.",
            .params = {
                {"soundPath", "string", "Path to the sound file (relative to assets/)"}
            },
            .returns = {{"Channel", "Handle to the playing channel"}},
            .example = R"lua(
                local ch = bestow.audio.play("sounds/explosion.wav")
            )lua",
            .seeAlso = {"bestow.audio.stop", "bestow.audio.core.playOnChannel"},
        },
        {
            .name = "playOnChannel",
            .level = ApiLevel::Core,     // exposed on bestow.audio.core.*
            .description = "Play a loaded sound on a specific channel.",
            .params = {
                {"sound", "SoundHandle", "Handle from asset system"},
                {"channel", "number", "Channel index (0-based)"}
            },
            .returns = {{"Channel", "The channel used"}},
            .example = R"lua(
                local handle = bestow.assets.registerAsset(
                    bestow.assets.Type.Sound, "sounds/boom.wav")
                bestow.assets.loadAsset(handle)
                local ch = bestow.audio.core.playOnChannel(handle, 0)
            )lua",
        },
        // ... all methods for both System and Core ...
    },

    .enums = { /* ... */ },
    .types = { /* ... */ },
};

// === COMPILE-TIME VALIDATION ===
static_assert(
    IAudioSystem::kContractVersion == kAudioSystem.contractVersion,
    "Audio system definition is out of sync with IAudioSystem contract. "
    "Bump contractVersion in the definition file."
);

static_assert(
    kAudioSystem.countMethodsAtLevel(ApiLevel::System) == IAudioSystem::kMethodCount,
    "Audio system definition has wrong System-level method count. "
    "A method was added to or removed from IAudioSystem without updating the definition."
);

static_assert(
    kAudioSystem.countMethodsAtLevel(ApiLevel::Core) == IAudioCore::kMethodCount,
    "Audio core definition has wrong Core-level method count. "
    "A method was added to or removed from IAudioCore without updating the definition."
);

} // namespace bestow::definitions
```

### Component 3: Generated Stubs From Definitions

The StubGenerator is rewritten to read from definitions instead of hardcoding:

```cpp
// NEW StubGenerator - reads from SystemDefinition objects
void StubGenerator::generate(const std::filesystem::path& outputDir) {
    for (const auto& systemDef : getAllDefinitions()) {
        generateSystemStub(outputDir, systemDef);
    }
}

void StubGenerator::generateSystemStub(
    const std::filesystem::path& dir,
    const SystemDefinition& def
) {
    std::ofstream file(dir / (def.name + ".lua"));
    file << "---@meta\n";
    file << "-- " << def.description << "\n\n";

    // High-level namespace
    file << "---@class bestow." << def.name << "\n";
    file << "bestow." << def.name << " = {}\n\n";

    for (const auto& method : def.methods) {
        if (method.level != ApiLevel::System) continue;
        writeMethodStub(file, "bestow." + def.name, method);
    }

    // Core namespace (if methods exist at Core level)
    if (def.hasCoreMethods()) {
        file << "---@class bestow." << def.name << ".core\n";
        file << "bestow." << def.name << ".core = {}\n\n";

        for (const auto& method : def.methods) {
            if (method.level != ApiLevel::Core) continue;
            writeMethodStub(file, "bestow." + def.name + ".core", method);
        }
    }
}

void StubGenerator::writeMethodStub(
    std::ofstream& file,
    const std::string& ns,
    const MethodDef& method
) {
    file << "---" << method.description << "\n";
    for (const auto& param : method.params) {
        file << "---@param " << param.name << " " << param.type;
        if (param.optional) file << "?";
        file << " " << param.description << "\n";
    }
    for (const auto& ret : method.returns) {
        file << "---@return " << ret.type << " # " << ret.description << "\n";
    }
    file << "function " << ns << "." << method.name << "(";
    // param list
    for (size_t i = 0; i < method.params.size(); ++i) {
        if (i > 0) file << ", ";
        file << method.params[i].name;
    }
    file << ") end\n\n";
}
```

**Result:** Stubs are always 1:1 with definitions. No hardcoded strings. No drift.

### Component 4: Generated CLI Help

Replace hardcoded `printUsage()` with definition-driven help:

```cpp
// CommandLine.cpp
void printUsage(const char* programName) {
    auto commands = getCommandDefinitions(); // Single source
    fmt::print("Bestow Game Engine - Lua-Driven Game Development\n\n");
    fmt::print("Usage: {} <command> [options]\n\n", programName);
    fmt::print("Commands:\n");
    for (const auto& cmd : commands) {
        fmt::print("  {:<20} {}\n", cmd.usage, cmd.description);
    }
}

// In handleHelp for a specific command:
void printCommandHelp(const CommandDefinition& cmd) {
    fmt::print("{}\n\n", cmd.description);
    fmt::print("Usage: bestow {}\n\n", cmd.usage);
    if (!cmd.options.empty()) {
        fmt::print("Options:\n");
        for (const auto& opt : cmd.options) {
            fmt::print("  --{:<20} {}", opt.longName, opt.description);
            if (!opt.defaultValue.empty())
                fmt::print(" [default: {}]", opt.defaultValue);
            fmt::print("\n");
        }
    }
}
```

### Component 5: Doc-Tests (Lua Examples That Run)

Every `example` field in a `MethodDef` is a runnable Lua snippet. A test harness validates them:

```cpp
TEST(DocTests, AllExamplesExecuteSuccessfully) {
    sol::state lua;
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string);

    // Register all bindings (populates both Lua state and DocRegistry)
    registerAllBindings(lua);

    for (const auto& systemDef : getAllDefinitions()) {
        for (const auto& method : systemDef.methods) {
            if (method.example.empty()) continue;

            SCOPED_TRACE(fmt::format("{}.{}", systemDef.qualifiedName, method.name));

            auto result = lua.safe_script(method.example, sol::script_pass_on_error);
            EXPECT_TRUE(result.valid())
                << "Doc example failed for " << systemDef.qualifiedName
                << "." << method.name << ":\n"
                << result.get<sol::error>().what();
        }
    }
}
```

### Component 6: Binding Registration Uses Definitions

Instead of separate binding and doc files, one registration reads from the definition:

```cpp
void bindAudioSystem(sol::state& lua, IAudioSystem& system, IAudioCore& core) {
    const auto& def = definitions::kAudioSystem;

    // High-level: bestow.audio.*
    sol::table audioTable = lua["bestow"].get_or_create<sol::table>()
                                ["audio"].get_or_create<sol::table>();

    audioTable["play"] = [&system](const std::string& path) {
        return system.play(path);
    };
    audioTable["stop"] = [&system](Channel ch) { system.stop(ch); };
    // ... remaining System-level methods

    // Low-level: bestow.audio.core.*
    sol::table coreTable = audioTable["core"].get_or_create<sol::table>();

    coreTable["playOnChannel"] = [&core](SoundHandle s, int ch) {
        return core.playOnChannel(s, ch);
    };
    coreTable["setChannelVolume"] = [&core](int ch, float v) {
        core.setChannelVolume(ch, v);
    };
    // ... remaining Core-level methods

    // Register docs from the SAME definition used for validation
    DocRegistry::instance().addSystem(def.toDocFormat());
}
```

### Component 7: Coverage Test (The Safety Net)

A test verifies that every Lua-bound method has documentation and vice versa:

```cpp
TEST(DocCoverage, AllBoundMethodsAreDocumented) {
    sol::state lua;
    registerAllBindings(lua);
    auto& registry = DocRegistry::instance();

    for (const auto& systemDef : getAllDefinitions()) {
        // Check bestow.system.* methods
        sol::table sysTable = lua["bestow"][systemDef.name];
        for (const auto& [key, value] : sysTable) {
            if (value.is<sol::function>()) {
                std::string methodName = key.as<std::string>();
                EXPECT_TRUE(registry.getMethod(
                    systemDef.qualifiedName + "." + methodName))
                    << "Bound method " << systemDef.qualifiedName
                    << "." << methodName << " has no documentation!";
            }
        }

        // Check bestow.system.core.* methods
        if (sysTable["core"].valid()) {
            sol::table coreTable = sysTable["core"];
            for (const auto& [key, value] : coreTable) {
                if (value.is<sol::function>()) {
                    std::string methodName = key.as<std::string>();
                    EXPECT_TRUE(registry.getMethod(
                        systemDef.qualifiedName + ".core." + methodName))
                        << "Bound core method " << systemDef.qualifiedName
                        << ".core." << methodName << " has no documentation!";
                }
            }
        }
    }
}

TEST(DocCoverage, AllDocumentedMethodsAreBound) {
    sol::state lua;
    registerAllBindings(lua);

    for (const auto& systemDef : getAllDefinitions()) {
        for (const auto& method : systemDef.methods) {
            std::string luaPath = (method.level == ApiLevel::Core)
                ? "bestow." + systemDef.name + ".core." + method.name
                : "bestow." + systemDef.name + "." + method.name;

            // Resolve the Lua path
            sol::object obj = resolveLuaPath(lua, luaPath);
            EXPECT_TRUE(obj.is<sol::function>())
                << "Documented method " << luaPath
                << " is not bound in Lua!";
        }
    }
}
```

### Summary: What Changes

| Component | V1 (Current) | V2 (Proposed) |
|-----------|-------------|---------------|
| Contract .cppm | Informal comments | + `kContractVersion` + `kMethodCount` |
| Binding code | No docs | Reads from SystemDefinition |
| Binding docs | Manual `*_binding_doc.cpp` (21 files) | **Deleted.** Replaced by definition files |
| Stub generator | 1250 lines of hardcoded strings | Reads from SystemDefinition |
| CLI help | Hardcoded in CommandLine.cpp | Generated from CommandDefinition |
| Doc validation | None | static_assert + coverage tests + doc-tests |
| Sources of truth | **3** (contract, binding doc, stubs) | **1** (SystemDefinition) |

### File Layout After Migration

```
bestow-contract/src/
  bestow.audio.cppm           ← Interface (kContractVersion, kMethodCount)
  bestow.entity.cppm
  ...

bestow-luabind/src/
  definitions/                 ← NEW: single source of truth per system
    audio_definition.cpp       ← Methods, docs, examples, validation
    entity_definition.cpp
    physics_definition.cpp
    ...
  bindings/                    ← Binding code (reads from definitions)
    audio_binding.cpp
    entity_binding.cpp
    ...
  docs/                        ← DELETED (replaced by definitions/)
    ~~audio_binding_doc.cpp~~
    ~~entity_binding_doc.cpp~~
    ...
  stubs/
    StubGenerator.cpp          ← REWRITTEN: reads from definitions
```

---

## Part 3: Complete V2 Contract Definitions

### Guiding Principles for the System/Core Split

1. **System** contains methods a game developer needs in a typical game loop. If you can build a complete game using only System methods, the split is correct.
2. **Core** contains lifecycle management, advanced configuration, implementation-specific features, and escape hatches.
3. **System methods may call Core methods internally.** The facade delegates to the full API.
4. **System methods use simpler types.** Paths instead of handles. Strings instead of enums where appropriate. Lua tables instead of typed structs.
5. **Core methods use precise types.** Handles, enums, explicit parameters. No convenience overloading.

---

### Fully Detailed Systems

The following 4 systems are fully specified to establish the pattern. All remaining systems follow the same structure.

---

#### 1. Audio

##### `IAudioSystem` (High-Level) -- `bestow.audio.*`

```cpp
class IAudioSystem {
public:
    static constexpr int kContractVersion = 1;
    static constexpr int kMethodCount = 10;

    virtual ~IAudioSystem() = default;

    // Play a sound by file path. Auto-loads if needed.
    virtual Channel play(const std::string& soundPath) = 0;

    // Play a sound at a 3D position.
    virtual Channel playAt(const std::string& soundPath, Vec3 position) = 0;

    // Play background music (cross-fades if music is already playing).
    virtual void playMusic(const std::string& musicPath) = 0;

    // Stop background music with optional fade-out duration.
    virtual void stopMusic(float fadeDuration = 0.0f) = 0;

    // Stop a specific channel.
    virtual void stop(Channel channel) = 0;

    // Stop all sounds and music.
    virtual void stopAll() = 0;

    // Set master volume (0.0 to 1.0).
    virtual void setVolume(float volume) = 0;

    // Set music volume independently (0.0 to 1.0).
    virtual void setMusicVolume(float volume) = 0;

    // Set sound effects volume independently (0.0 to 1.0).
    virtual void setSFXVolume(float volume) = 0;

    // Check if a channel is still playing.
    virtual bool isPlaying(Channel channel) const = 0;
};
```

##### `IAudioCore` (Low-Level) -- `bestow.audio.core.*`

```cpp
class IAudioCore {
public:
    static constexpr int kContractVersion = 1;
    static constexpr int kMethodCount = 38;

    virtual ~IAudioCore() = default;

    // --- Lifecycle (engine-internal) ---
    virtual void initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update(DeltaTime dt) = 0;

    // --- Channel Management ---
    virtual Channel playOnChannel(SoundHandle sound, int channel) = 0;
    virtual void stopChannel(int channel) = 0;
    virtual void pauseChannel(int channel) = 0;
    virtual void resumeChannel(int channel) = 0;
    virtual void setChannelVolume(int channel, float volume) = 0;
    virtual void setChannelPitch(int channel, float pitch) = 0;
    virtual void setChannelPan(int channel, float pan) = 0;
    virtual void seekChannel(int channel, float positionMs) = 0;
    virtual bool isChannelPlaying(int channel) const = 0;
    virtual ChannelState getChannelState(int channel) const = 0;
    virtual float getChannelPlaybackPosition(int channel) const = 0;

    // --- Positional / 3D Audio ---
    virtual int playPositional(SoundHandle sound, const PositionalSound& config) = 0;
    virtual void updatePositionalPosition(int id, Vec3 position) = 0;
    virtual void setPositionalVelocity(int id, Vec3 velocity) = 0;
    virtual void stopPositional(int id) = 0;
    virtual bool isPositionalPlaying(int id) const = 0;

    // --- 3D Listener ---
    virtual void setListener(const AudioListener& listener) = 0;
    virtual AudioListener getListener() const = 0;

    // --- Channel Groups ---
    virtual void setGroupVolume(const std::string& group, float volume) = 0;
    virtual void assignChannelToGroup(int channel, const std::string& group) = 0;

    // --- Global Controls ---
    virtual void setMasterVolume(float volume) = 0;
    virtual float getMasterVolume() const = 0;
    virtual void pauseAll() = 0;
    virtual void resumeAll() = 0;
    virtual void stopAll() = 0;

    // === NEW IN V2: Audio Effects ===
    virtual void setChannelReverb(int channel, float wetLevel) = 0;
    virtual void setChannelEQ(int channel, float low, float mid, float high) = 0;
    virtual void setChannelLowPass(int channel, float cutoffHz) = 0;
    virtual void setChannelHighPass(int channel, float cutoffHz) = 0;

    // === NEW IN V2: Bus/Submix Routing ===
    virtual int createBus(const std::string& name) = 0;
    virtual void destroyBus(int busId) = 0;
    virtual void routeChannelToBus(int channel, int busId) = 0;
    virtual void setBusVolume(int busId, float volume) = 0;
    virtual void setBusEffect(int busId, const std::string& effectName, float wet) = 0;

    // === NEW IN V2: Doppler ===
    virtual void setDopplerScale(float scale) = 0;

    // === NEW IN V2: Fade ===
    virtual void fadeChannel(int channel, float targetVol, float durationSec) = 0;
};
```

---

#### 2. Entity

##### `IEntitySystem` (High-Level) -- `bestow.entity.*`

```cpp
class IEntitySystem {
public:
    static constexpr int kContractVersion = 1;
    static constexpr int kMethodCount = 12;

    virtual ~IEntitySystem() = default;

    // Create a new entity.
    virtual Entity create() = 0;

    // Destroy an entity and all its components.
    virtual void destroy(Entity entity) = 0;

    // Check if an entity is valid and alive.
    virtual bool isValid(Entity entity) const = 0;

    // Add a component by name (Lua-friendly, type-erased).
    virtual bool addComponent(Entity entity, const std::string& name,
                              const ComponentData& data = {}) = 0;

    // Remove a component by name.
    virtual bool removeComponent(Entity entity, const std::string& name) = 0;

    // Check if entity has a component by name.
    virtual bool hasComponent(Entity entity, const std::string& name) const = 0;

    // Get a component field value by name.
    virtual std::optional<ComponentFieldValue> getField(
        Entity entity, const std::string& component,
        const std::string& field) const = 0;

    // Set a component field value by name.
    virtual bool setField(Entity entity, const std::string& component,
                          const std::string& field,
                          const ComponentFieldValue& value) = 0;

    // Get all fields of a component as a map.
    virtual std::optional<ComponentData> getComponent(
        Entity entity, const std::string& name) const = 0;

    // Count total entities.
    virtual std::size_t count() const = 0;

    // Find all entities with a given component.
    virtual std::vector<Entity> withComponent(const std::string& name) const = 0;

    // Find the first entity with a given component, or nil.
    virtual std::optional<Entity> findFirst(const std::string& name) const = 0;
};
```

##### `IEntityCore` (Low-Level) -- `bestow.entity.core.*`

```cpp
class IEntityCore {
public:
    static constexpr int kContractVersion = 1;
    static constexpr int kMethodCount = 24;

    virtual ~IEntityCore() = default;

    // --- Lifecycle ---
    virtual Entity createEntity() = 0;
    virtual void destroyEntity(Entity entity) = 0;
    virtual bool isValid(Entity entity) const = 0;
    virtual std::size_t entityCount() const = 0;

    // --- Typed Component Access (C++ only, not Lua-bound) ---
    template<typename T, typename... Args>
    T& emplace(Entity entity, Args&&... args);
    template<typename T>
    void remove(Entity entity);
    template<typename T>
    T& get(Entity entity);
    template<typename T>
    T* tryGet(Entity entity);

    // --- Type-Erased Component Access ---
    virtual bool addComponentByName(Entity, const std::string&, const ComponentData&) = 0;
    virtual bool removeComponentByName(Entity, const std::string&) = 0;
    virtual bool hasComponentByName(Entity, const std::string&) const = 0;
    virtual std::optional<ComponentData> getComponentByName(Entity, const std::string&) const = 0;
    virtual bool setComponentByName(Entity, const std::string&, const ComponentData&) = 0;

    // --- Field Access ---
    virtual std::optional<ComponentFieldValue> getComponentField(
        Entity, const std::string& comp, const std::string& field) const = 0;
    virtual bool setComponentField(
        Entity, const std::string& comp, const std::string& field,
        const ComponentFieldValue& value) = 0;
    virtual std::vector<ComponentFieldInfo> getComponentFields(
        Entity, const std::string& comp) const = 0;

    // --- Reflection / Registration ---
    virtual void registerComponentType(const ComponentTypeInfo& info) = 0;
    virtual void unregisterComponentType(const std::string& name) = 0;
    virtual bool isComponentTypeRegistered(const std::string& name) const = 0;
    virtual std::optional<ComponentTypeInfo> getComponentTypeInfo(const std::string&) const = 0;
    virtual std::vector<std::string> getRegisteredComponentTypes() const = 0;

    // --- Query API ---
    virtual EntitySelector query() = 0;

    // --- Registry Access (C++ only) ---
    virtual entt::registry& getRegistry() = 0;
    virtual const entt::registry& getRegistry() const = 0;

    // === NEW IN V2: Object Pooling ===
    virtual void reserveEntities(std::size_t count) = 0;
    virtual Entity createFromPool(const std::string& poolName) = 0;
    virtual void returnToPool(Entity entity, const std::string& poolName) = 0;
    virtual void createPool(const std::string& name, std::size_t size) = 0;
};
```

---

#### 3. Physics 2D

##### `IPhysicsSystem` (High-Level) -- `bestow.physics.*`

```cpp
class IPhysicsSystem {
public:
    static constexpr int kContractVersion = 1;
    static constexpr int kMethodCount = 16;

    virtual ~IPhysicsSystem() = default;

    // Add a physics body to an entity using a definition table.
    virtual void addBody(Entity entity, const PhysicsBodyDef& def) = 0;

    // Remove the physics body from an entity.
    virtual void removeBody(Entity entity) = 0;

    // Apply a force to an entity (continuous, for movement).
    virtual void applyForce(Entity entity, Vec2 force) = 0;

    // Apply an impulse to an entity (instant, for jumps/hits).
    virtual void applyImpulse(Entity entity, Vec2 impulse) = 0;

    // Set the velocity of an entity directly.
    virtual void setVelocity(Entity entity, Vec2 velocity) = 0;

    // Get the current velocity of an entity.
    virtual Vec2 getVelocity(Entity entity) const = 0;

    // Set the position of an entity (teleport).
    virtual void setPosition(Entity entity, Vec2 position) = 0;

    // Get the position of an entity.
    virtual Vec2 getPosition(Entity entity) const = 0;

    // Check if an entity is on the ground (platformer helper).
    virtual GroundCheckResult checkGrounded(Entity entity,
                                             const GroundCheckParams& params) const = 0;

    // Cast a ray and return the first hit.
    virtual std::optional<RaycastHit> raycast(Vec2 origin, Vec2 direction,
                                               float distance) const = 0;

    // Query all entities in an area.
    virtual std::vector<Entity> queryArea(Vec2 center, Vec2 halfExtents) const = 0;

    // Query all entities in a circle.
    virtual std::vector<Entity> queryCircle(Vec2 center, float radius) const = 0;

    // Set global gravity.
    virtual void setGravity(Vec2 gravity) = 0;

    // Get global gravity.
    virtual Vec2 getGravity() const = 0;

    // Set a callback for collisions between two entities.
    virtual void onCollision(std::function<void(Entity, Entity, Vec2)> callback) = 0;

    // Set a callback for trigger enter/exit events.
    virtual void onTrigger(std::function<void(Entity, Entity, bool)> callback) = 0;
};
```

##### `IPhysicsCore` (Low-Level) -- `bestow.physics.core.*`

```cpp
class IPhysicsCore {
public:
    static constexpr int kContractVersion = 1;
    static constexpr int kMethodCount = 30;

    virtual ~IPhysicsCore() = default;

    // --- Lifecycle ---
    virtual void update(DeltaTime dt) = 0;

    // --- Body Management ---
    virtual void createBody(Entity entity, const PhysicsBodyDef& def) = 0;
    virtual void destroyBody(Entity entity) = 0;
    virtual bool hasBody(Entity entity) const = 0;
    virtual void setBodyType(Entity entity, BodyType type) = 0;
    virtual BodyType getBodyType(Entity entity) const = 0;

    // --- Transform ---
    virtual void setPosition(Entity entity, Vec2 pos) = 0;
    virtual Vec2 getPosition(Entity entity) const = 0;
    virtual void setRotation(Entity entity, float radians) = 0;
    virtual float getRotation(Entity entity) const = 0;

    // --- Dynamics ---
    virtual void setVelocity(Entity entity, Vec2 vel) = 0;
    virtual Vec2 getVelocity(Entity entity) const = 0;
    virtual void setAngularVelocity(Entity entity, float omega) = 0;
    virtual float getAngularVelocity(Entity entity) const = 0;
    virtual void applyForce(Entity entity, Vec2 force) = 0;
    virtual void applyImpulse(Entity entity, Vec2 impulse) = 0;
    virtual void applyTorque(Entity entity, float torque) = 0;

    // --- Collision Filtering ---
    virtual void setCollisionLayer(Entity entity, uint16_t layer) = 0;
    virtual void setCollisionMask(Entity entity, uint16_t mask) = 0;
    virtual uint16_t getCollisionLayer(Entity entity) const = 0;
    virtual void setSensor(Entity entity, bool isSensor) = 0;
    virtual Vec2 getBodySize(Entity entity) const = 0;

    // --- Spatial Queries ---
    virtual std::vector<Entity> queryAABB(Vec2 min, Vec2 max) const = 0;
    virtual std::vector<Entity> queryCircle(Vec2 center, float radius) const = 0;
    virtual std::optional<RaycastHit> raycast(Vec2 origin, Vec2 dir, float dist) const = 0;
    virtual std::vector<RaycastHit> raycastAll(Vec2 origin, Vec2 dir, float dist) const = 0;

    // --- Ground Detection ---
    virtual GroundCheckResult checkGrounded(Entity, const GroundCheckParams&) const = 0;

    // --- Callbacks ---
    virtual void setCollisionCallback(
        std::function<void(const CollisionEvent&)> cb) = 0;

    // --- World ---
    virtual void setGravity(Vec2 gravity) = 0;
    virtual Vec2 getGravity() const = 0;

    // === NEW IN V2: Joints ===
    virtual int createRevoluteJoint(Entity a, Entity b, Vec2 anchor) = 0;
    virtual int createDistanceJoint(Entity a, Entity b, float length) = 0;
    virtual int createPrismaticJoint(Entity a, Entity b, Vec2 axis) = 0;
    virtual void destroyJoint(int jointId) = 0;
    virtual void setJointMotor(int jointId, float speed, float maxForce) = 0;

    // === NEW IN V2: Continuous Collision Detection ===
    virtual void enableCCD(Entity entity, bool enabled) = 0;
};
```

---

#### 4. Particles (New System)

##### `IParticleSystem` (High-Level) -- `bestow.particles.*`

```cpp
class IParticleSystem {
public:
    static constexpr int kContractVersion = 1;
    static constexpr int kMethodCount = 8;

    virtual ~IParticleSystem() = default;

    // Spawn a particle effect from a Lua definition file at a position.
    virtual EmitterHandle spawn(const std::string& effectPath, Vec3 position) = 0;

    // Spawn an effect attached to an entity (follows it).
    virtual EmitterHandle spawnAttached(const std::string& effectPath, Entity entity) = 0;

    // Emit a one-shot burst from an existing emitter.
    virtual void burst(EmitterHandle emitter, int count) = 0;

    // Stop an emitter (existing particles finish their life).
    virtual void stop(EmitterHandle emitter) = 0;

    // Immediately destroy an emitter and all its particles.
    virtual void destroy(EmitterHandle emitter) = 0;

    // Stop all emitters.
    virtual void stopAll() = 0;

    // Check if an emitter is still active.
    virtual bool isActive(EmitterHandle emitter) const = 0;

    // Get total active particle count across all emitters.
    virtual int getActiveCount() const = 0;
};
```

##### `IParticleCore` (Low-Level) -- `bestow.particles.core.*`

```cpp
class IParticleCore {
public:
    static constexpr int kContractVersion = 1;
    static constexpr int kMethodCount = 28;

    virtual ~IParticleCore() = default;

    // --- Lifecycle ---
    virtual void initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update(DeltaTime dt) = 0;

    // --- Emitter Management ---
    virtual EmitterHandle createEmitter(const ParticleEmitterDef& def) = 0;
    virtual void destroyEmitter(EmitterHandle handle) = 0;
    virtual bool hasEmitter(EmitterHandle handle) const = 0;

    // --- Emitter Transform ---
    virtual void setEmitterPosition(EmitterHandle, Vec3 position) = 0;
    virtual void setEmitterRotation(EmitterHandle, Quat rotation) = 0;

    // --- Emitter Control ---
    virtual void startEmitting(EmitterHandle) = 0;
    virtual void stopEmitting(EmitterHandle) = 0;
    virtual void burst(EmitterHandle, int count) = 0;
    virtual bool isEmitting(EmitterHandle) const = 0;

    // --- Runtime Modification ---
    virtual void setEmissionRate(EmitterHandle, float particlesPerSecond) = 0;
    virtual void setParticleLifetime(EmitterHandle, float seconds) = 0;
    virtual void setGravityScale(EmitterHandle, float scale) = 0;
    virtual void setInitialVelocity(EmitterHandle, Vec3 velocity) = 0;
    virtual void setColorOverLifetime(EmitterHandle, Color start, Color end) = 0;
    virtual void setSizeOverLifetime(EmitterHandle, float startScale, float endScale) = 0;

    // --- Entity Attachment ---
    virtual void attachToEntity(EmitterHandle, Entity entity) = 0;
    virtual void detachFromEntity(EmitterHandle) = 0;

    // --- Effect Presets (Lua-defined) ---
    virtual ParticleEffectHandle loadEffect(const std::string& path) = 0;
    virtual void unloadEffect(ParticleEffectHandle) = 0;
    virtual EmitterHandle spawnEffect(ParticleEffectHandle, Vec3 position) = 0;

    // --- Collision ---
    virtual void enableCollision(EmitterHandle, bool enabled) = 0;
    virtual void setCollisionBounce(EmitterHandle, float bounciness) = 0;

    // --- Queries ---
    virtual int getActiveParticleCount(EmitterHandle) const = 0;
    virtual int getTotalActiveParticles() const = 0;

    // --- Render Data (consumed by Graphics3D) ---
    virtual std::span<const Vec3> getParticlePositions(EmitterHandle) const = 0;
    virtual std::span<const Color> getParticleColors(EmitterHandle) const = 0;
    virtual std::span<const float> getParticleSizes(EmitterHandle) const = 0;
};
```

---

### All Remaining Systems

Each system below lists its **System** (high-level) and **Core** (low-level) methods. New methods added in v2 are marked with `[NEW]`.

---

#### 5. Input

##### `IInputSystem` -- `bestow.input.*` (12 methods)

| Method | Description |
|--------|-------------|
| `isActionActive(name)` | Check if a named action is currently active |
| `wasActionJustPressed(name)` | Check if action was just pressed this frame |
| `wasActionJustReleased(name)` | Check if action was just released this frame |
| `getActionValue(name)` | Get analog value of an action (0-1 for axes) |
| `getActionHoldDuration(name)` | Get how long an action has been held |
| `getMousePosition()` | Get current mouse position |
| `getMouseDelta()` | Get mouse movement since last frame |
| `getLeftStick()` | Get left gamepad stick as Vec2 |
| `getRightStick()` | Get right gamepad stick as Vec2 |
| `isTextInputEnabled()` | Check if text input mode is active |
| `getTextInput()` | Get accumulated text input string |
| `getScrollDelta()` | Get mouse scroll wheel delta |

##### `IInputCore` -- `bestow.input.core.*` (35 methods)

| Method | Description |
|--------|-------------|
| `initialize()` / `shutdown()` / `update()` | Lifecycle |
| `pushPhase(name)` / `popPhase()` / `changePhase(name)` | Phase management |
| `getCurrentPhase()` / `getPhaseStack()` / `isPhaseActive(name)` | Phase queries |
| `registerAction(reg)` / `unregisterAction(name)` / `getActions()` | Action management |
| `getInputState(action)` / `getInputHoldDuration(action)` | Input state |
| `loadInputConfig(path)` / `reloadInputConfig()` | Configuration |
| `isKeyDown(key)` / `wasKeyJustPressed(key)` / `wasKeyJustReleased(key)` | Raw keyboard |
| `isGamepadButtonDown(btn)` / `wasGamepadButtonJustPressed(btn)` | Raw gamepad buttons |
| `getGamepadAxisValue(axis)` | Raw gamepad axes |
| `isMouseButtonDown(btn)` / `wasMouseButtonJustPressed(btn)` | Raw mouse buttons |
| `enableTextInput()` / `disableTextInput()` / `clearTextInput()` | Text input |
| `showMouseCursor()` / `hideMouseCursor()` / `setCursorMode(mode)` | Cursor |
| `getConnectedControllerCount()` / `isControllerConnected(idx)` | Controller |
| `[NEW] setRumble(intensity, duration)` | Haptic feedback |
| `[NEW] getRumbleSupported()` | Check haptic support |

---

#### 6. Graphics 3D

##### `IGraphics3DSystem` -- `bestow.graphics3d.*` (18 methods)

| Method | Description |
|--------|-------------|
| `drawMesh(meshPath, materialPath, transform)` | Draw a mesh with auto-loading |
| `drawModel(modelPath, transform)` | Draw a model (multi-mesh) with auto-loading |
| `setCamera(camera3D)` | Set the active camera |
| `getCamera()` | Get the current camera |
| `addLight(lightDef)` | Add a light to the scene |
| `removeLight(lightId)` | Remove a light |
| `setAmbientLight(color)` | Set ambient light color and intensity |
| `setSkybox(cubemapPath)` | Set a skybox from a cubemap path |
| `setFog(fogDef)` | Set fog parameters |
| `drawText3D(text, position, style)` | Draw 3D text |
| `screenToWorldRay(screenPos)` | Convert screen point to world ray |
| `worldToScreen(worldPos)` | Convert world point to screen coordinates |
| `enableShadows(enabled)` | Toggle shadow rendering |
| `enablePostProcessing(enabled)` | Toggle post-processing |
| `setBloom(enabled, intensity, threshold)` | Configure bloom |
| `debugDrawLine(from, to, color)` | Draw a debug line |
| `debugDrawBox(center, size, color)` | Draw a debug box |
| `debugClear()` | Clear debug draws |

##### `IGraphics3DCore` -- `bestow.graphics3d.core.*` (65+ methods)

| Category | Methods |
|----------|---------|
| **Lifecycle** | `initialize()`, `shutdown()`, `beginFrame()`, `endFrame()` |
| **Meshes** | `createMesh()`, `destroyMesh()`, `hasMesh()`, `getMeshBounds()`, `updateMeshVertices()`, `createCubeMesh()`, `createSphereMesh()`, `createCylinderMesh()`, `createCapsuleMesh()`, `createPlaneMesh()` |
| **Materials** | `createMaterial()`, `createUnlitMaterial()`, `destroyMaterial()`, `setMaterialTexture()`, `setMaterialBaseColor()`, `setMaterialMetallicRoughness()`, `setMaterialEmissive()`, `getMaterialProperties()`, `getDefaultPBRMaterial()`, `getErrorMaterial()` |
| **Render Queue** | `queueRenderItem()`, `queueRenderItems()`, `flushRenderQueue()` |
| **Entity Rendering** | `renderEntities(registry, camera)` |
| **Camera** | `setCamera()`, `getCamera()`, `setCameraTarget()`, `screenToWorldRay()`, `worldToScreen()` |
| **Lighting** | `setDirectionalLight()`, `clearDirectionalLight()`, `addPointLight()`, `addSpotLight()`, `setLightPosition()`, `removeLight()`, `clearLights()`, `setAmbientLight()` |
| **Environment** | `setSkybox()`, `clearSkybox()`, `setEnvironmentMap()`, `clearEnvironmentMap()`, `setFog()` |
| **Shadows** | `setShadowsEnabled()`, `areShadowsEnabled()`, `setDirectionalShadowResolution()`, `setShadowDistance()` |
| **Post-Processing** | `setToneMapping()`, `setExposure()`, `setBloom()`, `setSSAO()` |
| **`[NEW]`** | `setDOF(enabled, focusDist, aperture)`, `setColorGrading(lut)`, `setFXAA(enabled)`, `setTAA(enabled)` |
| **Instancing** | `createInstanceBuffer()`, `updateInstanceBuffer()`, `destroyInstanceBuffer()`, `drawInstanced()` |
| **LOD** | `setLODDistances()`, `registerLODMeshes()`, `setLODBias()` |
| **Skeletal Rendering** | `createSkeleton()`, `createAnimationClip()`, `sampleAnimation()`, `blendAnimations()`, `drawSkinnedMesh()` |
| **3D Text** | `loadFont3D()`, `destroyFont3D()`, `drawText3D()`, `measureText3D()` |
| **Debug** | `debugDrawLine()`, `debugDrawBox()`, `debugDrawSphere()`, `debugDrawCapsule()`, `debugDrawFrustum()`, `debugDrawRay()`, `debugDrawAxes()`, `debugClear()`, `setDebugRenderingEnabled()` |
| **Culling** | `setFrustumCulling()`, `isFrustumCullingEnabled()` |
| **Runtime Config** | `loadRuntimeConfig()`, `applyRuntimeConfig()`, `getRuntimeConfig()`, `reloadRuntimeConfig()` |
| **Window** | `setWindowSize()`, `getWindowMode()`, `setWindowMode()`, `shouldClose()`, `setFullscreen()` |
| **Stats** | `getStats()` |
| **`[NEW] Compute`** | `dispatchCompute(shader, groupsX, groupsY, groupsZ)`, `createStorageBuffer(size)`, `updateStorageBuffer(buf, data)`, `readStorageBuffer(buf)` |

---

#### 7. Physics 3D

##### `IPhysics3DSystem` -- `bestow.physics3d.*` (16 methods)

| Method | Description |
|--------|-------------|
| `addBody(entity, def)` | Add a physics body to an entity |
| `removeBody(entity)` | Remove a physics body |
| `applyForce(entity, force)` | Apply a continuous force |
| `applyImpulse(entity, impulse)` | Apply an instant impulse |
| `setVelocity(entity, velocity)` | Set linear velocity |
| `getVelocity(entity)` | Get linear velocity |
| `setPosition(entity, position)` | Teleport entity |
| `getPosition(entity)` | Get entity position |
| `raycast(origin, direction, distance)` | Cast a ray, get first hit |
| `overlapSphere(center, radius)` | Find all entities in a sphere |
| `overlapBox(center, halfExtents)` | Find all entities in a box |
| `setGravity(gravity)` | Set world gravity |
| `createCharacter(entity, def)` | Create a character controller |
| `moveCharacter(entity, velocity)` | Move a character controller |
| `isCharacterGrounded(entity)` | Check if character is on ground |
| `onCollision(callback)` | Set collision callback |

##### `IPhysics3DCore` -- `bestow.physics3d.core.*` (85+ methods)

All existing `IPhysics3DSystem` methods from v1 (100+), reorganized with the full Jolt API exposure: body management, shape management, constraints (hinge, slider, cone, point, distance), character controller, vehicle physics, contact queries, shape casting, mass properties, sleep management, debug draw, and statistics.

---

#### 8. Animation

##### `IAnimationSystem` -- `bestow.animation.*` (10 methods)

| Method | Description |
|--------|-------------|
| `play(entity, animationName)` | Play a named animation on an entity |
| `crossfade(entity, animationName, duration)` | Cross-fade to a new animation |
| `stop(entity)` | Stop the current animation |
| `setSpeed(entity, speed)` | Set playback speed multiplier |
| `isPlaying(entity)` | Check if an animation is playing |
| `getCurrentAnimation(entity)` | Get the name of the current animation |
| `getAnimationNames(entity)` | Get all available animations for an entity |
| `setParameter(entity, name, value)` | Set a state machine parameter (bool/int/float) |
| `trigger(entity, name)` | Fire a trigger parameter |
| `getProgress(entity)` | Get normalized playback progress (0-1) |

##### `IAnimationCore` -- `bestow.animation.core.*` (40+ methods)

All existing skeleton, clip, blending, IK, socket, and state machine methods plus:

| New Method | Description |
|------------|-------------|
| `[NEW] enableRootMotion(entity, enabled)` | Enable root motion extraction |
| `[NEW] getRootMotionDelta(entity)` | Get root motion translation/rotation this frame |
| `[NEW] createBlendSpace1D(name, clips, thresholds)` | Create a 1D blend space |
| `[NEW] createBlendSpace2D(name, clips, positions)` | Create a 2D blend space |
| `[NEW] sampleBlendSpace(handle, paramValue)` | Sample a blend space at a parameter |
| `[NEW] addAdditiveLayer(entity, animName, weight)` | Add additive animation layer |
| `[NEW] removeAdditiveLayer(entity, layerIndex)` | Remove additive layer |
| `[NEW] setMorphTarget(entity, targetName, weight)` | Set blend shape weight |
| `[NEW] retargetAnimation(sourceEntity, targetEntity, animName)` | Retarget animation between skeletons |

---

#### 9. AI

##### `IAISystem` -- `bestow.ai.*` (10 methods)

| Method | Description |
|--------|-------------|
| `moveTo(entity, targetPosition)` | Navigate entity to a position |
| `stopMovement(entity)` | Stop navigation |
| `isMoving(entity)` | Check if entity is navigating |
| `patrol(entity, points)` | Set a patrol route |
| `findPath(from, to)` | Find a path between two points |
| `findNearest(position, radius, filter)` | Find nearest entity matching filter |
| `hasLineOfSight(from, to)` | Check line of sight between entities |
| `setBehavior(entity, treePath)` | Load and attach a behavior tree |
| `setBlackboard(entity, key, value)` | Set a blackboard value |
| `getBlackboard(entity, key)` | Get a blackboard value |

##### `IAICore` -- `bestow.ai.core.*` (25+ methods)

All existing behavior tree, navmesh, steering, and spatial query methods plus:

| New Method | Description |
|------------|-------------|
| `[NEW] setRVOAgent(entity, radius, maxSpeed)` | Register entity for reciprocal velocity obstacle avoidance |
| `[NEW] removeRVOAgent(entity)` | Remove from RVO simulation |
| `[NEW] setPreferredVelocity(entity, velocity)` | Set preferred movement velocity for RVO |
| `[NEW] getAvoidanceVelocity(entity)` | Get the collision-free velocity from RVO |

---

#### 10. Network (New)

##### `INetworkSystem` -- `bestow.network.*` (12 methods)

| Method | Description |
|--------|-------------|
| `host(port, maxClients)` | Start a server |
| `connect(address, port)` | Connect to a server |
| `disconnect()` | Disconnect |
| `isConnected()` | Check connection status |
| `isServer()` | Check if running as server |
| `getClientId()` | Get local client ID |
| `callServer(rpcName, args)` | Call an RPC on the server |
| `callClient(rpcName, clientId, args)` | Call an RPC on a specific client |
| `callAll(rpcName, args)` | Call an RPC on all clients |
| `onRPC(rpcName, handler)` | Register an RPC handler |
| `onPlayerJoined(callback)` | Register join callback |
| `onPlayerLeft(callback)` | Register leave callback |

##### `INetworkCore` -- `bestow.network.core.*` (30+ methods)

| Category | Methods |
|----------|---------|
| **Lifecycle** | `initialize()`, `shutdown()`, `update()`, `sendUpdates()` |
| **Connection** | `host(config)`, `connect(config)`, `disconnect()`, `getRole()`, `getConnectionState()`, `getLocalClientId()` |
| **Client Management** | `getConnectedClients()`, `kickClient(id, reason)` |
| **Entity Replication** | `registerNetworkEntity(entity, netId)`, `unregisterNetworkEntity(entity)`, `getNetworkId(entity)`, `getEntityByNetworkId(netId)`, `setEntityOwner(entity, clientId)`, `getEntityOwner(entity)`, `isLocalAuthority(entity)` |
| **Property Replication** | `registerReplicatedProperty(prop)`, `markDirty(entity, component)` |
| **RPCs** | `registerRPC(name, handler)`, `callServerRPC(id, args)`, `callClientRPC(id, target, args)`, `callMulticastRPC(id, args)` |
| **Callbacks** | `onClientConnected(cb)`, `onClientDisconnected(cb)`, `unsubscribe(id)` |
| **Statistics** | `getRoundTripTime()`, `getPacketLoss()`, `getBytesSentPerSecond()`, `getBytesReceivedPerSecond()` |
| **`[NEW] Prediction`** | `enablePrediction(entity)`, `disablePrediction(entity)`, `reconcile(entity, serverState)` |
| **`[NEW] Interest`** | `setRelevanceRadius(entity, radius)`, `setAlwaysRelevant(entity, bool)` |

---

#### 11. Config

##### `IConfigSystem` -- `bestow.config.*` (12 methods)

| Method | Description |
|--------|-------------|
| `get(key, default)` | Get a value with type inference and default |
| `getFloat(key, default)` | Get a float |
| `getInt(key, default)` | Get an integer |
| `getBool(key, default)` | Get a boolean |
| `getString(key, default)` | Get a string |
| `has(key)` | Check if a key exists |
| `set(key, value)` | Set a runtime value |
| `getArray(key)` | Get an array value |
| `[NEW] getTable(key)` | Get a nested table (returns Lua table) |
| `[NEW] getKeysIn(prefix)` | Get all keys under a prefix |
| `onChange(key, callback)` | Subscribe to changes on a specific key |
| `onAnyChange(callback)` | Subscribe to any config change |

##### `IConfigCore` -- `bestow.config.core.*` (20 methods)

All existing `IConfigSystem` methods from v1: `loadConfig()`, `loadConfigAsset()`, `reloadAll()`, `reloadConfig()`, type-specific getters/setters with full variant support, `hasKey()`, `getKeysWithPrefix()`, `getLoadedConfigs()`, `enableHotReload()`, `parseLuaString()`, `getLuaState()`, metadata queries.

---

#### 12. UI

##### `IUISystem` -- `bestow.ui.*` (14 methods)

| Method | Description |
|--------|-------------|
| `show(documentPath)` | Load and show a UI document |
| `hide(documentPath)` | Hide a loaded document |
| `isVisible(documentPath)` | Check visibility |
| `setText(elementId, text)` | Set text content of an element |
| `getText(elementId)` | Get text content |
| `addClass(elementId, className)` | Add a CSS class |
| `removeClass(elementId, className)` | Remove a CSS class |
| `setStyle(elementId, property, value)` | Set an inline style |
| `setVisible(elementId, visible)` | Show/hide an element |
| `onClick(elementId, callback)` | Register a click handler |
| `onEvent(elementId, eventName, callback)` | Register any event handler |
| `bind(key, value)` | Bind a data value for data binding |
| `sync()` | Sync all data bindings to the DOM |
| `[NEW] setAttribute(elementId, attr, value)` | Set an HTML attribute |

##### `IUICore` -- `bestow.ui.core.*` (40+ methods)

All existing `IUISystem` methods from v1: full DOM manipulation (createElement, appendChild, removeElement, setInnerRml), document management (loadDocument, loadDocumentFromString, unloadDocument), stylesheet management, data binding (bindData with 4 variants), element queries (getElementById, getElementsByClass, getElementsByTag), input processing (processInput, wantsKeyboardInput), font loading, debug mode, viewport/DPI.

---

#### 13. State

##### `IStateSystem` -- `bestow.state.*` (14 methods)

| Method | Description |
|--------|-------------|
| `set(key, value)` | Set any value (number, string, bool, table) |
| `get(key, default)` | Get a value with default |
| `has(key)` | Check if key exists |
| `remove(key)` | Delete a key |
| `save(slotName)` | Save current state to a named slot |
| `load(slotName)` | Load state from a named slot |
| `deleteSave(slotName)` | Delete a save slot |
| `hasSave(slotName)` | Check if a save slot exists |
| `getSaves()` | List all save slots with metadata |
| `quickSave()` | Save to the quick save slot |
| `quickLoad()` | Load from the quick save slot |
| `setProfile(name)` | Set the active profile |
| `getProfile()` | Get the active profile name |
| `getPlaytime()` | Get session play time in seconds |

##### `IStateCore` -- `bestow.state.core.*` (25 methods)

All existing `IStateSystem` methods from v1: typed setters/getters (setNumber, setString, setBool, setJsonData), commit/restore, auto-commit, slot metadata, profiles, playtime/version tracking, format migration, JSON export/import, stateful registration.

---

#### 14. GAS (Gameplay Ability System)

##### `IGASSystem` -- `bestow.gas.*` (16 methods)

| Method | Description |
|--------|-------------|
| `init(entity)` | Initialize GAS for an entity |
| `setAttribute(entity, name, value)` | Set an attribute value |
| `getAttribute(entity, name)` | Get current attribute value |
| `getBaseAttribute(entity, name)` | Get base (unmodified) attribute value |
| `applyEffect(entity, effectName)` | Apply a named effect |
| `removeEffect(entity, effectId)` | Remove an active effect |
| `hasEffect(entity, effectName)` | Check for an active effect |
| `grantAbility(entity, abilityName)` | Grant an ability |
| `activateAbility(entity, abilityName)` | Try to activate an ability |
| `endAbility(entity, abilityName)` | End an active ability |
| `hasAbility(entity, abilityName)` | Check for a granted ability |
| `addTag(entity, tagName)` | Add a gameplay tag |
| `removeTag(entity, tagName)` | Remove a gameplay tag |
| `hasTag(entity, tagName)` | Check for a gameplay tag |
| `onAttributeChanged(entity, name, callback)` | Watch attribute changes |
| `loadDefinitions(luaPath)` | Load GAS definitions from a Lua file |

##### `IGASCore` -- `bestow.gas.core.*` (35 methods)

All existing `IGASSystem` methods from v1: tag registration/hierarchy, attribute definitions, effect definitions with modifier operations, ability definitions with activation policies and costs, component management, tag containers, active effect management, ability cooldown tracking, callbacks for attribute/effect/ability events.

---

#### 15. Graphics 2D

##### `IGraphicsSystem` -- `bestow.graphics.*` (14 methods)

| Method | Description |
|--------|-------------|
| `drawSprite(path, x, y)` | Draw a sprite at a position |
| `drawRect(x, y, w, h, color)` | Draw a filled rectangle |
| `drawLine(x1, y1, x2, y2, color)` | Draw a line |
| `drawCircle(x, y, radius, color)` | Draw a circle |
| `drawText(text, x, y, font, size)` | Draw text |
| `setCamera(camera)` | Set the 2D camera |
| `getCamera()` | Get the 2D camera |
| `worldToScreen(worldPos)` | Convert world to screen coordinates |
| `screenToWorld(screenPos)` | Convert screen to world coordinates |
| `setClearColor(color)` | Set background clear color |
| `renderEntities()` | Render all entities with sprite components |
| `[NEW] drawSpriteEx(path, x, y, opts)` | Draw with rotation, scale, tint, flip |
| `[NEW] setBlendMode(mode)` | Set blend mode for subsequent draws |
| `[NEW] enableLight2D(enabled)` | Toggle 2D lighting |

##### `IGraphicsCore` -- `bestow.graphics.core.*` (20+ methods)

All existing methods: frame management (beginFrame/endFrame), batch drawing, animated sprites, text measurement, polygon drawing, window management, viewport culling, fullscreen, VSync.

New:
| Method | Description |
|--------|-------------|
| `[NEW] addLight2D(position, radius, color, intensity)` | Add a 2D light |
| `[NEW] removeLight2D(lightId)` | Remove a 2D light |
| `[NEW] setLight2DPosition(lightId, position)` | Move a 2D light |
| `[NEW] setAmbientLight2D(color)` | Set 2D ambient light |

---

#### Single-Contract Systems

These systems are simple enough that one contract suffices. No `.core` path.

##### Events (`IEventSystem`) -- `bestow.events.*` (8 methods)

| Method | Description |
|--------|-------------|
| `subscribe(eventName, callback)` | Subscribe to an event |
| `unsubscribe(subscriptionId)` | Unsubscribe |
| `publish(eventName, data)` | Publish an event immediately |
| `queue(eventName, data)` | Queue an event for next processQueue |
| `publishLocal(eventName, data)` | `[NEW]` Publish only to same-system subscribers |
| `once(eventName, callback)` | `[NEW]` Subscribe for a single event then auto-unsubscribe |
| `hasSubscribers(eventName)` | `[NEW]` Check if anyone is listening |
| `unsubscribeAll(owner)` | Remove all subscriptions for an owner |

##### Camera (`ICameraSystem`) -- `bestow.camera.*` (16 methods)

All existing: setTarget, clearTarget, getTarget, setFollowSmoothing, setOffset, setDeadzone, setBounds, clearBounds, shake, stopShake, setZoom, getZoom, getPosition, screenToWorld, worldToScreen, update.

New (absorbed from Graphics3D):

| Method | Description |
|--------|-------------|
| `[NEW] lockOn(config)` | Initiate lock-on targeting |
| `[NEW] unlock()` | Release lock-on |
| `[NEW] isLocked()` | Check if locked on |
| `[NEW] getLockTarget()` | Get the locked entity |
| `[NEW] shiftLockTarget(direction)` | Shift to next/prev target |
| `[NEW] getPotentialTargets()` | Get all lockable targets in range |

##### Tween (`ITweenSystem`) -- `bestow.tween.*` (16 methods)

| Method | Description |
|--------|-------------|
| `to(from, to, duration, easing, onUpdate)` | Tween a float value |
| `toVec2(from, to, duration, easing, onUpdate)` | Tween a Vec2 |
| `toVec3(from, to, duration, easing, onUpdate)` | Tween a Vec3 |
| `toColor(from, to, duration, easing, onUpdate)` | Tween a Color |
| `moveEntity(entity, target, duration, easing)` | Move entity to position |
| `rotateEntity(entity, target, duration, easing)` | Rotate entity |
| `scaleEntity(entity, target, duration, easing)` | Scale entity |
| `pause(handle)` | Pause a tween |
| `resume(handle)` | Resume a tween |
| `cancel(handle)` | Cancel a tween |
| `complete(handle)` | Jump to end value |
| `onComplete(handle, callback)` | Register completion callback |
| `sequence(tweens)` | Run tweens in sequence |
| `parallel(tweens)` | Run tweens simultaneously |
| `cancelAll()` | Cancel all active tweens |
| `getActiveCount()` | Get number of active tweens |

##### Scene (`ISceneSystem`) -- `bestow.scene.*` (9 methods)

| Method | Description |
|--------|-------------|
| `push(sceneName, params)` | Push a scene onto the stack |
| `pop()` | Pop the top scene |
| `replace(sceneName, params)` | Replace the top scene |
| `clear()` | Clear the entire scene stack |
| `getActive()` | Get the active scene name |
| `getStack()` | Get the full scene stack |
| `register(name, callbacks)` | Register a scene with enter/exit/update/render |
| `unregister(name)` | Unregister a scene |
| `getRegistered()` | List all registered scene names |

##### Game State (`IGameStateSystem`) -- `bestow.gamestate.*` (10 methods)

| Method | Description |
|--------|-------------|
| `push(stateName)` | Push a state |
| `pop()` | Pop the current state |
| `replace(stateName)` | Replace the current state |
| `clear()` | Clear all states |
| `getCurrent()` | Get current state name |
| `getCount()` | Get stack depth |
| `isEmpty()` | Check if state stack is empty |
| `isTransitioning()` | Check if a transition is in progress |
| `registerFactory(name, factory)` | Register a state factory |
| `setChangeCallback(callback)` | Register state change callback |

##### Blueprints (`IBlueprintFactory`) -- `bestow.blueprints.*` (7 methods)

| Method | Description |
|--------|-------------|
| `create(blueprintName)` | Create an entity from a blueprint |
| `createAt(blueprintName, position)` | Create at a specific position |
| `createWithOverrides(name, overrides)` | Create with property overrides |
| `has(blueprintName)` | Check if a blueprint exists |
| `getNames()` | List all blueprint names |
| `reload()` | Reload all blueprint definitions |
| `getBlueprint(name)` | Get the blueprint definition table |

---

## Appendix: Method Count Summary

| System | System Methods | Core Methods | Total | Lua Paths |
|--------|---------------|-------------|-------|-----------|
| Audio | 10 | 38 | 48 | `bestow.audio.*`, `.core.*` |
| Entity | 12 | 24 | 36 | `bestow.entity.*`, `.core.*` |
| Input | 12 | 35 | 47 | `bestow.input.*`, `.core.*` |
| Graphics 3D | 18 | 65+ | 83+ | `bestow.graphics3d.*`, `.core.*` |
| Graphics 2D | 14 | 20+ | 34+ | `bestow.graphics.*`, `.core.*` |
| Physics 2D | 16 | 30 | 46 | `bestow.physics.*`, `.core.*` |
| Physics 3D | 16 | 85+ | 101+ | `bestow.physics3d.*`, `.core.*` |
| Animation | 10 | 40+ | 50+ | `bestow.animation.*`, `.core.*` |
| AI | 10 | 25+ | 35+ | `bestow.ai.*`, `.core.*` |
| UI | 14 | 40+ | 54+ | `bestow.ui.*`, `.core.*` |
| State | 14 | 25 | 39 | `bestow.state.*`, `.core.*` |
| Config | 12 | 20 | 32 | `bestow.config.*`, `.core.*` |
| GAS | 16 | 35 | 51 | `bestow.gas.*`, `.core.*` |
| Particles (NEW) | 8 | 28 | 36 | `bestow.particles.*`, `.core.*` |
| Network (NEW) | 12 | 30+ | 42+ | `bestow.network.*`, `.core.*` |
| Events | 8 | -- | 8 | `bestow.events.*` |
| Camera | 22 | -- | 22 | `bestow.camera.*` |
| Tween (NEW) | 16 | -- | 16 | `bestow.tween.*` |
| Scene | 9 | -- | 9 | `bestow.scene.*` |
| Game State | 10 | -- | 10 | `bestow.gamestate.*` |
| Blueprints | 7 | -- | 7 | `bestow.blueprints.*` |
| **TOTAL** | **~266** | **~540+** | **~806+** | **21 systems, 36 Lua namespaces** |

### Comparison: V1 vs V2

| Metric | V1 | V2 |
|--------|----|----|
| Contracts (C++ interfaces) | 26 | 36 (21 System + 15 Core) |
| Lua namespaces | 19 (no AI, Camera bindings) | 36 (all systems, System + Core paths) |
| Documented methods | ~500 (in binding docs) | 806+ (all methods documented) |
| Sources of truth | 3 (contract, binding doc, stubs) | 1 (SystemDefinition files) |
| Compile-time doc validation | None | static_assert on version + count |
| Doc-tests | None | Every method example is runnable |
| Missing Lua bindings | AI, Camera, Graphics2D, Tween | 0 |
| New features vs competitors | -- | Particles, Network, Tween, 2D Lighting, Audio FX, Joints, Compute, RVO, Root Motion, Blend Spaces |
