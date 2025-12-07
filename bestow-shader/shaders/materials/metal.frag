#version 410 core

// Stylized Metal Material Fragment Shader
// Simplified metallic look without full PBR
//
// Configurable Parameters:
// - uMetalColor: Base metal tint (default: silver)
// - uRoughness: Surface roughness (default: 0.3)
// - uAnisotropy: Brushed metal effect (default: 0.0)

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform vec3 uCameraPos;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec4 uMetalColor;
uniform float uRoughness;
uniform float uAnisotropy;

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 L = normalize(-uLightDir);
    vec3 H = normalize(L + V);

    // Anisotropic brushed metal (simplified)
    vec3 T = normalize(vec3(1, 0, 0)); // Tangent direction
    float aniso = mix(1.0, abs(dot(H, T)), uAnisotropy);

    // Specular
    float roughness = max(uRoughness, 0.04);
    float specPower = 2.0 / (roughness * roughness) - 2.0;
    float spec = pow(max(dot(N, H), 0.0), specPower * aniso);

    // Fresnel (metals have high reflectance)
    vec3 F0 = uMetalColor.rgb;
    float cosTheta = max(dot(H, V), 0.0);
    vec3 fresnel = F0 + (vec3(1.0) - F0) * pow(1.0 - cosTheta, 5.0);

    // Metals have no diffuse, only specular
    vec3 specular = fresnel * spec;

    // Minimal ambient (metals reflect environment)
    vec3 ambient = uMetalColor.rgb * 0.1;

    // Environment reflection approximation
    vec3 R = reflect(-V, N);
    vec3 envColor = mix(vec3(0.3), uLightColor, max(dot(R, L), 0.0));

    vec3 color = ambient + specular * uLightColor + envColor * fresnel * 0.5;

    FragColor = vec4(color, 1.0);
}
