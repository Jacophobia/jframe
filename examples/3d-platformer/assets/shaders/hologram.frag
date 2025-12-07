// Hologram Shader - sci-fi holographic effect
#version 330 core

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform vec3 uCameraPos;
uniform float uTime;

// Hologram parameters
uniform vec3 uHoloColor;       // Main hologram color (default: cyan 0, 1, 1)
uniform float uScanlineSpeed;  // How fast scanlines move (default: 2.0)
uniform float uScanlineCount;  // Number of scanlines (default: 50)
uniform float uFlickerSpeed;   // Flicker speed (default: 10.0)
uniform float uGlitchIntensity; // Glitch amount (default: 0.1)
uniform float uFresnelPower;   // Edge glow intensity (default: 2.0)

// Pseudo-random function
float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);

    // Fresnel effect for edge glow
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), uFresnelPower);

    // Scanlines moving up the object
    float scanline = sin((vWorldPos.y + uTime * uScanlineSpeed) * uScanlineCount) * 0.5 + 0.5;
    scanline = smoothstep(0.3, 0.7, scanline);

    // Flicker effect
    float flicker = 0.9 + 0.1 * sin(uTime * uFlickerSpeed * 6.28);
    flicker *= 0.95 + 0.05 * random(vec2(floor(uTime * 20.0), 0.0));

    // Glitch offset (occasional horizontal displacement)
    float glitchTime = floor(uTime * 10.0);
    float glitchRand = random(vec2(glitchTime, vWorldPos.y * 10.0));
    float glitch = step(1.0 - uGlitchIntensity * 0.3, glitchRand);

    // Horizontal bands for glitch
    float bands = step(0.5, fract(vWorldPos.y * 20.0 + glitchTime * 0.1));

    // Combine effects
    float alpha = (0.3 + fresnel * 0.5 + scanline * 0.2) * flicker;

    // Add some noise texture
    float noise = random(vTexCoord * 100.0 + uTime);
    alpha += noise * 0.05;

    // Color variation
    vec3 color = uHoloColor;
    color += vec3(0.1, 0.0, 0.0) * glitch * bands;  // Red shift on glitch
    color += vec3(fresnel * 0.3);  // Brighter at edges

    // Output
    FragColor = vec4(color, alpha * uHoloColor.r * 1.5);
}
