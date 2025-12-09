#version 450

// Cross-Hatching Shader - Pen & ink illustration style
// Creates layered line patterns based on lighting intensity

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 viewProjection;
    vec4 baseColor;
    vec4 lightDir;
    vec4 lightColor;
    vec4 ambientColor;
    vec4 cameraPos;
} pc;

// Crosshatch parameters
const float uLineScale = 40.0;
const float uLineWidth = 0.15;
const vec3 uInkColor = vec3(0.05, 0.02, 0.1);
const vec3 uPaperColor = vec3(0.95, 0.92, 0.85);

// Create a single line pattern at given angle
float linePattern(vec2 uv, float angle, float width) {
    vec2 rotatedUV = vec2(
        uv.x * cos(angle) - uv.y * sin(angle),
        uv.x * sin(angle) + uv.y * cos(angle)
    );

    float line = abs(fract(rotatedUV.y) - 0.5) * 2.0;
    return smoothstep(width, width + 0.1, line);
}

void main() {
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(-pc.lightDir.xyz);

    // Calculate lighting intensity
    float NdotL = dot(N, L) * 0.5 + 0.5;

    // Create screen-space UV
    vec2 uv = fragWorldPos.xy * uLineScale;

    // Four layers of hatching at different angles
    // Each layer appears as darkness increases
    float layer1 = linePattern(uv, 0.785, uLineWidth);           // 45 degrees
    float layer2 = linePattern(uv, -0.785, uLineWidth);          // -45 degrees
    float layer3 = linePattern(uv, 0.0, uLineWidth * 0.8);       // Horizontal
    float layer4 = linePattern(uv, 1.5708, uLineWidth * 0.8);    // Vertical

    // Combine layers based on darkness
    float hatch = 1.0;

    // First layer appears at medium darkness
    if (NdotL < 0.75) {
        float t = smoothstep(0.75, 0.5, NdotL);
        hatch = mix(1.0, layer1, t);
    }

    // Second layer (cross-hatch) at darker areas
    if (NdotL < 0.5) {
        float t = smoothstep(0.5, 0.25, NdotL);
        hatch = min(hatch, mix(1.0, layer2, t));
    }

    // Third layer for deep shadows
    if (NdotL < 0.35) {
        float t = smoothstep(0.35, 0.15, NdotL);
        hatch = min(hatch, mix(1.0, layer3, t));
    }

    // Fourth layer for darkest areas
    if (NdotL < 0.2) {
        float t = smoothstep(0.2, 0.0, NdotL);
        hatch = min(hatch, mix(1.0, layer4, t));
    }

    // Mix ink and paper colors
    vec3 color = mix(uInkColor, uPaperColor, hatch);

    // Subtle tint from base color
    color = mix(color, color * pc.baseColor.rgb, 0.3);

    outColor = vec4(color, pc.baseColor.a);
}
