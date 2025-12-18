#version 450

layout(location = 0) in vec2 frag_texCoord;

layout(location = 0) out vec4 out_color;

// Binding 0: A textúra sampler
layout(binding = 0) uniform sampler2D texSampler;

void main() {
    // Minta vételezése a textúrából
    out_color = texture(texSampler, frag_texCoord);
}