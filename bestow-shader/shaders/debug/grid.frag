#version 410 core

// Checkerboard Grid Debug Shader
// Shows a procedural checkerboard pattern for testing

out vec4 FragColor;

in vec2 vTexCoord;

uniform vec4 uColor1;
uniform vec4 uColor2;
uniform float uGridScale;

void main() {
    vec2 grid = floor(vTexCoord * uGridScale);
    float checker = mod(grid.x + grid.y, 2.0);

    vec4 color = mix(uColor1, uColor2, checker);

    FragColor = color;
}
