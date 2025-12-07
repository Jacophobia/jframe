-- Toon/Cel Shading Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/toon.frag"
    },
    uniforms = {
        uBaseColor = {0.8, 0.2, 0.3, 1.0},
        uBands = 3,
        uOutlineWidth = 0.03,
        uOutlineColor = {0.0, 0.0, 0.0},
        uSpecularSize = 0.9,
        uShadowTint = {0.5, 0.5, 0.8},
        uSmoothness = 0.05
    },
    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}
