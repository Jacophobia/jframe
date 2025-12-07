#version 410 core

// Outline/Silhouette Fragment Shader
// Solid color for outline rendering
//
// Usage: Render this first with backface culling off, then render normal object on top

out vec4 FragColor;

uniform vec4 uOutlineColor;

void main() {
    FragColor = uOutlineColor;
}
