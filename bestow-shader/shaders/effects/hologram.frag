#version 410 core

// Sci-Fi Hologram Fragment Shader
// Creates a futuristic hologram effect with scanlines and flickering
//
// Configurable Parameters:
// - uHologramColor: Primary hologram color (default: cyan)
// - uScanlineSpeed: Speed of scanline movement (default: 2.0)
// - uScanlineScale: Density of scanlines (default: 100.0)
// - uGlitchIntensity: Amount of glitch effect (default: 0.1)
// - uFlickerSpeed: Flicker frequency (default: 5.0)

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform vec4 uBaseColor;
uniform vec3 uCameraPos;
uniform vec3 uHologramColor;
uniform float uTime;
uniform float uScanlineSpeed;
uniform float uScanlineScale;
uniform float uGlitchIntensity;
uniform float uFlickerSpeed;
uniform bool uUseTexture;
uniform sampler2D uTexture;

// Hash for noise
float hash(float n) {
    return fract(sin(n) * 43758.5453);
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);

    // Base color
    vec4 albedo = uBaseColor * vColor;
    if (uUseTexture) {
        albedo *= texture(uTexture, vTexCoord);
    }

    // Fresnel effect for hologram edges
    float fresnel = pow(1.0 - abs(dot(V, N)), 2.0);

    // Scanlines
    float scanline = sin((vTexCoord.y + uTime * uScanlineSpeed * 0.1) * uScanlineScale);
    scanline = scanline * 0.5 + 0.5;
    scanline = pow(scanline, 3.0);

    // Flickering
    float flicker = hash(floor(uTime * uFlickerSpeed)) * 0.1 + 0.9;

    // Glitch effect
    float glitch = step(0.98, hash(floor(uTime * 10.0 + vWorldPos.y)));
    vec2 glitchUV = vTexCoord;
    glitchUV.x += glitch * (hash(floor(uTime * 20.0)) - 0.5) * uGlitchIntensity;

    // Sample with glitched UVs if applicable
    if (uUseTexture && glitch > 0.5) {
        albedo = texture(uTexture, glitchUV) * uBaseColor;
    }

    // Combine hologram effects
    vec3 hologramEffect = uHologramColor * (fresnel + scanline * 0.5) * flicker;
    vec3 finalColor = albedo.rgb * uHologramColor + hologramEffect;

    // Fade based on alpha and fresnel
    float alpha = albedo.a * (0.3 + fresnel * 0.7) * flicker;

    FragColor = vec4(finalColor, alpha);
}
