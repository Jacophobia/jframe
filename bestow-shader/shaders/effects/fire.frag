#version 410 core

// Procedural Fire Fragment Shader
// Creates animated fire effect using noise
//
// Configurable Parameters:
// - uFireSpeed: Speed of flame animation (default: 1.0)
// - uFireIntensity: Brightness of flames (default: 1.5)
// - uFlameHeight: How high flames reach (default: 1.0)

out vec4 FragColor;

in vec2 vTexCoord;
in vec4 vColor;

uniform float uTime;
uniform float uFireSpeed;
uniform float uFireIntensity;
uniform float uFlameHeight;

// Hash function
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

// 2D Noise
float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

// Fractal Brownian Motion
float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < 6; i++) {
        value += amplitude * noise(p);
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

void main() {
    vec2 uv = vTexCoord;

    // Animate upward
    vec2 q = vec2(uv.x, uv.y - uTime * uFireSpeed * 0.5);

    // Multiple octaves of noise for turbulent flames
    float n = fbm(q * 3.0);
    n += fbm(q * 6.0 + vec2(0.0, -uTime * uFireSpeed)) * 0.5;

    // Flame shape (hotter at bottom, dissipates upward)
    float flame = n * (1.0 - uv.y * uFlameHeight);

    // Color gradient (yellow-orange-red-dark)
    vec3 color;
    if (flame > 0.6) {
        // Hot yellow-white
        color = mix(vec3(1.0, 0.9, 0.3), vec3(1.0, 1.0, 0.9), (flame - 0.6) * 2.5);
    } else if (flame > 0.3) {
        // Orange
        color = mix(vec3(1.0, 0.3, 0.0), vec3(1.0, 0.9, 0.3), (flame - 0.3) * 3.33);
    } else {
        // Red-dark
        color = mix(vec3(0.1, 0.0, 0.0), vec3(1.0, 0.3, 0.0), flame * 3.33);
    }

    color *= uFireIntensity;

    // Alpha based on flame intensity
    float alpha = smoothstep(0.0, 0.5, flame);

    FragColor = vec4(color * vColor.rgb, alpha * vColor.a);
}
