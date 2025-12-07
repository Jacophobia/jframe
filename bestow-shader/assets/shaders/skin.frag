// Skin Shader - Subsurface scattering for skin
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

// Skin parameters
uniform vec3 uSubsurfaceColor;   // Subsurface color (default: reddish)
uniform float uScatterWidth;     // Scatter width (default: 0.5)
uniform float uOiliness;         // Skin oiliness/shine (default: 0.3)
uniform float uPoreSize;         // Pore size (default: 0.01)
uniform vec3 uSpecularColor;     // Specular tint (default: white)

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

    // Skin pores
    vec2 uv = vTexCoord / uPoreSize;
    float pores = noise(uv * 50.0) * 0.5 + noise(uv * 100.0) * 0.3;
    pores = pow(pores, 2.0) * 0.1;

    // Perturb normal slightly
    vec3 skinNormal = normalize(N + vec3(
        (noise(uv * 30.0) - 0.5) * 0.05,
        (noise(uv * 30.0 + 100.0) - 0.5) * 0.05,
        0.0
    ));

    // Diffuse with wrap lighting (subsurface approximation)
    float NdotL = dot(skinNormal, L);
    float wrappedDiffuse = max((NdotL + uScatterWidth) / (1.0 + uScatterWidth), 0.0);

    vec3 baseColor = uBaseColor.rgb;
    baseColor *= (1.0 - pores); // Darken pores slightly

    vec3 diffuse = baseColor * uLightColor * wrappedDiffuse;

    // Subsurface scattering (back-lit translucency)
    float backLight = max(dot(-skinNormal, L), 0.0);
    vec3 subsurface = uSubsurfaceColor * uLightColor * pow(backLight, 2.0) * 0.4;

    // Dual-lobe specular (skin has both oily and dry regions)
    float NdotH = max(dot(skinNormal, H), 0.0);
    float specPrimary = pow(NdotH, 32.0) * 0.2; // Dry skin
    float specSecondary = pow(NdotH, 256.0) * uOiliness; // Oily shine
    vec3 specular = uSpecularColor * uLightColor * (specPrimary + specSecondary);

    // Fresnel for rim lighting
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 3.0);
    vec3 rim = uSubsurfaceColor * fresnel * 0.2;

    // Ambient
    vec3 ambient = uAmbientColor * baseColor * 0.4;

    // Combine
    vec3 color = ambient + diffuse + subsurface + specular + rim;

    FragColor = vec4(color, uBaseColor.a);
}
