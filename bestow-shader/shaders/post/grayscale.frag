#version 410 core

// Grayscale Post-Process Shader
// Converts image to black and white

out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D uTexture;
uniform float uIntensity; // 0 = color, 1 = full grayscale

void main() {
    vec4 color = texture(uTexture, vTexCoord);

    // Luminance calculation (perceptually accurate)
    float gray = dot(color.rgb, vec3(0.299, 0.587, 0.114));

    vec3 result = mix(color.rgb, vec3(gray), uIntensity);

    FragColor = vec4(result, color.a);
}
