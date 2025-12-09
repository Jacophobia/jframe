-- Hologram Material
-- Sci-fi projection effect with scanlines and flickering

return {
    shader = {
        vertex = ":library:/shaders/toon.vert",
        fragment = ":library:/shaders/hologram.frag"
    },

    uniforms = {
        uBaseColor = {0.2, 0.8, 1.0, 0.8},  -- Cyan hologram with alpha

        -- Hologram settings (in shader constants)
        uScanlineScale = 200.0,
        uFlickerSpeed = 3.0,
    },

    blendMode = "alpha",  -- Transparent hologram
    cullMode = "none",    -- Visible from both sides
    depthWrite = false,   -- Don't occlude other objects
    depthTest = true,
    hotReload = true
}
