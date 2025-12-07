// Clouds Shader - Procedural volumetric clouds
#version 410 core

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform vec3 uCameraPos;
uniform float uTime;

// Cloud parameters
uniform vec3 uCloudColor;        // Cloud color (default: white)
uniform vec3 uSkyColor;          // Sky background (default: blue)
uniform float uCloudSpeed;       // Cloud movement speed (default: 0.1)
uniform float uCloudDensity;     // Cloud density (default: 0.5)
uniform float uCloudScale;       // Cloud size scale (default: 3.0)
uniform vec3 uSunDirection;      // Sun direction for lighting (default: up)

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
    // View direction
    vec3 viewDir = normalize(vWorldPos - uCameraPos);

    // Only render clouds in upper hemisphere
    if (viewDir.y < 0.1) {
        FragColor = vec4(uSkyColor, 0.0);
        return;
    }

    // Cloud plane projection
    vec2 cloudUV = viewDir.xz / (viewDir.y + 0.1) * uCloudScale;
    cloudUV += uTime * uCloudSpeed;

    // Multi-octave cloud pattern
    float clouds = fbm(cloudUV);

    // Add more detail
    clouds += fbm(cloudUV * 3.0 + uTime * 0.05) * 0.3;

    // Cloud density threshold
    clouds = smoothstep(1.0 - uCloudDensity, 1.0, clouds);

    // Cloud lighting (brighter toward sun)
    float sunDot = dot(viewDir, normalize(uSunDirection));
    float lighting = 0.7 + sunDot * 0.3;

    // Cloud color with lighting
    vec3 color = uCloudColor * lighting;

    // Fade clouds near horizon
    float horizonFade = smoothstep(0.1, 0.3, viewDir.y);
    clouds *= horizonFade;

    // Mix with sky
    color = mix(uSkyColor, color, clouds);

    float alpha = clouds * 0.9;

    FragColor = vec4(color, alpha);
}
