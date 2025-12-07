#version 410 core

// Depth Visualization Debug Shader
// Shows depth buffer as grayscale

out vec4 FragColor;

in vec3 vWorldPos;

uniform vec3 uCameraPos;
uniform float uNearPlane;
uniform float uFarPlane;

void main() {
    // Linear depth
    float depth = length(vWorldPos - uCameraPos);
    float linearDepth = (depth - uNearPlane) / (uFarPlane - uNearPlane);
    linearDepth = clamp(linearDepth, 0.0, 1.0);

    // Visualize as grayscale (closer = darker, farther = lighter)
    vec3 color = vec3(linearDepth);

    FragColor = vec4(color, 1.0);
}
