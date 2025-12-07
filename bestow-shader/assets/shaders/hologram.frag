// Hologram Shader - Sci-fi holographic effect
#version 410 core

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform vec4 uBaseColor;
uniform vec3 uCameraPos;
uniform float uTime;

// Hologram parameters
uniform vec3 uHoloColor;         // Hologram color (default: cyan)
uniform float uScanlineSpeed;    // Scanline animation speed (default: 2.0)
uniform float uScanlineDensity;  // Scanline density (default: 20.0)
uniform float uFlickerSpeed;     // Flicker speed (default: 10.0)
uniform float uGlitchAmount;     // Glitch intensity (default: 0.1)
uniform float uTransparency;     // Base transparency (default: 0.5)

float hash(float p) {
    return fract(sin(p) * 43758.5453);
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);

    // Scanlines
    float scanline = fract(vWorldPos.y * uScanlineDensity + uTime * uScanlineSpeed);
    scanline = smoothstep(0.4, 0.6, scanline);

    // Fresnel for hologram edge
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 3.0);

    // Flickering
    float flicker = hash(floor(uTime * uFlickerSpeed)) * 0.2 + 0.8;

    // Random glitches
    float glitchTime = floor(uTime * 2.0);
    float glitch = hash(glitchTime + vWorldPos.y * 10.0);
    glitch = step(1.0 - uGlitchAmount, glitch) * 0.5;

    // Hologram color
    vec3 color = uHoloColor * uBaseColor.rgb;
    color *= (scanline * 0.3 + 0.7);
    color *= flicker;
    color += fresnel * uHoloColor * 0.8;
    color += glitch * uHoloColor;

    // Transparency with fresnel
    float alpha = mix(uTransparency, 1.0, fresnel * 0.5);
    alpha *= flicker;

    FragColor = vec4(color, alpha);
}
