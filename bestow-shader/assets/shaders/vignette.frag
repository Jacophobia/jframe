// Vignette Post-Process Shader - Darkened edges
#version 410 core

out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D uSceneTexture;  // Scene color texture
uniform float uIntensity;         // Vignette intensity (default: 0.5)
uniform float uPower;             // Vignette power/falloff (default: 2.0)
uniform vec3 uColor;              // Vignette color (default: black)

void main() {
    vec3 sceneColor = texture(uSceneTexture, vTexCoord).rgb;

    // Distance from center
    vec2 uv = vTexCoord - 0.5;
    float dist = length(uv);

    // Vignette factor
    float vignette = 1.0 - pow(dist * 2.0, uPower) * uIntensity;
    vignette = clamp(vignette, 0.0, 1.0);

    // Apply vignette
    vec3 color = mix(uColor, sceneColor, vignette);

    FragColor = vec4(color, 1.0);
}
