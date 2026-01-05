#version 450

// Celestial body fragment shader with logarithmic depth support
// Writes custom depth for extreme-range rendering

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in float fragLogDepth;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 viewProjection;
    vec4 baseColor;
    vec4 lightDir;
    vec4 lightColor;
    vec4 ambientColor;
    vec4 cameraPos;
    vec4 depthParams;  // x = logDepthCoeff, y = farPlane, z = 1/log(C*far+1), w = useLogDepth
} pc;

void main() {
    // Normalize inputs
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(-pc.lightDir.xyz);

    // Diffuse lighting (Lambert)
    float NdotL = max(dot(N, L), 0.0);

    // Combine lighting
    vec3 diffuse = pc.baseColor.rgb * pc.lightColor.rgb * NdotL;
    vec3 ambient = pc.baseColor.rgb * pc.ambientColor.rgb * pc.ambientColor.a;

    vec3 color = diffuse + ambient;

    outColor = vec4(color, pc.baseColor.a);

    // Write logarithmic depth if enabled
    // For reversed-Z: 1.0 = near, 0.0 = far
    // fragLogDepth is already normalized to [0,1] range
    if (pc.depthParams.w > 0.5) {
        // Reversed-Z: invert the log depth (1.0 - logDepth gives us near=1, far=0)
        gl_FragDepth = 1.0 - fragLogDepth;
    } else {
        // Standard depth (use hardware depth)
        gl_FragDepth = gl_FragCoord.z;
    }
}
