// Bloom Post-Process Shader - Glow/bloom effect
#version 410 core

out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D uSceneTexture;  // Scene color texture
uniform float uThreshold;         // Brightness threshold (default: 1.0)
uniform float uIntensity;         // Bloom intensity (default: 1.0)
uniform float uBlurSize;          // Blur size (default: 2.0)

// Simple box blur for bloom
vec3 sampleBlur(vec2 uv, float size) {
    vec3 color = vec3(0.0);
    float total = 0.0;

    for (float x = -size; x <= size; x += 1.0) {
        for (float y = -size; y <= size; y += 1.0) {
            vec2 offset = vec2(x, y) * 0.005;
            color += texture(uSceneTexture, uv + offset).rgb;
            total += 1.0;
        }
    }

    return color / total;
}

void main() {
    vec3 sceneColor = texture(uSceneTexture, vTexCoord).rgb;

    // Extract bright areas
    float brightness = dot(sceneColor, vec3(0.2126, 0.7152, 0.0722));
    vec3 bright = sceneColor * step(uThreshold, brightness);

    // Blur bright areas
    vec3 bloom = sampleBlur(vTexCoord, uBlurSize);

    // Combine with original
    vec3 color = sceneColor + bloom * bright * uIntensity;

    FragColor = vec4(color, 1.0);
}
