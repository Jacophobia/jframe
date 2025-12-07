// Grid/Checkerboard Shader - useful for prototyping and level design
#version 330 core

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbientColor;

// Grid parameters
uniform vec3 uColor1;          // First grid color (default: 0.3, 0.3, 0.3)
uniform vec3 uColor2;          // Second grid color (default: 0.5, 0.5, 0.5)
uniform float uGridScale;      // Size of grid squares (default: 1.0)
uniform float uLineWidth;      // Width of grid lines (default: 0.02)
uniform vec3 uLineColor;       // Color of grid lines (default: 0.2, 0.2, 0.2)
uniform bool uShowLines;       // Whether to show grid lines

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uLightDir);

    // Use world position for consistent grid across objects
    vec2 gridPos = vWorldPos.xz / uGridScale;

    // Checkerboard pattern
    vec2 checker = floor(gridPos);
    float pattern = mod(checker.x + checker.y, 2.0);

    // Base color from checkerboard
    vec3 baseColor = mix(uColor1, uColor2, pattern);

    // Grid lines (optional)
    if (uShowLines) {
        vec2 gridFrac = fract(gridPos);
        vec2 gridDist = min(gridFrac, 1.0 - gridFrac);
        float line = step(min(gridDist.x, gridDist.y), uLineWidth);
        baseColor = mix(baseColor, uLineColor, line);
    }

    // Apply simple diffuse lighting
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = baseColor * NdotL * uLightColor;
    vec3 ambient = uAmbientColor * baseColor;

    vec3 color = ambient + diffuse;

    // Gamma correction
    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color * vColor.rgb, vColor.a);
}
