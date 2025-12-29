#version 450

// Atmospheric scattering fragment shader
// Implements Rayleigh and Mie scattering for realistic atmospheric rendering
// Based on GPU Gems 2 "Accurate Atmospheric Scattering" by Sean O'Neil

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragViewDir;
layout(location = 2) in float fragLogDepth;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 viewProjection;
    vec4 planetCenter;      // xyz = world position, w = planet radius
    vec4 atmosphereParams;  // x = atmosphere radius, y = scale height Rayleigh, z = scale height Mie, w = unused
    vec4 cameraPos;         // xyz = camera world position, w = unused
    vec4 sunDir;            // xyz = direction to sun (normalized), w = sun intensity
    vec4 depthParams;       // x = logDepthCoeff, y = farPlane, z = 1/log(C*far+1), w = useLogDepth
    vec4 scatterCoeffs;     // xyz = Rayleigh coefficients (wavelength dependent), w = Mie coefficient
    vec4 mieParams;         // x = Mie g (anisotropy), y = Mie scale, z,w = unused
} pc;

const float PI = 3.14159265359;
const int NUM_SAMPLES = 8;
const int NUM_LIGHT_SAMPLES = 4;

// Rayleigh phase function
float rayleighPhase(float cosTheta) {
    return 3.0 / (16.0 * PI) * (1.0 + cosTheta * cosTheta);
}

// Mie phase function (Henyey-Greenstein)
float miePhase(float cosTheta, float g) {
    float g2 = g * g;
    return 3.0 / (8.0 * PI) * ((1.0 - g2) * (1.0 + cosTheta * cosTheta)) /
           ((2.0 + g2) * pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5));
}

// Density at height (exponential falloff)
float densityAtHeight(float height, float scaleHeight) {
    return exp(-height / scaleHeight);
}

// Ray-sphere intersection (returns distances to near and far intersections)
vec2 raySphereIntersect(vec3 rayOrigin, vec3 rayDir, vec3 sphereCenter, float sphereRadius) {
    vec3 oc = rayOrigin - sphereCenter;
    float b = dot(oc, rayDir);
    float c = dot(oc, oc) - sphereRadius * sphereRadius;
    float discriminant = b * b - c;

    if (discriminant < 0.0) {
        return vec2(-1.0, -1.0);
    }

    float sqrtD = sqrt(discriminant);
    return vec2(-b - sqrtD, -b + sqrtD);
}

// Optical depth along a ray segment
float opticalDepth(vec3 rayOrigin, vec3 rayDir, float rayLength, float scaleHeight, vec3 planetCenter, float planetRadius) {
    float stepSize = rayLength / float(NUM_LIGHT_SAMPLES);
    float opticalDepthSum = 0.0;

    for (int i = 0; i < NUM_LIGHT_SAMPLES; i++) {
        vec3 samplePoint = rayOrigin + rayDir * (stepSize * (float(i) + 0.5));
        float height = length(samplePoint - planetCenter) - planetRadius;
        opticalDepthSum += densityAtHeight(max(height, 0.0), scaleHeight) * stepSize;
    }

    return opticalDepthSum;
}

