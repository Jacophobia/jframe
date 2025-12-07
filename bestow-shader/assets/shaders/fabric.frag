// Fabric Shader - Cloth with subsurface scattering
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

// Fabric parameters
uniform float uRoughness;        // Surface roughness (default: 0.8)
uniform vec3 uSheenColor;        // Sheen color (default: white)
uniform float uSheenAmount;      // Sheen intensity (default: 0.3)
uniform float uFuzziness;        // Fabric fuzz (default: 0.5)
uniform vec3 uSubsurfaceColor;   // Subsurface color (default: base color)

// Noise for fabric weave
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

    // Fabric weave pattern
    vec2 uv = vTexCoord * 50.0;
    float weave = noise(uv) * 0.1 + noise(uv * 2.0) * 0.05;

    // Perturb normal for fabric texture
    vec3 fabricNormal = normalize(N + vec3(
        (noise(uv) - 0.5) * 0.1,
        (noise(uv + 50.0) - 0.5) * 0.1,
        0.0
    ));

    // Diffuse lighting with wrap
    float NdotL = dot(fabricNormal, L);
    float wrap = 0.5; // Subsurface scattering wrap
    float wrappedDiffuse = max((NdotL + wrap) / (1.0 + wrap), 0.0);

    vec3 baseColor = uBaseColor.rgb;
    vec3 diffuse = baseColor * uLightColor * wrappedDiffuse;

    // Subsurface scattering (light bleeding through fabric)
    float backLight = max(dot(-fabricNormal, L), 0.0);
    vec3 subsurface = uSubsurfaceColor * uLightColor * backLight * 0.3;

    // Sheen (fabric has characteristic sheen at grazing angles)
    float sheenDot = pow(1.0 - max(dot(fabricNormal, V), 0.0), 5.0);
    vec3 sheen = uSheenColor * sheenDot * uSheenAmount;

    // Fuzz (velvet-like edge glow)
    float fuzz = pow(1.0 - max(dot(N, V), 0.0), 3.0) * uFuzziness;
    vec3 fuzzColor = baseColor * fuzz * 0.5;

    // Ambient
    vec3 ambient = uAmbientColor * baseColor * 0.5;

    // Combine
    vec3 color = ambient + diffuse + subsurface + sheen + fuzzColor;

    // Add weave variation
    color *= (1.0 + weave - 0.05);

    FragColor = vec4(color, uBaseColor.a);
}
