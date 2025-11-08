#version 450
// 🔹 GLSL verzió megadása

// 🔹 Bemeneti változó a vertex shadertől (pl. interpolált szín)
// 🔹 layout(location = 0) → A bemeneti attribútum indexe
// 🔹 in → ez a változó a fragment shader bemenete
// 🔹 vec3 → 3 komponensű vektor (R, G, B)
layout(location = 0) in vec3 fragColor;

// 🔹 Kimeneti változó a framebuffer-be
// 🔹 layout(location = 0) → A kimeneti csatorna indexe, 0 az első (vagy egyetlen) render target
// 🔹 out → ez a változó a fragment shader kimenete
// 🔹 vec4 → 4 komponensű vektor (R, G, B, A)
// 🔹 outColor → változó neve, amit a main() belsejében állítunk be
layout(location = 0) out vec4 outColor;

// 🔹 Ez fut minden fragmentre (pixelre) a rasterizált primitívből
void main() {

    // 🔹 Beállítja a pixel kimeneti színét
    // 🔹 A bemeneti 'fragColor' (R, G, B) színt vesszük,
    //    és kiegészítjük egy Alpha (A) komponenssel (1.0)
    outColor = vec4(fragColor, 1.0);

    // 🔹 Az 'outColor' így vec4(fragColor.r, fragColor.g, fragColor.b, 1.0) lesz
    // 🔹 A = 1.0 → teljesen átlátszatlan
    // 🔹 GLSL-ben a színek lebegőpontos értékek 0.0–1.0 között
}