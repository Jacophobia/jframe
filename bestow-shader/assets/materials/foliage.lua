-- Foliage Material (use with animated.vert for wind)
return {
    shader = {
        vertex = "shaders/animated.vert",
        fragment = "shaders/foliage.frag"
    },
    uniforms = {
        uBaseColor = {0.3, 0.6, 0.2, 1.0},
        uSubsurfaceColor = {0.5, 0.8, 0.3},
        uSubsurfaceAmount = 0.5,
        uAlphaCutoff = 0.5,
        uTwoSided = true,
        -- Vertex animation (from animated.vert)
        uWindStrength = 0.1,
        uWindFrequency = 1.0,
        uWaveAmplitude = 0.0,
        uWaveFrequency = 0.0
    },
    blendMode = "alpha_test",
    cullMode = "none",
    depthWrite = true,
    depthTest = true,
    hotReload = true
}
