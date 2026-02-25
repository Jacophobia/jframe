# Scene System

> **Visibility:** Public (Lua + C++ API)
> **Tier:** 6
> **Dependencies:** Types, Entity, Assets, Blueprints, Events, Input
> **Lua Paths:** `bestow.scene` (high-level), `bestow.scene.core` (low-level)

## Purpose

The Scene System provides stack-based scene management for organizing game flow into discrete screens such as menus, gameplay levels, pause overlays, and cutscenes. Scenes are registered by name with a Lua script path, then pushed, popped, or replaced on a stack. The active scene (top of stack) receives update and render calls while scenes below it may remain paused. The high-level API covers registration and basic stack operations, while the low-level API adds scene unregistration, stack clearing, transition effects, state queries, and scene-change event subscriptions.

## High-Level API: `ISceneSystem`

The simplified API for common scene management tasks. Register scenes with Lua file paths, push/pop/replace on the scene stack, and query the current state. No lifecycle methods -- the engine manages those internally.

### Registration

| Method | Returns | Description |
|--------|---------|-------------|
| `registerScene(std::string_view name, std::string_view luaPath)` | `Result<void>` | Register a scene by name, associating it with a Lua script file that defines its behavior |

### Stack Operations

| Method | Returns | Description |
|--------|---------|-------------|
| `pushScene(std::string_view name, const SceneParams& params = {})` | `Result<void>` | Push a registered scene onto the stack, making it the active scene; optionally pass typed parameters |
| `popScene()` | `Result<void>` | Pop the active scene from the stack, resuming the scene below it |
| `replaceScene(std::string_view name, const SceneParams& params = {})` | `Result<void>` | Replace the active scene with a different registered scene without modifying the rest of the stack |

### Queries

| Method | Returns | Description |
|--------|---------|-------------|
| `getActiveScene()` | `std::optional<std::string>` | Return the name of the currently active (top of stack) scene, or nullopt if the stack is empty |
| `getSceneStack()` | `std::vector<std::string>` | Return the names of all scenes on the stack from bottom to top |

## Low-Level API: `ISceneCore`

Full control API. Exposes lifecycle methods, full registration and stack control, transition configuration, state queries, and scene-change events.

### Lifecycle

| Method | Returns | Description |
|--------|---------|-------------|
| `update(DeltaTime dt)` | `void` | Update the active scene and process any pending transitions |
| `render()` | `void` | Render the active scene (and optionally scenes below it) |

### Registration

| Method | Returns | Description |
|--------|---------|-------------|
| `registerScene(std::string_view name, AssetHandle sceneAsset)` | `Result<void>` | Register a scene by name using a previously loaded asset handle |
| `unregisterScene(std::string_view name)` | `Result<void>` | Unregister a scene by name; fails if the scene is currently on the stack |

### Stack Operations

| Method | Returns | Description |
|--------|---------|-------------|
| `pushScene(std::string_view name, const SceneParams& params = {})` | `Result<void>` | Push a registered scene onto the stack with optional typed parameters |
| `popScene()` | `Result<void>` | Pop the active scene from the stack |
| `replaceScene(std::string_view name, const SceneParams& params = {})` | `Result<void>` | Replace the current top-of-stack scene |
| `clearStack()` | `void` | Remove all scenes from the stack |

### Queries

| Method | Returns | Description |
|--------|---------|-------------|
| `getActiveSceneName()` | `std::optional<std::string>` | Return the name of the active scene, or nullopt if empty |
| `getSceneStack()` | `std::vector<std::string>` | Return all scene names on the stack from bottom to top |
| `getRegisteredScenes()` | `std::vector<std::string>` | Return the names of all registered scenes |

### Transitions

| Method | Returns | Description |
|--------|---------|-------------|
| `setDefaultTransition(SceneTransition transition)` | `void` | Set the default transition effect used when pushing, popping, or replacing scenes |

### Events

| Method | Returns | Description |
|--------|---------|-------------|
| `onSceneChanged(std::function<void(std::string_view from, std::string_view to)> cb)` | `SubscriptionId` | Subscribe to scene change events; called with the name of the departing and arriving scene |
| `unsubscribe(SubscriptionId id)` | `void` | Remove a previously registered scene change subscription |

## Types

### SceneParams

A typed parameter map passed to scenes during push or replace operations. Replaces `std::any` with a closed variant set.

