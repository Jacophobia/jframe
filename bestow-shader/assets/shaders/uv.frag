// UV Debug Shader - Visualize UV coordinates
#version 410 core

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform bool uShowGrid;          // Show UV grid (default: true)
uniform float uGridDensity;      // Grid density (default: 10.0)

void main() {
    // Map UVs to color
    vec3 color = vec3(vTexCoord, 0.0);

    if (uShowGrid) {
        // Add grid overlay
        vec2 grid = fract(vTexCoord * uGridDensity);
        float gridLine = min(min(grid.x, 1.0 - grid.x), min(grid.y, 1.0 - grid.y));
        gridLine = step(gridLine, 0.05);

        color = mix(color, vec3(1.0), gridLine * 0.5);
    }

    FragColor = vec4(color, 1.0);
}
