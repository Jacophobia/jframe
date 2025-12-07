// Force Field Shader - Energy shield effect
#version 410 core

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform vec4 uBaseColor;
uniform vec3 uCameraPos;
uniform float uTime;

// Force field parameters
uniform vec3 uFieldColor;        // Field color (default: blue)
uniform float uHexSize;          // Hexagon grid size (default: 0.3)
uniform float uPulseSpeed;       // Energy pulse speed (default: 1.5)
uniform float uImpactX;          // Impact position X (default: 0.0)
uniform float uImpactY;          // Impact position Y (default: 0.0)
uniform float uImpactStrength;   // Impact ripple strength (default: 0.0)

float hexDist(vec2 p) {
    p = abs(p);
    float c = dot(p, normalize(vec2(1.0, 1.73)));
    return max(c, p.x);
}

vec2 hexCoord(vec2 p) {
    vec2 r = vec2(1.0, 1.73);
    vec2 h = r * 0.5;
    vec2 a = mod(p, r) - h;
    vec2 b = mod(p - h, r) - h;
    return length(a) < length(b) ? a : b;
}

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);

    // Hexagonal grid
    vec2 hexUV = vWorldPos.xy / uHexSize;
    vec2 hexOffset = hexCoord(hexUV);
    float hexPattern = 1.0 - smoothstep(0.4, 0.5, hexDist(hexOffset));

    // Energy pulse along grid
    float pulse = sin(uTime * uPulseSpeed + hash(floor(hexUV)) * 6.28) * 0.5 + 0.5;

    // Impact ripple
    vec2 impactPos = vec2(uImpactX, uImpactY);
    float impactDist = length(vWorldPos.xy - impactPos);
    float ripple = sin(impactDist * 10.0 - uTime * 5.0) * 0.5 + 0.5;
    ripple *= smoothstep(2.0, 0.0, impactDist) * uImpactStrength;

    // Fresnel (shield is more visible at edges)
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 4.0);

    // Combine effects
    vec3 color = uFieldColor;
    color *= hexPattern * 0.5 + 0.5;
    color *= pulse * 0.3 + 0.7;
    color += ripple * uFieldColor * 2.0;
    color += fresnel * uFieldColor * 1.5;

    // Transparency (only visible at edges and impacts)
    float alpha = fresnel * 0.7 + hexPattern * 0.2 + ripple;
    alpha = clamp(alpha, 0.0, 0.9);

    FragColor = vec4(color, alpha);
}
