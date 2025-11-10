#version 450

// Bemenet a vertex shaderből
layout(location = 0) in vec3 in_color;

// Kimenet a framebufferbe
layout(location = 0) out vec4 out_color;

void main() {
    out_color = vec4(in_color, 1.0f);
}