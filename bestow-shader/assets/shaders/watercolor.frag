// Watercolor Shader - Soft, flowing watercolor paint effect
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

// Watercolor parameters
uniform float uBleedAmount;      // Color bleeding (default: 0.3)
uniform float uPaperTexture;     // Paper grain strength (default: 0.2)
uniform float uEdgeDarkening;    // Dark edges (default: 0.4)
uniform float uPigmentDensity;   // Pigment concentration (default: 0.6)

// Noise functions
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

float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < 4; i++) {
        value += amplitude * noise(p);
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uLightDir);
    vec3 V = normalize(uCameraPos - vWorldPos);

    // Lighting
    float NdotL = max(dot(N, L), 0.0);

    // Paper texture
    vec2 uv = vWorldPos.xy * 20.0;
    float paper = fbm(uv) * uPaperTexture;

    // Color bleeding effect
    vec2 bleedUV = uv * 0.5;
    float bleed = fbm(bleedUV) * uBleedAmount;

    // Edge darkening (pigment accumulation)
    float fresnel = 1.0 - max(dot(N, V), 0.0);
    float edgeDark = pow(fresnel, 2.0) * uEdgeDarkening;

    // Pigment density variation
    float pigment = fbm(uv * 0.3) * uPigmentDensity;

    // Base color with variations
    vec3 baseColor = uBaseColor.rgb;
    baseColor *= (1.0 - bleed * 0.3); // Bleeding lightens color
    baseColor *= (1.0 + pigment * 0.4 - uPigmentDensity * 0.2); // Pigment variation

    // Apply lighting with soft falloff
    float softLight = pow(NdotL, 0.7);
    vec3 diffuse = baseColor * uLightColor * softLight;
    vec3 ambient = baseColor * uAmbientColor * 0.6;

    vec3 color = diffuse + ambient;

    // Apply paper texture
    color += vec3(paper) * 0.1;

    // Apply edge darkening
    color *= (1.0 - edgeDark);

    // Desaturate slightly for watercolor look
    float luma = dot(color, vec3(0.299, 0.587, 0.114));
    color = mix(vec3(luma), color, 0.9);

    // Soften overall
    color = pow(color, vec3(0.95));

    FragColor = vec4(color, uBaseColor.a);
}
