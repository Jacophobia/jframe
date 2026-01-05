#version 450

// Planetary surface vertex shader
// For rendering planet terrain with atmospheric contribution

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragWorldPos;
layout(location = 3) out vec3 fragViewDir;
layout(location = 4) out float fragLogDepth;
layout(location = 5) out float fragHeight;  // Height above surface for atmosphere calculation

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 viewProjection;
    vec4 planetCenter;      // xyz = planet center, w = planet radius
    vec4 atmosphereParams;  // x = atmosphere radius, y = scale height, z,w = unused
    vec4 cameraPos;         // xyz = camera world position, w = unused
    vec4 sunDir;            // xyz = direction to sun (normalized), w = sun intensity
    vec4 depthParams;       // x = logDepthCoeff, y = farPlane, z = 1/log(C*far+1), w = useLogDepth
    vec4 terrainParams;     // x = max height, y = texture scale, z,w = unused
} pc;

void main() {
    vec4 worldPos = pc.model * vec4(inPosition, 1.0);
    fragWorldPos = worldPos.xyz;
    fragNormal = normalize(mat3(pc.model) * inNormal);
    fragTexCoord = inTexCoord;

    // View direction
    fragViewDir = normalize(pc.cameraPos.xyz - worldPos.xyz);

    // Height above planet surface
    fragHeight = length(worldPos.xyz - pc.planetCenter.xyz) - pc.planetCenter.w;

    vec4 clipPos = pc.viewProjection * worldPos;
    gl_Position = clipPos;

    // Logarithmic depth
    float linearDepth = clipPos.w;
    float C = pc.depthParams.x;
    fragLogDepth = log(C * linearDepth + 1.0) * pc.depthParams.z;
}
