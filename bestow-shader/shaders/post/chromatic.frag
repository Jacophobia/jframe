#version 410 core

// Chromatic Aberration Post-Process Shader
// Simulates lens chromatic aberration (color fringing)
//
// Configurable Parameters:
// - uAberrationStrength: Amount of color separation (default: 0.005)

out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D uTexture;
uniform float uAberrationStrength;

void main() {
    vec2 uv = vTexCoord;

    // Direction from center
    vec2 direction = uv - 0.5;
    float dist = length(direction);

    // Sample RGB channels with offset
    vec2 offset = normalize(direction) * uAberrationStrength * dist;

    float r = texture(uTexture, uv - offset).r;
    float g = texture(uTexture, uv).g;
    float b = texture(uTexture, uv + offset).b;

    FragColor = vec4(r, g, b, 1.0);
}
