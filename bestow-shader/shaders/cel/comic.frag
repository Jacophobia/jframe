#version 410 core

// Comic Book Style Fragment Shader
// Creates a comic book appearance with halftone dots and bold colors
//
// Configurable Parameters:
// - uHalftoneScale: Size of halftone dots (default: 50.0)
// - uOutlineThickness: Thickness of ink outlines (default: 0.02)
// - uColorBoost: Saturation boost for bold colors (default: 1.3)

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform vec4 uBaseColor;
uniform vec3 uCameraPos;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbientColor;

uniform float uHalftoneScale;
uniform float uOutlineThickness;
uniform float uColorBoost;
uniform bool uUseTexture;
uniform sampler2D uTexture;

// Convert RGB to HSV
vec3 rgb2hsv(vec3 c) {
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

// Convert HSV to RGB
vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

// Halftone pattern
float halftone(vec2 uv, float intensity) {
    vec2 nearest = fract(uv) - 0.5;
    float dist = length(nearest);
    float radius = 0.5 * sqrt(intensity);
    return smoothstep(radius, radius - 0.05, dist);
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uLightDir);
    vec3 V = normalize(uCameraPos - vWorldPos);

    // Calculate lighting
    float NdotL = max(dot(N, L), 0.0);

    // Quantize to 3 levels for comic look
    float intensity;
    if (NdotL > 0.7) intensity = 1.0;
    else if (NdotL > 0.3) intensity = 0.6;
    else intensity = 0.3;

    // Base color with boosted saturation
    vec4 albedo = uBaseColor * vColor;
    if (uUseTexture) {
        albedo *= texture(uTexture, vTexCoord);
    }

    // Boost saturation for bold comic colors
    vec3 hsv = rgb2hsv(albedo.rgb);
    hsv.y = min(hsv.y * uColorBoost, 1.0);
    vec3 boostedColor = hsv2rgb(hsv);

    // Apply halftone in shadow areas
    float halftonePattern = halftone(vTexCoord * uHalftoneScale, intensity);
    vec3 shadedColor = mix(boostedColor * 0.3, boostedColor, halftonePattern);

    // Apply lighting
    vec3 color = shadedColor * uLightColor * intensity;
    color += uAmbientColor * boostedColor * 0.3;

    // Outline detection (simplified - ideally done in post-process)
    float edge = pow(1.0 - abs(dot(V, N)), 2.0);
    if (edge > (1.0 - uOutlineThickness)) {
        color *= 0.2; // Dark ink outline
    }

    FragColor = vec4(color, albedo.a);
}
