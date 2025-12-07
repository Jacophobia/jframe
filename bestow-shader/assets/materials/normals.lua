-- Normals Debug Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/normals.frag"
    },
    uniforms = {
        uWorldSpace = true
    },
    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}
