// Painterly Shader - Oil painting/brushstroke effect
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
uniform float uTime;

// Painterly parameters
uniform float uBrushSize;        // Brush stroke size (default: 0.05)
uniform float uBrushStrength;    // Brush texture strength (default: 0.3)
uniform float uColorVariation;   // Color variation (default: 0.15)
uniform float uImpasto;          // Impasto/thickness effect (default: 0.2)

// Noise function for brush strokes
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

// Directional brush stroke pattern
float brushStroke(vec2 uv, vec3 normal) {
    // Create stroke direction based on surface normal
    vec2 direction = normalize(normal.xy);

    // Rotate UV based on direction
    float angle = atan(direction.y, direction.x);
    float c = cos(angle);
    float s = sin(angle);
    vec2 rotatedUV = vec2(
        uv.x * c - uv.y * s,
        uv.x * s + uv.y * c
    );

    // Multi-scale noise for brush texture
    float brush = 0.0;
    brush += noise(rotatedUV / uBrushSize * 8.0) * 0.5;
    brush += noise(rotatedUV / uBrushSize * 16.0) * 0.3;
    brush += noise(rotatedUV / uBrushSize * 32.0) * 0.2;

    return brush;
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uLightDir);
    vec3 V = normalize(uCameraPos - vWorldPos);

    // Lighting
    float NdotL = max(dot(N, L), 0.0);

    // Create brush stroke pattern
    vec2 uv = vWorldPos.xy * 10.0;
    float brush = brushStroke(uv, N);

    // Add color variation for painterly look
    vec3 colorVariation = vec3(
        noise(uv * 3.0),
        noise(uv * 3.0 + 100.0),
        noise(uv * 3.0 + 200.0)
    ) * uColorVariation;

    // Base color with variation
    vec3 baseColor = uBaseColor.rgb + colorVariation - uColorVariation * 0.5;
    baseColor = clamp(baseColor, 0.0, 1.0);

    // Apply lighting with brush texture
    vec3 diffuse = baseColor * uLightColor * NdotL;
    vec3 ambient = baseColor * uAmbientColor;

    vec3 color = diffuse + ambient;

    // Apply brush stroke texture
    color *= mix(1.0, brush, uBrushStrength);

    // Impasto effect - highlights get brighter on edges
    float impasto = pow(1.0 - abs(dot(N, V)), 2.0) * uImpasto;
    color += vec3(impasto) * NdotL;

    // Slight saturation boost
    float luma = dot(color, vec3(0.299, 0.587, 0.114));
    color = mix(vec3(luma), color, 1.2);

    FragColor = vec4(color, uBaseColor.a);
}
