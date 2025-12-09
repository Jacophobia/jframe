# Bestow Development Guidelines

> **Note:** This file contains **principles and guidelines** for programming in this repository. For future work, improvement plans, and progress tracking, see `TODO.md` at the project root.

## User Preferences

**Keyboard Layout:** The project owner uses **Dvorak**. When implementing keyboard controls, **STRONGLY prefer ,AOE over WASD** for movement (Dvorak-equivalent of WASD positions). This applies to all demos, examples, and default configurations.

**Renderer Priority:** **Vulkan is the primary renderer**. OpenGL exists primarily for rapid prototyping and fallback purposes. When implementing graphics features:
- Prioritize Vulkan (`bestow-vulkan`) implementation first
- OpenGL (`bestow-graphics3d`) is secondary and may lag behind in features
- Vulkan should be the most polished and feature-complete backend

**Lua-First Design:** Lua is the primary interface for game developers using Bestow. Configuration, materials, levels, blueprints, and game logic should be Lua-driven wherever possible. Game developers should spend most of their time in Lua files, not C++.

## Language Standard

**C++23 Required** - Use modern C++ features throughout the codebase.

### Required C++23 Features

- `std::expected` for error handling without exceptions
- `std::ranges` for modern iteration patterns
- `std::format` for string formatting (via fmt fallback)
- `std::span` for non-owning views
- Deducing `this` for CRTP simplification
- `if constexpr` with lambdas
- `std::unreachable()` for impossible code paths

### Compiler Requirements

**CRITICAL:** Bestow uses `import std;` which requires **LLVM Clang 20+** on macOS. Apple Clang (Xcode) does not yet support `import std;`.

| Platform | Compiler | Minimum Version | Notes |
|----------|----------|-----------------|-------|
| macOS | **LLVM Clang** | **20.0+** | **Required** - Install via Homebrew (`brew install llvm@20`) |
| macOS | ~~Apple Clang~~ | ~~N/A~~ | Does not support `import std;` |
| Windows | MSVC | 19.38+ (VS 2022 17.8+) | With `/std:c++latest` |
| Windows | Clang-CL | 17.0+ | |
| Linux | GCC | 13.0+ | |
| Linux | Clang | 17.0+ | |

**Setup Instructions:**
- **macOS:** See `docs/LLVM20-SETUP.md` for LLVM 20 installation and configuration
- **All Platforms:** Use `CMakePresets.json` to configure the correct compiler

**Important:** All source files must use `import std;` consistently. Do NOT mix `#include <standard_header>` with `import std;` in the same module or translation unit.

## Build System

**CMake 3.28+** required for C++23 module support.

```bash
# Configure (macOS)
cmake --preset macos-debug

# Build
cmake --build --preset macos-debug

# Test
ctest --preset macos-debug
```

## C++ Modules

Bestow uses C++23 modules. Follow these conventions:

### Module Naming

```
bestow.types          // Core types and aliases
bestow.core           // Core utilities (Timer, FrameTimer, Easing, JobSystem, Logging)
bestow.config         // IConfigSystem interface
bestow.entity         // IEntitySystem interface
bestow.graphics       // IGraphicsSystem interface
bestow.audio          // IAudioSystem interface
bestow.input          // IInputSystem interface
bestow.assets         // IAssetSystem interface
bestow.save           // ISaveSystem interface
bestow.level          // ILevelSystem interface
bestow.events         // IEventSystem interface
bestow.physics        // IPhysicsSystem interface
bestow.ai             // IAISystem interface
bestow.dev            // Development tools (Debug builds only)
bestow                // Primary module (re-exports all)
```

### File Extensions

| Compiler | Interface Unit | Implementation Unit |
|----------|----------------|---------------------|
| Clang | `.cppm` | `.cpp` |
| MSVC | `.ixx` | `.cpp` |
| GCC | `.cppm` | `.cpp` |

### Module Structure

```cpp
// Global module fragment - include third-party headers here
module;

#include <third_party_header.hpp>

// Module declaration
export module bestow.modulename;

// Standard library imports
import std;

export namespace bestow {
    // Exported declarations
}
```

### Import Patterns

