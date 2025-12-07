// Toon/Cel Shader - cartoon-style lighting with discrete bands
#version 330 core

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
uniform float uOutlineWidth;  // Outline width (default: 0.03)
uniform vec3 uOutlineColor;   // Outline color (default: black)
uniform float uSpecularSize;  // Specular highlight size (default: 0.9)

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uLightDir);
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 H = normalize(L + V);

    // Calculate diffuse with bands
    float NdotL = max(dot(N, L), 0.0);

    // Quantize to discrete bands
    int bands = max(uBands, 1);
    float bandSize = 1.0 / float(bands);
    float quantized = floor(NdotL / bandSize) * bandSize + bandSize * 0.5;

    // Apply smooth transition at band edges
    float edge = fract(NdotL / bandSize);
    float smoothEdge = smoothstep(0.0, 0.1, edge) * smoothstep(1.0, 0.9, edge);
    quantized = mix(quantized - bandSize * 0.5, quantized, smoothEdge);

    // Calculate specular highlight (sharp edge)
    float NdotH = max(dot(N, H), 0.0);
    float specular = step(uSpecularSize, NdotH);

    // Fresnel/rim outline
    float fresnel = 1.0 - max(dot(N, V), 0.0);
    float outline = step(1.0 - uOutlineWidth, fresnel);

    // Combine colors
    vec3 baseColor = uBaseColor.rgb * vColor.rgb;
    vec3 diffuse = baseColor * quantized;
    vec3 ambient = uAmbientColor * baseColor;
    vec3 spec = uLightColor * specular * 0.5;

    vec3 color = ambient + diffuse * uLightColor + spec;

    // Apply outline
    color = mix(color, uOutlineColor, outline);

    FragColor = vec4(color, uBaseColor.a * vColor.a);
}
