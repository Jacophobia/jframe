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

// Base color texture (set 1, binding 0)
layout(set = 1, binding = 0) uniform sampler2D baseColorTex;

void main() {
    // Sample diffuse texture
    vec4 texColor = texture(baseColorTex, fragTexCoord);

    // Alpha test - discard transparent pixels (cutout transparency)
    // Only effective if texture has alpha channel
    if (texColor.a < 0.1) {
        discard;
    }

    // Blend texture with base color
    vec4 albedo = texColor * pc.baseColor;

    // Normalize the interpolated normal
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(-pc.lightDir.xyz);

    // Simple directional lighting with wrap
    float NdotL = dot(N, L);
    float wrap = 0.5;  // How much light wraps around (0 = none, 1 = full)
    float diffuse = max(0.0, (NdotL + wrap) / (1.0 + wrap));

    // Strong ambient to fill shadows
    vec3 ambient = vec3(0.4);  // Fixed ambient, ignore passed value for now

    // Combine
    vec3 litColor = albedo.rgb * (pc.lightColor.rgb * diffuse + ambient);

    outColor = vec4(litColor, albedo.a);
}
