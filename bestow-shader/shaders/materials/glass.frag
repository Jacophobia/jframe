#version 410 core

// Glass Material Fragment Shader
// Simulates transparent glass with refraction and reflection
//
// Configurable Parameters:
// - uRefractiveIndex: Index of refraction (default: 1.5 for glass)
// - uThickness: Glass thickness for absorption (default: 0.1)
// - uTint: Glass color tint (default: slight blue)

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform vec3 uCameraPos;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform float uRefractiveIndex;
uniform float uThickness;
uniform vec4 uTint;
uniform samplerCube uEnvironmentMap; // For reflections/refractions

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 L = normalize(-uLightDir);

    // Fresnel effect
    float fresnel = pow(1.0 - max(dot(V, N), 0.0), 3.0);
    fresnel = mix(0.04, 1.0, fresnel); // Glass has ~4% base reflection

    // Refraction
    vec3 refractDir = refract(-V, N, 1.0 / uRefractiveIndex);

    // Reflection
    vec3 reflectDir = reflect(-V, N);

    // Sample environment (if available)
    vec3 refractColor = uTint.rgb; // Fallback to tint
    vec3 reflectColor = uLightColor;

    // Note: In production, sample from environment map:
    // refractColor = texture(uEnvironmentMap, refractDir).rgb * uTint.rgb;
    // reflectColor = texture(uEnvironmentMap, reflectDir).rgb;

    // Absorption based on thickness (Beer's law approximation)
    float absorption = exp(-uThickness * (1.0 - uTint.a));
    refractColor *= absorption;

    // Mix refraction and reflection based on Fresnel
    vec3 color = mix(refractColor, reflectColor, fresnel);

    // Add specular highlight
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), 128.0);
    color += spec * uLightColor * 0.5;

    // Glass is semi-transparent
    float alpha = mix(0.1, 0.9, fresnel);

    FragColor = vec4(color, alpha);
}
