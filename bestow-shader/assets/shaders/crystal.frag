// Crystal Shader - Crystalline gem with dispersion
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

// Crystal parameters
uniform vec3 uCrystalColor;      // Crystal color (default: white)
uniform float uRefractiveIndex;  // IOR (default: 2.4)
uniform float uDispersion;       // Color dispersion (default: 0.3)
uniform float uFacets;           // Facet sharpness (default: 8.0)
uniform float uTransparency;     // Transparency (default: 0.4)

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(hash(i), hash(i + vec2(1.0, 0.0)), f.x),
        mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), f.x),
        f.y
    );
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 L = normalize(-uLightDir);
    vec3 H = normalize(V + L);

    // Faceted normal (quantize normal for crystal facets)
    vec3 facetNormal = normalize(floor(N * uFacets) / uFacets);

    // High Fresnel for crystal
    vec3 F0 = vec3(0.2);
    vec3 F = fresnelSchlick(max(dot(facetNormal, V), 0.0), F0);

    // Chromatic dispersion (different IOR for RGB)
    float etaR = 1.0 / (uRefractiveIndex - uDispersion * 0.1);
    float etaG = 1.0 / uRefractiveIndex;
    float etaB = 1.0 / (uRefractiveIndex + uDispersion * 0.1);

    vec3 refractR = refract(-V, facetNormal, etaR);
    vec3 refractG = refract(-V, facetNormal, etaG);
    vec3 refractB = refract(-V, facetNormal, etaB);

    // Simulate refracted color (simplified)
    vec3 refraction = vec3(
        length(refractR) > 0.0 ? 1.0 : 0.5,
        length(refractG) > 0.0 ? 1.0 : 0.5,
        length(refractB) > 0.0 ? 1.0 : 0.5
    ) * uCrystalColor;

    // Internal reflections
    vec2 uv = vTexCoord * 5.0;
    float internal = noise(uv) * 0.3 + noise(uv * 2.0) * 0.2;

    // Reflection
    vec3 reflection = uAmbientColor * 0.8;

    // Sharp specular highlights
    float NdotH = max(dot(facetNormal, H), 0.0);
    float spec = pow(NdotH, 256.0);
    vec3 specular = uLightColor * spec;

    // Combine
    vec3 color = mix(refraction, reflection, F.r);
    color += specular * 0.8;
    color += internal * uCrystalColor * 0.2;

    // Sparkle
    float sparkle = pow(noise(uv * 20.0), 10.0);
    color += vec3(sparkle) * 2.0;

    float alpha = mix(uTransparency, 1.0, F.r);

    FragColor = vec4(color, alpha);
}
