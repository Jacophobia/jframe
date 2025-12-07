#version 410 core

// Sci-Fi Force Field Fragment Shader
// Creates an energy shield with hexagonal pattern and intersection glow
//
// Configurable Parameters:
// - uFieldColor: Color of the force field (default: cyan)
// - uHexScale: Size of hexagons (default: 10.0)
// - uPulseSpeed: Animation speed (default: 2.0)
// - uIntersectionGlow: Brightness at edges (default: 2.0)

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform vec3 uCameraPos;
uniform vec4 uFieldColor;
uniform float uHexScale;
uniform float uPulseSpeed;
uniform float uIntersectionGlow;
uniform float uTime;

// Hexagonal grid pattern
float hexDist(vec2 p) {
    p = abs(p);
    float c = dot(p, normalize(vec2(1.0, 1.73)));
    c = max(c, p.x);
    return c;
}

vec4 hexCoord(vec2 p, float scale) {
    vec2 r = vec2(1.0, 1.73);
    vec2 h = r * 0.5;
    vec2 a = mod(p, r) - h;
    vec2 b = mod(p - h, r) - h;

    vec2 gv = length(a) < length(b) ? a : b;
    float x = atan(gv.x, gv.y);
    float y = 0.5 - hexDist(gv);
    vec2 id = p - gv;

    return vec4(x, y, id.x, id.y);
}

// Hash for noise
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);

    // Fresnel for edge glow
    float fresnel = pow(1.0 - abs(dot(V, N)), 2.0);

    // Hexagonal pattern
    vec2 hexUV = vWorldPos.xy * uHexScale;
    vec4 hex = hexCoord(hexUV, uHexScale);

    // Hex edge pattern
    float hexEdge = smoothstep(0.05, 0.1, hex.y);
    float hexPattern = 1.0 - hexEdge;

    // Animated pulse through hexagons
    float hexHash = hash(hex.zw);
    float pulse = sin(uTime * uPulseSpeed + hexHash * 6.28318) * 0.5 + 0.5;

    // Combine effects
    float intensity = (fresnel + hexPattern * pulse * 0.5) * (1.0 + fresnel * uIntersectionGlow);

    vec3 color = uFieldColor.rgb * intensity;
    float alpha = uFieldColor.a * (0.2 + fresnel * 0.8);

    FragColor = vec4(color, alpha);
}
