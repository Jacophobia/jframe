// Ghost Shader - Ethereal ghostly transparency
#version 410 core

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform vec4 uBaseColor;
uniform vec3 uCameraPos;
uniform float uTime;

// Ghost parameters
uniform vec3 uGhostColor;        // Ghost color (default: pale blue)
uniform float uWispiness;        // Wispy trail amount (default: 0.5)
uniform float uFloatSpeed;       // Floating animation speed (default: 1.0)
uniform float uTransparency;     // Base transparency (default: 0.3)
uniform float uGlowIntensity;    // Ethereal glow (default: 1.5)

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(hash(i), hash(i + vec2(1.0, 0.0)), f.x),
        mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), f.x),
        f.y
    );
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);

    // Wispy smoke pattern
    vec2 uv = vTexCoord * 3.0;
    float t = uTime * uFloatSpeed;
    float wisp = noise(uv + t * 0.2) * 0.5 + noise(uv * 2.0 - t * 0.3) * 0.3;
    wisp = pow(wisp, 1.5) * uWispiness;

    // Floating animation
    float floatPattern = sin(t + vWorldPos.y * 2.0) * 0.5 + 0.5;

    // Fresnel for ethereal edge glow
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 3.0);

    // Ghost color with variations
    vec3 color = uGhostColor * uBaseColor.rgb;
    color *= (wisp * 0.5 + 0.5);
    color += fresnel * uGhostColor * uGlowIntensity;
    color *= (floatPattern * 0.3 + 0.7);

    // Pulsing transparency
    float alpha = uTransparency + wisp * 0.3;
    alpha *= (floatPattern * 0.4 + 0.6);
    alpha += fresnel * 0.3;

    FragColor = vec4(color, alpha);
}
