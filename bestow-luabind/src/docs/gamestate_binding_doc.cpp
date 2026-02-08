// bestow-luabind/src/docs/gamestate_binding_doc.cpp
// API documentation for bestow.gamestate

module bestow.luabind;

import std;

namespace bestow {

void registerGamestateDoc(DocRegistry& registry) {
    SystemDoc sys;
    sys.name = "gamestate";
    sys.qualifiedName = "bestow.gamestate";
    sys.description = "Game state machine system. Manages a stack of game states (e.g., MainMenu, Playing, Paused, GameOver) with push/pop transitions, lifecycle hooks, and input routing.";

    //=========================================================================
    // Enums
    //=========================================================================

    sys.enums.push_back(EnumDoc{
        .name = "GameStateFlags",
        .qualifiedName = "GameStateFlags",
        .description = "Bitmask flags controlling how a game state interacts with states below it on the stack.",
        .values = {
            {.name = "None", .description = "No special behavior -- state is opaque and blocks everything below"},
            {.name = "UpdateBelow", .description = "Continue updating states below this one on the stack"},
            {.name = "RenderBelow", .description = "Continue rendering states below this one on the stack"},
            {.name = "BlockInput", .description = "Block input from reaching states below this one"},
            {.name = "Overlay", .description = "Combination: UpdateBelow | RenderBelow | BlockInput (typical pause menu)"},
            {.name = "Popup", .description = "Combination: RenderBelow | BlockInput (popup dialog that pauses the game)"},
        },
    });

    sys.enums.push_back(EnumDoc{
        .name = "TransitionType",
        .qualifiedName = "TransitionType",
        .description = "Visual transition type used when switching between game states.",
        .values = {
            {.name = "None", .description = "Instant transition with no animation"},
            {.name = "Fade", .description = "Fade out the old state, fade in the new state"},
            {.name = "Slide", .description = "Slide transition between states"},
            {.name = "Custom", .description = "User-defined transition effect"},
        },
    });

    //=========================================================================
    // Types
    //=========================================================================

    sys.types.push_back(TypeDoc{
        .name = "StateTransition",
        .qualifiedName = "StateTransition",
        .description = "Describes how to animate between game states during push, pop, or replace operations.",
        .fields = {
            {.name = "type", .type = "TransitionType", .description = "The visual transition style"},
            {.name = "duration", .type = "number", .description = "Transition duration in seconds (default: 0.3)"},
        },
        .methods = {
            {.name = "instant", .qualifiedName = "StateTransition.instant", .description = "Create an instant transition with no animation.", .returns = {{.type = "StateTransition", .description = "An instant (zero-duration) transition"}}},
            {.name = "fade", .qualifiedName = "StateTransition.fade", .description = "Create a fade transition.", .params = {{.name = "duration", .type = "number", .description = "Fade duration in seconds", .optional = true, .defaultVal = "0.3"}}, .returns = {{.type = "StateTransition", .description = "A fade transition"}}},
            {.name = "slide", .qualifiedName = "StateTransition.slide", .description = "Create a slide transition.", .params = {{.name = "duration", .type = "number", .description = "Slide duration in seconds", .optional = true, .defaultVal = "0.3"}}, .returns = {{.type = "StateTransition", .description = "A slide transition"}}},
        },
    });

    sys.types.push_back(TypeDoc{
        .name = "StateInputEvent",
        .qualifiedName = "StateInputEvent",
        .description = "Input event passed to game states for handling. Wraps keyboard, mouse, and gamepad events.",
        .fields = {
            {.name = "type", .type = "string", .description = "Event type: \"KeyPressed\", \"KeyReleased\", \"MousePressed\", \"MouseReleased\", \"MouseMoved\", \"MouseScrolled\", \"GamepadButton\", \"GamepadAxis\""},
            {.name = "code", .type = "number", .description = "Key code, mouse button, or gamepad button index"},
            {.name = "value", .type = "number", .description = "Axis value or scroll amount"},
            {.name = "x", .type = "number", .description = "Mouse X position"},
            {.name = "y", .type = "number", .description = "Mouse Y position"},
            {.name = "modifiers", .type = "number", .description = "Modifier key flags (Shift, Ctrl, Alt)"},
        },
    });

    //=========================================================================
    // Stack Operations
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "pushState",
        .qualifiedName = "bestow.gamestate.pushState",
        .description = "Push a new state onto the state stack. The new state becomes the active state and receives onEnter(). The previous top state receives onPause().",
        .params = {
            {.name = "stateName", .type = "string", .description = "Name of a registered state factory to create and push"},
            {.name = "transition", .type = "StateTransition", .description = "Transition animation to use", .optional = true},
        },
        .example = "-- Push a pause menu with a fade transition\nbestow.gamestate.pushState(\"PauseMenu\", StateTransition.fade(0.5))",
        .seeAlso = {"bestow.gamestate.popState", "bestow.gamestate.replaceState"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "popState",
        .qualifiedName = "bestow.gamestate.popState",
        .description = "Pop the current state from the stack. The popped state receives onExit(). The state below (if any) receives onResume().",
        .params = {
            {.name = "transition", .type = "StateTransition", .description = "Transition animation to use", .optional = true},
        },
        .seeAlso = {"bestow.gamestate.pushState"},
    });