```cpp
// Import everything
import bestow;

// Import specific modules
import bestow.entity;
import bestow.graphics;

// In game code
import bestow;
import bestow.dev;  // Only available in debug builds
```

## Architecture Principles

### Program to Interfaces

All systems implement abstract interfaces from `bestow-contract`:

```cpp
class IEntitySystem {
public:
    virtual ~IEntitySystem() = default;
    virtual Entity createEntity() = 0;
    // ...
};
```

### Composition Over Inheritance

Use ECS architecture with EnTT. Prefer components over class hierarchies.

### Data-Driven Design

- Blueprints, levels, and behaviors defined in Lua
- Settings and save files in JSON
- Binary serialization with cereal for game saves

### Dependency Injection

Use Fruit DI for system wiring in the composition root.

### AssetSystem Architecture (CRITICAL)

**The AssetSystem is the SOLE GATEWAY to the file system.** No other system should directly read files. This is a fundamental architectural principle.

#### The Rule

**ALL file system interactions MUST go through AssetSystem.** This includes:
- Loading shaders (GLSL, SPIR-V)
- Loading Lua files (configs, materials, blueprints, levels)
- Loading textures, audio, fonts, meshes, models
- Reading any data files
- Checking file existence
- Monitoring file modification times

**NEVER do these in other systems:**
```cpp
// WRONG - Direct file I/O
std::ifstream file(path);
lua.safe_script_file(path);
std::filesystem::exists(path);
std::filesystem::last_write_time(path);
```

```cpp
// CORRECT - Through AssetSystem
AssetHandle handle = assets->registerAsset(AssetType::Shader, "shaders/toon.frag");
assets->loadAsset(handle);
const ShaderData* data = assets->getAsset<ShaderData>(handle);
```

#### Why This Matters

1. **Centralized caching** - Assets loaded once, reused everywhere
2. **Hot reload support** - AssetSystem monitors files via efsw
3. **Lifecycle management** - Assets properly loaded/unloaded
4. **Path resolution** - `:library:/` and `:assets:/` prefixes handled consistently
5. **Async loading** - Non-blocking asset loads on background threads
6. **Error handling** - Unified error reporting for all file operations

#### Correct Asset Workflow

```cpp
// 1. Register the asset (doesn't load yet)
AssetHandle handle = assets->registerAsset(AssetType::Shader, ":library:/shaders/toon.frag");

// 2. Subscribe to changes for hot reload (via AssetSystem subscriptions)
SubscriptionId subId = assets->subscribe(handle, [this](AssetHandle h, AssetType t) {
    onAssetChanged(h, t);  // Called when file changes on disk
});

// Or subscribe to ALL assets of a type:
SubscriptionId typeSubId = assets->subscribeToType(AssetType::Shader, [this](AssetHandle h, AssetType t) {
    onShaderChanged(h, t);  // Called when ANY shader changes
});

// 3. Load the asset (sync or async)
assets->loadAssetAsync(handle, [this](AssetHandle h, AssetState state) {
    if (state == AssetState::Loaded) {
        const ShaderData* shader = assets->getAsset<ShaderData>(h);
        // Use the shader data
    }
});

// 4. In your update loop, process async completions
void update() {
    assets->update();  // Processes async loads and hot reload notifications
}

// 5. When done, clean up
assets->unsubscribe(subId);
assets->unloadAsset(handle);
```

#### Hot Reload Flow

1. Enable hot reload: `assets->enableHotReload(true)`
2. AssetSystem uses efsw to watch registered asset directories
3. When a file changes, efsw queues the event
4. On `assets->update()`, queued changes are processed
5. `reloadAsset()` reloads the file data
6. All subscribers are notified via their callbacks
7. Consuming systems (Graphics, Shader, etc.) receive notification and update

#### Systems That Must Use AssetSystem

| System | Asset Types |
|--------|-------------|
| Graphics/Vulkan/OpenGL | Shaders, Textures, Materials, Meshes, Models, Cubemaps |
| Shader | Shader source files (.glsl, .vert, .frag), Material definitions (.lua) |
| Config | Config files (.lua) |
| Audio | Sound files (.wav, .ogg, .mp3), Music files |
| Level | Level definitions (.lua), Blueprints (.lua) |
| Save | N/A - see "Intentional Exceptions" below |
| UI | RML documents, fonts, stylesheets |

