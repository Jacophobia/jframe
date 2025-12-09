#version 450

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
    vec4 cameraPos;  // Added for specular/rim lighting
} pc;

// Toon shader parameters (matching OpenGL defaults)
const int uBands = 3;
const float uSpecularSize = 0.9;
const vec3 uShadowTint = vec3(0.4, 0.5, 0.7);

void main() {
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(-pc.lightDir.xyz);
    vec3 V = normalize(pc.cameraPos.xyz - fragWorldPos);
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
    vec3 baseColor = pc.baseColor.rgb;

    // Shadow tinting - shadows get tinted with cool color for depth
    vec3 shadowColor = mix(baseColor * 0.4, uShadowTint * baseColor * 0.6, 0.4);
    vec3 litColor = mix(shadowColor, baseColor, quantized);

    // Apply lighting
    vec3 diffuse = litColor * pc.lightColor.rgb;
    vec3 ambient = pc.ambientColor.rgb * baseColor * 0.4;
    vec3 spec = pc.lightColor.rgb * specular * baseColor;  // Tint specular with base color
    vec3 rimColor = pc.lightColor.rgb * rim * baseColor;

    vec3 color = ambient + diffuse + spec + rimColor;
    color = mix(color, vec3(1.0, 0.3, 0.3), 0.3);  // HOT RELOAD TEST - add red tint

    // Slight saturation boost for cartoon look
    float gray = dot(color, vec3(0.299, 0.587, 0.114));
    color = mix(vec3(gray), color, 1.15);

    // Clamp to prevent over-brightness
    color = min(color, vec3(1.2));

    // Note: sRGB swapchain handles gamma correction automatically
    outColor = vec4(color, pc.baseColor.a);
}
