#version 410 core

// Skin Material Fragment Shader
// Simplified subsurface scattering for skin
//
// Configurable Parameters:
// - uSkinColor: Base skin tone (default: peach)
// - uSubsurfaceColor: Subsurface color (default: red)
// - uScatterAmount: Amount of subsurface scattering (default: 0.5)

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform vec3 uCameraPos;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec4 uSkinColor;
uniform vec3 uSubsurfaceColor;
uniform float uScatterAmount;

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 L = normalize(-uLightDir);
    vec3 H = normalize(L + V);

    // Diffuse
    float NdotL = max(dot(N, L), 0.0);

    // Subsurface scattering approximation (wrap lighting)
    float wrap = 0.5;
    float scatter = max(0.0, (dot(N, L) + wrap) / (1.0 + wrap));
    scatter = pow(scatter, 2.0);

    // Backscattering
    float backScatter = max(0.0, dot(-N, L));
    backScatter = pow(backScatter, 3.0);

    // Combine subsurface
    vec3 subsurface = uSubsurfaceColor * (scatter + backScatter * 0.5) * uScatterAmount;

    // Regular diffuse
    vec3 diffuse = uSkinColor.rgb * NdotL;

    // Soft specular
    float spec = pow(max(dot(N, H), 0.0), 32.0) * 0.3;

    // Combine
    vec3 color = diffuse + subsurface + spec * uLightColor;
    color += uSkinColor.rgb * 0.2; // Ambient

    FragColor = vec4(color, uSkinColor.a);
}
