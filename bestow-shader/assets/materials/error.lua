-- Error Material (missing material fallback)
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/error.frag"
    },
    uniforms = {},
    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}
