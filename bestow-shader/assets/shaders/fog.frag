// Fog Post-Process Shader - Distance fog
#version 410 core

out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D uSceneTexture;  // Scene color texture
uniform sampler2D uDepthTexture;  // Depth texture
uniform vec3 uFogColor;           // Fog color (default: light gray)
uniform float uFogDensity;        // Fog density (default: 0.05)
uniform float uFogStart;          // Fog start distance (default: 10.0)
uniform float uFogEnd;            // Fog end distance (default: 100.0)
uniform bool uExponential;        // Use exponential fog (default: false)

void main() {
    vec3 sceneColor = texture(uSceneTexture, vTexCoord).rgb;
    float depth = texture(uDepthTexture, vTexCoord).r;

    // Convert depth to linear distance
    float distance = depth * 100.0; // Simplified - would need proper linearization

    float fogFactor;
    if (uExponential) {
        // Exponential fog
        fogFactor = exp(-uFogDensity * distance);
    } else {
        // Linear fog
        fogFactor = (uFogEnd - distance) / (uFogEnd - uFogStart);
    }

    fogFactor = clamp(fogFactor, 0.0, 1.0);

    // Mix scene color with fog
    vec3 color = mix(uFogColor, sceneColor, fogFactor);

    FragColor = vec4(color, 1.0);
}
