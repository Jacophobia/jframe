// Portal Shader - Swirling portal effect
#version 410 core

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform float uTime;

// Portal parameters
uniform vec3 uPortalColor1;      // Inner color (default: purple)
uniform vec3 uPortalColor2;      // Outer color (default: blue)
uniform float uRotationSpeed;    // Rotation speed (default: 1.0)
uniform float uWarpAmount;       // Space warping (default: 0.5)
uniform float uPulseSpeed;       // Pulse speed (default: 2.0)

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
    vec2 uv = vTexCoord - 0.5; // Center
    float dist = length(uv);
    float angle = atan(uv.y, uv.x);

    float t = uTime * uRotationSpeed;

    // Spiral rotation
    float spiral = angle + dist * 10.0 - t * 3.0;

    // Warp space near center
    vec2 warpedUV = uv / (1.0 - dist * uWarpAmount);

    // Swirling pattern
    float pattern = noise(vec2(spiral * 2.0, dist * 5.0 - t));
    pattern += noise(warpedUV * 5.0 + t * 0.5) * 0.5;

    // Radial gradient
    float radial = 1.0 - smoothstep(0.0, 0.5, dist);

    // Pulsing energy
    float pulse = sin(t * uPulseSpeed + dist * 10.0) * 0.5 + 0.5;

    // Color gradient from center to edge
    vec3 color = mix(uPortalColor2, uPortalColor1, dist * 2.0);
    color *= pattern;
    color += uPortalColor1 * pulse * 0.3;

    // Bright center
    color += uPortalColor1 * (1.0 - dist) * 2.0;

    // Alpha fades at edges
    float alpha = radial * 0.9;

    FragColor = vec4(color, alpha);
}
