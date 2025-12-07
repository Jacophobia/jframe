// Posterize Shader - Limited color palette effect
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

// Posterize parameters
uniform int uLevels;             // Color levels per channel (default: 4)
uniform float uEdgeStrength;     // Edge detection strength (default: 0.2)
uniform vec3 uEdgeColor;         // Edge color (default: black)
uniform bool uQuantizeHSV;       // Quantize in HSV space (default: false)

// RGB to HSV conversion
vec3 rgb2hsv(vec3 c) {
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

// HSV to RGB conversion
vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 posterize(vec3 color, int levels) {
    if (uQuantizeHSV) {
        vec3 hsv = rgb2hsv(color);
        hsv.x = floor(hsv.x * float(levels)) / float(levels);
        hsv.y = floor(hsv.y * float(levels)) / float(levels);
        hsv.z = floor(hsv.z * float(levels)) / float(levels);
        return hsv2rgb(hsv);
    } else {
        float step = 1.0 / float(levels - 1);
        return floor(color / step + 0.5) * step;
    }
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uLightDir);

    // Calculate lighting
    float NdotL = max(dot(N, L), 0.0);

    // Apply lighting
    vec3 baseColor = uBaseColor.rgb;
    vec3 lit = baseColor * (uLightColor * NdotL + uAmbientColor);

    // Posterize the color
    vec3 color = posterize(lit, max(uLevels, 2));

    // Edge detection using normal discontinuity
    float edge = length(fwidth(N)) * 10.0;
    edge = smoothstep(0.0, uEdgeStrength, edge);

    // Apply edge color
    color = mix(color, uEdgeColor, edge);

    FragColor = vec4(color, uBaseColor.a);
}
