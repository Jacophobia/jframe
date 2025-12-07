// Foliage Shader - Vegetation with wind animation (use with animated.vert)
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
uniform float uTime;

// Foliage parameters
uniform vec3 uSubsurfaceColor;   // Subsurface color (default: lighter green)
uniform float uSubsurfaceAmount; // Subsurface strength (default: 0.5)
uniform float uAlphaCutoff;      // Alpha cutoff for leaves (default: 0.5)
uniform bool uTwoSided;          // Two-sided rendering (default: true)

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uLightDir);
    vec3 V = normalize(uCameraPos - vWorldPos);

    // Flip normal for backfaces if two-sided
    if (uTwoSided && !gl_FrontFacing) {
        N = -N;
    }

    // Subsurface scattering (light through leaves)
    float backLight = max(dot(-N, L), 0.0);
    vec3 subsurface = uSubsurfaceColor * uLightColor * backLight * uSubsurfaceAmount;

    // Diffuse lighting with wrap
    float NdotL = dot(N, L);
    float wrappedDiffuse = max((NdotL + 0.4) / 1.4, 0.0);

    vec3 baseColor = uBaseColor.rgb;

    // Apply vertex color for variation
    if (length(vColor.rgb) > 0.01) {
        baseColor *= vColor.rgb;
    }

    vec3 diffuse = baseColor * uLightColor * wrappedDiffuse;
    vec3 ambient = baseColor * uAmbientColor * 0.6;

    vec3 color = ambient + diffuse + subsurface;

    // Slight wind sway color variation
    float swayNoise = sin(uTime * 2.0 + vWorldPos.x + vWorldPos.z) * 0.5 + 0.5;
    color *= (0.95 + swayNoise * 0.1);

    // Alpha from vertex alpha (for leaf cutouts)
    float alpha = vColor.a * uBaseColor.a;

    // Alpha test for leaves
    if (alpha < uAlphaCutoff) {
        discard;
    }

    FragColor = vec4(color, alpha);
}
