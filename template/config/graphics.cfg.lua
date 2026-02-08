-- Graphics configuration
-- All fields are explicit with sane defaults. Modify as needed.

return {
    -- Window settings
    window = {
        title       = "My Game",
        width       = 1920,
        height      = 1080,
        fullscreen  = false,        -- true = exclusive fullscreen
        vsync       = true,
    },

    -- Rendering pipeline
    rendering = {
        clearColor      = { r = 0.05, g = 0.05, b = 0.08, a = 1.0 },
        renderScale     = 1.0,      -- 1.0 = native, 0.5 = half resolution
        frustumCulling  = true,
        toneMapping     = true,
        exposure        = 1.0,

        bloom = {
            enabled     = true,
            threshold   = 1.0,
            intensity   = 0.3,
        },

        ssao = {
            enabled     = true,
            radius      = 0.5,
            intensity   = 1.0,
        },

        shadows = {
            enabled     = true,
            resolution  = 2048,     -- Shadow map size (512, 1024, 2048, 4096)
            distance    = 100.0,    -- Max shadow draw distance
        },
    },

    -- Camera defaults
    camera = {
        fov         = 60.0,         -- Vertical field of view in degrees
        near        = 0.1,
        far         = 1000.0,
        projection  = "Perspective", -- "Perspective" or "Orthographic"
    },

    -- Ambient lighting
    lighting = {
        ambient = { r = 0.1, g = 0.1, b = 0.15 },
    },
}
