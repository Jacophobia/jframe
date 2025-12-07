// Terrain Shader - Multi-texture terrain blending
#version 410 core

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbientColor;
uniform vec3 uCameraPos;

// Terrain parameters
uniform vec3 uGrassColor;        // Grass color (default: green)
uniform vec3 uDirtColor;         // Dirt color (default: brown)
uniform vec3 uRockColor;         // Rock color (default: gray)
uniform vec3 uSandColor;         // Sand color (default: tan)
uniform float uSlopeThreshold;   // Slope for rock (default: 0.7)
uniform float uHeightScale;      // Height scale (default: 10.0)

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
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 H = normalize(L + V);

    // Determine terrain type based on slope and height
    float slope = 1.0 - N.y; // How steep the surface is
    float height = vWorldPos.y / uHeightScale;

    // Texture variation
    vec2 uv = vWorldPos.xz * 0.5;
    float variation = noise(uv * 5.0);

    // Blend terrain colors
    vec3 baseColor;

    // Rock on steep slopes
    float rockMix = smoothstep(uSlopeThreshold - 0.1, uSlopeThreshold + 0.1, slope);

    // Height-based blending
    if (height < 0.2) {
        // Sand at low areas
        baseColor = mix(uSandColor, uGrassColor, height * 5.0);
    } else if (height < 0.8) {
        // Grass in mid areas
        baseColor = uGrassColor;
        // Mix in some dirt based on noise
        baseColor = mix(baseColor, uDirtColor, variation * 0.3);
    } else {
        // Rock/snow at peaks
        baseColor = mix(uRockColor, vec3(0.9, 0.9, 0.95), (height - 0.8) * 5.0);
    }

    // Apply slope-based rock
    baseColor = mix(baseColor, uRockColor, rockMix);

    // Lighting
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = baseColor * uLightColor * NdotL;
    vec3 ambient = baseColor * uAmbientColor * 0.5;

    // Subtle specular on wet areas
    float NdotH = max(dot(N, H), 0.0);
    float spec = pow(NdotH, 32.0) * 0.1 * (1.0 - rockMix);
    vec3 specular = uLightColor * spec;

    vec3 color = ambient + diffuse + specular;

    FragColor = vec4(color, 1.0);
}
