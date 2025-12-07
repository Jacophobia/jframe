#version 410 core

// Bloom Post-Process Shader
// Extracts bright areas and blurs them for glow effect
//
// Configurable Parameters:
// - uThreshold: Brightness threshold for bloom (default: 1.0)
// - uIntensity: Bloom intensity (default: 1.0)
// - uBloomTexture: Pre-blurred bright areas (pass 2)

out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D uTexture;
uniform sampler2D uBloomTexture; // Blurred bright-pass texture
uniform float uThreshold;
uniform float uIntensity;
uniform bool uExtractBrightPass; // true for pass 1, false for combine pass

void main() {
    vec4 color = texture(uTexture, vTexCoord);

    if (uExtractBrightPass) {
        // Pass 1: Extract bright areas
        float brightness = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
        if (brightness > uThreshold) {
            FragColor = color;
        } else {
            FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        }
    } else {
        // Pass 2: Combine original with blurred bloom
        vec3 bloom = texture(uBloomTexture, vTexCoord).rgb;
        vec3 result = color.rgb + bloom * uIntensity;
        FragColor = vec4(result, color.a);
    }
}
