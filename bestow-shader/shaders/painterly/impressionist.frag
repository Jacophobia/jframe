#version 410 core

// Impressionist Style Fragment Shader
// Visible brush strokes with dabs of color (post-process style)
//
// Configurable Parameters:
// - uStrokeSize: Size of brush dabs (default: 8.0)
// - uColorJitter: Amount of color variation (default: 0.15)
// - uStrokeIntensity: Visibility of strokes (default: 0.7)

out vec4 FragColor;

in vec2 vTexCoord;
in vec4 vColor;

uniform sampler2D uTexture;
uniform vec2 uResolution;
uniform float uStrokeSize;
uniform float uColorJitter;
uniform float uStrokeIntensity;

// Hash for randomness
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

void main() {
    vec2 uv = vTexCoord;
    vec2 pixelSize = 1.0 / uResolution;

    // Determine which "brush stroke" this pixel belongs to
    vec2 strokeCoord = floor(uv * uResolution / uStrokeSize);
    vec2 strokeCenter = (strokeCoord + 0.5) * uStrokeSize / uResolution;

    // Random stroke direction per region
    float angle = hash(strokeCoord) * 6.28318;
    vec2 strokeDir = vec2(cos(angle), sin(angle));

    // Sample color from stroke center with offset
    vec2 offset = strokeDir * pixelSize * hash(strokeCoord + vec2(100.0)) * uStrokeSize * 0.5;
    vec4 baseColor = texture(uTexture, strokeCenter + offset);

    // Add color variation
    vec3 jitter = vec3(
        hash(strokeCoord),
        hash(strokeCoord + vec2(43.0)),
        hash(strokeCoord + vec2(17.0))
    ) * 2.0 - 1.0;

    vec3 color = baseColor.rgb + jitter * uColorJitter;
    color = clamp(color, 0.0, 1.0);

    // Distance from stroke center for dab shape
    vec2 localPos = (uv - strokeCenter) * uResolution;
    float dist = length(localPos);
    float strokeMask = smoothstep(uStrokeSize * 0.6, uStrokeSize * 0.4, dist);

    // Mix between original and stroke pattern
    vec3 originalColor = texture(uTexture, uv).rgb;
    vec3 finalColor = mix(originalColor, color, strokeMask * uStrokeIntensity);

    FragColor = vec4(finalColor, baseColor.a);
}
