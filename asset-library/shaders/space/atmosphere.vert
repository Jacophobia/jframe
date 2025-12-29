#version 450

// Atmospheric scattering vertex shader
// Renders the atmospheric shell around a planet

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec3 fragViewDir;
layout(location = 2) out float fragLogDepth;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 viewProjection;
    vec4 planetCenter;      // xyz = world position, w = planet radius
    vec4 atmosphereParams;  // x = atmosphere radius, y = scale height, z = unused, w = unused
    vec4 cameraPos;         // xyz = camera world position, w = unused
    vec4 sunDir;            // xyz = direction to sun (normalized), w = sun intensity
    vec4 depthParams;       // x = logDepthCoeff, y = farPlane, z = 1/log(C*far+1), w = useLogDepth
} pc;

void main() {
    vec4 worldPos = pc.model * vec4(inPosition, 1.0);
    fragWorldPos = worldPos.xyz;

    // View direction from camera to fragment
    fragViewDir = normalize(worldPos.xyz - pc.cameraPos.xyz);

    vec4 clipPos = pc.viewProjection * worldPos;
    gl_Position = clipPos;

    // Logarithmic depth for extreme distances
    float linearDepth = clipPos.w;
    float C = pc.depthParams.x;
    fragLogDepth = log(C * linearDepth + 1.0) * pc.depthParams.z;
}
