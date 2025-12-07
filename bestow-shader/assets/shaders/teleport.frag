// Teleport Shader - Teleportation sparkle/particle effect
#version 410 core

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform vec4 uBaseColor;
uniform vec3 uCameraPos;
uniform float uTime;

// Teleport parameters
uniform float uTeleportProgress;  // Teleport animation 0-1 (default: 0.0)
uniform vec3 uParticleColor;      // Particle color (default: white)
uniform float uParticleSpeed;     // Particle movement speed (default: 2.0)
uniform float uParticleDensity;   // Particle density (default: 50.0)
uniform float uDissolveEdge;      // Dissolve edge width (default: 0.1)

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

    // Particle field
    vec2 particleUV = vTexCoord * uParticleDensity;
    particleUV.y -= uTime * uParticleSpeed;

    float particles = 0.0;
    for (int i = 0; i < 3; i++) {
        float offset = float(i) * 100.0;
        vec2 cellUV = fract(particleUV + offset) - 0.5;
        float cellNoise = hash(floor(particleUV + offset));

        // Star-shaped particles
        float particle = max(abs(cellUV.x), abs(cellUV.y));
        particle = 1.0 - smoothstep(0.0, 0.1, particle);
        particle *= cellNoise;

        particles += particle;
    }

    // Dissolve pattern
    float dissolveNoise = noise(vTexCoord * 5.0);
    float dissolve = dissolveNoise - uTeleportProgress;

    // Base color fading
    vec3 baseColor = uBaseColor.rgb;
    float baseFade = smoothstep(-uDissolveEdge, uDissolveEdge, dissolve);

    // Edge glow where dissolving
    float edgeGlow = smoothstep(-uDissolveEdge, 0.0, dissolve) *
                     smoothstep(uDissolveEdge, 0.0, dissolve);

    // Fresnel particles
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 3.0);
    particles += fresnel * uTeleportProgress;

    // Combine
    vec3 color = baseColor * baseFade;
    color += uParticleColor * particles * 2.0;
    color += uParticleColor * edgeGlow * 3.0;

    float alpha = baseFade + particles * 0.5 + edgeGlow;
    alpha = min(alpha, 1.0);

    FragColor = vec4(color, alpha);
}
