// Metallic Shader - shiny metallic surface with strong specular
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
uniform float uRoughness;      // 0.0 = mirror, 1.0 = rough
uniform float uMetallic;       // 1.0 = fully metallic
uniform float uReflectivity;   // Reflection strength

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uLightDir);
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 H = normalize(L + V);
    vec3 R = reflect(-V, N);

    // Base diffuse lighting
    float NdotL = max(dot(N, L), 0.0);

    // Specular - Blinn-Phong with variable roughness
    float NdotH = max(dot(N, H), 0.0);
    float shininess = mix(256.0, 8.0, uRoughness);  // Higher = sharper specular
    float specular = pow(NdotH, shininess);

    // Fresnel effect - edges are more reflective
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 5.0);
    fresnel = mix(0.04, 1.0, fresnel);  // F0 for dielectrics is ~0.04

    // Metallic surfaces tint specular with base color
    vec3 baseColor = uBaseColor.rgb;
    vec3 specColor = mix(vec3(1.0), baseColor, uMetallic);

    // Fake environment reflection (use reflected light direction)
    vec3 envColor = mix(uAmbientColor, uLightColor, max(dot(R, L), 0.0) * 0.5 + 0.5);
    envColor = mix(vec3(0.3, 0.4, 0.5), envColor, 0.7);  // Blend with sky-ish color

    // Combine components
    // Diffuse is reduced for metallic surfaces
    vec3 diffuse = baseColor * uLightColor * NdotL * (1.0 - uMetallic * 0.8);

    // Ambient
    vec3 ambient = baseColor * uAmbientColor * 0.3;

    // Specular highlight
    vec3 spec = specColor * uLightColor * specular * uReflectivity;

    // Environment reflection (stronger for metallic)
    vec3 reflection = envColor * fresnel * uReflectivity * uMetallic;

    // Rim lighting for metallic edge glow
    float rim = pow(1.0 - max(dot(N, V), 0.0), 3.0) * 0.2 * uMetallic;
    vec3 rimColor = specColor * uLightColor * rim;

    vec3 color = ambient + diffuse + spec + reflection + rimColor;

    // Slight saturation boost
    float gray = dot(color, vec3(0.299, 0.587, 0.114));
    color = mix(vec3(gray), color, 1.1);

    // Clamp
    color = min(color, vec3(1.5));

    FragColor = vec4(color, uBaseColor.a);
}
