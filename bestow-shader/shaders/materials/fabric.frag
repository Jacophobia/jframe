#version 410 core

// Fabric/Cloth Material Fragment Shader
// Soft diffuse with velvet-like sheen
//
// Configurable Parameters:
// - uFabricColor: Base fabric color (default: white)
// - uRoughness: Surface roughness (default: 0.9)
// - uSheenColor: Edge sheen color (default: same as fabric)
// - uSheenIntensity: Strength of sheen (default: 0.3)

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform vec3 uCameraPos;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec4 uFabricColor;
uniform float uRoughness;
uniform vec3 uSheenColor;
uniform float uSheenIntensity;

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 L = normalize(-uLightDir);
    vec3 H = normalize(L + V);

    // Soft diffuse (Oren-Nayar approximation)
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    float roughness2 = uRoughness * uRoughness;
    float A = 1.0 - 0.5 * roughness2 / (roughness2 + 0.33);
    float B = 0.45 * roughness2 / (roughness2 + 0.09);

    float angleVN = acos(NdotV);
    float angleLN = acos(NdotL);
    float alpha = max(angleVN, angleLN);
    float beta = min(angleVN, angleLN);

    float diffuse = NdotL * (A + B * max(0.0, cos(angleVN - angleLN)) * sin(alpha) * tan(beta));

    // Fabric sheen (inverted Fresnel for velvet effect)
    float sheen = pow(1.0 - max(dot(V, H), 0.0), 5.0);
    sheen *= pow(NdotL, 0.5); // Only where lit
    vec3 sheenContrib = uSheenColor * sheen * uSheenIntensity;

    // Combine
    vec3 baseColor = uFabricColor.rgb * vColor.rgb;
    vec3 color = baseColor * diffuse + sheenContrib;
    color += baseColor * 0.2; // Ambient

    FragColor = vec4(color, uFabricColor.a * vColor.a);
}
