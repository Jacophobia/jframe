// Depth Debug Shader - Visualize depth buffer
#version 410 core

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform vec3 uCameraPos;
uniform float uNearPlane;        // Near plane distance (default: 0.1)
uniform float uFarPlane;         // Far plane distance (default: 100.0)
uniform bool uLinearize;         // Linearize depth (default: true)

void main() {
    // Calculate depth from camera
    float depth = length(vWorldPos - uCameraPos);

    if (uLinearize) {
        // Linearize depth
        depth = (depth - uNearPlane) / (uFarPlane - uNearPlane);
    }

    depth = clamp(depth, 0.0, 1.0);

    // Visualize as grayscale
    vec3 color = vec3(depth);

    FragColor = vec4(color, 1.0);
}
