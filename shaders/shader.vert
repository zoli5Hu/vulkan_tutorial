#version 450

// Bemenet a C++ vertex bufferből
layout(location = 0) in vec3 in_position;

// Kimenet a fragment shader felé
layout(location = 0) out vec3 out_color;

// A push constant (ez már megvan)
layout(push_constant) uniform PushConstants {
    layout(offset = 0) mat4 mvp;
} constants;

// Hard-kódolt színek (a tanárod megoldása alapján)
const vec3 colors[3] = vec3[](
vec3(1.0f, 0.0f, 0.0f),
vec3(0.0f, 1.0f, 0.0f),
vec3(0.0f, 0.0f, 1.0f)
);

void main() {
    // A C++-ból kapott pozíciót használjuk
    gl_Position = constants.mvp * vec4(in_position, 1.0f);

    // A színt a vertex index alapján generáljuk
    out_color = colors[gl_VertexIndex % 3];
}