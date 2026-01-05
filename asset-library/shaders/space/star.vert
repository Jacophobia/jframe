#version 450

// Star/Sun rendering vertex shader
// For rendering bright emissive celestial bodies with corona/glow

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragWorldPos;
layout(location = 3) out float fragLogDepth;
layout(location = 4) out vec3 fragLocalPos;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 viewProjection;
    vec4 starParams;        // x = radius, y = temperature (Kelvin), z = luminosity, w = time
    vec4 cameraPos;
    vec4 depthParams;       // x = logDepthCoeff, y = farPlane, z = 1/log(C*far+1), w = useLogDepth
    vec4 coronaParams;      // x = corona size, y = corona intensity, z = limb darkening, w = unused
} pc;

void main() {
    fragLocalPos = inPosition;
    fragNormal = mat3(pc.model) * inNormal;
    fragTexCoord = inTexCoord;

    vec4 worldPos = pc.model * vec4(inPosition, 1.0);
    fragWorldPos = worldPos.xyz;

    vec4 clipPos = pc.viewProjection * worldPos;
    gl_Position = clipPos;

    // Logarithmic depth
    float linearDepth = clipPos.w;
    float C = pc.depthParams.x;
    fragLogDepth = log(C * linearDepth + 1.0) * pc.depthParams.z;
}
