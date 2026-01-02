// bestow-contract/src/bestow.uirender.cppm
// UI Render Backend interface
// Abstracts low-level 2D rendering primitives needed by UI frameworks like RmlUi

module;

#include <cstdint>
#include <span>
#include <filesystem>

export module bestow.uirender;

import bestow.types;

export namespace bestow {

//==========================================================================
// Handle Types
//==========================================================================

/// Handle to compiled UI geometry (vertices + indices on GPU)
using UIGeometryHandle = std::uint64_t;

/// Handle to a UI texture
using UITextureHandle = std::uint64_t;

/// Invalid handle constants
constexpr UIGeometryHandle InvalidUIGeometry = 0;
constexpr UITextureHandle InvalidUITexture = 0;

//==========================================================================
// Vertex Format
//==========================================================================

/// UI vertex format - matches RmlUi's vertex layout for easy conversion.
/// Position is in screen coordinates (pixels).
/// Color components are 0-255, will be converted to premultiplied alpha by backend.
struct UIVertex {
    Vec2 position;     // Screen-space position in pixels
    Color color;       // RGBA color (0-255 per component)
    Vec2 texCoord;     // Texture UV coordinates (0.0 - 1.0)
};

//==========================================================================
// Scissor Region
//==========================================================================

/// Scissor rectangle for clipping UI elements.
/// Coordinates are in screen pixels, origin at top-left.
struct UIScissorRect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

//==========================================================================
// Error Types
//==========================================================================

enum class UIRenderError {
    Success,
    NotInitialized,
    OutOfMemory,
    InvalidGeometry,
    InvalidTexture,
    TextureLoadFailed,
    ShaderCompilationFailed,
    PipelineCreationFailed,
    InternalError
};

//==========================================================================
// IUIRenderBackend Interface
//==========================================================================

/// Low-level 2D rendering interface for UI systems.
/// Implementations exist for each graphics backend (OpenGL, Vulkan).
/// The graphics system owns and creates this backend.
class IUIRenderBackend {
public:
    virtual ~IUIRenderBackend() = default;

    //======================================================================
    // Lifecycle
    //======================================================================

    /// Initialize the UI render backend.
    /// Called by the graphics system after its own initialization.
    virtual bool initialize() = 0;

    /// Shutdown and release all resources.
    virtual void shutdown() = 0;

    /// Check if the backend is initialized and ready.
    virtual bool isInitialized() const = 0;

    //======================================================================
    // Geometry Management
    //======================================================================

    /// Compile vertex and index data into GPU-optimized geometry.
    /// Returns a handle for use with renderGeometry().
    /// The geometry is immutable once compiled.
    virtual UIGeometryHandle compileGeometry(
        std::span<const UIVertex> vertices,
        std::span<const std::uint32_t> indices) = 0;

    /// Release compiled geometry and free GPU resources.
    virtual void releaseGeometry(UIGeometryHandle geometry) = 0;

    //======================================================================
    // Rendering
    //======================================================================

    /// Begin a UI rendering pass.
    /// Must be called before any renderGeometry() calls.
    /// Sets up state for 2D overlay rendering:
    /// - Disables depth testing
    /// - Enables blending (premultiplied alpha)
    /// - Sets orthographic projection
    virtual void beginUIPass() = 0;

    /// Render compiled geometry.
    /// @param geometry Handle from compileGeometry()
    /// @param translation Offset in screen pixels
    /// @param texture Texture handle, or InvalidUITexture for solid color
    virtual void renderGeometry(
        UIGeometryHandle geometry,
        Vec2 translation,
        UITextureHandle texture = InvalidUITexture) = 0;

    /// End the UI rendering pass.
    /// Restores previous render state.
    virtual void endUIPass() = 0;

    //======================================================================
    // Texture Management
    //======================================================================

    /// Load a texture from file path.
    /// @param path Path to texture file (PNG, JPG, etc.)
    /// @param outWidth Receives the texture width
    /// @param outHeight Receives the texture height
    /// @return Texture handle or InvalidUITexture on failure
    virtual UITextureHandle loadTexture(
        const std::filesystem::path& path,
        int& outWidth,
        int& outHeight) = 0;

    /// Create a texture from raw RGBA8 pixel data.
    /// Used for runtime-generated textures (font atlases, etc.)
    /// @param data RGBA8 pixel data (4 bytes per pixel)
    /// @param width Texture width in pixels
    /// @param height Texture height in pixels
    /// @return Texture handle or InvalidUITexture on failure
    virtual UITextureHandle createTexture(
        std::span<const std::uint8_t> data,
        int width,
        int height) = 0;

    /// Release a texture and free GPU memory.
    virtual void releaseTexture(UITextureHandle texture) = 0;

    //======================================================================
    // Scissor (Clipping)
    //======================================================================

    /// Enable or disable scissor testing.
    /// When enabled, only pixels within the scissor region are drawn.
    virtual void enableScissor(bool enable) = 0;

    /// Set the scissor region.
    /// Coordinates are in screen pixels, origin at top-left.
    /// Only effective when scissor is enabled.
    virtual void setScissorRegion(const UIScissorRect& region) = 0;

    //======================================================================
    // Viewport
    //======================================================================

    /// Update the viewport size.
    /// Should be called when the window resizes.
    /// Updates the orthographic projection matrix.
    virtual void setViewportSize(int width, int height) = 0;

    /// Get current viewport size.
    virtual Size getViewportSize() const = 0;

    //======================================================================
    // Statistics (Debug)
    //======================================================================

    /// Get number of draw calls in the last frame.
    virtual std::uint32_t getDrawCallCount() const = 0;

    /// Get number of triangles rendered in the last frame.
    virtual std::uint32_t getTriangleCount() const = 0;
};

}  // namespace bestow
