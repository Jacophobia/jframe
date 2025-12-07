-- Comic Book Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/comic.frag"
    },
    uniforms = {
        uBaseColor = {0.9, 0.2, 0.2, 1.0},
        uDotDensity = 30.0,
        uOutlineWidth = 0.05,
        uOutlineColor = {0.0, 0.0, 0.0},
        uDotContrast = 0.8,
        uHighlightColor = {1.0, 1.0, 1.0}
    },
    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}
