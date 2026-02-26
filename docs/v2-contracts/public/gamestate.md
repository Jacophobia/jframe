# Game State Machine

> **Visibility:** Public (Lua + C++ API)
> **Tier:** 6
> **Dependencies:** Types, Input, Events
> **Lua Paths:** `bestow.gamestate` (single-level, no Core/System split)

## Purpose

The Game State Machine manages a stack-based state hierarchy for organizing high-level game flow -- main menu, gameplay, pause screen, inventory overlay, dialog popups, and similar screen-level states. Each state receives lifecycle callbacks (enter, exit, pause, resume) and per-frame update, render, and input handling. States carry bitmask flags that control stacking behavior such as whether lower states continue updating or rendering, whether input is blocked, and whether the state is transparent. The system supports factory-based state creation, transition callbacks, and utility base classes for common patterns. Because the state machine is simple and C++-heavy, it uses a single-level API with no Core/System split.

## Single-Level API: `IGameStateSystem`

The complete game state machine API. Manages a stack of `IGameState` instances with push/pop/replace semantics, factory registration for Lua-driven state creation, and per-frame dispatch of update/render/input to the active state stack.

### Stack Operations

| Method | Returns | Description |
|--------|---------|-------------|
| `pushState(std::string_view stateName, const SceneParams& params)` | `Result<void>` | Create and push a new state onto the stack; the current state receives `onPause()` |
| `popState()` | `Result<void>` | Pop the top state from the stack; it receives `onExit()` and the state below receives `onResume()` |
| `replaceState(std::string_view stateName, const SceneParams& params)` | `Result<void>` | Pop the top state and push a new one in a single operation |
| `popUntil(std::function<bool(const IGameState&)> predicate)` | `Result<void>` | Pop states from the stack until the predicate returns true for the top state |
| `clearStates()` | `void` | Pop all states from the stack, calling `onExit()` on each in top-to-bottom order |

### Queries

| Method | Returns | Description |
|--------|---------|-------------|
| `getCurrentState()` | `IGameState*` | Return a pointer to the top state on the stack, or nullptr if the stack is empty |
| `getStateAt(int index)` | `IGameState*` | Return a pointer to the state at the given stack index (0 = bottom), or nullptr if out of range |
| `getStateCount()` | `int` | Return the number of states currently on the stack |
| `isEmpty()` | `bool` | Check whether the state stack is empty |
| `isTransitioning()` | `bool` | Check whether a state transition is currently in progress |

### Per-Frame Dispatch

| Method | Returns | Description |
|--------|---------|-------------|
| `update(DeltaTime dt)` | `void` | Dispatch `update()` to the top state and any states below it that have `UpdateBelow` flagged |
| `render()` | `void` | Dispatch `render()` to the top state and any states below it that have `RenderBelow` flagged |
| `handleInput(const StateInputEvent& event)` | `bool` | Dispatch an input event to the top state; returns true if the event was consumed |

### Factory Registration

| Method | Returns | Description |
|--------|---------|-------------|
| `registerStateFactory(std::string_view name, StateFactory factory)` | `void` | Register a factory function that creates a named state type, enabling Lua-driven state creation |
| `createState(std::string_view name, const SceneParams& params)` | `Result<std::unique_ptr<IGameState>>` | Create a state instance from a registered factory without pushing it onto the stack |

### Callbacks

| Method | Returns | Description |
|--------|---------|-------------|
| `setStateChangeCallback(std::function<void(const StateTransition&)> cb)` | `void` | Set a callback invoked whenever the state stack changes (push, pop, replace, clear) |

## IGameState Interface

The interface that individual game states implement. Each state provides lifecycle hooks, per-frame methods, and configuration flags.

### Lifecycle

| Method | Returns | Description |
|--------|---------|-------------|
| `onEnter(const SceneParams& params)` | `void` | Called when the state is first pushed onto the stack; receives initialization parameters |
| `onExit()` | `void` | Called when the state is popped from the stack; perform cleanup here |
| `onPause()` | `void` | Called when another state is pushed on top of this one |
| `onResume()` | `void` | Called when the state above this one is popped, making this the active state again |

### Per-Frame

| Method | Returns | Description |
|--------|---------|-------------|
| `update(DeltaTime dt)` | `void` | Called once per frame to update state logic |
| `render()` | `void` | Called once per frame to issue render commands |
| `handleInput(const StateInputEvent& event)` | `bool` | Called to process an input event; return true to consume the event |

