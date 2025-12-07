// Fire Shader - Procedural fire effect
#version 410 core

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform float uTime;

// Fire parameters
uniform vec3 uFireColorHot;      // Hot flame color (default: yellow-white)
uniform vec3 uFireColorMid;      // Mid flame color (default: orange)
uniform vec3 uFireColorCool;     // Cool flame color (default: red)
uniform float uFlameSpeed;       // Flame animation speed (default: 2.0)
uniform float uFlameHeight;      // Flame height (default: 1.0)
uniform float uTurbulence;       // Flame turbulence (default: 0.5)

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

float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < 5; i++) {
        value += amplitude * noise(p);
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

void main() {
    vec2 uv = vTexCoord;
    float t = uTime * uFlameSpeed;

    // Rising flame pattern
    vec2 flameUV = vec2(uv.x * 3.0, uv.y * uFlameHeight - t);

    // Turbulent noise
    float flame = fbm(flameUV + fbm(flameUV + t * 0.5) * uTurbulence);

    // Height falloff (flames are hotter at base)
    float heightFalloff = 1.0 - uv.y;
    flame *= heightFalloff;

    // Additional flickering
    flame += noise(uv * 10.0 + t * 5.0) * 0.1;

    // Map flame intensity to colors (hot -> mid -> cool)
    vec3 color;
    if (flame > 0.7) {
        color = mix(uFireColorMid, uFireColorHot, (flame - 0.7) / 0.3);
    } else if (flame > 0.3) {
        color = mix(uFireColorCool, uFireColorMid, (flame - 0.3) / 0.4);
    } else {
        color = uFireColorCool * (flame / 0.3);
    }

    // Brightness boost
    color *= 2.0;

    // Fade out at edges
    float alpha = smoothstep(0.0, 0.2, flame);

    FragColor = vec4(color, alpha);
}
