// Cross-Hatching Shader - Pen-and-ink sketch effect
#version 410 core

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform vec4 uBaseColor;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbientColor;
uniform vec3 uCameraPos;

// Hatching parameters
uniform float uHatchDensity;     // Lines per unit (default: 20.0)
uniform float uLineThickness;    // Line thickness (default: 0.3)
uniform int uHatchLayers;        // Number of cross-hatch directions (default: 3)
uniform vec3 uPaperColor;        // Paper color (default: 0.95, 0.92, 0.88)
uniform vec3 uInkColor;          // Ink color (default: 0.1, 0.1, 0.1)

// Generate hatching pattern at different angles
float hatchPattern(vec2 coord, float angle, float density, float thickness) {
    float c = cos(angle);
    float s = sin(angle);
    vec2 rotated = vec2(
        coord.x * c - coord.y * s,
        coord.x * s + coord.y * c
    );
    float line = fract(rotated.x * density);
    return smoothstep(thickness, thickness + 0.05, line);
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uLightDir);

    // Calculate lighting intensity
    float NdotL = max(dot(N, L), 0.0);
    float intensity = NdotL * 0.7 + 0.3; // Add ambient base

    // Use world position for consistent hatching
    vec2 coord = vWorldPos.xy * 0.1;

    // Layer multiple hatching directions based on darkness
    float hatch = 1.0;

    // First layer (horizontal-ish)
    if (intensity < 0.95) {
        float pattern = hatchPattern(coord, 0.0, uHatchDensity, uLineThickness);
        hatch *= pattern;
    }

    // Second layer (vertical-ish)
    if (intensity < 0.7 && uHatchLayers >= 2) {
        float pattern = hatchPattern(coord, 1.5708, uHatchDensity, uLineThickness);
        hatch *= pattern;
    }

    // Third layer (diagonal)
    if (intensity < 0.4 && uHatchLayers >= 3) {
        float pattern = hatchPattern(coord, 0.7854, uHatchDensity * 0.8, uLineThickness);
        hatch *= pattern;
    }

    // Fourth layer (opposite diagonal)
    if (intensity < 0.2 && uHatchLayers >= 4) {
        float pattern = hatchPattern(coord, -0.7854, uHatchDensity * 0.8, uLineThickness);
        hatch *= pattern;
    }

    // Mix paper and ink colors
    vec3 color = mix(uInkColor, uPaperColor, hatch);

    // Apply base color tint
    color *= uBaseColor.rgb;

    FragColor = vec4(color, 1.0);
}
