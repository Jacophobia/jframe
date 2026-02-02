#version 450

layout(push_constant) uniform PushConstants {
    mat4 projection;
    vec2 translation;
    int hasTexture;
} pc;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    vec2 pos = inPosition + pc.translation;
    gl_Position = pc.projection * vec4(pos, 0.0, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}
