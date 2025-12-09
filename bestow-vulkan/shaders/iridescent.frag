#version 450

// Iridescence Shader - Thin-film interference effect
// Simulates soap bubbles, oil slicks, beetle shells

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 viewProjection;
    vec4 baseColor;
    vec4 lightDir;
    vec4 lightColor;
    vec4 ambientColor;
    vec4 cameraPos;
} pc;

// Iridescence parameters
const float uFilmThickness = 2.5;     // Controls color cycle frequency
const float uIridescenceStrength = 0.8;
const float uFresnelPower = 3.0;

// Convert wavelength (380-780nm normalized to 0-1) to RGB
vec3 wavelengthToRGB(float wavelength) {
    // Attempt to match visible spectrum colors
    vec3 color;

    if (wavelength < 0.17) {
        // Violet to blue
        color = vec3(0.5 - wavelength * 2.0, 0.0, 1.0);
    } else if (wavelength < 0.33) {
        // Blue to cyan
        float t = (wavelength - 0.17) / 0.16;
        color = vec3(0.0, t, 1.0);
    } else if (wavelength < 0.5) {
        // Cyan to green
        float t = (wavelength - 0.33) / 0.17;
        color = vec3(0.0, 1.0, 1.0 - t);
    } else if (wavelength < 0.67) {
        // Green to yellow
        float t = (wavelength - 0.5) / 0.17;
        color = vec3(t, 1.0, 0.0);
    } else if (wavelength < 0.83) {
        // Yellow to orange
        float t = (wavelength - 0.67) / 0.16;
        color = vec3(1.0, 1.0 - t * 0.5, 0.0);
    } else {
        // Orange to red
        float t = (wavelength - 0.83) / 0.17;
        color = vec3(1.0, 0.5 - t * 0.5, 0.0);
    }

    return color;
}

void main() {
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(-pc.lightDir.xyz);
    vec3 V = normalize(pc.cameraPos.xyz - fragWorldPos);
    vec3 H = normalize(L + V);

    // Calculate view-dependent thin-film interference
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);

    // Fresnel effect - stronger iridescence at glancing angles
    float fresnel = pow(1.0 - NdotV, uFresnelPower);

    // Thin-film interference pattern
    // Path difference creates color based on viewing angle
    float phase = NdotV * uFilmThickness;

    // Create multiple color cycles for realistic interference
    float cycle1 = fract(phase);
    float cycle2 = fract(phase * 1.5 + 0.33);
    float cycle3 = fract(phase * 2.0 + 0.66);

    // Convert to spectrum colors
    vec3 iridColor1 = wavelengthToRGB(cycle1);
    vec3 iridColor2 = wavelengthToRGB(cycle2);
    vec3 iridColor3 = wavelengthToRGB(cycle3);

    // Blend interference colors
    vec3 iridescence = (iridColor1 + iridColor2 * 0.5 + iridColor3 * 0.25) / 1.75;

    // Base diffuse lighting
    float diffuse = NdotL * 0.5 + 0.5;
    vec3 baseColor = pc.baseColor.rgb * diffuse;

    // Specular highlight
    float NdotH = max(dot(N, H), 0.0);
    float spec = pow(NdotH, 64.0) * 0.5;

    // Blend base color with iridescence based on fresnel
    float iridMix = fresnel * uIridescenceStrength;
    vec3 color = mix(baseColor, iridescence, iridMix);

    // Add specular
    color += pc.lightColor.rgb * spec;

    // Slight reflection boost
    color = mix(color, iridescence, fresnel * 0.2);

    outColor = vec4(color, pc.baseColor.a);
}
