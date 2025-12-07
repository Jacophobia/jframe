// Toon/Cel Shader - cartoon-style lighting with discrete bands
#version 410 core

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

// Toon shader parameters
uniform int uBands;           // Number of light bands (default: 3)
uniform float uOutlineWidth;  // Outline width (default: 0.02)
uniform vec3 uOutlineColor;   // Outline color (default: dark)
uniform float uSpecularSize;  // Specular highlight size (default: 0.9)
uniform vec3 uShadowTint;     // Shadow tint color (default: cool blue)

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uLightDir);
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 H = normalize(L + V);

    // Calculate diffuse with bands
    float NdotL = max(dot(N, L), 0.0);

    // Ensure some minimum lighting even on back-facing surfaces
    NdotL = NdotL * 0.7 + 0.3;  // Remap to 0.3-1.0 range (brighter shadows)

    // Smooth cel-shading with soft band transitions
    int bands = max(uBands, 2);
    float bandSize = 1.0 / float(bands);
    float bandPos = NdotL / bandSize;
    float bandIndex = floor(bandPos);
    float bandFrac = fract(bandPos);

    // Smooth transition between bands (prevents harsh rectangles on curved surfaces)
    float smoothFrac = smoothstep(0.0, 0.15, bandFrac) * (1.0 - smoothstep(0.85, 1.0, bandFrac));
    float quantized = (bandIndex + 0.5 + smoothFrac * 0.3) * bandSize;

    // Calculate specular highlight (subtle, not too harsh)
    float NdotH = max(dot(N, H), 0.0);
    float specular = smoothstep(uSpecularSize, uSpecularSize + 0.05, NdotH) * 0.3;

    // Subtle rim lighting (reduced intensity)
    float fresnel = 1.0 - max(dot(N, V), 0.0);
    float rim = pow(fresnel, 5.0) * 0.1;

    // Base color (use uBaseColor directly - per-object color is set before drawing)
    vec3 baseColor = uBaseColor.rgb;

    // Shadow tinting - shadows get tinted with cool color for depth
    vec3 shadowColor = mix(baseColor * 0.4, uShadowTint * baseColor * 0.6, 0.4);
    vec3 litColor = mix(shadowColor, baseColor, quantized);

    // Apply lighting
    vec3 diffuse = litColor * uLightColor;
    vec3 ambient = uAmbientColor * baseColor * 0.4;
    vec3 spec = uLightColor * specular * baseColor;  // Tint specular with base color
    vec3 rimColor = uLightColor * rim * baseColor;

    vec3 color = ambient + diffuse + spec + rimColor;

    // Slight saturation boost for cartoon look
    float gray = dot(color, vec3(0.299, 0.587, 0.114));
    color = mix(vec3(gray), color, 1.15);

    // Clamp to prevent over-brightness
    color = min(color, vec3(1.2));

    FragColor = vec4(color, uBaseColor.a);
}
