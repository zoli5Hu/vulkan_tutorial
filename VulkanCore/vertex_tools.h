#pragma once

/**
 * Normálvektor számítása egy háromszöghöz a csúcspontjai (v1, v2, v3) alapján.
 * A normálvektor határozza meg a felület orientációját, ami alapvető a fényvisszaverődés kiszámításához.
 */
glm::vec3 calculateNormal(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3) {
    // Két élvektor meghatározása a háromszög síkján
    const glm::vec3 v1v2 = v1 - v2;
    const glm::vec3 v2v3 = v2 - v3;

    // A két élvektor vektoriális szorzata (cross product) egy, a síkra merőleges vektort ad
    const glm::vec3 cross = glm::cross(v1v2, v2v3);

    // Normalizálás, hogy a vektor hossza egységnyi (1.0) legyen
    return glm::normalize(cross);
}

/**
 * Tangens vektor számítása egy háromszöghöz.
 * Elengedhetetlen a Normal Mapping technológiához (TBN mátrix felépítése).
 * A tangens az UV tér 'U' (vízszintes) irányát jelöli a 3D térben.
 * * @param v1, v2, v3 A háromszög csúcspontjainak pozíciói
 * @param uv1, uv2, uv3 A csúcspontokhoz tartozó textúra (UV) koordináták
 */
glm::vec3 calculateTangent(
    const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3,
    const glm::vec2& uv1, const glm::vec2& uv2, const glm::vec2& uv3)
{
    // Pozíció különbségek (élek a 3D térben)
    glm::vec3 edge1 = v2 - v1;
    glm::vec3 edge2 = v3 - v1;

    // UV koordináta különbségek (eltolódás a textúra térben)
    glm::vec2 deltaUV1 = uv2 - uv1;
    glm::vec2 deltaUV2 = uv3 - uv1;

    // Skálázási tényező (determináns alapú), amely az UV tér és a 3D tér közötti arányt korrigálja
    float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

    // A tangens vektor komponenseinek kiszámítása a lineáris egyenletrendszer megoldásával
    glm::vec3 tangent;
    tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
    tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
    tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

    // Az eredmény normalizálása az irány megtartása mellett
    return glm::normalize(tangent);
}