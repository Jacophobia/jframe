#version 450

// Halftone Shader - Comic book style with Ben-Day dots
// Creates distinct dot patterns based on lighting intensity

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
    vec4 cameraPos;
} pc;

// Halftone parameters
const float uDotScale = 80.0;      // Density of dots
const float uDotSmooth = 0.05;     // Edge smoothness
const vec3 uInkColor = vec3(0.1, 0.05, 0.15);  // Dark ink color
const vec3 uPaperColor = vec3(0.98, 0.95, 0.9); // Off-white paper

void main() {
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(-pc.lightDir.xyz);

    // Calculate lighting intensity
    float NdotL = dot(N, L) * 0.5 + 0.5;  // Remap to 0-1

    // Create screen-space UV for dots (using world pos projected)
    vec2 screenUV = fragWorldPos.xy * uDotScale;

    // Rotate the dot grid 45 degrees for classic halftone look
    float angle = 0.785398; // 45 degrees
    vec2 rotatedUV = vec2(
        screenUV.x * cos(angle) - screenUV.y * sin(angle),
        screenUV.x * sin(angle) + screenUV.y * cos(angle)
    );

    // Create dot pattern - distance from nearest grid center
    vec2 gridPos = fract(rotatedUV) - 0.5;
    float dist = length(gridPos);

    // Dot size varies with light intensity
    // Darker areas = bigger dots, lighter areas = smaller/no dots
    float dotRadius = (1.0 - NdotL) * 0.45;

    // Smooth dot edge
    float dot = 1.0 - smoothstep(dotRadius - uDotSmooth, dotRadius + uDotSmooth, dist);

    // Mix between paper and ink based on dot pattern
    vec3 halftoneColor = mix(uPaperColor, uInkColor, dot);

    // Tint with base color
    vec3 tintedColor = halftoneColor * pc.baseColor.rgb;

    // Add slight color variation in the dots for CMYK feel
    vec3 colorTint = pc.baseColor.rgb;
    tintedColor = mix(tintedColor, colorTint * halftoneColor, 0.5);

    outColor = vec4(tintedColor, pc.baseColor.a);
}
