#version 410 core

// Gaussian Blur Post-Process Shader
// Configurable Parameters:
// - uBlurRadius: Blur kernel size (default: 5)
// - uBlurDirection: 0=horizontal, 1=vertical for two-pass blur

out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D uTexture;
uniform vec2 uResolution;
uniform int uBlurRadius;
uniform int uBlurDirection; // 0 = horizontal, 1 = vertical

// Gaussian weights for 5-tap kernel
const float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    vec2 texelSize = 1.0 / uResolution;
    vec2 direction = uBlurDirection == 0 ? vec2(1.0, 0.0) : vec2(0.0, 1.0);

    vec3 result = texture(uTexture, vTexCoord).rgb * weights[0];

    for (int i = 1; i < uBlurRadius && i < 5; i++) {
        vec2 offset = direction * texelSize * float(i);
        result += texture(uTexture, vTexCoord + offset).rgb * weights[i];
        result += texture(uTexture, vTexCoord - offset).rgb * weights[i];
    }

    FragColor = vec4(result, 1.0);
}
