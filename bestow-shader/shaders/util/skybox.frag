#version 410 core

// Skybox Fragment Shader
// Samples from cubemap and applies tone mapping

out vec4 FragColor;

in vec3 vTexCoord;

uniform samplerCube uSkybox;
uniform float uExposure;
uniform bool uApplyToneMapping;

void main() {
    vec3 color = texture(uSkybox, vTexCoord).rgb;

    // Apply exposure
    color *= uExposure;

    if (uApplyToneMapping) {
        // Reinhard tone mapping
        color = color / (color + vec3(1.0));

        // Gamma correction
        color = pow(color, vec3(1.0 / 2.2));
    }

    FragColor = vec4(color, 1.0);
}