#### Intentional Exceptions

**SaveSystem** is an intentional exception to the AssetSystem rule because:

1. **User data vs game assets**: Save files are USER-generated data, not game assets bundled with the application
2. **Write operations required**: IAssetSystem is READ-ONLY by design. SaveSystem needs `std::ofstream` for writing saves
3. **No hot reload needed**: Users don't modify save files while the game runs
4. **Different lifecycle**: Save files are created, modified, and deleted by the user during gameplay

The SaveSystem MAY use direct file I/O (`std::ifstream`, `std::ofstream`, `std::filesystem`) for save operations.

```cpp
// SaveSystem exception - this is ACCEPTABLE:
std::ofstream saveFile(savePath, std::ios::binary);
cereal::BinaryOutputArchive archive(saveFile);
archive(saveData);
```

### EventSystem Architecture

**Prefer EventSystem over direct callbacks for inter-system communication.** This provides decoupling, testability, and flexibility.

#### When to Use EventSystem

Use EventSystem for:
- Cross-system notifications (asset loaded, level changed, entity created)
- Decoupled communication where the sender doesn't need to know about receivers
- Events with multiple potential subscribers

Use direct callbacks for:
- Performance-critical paths where event dispatch overhead matters
- Simple 1:1 relationships within the same system

#### Benefits

1. **Decoupling**: Systems don't need references to each other
2. **Testability**: Easy to mock events in unit tests
3. **Flexibility**: Multiple subscribers to same event
4. **Debugging**: Central place to log all system communication

#### Example Usage

```cpp
// Subscribe to events in init()
eventSubId_ = events_->subscribe(Events::AssetLoaded, [this](const EventData& data) {
    auto& event = std::get<AssetLoadedEvent>(data);
    if (event.type == AssetType::Shader) {
        onShaderLoaded(event.handle);
    }
});

// Clean up in shutdown
events_->unsubscribe(eventSubId_);
```

## Error Handling

Use `std::expected` instead of exceptions:

```cpp
template<typename T, typename E = std::error_code>
using Result = std::expected<T, E>;

Result<Entity, std::error_code> createPlayerEntity();
```

## Code Style

### Formatting

Run `clang-format` before committing. Configuration is in `.clang-format`:

- 4 space indentation
- 100 character line limit
- Attach braces
- LLVM base style

### Static Analysis

Run `clang-tidy` for static analysis. Configuration is in `.clang-tidy`.

### Naming Conventions

```cpp
// Types: PascalCase
class EntitySystem;
struct TransformComponent;
enum class AssetType;

// Functions/Methods: camelCase
void createEntity();
bool isValid() const;

// Variables: camelCase
Entity playerEntity;
float deltaTime;

// Constants: SCREAMING_SNAKE_CASE or camelCase with 'k' prefix
constexpr int MAX_ENTITIES = 10000;
constexpr float kDefaultGravity = -980.0f;

// Namespaces: lowercase
namespace bestow::events { }

// Member variables: trailing underscore
class Example {
    int value_;
    std::string name_;
};
```

## Data Formats

| Data Type | Format | Reason |
|-----------|--------|--------|
| Blueprints | Lua | Inheritance, functions, reusable components |
| Levels | Lua | Loops, patterns, procedural placement |
| Game Config | Lua | Variables, computed values |
| User Settings | JSON | Simple key-value, runtime-saved |
| Save Files | Binary (cereal) | Fast, compact, versioned |

## Lua Usage

### Why Lua Instead of JSON

- Comments allowed (`-- comment`)
- Trailing commas OK
- Variables (`local GROUND_Y = 100`)
- Loops (`for i = 1, 10 do ... end`)
- Functions (`makeEnemyWave(x, count)`)
- Math (`math.sin(i) * 100`)
- Conditionals (`DEBUG and {...} or {}`)

### Sandboxing

Always sandbox Lua execution. Remove dangerous functions:

```cpp
lua["os"] = sol::nil;
lua["io"] = sol::nil;
lua["loadfile"] = sol::nil;
lua["dofile"] = sol::nil;
lua["load"] = sol::nil;
```

