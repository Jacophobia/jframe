#version 410 core

// Emissive Glow Fragment Shader
// Creates a pulsing glow effect with customizable parameters
//
// Configurable Parameters:
// - uGlowColor: Color of the glow (default: white)
// - uGlowIntensity: Brightness of glow (default: 2.0)
// - uPulseSpeed: Speed of pulse animation (default: 2.0)
// - uPulseAmount: How much the glow pulses (default: 0.3)

out vec4 FragColor;

in vec2 vTexCoord;
in vec4 vColor;

uniform vec4 uBaseColor;
uniform vec3 uGlowColor;
uniform float uGlowIntensity;
uniform float uPulseSpeed;
uniform float uPulseAmount;
uniform float uTime;
uniform bool uUseTexture;
uniform sampler2D uTexture;

void main() {
    vec4 color = uBaseColor * vColor;

    if (uUseTexture) {
        color *= texture(uTexture, vTexCoord);
    }

    // Pulsing animation
    float pulse = sin(uTime * uPulseSpeed) * 0.5 + 0.5;
    float intensity = uGlowIntensity * (1.0 - pulse * uPulseAmount);

    // Apply glow
    vec3 glow = uGlowColor * intensity;
    vec3 finalColor = color.rgb + glow;

    FragColor = vec4(finalColor, color.a);
}
