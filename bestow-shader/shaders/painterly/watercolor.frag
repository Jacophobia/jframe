#version 410 core

// Watercolor Painting Fragment Shader
// Soft, blended colors with paper texture and pigment accumulation
//
// Configurable Parameters:
// - uPaperScale: Scale of paper texture (default: 100.0)
// - uPigmentDensity: How much pigment accumulates (default: 1.2)
// - uWaterBleed: Amount of color bleeding (default: 0.3)

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform vec4 uBaseColor;
uniform vec3 uCameraPos;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform float uTime;

uniform float uPaperScale;
uniform float uPigmentDensity;
uniform float uWaterBleed;
uniform bool uUseTexture;
uniform sampler2D uTexture;

// Simplex noise function (simplified)
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

// Paper texture
float paperTexture(vec2 uv) {
    float n = noise(uv * 2.0) * 0.5;
    n += noise(uv * 4.0) * 0.25;
    n += noise(uv * 8.0) * 0.125;
    return n;
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uLightDir);

    // Soft lighting
    float NdotL = max(dot(N, L), 0.0);
    float diffuse = pow(NdotL, 0.7); // Softer than linear

    // Base color
    vec4 albedo = uBaseColor * vColor;
    if (uUseTexture) {
        albedo *= texture(uTexture, vTexCoord);
    }

    // Paper texture
    float paper = paperTexture(vTexCoord * uPaperScale);
    paper = mix(0.9, 1.1, paper);

    // Pigment accumulation in shadows (darker = more pigment)
    float pigment = mix(1.0, uPigmentDensity, 1.0 - diffuse);

    // Color bleeding effect
    vec2 bleedUV = vTexCoord + vec2(
        noise(vTexCoord * 10.0 + uTime * 0.1),
        noise(vTexCoord * 10.0 + uTime * 0.1 + 100.0)
    ) * uWaterBleed * 0.01;

    vec3 bleedColor = albedo.rgb;
    if (uUseTexture) {
        bleedColor = (texture(uTexture, bleedUV) * uBaseColor).rgb;
    }

    // Mix bled color slightly
    vec3 finalColor = mix(albedo.rgb, bleedColor, uWaterBleed * 0.5);

    // Apply lighting and paper texture
    finalColor = finalColor * diffuse * pigment * paper;

    // Add soft ambient
    finalColor += albedo.rgb * 0.3;

    FragColor = vec4(finalColor, albedo.a);
}
