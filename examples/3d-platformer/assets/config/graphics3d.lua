-- Graphics3D Configuration
-- This file controls rendering pipeline settings for both OpenGL and Vulkan backends.
-- Changes to this file are hot-reloaded automatically via file watching.

--============================================================================
-- PRODUCTION LINEAR WORKFLOW
--============================================================================
-- This config uses a linear color space workflow for physically correct rendering:
--
-- How it works:
--   1. All lighting math happens in LINEAR space (physically accurate)
--   2. Textures are assumed sRGB and converted to linear on sample
--   3. Final output is gamma-corrected by hardware (swapchain/framebuffer)
--
-- Benefits:
--   - Physically correct light falloff and blending
--   - Proper specular highlights
--   - Better color mixing (no muddy darks)
--   - HDR-ready pipeline
--
-- Color values in this file are in LINEAR space (not sRGB).
-- Use these approximate conversions:
--   sRGB 0.5 ≈ Linear 0.21
--   sRGB 0.7 ≈ Linear 0.45
--   sRGB 0.8 ≈ Linear 0.60
--   sRGB 0.9 ≈ Linear 0.79
--============================================================================

return {
    --========================================================================
    -- Unified Rendering Settings (apply to all backends)
    --========================================================================

    -- Hardware gamma correction
    -- When true: shaders output linear colors, hardware converts to sRGB
    -- When false: shaders must output pre-gamma-corrected colors
    gammaCorrection = true,

    -- MSAA samples (1 = disabled, 2, 4, 8)
    msaaSamples = 4,

    -- V-Sync mode
    -- "off"      - No sync (may tear, lowest latency)
    -- "on"       - V-Sync enabled (default, no tearing)
    -- "adaptive" - Triple buffering where supported (low latency, no tearing)
    vsync = "on",

    -- Shader Search Paths (in order of priority)
    -- Use :assets:/ for game-specific assets (relative to executable)
    -- Use :library:/ for engine-provided assets (Bestow library shaders)
    shaderPaths = {
        ":assets:/shaders",             -- Game-specific shaders first
        ":library:/shaders"             -- Engine-provided shader fallback
    },

    --========================================================================
    -- Vulkan-Only Settings (features unique to Vulkan)
    --========================================================================
    vulkan = {
        -- Enable validation layers in debug builds (Vulkan-only feature)
        validationLayers = true,
    },

    --========================================================================
    -- Global Lighting (values in LINEAR space)
    --========================================================================
    lighting = {
        -- Directional light direction (normalized automatically)
        -- Points toward the light source (sun direction)
        lightDirection = {0.5, -1.0, 0.3},

        -- Light color in LINEAR space (warm sunlight)
        -- sRGB {1.0, 0.95, 0.9} ≈ Linear {1.0, 0.89, 0.79}
        lightColor = {1.0, 0.89, 0.79},

        -- Ambient light color in LINEAR space (cool sky bounce)
        -- sRGB {0.4, 0.45, 0.55} ≈ Linear {0.13, 0.17, 0.26}
        ambientColor = {0.13, 0.17, 0.26},

        -- Ambient intensity multiplier
        ambientIntensity = 0.3,
    },

    --========================================================================
    -- Clear Color / Sky (values in LINEAR space)
    --========================================================================
    -- Sky blue in LINEAR space
    -- sRGB {0.529, 0.808, 0.922} ≈ Linear {0.24, 0.62, 0.83}
    clearColor = {0.24, 0.62, 0.83, 1.0},

    --========================================================================
    -- Debug Options
    --========================================================================
    debug = {
        -- Show wireframe overlay
        wireframe = false,

        -- Show normals as colors (useful for debugging)
        showNormals = false,

        -- Display FPS counter
        showFps = true,

        -- Hot reload shaders on file change
        hotReload = true,
    },
}
