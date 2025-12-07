#version 410 core

// Sepia Tone Post-Process Shader
// Classic sepia/old photo effect

out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D uTexture;
uniform float uIntensity; // 0 = color, 1 = full sepia

void main() {
    vec4 color = texture(uTexture, vTexCoord);

    // Sepia tone matrix
    vec3 sepia;
    sepia.r = dot(color.rgb, vec3(0.393, 0.769, 0.189));
    sepia.g = dot(color.rgb, vec3(0.349, 0.686, 0.168));
    sepia.b = dot(color.rgb, vec3(0.272, 0.534, 0.131));

    vec3 result = mix(color.rgb, sepia, uIntensity);

    FragColor = vec4(result, color.a);
}
