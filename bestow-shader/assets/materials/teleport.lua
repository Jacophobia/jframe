-- Teleport Effect Material
return {
    shader = {
        vertex = "shaders/basic.vert",
        fragment = "shaders/teleport.frag"
    },
    uniforms = {
        uBaseColor = {0.7, 0.7, 0.7, 1.0},
        uTeleportProgress = 0.0,  -- Animate from 0 to 1
        uParticleColor = {1.0, 1.0, 1.0},
        uParticleSpeed = 2.0,
        uParticleDensity = 50.0,
        uDissolveEdge = 0.1
    },
    blendMode = "blend",
    cullMode = "back",
    depthWrite = false,
    depthTest = true,
    hotReload = true
}
