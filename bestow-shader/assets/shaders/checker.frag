// Checker Debug Shader - Checkerboard pattern for UV testing
#version 410 core

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform vec3 uColor1;            // First checker color (default: white)
uniform vec3 uColor2;            // Second checker color (default: black)
uniform float uDensity;          // Checker density (default: 8.0)

void main() {
    vec2 checker = floor(vTexCoord * uDensity);
    float pattern = mod(checker.x + checker.y, 2.0);

    vec3 color = mix(uColor1, uColor2, pattern);

    FragColor = vec4(color, 1.0);
}
