#version 450

// Kuwahara-Inspired Shader - Oil painting effect
// Creates painterly look with preserved edges using normal-based regions

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

// Kuwahara parameters
const float uBrushScale = 15.0;
const float uColorVariation = 0.15;
const int uBrushStrokes = 4;

// Noise functions for brush texture
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < 4; i++) {
        value += amplitude * noise(p);
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

void main() {
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(-pc.lightDir.xyz);
    vec3 V = normalize(pc.cameraPos.xyz - fragWorldPos);

    // Basic lighting
    float NdotL = max(dot(N, L), 0.0);
    float diffuse = NdotL * 0.6 + 0.4;

    // Create brush stroke UV based on world position
    vec2 brushUV = fragWorldPos.xy * uBrushScale;

    // Add directional brush strokes aligned with surface tangent
    vec3 tangent = normalize(cross(N, vec3(0.0, 1.0, 0.0)));
    if (length(tangent) < 0.1) tangent = normalize(cross(N, vec3(1.0, 0.0, 0.0)));

    // Rotate brush based on surface orientation
    float angle = atan(tangent.y, tangent.x);
    vec2 rotatedUV = vec2(
        brushUV.x * cos(angle) - brushUV.y * sin(angle),
        brushUV.x * sin(angle) + brushUV.y * cos(angle)
    );

    // Generate brush texture
    float brushNoise = fbm(rotatedUV * 0.5);
    float strokePattern = fbm(rotatedUV * vec2(0.3, 1.0));  // Elongated strokes

    // Quantize colors like oil paint mixing
    vec3 baseColor = pc.baseColor.rgb;

    // Create color palette variations (like mixed paint)
    vec3 palette[4];
    palette[0] = baseColor * 1.2;  // Highlight
    palette[1] = baseColor;         // Base
    palette[2] = baseColor * 0.7;   // Shadow
    palette[3] = mix(baseColor, vec3(0.2, 0.1, 0.3), 0.3);  // Deep shadow with blue

    // Select palette color based on lighting and noise
    float paletteIndex = diffuse + brushNoise * uColorVariation;
    paletteIndex = clamp(paletteIndex * 3.0, 0.0, 3.0);

    int idx = int(paletteIndex);
    float t = fract(paletteIndex);

    // Smooth palette interpolation
    vec3 color;
    if (idx >= 3) {
        color = palette[3];
    } else {
        color = mix(palette[idx], palette[min(idx + 1, 3)], smoothstep(0.3, 0.7, t));
    }

    // Add brush stroke texture
    float strokeIntensity = strokePattern * 0.1 + 0.95;
    color *= strokeIntensity;

    // Add impasto effect (thick paint catching light)
    float impasto = pow(brushNoise, 2.0) * 0.15;
    vec3 H = normalize(L + V);
    float impastoHighlight = pow(max(dot(N, H), 0.0), 16.0) * impasto;
    color += pc.lightColor.rgb * impastoHighlight;

    // Slight color bleeding at edges
    float fresnel = pow(1.0 - abs(dot(N, V)), 2.0);
    vec3 bleedColor = mix(baseColor, vec3(0.9, 0.85, 0.8), 0.5);  // Canvas color
    color = mix(color, bleedColor, fresnel * 0.15);

    // Subtle canvas texture
    float canvas = noise(fragWorldPos.xy * 50.0) * 0.05 + 0.975;
    color *= canvas;

    outColor = vec4(color, pc.baseColor.a);
}
