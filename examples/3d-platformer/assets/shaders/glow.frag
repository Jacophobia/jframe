// Pulsing Glow Shader - emissive material with animated glow
#version 330 core

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

// Glow parameters
uniform vec3 uGlowColor;       // Color of the glow (default: 1, 0.5, 0)
uniform float uGlowIntensity;  // Base glow strength (default: 1.0)
uniform float uPulseSpeed;     // Pulse speed (default: 2.0)
uniform float uPulseMin;       // Minimum pulse value (default: 0.3)
uniform float uFresnelGlow;    // Edge glow amount (default: 0.5)

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uLightDir);
    vec3 V = normalize(uCameraPos - vWorldPos);

    // Basic diffuse lighting
    float NdotL = max(dot(N, L), 0.0);

    // Fresnel for edge glow
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 2.0);

    // Pulsing animation
    float pulse = sin(uTime * uPulseSpeed * 6.28) * 0.5 + 0.5;
    pulse = mix(uPulseMin, 1.0, pulse);

    // Calculate lighting
    vec3 baseColor = uBaseColor.rgb * vColor.rgb;
    vec3 diffuse = baseColor * NdotL * uLightColor;
    vec3 ambient = uAmbientColor * baseColor * 0.3;

    // Calculate glow
    vec3 glow = uGlowColor * uGlowIntensity * pulse;
    glow += uGlowColor * fresnel * uFresnelGlow;

    // Combine
    vec3 color = ambient + diffuse + glow;

    // Apply HDR tone mapping
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, uBaseColor.a * vColor.a);
}
