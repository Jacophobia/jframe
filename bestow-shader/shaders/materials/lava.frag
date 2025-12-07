#version 410 core

// Lava Material Fragment Shader
// Flowing molten lava with heat distortion
//
// Configurable Parameters:
// - uLavaColor1: Hot spots (default: yellow-white)
// - uLavaColor2: Cooler areas (default: orange-red)
// - uFlowSpeed: Speed of lava flow (default: 0.5)
// - uGlowIntensity: Emissive glow strength (default: 3.0)

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform vec3 uLavaColor1;
uniform vec3 uLavaColor2;
uniform float uFlowSpeed;
uniform float uGlowIntensity;
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
    for (int i = 0; i < 5; i++) {
        value += amplitude * noise(p);
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

void main() {
    // Flowing lava texture
    vec2 flowUV = vTexCoord + vec2(uTime * uFlowSpeed * 0.1, 0.0);

    // Multiple layers of noise for turbulent flow
    float lava = fbm(flowUV * 3.0);
    lava += fbm(flowUV * 6.0 - vec2(uTime * uFlowSpeed * 0.2, 0.0)) * 0.5;

    // Hot spots and cooler crust
    vec3 color;
    if (lava > 0.6) {
        // Hot yellow-white
        color = mix(uLavaColor1, vec3(1.0, 1.0, 0.9), (lava - 0.6) * 2.5);
    } else {
        // Orange to dark red
        color = mix(uLavaColor2, uLavaColor1, lava * 1.67);
    }

    // Cracks (dark veins)
    float cracks = fbm(vTexCoord * 20.0);
    cracks = smoothstep(0.45, 0.55, cracks);
    color = mix(color * 0.2, color, cracks);

    // Apply glow
    color *= uGlowIntensity;

    // Emissive lava is fully opaque
    FragColor = vec4(color, 1.0);
}
