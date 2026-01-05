#version 450

// Planetary surface fragment shader
// Renders terrain with atmospheric in-scattering

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in vec3 fragViewDir;
layout(location = 4) in float fragLogDepth;
layout(location = 5) in float fragHeight;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D albedoTexture;
layout(set = 0, binding = 1) uniform sampler2D normalMap;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 viewProjection;
    vec4 planetCenter;      // xyz = planet center, w = planet radius
    vec4 atmosphereParams;  // x = atmosphere radius, y = scale height, z = rayleigh strength, w = mie strength
    vec4 cameraPos;         // xyz = camera world position, w = unused
    vec4 sunDir;            // xyz = direction to sun (normalized), w = sun intensity
    vec4 depthParams;       // x = logDepthCoeff, y = farPlane, z = 1/log(C*far+1), w = useLogDepth
    vec4 terrainParams;     // x = max height, y = texture scale, z = metallic, w = roughness
} pc;

const float PI = 3.14159265359;

// Simplified atmospheric in-scattering for surface
vec3 computeAtmosphericScattering(vec3 surfaceColor, vec3 viewDir, vec3 sunDir, float height, float distance) {
    float atmosphereHeight = pc.atmosphereParams.x - pc.planetCenter.w;
    float scaleHeight = pc.atmosphereParams.y;

    // Atmospheric density at surface (exponential falloff)
    float density = exp(-height / scaleHeight);

    // Distance through atmosphere (simplified)
    float atmosphereDepth = distance * density;

    // Rayleigh scattering color (blue sky)
    vec3 rayleighColor = vec3(0.0058, 0.0135, 0.0331) * pc.atmosphereParams.z;

    // Mie scattering (haze)
    float cosTheta = dot(viewDir, sunDir);
    float miePhase = 1.5 * ((1.0 - 0.98 * 0.98) * (1.0 + cosTheta * cosTheta)) /
                     ((2.0 + 0.98 * 0.98) * pow(1.0 + 0.98 * 0.98 - 2.0 * 0.98 * cosTheta, 1.5));
    vec3 mieColor = vec3(1.0) * miePhase * pc.atmosphereParams.w;

    // In-scattering
    vec3 inscatter = (rayleighColor + mieColor) * atmosphereDepth * pc.sunDir.w;

    // Out-scattering (attenuation of surface color)
    vec3 extinction = exp(-rayleighColor * atmosphereDepth * 0.5);

    return surfaceColor * extinction + inscatter;
}

void main() {
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(pc.sunDir.xyz);
    vec3 V = normalize(fragViewDir);

    // Sample textures
    vec2 scaledUV = fragTexCoord * pc.terrainParams.y;
    vec4 albedo = texture(albedoTexture, scaledUV);

    // Normal mapping (if normal map bound)
    vec3 normal = N;
    // Could sample normalMap here for detailed normals

    // Basic lighting
    float NdotL = max(dot(normal, L), 0.0);

    // Simple ambient occlusion based on height
    float ao = clamp(fragHeight / pc.terrainParams.x + 0.5, 0.3, 1.0);

    // Diffuse lighting
    vec3 diffuse = albedo.rgb * NdotL * pc.sunDir.w;

    // Ambient
    vec3 ambient = albedo.rgb * 0.03 * ao;

    // Combine lighting
    vec3 surfaceColor = diffuse + ambient;

    // Distance from camera
    float distance = length(fragWorldPos - pc.cameraPos.xyz);

    // Apply atmospheric scattering
    vec3 finalColor = computeAtmosphericScattering(surfaceColor, -V, L, fragHeight, distance);

    // Tone mapping
    finalColor = finalColor / (finalColor + vec3(1.0));

    outColor = vec4(finalColor, 1.0);

    // Logarithmic depth
    if (pc.depthParams.w > 0.5) {
        gl_FragDepth = 1.0 - fragLogDepth;
    } else {
        gl_FragDepth = gl_FragCoord.z;
    }
}