void main() {
    vec3 planetCenter = pc.planetCenter.xyz;
    float planetRadius = pc.planetCenter.w;
    float atmosphereRadius = pc.atmosphereParams.x;
    float scaleHeightR = pc.atmosphereParams.y;
    float scaleHeightM = pc.atmosphereParams.z;
    vec3 sunDirection = normalize(pc.sunDir.xyz);
    float sunIntensity = pc.sunDir.w;

    vec3 cameraPosition = pc.cameraPos.xyz;
    vec3 viewDir = normalize(fragWorldPos - cameraPosition);

    // Ray-atmosphere intersection
    vec2 atmosphereHit = raySphereIntersect(cameraPosition, viewDir, planetCenter, atmosphereRadius);

    if (atmosphereHit.x < 0.0 && atmosphereHit.y < 0.0) {
        discard;
    }

    // Clamp ray to atmosphere bounds
    float rayStart = max(atmosphereHit.x, 0.0);
    float rayEnd = atmosphereHit.y;

    // Check for planet intersection (ray blocked by solid surface)
    vec2 planetHit = raySphereIntersect(cameraPosition, viewDir, planetCenter, planetRadius);
    if (planetHit.x > 0.0) {
        rayEnd = min(rayEnd, planetHit.x);
    }

    float rayLength = rayEnd - rayStart;
    if (rayLength <= 0.0) {
        discard;
    }

    // Scattering coefficients
    vec3 rayleighCoeff = pc.scatterCoeffs.xyz;
    float mieCoeff = pc.scatterCoeffs.w * pc.mieParams.y;
    float mieG = pc.mieParams.x;

    // Integrate scattering along view ray
    float stepSize = rayLength / float(NUM_SAMPLES);
    vec3 rayleighSum = vec3(0.0);
    vec3 mieSum = vec3(0.0);
    float opticalDepthR = 0.0;
    float opticalDepthM = 0.0;

    for (int i = 0; i < NUM_SAMPLES; i++) {
        vec3 samplePoint = cameraPosition + viewDir * (rayStart + stepSize * (float(i) + 0.5));
        float height = length(samplePoint - planetCenter) - planetRadius;

        // Density at sample point
        float densityR = densityAtHeight(height, scaleHeightR);
        float densityM = densityAtHeight(height, scaleHeightM);

        // Optical depth from sample to sun
        vec2 sunHit = raySphereIntersect(samplePoint, sunDirection, planetCenter, atmosphereRadius);
        float sunRayLength = sunHit.y;

        // Check if sun ray hits planet (in shadow)
        vec2 sunPlanetHit = raySphereIntersect(samplePoint, sunDirection, planetCenter, planetRadius);
        if (sunPlanetHit.x > 0.0 && sunPlanetHit.x < sunRayLength) {
            // In planet's shadow - skip this sample
            opticalDepthR += densityR * stepSize;
            opticalDepthM += densityM * stepSize;
            continue;
        }

        float sunOpticalDepthR = opticalDepth(samplePoint, sunDirection, sunRayLength, scaleHeightR, planetCenter, planetRadius);
        float sunOpticalDepthM = opticalDepth(samplePoint, sunDirection, sunRayLength, scaleHeightM, planetCenter, planetRadius);

        // Total optical depth (view ray + sun ray)
        vec3 tau = rayleighCoeff * (opticalDepthR + sunOpticalDepthR) +
                   mieCoeff * (opticalDepthM + sunOpticalDepthM);
        vec3 attenuation = exp(-tau);

        // Accumulate scattering
        rayleighSum += densityR * attenuation * stepSize;
        mieSum += densityM * attenuation * stepSize;

        // Accumulate optical depth along view ray
        opticalDepthR += densityR * stepSize;
        opticalDepthM += densityM * stepSize;
    }

    // Phase functions
    float cosTheta = dot(viewDir, sunDirection);
    float phaseR = rayleighPhase(cosTheta);
    float phaseM = miePhase(cosTheta, mieG);

    // Final color
    vec3 rayleighColor = rayleighSum * rayleighCoeff * phaseR;
    vec3 mieColor = mieSum * mieCoeff * phaseM;
    vec3 scatterColor = (rayleighColor + mieColor) * sunIntensity;

    // Apply exposure/tone mapping
    scatterColor = 1.0 - exp(-scatterColor);

    outColor = vec4(scatterColor, max(max(scatterColor.r, scatterColor.g), scatterColor.b));

    // Logarithmic depth
    if (pc.depthParams.w > 0.5) {
        gl_FragDepth = 1.0 - fragLogDepth;
    } else {
        gl_FragDepth = gl_FragCoord.z;
    }
}
