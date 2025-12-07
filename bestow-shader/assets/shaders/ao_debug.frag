// Ambient Occlusion Debug Shader - Visualize AO
#version 410 core

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform vec3 uCameraPos;

// Simple ambient occlusion approximation for visualization
uniform float uAOStrength;       // AO strength (default: 1.0)
uniform float uAORadius;         // AO radius (default: 1.0)

// Simple noise for AO approximation
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(hash(i), hash(i + vec2(1.0, 0.0)), f.x),
        mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), f.x),
        f.y
    );
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);

    // Simple AO approximation based on normal/view angle and noise
    float ao = 1.0 - pow(max(dot(N, V), 0.0), 2.0);

    // Add noise-based cavity approximation
    vec2 uv = vTexCoord * 20.0;
    float cavity = noise(uv) * 0.5 + noise(uv * 2.0) * 0.3;
    cavity = pow(cavity, 2.0);

    ao = mix(ao, cavity, 0.5);
    ao = 1.0 - (ao * uAOStrength);

    vec3 color = vec3(ao);

    FragColor = vec4(color, 1.0);
}
