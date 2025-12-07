// Lava Shader - Animated molten lava
#version 410 core

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform vec4 uBaseColor;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbientColor;
uniform vec3 uCameraPos;
uniform float uTime;

// Lava parameters
uniform vec3 uHotColor;          // Hot lava color (default: bright orange)
uniform vec3 uCoolColor;         // Cooled lava color (default: dark red)
uniform float uFlowSpeed;        // Lava flow speed (default: 0.5)
uniform float uFlowComplexity;   // Flow pattern complexity (default: 3.0)
uniform float uEmissive;         // Emissive intensity (default: 2.0)

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
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);

    // Flowing lava pattern
    vec2 pos = vTexCoord * uFlowComplexity;
    float t = uTime * uFlowSpeed;

    // Create flowing motion
    pos.x += t * 0.5;
    pos.y += fbm(pos + t * 0.2) * 0.5;

    // Lava heat pattern
    float heat = fbm(pos);
    heat += noise(pos * 4.0 + t) * 0.3;

    // Map heat to lava colors
    vec3 lavaColor = mix(uCoolColor, uHotColor, heat);

    // Add bright veins
    float veins = abs(noise(pos * 8.0 + t * 2.0) - 0.5) * 2.0;
    veins = pow(veins, 3.0);
    lavaColor = mix(lavaColor, uHotColor * 1.5, veins);

    // Emissive glow
    vec3 emissive = lavaColor * uEmissive;

    // Pulsating effect
    float pulse = sin(t * 2.0 + heat * 6.28) * 0.1 + 0.9;
    emissive *= pulse;

    // Slight ambient interaction
    vec3 color = emissive + uAmbientColor * uCoolColor * 0.1;

    FragColor = vec4(color, uBaseColor.a);
}
