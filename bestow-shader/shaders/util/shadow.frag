#version 410 core

// Shadow Map Generation Fragment Shader
// Outputs depth for shadow mapping

// No output needed - depth is written automatically to depth buffer
// But we can write to gl_FragDepth if we need to modify it

void main() {
    // Depth is written automatically
    // For alpha-tested shadows, you could sample texture and discard here
}
