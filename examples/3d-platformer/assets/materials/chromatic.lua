-- Chromatic Aberration Material
-- RGB channel separation for trippy/glitchy effect

return {
    shader = {
        vertex = ":library:/shaders/toon.vert",
        fragment = ":library:/shaders/chromatic.frag"
    },

    uniforms = {
        uBaseColor = {0.8, 0.7, 0.9, 1.0},  -- Light purple base

        -- Aberration settings (in shader constants)
        uAberrationStrength = 0.15,
        uDistortionFreq = 3.0,
    },

    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}
