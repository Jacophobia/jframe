// Wood Shader - Procedural wood grain
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

// Wood parameters
uniform vec3 uDarkWoodColor;     // Dark grain color (default: dark brown)
uniform vec3 uLightWoodColor;    // Light grain color (default: light brown)
uniform float uGrainFrequency;   // Grain ring frequency (default: 5.0)
uniform float uGrainVariation;   // Grain irregularity (default: 0.3)
uniform float uRoughness;        // Surface roughness (default: 0.5)

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

    // Wood grain pattern (circular rings)
    vec2 pos = vTexCoord * uGrainFrequency;

    // Add noise to make grain irregular
    float noiseVal = noise(pos * 2.0) * uGrainVariation;
    pos += vec2(noiseVal);

    // Circular grain pattern
    float dist = length(pos - floor(pos) - 0.5);
    float grain = fract(dist * 10.0);

    // Add detail to grain
    grain += noise(pos * 20.0) * 0.2;
    grain = smoothstep(0.3, 0.7, grain);

    // Mix wood colors
    vec3 woodColor = mix(uDarkWoodColor, uLightWoodColor, grain);
    woodColor *= uBaseColor.rgb;

    // Lighting
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = woodColor * uLightColor * NdotL;
    vec3 ambient = woodColor * uAmbientColor * 0.5;

    // Subtle specular
    float NdotH = max(dot(N, H), 0.0);
    float spec = pow(NdotH, (1.0 - uRoughness) * 32.0) * 0.2;
    vec3 specular = uLightColor * spec;

    vec3 color = ambient + diffuse + specular;

    FragColor = vec4(color, uBaseColor.a);
}
