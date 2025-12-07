#version 410 core

// Shadow Map Generation Vertex Shader
// Renders from light's perspective for shadow mapping

layout(location = 0) in vec3 aPosition;

uniform mat4 uLightSpaceMatrix; // Light's view-projection matrix
uniform mat4 uModel;

void main() {
    gl_Position = uLightSpaceMatrix * uModel * vec4(aPosition, 1.0);
}
