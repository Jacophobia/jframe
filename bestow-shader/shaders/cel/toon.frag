#version 410 core

// Classic Cel-Shading Fragment Shader
// Creates discrete light bands for a cartoon/anime look
//
// Configurable Parameters:
// - uBands: Number of discrete light levels (default: 3)
// - uRimPower: Strength of rim lighting effect (default: 3.0)
// - uRimColor: Color of rim light (default: white)

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform vec4 uBaseColor;
uniform vec3 uCameraPos;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbientColor;

// Cel-shading parameters
uniform int uBands;           // Number of discrete light bands (2-8)
uniform float uRimPower;      // Rim light intensity
uniform vec3 uRimColor;       // Rim light color
uniform bool uUseTexture;
uniform sampler2D uTexture;

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uLightDir);
    vec3 V = normalize(uCameraPos - vWorldPos);

    // Calculate lighting
    float NdotL = dot(N, L);

    // Quantize to discrete bands
    int bands = max(uBands, 2);
    float intensity = ceil(NdotL * float(bands)) / float(bands);
    intensity = clamp(intensity, 0.0, 1.0);

    // Base color
    vec4 albedo = uBaseColor * vColor;
    if (uUseTexture) {
        albedo *= texture(uTexture, vTexCoord);
    }

    // Diffuse lighting with banding
    vec3 diffuse = albedo.rgb * uLightColor * intensity;

    // Rim lighting (Fresnel effect)
    float rim = 1.0 - max(dot(V, N), 0.0);
    rim = pow(rim, uRimPower);
    vec3 rimLight = uRimColor * rim;

    // Ambient
    vec3 ambient = uAmbientColor * albedo.rgb;

    // Combine
    vec3 color = ambient + diffuse + rimLight;

    FragColor = vec4(color, albedo.a);
}
