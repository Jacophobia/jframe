#version 410 core

in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec4 uTint;

void main() {
    vec4 texColor = texture(uTexture, vTexCoord);
    FragColor = texColor * uTint;
}
