#version 450

// Interpolated color coming in from the vertex shader.
layout(location = 0) in vec3 fragColor;

// Final pixel color written to the framebuffer.
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
