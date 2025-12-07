// Glitch Shader - Digital glitch/corruption effect
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
uniform float uTime;

// Glitch parameters
uniform float uGlitchIntensity;  // Glitch intensity (default: 0.5)
uniform float uGlitchSpeed;      // Glitch frequency (default: 5.0)
uniform float uBlockSize;        // Glitch block size (default: 0.1)
uniform vec3 uGlitchColor;       // Glitch color overlay (default: cyan/magenta)

float hash(float p) {
    return fract(sin(p) * 43758.5453);
}

float hash2(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uLightDir);

    // Time-based glitch trigger
    float glitchTime = floor(uTime * uGlitchSpeed);
    float glitchRandom = hash(glitchTime);

    // Only glitch occasionally
    float glitchActive = step(1.0 - uGlitchIntensity, glitchRandom);

    // Block-based displacement
    vec2 blockUV = floor(vTexCoord / uBlockSize) * uBlockSize;
    float blockRandom = hash2(blockUV + glitchTime);

    // Offset coordinates
    vec2 glitchOffset = vec2(
        (blockRandom - 0.5) * 0.1 * glitchActive,
        0.0
    );

    // RGB channel split
    float splitAmount = blockRandom * 0.05 * glitchActive;

    // Base lighting
    float NdotL = max(dot(N, L), 0.0);
    vec3 baseColor = uBaseColor.rgb;
    vec3 lit = baseColor * (uLightColor * NdotL + uAmbientColor * 0.5);

    // Apply RGB split
    vec3 color = lit;
    if (glitchActive > 0.5) {
        // Simulate RGB channel separation
        color.r *= (1.0 + splitAmount);
        color.b *= (1.0 - splitAmount);
    }

    // Random color corruption
    if (blockRandom > 0.95 && glitchActive > 0.5) {
        color = mix(color, uGlitchColor, 0.5);
    }

    // Scanline corruption
    float scanline = step(0.7, hash(vWorldPos.y * 50.0 + glitchTime));
    color += scanline * glitchActive * 0.2;

    FragColor = vec4(color, uBaseColor.a);
}
