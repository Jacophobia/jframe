// Error Shader - Magenta error shader (missing material fallback)
#version 410 core

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform float uTime;

void main() {
    // Animated magenta/black checkerboard (classic error pattern)
    vec2 checker = floor((vTexCoord + uTime * 0.5) * 8.0);
    float pattern = mod(checker.x + checker.y, 2.0);

    vec3 magenta = vec3(1.0, 0.0, 1.0);
    vec3 black = vec3(0.0);

    vec3 color = mix(magenta, black, pattern);

    FragColor = vec4(color, 1.0);
}
