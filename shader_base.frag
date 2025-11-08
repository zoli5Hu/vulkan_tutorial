#version 450
// 🔹 GLSL verzió megadása
layout(location = 0) out vec4 outColor;
// 🔹 layout(location = 0) → A kimeneti csatorna indexe, 0 az első (vagy egyetlen) render target
// 🔹 out → ez a változó a fragment shader kimenete, amit a framebufferbe ír
// 🔹 vec4 → 4 komponensű vektor (R, G, B, A)
// 🔹 outColor → változó neve, amit a main() belsejében állítunk be

// 🔹 Ez fut minden fragmentre (pixelre) a rasterizált primitívből
void main() {

    outColor = vec4(1.0, 0.0, 0.0, 1.0);
    // 🔹 Beállítja a pixel színét
    // 🔹 vec4(R, G, B, A)
    // 🔹 R = 1.0 → maximális piros
    // 🔹 G = 0.0 → nincs zöld
    // 🔹 B = 0.0 → nincs kék
    // 🔹 A = 1.0 → teljesen átlátszatlan
    // 🔹 GLSL-ben a színek lebegőpontos értékek 0.0–1.0 között, 255 helyett normalizálva
}
