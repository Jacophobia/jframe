// Vertex Colors Debug Shader - Display vertex colors
#version 410 core

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform bool uShowAlpha;         // Visualize alpha channel (default: false)

void main() {
    if (uShowAlpha) {
        // Show alpha as grayscale
        vec3 color = vec3(vColor.a);
        FragColor = vec4(color, 1.0);
    } else {
        // Show RGB vertex colors
        FragColor = vec4(vColor.rgb, 1.0);
    }
}
