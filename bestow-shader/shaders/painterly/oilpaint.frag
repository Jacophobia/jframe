#version 410 core

// Oil Painting Fragment Shader
// Thick brush strokes with visible texture and impasto effect
//
// Configurable Parameters:
// - uBrushScale: Size of brush strokes (default: 20.0)
// - uImpasto: Height/thickness of paint (default: 0.5)
// - uColorVariation: Random color variation (default: 0.1)

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform vec4 uBaseColor;
uniform vec3 uCameraPos;
uniform vec3 uLightDir;
uniform vec3 uLightColor;

uniform float uBrushScale;
uniform float uImpasto;
uniform float uColorVariation;
uniform bool uUseTexture;
uniform sampler2D uTexture;

// Hash function for randomness
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

// Voronoi-based brush strokes
vec2 voronoi(vec2 x) {
    vec2 p = floor(x);
    vec2 f = fract(x);

    float minDist = 1.0;
    vec2 minPoint = vec2(0.0);

    for (int j = -1; j <= 1; j++) {
        for (int i = -1; i <= 1; i++) {
            vec2 neighbor = vec2(float(i), float(j));
            vec2 point = vec2(
                hash(p + neighbor),
                hash(p + neighbor + vec2(43.0, 17.0))
            );

            vec2 diff = neighbor + point - f;
            float dist = length(diff);

            if (dist < minDist) {
                minDist = dist;
                minPoint = p + neighbor;
            }
        }
    }

    return minPoint;
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uLightDir);
    vec3 V = normalize(uCameraPos - vWorldPos);

    // Calculate lighting
    float NdotL = max(dot(N, L), 0.0);

    // Base color
    vec4 albedo = uBaseColor * vColor;
    if (uUseTexture) {
        albedo *= texture(uTexture, vTexCoord);
    }

    // Brush stroke pattern
    vec2 brushUV = vTexCoord * uBrushScale;
    vec2 cell = voronoi(brushUV);

    // Random color variation per brush stroke
    float variation = hash(cell) * 2.0 - 1.0;
    vec3 variedColor = albedo.rgb + vec3(variation) * uColorVariation;
    variedColor = clamp(variedColor, 0.0, 1.0);

    // Brush stroke direction (simplified)
    float angle = hash(cell + vec2(123.0)) * 6.28318;
    vec2 brushDir = vec2(cos(angle), sin(angle));

    // Impasto effect (paint thickness creates highlights)
    float impastoNoise = hash(cell + vec2(456.0));
    vec3 impastoNormal = normalize(N + vec3(brushDir * uImpasto, 0.0) * impastoNoise);
    float impastoLight = max(dot(impastoNormal, L), 0.0);

    // Mix regular and impasto lighting
    float lighting = mix(NdotL, impastoLight, uImpasto);

    // Apply lighting
    vec3 color = variedColor * uLightColor * lighting;

    // Add ambient
    color += variedColor * 0.2;

    // Add specular from paint texture
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(impastoNormal, H), 0.0), 20.0) * uImpasto;
    color += spec * 0.3;

    FragColor = vec4(color, albedo.a);
}
