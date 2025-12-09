#version 450

// Stained Glass Shader - Voronoi cells with colored light transmission
// Creates beautiful mosaic patterns with light passing through

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 viewProjection;
    vec4 baseColor;
    vec4 lightDir;
    vec4 lightColor;
    vec4 ambientColor;
    vec4 cameraPos;
} pc;

// Stained glass parameters
const float uCellScale = 8.0;
const float uLeadWidth = 0.08;
const vec3 uLeadColor = vec3(0.1, 0.1, 0.12);

// Hash functions for randomness
vec2 hash2(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
    return fract(sin(p) * 43758.5453);
}

vec3 hash3(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * vec3(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yxz + 33.33);
    return fract((p3.xxy + p3.yxx) * p3.zyx);
}

// Voronoi distance
vec2 voronoi(vec2 x) {
    vec2 n = floor(x);
    vec2 f = fract(x);

    float minDist = 8.0;
    vec2 minPoint = vec2(0.0);
    vec2 cellId = vec2(0.0);

    for (int j = -1; j <= 1; j++) {
        for (int i = -1; i <= 1; i++) {
            vec2 g = vec2(float(i), float(j));
            vec2 o = hash2(n + g);
            vec2 r = g + o - f;
            float d = dot(r, r);

            if (d < minDist) {
                minDist = d;
                minPoint = r;
                cellId = n + g;
            }
        }
    }

    // Also calculate distance to edge
    float edgeDist = 8.0;
    for (int j = -2; j <= 2; j++) {
        for (int i = -2; i <= 2; i++) {
            vec2 g = vec2(float(i), float(j));
            vec2 o = hash2(n + g);
            vec2 r = g + o - f;

            if (dot(minPoint - r, minPoint - r) > 0.00001) {
                edgeDist = min(edgeDist, dot(0.5 * (minPoint + r), normalize(r - minPoint)));
            }
        }
    }

    return vec2(sqrt(minDist), edgeDist);
}

void main() {
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(-pc.lightDir.xyz);
    vec3 V = normalize(pc.cameraPos.xyz - fragWorldPos);

    // Calculate lighting
    float NdotL = dot(N, L);
    float frontLight = max(NdotL, 0.0);
    float backLight = max(-NdotL, 0.0) * 0.8;  // Light coming through
    float diffuse = frontLight + backLight;    // Two-sided lighting

    // Create voronoi pattern in world space
    vec2 uv = fragWorldPos.xy * uCellScale;
    vec2 vor = voronoi(uv);

    // Get cell ID for color
    vec2 cellUV = floor(uv + 0.5);
    vec3 cellColor = hash3(cellUV);

    // Make colors more saturated and jewel-like
    cellColor = pow(cellColor, vec3(0.6));  // Boost saturation
    cellColor = mix(cellColor, pc.baseColor.rgb, 0.3);  // Tint with base color

    // Create lead lines (dark borders between cells)
    float lead = smoothstep(uLeadWidth, uLeadWidth + 0.02, vor.y);

    // Glass color with light transmission
    vec3 glassColor = cellColor * pc.lightColor.rgb;

    // Brighter when light passes through (backlit)
    glassColor *= (diffuse * 0.6 + 0.4);
    glassColor += backLight * cellColor * 0.5;  // Extra glow when backlit

    // Add subtle fresnel for glass-like reflection
    float fresnel = pow(1.0 - abs(dot(N, V)), 3.0);
    glassColor += fresnel * pc.lightColor.rgb * 0.2;

    // Mix glass and lead
    vec3 color = mix(uLeadColor, glassColor, lead);

    // Add slight cell center brightness variation
    float centerGlow = 1.0 - vor.x * 0.3;
    color *= centerGlow;

    outColor = vec4(color, pc.baseColor.a);
}
