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
    vec4 cameraPos;
} pc;

void main() {
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(-pc.lightDir.xyz);

    // View direction from camera position
    vec3 V = normalize(pc.cameraPos.xyz - fragWorldPos);
    vec3 H = normalize(L + V);
    vec3 R = reflect(-L, N);

    // Base diffuse lighting
    float NdotL = max(dot(N, L), 0.0);

    // Strong specular for metallic look
    float NdotH = max(dot(N, H), 0.0);
    float shininess = 128.0;  // Sharp specular
    float specular = pow(NdotH, shininess);

    // Fresnel effect - edges are more reflective
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 4.0);
    fresnel = 0.04 + 0.96 * fresnel;

    // Metallic properties (hardcoded for Vulkan)
    float metallic = 1.0;
    float roughness = 0.15;
    float reflectivity = 1.2;

    // Base color with metallic tint
    vec3 baseColor = pc.baseColor.rgb;
    vec3 specColor = baseColor;  // Metallic surfaces tint specular

    // Fake environment reflection
    vec3 envColor = mix(pc.ambientColor.rgb, pc.lightColor.rgb, max(dot(R, L), 0.0) * 0.5 + 0.5);
    envColor = mix(vec3(0.3, 0.4, 0.5), envColor, 0.7);

    // Diffuse is reduced for metallic
    vec3 diffuse = baseColor * pc.lightColor.rgb * NdotL * 0.2;

    // Ambient
    vec3 ambient = baseColor * pc.ambientColor.rgb * 0.2;

    // Specular highlight
    vec3 spec = specColor * pc.lightColor.rgb * specular * reflectivity;

    // Environment reflection
    vec3 reflection = envColor * fresnel * reflectivity * metallic * 0.5;

    // Rim lighting
    float rim = pow(1.0 - max(dot(N, V), 0.0), 3.0) * 0.3;
    vec3 rimColor = specColor * pc.lightColor.rgb * rim;

    vec3 color = ambient + diffuse + spec + reflection + rimColor;

    // Slight saturation boost
    float gray = dot(color, vec3(0.299, 0.587, 0.114));
    color = mix(vec3(gray), color, 1.1);

    // Clamp
    color = min(color, vec3(1.5));

    // Note: sRGB swapchain handles gamma correction automatically
    outColor = vec4(color, pc.baseColor.a);
}
