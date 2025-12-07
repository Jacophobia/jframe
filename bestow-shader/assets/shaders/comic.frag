// Comic Book Shader - Halftone dots and bold outlines
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

// Comic parameters
uniform float uDotDensity;       // Halftone dot density (default: 30.0)
uniform float uOutlineWidth;     // Outline width (default: 0.05)
uniform vec3 uOutlineColor;      // Outline color (default: black)
uniform float uDotContrast;      // Dot contrast (default: 0.8)
uniform vec3 uHighlightColor;    // Highlight color (default: white)

// Halftone dot pattern
float halftone(vec2 coord, float density, float intensity) {
    vec2 cell = fract(coord * density);
    vec2 center = vec2(0.5);
    float dist = length(cell - center);

    // Dot size based on intensity
    float radius = 0.4 * sqrt(1.0 - intensity);
    return smoothstep(radius - 0.05, radius, dist);
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uLightDir);
    vec3 V = normalize(uCameraPos - vWorldPos);

    // Calculate lighting
    float NdotL = max(dot(N, L), 0.0);
    float intensity = NdotL * 0.7 + 0.3;

    // Apply halftone pattern
    vec2 coord = vWorldPos.xy * 0.1;
    float dots = halftone(coord, uDotDensity, intensity);

    // Bold outlines
    float fresnel = 1.0 - max(dot(N, V), 0.0);
    float outline = step(1.0 - uOutlineWidth, fresnel);

    // Hard specular highlight for comic look
    vec3 H = normalize(L + V);
    float specular = pow(max(dot(N, H), 0.0), 64.0);
    float hardSpec = step(0.7, specular);

    // Base color
    vec3 baseColor = uBaseColor.rgb;

    // Mix with halftone
    vec3 color = mix(baseColor * 0.3, baseColor, dots);

    // Apply lighting
    color *= uLightColor * mix(0.5, 1.0, intensity);
    color += uAmbientColor * baseColor * 0.2;

    // Add highlight spots
    color = mix(color, uHighlightColor, hardSpec * 0.8);

    // Apply outline
    color = mix(color, uOutlineColor, outline);

    // Boost contrast and saturation for comic look
    color = pow(color, vec3(0.9));
    float luma = dot(color, vec3(0.299, 0.587, 0.114));
    color = mix(vec3(luma), color, 1.3);

    FragColor = vec4(color, uBaseColor.a);
}
