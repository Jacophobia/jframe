#version 410 core

// Water Material Vertex Shader
// Animates waves and passes data for water rendering

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec4 aColor;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;
uniform float uTime;
uniform float uWaveHeight;
uniform float uWaveFrequency;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vTexCoord;
out vec4 vColor;

// Simple wave function
float wave(vec2 pos, float freq, float time) {
    return sin(pos.x * freq + time) * cos(pos.y * freq * 0.7 + time * 0.7);
}

void main() {
    vec3 pos = aPosition;

    // Apply multiple wave frequencies
    float h = 0.0;
    h += wave(pos.xz, uWaveFrequency, uTime * 2.0) * 0.5;
    h += wave(pos.xz * 1.5, uWaveFrequency * 1.3, uTime * 1.5) * 0.3;
    h += wave(pos.xz * 2.3, uWaveFrequency * 2.1, uTime * 2.5) * 0.2;

    pos.y += h * uWaveHeight;

    // Calculate new normal based on wave
    vec3 normal = aNormal;
    float epsilon = 0.1;
    vec3 tangent = vec3(epsilon,
        wave(pos.xz + vec2(epsilon, 0), uWaveFrequency, uTime * 2.0) * uWaveHeight -
        wave(pos.xz - vec2(epsilon, 0), uWaveFrequency, uTime * 2.0) * uWaveHeight,
        0.0);
    vec3 bitangent = vec3(0.0,
        wave(pos.xz + vec2(0, epsilon), uWaveFrequency, uTime * 2.0) * uWaveHeight -
        wave(pos.xz - vec2(0, epsilon), uWaveFrequency, uTime * 2.0) * uWaveHeight,
        epsilon);
    normal = normalize(cross(tangent, bitangent));

    vec4 worldPos = uModel * vec4(pos, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal = normalize(uNormalMatrix * normal);
    vTexCoord = aTexCoord;
    vColor = aColor;
    gl_Position = uProjection * uView * worldPos;
}