## Development Tools

Dev tools are available only in Debug/RelWithDebInfo builds:

```cpp
#if defined(BESTOW_DEV_TOOLS)
    hotReload_.update();
    devOverlay_.render();
#endif
```

### Hot Reload

| File Type | Behavior |
|-----------|----------|
| `blueprints/*.lua` | Re-execute Lua, update blueprint registry |
| `levels/*.lua` | Re-execute Lua, reload current level |
| `textures/*` | Reload texture in GPU |
| `audio/*` | Reload sound data |
| `config/*.lua` | Re-execute Lua, apply immediately |

## Testing

Use Google Test for unit and integration tests:

```cpp
#include <gtest/gtest.h>

TEST(EntitySystemTest, CreateEntity) {
    EntitySystem system;
    Entity e = system.createEntity();
    EXPECT_TRUE(system.isValid(e));
}
```

### Testing Workflow

Bestow follows a two-wave testing approach to ensure quality and catch issues early:

#### Wave 1: Test-Driven Development (Define Expected Behavior)

1. **Agents write tests first** - Before implementing features, write tests that:
   - Compile successfully
   - Define the expected behavior of the system
   - May fail initially (this is expected and correct)
   - Cover all interface methods and critical paths

2. **Tests as specifications** - Failing tests serve as:
   - Documentation of what needs to be implemented
   - Validation that the test infrastructure is working
   - A checklist of incomplete functionality

3. **Orchestrator commits** - After Wave 1, the orchestrator commits all tests to establish a baseline

#### Wave 2: Implementation and Test Fixing

1. **Analyze failures** - For each failing test, determine:
   - Is it a test bug? (Wrong assertions, incorrect setup, bad assumptions)
   - Is it an implementation bug? (Missing features, incorrect behavior)
   - Is it a documentation issue? (Interface contract unclear)

2. **Fix systematically** - Address failures in priority order:
   - Critical path functionality first
   - Edge cases second
   - Nice-to-have features last

3. **Verify fixes** - After fixing:
   - Run the specific test to confirm it passes
   - Run related tests to check for regressions
   - Document any assumptions or limitations

#### End-of-Wave Validation

At the completion of each development wave, agents perform a **swarming review**:

1. **Check for stubs** - Search for:
   ```bash
   grep -r "// TODO" bestow-yoursystem/src/
   grep -r "return.*;" bestow-yoursystem/src/  # Empty returns
   grep -r "throw.*NotImplemented" bestow-yoursystem/src/
   ```

2. **Verify all requirements** - Confirm:
   - All interface methods are fully implemented (not stubbed)
   - All tests pass
   - No placeholder code remains
   - Documentation is complete

3. **Handle blockers** - If something cannot be implemented:
   - Document the reason in comments
   - Add to `/TODO.md` at project root (see Stub/TODO Policy below)
   - Notify orchestrator for coordination

#### Running Tests

```bash
# Run all tests
ctest --preset macos-debug

# Run specific system tests
ctest --preset macos-debug -R "EntitySystemTest"

# Run with verbose output
ctest --preset macos-debug --output-on-failure

# Run in parallel
ctest --preset macos-debug -j8
```

### Stub and TODO Policy

**Stubs and TODO comments indicate incomplete work.** All systems must be fully implemented before being marked as complete.

#### What Counts as Incomplete

- **Stub functions** - Methods that compile but do nothing:
  ```cpp
  void doSomething() override {
      // TODO: Implement
  }
  ```

- **Placeholder returns** - Returning default values without logic:
  ```cpp
  Entity createEntity() override {
      return Entity{};  // Stub - not creating real entity
  }
  ```

- **TODO comments** - Any comment indicating missing functionality:
  ```cpp
  // TODO: Add collision filtering
  // FIXME: Memory leak here
  // HACK: Temporary workaround
  ```

#### Resolution Requirements

Before a system is considered complete:

1. **All stubs must be resolved** - Every interface method must have a real implementation
2. **All TODOs must be addressed** - Either implement the feature or document why it's deferred
3. **All tests must pass** - No skipped or failing tests

#### When Something Cannot Be Implemented

If a feature truly cannot be implemented due to external dependencies or technical limitations:

