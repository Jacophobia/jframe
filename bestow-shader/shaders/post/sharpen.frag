#version 410 core

// Sharpen Post-Process Shader
// Enhances edges using unsharp mask
//
// Configurable Parameters:
// - uSharpness: Sharpening intensity (default: 1.0)

out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D uTexture;
uniform vec2 uResolution;
uniform float uSharpness;

void main() {
    vec2 texelSize = 1.0 / uResolution;

    // Sample neighbors
    vec3 center = texture(uTexture, vTexCoord).rgb;
    vec3 left = texture(uTexture, vTexCoord + vec2(-texelSize.x, 0.0)).rgb;
    vec3 right = texture(uTexture, vTexCoord + vec2(texelSize.x, 0.0)).rgb;
    vec3 up = texture(uTexture, vTexCoord + vec2(0.0, texelSize.y)).rgb;
    vec3 down = texture(uTexture, vTexCoord + vec2(0.0, -texelSize.y)).rgb;

    // Sharpen kernel
    vec3 result = center * (1.0 + 4.0 * uSharpness);
    result -= (left + right + up + down) * uSharpness;

    FragColor = vec4(result, 1.0);
}
