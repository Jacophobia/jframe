// Leather Shader - Leather material with pores and wrinkles
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

// Leather parameters
uniform float uRoughness;        // Surface roughness (default: 0.6)
uniform float uPoreSize;         // Pore size (default: 0.02)
uniform float uWrinkleAmount;    // Wrinkle intensity (default: 0.3)
uniform float uGlossiness;       // Surface gloss (default: 0.2)

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
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 L = normalize(-uLightDir);
    vec3 H = normalize(V + L);

    // Leather pores
    vec2 uv = vTexCoord / uPoreSize;
    float pores = noise(uv * 30.0) * 0.5 + noise(uv * 60.0) * 0.3;
    pores = pow(pores, 2.0);

    // Wrinkles
    float wrinkles = noise(uv * 5.0) * 0.5 + noise(uv * 10.0) * 0.5;
    wrinkles = pow(abs(wrinkles - 0.5) * 2.0, 2.0) * uWrinkleAmount;

    // Perturb normal
    vec3 leatherNormal = normalize(N + vec3(
        (noise(uv * 20.0) - 0.5) * 0.15,
        (noise(uv * 20.0 + 100.0) - 0.5) * 0.15,
        (pores + wrinkles) * 0.1
    ));

    // Lighting
    float NdotL = max(dot(leatherNormal, L), 0.0);
    vec3 baseColor = uBaseColor.rgb;

    // Darken pores and wrinkles
    baseColor *= (1.0 - pores * 0.3 - wrinkles * 0.2);

    vec3 diffuse = baseColor * uLightColor * NdotL;
    vec3 ambient = baseColor * uAmbientColor * 0.4;

    // Glossy specular
    float NdotH = max(dot(leatherNormal, H), 0.0);
    float spec = pow(NdotH, (1.0 - uRoughness) * 64.0) * uGlossiness;
    vec3 specular = uLightColor * spec;

    vec3 color = ambient + diffuse + specular;

    FragColor = vec4(color, uBaseColor.a);
}
