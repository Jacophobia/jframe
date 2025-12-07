#version 410 core

// Fresnel Rim Lighting Fragment Shader
// Creates edge lighting/rim light effect
//
// Configurable Parameters:
// - uRimColor: Color of rim light (default: white)
// - uRimPower: Sharpness of rim (default: 3.0)
// - uRimIntensity: Brightness of rim (default: 1.0)

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform vec4 uBaseColor;
uniform vec3 uCameraPos;
uniform vec3 uLightDir;
uniform vec3 uRimColor;
uniform float uRimPower;
uniform float uRimIntensity;
uniform bool uUseTexture;
uniform sampler2D uTexture;

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 L = normalize(-uLightDir);

    // Base color
    vec4 albedo = uBaseColor * vColor;
    if (uUseTexture) {
        albedo *= texture(uTexture, vTexCoord);
    }

    // Simple lighting
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = albedo.rgb * (0.3 + NdotL * 0.7);

    // Fresnel rim light
    float rim = 1.0 - max(dot(V, N), 0.0);
    rim = pow(rim, uRimPower) * uRimIntensity;
    vec3 rimLight = uRimColor * rim;

    // Combine
    vec3 color = diffuse + rimLight;

    FragColor = vec4(color, albedo.a);
}
