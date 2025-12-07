// Water Shader - Animated water surface
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

// Water parameters
uniform float uWaveSpeed;        // Wave animation speed (default: 1.0)
uniform float uWaveFrequency;    // Wave frequency (default: 2.0)
uniform float uWaveAmplitude;    // Wave height (default: 0.1)
uniform vec3 uShallowColor;      // Shallow water color (default: cyan)
uniform vec3 uDeepColor;         // Deep water color (default: dark blue)
uniform float uFoamAmount;       // Foam on peaks (default: 0.3)
uniform float uTransparency;     // Water transparency (default: 0.7)

// Noise for waves
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
    vec3 L = normalize(-uLightDir);
    vec3 H = normalize(V + L);

    // Animated wave normal perturbation
    vec2 uv = vWorldPos.xy * uWaveFrequency;
    float t = uTime * uWaveSpeed;

    float wave1 = noise(uv + t * 0.5);
    float wave2 = noise(uv * 2.0 - t * 0.3);
    float wave3 = noise(uv * 4.0 + t * 0.7);

    float wavePattern = (wave1 * 0.5 + wave2 * 0.3 + wave3 * 0.2) * uWaveAmplitude;

    // Perturb normal based on waves
    vec3 waveNormal = normalize(N + vec3(
        (wave1 - 0.5) * 0.3,
        (wave2 - 0.5) * 0.3,
        wavePattern
    ));

    // Fresnel for water
    float fresnel = pow(1.0 - max(dot(waveNormal, V), 0.0), 3.0);

    // Water color based on depth (simulated)
    vec3 waterColor = mix(uShallowColor, uDeepColor, fresnel);

    // Reflection
    vec3 R = reflect(-V, waveNormal);
    vec3 reflection = uAmbientColor * 0.6;

    // Specular (sun reflection on water)
    float NdotL = max(dot(waveNormal, L), 0.0);
    float spec = pow(max(dot(waveNormal, H), 0.0), 64.0);

    // Foam on wave peaks
    float foam = smoothstep(0.7, 1.0, wavePattern * 10.0) * uFoamAmount;

    // Combine
    vec3 color = mix(waterColor, reflection, fresnel * 0.5);
    color += uLightColor * spec * 0.8;
    color = mix(color, vec3(1.0), foam);

    // Apply lighting
    color *= (NdotL * 0.5 + 0.5) * uLightColor + uAmbientColor * 0.3;

    float alpha = mix(uTransparency, 1.0, fresnel);

    FragColor = vec4(color, alpha);
}
