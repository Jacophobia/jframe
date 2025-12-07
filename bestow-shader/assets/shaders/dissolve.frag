// Dissolve Shader - Burn/dissolve away effect
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
uniform float uTime;

// Dissolve parameters
uniform float uDissolveAmount;   // Dissolve progress 0-1 (default: 0.0)
uniform vec3 uEdgeColor;         // Burn edge color (default: orange)
uniform vec3 uEdgeColor2;        // Secondary edge color (default: yellow)
uniform float uEdgeWidth;        // Edge glow width (default: 0.1)
uniform float uNoiseScale;       // Noise pattern scale (default: 5.0)

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

    // Dissolve noise pattern
    vec2 uv = vTexCoord * uNoiseScale;
    float dissolveNoise = noise(uv) * 0.6 + noise(uv * 2.0) * 0.4;

    // Dissolve threshold
    float dissolve = dissolveNoise - uDissolveAmount;

    // Discard dissolved pixels
    if (dissolve < 0.0) {
        discard;
    }

    // Edge glow
    float edgeFactor = smoothstep(0.0, uEdgeWidth, dissolve);

    // Base lighting
    float NdotL = max(dot(N, L), 0.0);
    vec3 baseColor = uBaseColor.rgb;
    vec3 lit = baseColor * (uLightColor * NdotL + uAmbientColor * 0.5);

    // Edge colors (hot to cool gradient)
    vec3 edgeGlow = mix(uEdgeColor, uEdgeColor2, edgeFactor / uEdgeWidth);

    // Combine
    vec3 color = mix(edgeGlow * 3.0, lit, edgeFactor);

    FragColor = vec4(color, 1.0);
}