    sys.methods.push_back(MethodDoc{
        .name = "replaceState",
        .qualifiedName = "bestow.gamestate.replaceState",
        .description = "Pop all states and push a new one. Useful for \"quit to main menu\" scenarios. All existing states receive onExit().",
        .params = {
            {.name = "stateName", .type = "string", .description = "Name of a registered state factory to create and push"},
            {.name = "transition", .type = "StateTransition", .description = "Transition animation to use", .optional = true},
        },
        .example = "-- Return to main menu, clearing the entire state stack\nbestow.gamestate.replaceState(\"MainMenu\", StateTransition.fade(1.0))",
    });

    sys.methods.push_back(MethodDoc{
        .name = "clearStates",
        .qualifiedName = "bestow.gamestate.clearStates",
        .description = "Clear all states from the stack. Each state receives onExit() in reverse order.",
    });

    //=========================================================================
    // State Access
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "getStateCount",
        .qualifiedName = "bestow.gamestate.getStateCount",
        .description = "Get the number of states currently on the stack.",
        .returns = {{.type = "number", .description = "Number of states on the stack"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "isEmpty",
        .qualifiedName = "bestow.gamestate.isEmpty",
        .description = "Check if the state stack is empty.",
        .returns = {{.type = "boolean", .description = "true if there are no states on the stack"}},
    });

    sys.methods.push_back(MethodDoc{
        .name = "isTransitioning",
        .qualifiedName = "bestow.gamestate.isTransitioning",
        .description = "Check if a state transition animation is currently in progress.",
        .returns = {{.type = "boolean", .description = "true if a transition is playing"}},
    });

    //=========================================================================
    // Main Loop Integration
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "update",
        .qualifiedName = "bestow.gamestate.update",
        .description = "Update all active states, respecting UpdateBelow flags. Call once per frame.",
        .params = {
            {.name = "dt", .type = "number", .description = "Delta time in seconds since the last frame"},
        },
    });

    sys.methods.push_back(MethodDoc{
        .name = "render",
        .qualifiedName = "bestow.gamestate.render",
        .description = "Render all visible states, respecting RenderBelow flags. Call once per frame after update.",
    });

    sys.methods.push_back(MethodDoc{
        .name = "handleInput",
        .qualifiedName = "bestow.gamestate.handleInput",
        .description = "Process an input event through the state stack. States are queried top-down; the first state to consume the event stops propagation.",
        .params = {
            {.name = "event", .type = "StateInputEvent", .description = "The input event to process"},
        },
        .returns = {{.type = "boolean", .description = "true if the event was consumed by a state"}},
    });

    //=========================================================================
    // State Registration
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "registerStateFactory",
        .qualifiedName = "bestow.gamestate.registerStateFactory",
        .description = "Register a state factory by name. The factory is called to create state instances when pushState or replaceState is used with this name.",
        .params = {
            {.name = "stateName", .type = "string", .description = "Unique name for the state (e.g., \"MainMenu\", \"Playing\")"},
            {.name = "factory", .type = "function", .description = "Factory function that returns a new state table with lifecycle methods (onEnter, onExit, update, render, etc.)"},
        },
        .example = "bestow.gamestate.registerStateFactory(\"MainMenu\", function()\n    return {\n        onEnter = function(self)\n            -- Set up main menu UI\n        end,\n        onExit = function(self)\n            -- Clean up\n        end,\n        update = function(self, dt) end,\n        render = function(self) end,\n    }\nend)",
    });

    sys.methods.push_back(MethodDoc{
        .name = "createState",
        .qualifiedName = "bestow.gamestate.createState",
        .description = "Create a state instance by its registered name without pushing it onto the stack.",
        .params = {
            {.name = "stateName", .type = "string", .description = "Name of a registered state factory"},
        },
        .returns = {{.type = "GameState|nil", .description = "The created state, or nil if the name is not registered"}},
    });

    //=========================================================================
    // Callbacks
    //=========================================================================

    sys.methods.push_back(MethodDoc{
        .name = "setStateChangeCallback",
        .qualifiedName = "bestow.gamestate.setStateChangeCallback",
        .description = "Register a callback that fires whenever the active (topmost) state changes. Useful for analytics, logging, or global state-change reactions.",
        .params = {
            {.name = "callback", .type = "function", .description = "Callback function(oldState, newState) called when the active state changes"},
        },
        .example = "bestow.gamestate.setStateChangeCallback(function(oldState, newState)\n    print(\"State changed from\", oldState, \"to\", newState)\nend)",
    });

    registry.addSystem(std::move(sys));
}

} // namespace bestow