1. **Document in code** - Add a detailed comment explaining:
   ```cpp
   // NOTE: Hot reload for FMOD sounds is not supported by FMOD API.
   // Workaround: Unload and reload the sound manually.
   ```

2. **Add to project TODO** - Create or update `/TODO.md` at project root:
   ```markdown
   ## Deferred Features

   ### Audio System
   - [ ] Hot reload for FMOD sounds - Blocked by FMOD API limitation
   - [ ] Spatial audio reverb - Requires FMOD Studio (not Core)
   ```

3. **Notify orchestrator** - Flag the issue for project-level decision:
   - Is this a critical feature?
   - Should we switch libraries?
   - Can we defer to a future version?

#### Checking for Incomplete Work

```bash
# Find all TODOs in a system
grep -rn "// TODO\|// FIXME\|// HACK" bestow-yoursystem/src/

# Find stub functions (empty or single-line implementations)
# Manual review required - look for minimal implementations

# Run static analysis
clang-tidy bestow-yoursystem/src/*.cpp

# Verify test coverage
ctest --preset macos-debug -R "YourSystemTest" --verbose
```

## Profiling

Tracy integration is available with `BESTOW_ENABLE_TRACY`:

```cpp
#include <tracy/Tracy.hpp>

void update(DeltaTime dt) {
    ZoneScoped;
    // ...
}
```

## Key Dependencies

| Library | Version | Purpose | System |
|---------|---------|---------|--------|
| **Core Libraries** |
| GLFW | 3.3+ | Windowing/Input | Graphics |
| SDL2 | 2.28+ | Game Controllers | Input |
| glad | 0.1.36+ | OpenGL Loader | Graphics |
| glm | 0.9.9+ | Math | Core/All |
| spdlog | 1.12+ | Logging | Core |
| nlohmann_json | 3.11+ | JSON Parsing | Save/Assets |
| **Rendering** |
| stb_image | Latest | Image Loading | Assets |
| FreeType | 2.13+ | Font Rasterization | Graphics |
| msdf-atlas-gen | Latest | MSDF Font Atlas Generation | Graphics |
| **Scripting** |
| Lua | 5.4+ | Scripting | Level |
| sol2 | 3.3+ | Lua C++ Bindings | Level |
| **Game Systems** |
| EnTT | 3.12+ | ECS | Entity |
| Box2D | 3.1+ | 2D Physics | Physics |
| FMOD Core | 2.02+ | Audio Engine | Audio |
| **Serialization** |
| cereal | 1.3+ | Binary Serialization | Save |
| zstd | 1.5+ | Compression | Save |
| **AI** |
| BehaviorTree.CPP | 4.5+ | Behavior Trees | AI |
| Recast | Latest | Navigation Mesh Generation | AI |
| Detour | Latest | Pathfinding | AI |
| **Utilities** |
| Taskflow | 3.6+ | Job System | Core |
| efsw | 1.3+ | File Watching (Hot Reload) | Dev |
| ImGui | 1.90+ | Debug UI | Dev |

