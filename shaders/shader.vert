#version 450

// The 3 triangle corners and their colors are hardcoded right here in the
// shader, indexed by gl_VertexIndex (0, 1, 2). This means main.cpp does NOT
// need a vertex buffer at all -- one less thing that can go wrong.
vec2 positions[3] = vec2[](
    vec2( 0.0, -0.5),   // top
    vec2( 0.5,  0.5),   // bottom right
    vec2(-0.5,  0.5)    // bottom left
);

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0), // red
    vec3(0.0, 1.0, 0.0), // green
    vec3(0.0, 0.0, 1.0)  // blue
);

// Passed to the fragment shader, interpolated across the triangle.
layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
}
