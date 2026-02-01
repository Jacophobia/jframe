#version 450

// UI Fragment Shader
// Samples texture if present, multiplies by vertex color, applies premultiplied alpha.

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D texSampler;

layout(push_constant) uniform PushConstants {
    mat4 projection;
    vec2 translation;
    int hasTexture;
} pc;

void main() {
    vec4 texColor = pc.hasTexture != 0 ? texture(texSampler, fragTexCoord) : vec4(1.0);
    vec4 color = fragColor * texColor;
    // Convert to premultiplied alpha
    color.rgb *= color.a;
    outColor = color;
}
