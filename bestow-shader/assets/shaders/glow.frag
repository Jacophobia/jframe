// Glow Shader - Emissive glow effect
#version 410 core

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform vec4 uBaseColor;
uniform vec3 uCameraPos;
uniform float uTime;

// Glow parameters
uniform vec3 uGlowColor;         // Glow color (default: bright cyan)
uniform float uGlowIntensity;    // Glow intensity (default: 2.0)
uniform float uPulseSpeed;       // Pulse animation speed (default: 1.0)
uniform float uPulseAmount;      // Pulse amount (default: 0.3)
uniform float uFresnelPower;     // Fresnel power for edge glow (default: 2.0)

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);

    // Fresnel for edge glow
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), uFresnelPower);

    // Pulsating animation
    float pulse = sin(uTime * uPulseSpeed) * uPulseAmount + (1.0 - uPulseAmount);

    // Base emissive
    vec3 emissive = uGlowColor * uGlowIntensity * pulse;

    // Add fresnel rim
    emissive += uGlowColor * fresnel * uGlowIntensity * 0.5;

    // Mix with base color
    vec3 color = mix(uBaseColor.rgb, emissive, 0.7);

    FragColor = vec4(color, uBaseColor.a);
}
