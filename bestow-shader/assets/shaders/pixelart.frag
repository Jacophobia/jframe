// Pixel Art Shader - Retro pixel art with dithering
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

// Pixel art parameters
uniform int uPaletteSize;        // Number of colors (default: 8)
uniform float uPixelSize;        // Pixel grid size (default: 0.1)
uniform bool uDithering;         // Enable dithering (default: true)
uniform float uOutlineThreshold; // Outline threshold (default: 0.3)

// Bayer matrix for dithering
const mat4 bayerMatrix = mat4(
    0.0,  8.0,  2.0, 10.0,
    12.0, 4.0, 14.0,  6.0,
    3.0, 11.0,  1.0,  9.0,
    15.0, 7.0, 13.0,  5.0
) / 16.0;

float bayerDither(vec2 pos) {
    int x = int(mod(pos.x, 4.0));
    int y = int(mod(pos.y, 4.0));
    return bayerMatrix[x][y];
}

vec3 posterize(vec3 color, int levels) {
    float step = 1.0 / float(levels - 1);
    return floor(color / step + 0.5) * step;
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uLightDir);

    // Pixelate world position
    vec2 pixelPos = floor(vWorldPos.xy / uPixelSize) * uPixelSize;

    // Calculate lighting
    float NdotL = max(dot(N, L), 0.0);

    // Apply lighting to base color
    vec3 baseColor = uBaseColor.rgb;
    vec3 lit = baseColor * (uLightColor * NdotL + uAmbientColor);

    // Posterize to limited palette
    vec3 color = posterize(lit, uPaletteSize);

    // Apply dithering
    if (uDithering) {
        vec2 ditherPos = pixelPos / uPixelSize;
        float dither = bayerDither(ditherPos);

        // Use dithering to simulate shades between palette colors
        vec3 colorHigh = posterize(lit + 1.0 / float(uPaletteSize), uPaletteSize);
        float threshold = fract(lit.r * float(uPaletteSize));
        color = mix(color, colorHigh, step(threshold, dither));
    }

    // Simple outline (dark pixels on edges)
    float outline = step(NdotL, uOutlineThreshold);
    color = mix(color, vec3(0.0), outline * 0.7);

    FragColor = vec4(color, uBaseColor.a);
}
