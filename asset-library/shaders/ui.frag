#version 450

layout(push_constant) uniform PushConstants {
    mat4 projection;
    vec2 translation;
    int hasTexture;
} pc;

layout(set = 0, binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    if (pc.hasTexture != 0) {
        vec4 texColor = texture(texSampler, fragTexCoord);
        outColor = fragColor * texColor;
    } else {
        outColor = fragColor;
    }
}
