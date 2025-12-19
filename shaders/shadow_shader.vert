#version 450

// Csak a pozíció kell az árnyékhoz
layout(location = 0) in vec3 inPosition;

// Push Constant: MVP mátrix a fény szemszögéből (LightSpaceMatrix * Model)
layout(push_constant) uniform PushConstants {
    mat4 lightMVP;
} push;

void main() {
    gl_Position = push.lightMVP * vec4(inPosition, 1.0);
}