#version 410 core

// UV Coordinate Visualization Debug Shader
// Shows UV coordinates as colors (U=Red, V=Green)

out vec4 FragColor;

in vec2 vTexCoord;

uniform bool uShowGrid;

void main() {
    vec2 uv = fract(vTexCoord); // Wrap to [0, 1]

    vec3 color = vec3(uv.x, uv.y, 0.0);

    // Optional grid overlay
    if (uShowGrid) {
        vec2 grid = abs(fract(vTexCoord * 10.0) - 0.5);
        float gridLine = step(0.48, max(grid.x, grid.y));
        color = mix(color, vec3(1.0), gridLine * 0.5);
    }

    FragColor = vec4(color, 1.0);
}
