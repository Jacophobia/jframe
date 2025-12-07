// Depth of Field Post-Process Shader - Bokeh blur
#version 410 core

out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D uSceneTexture;  // Scene color texture
uniform sampler2D uDepthTexture;  // Depth texture
uniform float uFocusDistance;     // Focus distance (default: 10.0)
uniform float uFocusRange;        // Focus range (default: 5.0)
uniform float uBokehSize;         // Bokeh blur size (default: 3.0)

void main() {
    vec3 sceneColor = texture(uSceneTexture, vTexCoord).rgb;
    float depth = texture(uDepthTexture, vTexCoord).r;

    // Convert depth to linear distance
    float distance = depth * 100.0; // Simplified

    // Calculate blur amount based on distance from focus
    float focusDiff = abs(distance - uFocusDistance);
    float blurAmount = smoothstep(0.0, uFocusRange, focusDiff);

    // Simple bokeh blur
    vec3 blur = vec3(0.0);
    float total = 0.0;
    float size = uBokehSize * blurAmount;

    for (float angle = 0.0; angle < 6.28; angle += 0.5) {
        for (float radius = 0.0; radius < size; radius += 1.0) {
            vec2 offset = vec2(cos(angle), sin(angle)) * radius * 0.005;
            blur += texture(uSceneTexture, vTexCoord + offset).rgb;
            total += 1.0;
        }
    }

    blur /= total;

    // Mix focused and blurred
    vec3 color = mix(sceneColor, blur, blurAmount);

    FragColor = vec4(color, 1.0);
}
