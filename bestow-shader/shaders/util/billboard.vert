#version 410 core

// Billboard/Sprite Vertex Shader
// Keeps quads always facing the camera

layout(location = 0) in vec3 aPosition;  // Center position
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aColor;
layout(location = 3) in vec2 aSize;      // Width, Height

uniform mat4 uView;
uniform mat4 uProjection;
uniform bool uSphericalBillboard; // true = face camera, false = cylindrical (Y-axis aligned)

out vec2 vTexCoord;
out vec4 vColor;

void main() {
    vec3 cameraRight;
    vec3 cameraUp;

    if (uSphericalBillboard) {
        // Fully spherical billboard (face camera completely)
        cameraRight = vec3(uView[0][0], uView[1][0], uView[2][0]);
        cameraUp = vec3(uView[0][1], uView[1][1], uView[2][1]);
    } else {
        // Cylindrical billboard (rotate only around Y axis)
        vec3 cameraForward = -vec3(uView[0][2], 0.0, uView[2][2]);
        cameraForward = normalize(cameraForward);
        cameraRight = cross(vec3(0, 1, 0), cameraForward);
        cameraUp = vec3(0, 1, 0);
    }

    // Create billboard quad vertices
    vec2 quadOffset = (aTexCoord - 0.5) * aSize;
    vec3 worldPos = aPosition + cameraRight * quadOffset.x + cameraUp * quadOffset.y;

    vTexCoord = aTexCoord;
    vColor = aColor;

    gl_Position = uProjection * uView * vec4(worldPos, 1.0);
}
