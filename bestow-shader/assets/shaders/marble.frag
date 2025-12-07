// Marble Shader - Procedural marble/stone
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

// Marble parameters
uniform vec3 uVeinColor;         // Vein color (default: dark gray)
uniform vec3 uBaseStoneColor;    // Base stone color (default: white)
uniform float uVeinFrequency;    // Vein pattern frequency (default: 3.0)
uniform float uVeinComplexity;   // Vein complexity (default: 0.5)
uniform float uGlossiness;       // Surface glossiness (default: 0.7)

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
    for (int i = 0; i < 5; i++) {
        value += amplitude * noise(p);
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 L = normalize(-uLightDir);
    vec3 H = normalize(V + L);

    // Marble veining pattern
    vec2 pos = vTexCoord * uVeinFrequency;

    // Create flowing vein pattern
    float vein = fbm(pos + vec2(0.0, fbm(pos) * uVeinComplexity));

    // Sharpen veins
    vein = abs(vein - 0.5) * 2.0;
    vein = pow(vein, 1.5);

    // Mix vein and base colors
    vec3 marbleColor = mix(uVeinColor, uBaseStoneColor, vein);
    marbleColor *= uBaseColor.rgb;

    // Add subtle color variation
    float variation = noise(pos * 5.0) * 0.1;
    marbleColor += vec3(variation);

    // Lighting
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = marbleColor * uLightColor * NdotL;
    vec3 ambient = marbleColor * uAmbientColor * 0.4;

    // Polished specular
    float NdotH = max(dot(N, H), 0.0);
    float spec = pow(NdotH, 128.0) * uGlossiness;
    vec3 specular = uLightColor * spec;

    vec3 color = ambient + diffuse + specular;

    FragColor = vec4(color, uBaseColor.a);
}
