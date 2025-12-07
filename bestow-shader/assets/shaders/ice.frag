// Ice Shader - Frozen/crystalline material
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

// Ice parameters
uniform float uFrostiness;       // Frost amount (default: 0.5)
uniform float uRefractiveIndex;  // IOR (default: 1.31)
uniform vec3 uIceTint;           // Ice color tint (default: light blue)
uniform float uCrystalSize;      // Crystal grain size (default: 0.1)
uniform float uTransparency;     // Transparency (default: 0.6)

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

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 L = normalize(-uLightDir);
    vec3 H = normalize(V + L);

    // Crystal structure noise
    vec2 uv = vWorldPos.xy / uCrystalSize;
    float crystal = noise(uv * 10.0) * 0.3 + noise(uv * 20.0) * 0.2;

    // Perturb normal for crystalline look
    vec3 crystalNormal = normalize(N + vec3(
        (noise(uv) - 0.5) * 0.2,
        (noise(uv + 100.0) - 0.5) * 0.2,
        0.0
    ));

    // Fresnel
    vec3 F0 = vec3(0.04);
    vec3 F = fresnelSchlick(max(dot(crystalNormal, V), 0.0), F0);

    // Frost pattern
    float frost = pow(noise(uv * 30.0), 2.0) * uFrostiness;

    // Base ice color
    vec3 iceColor = uIceTint * uBaseColor.rgb;

    // Refraction (simplified)
    vec3 refraction = iceColor * (1.0 - frost);

    // Reflection
    vec3 reflection = uAmbientColor * 0.7;

    // Subsurface scattering approximation
    float scatter = pow(max(dot(-L, V), 0.0), 2.0) * 0.3;
    vec3 subsurface = iceColor * uLightColor * scatter;

    // Specular highlights
    float NdotL = max(dot(crystalNormal, L), 0.0);
    float spec = pow(max(dot(crystalNormal, H), 0.0), 128.0);

    // Combine
    vec3 color = mix(refraction, reflection, F.r);
    color += subsurface;
    color += uLightColor * spec * 0.6;
    color += frost * vec3(1.0) * 0.3; // Frost is bright

    // Add crystal sparkles
    color += crystal * 0.2;

    float alpha = mix(uTransparency, 1.0, frost);

    FragColor = vec4(color, alpha);
}
