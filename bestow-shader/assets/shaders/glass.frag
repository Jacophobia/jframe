// Glass Shader - Transparent glass with refraction
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

// Glass parameters
uniform float uRefractiveIndex;  // Index of refraction (default: 1.5)
uniform float uThickness;        // Glass thickness (default: 0.1)
uniform float uRoughness;        // Surface roughness (default: 0.05)
uniform vec3 uTintColor;         // Glass tint (default: white)
uniform float uOpacity;          // Opacity (default: 0.3)

const float PI = 3.14159265359;

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 L = normalize(-uLightDir);
    vec3 H = normalize(V + L);

    // Fresnel effect (glass is more reflective at grazing angles)
    vec3 F0 = vec3(0.04); // Glass base reflectivity
    vec3 F = fresnelSchlick(max(dot(N, V), 0.0), F0);

    // Reflection
    vec3 R = reflect(-V, N);
    vec3 reflection = uAmbientColor * 0.8; // Simplified environment

    // Refraction (simplified - no actual texture refraction)
    float eta = 1.0 / uRefractiveIndex;
    vec3 refracted = refract(-V, N, eta);
    vec3 refraction = uTintColor * uBaseColor.rgb;

    // Absorption based on thickness
    float absorption = exp(-uThickness * 2.0);
    refraction *= absorption;

    // Mix reflection and refraction based on Fresnel
    vec3 color = mix(refraction, reflection, F);

    // Specular highlight
    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float spec = pow(NdotH, (1.0 - uRoughness) * 128.0);
    color += uLightColor * spec * 0.5;

    // Add slight ambient
    color += uAmbientColor * uTintColor * 0.1;

    // Calculate final alpha (more transparent at perpendicular angles)
    float alpha = mix(uOpacity, 1.0, F.r);

    FragColor = vec4(color, alpha);
}