**Manual Installation Required:**
- **FMOD Core API** - Download from [fmod.com](https://www.fmod.com/download) and place in `external/fmod/`
- See `docs/SYSTEM-IMPLEMENTATION-GUIDE.md` for FMOD setup instructions

## Common Patterns

### Entity Creation

```cpp
Entity player = entities->createEntity();
entities->emplace<TransformComponent>(player, Transform2D{100, 200});
entities->emplace<Health>(player, 100, 100);
```

### Event Subscription

```cpp
auto id = events->subscribe(Events::Collision, [this](const EventData& data) {
    auto& collision = std::get<CollisionEvent>(data);
    handleCollision(collision);
});
```

### Asset Loading

```cpp
AssetHandle texture = assets->registerAsset(AssetType::Texture, "textures/player.png");
assets->loadAssetAsync(texture, [](AssetHandle h, AssetState state) {
    if (state == AssetState::Loaded) {
        // Ready to use
    }
});
```

### Physics Body

```cpp
PhysicsBodyDef def{
    .type = BodyType::Dynamic,
    .transform = {.x = 100, .y = 200},
    .fixedRotation = true
};
physics->createBody(entity, def);
```

---

## Agent Coordination Guide

This section provides guidance for AI agents working independently on Bestow to minimize merge conflicts and maximize parallel productivity.

### File Ownership Model

Each system has clear file boundaries. Agents should claim ownership of exactly one system at a time:

| System | Owned Files | Dependencies |
|--------|-------------|--------------|
| Types | `bestow-contract/src/bestow.types.cppm` | None |
| Core Utilities | `bestow-core/*` | Types |
| Events | `bestow-events/*` | Types |
| Config | `bestow-config/*` | Types |
| Entity | `bestow-entity/*` | Types, Events |
| Assets | `bestow-assets/*` | Types, Events |
| Input | `bestow-input/*` | Types, Events |
| Save | `bestow-save/*` | Types, Events |
| Graphics | `bestow-graphics/*` | Types, Entity, Assets |
| Audio | `bestow-audio/*` | Types, Entity, Assets |
| Physics | `bestow-physics/*` | Types, Entity, Events |
| Level | `bestow-level/*` | Types, Entity, Assets, Events |
| AI | `bestow-ai/*` | Types, Entity, Physics |
| Dev Tools | `bestow-dev/*` | All systems |
| Platformer | `examples/platformer/*` | All systems |

### Rules for Independent Work

1. **Never modify files outside your claimed system** - If you need a change to a dependency interface, document the requirement and leave a `// TODO(agent): Need X from Y system` comment.

2. **Interface contracts are immutable** - The `.cppm` files in `bestow-contract/` define the API. Don't change signatures without orchestrator approval.

3. **Use the test harness** - Each system has its own test file (`tests/test_*.cpp`). Write tests before implementing.

4. **Document assumptions** - If your implementation depends on behavior from another system, add a comment explaining the assumption.

5. **Compile in isolation** - Your system should compile independently: `cmake --build --preset macos-debug --target bestow-yoursystem`

### Parallel-Safe Systems (No Coordination Needed)

These systems can be implemented simultaneously by different agents:

- **Events** + **Config** + **Assets** + **Input** + **Save** (all Tier 1, no dependencies on each other)
- **Graphics** and **Audio** (both need Entity/Assets, but don't interact)
- **Physics** and **AI** (both need Entity, minimal interaction)

### Sequential Dependencies (Requires Coordination)

These must be completed in order:

1. **Entity** must be functional before Graphics, Audio, Physics, Level, AI
2. **Physics** must be functional before AI (needs collision queries)
3. **All systems** must be functional before Dev Tools and Core Engine

### Communication Protocol

When completing work, leave a status comment at the top of your implementation file:

```cpp
// STATUS: Implemented 2024-XX-XX
// COMPLETE: createEntity, destroyEntity, isValid, emplace, get
// PARTIAL: systems (needs iteration support)
// BLOCKED: None
// TESTS: 12/15 passing
```

### What NOT to Touch

- `CMakeLists.txt` in project root (orchestrator only)
- `vcpkg.json` (orchestrator only)
- `cmake/` directory (orchestrator only)
- `bestow-contract/*.cppm` interfaces (orchestrator approval required)
- Other agent's system directories

---

## Orchestrator Guide

This section is for the coordinating agent or human managing parallel development.

### Task Assignment Strategy

#### Phase 1: Foundation (Parallelizable)
Assign these to 5 different agents simultaneously:
- Agent A: Events System (already mostly complete)
- Agent B: Config System
- Agent C: Assets System
- Agent D: Input System
- Agent E: Save System

#### Phase 2: Core Systems (After Phase 1)
Assign these to 3 different agents simultaneously:
- Agent F: Entity System (can start early, Events is done)
- Agent G: Graphics System (needs Entity + Assets)
- Agent H: Audio System (needs Entity + Assets)

#### Phase 3: Simulation (After Entity)
Assign these to 2 different agents simultaneously:
- Agent I: Physics System
- Agent J: AI System (can start after Physics basics are done)

#### Phase 4: Integration
- Agent K: Level System (needs Entity, Assets, Events)
- Agent L: Dev Tools (needs all systems as read-only dependencies)

#### Phase 5: Engine Assembly
- Single agent: Core Engine composition
- Single agent: Platformer example game

### Searching and Research

**Always use explore agents for searching instead of searching directly.** When you need to:

- Search the codebase for specific patterns, files, or implementations
- Understand how a feature works across multiple files
- Research external documentation or web resources
- Find all usages of a function, class, or pattern

Use the Task tool with `subagent_type=Explore` (for codebase exploration) or appropriate web search agents instead of manually running grep/glob commands. This approach:

- **Reduces context usage** - Agents return summarized findings rather than raw search results
- **Improves accuracy** - Agents can iteratively refine searches based on initial results
- **Saves time** - Let the agent handle multiple rounds of searching automatically
- **Maintains focus** - Keep your working context clean for implementation work

Example:
```
// Instead of:
Grep pattern="handleCollision" path="bestow-physics"
Read file1.cpp
Read file2.cpp
Grep pattern="collision" type="cpp"

// Do this:
Task {
  subagent_type: "Explore",
  prompt: "Find all collision handling code in the physics system..."
}
```

### Monitoring Progress

Check these indicators for each system:

```bash
# Does it compile?
cmake --build --preset macos-debug --target bestow-systemname

# Do tests pass?
ctest --preset macos-debug -R "SystemNameTest"

# What's the implementation coverage?
grep -c "// TODO" bestow-systemname/src/*.cpp
```

### Handling Interface Changes

If an agent needs to change an interface in `bestow-contract/`:

1. Agent documents the proposed change in their status comment
2. Orchestrator reviews impact on dependent systems
3. Orchestrator updates interface and notifies affected agents
4. All affected agents update their implementations

### Merge Strategy

1. Agents push to feature branches: `feature/agent-X-systemname`
2. Orchestrator reviews and merges in dependency order
3. After merge, orchestrator runs full build to catch integration issues
4. If conflicts, orchestrator resolves or reassigns

### Quality Gates

Before marking a system complete:

- [ ] All interface methods implemented (not stubbed)
- [ ] Unit tests for public API
- [ ] No `// TODO` comments in critical paths
- [ ] Compiles with `-Werror` (warnings as errors)
- [ ] Integration test with dependent systems passes

### Reference Documents

- **Technical Specification**: `docs/bestow-technical-design.md`
- **Project Status & Checklist**: `docs/PROJECT-STATUS.md`
- **This File**: Development guidelines and coordination rules

### Estimated Total Effort

Based on the implementation checklist in PROJECT-STATUS.md:

| Priority | Systems | Estimated Files | Complexity |
|----------|---------|-----------------|------------|
| Tier 0 | Types finalization | 1 file | Low |
| Tier 1 | Events, Config, Assets, Input, Save | ~25 files | Medium |
| Tier 2 | Entity, Graphics, Audio | ~15 files | High |
| Tier 3 | Physics, Level | ~10 files | High |
| Tier 4 | AI | ~5 files | Medium |
| Tier 5 | Dev Tools | ~10 files | Medium |
| Tier 6 | Core, Platformer | ~15 files | Medium |

### Quick Start for New Agent

1. Read `docs/PROJECT-STATUS.md` for current state
2. Read this file (`CLAUDE.md`) for coding standards
3. Read `docs/bestow-technical-design.md` sections relevant to your assigned system
4. Read the interface file: `bestow-contract/src/bestow.yoursystem.cppm`
5. Read existing implementation: `bestow-yoursystem/src/*.cpp`
6. Run existing tests: `ctest --preset macos-debug -R "YourSystemTest"`
7. Implement, test, document, push

---

## CRITICAL: No Time-Based Estimates

**NEVER use units of time to describe work.** Do not say things like:
- "This will take 2-3 weeks"
- "Estimated 1 week of effort"
- "Can be done in a few days"

Instead, describe work in terms of:
- **Tasks and subtasks** - What needs to be done
- **Dependencies** - What must be completed first
- **Complexity** - Low/Medium/High based on technical difficulty
- **File count** - Approximate number of files to create/modify
- **Test coverage** - What tests need to be written

Agents can implement features very quickly when given clear specifications. Time estimates create false constraints and are meaningless in an AI-assisted development context. Focus on WHAT needs to be done, not WHEN.
