#version 410 core

// GPU Instancing Vertex Shader
// Renders multiple instances with per-instance transforms

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec4 aColor;

// Per-instance data (requires glVertexAttribDivisor)
layout(location = 4) in mat4 aInstanceTransform;  // Columns at 4,5,6,7
layout(location = 8) in vec4 aInstanceColor;

uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vTexCoord;
out vec4 vColor;

void main() {
    vec4 worldPos = aInstanceTransform * vec4(aPosition, 1.0);
    vWorldPos = worldPos.xyz;

    // Extract normal matrix from instance transform
    mat3 normalMatrix = mat3(aInstanceTransform);
    vNormal = normalize(normalMatrix * aNormal);

    vTexCoord = aTexCoord;
    vColor = aColor * aInstanceColor;

    gl_Position = uProjection * uView * worldPos;
}
