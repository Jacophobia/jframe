#version 410 core

// Vignette Post-Process Shader
// Darkens the edges of the screen
//
// Configurable Parameters:
// - uVignetteStrength: How dark the edges are (default: 0.5)
// - uVignetteFalloff: How quickly it fades (default: 0.5)

out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D uTexture;
uniform float uVignetteStrength;
uniform float uVignetteFalloff;

void main() {
    vec4 color = texture(uTexture, vTexCoord);

    // Distance from center
    vec2 uv = vTexCoord * 2.0 - 1.0;
    float dist = length(uv);

    // Vignette calculation
    float vignette = smoothstep(uVignetteFalloff, uVignetteFalloff - 0.5, dist);
    vignette = mix(1.0 - uVignetteStrength, 1.0, vignette);

    FragColor = vec4(color.rgb * vignette, color.a);
}
