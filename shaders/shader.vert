#version 450

layout(location = 0) out vec3 fragColor;

// ÚJ: Definiáljuk a push constant blokkot, ami fogadja a C++ adatot
layout(push_constant) uniform PushConstants {
    layout(offset = 0) mat4 mvp; // Model-View-Projection mátrix
} constants;

// A te meglévő, hard-kódolt pozícióid
vec2 positions[3] = vec2[](
vec2(0.0, -0.5),
vec2(0.5, 0.5),
vec2(-0.5, 0.5)
);

// A te meglévő, hard-kódolt színeid
vec3 colors[3] = vec3[](
vec3(1.0, 0.0, 0.0),
vec3(0.0, 1.0, 0.0),
vec3(0.0, 0.0, 1.0)
);

void main() {
    // MÓDOSÍTVA: Szorozzuk be a pozíciót a C++-ból kapott mvp mátrixszal
    // A 'positions'-ból kapott 2D pozíciót (vec2) kiegészítjük Z=0.0 és W=1.0 értékekkel,
    // hogy 4D vektor (vec4) legyen, amit a mat4 mátrixszal szorozhatunk.
    gl_Position = constants.mvp * vec4(positions[gl_VertexIndex], 0.0, 1.0);

    // Ez marad a régi
    fragColor = colors[gl_VertexIndex];
}