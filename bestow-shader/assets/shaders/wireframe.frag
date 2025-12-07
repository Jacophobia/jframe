// Wireframe Shader - Wireframe overlay visualization
#version 410 core

out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

uniform vec4 uBaseColor;
uniform vec3 uWireColor;         // Wireframe color (default: green)
uniform float uWireThickness;    // Wire thickness (default: 0.02)
uniform bool uShowSolid;         // Show solid fill (default: false)

void main() {
    // Use texture coordinates to create wireframe
    vec2 coord = fract(vTexCoord * 10.0);

    // Distance to nearest edge
    float edge = min(min(coord.x, 1.0 - coord.x), min(coord.y, 1.0 - coord.y));

    // Create wire lines
    float wire = smoothstep(uWireThickness, uWireThickness * 0.5, edge);

    vec3 color;
    if (uShowSolid) {
        color = mix(uBaseColor.rgb, uWireColor, wire);
    } else {
        color = uWireColor * wire;
    }

    float alpha = uShowSolid ? 1.0 : wire;

    FragColor = vec4(color, alpha);
}
