#version 450

// --- BEMENETEK (Input) ---
// Az árnyékoláshoz csak a geometria alakja (pozíciója) számít.
// A színek, normálok és UV koordináták itt nem szükségesek.
layout(location = 0) in vec3 inPosition; // Helyi koordináta (X, Y, Z)

// --- PUSH CONSTANTS ---
// Gyors adatátvitel a CPU-ról.
// Ez a mátrix tartalmazza a teljes transzformációs láncot a fény szemszögéből:
// lightMVP = LightProjection * LightView * Model
layout(push_constant) uniform PushConstants {
    mat4 lightMVP;
} push;

void main() {
    // A csúcspont transzformálása a fény "Clip Space" terébe.
    // A végeredmény Z komponense fogja reprezentálni a mélységet az árnyéktérképen.
    gl_Position = push.lightMVP * vec4(inPosition, 1.0);
}