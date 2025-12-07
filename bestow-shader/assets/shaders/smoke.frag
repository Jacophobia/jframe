// Smoke Shader - Volumetric smoke effect
#version 410 core

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform vec3 uAmbientColor;
uniform float uTime;

// Smoke parameters
uniform vec3 uSmokeColor;        // Smoke color (default: gray)
uniform float uDensity;          // Smoke density (default: 0.5)
uniform float uRiseSpeed;        // Rising speed (default: 0.5)
uniform float uTurbulence;       // Turbulence amount (default: 0.6)
uniform float uDissipation;      // Dissipation rate (default: 0.5)

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
    for (int i = 0; i < 4; i++) {
        value += amplitude * noise(p);
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

void main() {
    vec2 uv = vTexCoord;
    float t = uTime * uRiseSpeed;

    // Rising smoke
    vec2 smokeUV = vec2(uv.x * 2.0, uv.y - t * 0.5);

    // Turbulent swirls
    vec2 turbulentOffset = vec2(
        fbm(smokeUV * 2.0 + t) * uTurbulence,
        fbm(smokeUV * 2.0 - t + 100.0) * uTurbulence
    );

    smokeUV += turbulentOffset;

    // Smoke density
    float smoke = fbm(smokeUV);
    smoke *= uDensity;

    // Dissipate with height
    float dissipate = 1.0 - pow(uv.y, 1.0 / (1.0 - uDissipation + 0.1));
    smoke *= dissipate;

    // Wispy edges
    smoke = smoothstep(0.2, 0.6, smoke);

    // Color with ambient influence
    vec3 color = uSmokeColor + uAmbientColor * 0.2;

    float alpha = smoke * 0.7;

    FragColor = vec4(color, alpha);
}
