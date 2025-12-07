// Skybox Shader - Procedural sky gradient
#version 410 core

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform vec3 uCameraPos;
uniform float uTime;

// Sky parameters
uniform vec3 uSkyColorTop;       // Top sky color (default: blue)
uniform vec3 uSkyColorHorizon;   // Horizon color (default: light blue)
uniform vec3 uGroundColor;       // Ground color (default: gray)
uniform vec3 uSunDirection;      // Sun direction (default: up)
uniform vec3 uSunColor;          // Sun color (default: yellow)
uniform float uSunSize;          // Sun size (default: 0.05)
uniform bool uShowStars;         // Show stars (default: true)

float hash(vec3 p) {
    return fract(sin(dot(p, vec3(127.1, 311.7, 74.7))) * 43758.5453);
}

void main() {
    // Direction from camera
    vec3 viewDir = normalize(vWorldPos - uCameraPos);

    // Sky gradient based on vertical direction
    float skyGradient = viewDir.y;

    vec3 color;
    if (skyGradient > 0.0) {
        // Sky
        color = mix(uSkyColorHorizon, uSkyColorTop, pow(skyGradient, 0.5));
    } else {
        // Ground
        color = mix(uSkyColorHorizon, uGroundColor, pow(-skyGradient, 0.5));
    }

    // Sun
    float sunDot = dot(viewDir, normalize(uSunDirection));
    float sun = smoothstep(1.0 - uSunSize, 1.0 - uSunSize * 0.5, sunDot);
    color = mix(color, uSunColor, sun);

    // Sun glow
    float sunGlow = pow(max(sunDot, 0.0), 8.0) * 0.3;
    color += uSunColor * sunGlow;

    // Stars (only visible in upper hemisphere)
    if (uShowStars && skyGradient > 0.0) {
        vec3 starDir = viewDir * 100.0;
        float star = hash(floor(starDir));
        star = step(0.998, star);
        star *= pow(skyGradient, 2.0); // Fade near horizon
        color += vec3(star);
    }

    FragColor = vec4(color, 1.0);
}
