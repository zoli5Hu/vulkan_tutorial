#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_texCoord; // ÚJ: UV koordináta bemenet

layout(location = 0) out vec2 frag_texCoord; // Kimenet a frag shadernek

layout(push_constant) uniform PushConstants {
    layout(offset = 0) mat4 mvp;
} constants;

void main() {
    gl_Position = constants.mvp * vec4(in_position, 1.0f);
    frag_texCoord = in_texCoord; // Átadjuk a fragment shadernek
}