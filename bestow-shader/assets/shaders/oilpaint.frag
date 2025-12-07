// Oil Paint Shader - Thick oil painting effect
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

// Oil paint parameters
uniform float uBrushSize;        // Brush stroke size (default: 0.08)
uniform float uThickness;        // Paint thickness (default: 0.4)
uniform float uColorVariation;   // Color variation (default: 0.2)
uniform float uGlossiness;       // Surface glossiness (default: 0.3)

// Hash and noise functions
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(hash(i), hash(i + vec2(1.0, 0.0)), f.x),
        mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), f.x),
        f.y
    );
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uLightDir);
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 H = normalize(L + V);

    // Create brush stroke pattern
    vec2 uv = vWorldPos.xy / uBrushSize;

    // Directional strokes based on normal
    vec2 strokeDir = normalize(N.xy);
    float angle = atan(strokeDir.y, strokeDir.x);
    float c = cos(angle);
    float s = sin(angle);
    vec2 rotatedUV = vec2(uv.x * c - uv.y * s, uv.x * s + uv.y * c);

    // Multi-scale brush texture
    float brush = 0.0;
    brush += noise(rotatedUV * 4.0) * 0.4;
    brush += noise(rotatedUV * 8.0) * 0.3;
    brush += noise(rotatedUV * 16.0) * 0.3;

    // Color variation from brush strokes
    vec3 colorVar = vec3(
        noise(uv * 2.0),
        noise(uv * 2.0 + 50.0),
        noise(uv * 2.0 + 100.0)
    ) * uColorVariation - uColorVariation * 0.5;

    // Base color with variation
    vec3 baseColor = uBaseColor.rgb + colorVar;
    baseColor = clamp(baseColor, 0.0, 1.0);

    // Lighting
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = baseColor * uLightColor * NdotL;
    vec3 ambient = baseColor * uAmbientColor;

    // Glossy specular (oil paint has some sheen)
    float NdotH = max(dot(N, H), 0.0);
    float spec = pow(NdotH, 32.0) * uGlossiness;
    vec3 specular = uLightColor * spec;

    // Impasto effect - thick paint catches light on edges
    float impasto = pow(1.0 - max(dot(N, V), 0.0), 2.0) * uThickness;
    vec3 impastoHighlight = uLightColor * impasto * NdotL;

    vec3 color = ambient + diffuse + specular + impastoHighlight;

    // Apply brush texture
    color *= (0.8 + brush * 0.4);

    // Boost saturation and richness
    float luma = dot(color, vec3(0.299, 0.587, 0.114));
    color = mix(vec3(luma), color, 1.15);

    FragColor = vec4(color, uBaseColor.a);
}
