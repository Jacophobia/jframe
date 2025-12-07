// Toon/Cel Shader - Cartoon-style lighting with discrete bands
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
uniform int uBands;              // Number of light bands (default: 3)
uniform float uOutlineWidth;     // Outline width (default: 0.03)
uniform vec3 uOutlineColor;      // Outline color (default: black)
uniform float uSpecularSize;     // Specular highlight size (default: 0.9)
uniform vec3 uShadowTint;        // Shadow tint color (default: cool blue)
uniform float uSmoothness;       // Band edge smoothness (default: 0.05)

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uLightDir);
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 H = normalize(L + V);

    // Calculate diffuse with bands
    float NdotL = max(dot(N, L), 0.0);

    // Quantize to discrete bands for cel-shading look
    int bands = max(uBands, 2);
    float bandSize = 1.0 / float(bands);
    float quantized = floor(NdotL / bandSize) * bandSize + bandSize * 0.5;

    // Apply smooth transition at band edges
    float edge = fract(NdotL / bandSize);
    float smoothEdge = smoothstep(0.0, uSmoothness, edge) * smoothstep(1.0, 1.0 - uSmoothness, edge);
    quantized = mix(quantized - bandSize * 0.5, quantized, smoothEdge);

    // Calculate specular highlight (sharp cel-shaded edge)
    float NdotH = max(dot(N, H), 0.0);
    float specular = step(uSpecularSize, NdotH);

    // Fresnel/rim lighting for that anime pop
    float fresnel = 1.0 - max(dot(N, V), 0.0);
    float rim = pow(fresnel, 3.0) * 0.3;

    // Outline detection (silhouette edge)
    float outline = step(1.0 - uOutlineWidth, fresnel);

    // Base color with vertex color support
    vec3 baseColor = uBaseColor.rgb;
    if (length(vColor.rgb) > 0.01) {
        baseColor *= vColor.rgb;
    }

    // Shadow tinting
    vec3 shadowColor = mix(baseColor * 0.3, uShadowTint * baseColor * 0.5, 0.5);
    vec3 litColor = mix(shadowColor, baseColor, quantized);

    // Apply lighting
    vec3 diffuse = litColor * uLightColor;
    vec3 ambient = uAmbientColor * baseColor * 0.4;
    vec3 spec = uLightColor * specular * 0.6;
    vec3 rimColor = uLightColor * rim;

    vec3 color = ambient + diffuse + spec + rimColor;

    // Apply outline
    color = mix(color, uOutlineColor, outline);

    // Slight saturation boost for cartoon look
    float gray = dot(color, vec3(0.299, 0.587, 0.114));
    color = mix(vec3(gray), color, 1.1);

    FragColor = vec4(color, uBaseColor.a);
}
