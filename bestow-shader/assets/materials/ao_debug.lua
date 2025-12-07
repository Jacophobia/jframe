-- Ambient Occlusion Debug Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/ao_debug.frag"
    },
    uniforms = {
        uAOStrength = 1.0,
        uAORadius = 1.0
    },
    blendMode = "opaque",
    cullMode = "back",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}
