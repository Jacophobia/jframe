#version 410 core

// Water Material Fragment Shader
// Realistic water with reflections, refraction, and foam
//
// Configurable Parameters:
// - uWaterColor: Deep water color (default: dark blue)
// - uShallowColor: Shallow water color (default: cyan)
// - uSpecularPower: Shininess (default: 128.0)
// - uFoamAmount: Amount of foam at peaks (default: 0.3)

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform vec3 uCameraPos;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec4 uWaterColor;
uniform vec4 uShallowColor;
uniform float uSpecularPower;
uniform float uFoamAmount;
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
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 L = normalize(-uLightDir);
    vec3 H = normalize(L + V);

    // Fresnel for water
    float fresnel = pow(1.0 - max(dot(V, N), 0.0), 5.0);
    fresnel = mix(0.02, 1.0, fresnel);

    // Water depth simulation (based on normal)
    float depth = max(dot(N, vec3(0, 1, 0)), 0.0);
    vec3 waterColor = mix(uWaterColor.rgb, uShallowColor.rgb, depth);

    // Specular highlight
    float spec = pow(max(dot(N, H), 0.0), uSpecularPower);
    vec3 specular = spec * uLightColor;

    // Foam at wave peaks
    float foam = noise(vTexCoord * 20.0 + uTime * 0.5);
    foam = smoothstep(0.7, 1.0, foam) * smoothstep(1.0, 0.8, depth);
    vec3 foamColor = vec3(1.0) * foam * uFoamAmount;

    // Combine
    vec3 color = waterColor + specular + foamColor;

    // Water is semi-transparent
    float alpha = mix(0.7, 0.95, fresnel);

    FragColor = vec4(color, alpha);
}
