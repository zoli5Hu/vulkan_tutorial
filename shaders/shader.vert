#version 450

// --- BEMENETEK (Input) ---
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

// --- KIMENETEK (Output) ---
layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec4 fragPosLightSpace;

// --- PUSH CONSTANTS ---
layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 mvp;
} push;

// --- FÉNY MÁTRIX SZÁMÍTÁS (JAVÍTVA) ---
mat4 getLightSpaceMatrix() {
    vec3 lightPos = vec3(5.0, 5.0, 5.0);

    vec3 f = normalize(vec3(0.0) - lightPos);
    vec3 s = normalize(cross(f, vec3(0.0, 1.0, 0.0)));
    vec3 u = cross(s, f);

    // JAVÍTÁS: A mátrix oszlop-folytonos (column-major).
    // Az előző verzióban a sorokat adtuk meg oszlopként, ami transzponálást okozott.
    // Helyes View Mátrix rotáció:
    // Col 0: s.x, u.x, -f.x, 0
    // Col 1: s.y, u.y, -f.y, 0
    // Col 2: s.z, u.z, -f.z, 0
    mat4 view = mat4(
    vec4(s.x, u.x, -f.x, 0.0),
    vec4(s.y, u.y, -f.y, 0.0),
    vec4(s.z, u.z, -f.z, 0.0),
    vec4(0.0, 0.0, 0.0, 1.0)
    );

    // Transzláció (elmozgatás)
    view[3][0] = -dot(s, lightPos);
    view[3][1] = -dot(u, lightPos);
    view[3][2] = dot(f, lightPos);

    // Projection Mátrix (Vulkan 0..1 depth range)
    // Near=1.0, Far=20.0
    mat4 proj = mat4(1.0);
    proj[0][0] = 2.0 / 20.0;
    proj[1][1] = 2.0 / 20.0;
    proj[2][2] = 1.0 / (1.0 - 20.0);
    proj[3][2] = 1.0 / (1.0 - 20.0);

    return proj * view;
}

void main() {
    // 1. Világkoordináta
    vec4 worldPos = push.model * vec4(inPosition, 1.0);
    fragPos = worldPos.xyz;

    // 2. Normálvektor
    fragNormal = mat3(push.model) * inNormal;

    // 3. Textúra koordináta
    fragTexCoord = inTexCoord;

    // 4. Árnyék koordináta (Most már a javított mátrixszal!)
    fragPosLightSpace = getLightSpaceMatrix() * worldPos;

    // 5. Végső képernyő pozíció
    gl_Position = push.mvp * vec4(inPosition, 1.0);
}