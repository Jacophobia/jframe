#version 410 core

// Outline/Silhouette Vertex Shader
// Extrudes vertices along normals for outline effect

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform float uOutlineWidth;

void main() {
    // Extrude along normal
    vec3 extrudedPos = aPosition + aNormal * uOutlineWidth;
    gl_Position = uProjection * uView * uModel * vec4(extrudedPos, 1.0);
}
