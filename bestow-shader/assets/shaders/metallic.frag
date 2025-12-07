// Metallic Shader - Enhanced metallic surfaces
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

// Metallic parameters
uniform float uRoughness;        // Surface roughness (default: 0.2)
uniform float uAnisotropy;       // Anisotropic reflection (default: 0.0)
uniform vec3 uTintColor;         // Metallic tint (default: white)
uniform float uClearcoat;        // Clearcoat layer (default: 0.0)

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 L = normalize(-uLightDir);
    vec3 H = normalize(V + L);

    // Metallic tinted base color
    vec3 albedo = uBaseColor.rgb * uTintColor;

    // Metals have high F0 (base reflectivity)
    vec3 F0 = albedo;

    // Fresnel
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    // Specular
    float NDF = DistributionGGX(N, H, uRoughness);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    vec3 specular = F * NDF * uLightColor * NdotL / (4.0 * NdotV * NdotL + 0.001);

    // Metals don't have diffuse, only specular reflection
    vec3 color = specular;

    // Ambient reflection (environment)
    vec3 ambient = F * uAmbientColor * albedo * 0.5;
    color += ambient;

    // Optional clearcoat layer
    if (uClearcoat > 0.0) {
        float clearcoatNDF = DistributionGGX(N, H, 0.1);
        vec3 clearcoatF = fresnelSchlick(max(dot(H, V), 0.0), vec3(0.04));
        vec3 clearcoat = clearcoatF * clearcoatNDF * uLightColor * NdotL * uClearcoat;
        color += clearcoat;
    }

    // Boost brightness for metallic look
    color *= 1.2;

    FragColor = vec4(color, uBaseColor.a);
}
