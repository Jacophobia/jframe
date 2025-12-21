#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

// Push constants for material/light data
layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 viewProjection;
    vec4 baseColor;        // offset 128
    vec4 lightDir;         // offset 144
    vec4 lightColor;       // offset 160
    vec4 ambientColor;     // offset 176
    vec4 cameraPos;        // offset 192
} pc;

// Base color texture (set 1, binding 0) - optional
layout(set = 1, binding = 0) uniform sampler2D baseColorTex;

void main() {
    // Sample texture - use texCoord UV
    vec4 texColor = texture(baseColorTex, fragTexCoord);

    // Blend texture with base color (texture modulates the base color)
    vec4 albedo = texColor * pc.baseColor;

    // Normalize inputs
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(-pc.lightDir.xyz);
    vec3 V = normalize(pc.cameraPos.xyz - fragWorldPos);
    vec3 H = normalize(L + V);

    // Diffuse lighting (Lambert)
    float NdotL = max(dot(N, L), 0.0);

    // Simple specular (Blinn-Phong)
    float NdotH = max(dot(N, H), 0.0);
    float specular = pow(NdotH, 32.0) * 0.3;

    // Combine lighting
    vec3 diffuse = albedo.rgb * pc.lightColor.rgb * NdotL;
    vec3 ambient = albedo.rgb * pc.ambientColor.rgb * pc.ambientColor.a;
    vec3 spec = pc.lightColor.rgb * specular;

    vec3 color = diffuse + ambient + spec;

    outColor = vec4(color, albedo.a);
}
