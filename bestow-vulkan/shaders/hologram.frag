#version 450

// Hologram Shader - Sci-fi projection effect
// Scanlines, flickering, edge glow, transparency

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

// Hologram parameters
const vec3 uHoloColor = vec3(0.2, 0.8, 1.0);  // Cyan hologram
const float uScanlineScale = 200.0;
const float uScanlineIntensity = 0.3;
const float uFlickerSpeed = 3.0;
const float uGlitchIntensity = 0.1;
const float uEdgeGlow = 2.0;

// Simple pseudo-random
float hash(float n) {
    return fract(sin(n) * 43758.5453);
}

// Animated noise for flicker effect
float noise(float t) {
    float i = floor(t);
    float f = fract(t);
    return mix(hash(i), hash(i + 1.0), f);
}

void main() {
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(pc.cameraPos.xyz - fragWorldPos);

    // Use lightDir.w as time (or we simulate time with world position)
    float time = pc.lightDir.w + fragWorldPos.x * 0.1;

    // Fresnel for edge glow
    float fresnel = pow(1.0 - abs(dot(N, V)), uEdgeGlow);

    // Scanlines based on world Y position
    float scanline = sin(fragWorldPos.y * uScanlineScale) * 0.5 + 0.5;
    scanline = pow(scanline, 2.0);
    float scanlineEffect = 1.0 - scanline * uScanlineIntensity;

    // Horizontal bands (interference patterns)
    float bands = sin(fragWorldPos.y * 50.0 + time * 5.0) * 0.5 + 0.5;
    bands = smoothstep(0.4, 0.6, bands);

    // Flicker effect
    float flicker = noise(time * uFlickerSpeed) * 0.3 + 0.7;

    // Random glitch offset
    float glitch = step(0.98, noise(time * 20.0)) * uGlitchIntensity;

    // Base hologram color with edge glow
    vec3 holoBase = uHoloColor * pc.baseColor.rgb;
    vec3 edgeColor = uHoloColor * 2.0;  // Brighter edges

    // Combine effects
    vec3 color = mix(holoBase, edgeColor, fresnel);
    color *= scanlineEffect;
    color *= flicker;
    color += bands * uHoloColor * 0.2;

    // Add glitch offset color
    color += vec3(glitch, -glitch * 0.5, glitch * 0.5);

    // Alpha based on fresnel (more transparent in center)
    float alpha = mix(0.4, 0.9, fresnel);
    alpha *= flicker;
    alpha *= pc.baseColor.a;

    // Add subtle inner structure lines
    float innerLines = sin(fragWorldPos.z * 30.0) * sin(fragWorldPos.x * 30.0);
    innerLines = smoothstep(0.8, 1.0, abs(innerLines));
    color += innerLines * uHoloColor * 0.1;

    outColor = vec4(color, alpha);
}
