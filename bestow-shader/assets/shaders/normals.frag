// Normals Debug Shader - Visualize surface normals
#version 410 core

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform bool uWorldSpace;        // Show world-space normals (default: true)

void main() {
    vec3 N = normalize(vNormal);

    // Map normals from [-1,1] to [0,1] for visualization
    vec3 color = N * 0.5 + 0.5;

    FragColor = vec4(color, 1.0);
}
