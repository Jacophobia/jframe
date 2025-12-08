#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

// Use push constants for material/light data (simple version)
layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 viewProjection;
    vec4 baseColor;        // offset 128
    vec4 lightDir;         // offset 144
    vec4 lightColor;       // offset 160
    vec4 ambientColor;     // offset 176
    vec4 cameraPos;        // offset 192
} pc;

void main() {
    // Normalize inputs
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(-pc.lightDir.xyz);

    // Diffuse lighting (Lambert)
    float NdotL = max(dot(N, L), 0.0);

    // Combine lighting
    vec3 diffuse = pc.baseColor.rgb * pc.lightColor.rgb * NdotL;
    vec3 ambient = pc.baseColor.rgb * pc.ambientColor.rgb * pc.ambientColor.a;

    vec3 color = diffuse + ambient;

    // Note: sRGB swapchain handles gamma correction automatically
    outColor = vec4(color, pc.baseColor.a);
}
