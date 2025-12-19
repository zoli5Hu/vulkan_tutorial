#version 450

// 0: Pozíció (vec3)
layout(location = 0) in vec3 inPosition;
// 1: Normálvektor (vec3) - EZ HIÁNYZOTT!
layout(location = 1) in vec3 inNormal;
// 2: UV (vec2) - Ez eltolódott a 2-es helyre
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;

layout(push_constant) uniform PushConstants {
    mat4 model;
} push;

void main() {
    gl_Position = push.model * vec4(inPosition, 1.0);
    fragPos = vec3(push.model * vec4(inPosition, 1.0));

    // Normálvektor továbbítása a fragment shadernek
    fragNormal = mat3(push.model) * inNormal;
    fragTexCoord = inTexCoord;
}