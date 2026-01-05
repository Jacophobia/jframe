#version 450

// Star/Sun rendering fragment shader
// Renders stars with blackbody color, limb darkening, and corona

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in float fragLogDepth;
layout(location = 4) in vec3 fragLocalPos;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 viewProjection;
    vec4 starParams;        // x = radius, y = temperature (Kelvin), z = luminosity, w = time
    vec4 cameraPos;
    vec4 depthParams;       // x = logDepthCoeff, y = farPlane, z = 1/log(C*far+1), w = useLogDepth
    vec4 coronaParams;      // x = corona size, y = corona intensity, z = limb darkening, w = unused
} pc;

// Convert temperature (Kelvin) to RGB color (blackbody radiation)
vec3 temperatureToRGB(float temperature) {
    // Algorithm based on approximation by Tanner Helland
    float temp = clamp(temperature, 1000.0, 40000.0) / 100.0;

    vec3 color;

    // Red
    if (temp <= 66.0) {
        color.r = 1.0;
    } else {
        color.r = 1.29293618606 * pow(temp - 60.0, -0.1332047592);
    }

    // Green
    if (temp <= 66.0) {
        color.g = 0.390081578769 * log(temp) - 0.631841443789;
    } else {
        color.g = 1.12989086089 * pow(temp - 60.0, -0.0755148492);
    }

    // Blue
    if (temp >= 66.0) {
        color.b = 1.0;
    } else if (temp <= 19.0) {
        color.b = 0.0;
    } else {
        color.b = 0.543206789110 * log(temp - 10.0) - 1.19625408914;
    }

    return clamp(color, 0.0, 1.0);
}

// Procedural noise for surface detail
float hash(vec3 p) {
    p = fract(p * vec3(443.897, 441.423, 437.195));
    p += dot(p, p.yxz + 19.19);
    return fract((p.x + p.y) * p.z);
}

float noise(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    return mix(
        mix(mix(hash(i + vec3(0,0,0)), hash(i + vec3(1,0,0)), f.x),
            mix(hash(i + vec3(0,1,0)), hash(i + vec3(1,1,0)), f.x), f.y),
        mix(mix(hash(i + vec3(0,0,1)), hash(i + vec3(1,0,1)), f.x),
            mix(hash(i + vec3(0,1,1)), hash(i + vec3(1,1,1)), f.x), f.y),
        f.z);
}

float fbm(vec3 p) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;

    for (int i = 0; i < 4; i++) {
        value += amplitude * noise(p * frequency);
        amplitude *= 0.5;
        frequency *= 2.0;
    }

    return value;
}

void main() {
    float temperature = pc.starParams.y;
    float luminosity = pc.starParams.z;
    float time = pc.starParams.w;

    // Base star color from temperature
    vec3 starColor = temperatureToRGB(temperature);

    // View direction
    vec3 viewDir = normalize(pc.cameraPos.xyz - fragWorldPos);
    vec3 N = normalize(fragNormal);

    // Limb darkening (edge of star appears darker)
    float NdotV = max(dot(N, viewDir), 0.0);
    float limbDarkening = pow(NdotV, pc.coronaParams.z);

    // Surface detail (convection cells, granulation)
    vec3 animatedPos = fragLocalPos * 5.0 + vec3(0.0, 0.0, time * 0.1);
    float surfaceDetail = fbm(animatedPos);
    surfaceDetail = surfaceDetail * 0.15 + 0.85;

    // Combine
    vec3 finalColor = starColor * limbDarkening * surfaceDetail * luminosity;

    // Corona effect at edges
    float corona = 1.0 - NdotV;
    corona = pow(corona, 2.0) * pc.coronaParams.y;
    finalColor += starColor * corona;

    // HDR output (bloom post-process will handle the glow)
    outColor = vec4(finalColor, 1.0);

    // Logarithmic depth
    if (pc.depthParams.w > 0.5) {
        gl_FragDepth = 1.0 - fragLogDepth;
    } else {
        gl_FragDepth = gl_FragCoord.z;
    }
}
