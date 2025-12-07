// Water Vertex Shader - animated wave displacement
#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec4 aColor;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;
uniform float uTime;

// Wave parameters
uniform float uWaveHeight;     // Height of waves (default: 0.3)
uniform float uWaveSpeed;      // Speed of wave animation (default: 1.0)
uniform float uWaveFrequency;  // Frequency of waves (default: 2.0)

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vTexCoord;
out vec4 vColor;

void main() {
    // Animate vertex position with waves
    vec3 pos = aPosition;

    // Multiple overlapping waves for more natural look
    float wave1 = sin(pos.x * uWaveFrequency + uTime * uWaveSpeed) * uWaveHeight;
    float wave2 = sin(pos.z * uWaveFrequency * 0.7 + uTime * uWaveSpeed * 1.3) * uWaveHeight * 0.5;
    float wave3 = sin((pos.x + pos.z) * uWaveFrequency * 0.5 + uTime * uWaveSpeed * 0.8) * uWaveHeight * 0.3;

    pos.y += wave1 + wave2 + wave3;

    // Calculate displaced normal
    // Approximate by taking derivative of wave function
    float dx = cos(pos.x * uWaveFrequency + uTime * uWaveSpeed) * uWaveFrequency * uWaveHeight;
    float dz = cos(pos.z * uWaveFrequency * 0.7 + uTime * uWaveSpeed * 1.3) * uWaveFrequency * 0.7 * uWaveHeight * 0.5;

    vec3 normal = normalize(vec3(-dx, 1.0, -dz));

    vec4 worldPos = uModel * vec4(pos, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal = normalize(uNormalMatrix * normal);
    vTexCoord = aTexCoord;
    vColor = aColor;
    gl_Position = uProjection * uView * worldPos;
}
