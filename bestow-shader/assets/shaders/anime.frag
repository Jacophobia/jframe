// Anime Shader - Anime-style with sharp highlights and strong rim lighting
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

// Anime parameters
uniform vec3 uShadowColor;       // Shadow color (default: cool purple tint)
uniform float uShadowSharpness;  // Shadow edge sharpness (default: 0.02)
uniform float uRimPower;         // Rim light power (default: 3.0)
uniform float uRimIntensity;     // Rim light intensity (default: 0.8)
uniform vec3 uRimColor;          // Rim light color (default: light blue)
uniform float uSpecSharpness;    // Specular sharpness (default: 0.98)
uniform float uSpecIntensity;    // Specular intensity (default: 1.0)

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uLightDir);
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 H = normalize(L + V);

    // Sharp two-tone lighting (anime style)
    float NdotL = max(dot(N, L), 0.0);
    float lightStep = smoothstep(0.5 - uShadowSharpness, 0.5 + uShadowSharpness, NdotL);

    // Base color
    vec3 baseColor = uBaseColor.rgb;

    // Two-tone shading
    vec3 litColor = baseColor * uLightColor;
    vec3 shadowTint = baseColor * uShadowColor;
    vec3 diffuse = mix(shadowTint, litColor, lightStep);

    // Strong rim lighting (key anime characteristic)
    float rimDot = 1.0 - max(dot(N, V), 0.0);
    float rim = pow(rimDot, uRimPower) * uRimIntensity;
    rim *= lightStep; // Only on lit side
    vec3 rimLight = uRimColor * rim;

    // Sharp specular highlight
    float NdotH = max(dot(N, H), 0.0);
    float spec = smoothstep(uSpecSharpness - 0.01, uSpecSharpness, NdotH);
    spec *= lightStep;
    vec3 specular = uLightColor * spec * uSpecIntensity;

    // Ambient
    vec3 ambient = uAmbientColor * baseColor * 0.3;

    // Combine
    vec3 color = ambient + diffuse + rimLight + specular;

    // Boost saturation for vibrant anime look
    float luma = dot(color, vec3(0.299, 0.587, 0.114));
    color = mix(vec3(luma), color, 1.25);

    // Slight contrast boost
    color = pow(color, vec3(0.95));

    FragColor = vec4(color, uBaseColor.a);
}
