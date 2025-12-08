// Toon/Cel Shader - Bestow Shader Library
// Cartoon-style lighting with discrete bands
// Works with both OpenGL (uniforms) and Vulkan (push constants)
#version 450

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTexCoord;

layout(location = 0) out vec4 FragColor;

#ifdef VULKAN
layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 viewProjection;
    vec4 baseColor;
    vec4 lightDir;
    vec4 lightColor;
    vec4 ambientColor;
    vec4 cameraPos;
} pc;
#define uBaseColor pc.baseColor
#define uLightDir pc.lightDir.xyz
#define uLightColor pc.lightColor.rgb
#define uAmbientColor pc.ambientColor.rgb
#define uCameraPos pc.cameraPos.xyz
#else
uniform vec4 uBaseColor;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbientColor;
uniform vec3 uCameraPos;
#endif

// Toon shader parameters (defaults can be overridden via uniforms in OpenGL)
#ifdef VULKAN
const int uBands = 3;
const float uSpecularSize = 0.9;
const vec3 uShadowTint = vec3(0.4, 0.5, 0.7);
#else
uniform int uBands;
uniform float uSpecularSize;
uniform vec3 uShadowTint;
#endif

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uLightDir);
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 H = normalize(L + V);

    // Calculate diffuse with bands
    float NdotL = max(dot(N, L), 0.0);

    // Ensure some minimum lighting even on back-facing surfaces
    NdotL = NdotL * 0.7 + 0.3;  // Remap to 0.3-1.0 range (brighter shadows)

    // Smooth cel-shading with soft band transitions
    int bands = max(uBands, 2);
    float bandSize = 1.0 / float(bands);
    float bandPos = NdotL / bandSize;
    float bandIndex = floor(bandPos);
    float bandFrac = fract(bandPos);

    // Smooth transition between bands (prevents harsh rectangles on curved surfaces)
    float smoothFrac = smoothstep(0.0, 0.15, bandFrac) * (1.0 - smoothstep(0.85, 1.0, bandFrac));
    float quantized = (bandIndex + 0.5 + smoothFrac * 0.3) * bandSize;

    // Calculate specular highlight (subtle, not too harsh)
    float NdotH = max(dot(N, H), 0.0);
    float specular = smoothstep(uSpecularSize, uSpecularSize + 0.05, NdotH) * 0.3;

    // Subtle rim lighting (reduced intensity)
    float fresnel = 1.0 - max(dot(N, V), 0.0);
    float rim = pow(fresnel, 5.0) * 0.1;

    // Base color
    vec3 baseColor = uBaseColor.rgb;

    // Shadow tinting - shadows get tinted with cool color for depth
    vec3 shadowColor = mix(baseColor * 0.4, uShadowTint * baseColor * 0.6, 0.4);
    vec3 litColor = mix(shadowColor, baseColor, quantized);

    // Apply lighting
    vec3 diffuse = litColor * uLightColor;
    vec3 ambient = uAmbientColor * baseColor * 0.4;
    vec3 spec = uLightColor * specular * baseColor;  // Tint specular with base color
    vec3 rimColor = uLightColor * rim * baseColor;

    vec3 color = ambient + diffuse + spec + rimColor;

    // Slight saturation boost for cartoon look
    float gray = dot(color, vec3(0.299, 0.587, 0.114));
    color = mix(vec3(gray), color, 1.15);

    // Clamp to prevent over-brightness
    color = min(color, vec3(1.2));

    FragColor = vec4(color, uBaseColor.a);
}
