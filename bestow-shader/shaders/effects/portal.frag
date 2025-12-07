#version 410 core

// Portal Effect Fragment Shader
// Creates a swirling vortex portal effect
//
// Configurable Parameters:
// - uPortalColor1: Inner portal color (default: purple)
// - uPortalColor2: Outer portal color (default: blue)
// - uRotationSpeed: Speed of spiral rotation (default: 1.0)
// - uDistortionAmount: How much the portal distorts (default: 0.3)

out vec4 FragColor;

in vec2 vTexCoord;
in vec4 vColor;

uniform vec3 uPortalColor1;
uniform vec3 uPortalColor2;
uniform float uRotationSpeed;
uniform float uDistortionAmount;
uniform float uTime;

// Hash for noise
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

void main() {
    vec2 uv = vTexCoord * 2.0 - 1.0;

    // Distance from center
    float dist = length(uv);

    // Angle for spiral
    float angle = atan(uv.y, uv.x);

    // Spiral distortion
    float spiral = angle + dist * 5.0 - uTime * uRotationSpeed;

    // Noise layers for turbulence
    float n = noise(vec2(spiral * 2.0, dist * 5.0 + uTime * 0.5));
    n += noise(vec2(spiral * 4.0, dist * 10.0 - uTime * 0.3)) * 0.5;

    // Distort based on noise
    vec2 distortedUV = uv + vec2(
        cos(spiral + n * uDistortionAmount),
        sin(spiral + n * uDistortionAmount)
    ) * 0.1;

    float distortedDist = length(distortedUV);

    // Color gradient from center to edge
    vec3 color = mix(uPortalColor1, uPortalColor2, distortedDist);

    // Add bright spiral bands
    float bands = sin(spiral * 3.0 + n * 2.0) * 0.5 + 0.5;
    color += bands * 0.3;

    // Fade at edges
    float alpha = smoothstep(1.0, 0.5, dist);

    // Brighten center
    color += (1.0 - smoothstep(0.0, 0.3, dist)) * uPortalColor1 * 0.5;

    FragColor = vec4(color * vColor.rgb, alpha * vColor.a);
}