### Configuration

| Method | Returns | Description |
|--------|---------|-------------|
| `getFlags()` | `GameStateFlags` | Return the bitmask flags controlling this state's stacking behavior |
| `getName()` | `std::string_view` | Return the human-readable name of this state for debugging and logging |

## Types

### GameStateFlags

A bitmask enum controlling how a state interacts with the states below it on the stack.

```cpp
enum class GameStateFlags : std::uint8_t {
    None         = 0,
    UpdateBelow  = 1 << 0,  // States below this one continue receiving update()
    RenderBelow  = 1 << 1,  // States below this one continue receiving render()
    BlockInput   = 1 << 2,  // This state consumes all input, preventing lower states from receiving it
    Transparent  = 1 << 3,  // Visual hint that this state does not fill the screen
    Overlay      = UpdateBelow | RenderBelow | Transparent,  // Convenience: overlay that shows content below
    Popup        = RenderBelow | BlockInput | Transparent     // Convenience: modal popup over rendered scene
};
```

| Flag | Value | Description |
|------|-------|-------------|
| `None` | `0x00` | Default behavior: lower states do not update or render |
| `UpdateBelow` | `0x01` | States below this one on the stack continue to receive `update()` calls |
| `RenderBelow` | `0x02` | States below this one on the stack continue to receive `render()` calls |
| `BlockInput` | `0x04` | This state consumes all input events, preventing them from reaching lower states |
| `Transparent` | `0x08` | Marks this state as not fully covering the screen, hinting that lower states should render |
| `Overlay` | `0x0B` | Convenience combination: UpdateBelow + RenderBelow + Transparent |
| `Popup` | `0x0E` | Convenience combination: RenderBelow + BlockInput + Transparent |

### StateInputEvent

An input event routed through the game state stack.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `actionName` | `std::string` | `""` | The name of the input action that fired (e.g., "jump", "pause", "confirm") |
| `state` | `InputState` | `Pressed` | The state of the input (Pressed, Released, Held) |
| `value` | `float` | `0.0f` | Analog value for axis-type inputs (0.0 to 1.0 for triggers, -1.0 to 1.0 for sticks) |

### TransitionType

Enum describing the kind of state stack change that occurred.

```cpp
enum class TransitionType : std::uint8_t {
    Push,
    Pop,
    Replace,
    Clear
};
```

| Value | Description |
|-------|-------------|
| `Push` | A new state was pushed onto the stack |
| `Pop` | The top state was popped from the stack |
| `Replace` | The top state was replaced with a new state |
| `Clear` | All states were removed from the stack |

### StateTransition

Event data passed to the state change callback describing what changed.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `type` | `TransitionType` | `Push` | The kind of transition that occurred |
| `fromState` | `std::string` | `""` | The name of the state that was exited, or empty if the stack was previously empty |
| `toState` | `std::string` | `""` | The name of the state that is now on top, or empty if the stack is now empty |
| `stackDepth` | `int` | `0` | The number of states on the stack after the transition |

### StateFactory

A function type used to create state instances by name.

```cpp
using StateFactory = std::function<std::unique_ptr<IGameState>(const SceneParams& params)>;
```

### SimpleGameState

A convenience base class that implements `IGameState` with default no-op methods. Override only the methods you need.

| Method | Default Behavior | Description |
|--------|-----------------|-------------|
| `onEnter(params)` | No-op | Override to initialize state resources |
| `onExit()` | No-op | Override to clean up state resources |
| `onPause()` | No-op | Override to handle being covered by another state |
| `onResume()` | No-op | Override to handle becoming active again |
| `update(dt)` | No-op | Override to add per-frame logic |
| `render()` | No-op | Override to issue draw calls |
| `handleInput(event)` | Returns `false` | Override to process input events |
| `getFlags()` | `GameStateFlags::None` | Override to change stacking behavior |
| `getName()` | `"SimpleGameState"` | Override to provide a descriptive name |

### OverlayState

A convenience base class for states that render on top of the scene without blocking updates below. Pre-configured with `GameStateFlags::Overlay`.

| Method | Default Behavior | Description |
|--------|-----------------|-------------|
| `getFlags()` | `GameStateFlags::Overlay` | Allows lower states to update and render; marked as transparent |

