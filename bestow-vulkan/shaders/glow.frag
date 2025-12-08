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
    vec4 ambientColor;  // w = time for animation
    vec4 cameraPos;
} pc;

void main() {
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(-pc.lightDir.xyz);

    // Basic diffuse lighting
    float NdotL = max(dot(N, L), 0.0);

    // Golden base color
    vec3 baseColor = pc.baseColor.rgb;

    // Glow parameters (golden glow)
    vec3 glowColor = vec3(1.0, 0.85, 0.3);
    float glowIntensity = 1.8;
    float pulseSpeed = 1.5;
    float pulseMin = 0.6;
    float fresnelGlow = 0.8;

    // Animated pulse using time from ambientColor.w
    float time = pc.ambientColor.w;
    float pulse = pulseMin + (1.0 - pulseMin) * (0.5 + 0.5 * sin(time * pulseSpeed * 6.28318));

    // Fresnel (edge glow)
    vec3 V = normalize(pc.cameraPos.xyz - fragWorldPos);
    float fresnel = 1.0 - max(dot(N, V), 0.0);
    fresnel = pow(fresnel, 3.0) * fresnelGlow;

    // Combine lighting
    vec3 diffuse = baseColor * pc.lightColor.rgb * NdotL;
    vec3 ambient = baseColor * pc.ambientColor.rgb * 0.3;

    // Add glow
    vec3 glow = glowColor * glowIntensity * pulse;
    vec3 edge = glowColor * fresnel * pulse;

    vec3 color = diffuse + ambient + glow * 0.3 + edge;

    // Note: sRGB swapchain handles gamma correction automatically
    outColor = vec4(color, pc.baseColor.a);
}
