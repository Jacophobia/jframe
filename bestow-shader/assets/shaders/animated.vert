// Animated Vertex Shader - Includes vertex animation support
#version 410 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec4 aColor;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vTexCoord;
out vec4 vColor;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;
uniform float uTime;

// Animation parameters
uniform float uWindStrength;     // Wind animation strength (default: 0.0)
uniform float uWindFrequency;    // Wind frequency (default: 1.0)
uniform float uWaveAmplitude;    // Wave amplitude (default: 0.0)
uniform float uWaveFrequency;    // Wave frequency (default: 1.0)

void main() {
    vec3 position = aPosition;

    // Apply wind animation (useful for foliage)
    if (uWindStrength > 0.0) {
        float windPhase = (position.x + position.z) * 0.5 + uTime * uWindFrequency;
        position.x += sin(windPhase) * uWindStrength * aTexCoord.y;
        position.z += cos(windPhase * 0.7) * uWindStrength * 0.5 * aTexCoord.y;
    }

    // Apply wave animation (useful for water, cloth)
    if (uWaveAmplitude > 0.0) {
        float wavePhase = (position.x * 2.0 + position.z) + uTime * uWaveFrequency;
        position.y += sin(wavePhase) * uWaveAmplitude;
    }

    vec4 worldPos = uModel * vec4(position, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal = normalize(uNormalMatrix * aNormal);
    vTexCoord = aTexCoord;
    vColor = aColor;

    gl_Position = uProjection * uView * worldPos;
}
