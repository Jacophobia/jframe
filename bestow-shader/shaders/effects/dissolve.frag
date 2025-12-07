#version 410 core

// Dissolve/Disintegration Fragment Shader
// Creates a burning/dissolving effect with customizable edge glow
//
// Configurable Parameters:
// - uDissolveAmount: Progress of dissolve (0=solid, 1=gone) (default: 0.5)
// - uEdgeWidth: Width of the dissolve edge (default: 0.1)
// - uEdgeColor: Color of dissolve edge (default: orange)
// - uNoiseScale: Scale of dissolve pattern (default: 5.0)

out vec4 FragColor;

in vec3 vWorldPos;
in vec2 vTexCoord;
in vec4 vColor;

uniform vec4 uBaseColor;
uniform float uDissolveAmount;
uniform float uEdgeWidth;
uniform vec3 uEdgeColor;
uniform float uNoiseScale;
uniform float uTime;
uniform bool uUseTexture;
uniform sampler2D uTexture;

// 3D noise function
float hash(vec3 p) {
    p = fract(p * 0.3183099 + 0.1);
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float noise(vec3 x) {
    vec3 i = floor(x);
    vec3 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);

    return mix(
        mix(mix(hash(i + vec3(0, 0, 0)), hash(i + vec3(1, 0, 0)), f.x),
            mix(hash(i + vec3(0, 1, 0)), hash(i + vec3(1, 1, 0)), f.x), f.y),
        mix(mix(hash(i + vec3(0, 0, 1)), hash(i + vec3(1, 0, 1)), f.x),
            mix(hash(i + vec3(0, 1, 1)), hash(i + vec3(1, 1, 1)), f.x), f.y),
        f.z
    );
}

float fbm(vec3 p) {
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < 5; i++) {
        value += amplitude * noise(p);
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

void main() {
    vec4 color = uBaseColor * vColor;
    if (uUseTexture) {
        color *= texture(uTexture, vTexCoord);
    }

    // Generate dissolve pattern
    vec3 noisePos = vWorldPos * uNoiseScale + uTime * 0.2;
    float dissolveNoise = fbm(noisePos);

    // Dissolve threshold with edge
    float threshold = uDissolveAmount;
    float dissolve = dissolveNoise - threshold;

    // Discard dissolved pixels
    if (dissolve < 0.0) {
        discard;
    }

    // Edge glow
    float edgeFactor = smoothstep(0.0, uEdgeWidth, dissolve);
    vec3 edgeGlow = uEdgeColor * (1.0 - edgeFactor) * 3.0;

    // Combine
    vec3 finalColor = color.rgb + edgeGlow;
    float alpha = color.a * smoothstep(0.0, uEdgeWidth * 0.5, dissolve);

    FragColor = vec4(finalColor, alpha);
}
