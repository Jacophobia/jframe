#version 410 core

// Wireframe Debug Shader
// Shows triangle edges (requires geometry shader or barycentric coords)
//
// Note: For full wireframe, use with a geometry shader that outputs barycentric coordinates
// This version uses screen-space derivatives as approximation

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;

uniform vec4 uWireColor;
uniform vec4 uFillColor;
uniform float uWireThickness;

void main() {
    // Screen-space derivative wireframe approximation
    vec3 fw = fwidth(vWorldPos);
    float edge = max(max(fw.x, fw.y), fw.z);
    edge = edge * uWireThickness;

    // Checkerboard pattern as fallback
    vec2 grid = abs(fract(vTexCoord * 10.0) - 0.5);
    float pattern = step(0.45, max(grid.x, grid.y));

    vec4 color = mix(uFillColor, uWireColor, pattern);

    FragColor = color;
}
