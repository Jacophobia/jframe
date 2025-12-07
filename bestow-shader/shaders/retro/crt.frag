#version 410 core

// CRT Monitor Effect Fragment Shader
// Simulates scanlines, screen curvature, and phosphor glow
//
// Configurable Parameters:
// - uScanlineIntensity: Darkness of scanlines (default: 0.15)
// - uCurvature: Screen curvature amount (default: 0.15)
// - uVignetteStrength: Corner darkening (default: 0.4)
// - uBrightness: Overall brightness boost (default: 1.2)

out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D uTexture;
uniform float uTime;
uniform float uScanlineIntensity;
uniform float uCurvature;
uniform float uVignetteStrength;
uniform float uBrightness;
uniform vec2 uResolution;

// Apply CRT curvature
vec2 curveUV(vec2 uv) {
    uv = uv * 2.0 - 1.0;
    vec2 offset = abs(uv.yx) / uCurvature;
    uv = uv + uv * offset * offset;
    uv = uv * 0.5 + 0.5;
    return uv;
}

void main() {
    vec2 uv = vTexCoord;

    // Apply screen curvature
    vec2 curvedUV = curveUV(uv);

    // Check if outside screen bounds
    if (curvedUV.x < 0.0 || curvedUV.x > 1.0 || curvedUV.y < 0.0 || curvedUV.y > 1.0) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    // Sample texture
    vec4 color = texture(uTexture, curvedUV);

    // Scanlines
    float scanline = sin(curvedUV.y * uResolution.y * 3.14159);
    scanline = mix(1.0, scanline, uScanlineIntensity);

    // Horizontal scanline flickering
    float flicker = sin(uTime * 10.0 + curvedUV.y * 20.0) * 0.01 + 1.0;

    // Phosphor glow (RGB separation)
    float pixel = fract(curvedUV.x * uResolution.x * 0.333);
    vec3 phosphor = vec3(1.0);
    if (pixel < 0.333) phosphor = vec3(1.0, 0.7, 0.7);
    else if (pixel < 0.666) phosphor = vec3(0.7, 1.0, 0.7);
    else phosphor = vec3(0.7, 0.7, 1.0);

    // Vignette
    vec2 vignetteUV = uv * 2.0 - 1.0;
    float vignette = 1.0 - dot(vignetteUV, vignetteUV) * uVignetteStrength;

    // Combine effects
    color.rgb *= scanline * flicker * phosphor * vignette * uBrightness;

    // Add slight noise
    float noise = fract(sin(dot(uv + uTime, vec2(12.9898, 78.233))) * 43758.5453);
    color.rgb += noise * 0.02;

    FragColor = vec4(color.rgb, 1.0);
}
