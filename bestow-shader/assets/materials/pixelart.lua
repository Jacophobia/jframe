-- Pixel Art Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/pixelart.frag"
    },
    uniforms = {
        uBaseColor = {0.8, 0.4, 0.2, 1.0},
        uPaletteSize = 8,
        uPixelSize = 0.1,
        uDithering = true,
        uOutlineThreshold = 0.3
    },
    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}
