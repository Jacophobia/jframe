#version 450

// Celestial body vertex shader with logarithmic depth support
// Used for rendering objects at extreme distances (planets, moons, stars)

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragWorldPos;
layout(location = 3) out float fragLogDepth;  // For logarithmic depth

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 viewProjection;
    vec4 baseColor;
    vec4 lightDir;
    vec4 lightColor;
    vec4 ambientColor;
    vec4 cameraPos;
    vec4 depthParams;  // x = logDepthCoeff, y = farPlane, z = 1/log(C*far+1), w = unused
} pc;

void main() {
    vec4 worldPos = pc.model * vec4(inPosition, 1.0);
    fragWorldPos = worldPos.xyz;
    fragNormal = mat3(pc.model) * inNormal;
    fragTexCoord = inTexCoord;

    vec4 clipPos = pc.viewProjection * worldPos;
    gl_Position = clipPos;

    // Compute logarithmic depth for fragment shader
    // Using reversed-Z: near = 1.0, far = 0.0
    // Linear depth after perspective divide
    float linearDepth = clipPos.w;  // Positive depth in view space

    // Log depth formula: log(C*z + 1) / log(C*far + 1)
    // depthParams.x = C (coefficient), depthParams.z = 1/log(C*far+1)
    float C = pc.depthParams.x;
    fragLogDepth = log(C * linearDepth + 1.0) * pc.depthParams.z;
}
