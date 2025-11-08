#version 450


layout(location = 0) out vec3 fragColor;

//háromszög pontjainak elhelyezkedése
vec2 positions[3] = vec2[](
vec2(0.0, -0.5),
vec2(0.5, 0.5),
vec2(-0.5, 0.5)
);

//all cordinata add different value R ,G ,B
vec3 colors[3] = vec3[](
vec3(1.0, 0.0, 0.0),
vec3(0.0, 1.0, 0.0),
vec3(0.0, 0.0, 1.0)
);

//a main metódus mint egy for ciklus az összes vertexen végigmegy
void main() {
    //beállítjuk a pozíciót a háromszög pontjai alapján
    //a gl_VertexIndex változó automatikusan elérhető és a jelenlegi vertex indexét tartalmazza ez beépíttett
    //a W koordináta 1.0 értékre van állítva, hogy a pozíció homogén koordinátákban legyen megadva így valós 3D térben is értelmezhető
    //a gl_Position a beépített kimeneti változó amely a vertex pozícióját határozza meg
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    //beállítjuk a színt a háromszög pontjai alapján
    fragColor = colors[gl_VertexIndex];
}