// Electricity Shader - Electric arc/lightning effect
#version 410 core

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform float uTime;

// Electricity parameters
uniform vec3 uArcColor;          // Arc color (default: electric blue)
uniform float uBoltSpeed;        // Arc animation speed (default: 10.0)
uniform float uBranchiness;      // Arc branching (default: 0.5)
uniform float uThickness;        // Arc thickness (default: 0.05)
uniform float uIntensity;        // Glow intensity (default: 2.0)

float hash(float p) {
    return fract(sin(p) * 43758.5453);
}

float hash2(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(hash2(i), hash2(i + vec2(1.0, 0.0)), f.x),
        mix(hash2(i + vec2(0.0, 1.0)), hash2(i + vec2(1.0, 1.0)), f.x),
        f.y
    );
}

void main() {
    vec2 uv = vTexCoord;
    float t = uTime * uBoltSpeed;

    // Main arc path (jagged lightning)
    float arcTime = floor(t);
    float arcPhase = fract(t);

    // Branching path
    float path = 0.5;
    float segments = 10.0;

    for (float i = 0.0; i < segments; i++) {
        float segmentPos = i / segments;
        if (uv.x > segmentPos && uv.x < segmentPos + 1.0 / segments) {
            float randomOffset = hash(arcTime + i) - 0.5;
            path += randomOffset * uBranchiness / segments;
        }
    }

    // Distance to arc
    float dist = abs(uv.y - path);

    // Arc brightness (varies with time)
    float brightness = hash(arcTime) * 0.5 + 0.5;
    brightness *= (1.0 - arcPhase); // Fade during transition

    // Create sharp arc line
    float arc = smoothstep(uThickness, uThickness * 0.5, dist);
    arc *= brightness;

    // Glow around arc
    float glow = exp(-dist * 20.0) * 0.5;
    glow *= brightness;

    // Combine
    float intensity = arc + glow;

    vec3 color = uArcColor * intensity * uIntensity;

    // Add core brightness
    color += vec3(1.0) * arc * 0.5;

    float alpha = intensity;

    FragColor = vec4(color, alpha);
}
