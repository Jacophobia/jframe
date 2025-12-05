# Input System Demo

Comprehensive demonstration of the Bestow Input System API.

## Overview

This demo exercises **every method** in the `IInputSystem` interface, demonstrating:

- Input mapping and action binding
- Multi-device support (keyboard, mouse, controller)
- Action state queries and helpers
- Input rebinding functionality
- Real-time input monitoring

## What This Demo Tests

### Lifecycle Methods
- `update()` - Called every frame to update input state

### Mapping Management
- `registerMapping()` - Register input bindings to actions
- `removeMapping()` - Remove specific bindings
- `clearMappings()` - Clear all mappings
- `getMappings()` - Query all registered mappings

### Action State Queries
- `getActionState()` - Get detailed state for a specific action
- `getAllActionStates()` - Get all action states at once
- `isActionActive()` - Check if action is currently active
- `wasActionJustPressed()` - Detect button press events
- `wasActionJustReleased()` - Detect button release events
- `getActionValue()` - Get action value (for analog inputs)

### Input Rebinding
- `startListeningForInput()` - Enter rebinding mode
- `stopListeningForInput()` - Exit rebinding mode
- `isListeningForInput()` - Check if listening
- `getLastInput()` - Retrieve captured input

### Mouse State
- `getMousePosition()` - Current mouse position
- `getMouseDelta()` - Mouse movement since last frame
- `isMouseButtonDown()` - Mouse button state

### Controller Support
- `getConnectedControllerCount()` - Number of connected controllers
- `isControllerConnected()` - Check specific controller
- `getControllerName()` - Get controller name/model

## Building and Running

### Build
```bash
# From project root
cmake --build --preset macos-debug --target input-demo
```

### Run
```bash
# From build directory
./bin/input-demo

# Or from project root
./build/macos-debug/bin/input-demo
```

## Interactive Controls

### Pre-Mapped Actions
The demo comes with these default mappings:

**Movement:**
- Arrow Keys or WASD → Move Left/Right
- Controller D-Pad or Left Stick → Move

**Actions:**
- Space or Controller A Button → Jump
- Left Ctrl or Controller X Button → Attack

**Menu:**
- Escape or Controller Start → Menu (exits demo)

### Demo Commands

While the demo is running, you can press:

- **R** - Enter rebinding mode (capture next input)
- **M** - Print all current input mappings
- **C** - Clear all mappings
- **T** - Test action helper methods (detailed output)
- **Q** - Quit the demo

### Mouse Testing
Move the mouse around and click buttons to see:
- Real-time position tracking
- Delta movement calculation
- Button state detection

### Controller Testing
If you have a game controller connected:
- The demo will detect it automatically
- Shows controller name and index
- All controller inputs work with the mapped actions
- Try buttons, D-pad, and analog sticks

## Input System Features Demonstrated

### Multi-Device Binding
The same action can be triggered by multiple devices:
```cpp
// Jump can be triggered by Space (keyboard) OR A button (controller)
registerMapping({Keyboard, "Space", "Jump"});
registerMapping({Controller, "A Button", "Jump"});
```

### Analog Input Support
Controller analog sticks provide value ranging from -1.0 to 1.0:
```cpp
// Left stick X-axis maps to movement
float value = getActionValue("MoveLeft");  // -1.0 to 0.0
```

### Deadzone Configuration
Prevents stick drift by ignoring small movements:
```cpp
InputBinding{
    .deviceType = Controller,
    .keyCode = AXIS_LEFTX,
    .scale = 1.0f,
    .deadzone = 0.2f  // Ignore values < 20%
}
```

### Just Pressed/Released Detection
Perfect for jump mechanics and UI buttons:
```cpp
if (wasActionJustPressed("Jump")) {
    // Player jumped this frame
}
```

### Live Rebinding
Capture any key/button/axis for remapping controls:
```cpp
startListeningForInput();
// User presses any button...
auto input = getLastInput();  // Captured!
```

## Expected Output

When running, you'll see:
```
========================================
  Bestow Input System Demo
========================================

[Instructions...]

Registering default input mappings...
Input mappings registered successfully!

=== Registered Input Mappings ===
  MoveLeft:
    - Keyboard Left Arrow
    - Keyboard A
    - Controller [#0] D-Pad Left
    - Controller [#0] Left Stick X (scale: -1)
  [...]

=== Controller State ===
  Connected Controllers: 1
  Controller #0: Sony DualSense Wireless Controller

=== Action States ===
  Jump: [JUST PRESSED] ACTIVE (value: 1)
  MoveLeft: ACTIVE (value: -0.85)

=== Mouse State ===
  Position: (450.0, 320.0)
  Delta: (2.5, -1.0)
  Buttons: Left Mouse
```

## Implementation Notes

This demo uses:
- `bestow.input.impl` - Input system implementation
- `bestow.types` - Input types (InputBinding, ActionState, etc.)
- GLFW for window creation and keyboard/mouse input
- SDL2 for game controller support

The demo runs at 60 FPS with input updates every frame, matching typical game loop behavior.

## API Coverage

This demo exercises **100% of the IInputSystem API**:
- ✅ All lifecycle methods
- ✅ All mapping management methods
- ✅ All action state query methods
- ✅ All action helper methods
- ✅ All raw input methods
- ✅ All mouse state methods
- ✅ All controller methods

Every method is called at least once during normal demo operation.
