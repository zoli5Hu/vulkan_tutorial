#version 450

// --- BEMENETEK (Vertex Shaderből) ---
// A vertex shaderből érkező interpolált adatok.
// A 'fragTangent' (location 4) elengedhetetlen a Normal Mappinghez.
layout(location = 0) in vec3 fragPos;          // Pixel világbeli pozíciója
layout(location = 1) in vec3 fragNormal;       // Geometriai normálvektor (interpolált)
layout(location = 2) in vec2 fragTexCoord;     // UV koordináták
layout(location = 3) in vec4 fragPosLightSpace; // A pixel pozíciója a fény szemszögéből (árnyékhoz)
layout(location = 4) in vec3 fragTangent;      // Érintő vektor (Tangent) - A TBN mátrix alapja

// --- KIMENET ---
layout(location = 0) out vec4 outColor;        // A pixel végső színe

// --- TEXTÚRÁK (Descriptor Sets) ---
// Set 0: Anyag textúrák (Diffuse, Roughness, Normal)
layout(set = 0, binding = 0) uniform sampler2D diffuseSampler;
layout(set = 0, binding = 1) uniform sampler2D roughnessSampler;
layout(set = 0, binding = 2) uniform sampler2D normalSampler; // Normal map (RGB = XYZ vektorok)

// Set 1: Árnyéktérkép (Külön set-ben, mert ez globális, nem anyagonként változik)
layout(set = 1, binding = 0) uniform sampler2D shadowMap;

/**
 * @brief Árnyékszámítás PCF (Percentage-Closer Filtering) technikával.
 * Lágyítja az árnyékok széleit és csökkenti a recésedést.
 */
float calculateShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    // 1. Perspektív osztás: [-w, w] tartományból [-1, 1]-be
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // 2. Transzformálás [0, 1] tartományba, hogy textúraként olvashassuk
    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    // Ha a Z koordináta > 1.0, akkor a pont távolabb van, mint a fény látómezője (nincs árnyék)
    if (projCoords.z > 1.0) {
        return 0.0;
    }

    // --- ADAPTÍV BIAS (Shadow Acne eltüntetése) ---
    // A felület dőlésszögétől függően állítjuk az eltolást.
    // Ha a fény laposan éri a felületet, nagyobb bias kell, merőlegesnél kisebb.
    float currentDepth = projCoords.z;
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);

    // --- PCF (Percentage-Closer Filtering) ---
    // A pixel körüli 3x3-as területet mintavételezzük, és átlagoljuk az eredményt.
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0); // Egy texel mérete uv térben

    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            // A szomszédos texel mélységi értéke a shadow map-ből
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;

            // Összehasonlítás: Ha a mi mélységünk (current) nagyobb mint a tárolt (pcf), akkor árnyékban vagyunk.
            // A bias-t levonjuk, hogy elkerüljük az önárnyékolási hibákat.
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0; // 9 minta átlaga (lágyítás)

    return shadow;
}

/**
 * @brief Egyszerűsített PBR-szerű világítási modell (Blinn-Phong).
 * A roughness textúrát használja a fényesség (specular) szabályozására.
 */
vec3 calcLight(vec3 lightPos, vec3 lightColor, vec3 normal, vec3 fragPos, vec3 viewDir, float roughness, bool useShadow, float shadowValue) {
    vec3 lightDir = normalize(lightPos - fragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir); // Blinn-Phong felezővektor

    // Diffuse komponens (Lambert)
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular komponens (Roughness alapú shininess)
    // Minél nagyobb a roughness, annál kisebb a shininess és a tükröződés.
    float shininess = (1.0 - roughness) * 64.0;
    float spec = pow(max(dot(normal, halfwayDir), 0.0), max(shininess, 0.001));
    vec3 specular = lightColor * spec * (1.0 - roughness) * 4.0; // Roughness csökkenti az intenzitást is

    // Árnyék faktor alkalmazása (csak a direkt fényekre hat, az ambientre nem)
    float shadowFactor = useShadow ? (1.0 - shadowValue) : 1.0;

    return shadowFactor * (diffuse + specular);
}

void main() {
    // 1. Textúrák mintavételezése
    vec3 objectColor = texture(diffuseSampler, fragTexCoord).rgb;
    float roughness = texture(roughnessSampler, fragTexCoord).g; // Roughness általában szürkeárnyalatos (G csatorna)
    roughness = pow(roughness, 2.0); // Gamma korrekció/erősítés a látványosabb hatáshoz

    // --- NORMAL MAPPING (TBN Mátrix építése) ---
    // A normal map RGB adatai [0, 1] tartományban vannak.
    // Ezt át kell alakítani [-1, 1] tartományba, hogy irányvektort kapjunk.
    vec3 normalMapValue = texture(normalSampler, fragTexCoord).rgb;
    normalMapValue = normalize(normalMapValue * 2.0 - 1.0);

    // Gram-Schmidt ortogonalizáció:
    // Biztosítjuk, hogy a Tangens (T) és a Normál (N) vektorok merőlegesek legyenek egymásra.
    // Ez korrigálja az interpolációs hibákat a háromszög felületén.
    vec3 N = normalize(fragNormal);   // Interpolált geometriai normál
    vec3 T = normalize(fragTangent);  // Interpolált tangens
    T = normalize(T - dot(T, N) * N); // T újraszámolása N-hez képest

    // Bitangens kiszámítása (Jobbkéz-szabály: N x T)
    vec3 B = cross(N, T);

    // TBN Mátrix: Tangent Space -> World Space transzformáció
    mat3 TBN = mat3(T, B, N);

    // A normal map-ből kiolvasott vektort átforgatjuk a világkoordináta-rendszerbe
    vec3 finalNormal = normalize(TBN * normalMapValue);

    // --- FÉNYSZÁMÍTÁS ---
    vec3 viewPos = vec3(0.0, 2.0, -8.0); // Ideiglenes: A C++ oldalról kéne jönnie (Uniform Buffer)
    vec3 viewDir = normalize(viewPos - fragPos);

    // 1. Fényforrás (Nap - Árnyékot vet)
    vec3 lightPos1 = vec3(5.0, 5.0, 5.0);
    vec3 lightColor1 = vec3(1.5, 1.2, 0.8); // Meleg napfény
    vec3 lightDir1 = normalize(lightPos1 - fragPos);

    // FONTOS: Az árnyék bias számításhoz az EREDETI geometriai normált (N) használjuk,
    // nem a normal map által módosítottat (finalNormal), különben műtermékek (artifact) jelennek meg.
    float shadow = calculateShadow(fragPosLightSpace, N, lightDir1);

    // A megvilágításhoz viszont már a részletgazdag finalNormal-t használjuk!
    vec3 lighting1 = calcLight(lightPos1, lightColor1, finalNormal, fragPos, viewDir, roughness, true, shadow);

    // 2. Fényforrás (Kék töltőfény - Nincs árnyék)
    vec3 lightPos2 = vec3(-5.0, 3.0, -5.0);
    vec3 lightColor2 = vec3(0.2, 0.4, 1.0) * 1.5; // Hideg kék fény
    vec3 lighting2 = calcLight(lightPos2, lightColor2, finalNormal, fragPos, viewDir, roughness, false, 0.0);

    // Ambient (Szórt) fény: Minimális alap megvilágítás
    vec3 ambient = 0.05 * objectColor;

    // Végső színösszeállítás
    vec3 result = ambient + (lighting1 + lighting2) * objectColor;

    outColor = vec4(result, 1.0);
}