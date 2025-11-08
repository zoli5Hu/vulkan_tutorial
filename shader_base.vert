#version 450

//háromszög pontjainak elhelyezkedése
vec2 positions[3] = vec2[](
vec2(0.0, -0.5),
vec2(0.5, 0.5),
vec2(-0.5, 0.5)
);

//a main metódus mint egy for ciklus az összes vertexen végigmegy
void main() {
    //beállítjuk a pozíciót a háromszög pontjai alapján
    //a gl_VertexIndex változó automatikusan elérhető és a jelenlegi vertex indexét tartalmazza ez beépíttett
    //a W koordináta 1.0 értékre van állítva, hogy a pozíció homogén koordinátákban legyen megadva így valós 3D térben is értelmezhető
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}