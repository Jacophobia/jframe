# UI System Overhaul

> **Status:** Planning Phase
> **Created:** 2025-12-21
> **Purpose:** Make the UI/HUD system fully renderer-agnostic and contract-compliant

## Executive Summary

The current UI system (`IUISystem` / `RmlUISystem`) has architectural violations that prevent it from working with Vulkan. This overhaul creates a proper contract-based architecture that allows ONE UI implementation to work with ALL renderer configurations:

- OpenGL 2D (`IGraphicsSystem`)
- OpenGL 3D (`IGraphics3DSystem`)
- Vulkan 3D (`IGraphics3DSystem`)
- Vulkan 2D (`IGraphicsSystem` - future)

## Problem Statement

### Current Issues

1. **Direct OpenGL coupling**: `RmlUISystem` includes GLFW headers and stores `GLFWwindow*`
2. **No Vulkan backend**: RmlUi requires a render interface that doesn't exist for Vulkan
3. **3D text rendering broken**: `drawText3D()` is completely stubbed in Vulkan
4. **Debug lines not working**: Even `debugDrawLine()` appears broken in Vulkan
5. **Contract violation**: UI system expects graphics implementation details, not interfaces

### Evidence

From `bestow-ui/src/bestow.ui.impl.cppm`:
```cpp
#include <GLFW/glfw3.h>  // Direct platform coupling

class RmlUISystem : public IUISystem {
    GLFWwindow* window_ = nullptr;  // Should not know about GLFW

    // TODO: Create custom render and system interfaces
    // For now, RmlUi needs to be initialized by the graphics system
    // which has access to the OpenGL context
};
```

From `games/game1/src/snake.game.cppm`:
```cpp
// Pixel Font Rendering (since drawText3D is not implemented)
// [Implements custom 5x7 pixel font using debugDrawLine]
```

## Solution Overview

Create a layered architecture with proper contract boundaries:

```
┌─────────────────────────────────────────────────────────────┐
│                      IUISystem                               │
│              (RmlUISystem - single implementation)           │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                   IGraphicsContext                           │
│        (base interface for all graphics systems)             │
│                                                              │
│   - getUIRenderBackend() → IUIRenderBackend*                │
│   - getViewportSize() → Size                                 │
│   - getNativeWindowHandle() → void*                          │
└─────────────────────────────────────────────────────────────┘
           │                                   │
           ▼                                   ▼
┌─────────────────────┐             ┌─────────────────────┐
│   IGraphicsSystem   │             │  IGraphics3DSystem  │
│       (2D)          │             │       (3D)          │
└─────────────────────┘             └─────────────────────┘
           │                                   │
     ┌─────┴─────┐                       ┌─────┴─────┐
     ▼           ▼                       ▼           ▼
┌─────────┐ ┌─────────┐           ┌─────────┐ ┌─────────┐
│ OpenGL  │ │ Vulkan  │           │ OpenGL  │ │ Vulkan  │
│   2D    │ │   2D    │           │   3D    │ │   3D    │
└────┬────┘ └────┬────┘           └────┬────┘ └────┬────┘
     │           │                     │           │
     ▼           ▼                     ▼           ▼
┌─────────────────────────────────────────────────────────────┐
│                    IUIRenderBackend                          │
│           (created by graphics implementation)               │
│                                                              │
│   OpenGL impl shares code between 2D/3D                     │
│   Vulkan impl shares code between 2D/3D                     │
└─────────────────────────────────────────────────────────────┘
```

## Documentation Structure

| Document | Description |
|----------|-------------|
| [ARCHITECTURE.md](./ARCHITECTURE.md) | Detailed architecture and dependency flow |
| [CONTRACTS.md](./CONTRACTS.md) | New and modified contract specifications |
| [IMPLEMENTATION.md](./IMPLEMENTATION.md) | Step-by-step implementation guide |
| [MIGRATION.md](./MIGRATION.md) | Migration plan and breaking changes |
| [VERIFICATION.md](./VERIFICATION.md) | Verification checklist and test plan |

## Key Design Decisions

### 1. Graphics System Owns UI Render Backend

The graphics implementation (not UI) creates and owns the `IUIRenderBackend`. This is because:
- Only the graphics impl knows its internal context (VkDevice, GL context)
- Ensures the UI backend is always compatible with the graphics system
- Lifecycle is tied to graphics system lifecycle

### 2. Single Base Interface (`IGraphicsContext`)

Both `IGraphicsSystem` and `IGraphics3DSystem` extend `IGraphicsContext`. This allows:
- UI system to work with either 2D or 3D graphics
- No code duplication for UI support methods
- Clean dependency: UI → IGraphicsContext (not specific graphics type)

### 3. Shared OpenGL/Vulkan UI Backend Code

OpenGL 2D and 3D can share the same `OpenGLUIRenderBackend` implementation because:
- Same OpenGL context
- Same shader/texture management
- Only difference is which graphics system creates it

Same applies to Vulkan.

### 4. RmlUi Adapters Inside UI System

The `BestowRmlRenderInterface` (adapting RmlUi to `IUIRenderBackend`) lives in `bestow-ui`:
- Keeps RmlUi dependency isolated
- Clean conversion between RmlUi types and Bestow types
- Could swap RmlUi for another UI library without changing graphics

## Files Changed

### New Files
- `bestow-contract/src/bestow.graphics.context.cppm`
- `bestow-contract/src/bestow.uirender.cppm`
- `bestow-vulkan/src/VulkanUIRenderBackend.cpp`
- `bestow-opengl/src/OpenGLUIRenderBackend.cpp`
- `bestow-ui/src/BestowRmlInterfaces.cpp`

### Modified Files
- `bestow-contract/src/bestow.graphics.cppm`
- `bestow-contract/src/bestow.graphics3d.cppm`
- `bestow-opengl/src/bestow.opengl.impl.cppm`
- `bestow-vulkan/src/bestow.vulkan.impl.cppm`
- `bestow-ui/src/bestow.ui.impl.cppm`

## Success Criteria

1. `RmlUISystem` has ZERO includes of GLFW, Vulkan, or OpenGL headers
2. Same `RmlUISystem` code works with OpenGL and Vulkan
3. UI renders correctly in snake game (Vulkan 3D)
4. Text rendering works via proper font system (not pixel hacks)
5. All existing 2D OpenGL games continue to work
6. Hot reload of UI documents works

## Next Steps

1. Review and approve this architecture
2. Create contract files (`IGraphicsContext`, `IUIRenderBackend`)
3. Implement `OpenGLUIRenderBackend`
4. Implement `VulkanUIRenderBackend`
5. Refactor `RmlUISystem` to use `IGraphicsContext`
6. Test with snake game
7. Test with 2D examples
