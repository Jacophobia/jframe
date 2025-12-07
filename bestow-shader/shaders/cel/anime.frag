#version 410 core

// Anime Style Fragment Shader
// Smooth gradients with bright specular highlights typical of anime
//
// Configurable Parameters:
// - uShadowSoftness: How soft the shadow transition is (default: 0.05)
// - uSpecularPower: Sharpness of specular highlights (default: 100.0)
// - uSpecularStrength: Intensity of specular (default: 0.8)

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

uniform float uShadowSoftness;
uniform float uSpecularPower;
uniform float uSpecularStrength;
uniform bool uUseTexture;
uniform sampler2D uTexture;

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uLightDir);
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 H = normalize(L + V);

    // Calculate lighting with soft transition
    float NdotL = max(dot(N, L), 0.0);

    // Smooth two-tone shading (anime style)
    float shadowEdge = 0.5;
    float diffuse = smoothstep(shadowEdge - uShadowSoftness, shadowEdge + uShadowSoftness, NdotL);

    // Base color
    vec4 albedo = uBaseColor * vColor;
    if (uUseTexture) {
        albedo *= texture(uTexture, vTexCoord);
    }

    // Diffuse shading
    vec3 lightColor = albedo.rgb * uLightColor * mix(0.5, 1.0, diffuse);

    // Anime-style sharp specular highlight
    float NdotH = max(dot(N, H), 0.0);
    float spec = pow(NdotH, uSpecularPower);

    // Quantize specular to create sharp anime highlight
    spec = step(0.5, spec) * uSpecularStrength;
    vec3 specular = vec3(spec) * uLightColor;

    // Rim lighting for depth
    float rim = 1.0 - max(dot(V, N), 0.0);
    rim = smoothstep(0.6, 1.0, rim) * 0.3;
    vec3 rimLight = albedo.rgb * rim;

    // Ambient
    vec3 ambient = uAmbientColor * albedo.rgb;

    // Combine
    vec3 color = ambient + lightColor + specular + rimLight;

    FragColor = vec4(color, albedo.a);
}
