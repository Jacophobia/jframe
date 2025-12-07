#version 410 core

// Solid Color Debug Shader
// Renders a flat color for debugging

out vec4 FragColor;

uniform vec4 uColor;

void main() {
    FragColor = uColor;
}
