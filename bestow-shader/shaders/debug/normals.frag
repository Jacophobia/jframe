#version 410 core

// Normal Visualization Debug Shader
// Shows normals as RGB colors (X=Red, Y=Green, Z=Blue)

out vec4 FragColor;

in vec3 vNormal;

void main() {
    // Convert normal from [-1, 1] to [0, 1] for color
    vec3 normalColor = normalize(vNormal) * 0.5 + 0.5;
    FragColor = vec4(normalColor, 1.0);
}
