# JFrame Development Guidelines

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

| Platform | Compiler | Minimum Version |
|----------|----------|-----------------|
| macOS | Apple Clang | 15.0+ (Xcode 15+) |
| macOS | LLVM Clang | 17.0+ |
| Windows | MSVC | 19.38+ (VS 2022 17.8+) |
| Windows | Clang-CL | 17.0+ |
| Linux | GCC | 13.0+ |
| Linux | Clang | 17.0+ |

**Recommended:** Clang 17+ for cross-platform consistency.

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

JFrame uses C++23 modules. Follow these conventions:

### Module Naming

```
jframe.types          // Core types and aliases
jframe.entity         // IEntitySystem interface
jframe.graphics       // IGraphicsSystem interface
jframe.audio          // IAudioSystem interface
jframe.input          // IInputSystem interface
jframe.assets         // IAssetSystem interface
jframe.save           // ISaveSystem interface
jframe.level          // ILevelSystem interface
jframe.events         // IEventSystem interface
jframe.physics        // IPhysicsSystem interface
jframe.ai             // IAISystem interface
jframe                // Primary module (re-exports all)
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
export module jframe.modulename;

// Standard library imports
import std;

export namespace jframe {
    // Exported declarations
}
```

### Import Patterns

```cpp
// Import everything
import jframe;

// Import specific modules
import jframe.entity;
import jframe.graphics;

// In game code
import jframe;
import jframe.dev;  // Only available in debug builds
```

## Architecture Principles

### Program to Interfaces

All systems implement abstract interfaces from `jframe-contract`:

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
namespace jframe::events { }

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
#if defined(JFRAME_DEV_TOOLS)
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

## Profiling

Tracy integration is available with `JFRAME_ENABLE_TRACY`:

```cpp
#include <tracy/Tracy.hpp>

void update(DeltaTime dt) {
    ZoneScoped;
    // ...
}
```

## Key Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| GLFW | 3.3+ | Windowing/Input |
| SDL2 | 2.28+ | Game Controllers |
| glm | 0.9.9+ | Math |
| spdlog | 1.12+ | Logging |
| Lua | 5.4+ | Scripting |
| sol2 | 3.3+ | Lua C++ Bindings |
| EnTT | 3.12+ | ECS |
| Box2D | 3.0+ | Physics |
| FMOD Core | 2.02+ | Audio |
| cereal | 1.3+ | Serialization |

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

This section provides guidance for AI agents working independently on JFrame to minimize merge conflicts and maximize parallel productivity.

### File Ownership Model

Each system has clear file boundaries. Agents should claim ownership of exactly one system at a time:

| System | Owned Files | Dependencies |
|--------|-------------|--------------|
| Events | `jframe-events/*` | None |
| Types/Core | `jframe-contract/src/jframe.types.cppm` | None |
| Entity | `jframe-entity/*` | Events |
| Assets | `jframe-assets/*` | Events |
| Input | `jframe-input/*` | Events |
| Save | `jframe-save/*` | Events |
| Graphics | `jframe-graphics/*` | Entity, Assets |
| Audio | `jframe-audio/*` | Entity, Assets |
| Physics | `jframe-physics/*` | Entity, Events |
| Level | `jframe-level/*` | Entity, Assets, Events |
| AI | `jframe-ai/*` | Entity, Physics |
| Dev Tools | `jframe-dev/*` | All systems |
| Core Engine | `jframe-core/*` | All systems |
| Platformer | `examples/platformer/*` | Core Engine |

### Rules for Independent Work

1. **Never modify files outside your claimed system** - If you need a change to a dependency interface, document the requirement and leave a `// TODO(agent): Need X from Y system` comment.

2. **Interface contracts are immutable** - The `.cppm` files in `jframe-contract/` define the API. Don't change signatures without orchestrator approval.

3. **Use the test harness** - Each system has its own test file (`tests/test_*.cpp`). Write tests before implementing.

4. **Document assumptions** - If your implementation depends on behavior from another system, add a comment explaining the assumption.

5. **Compile in isolation** - Your system should compile independently: `cmake --build --preset macos-debug --target jframe-yoursystem`

### Parallel-Safe Systems (No Coordination Needed)

These systems can be implemented simultaneously by different agents:

- **Events** + **Assets** + **Input** + **Save** (all Tier 1, no dependencies on each other)
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
- `jframe-contract/*.cppm` interfaces (orchestrator approval required)
- Other agent's system directories

---

## Orchestrator Guide

This section is for the coordinating agent or human managing parallel development.

### Task Assignment Strategy

#### Phase 1: Foundation (Parallelizable)
Assign these to 4 different agents simultaneously:
- Agent A: Events System (already mostly complete)
- Agent B: Assets System
- Agent C: Input System
- Agent D: Save System

#### Phase 2: Core Systems (After Phase 1)
Assign these to 3 different agents simultaneously:
- Agent E: Entity System (can start early, Events is done)
- Agent F: Graphics System (needs Entity + Assets)
- Agent G: Audio System (needs Entity + Assets)

#### Phase 3: Simulation (After Entity)
Assign these to 2 different agents simultaneously:
- Agent H: Physics System
- Agent I: AI System (can start after Physics basics are done)

#### Phase 4: Integration
- Agent J: Level System (needs Entity, Assets, Events)
- Agent K: Dev Tools (needs all systems as read-only dependencies)

#### Phase 5: Engine Assembly
- Single agent: Core Engine composition
- Single agent: Platformer example game

### Monitoring Progress

Check these indicators for each system:

```bash
# Does it compile?
cmake --build --preset macos-debug --target jframe-systemname

# Do tests pass?
ctest --preset macos-debug -R "SystemNameTest"

# What's the implementation coverage?
grep -c "// TODO" jframe-systemname/src/*.cpp
```

### Handling Interface Changes

If an agent needs to change an interface in `jframe-contract/`:

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

- **Technical Specification**: `docs/jframe-technical-design.md`
- **Project Status & Checklist**: `docs/PROJECT-STATUS.md`
- **This File**: Development guidelines and coordination rules

### Estimated Total Effort

Based on the implementation checklist in PROJECT-STATUS.md:

| Priority | Systems | Estimated Files | Complexity |
|----------|---------|-----------------|------------|
| Tier 0 | Types finalization | 1 file | Low |
| Tier 1 | Events, Assets, Input, Save | ~20 files | Medium |
| Tier 2 | Entity, Graphics, Audio | ~15 files | High |
| Tier 3 | Physics, Level | ~10 files | High |
| Tier 4 | AI | ~5 files | Medium |
| Tier 5 | Dev Tools | ~10 files | Medium |
| Tier 6 | Core, Platformer | ~15 files | Medium |

### Quick Start for New Agent

1. Read `docs/PROJECT-STATUS.md` for current state
2. Read this file (`CLAUDE.md`) for coding standards
3. Read `docs/jframe-technical-design.md` sections relevant to your assigned system
4. Read the interface file: `jframe-contract/src/jframe.yoursystem.cppm`
5. Read existing implementation: `jframe-yoursystem/src/*.cpp`
6. Run existing tests: `ctest --preset macos-debug -R "YourSystemTest"`
7. Implement, test, document, push
