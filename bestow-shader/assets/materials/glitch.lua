-- Glitch Effect Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/glitch.frag"
    },
    uniforms = {
        uBaseColor = {0.7, 0.7, 0.7, 1.0},
        uGlitchIntensity = 0.5,
        uGlitchSpeed = 5.0,
        uBlockSize = 0.1,
        uGlitchColor = {0.0, 1.0, 1.0}
    },
    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}
