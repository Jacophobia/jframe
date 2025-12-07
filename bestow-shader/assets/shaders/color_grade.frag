// Color Grading Post-Process Shader - Color correction
#version 410 core

out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D uSceneTexture;  // Scene color texture
uniform float uExposure;          // Exposure adjustment (default: 1.0)
uniform float uContrast;          // Contrast (default: 1.0)
uniform float uSaturation;        // Saturation (default: 1.0)
uniform float uBrightness;        // Brightness (default: 0.0)
uniform vec3 uColorTint;          // Color tint (default: white)
uniform float uGamma;             // Gamma correction (default: 2.2)

void main() {
    vec3 color = texture(uSceneTexture, vTexCoord).rgb;

    // Exposure
    color *= uExposure;

    // Brightness
    color += vec3(uBrightness);

    // Contrast
    color = (color - 0.5) * uContrast + 0.5;

    // Saturation
    float luma = dot(color, vec3(0.299, 0.587, 0.114));
    color = mix(vec3(luma), color, uSaturation);

    // Color tint
    color *= uColorTint;

    // Gamma correction
    color = pow(color, vec3(1.0 / uGamma));

    // Clamp
    color = clamp(color, 0.0, 1.0);

    FragColor = vec4(color, 1.0);
}