### PopupState

A convenience base class for modal popup states that render over the scene and block input. Pre-configured with `GameStateFlags::Popup`.

| Method | Default Behavior | Description |
|--------|-----------------|-------------|
| `getFlags()` | `GameStateFlags::Popup` | Allows lower states to render but blocks their input; marked as transparent |

## Lua Examples

```lua
-- Register state factories from Lua
bestow.gamestate.registerStateFactory("MainMenu", function(params)
    return {
        name = "MainMenu",
        flags = 0,  -- GameStateFlags.None
        onEnter = function(self, params)
            self.ui = bestow.ui.loadDocument("ui/main_menu.rml")
            bestow.ui.showDocument(self.ui)
        end,
        onExit = function(self)
            bestow.ui.hideDocument(self.ui)
        end,
        update = function(self, dt)
            -- Menu animation logic
        end,
        render = function(self)
            -- Menu rendering
        end,
        handleInput = function(self, event)
            if event.actionName == "confirm" and event.state == "pressed" then
                bestow.gamestate.replaceState("Gameplay")
                return true
            end
            return false
        end,
    }
end)

bestow.gamestate.registerStateFactory("PauseMenu", function(params)
    return {
        name = "PauseMenu",
        flags = 0x0E,  -- GameStateFlags.Popup
        onEnter = function(self, params)
            -- Show pause UI
        end,
        handleInput = function(self, event)
            if event.actionName == "pause" and event.state == "pressed" then
                bestow.gamestate.popState()
                return true
            end
            return false
        end,
    }
end)

-- Push initial state
bestow.gamestate.pushState("MainMenu")

-- Game flow
bestow.gamestate.pushState("Gameplay")
bestow.gamestate.pushState("PauseMenu")  -- Overlays on top of Gameplay

local current = bestow.gamestate.getCurrentState()
print("Current state: " .. current:getName())
print("Stack depth: " .. bestow.gamestate.getStateCount())

bestow.gamestate.popState()  -- Remove PauseMenu, resume Gameplay

-- Query the stack
if not bestow.gamestate.isEmpty() then
    local bottom = bestow.gamestate.getStateAt(0)
    print("Bottom state: " .. bottom:getName())
end

-- Transition callback
bestow.gamestate.setStateChangeCallback(function(transition)
    print("Transition: " .. transition.type .. " from=" .. transition.fromState
        .. " to=" .. transition.toState .. " depth=" .. transition.stackDepth)
end)

-- Clear all states (e.g., on quit)
bestow.gamestate.clearStates()
```

## C++ Examples

```cpp
// Register state factories
gameState->registerStateFactory("MainMenu", [](const SceneParams& params) {
    return std::make_unique<MainMenuState>(params);
});

gameState->registerStateFactory("Gameplay", [](const SceneParams& params) {
    return std::make_unique<GameplayState>(params);
});

gameState->registerStateFactory("PauseMenu", [](const SceneParams& params) {
    return std::make_unique<PauseMenuState>(params);
});

// Push initial state
gameState->pushState("MainMenu", {});

// Gameplay flow
gameState->replaceState("Gameplay", {{"level", std::string("level1")}});
gameState->pushState("PauseMenu", {});

// Query
IGameState* current = gameState->getCurrentState();
if (current) {
    auto name = current->getName();
}
int depth = gameState->getStateCount();
bool transitioning = gameState->isTransitioning();

// Pop until we find the gameplay state
gameState->popUntil([](const IGameState& state) {
    return state.getName() == "Gameplay";
});

// Transition callback
gameState->setStateChangeCallback([](const StateTransition& t) {
    spdlog::info("State change: {} -> {} (depth: {})",
        t.fromState, t.toState, t.stackDepth);
});

// Implementing a custom state
class PauseMenuState : public PopupState {
public:
    void onEnter(const SceneParams& params) override {
        // Load pause menu UI
    }

    void onExit() override {
        // Clean up UI
    }

    void render() override {
        // Draw darkened overlay and menu
    }

    bool handleInput(const StateInputEvent& event) override {
        if (event.actionName == "pause" && event.state == InputState::Pressed) {
            // Request pop from the game state system
            return true;
        }
        return false;
    }

    std::string_view getName() const override { return "PauseMenu"; }
};

// Per-frame dispatch (called by engine loop)
gameState->update(dt);
gameState->render();
```
