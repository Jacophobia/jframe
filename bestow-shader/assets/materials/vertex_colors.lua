-- Vertex Colors Debug Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/vertex_colors.frag"
    },
    uniforms = {
        uShowAlpha = false
    },
    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}
