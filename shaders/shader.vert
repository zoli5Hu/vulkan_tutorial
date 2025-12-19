#version 450

// --- BEMENETEK (Vertex Attributes) ---
// Ezek az adatok jönnek a CPU-ról a Vertex Bufferből (MeshObject.cpp).
layout(location = 0) in vec3 inPosition; // Helyi koordináta (X, Y, Z)
layout(location = 1) in vec3 inNormal;   // Normálvektor
layout(location = 2) in vec2 inTexCoord; // UV koordináta
// ÚJ: Tangens vektor (Normal Mapping-hez szükséges TBN mátrix alapja)
layout(location = 3) in vec3 inTangent;

// --- KIMENETEK (Varyings) ---
// Ezeket az adatokat küldjük tovább a Fragment Shader-nek (interpolálva).
layout(location = 0) out vec3 fragPos;         // Világbeli pozíció (fényekhez)
layout(location = 1) out vec3 fragNormal;      // Transzformált normálvektor
layout(location = 2) out vec2 fragTexCoord;    // UV koordináta
layout(location = 3) out vec4 fragPosLightSpace; // Pozíció a fény szemszögéből (árnyékhoz)
// ÚJ: Transzformált tangens vektor
layout(location = 4) out vec3 fragTangent;

// --- PUSH CONSTANTS ---
// Gyors adatátvitel a CPU-ról (MeshObject::draw hívásban).
// Max 128 byte, ami pont elég két 4x4-es mátrixnak.
layout(push_constant) uniform PushConstants {
    mat4 model; // Model -> World transzformáció
    mat4 mvp;   // Model -> View -> Projection (végleges képernyő pozícióhoz)
} push;

// --- FÉNY MÁTRIX SZÁMÍTÁS (Shadow Map) ---
// Ez a függvény kiszámolja a fény szemszögéből vett View-Projection mátrixot.
// Megjegyzés: Általában ezt CPU-n számoljuk és Uniform Bufferben küldjük át,
// de itt hardkódolva van a shaderben az egyszerűség/demonstráció kedvéért.
mat4 getLightSpaceMatrix() {
    vec3 lightPos = vec3(5.0, 5.0, 5.0); // Meg kell egyeznie a Fragment shaderrel!

    // LookAt mátrix manuális felépítése (Fény a (0,0,0) felé néz)
    vec3 f = normalize(vec3(0.0) - lightPos); // Előre (Forward)
    vec3 s = normalize(cross(f, vec3(0.0, 1.0, 0.0))); // Jobbra (Side)
    vec3 u = cross(s, f); // Fel (Up)

    // Oszlop-folytonos (Column-major) View Mátrix
    mat4 view = mat4(
    vec4(s.x, u.x, -f.x, 0.0),
    vec4(s.y, u.y, -f.y, 0.0),
    vec4(s.z, u.z, -f.z, 0.0),
    vec4(0.0, 0.0, 0.0, 1.0)
    );

    // Transzláció (eltolás) alkalmazása a View mátrixban
    view[3][0] = -dot(s, lightPos);
    view[3][1] = -dot(u, lightPos);
    view[3][2] = dot(f, lightPos);

    // Ortografikus Projection Mátrix manuális felépítése
    // Cél: [-10, 10] doboz leképezése [0, 1] mélységre (Vulkan standard)
    mat4 proj = mat4(1.0);
    proj[0][0] = 2.0 / 20.0; // 2 / (right - left) -> 2 / (10 - -10)
    proj[1][1] = 2.0 / 20.0; // 2 / (top - bottom)
    // Z-tengely: 1 / (near - far) -> 0..1 mapping Vulkanhoz (near=1.0, far=20.0)
    proj[2][2] = 1.0 / (1.0 - 20.0);
    proj[3][2] = 1.0 / (1.0 - 20.0) * 1.0; // near * proj[2][2]

    return proj * view;
}

void main() {
    // 1. Világkoordináta kiszámítása
    // Szükséges a pontos fény- és árnyékszámításhoz a Fragment shaderben
    vec4 worldPos = push.model * vec4(inPosition, 1.0);
    fragPos = worldPos.xyz;

    // 2. Normálvektor transzformálása
    // Csak a forgatást alkalmazzuk (mat3), az eltolást nem, mert a normálvektor csak irányt jelöl.
    fragNormal = normalize(mat3(push.model) * inNormal);

    // 3. Textúra koordináta továbbítása
    fragTexCoord = inTexCoord;

    // 4. Árnyék koordináta kiszámítása
    // A vertex pozícióját áttranszformáljuk a fényforrás koordináta-rendszerébe.
    // Ebből tudjuk majd, hogy a shadow map melyik pixelét kell olvasni.
    fragPosLightSpace = getLightSpaceMatrix() * worldPos;

    // 5. ÚJ: Tangens vektor transzformálása
    // Ugyanúgy forgatjuk, mint a normálvektort, hogy kövesse az objektum orientációját.
    // Ez kritikus a Normal Mapping helyes működéséhez forgó tárgyakon.
    fragTangent = normalize(mat3(push.model) * inTangent);

    // 6. Végső képernyő-pozíció (Clip Space)
    // A kamera szemszögéből transzformálva
    gl_Position = push.mvp * vec4(inPosition, 1.0);
}