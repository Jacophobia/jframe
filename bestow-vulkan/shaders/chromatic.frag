#version 450

// Chromatic Aberration Shader - RGB channel separation
// Creates a trippy, glitchy, VHS-style effect on 3D objects

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

// Chromatic aberration parameters
const float uAberrationStrength = 0.15;
const float uDistortionFreq = 3.0;
const vec3 uTintR = vec3(1.0, 0.2, 0.2);
const vec3 uTintG = vec3(0.2, 1.0, 0.2);
const vec3 uTintB = vec3(0.2, 0.2, 1.0);

void main() {
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(-pc.lightDir.xyz);
    vec3 V = normalize(pc.cameraPos.xyz - fragWorldPos);

    // Calculate base lighting
    float NdotL = max(dot(N, L), 0.0);
    float diffuse = NdotL * 0.7 + 0.3;

    // Fresnel-based aberration (stronger at edges)
    float fresnel = pow(1.0 - abs(dot(N, V)), 2.0);

    // Create offset based on normal (simulates refraction)
    vec3 offset = N * uAberrationStrength * fresnel;

    // Add some wave distortion
    float wave = sin(fragWorldPos.y * uDistortionFreq + fragWorldPos.x * uDistortionFreq);
    offset += vec3(wave * 0.02, 0.0, 0.0);

    // Sample "color channels" at offset positions
    // Since we don't have texture, we use lighting at offset normals
    vec3 Nr = normalize(N + offset * 1.0);
    vec3 Ng = N;
    vec3 Nb = normalize(N - offset * 1.0);

    float diffuseR = max(dot(Nr, L), 0.0) * 0.7 + 0.3;
    float diffuseG = max(dot(Ng, L), 0.0) * 0.7 + 0.3;
    float diffuseB = max(dot(Nb, L), 0.0) * 0.7 + 0.3;

    // Create separated color channels
    vec3 colorR = pc.baseColor.rgb * uTintR * diffuseR;
    vec3 colorG = pc.baseColor.rgb * uTintG * diffuseG;
    vec3 colorB = pc.baseColor.rgb * uTintB * diffuseB;

    // Blend channels with offset
    // R and B are offset, G is centered
    float aberrationMix = fresnel * 0.6 + 0.4;
    vec3 color;
    color.r = mix(colorG.r, colorR.r, aberrationMix);
    color.g = colorG.g;
    color.b = mix(colorG.b, colorB.b, aberrationMix);

    // Add some glow at edges
    vec3 edgeGlow = vec3(0.3, 0.1, 0.4) * fresnel * 0.5;
    color += edgeGlow;

    // Slight vignette darkening based on viewing angle
    color *= (1.0 - fresnel * 0.2);

    // Add subtle scanline effect
    float scanline = sin(fragWorldPos.y * 100.0) * 0.5 + 0.5;
    color *= 0.95 + scanline * 0.05;

    outColor = vec4(color, pc.baseColor.a);
}
