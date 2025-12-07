#version 410 core

// Skybox Vertex Shader
// Renders skybox at infinite distance

layout(location = 0) in vec3 aPosition;

uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vTexCoord;

void main() {
    // Remove translation from view matrix (keep rotation only)
    mat4 rotationView = mat4(mat3(uView));

    vec4 pos = uProjection * rotationView * vec4(aPosition, 1.0);

    // Set Z = W so depth is always 1.0 (maximum depth)
    gl_Position = pos.xyww;

    // Use position as cubemap coordinate
    vTexCoord = aPosition;
}
