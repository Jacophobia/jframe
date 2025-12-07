#version 410 core

// Pixel Art Fragment Shader (Post-Process)
// Downsamples to create a pixelated/low-res retro effect
//
// Configurable Parameters:
// - uPixelSize: Size of pixels (default: 4.0)
// - uColorDepth: Bits per channel for color quantization (default: 5)
// - uDithering: Enable dithering for smoother gradients (default: true)

out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D uTexture;
uniform vec2 uResolution;
uniform float uPixelSize;
uniform int uColorDepth;
uniform bool uDithering;

// Bayer matrix for ordered dithering
const mat4 bayerMatrix = mat4(
    0.0,  8.0,  2.0, 10.0,
    12.0, 4.0, 14.0,  6.0,
    3.0, 11.0,  1.0,  9.0,
    15.0, 7.0, 13.0,  5.0
) / 16.0;

void main() {
    // Downsample to pixel grid
    vec2 pixelUV = floor(vTexCoord * uResolution / uPixelSize) * uPixelSize / uResolution;
    vec4 color = texture(uTexture, pixelUV);

    // Color quantization
    float levels = pow(2.0, float(uColorDepth));

    if (uDithering) {
        // Ordered dithering
        vec2 screenPos = gl_FragCoord.xy;
        int x = int(mod(screenPos.x, 4.0));
        int y = int(mod(screenPos.y, 4.0));
        float threshold = bayerMatrix[x][y];

        vec3 quantized = floor(color.rgb * levels + threshold) / levels;
        FragColor = vec4(quantized, color.a);
    } else {
        // Simple quantization
        vec3 quantized = floor(color.rgb * levels + 0.5) / levels;
        FragColor = vec4(quantized, color.a);
    }
}
