// bestow-contract/src/bestow.graphics.context.cppm
// Base interface for all graphics systems (2D and 3D)
// Provides common capabilities needed by cross-cutting systems like UI

module;

#include <cstdint>

export module bestow.graphics.context;

import bestow.types;
import bestow.uirender;  // For IUIRenderBackend

export namespace bestow {

/// Base interface for all graphics systems.
/// Both IGraphicsSystem (2D) and IGraphics3DSystem (3D) extend this.
/// Provides access to shared functionality needed by UI and other overlay systems.
class IGraphicsContext {
public:
    virtual ~IGraphicsContext() = default;

    //======================================================================
    // UI Render Backend
    //======================================================================

    /// Get the UI render backend for this graphics system.
    /// The graphics system owns the backend - caller should not delete.
    /// Returns nullptr if UI rendering is not supported.
    /// The backend is created lazily on first call.
    virtual IUIRenderBackend* getUIRenderBackend() = 0;

    //======================================================================
    // Window/Viewport Information
    //======================================================================

    /// Get current window size in pixels.
    /// Note: Both IGraphicsSystem and IGraphics3DSystem already have this method,
    /// so implementations can simply delegate to their existing getWindowSize().
    virtual Size getWindowSize() const = 0;

    /// Get native window handle (e.g., GLFWwindow*).
    /// Used for input integration.
    virtual void* getNativeWindowHandle() const = 0;

    //======================================================================
    // Frame State
    //======================================================================

    /// Check if currently inside a frame (between beginFrame/endFrame).
    /// UI rendering should only occur within a frame.
    virtual bool isInFrame() const = 0;

    //======================================================================
    // Render Context Access (for backend implementation)
    //======================================================================

    /// Get renderer-specific context for internal use.
    /// Returns:
    /// - Vulkan: pointer to internal VulkanContext struct
    /// - OpenGL: nullptr (context is thread-bound)
    /// - Metal: pointer to MTLDevice (future)
    /// This is used by IUIRenderBackend implementations, not by game code.
    virtual void* getRenderContext() const = 0;

    /// Get current command buffer/state for synchronized rendering.
    /// Returns:
    /// - Vulkan: VkCommandBuffer
    /// - OpenGL: nullptr
    /// Used by UI backend to record into the current frame's commands.
    virtual void* getCurrentCommandBuffer() const = 0;
};

}  // namespace bestow
