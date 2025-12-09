-- Cross-Hatching Material
-- Pen & ink illustration style with layered line patterns

return {
    shader = {
        vertex = ":library:/shaders/toon.vert",
        fragment = ":library:/shaders/crosshatch.frag"
    },

    uniforms = {
        uBaseColor = {0.95, 0.92, 0.85, 1.0},  -- Paper color tint

        -- Line pattern settings (in shader constants)
        uLineScale = 40.0,
        uLineWidth = 0.15,
    },

    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}
