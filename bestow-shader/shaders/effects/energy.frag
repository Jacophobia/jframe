#version 410 core

// Energy Beam/Plasma Fragment Shader
// Creates animated energy/plasma effect with electric arcs
//
// Configurable Parameters:
// - uEnergyColor: Primary energy color (default: electric blue)
// - uCoreColor: Core/hotspot color (default: white)
// - uFlowSpeed: Speed of energy flow (default: 2.0)
// - uTurbulence: Amount of chaotic motion (default: 0.5)

out vec4 FragColor;

in vec2 vTexCoord;
in vec4 vColor;

uniform vec3 uEnergyColor;
uniform vec3 uCoreColor;
uniform float uFlowSpeed;
uniform float uTurbulence;
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
    vec2 uv = vTexCoord * 2.0 - 1.0;

    // Distance from center line
    float dist = abs(uv.y);

    // Flowing energy along X axis
    vec2 flowUV = vec2(uv.x - uTime * uFlowSpeed, uv.y * 3.0);

    // Electric arc turbulence
    float turbulence = fbm(flowUV * 2.0 + uTime) * uTurbulence;
    dist += turbulence * 0.3;

    // Core beam
    float core = smoothstep(0.2, 0.0, dist);

    // Outer glow
    float glow = smoothstep(0.8, 0.0, dist);

    // Flowing bands of energy
    float bands = sin(flowUV.x * 10.0 + fbm(flowUV) * 2.0) * 0.5 + 0.5;

    // Color mixing
    vec3 color = mix(uEnergyColor, uCoreColor, core);
    color *= (glow + core);
    color += bands * uEnergyColor * 0.3;

    // Alpha
    float alpha = glow + core * 2.0;

    FragColor = vec4(color * vColor.rgb, alpha * vColor.a);
}
