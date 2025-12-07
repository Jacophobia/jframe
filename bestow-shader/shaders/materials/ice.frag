#version 410 core

// Ice Material Fragment Shader
// Frozen surface with internal cracks and frost
//
// Configurable Parameters:
// - uIceColor: Tint of the ice (default: light blue)
// - uCrackDensity: Amount of internal cracks (default: 0.5)
// - uFrostAmount: Surface frost intensity (default: 0.3)

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform vec3 uCameraPos;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec4 uIceColor;
uniform float uCrackDensity;
uniform float uFrostAmount;

// Hash for noise
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float hash3(vec3 p) {
    return fract(sin(dot(p, vec3(127.1, 311.7, 74.7))) * 43758.5453);
}

// Voronoi for cracks
float voronoi(vec2 x) {
    vec2 p = floor(x);
    vec2 f = fract(x);

    float minDist = 1.0;
    for (int j = -1; j <= 1; j++) {
        for (int i = -1; i <= 1; i++) {
            vec2 neighbor = vec2(float(i), float(j));
            vec2 point = vec2(hash(p + neighbor), hash(p + neighbor + vec2(43.0)));
            vec2 diff = neighbor + point - f;
            float dist = length(diff);
            minDist = min(minDist, dist);
        }
    }
    return minDist;
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 L = normalize(-uLightDir);

    // Fresnel for glassy appearance
    float fresnel = pow(1.0 - max(dot(V, N), 0.0), 3.0);
    fresnel = mix(0.1, 1.0, fresnel);

    // Internal cracks using Voronoi
    float cracks = voronoi(vWorldPos.xz * 10.0);
    cracks = smoothstep(0.1, 0.0, cracks) * uCrackDensity;

    // Surface frost using noise
    float frost = hash3(vWorldPos * 50.0);
    frost = pow(frost, 3.0) * uFrostAmount;

    // Base ice color
    vec3 iceColor = uIceColor.rgb;

    // Darken along cracks
    iceColor = mix(iceColor, iceColor * 0.5, cracks);

    // Add frost as white speckles
    iceColor = mix(iceColor, vec3(1.0), frost);

    // Lighting
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = iceColor * (0.3 + NdotL * 0.7);

    // Strong specular for icy shine
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), 256.0);
    vec3 specular = spec * uLightColor;

    // Subsurface scattering approximation
    vec3 subsurface = uIceColor.rgb * pow(max(dot(-N, L), 0.0), 2.0) * 0.5;

    // Combine
    vec3 color = diffuse + specular + subsurface;

    // Ice is translucent
    float alpha = mix(0.6, 0.95, fresnel);

    FragColor = vec4(color, alpha);
}
