-- Hologram Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/hologram.frag"
    },
    uniforms = {
        uBaseColor = {0.3, 0.8, 1.0, 0.5},
        uHoloColor = {0.3, 0.8, 1.0},
        uScanlineSpeed = 2.0,
        uScanlineDensity = 20.0,
        uFlickerSpeed = 10.0,
        uGlitchAmount = 0.1,
        uTransparency = 0.5
    },
    blendMode = "blend",
    cullMode = "back",
    depthWrite = false,
    depthTest = true,
    hotReload = true
}
