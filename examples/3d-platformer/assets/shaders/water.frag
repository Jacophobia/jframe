// Water Fragment Shader - reflective, transparent water surface
#version 330 core

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform vec3 uCameraPos;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform float uTime;

// Water parameters
uniform vec4 uWaterColor;      // Water color (default: 0.1, 0.3, 0.5, 0.7)
uniform vec4 uDeepColor;       // Deep water color (default: 0.0, 0.1, 0.2, 0.9)
uniform float uFresnelPower;   // Fresnel intensity (default: 2.0)
uniform float uSpecularPower;  // Specular highlight sharpness (default: 64.0)
uniform float uFoamThreshold;  // Wave height for foam (default: 0.8)

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 L = normalize(-uLightDir);
    vec3 H = normalize(V + L);

    // Fresnel effect - more reflective at grazing angles
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), uFresnelPower);

    // Specular highlight
    float NdotH = max(dot(N, H), 0.0);
    float specular = pow(NdotH, uSpecularPower);

    // Fake caustics using animated noise
    float caustic = sin(vWorldPos.x * 3.0 + uTime * 2.0) *
                    sin(vWorldPos.z * 3.0 + uTime * 1.5) * 0.5 + 0.5;
    caustic = smoothstep(0.4, 0.6, caustic) * 0.2;

    // Mix water colors based on view angle (deeper = darker when looking down)
    vec4 waterColor = mix(uWaterColor, uDeepColor, 1.0 - fresnel);

    // Add foam at wave peaks (based on normal deviation)
    float foam = smoothstep(uFoamThreshold, 1.0, abs(N.x) + abs(N.z));

    // Combine colors
    vec3 color = waterColor.rgb;
    color += caustic * vec3(0.1, 0.2, 0.3);  // Caustics
    color += specular * uLightColor;          // Sun reflection
    color += fresnel * vec3(0.3, 0.5, 0.7);  // Sky reflection (simplified)
    color = mix(color, vec3(1.0), foam * 0.5);  // Foam

    // Transparency varies with fresnel
    float alpha = mix(waterColor.a, 0.95, fresnel);

    FragColor = vec4(color, alpha);
}