```cpp
using SceneParam = std::variant<float, int, bool, std::string, Vec2, Vec3>;
using SceneParams = std::unordered_map<std::string, SceneParam>;
```

| Key Type | Value Type | Description |
|----------|------------|-------------|
| `std::string` | `float` | Floating-point parameter |
| `std::string` | `int` | Integer parameter |
| `std::string` | `bool` | Boolean parameter |
| `std::string` | `std::string` | String parameter |
| `std::string` | `Vec2` | 2D vector parameter |
| `std::string` | `Vec3` | 3D vector parameter |

### SceneTransition

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `type` | `SceneTransition::Type` | `None` | Transition effect type |
| `duration` | `float` | `0.3f` | Duration of the transition in seconds |

### SceneTransition::Type

| Value | Description |
|-------|-------------|
| `None` | Instant scene switch with no visual transition |
| `Fade` | Fade out the old scene and fade in the new scene |
| `Slide` | Slide the old scene out and the new scene in |
| `Custom` | User-defined transition effect |

### SceneState

| Value | Description |
|-------|-------------|
| `Inactive` | Scene is registered but not on the stack |
| `Active` | Scene is at the top of the stack and receiving updates |
| `Paused` | Scene is on the stack but below the active scene |
| `TransitioningIn` | Scene is animating its entrance transition |
| `TransitioningOut` | Scene is animating its exit transition |

## Lua Examples

```lua
-- High-level: Register and navigate between scenes
bestow.scene.registerScene("mainMenu", "scenes/main_menu.lua")
bestow.scene.registerScene("gameplay", "scenes/gameplay.lua")
bestow.scene.registerScene("pause", "scenes/pause.lua")
bestow.scene.registerScene("gameOver", "scenes/game_over.lua")

-- Start with main menu
bestow.scene.pushScene("mainMenu")

-- Transition to gameplay with parameters
bestow.scene.replaceScene("gameplay", {
    level = 3,
    difficulty = "hard",
    spawnPoint = {100, 0, 50}
})

-- Push pause overlay (gameplay stays underneath)
bestow.scene.pushScene("pause")

-- Resume gameplay
bestow.scene.popScene()

-- Check current scene
local active = bestow.scene.getActiveScene()
local stack = bestow.scene.getSceneStack()
print("Active: " .. (active or "none"))
print("Stack depth: " .. #stack)

-- Low-level: Transitions and events
bestow.scene.core.setDefaultTransition({type = "Fade", duration = 0.5})

local subId = bestow.scene.core.onSceneChanged(function(from, to)
    print("Scene changed: " .. from .. " -> " .. to)
end)

-- List all registered scenes
local registered = bestow.scene.core.getRegisteredScenes()
for _, name in ipairs(registered) do
    print("Registered: " .. name)
end

-- Clear everything
bestow.scene.core.clearStack()

-- Clean up
bestow.scene.core.unsubscribe(subId)
```

## C++ Examples

```cpp
// High-level: Scene registration and navigation
scene->registerScene("mainMenu", "scenes/main_menu.lua");
scene->registerScene("gameplay", "scenes/gameplay.lua");
scene->registerScene("pause", "scenes/pause.lua");

scene->pushScene("mainMenu");

// Transition to gameplay with typed parameters
SceneParams params;
params["level"] = 3;
params["difficulty"] = std::string("hard");
params["spawnPoint"] = Vec3{100, 0, 50};
scene->replaceScene("gameplay", params);

// Push pause overlay
scene->pushScene("pause");

// Pop back to gameplay
scene->popScene();

// Query state
auto active = scene->getActiveScene(); // "gameplay"
auto stack = scene->getSceneStack();   // {"gameplay"}

// Low-level: Transitions
sceneCore->setDefaultTransition(SceneTransition{
    .type = SceneTransition::Type::Fade,
    .duration = 0.5f
});

// Subscribe to scene changes
auto subId = sceneCore->onSceneChanged(
    [](std::string_view from, std::string_view to) {
        spdlog::info("Scene changed: {} -> {}", from, to);
    });

// Query registered scenes
auto registered = sceneCore->getRegisteredScenes();

// Clear the stack (e.g., on fatal error)
sceneCore->clearStack();

// Unsubscribe
sceneCore->unsubscribe(subId);
```
