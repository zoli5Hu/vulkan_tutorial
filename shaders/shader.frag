#version 450

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D diffuseSampler;
layout(binding = 1) uniform sampler2D roughnessSampler;

void main() {
    // 1. Alapszín
    vec3 objectColor = texture(diffuseSampler, fragTexCoord).rgb;

    // 2. Roughness (Érdesség)
    // ARM textúra: G csatorna = Roughness
    float roughness = texture(roughnessSampler, fragTexCoord).g;

    // --- TUNING ZÓNA: Itt csalunk, hogy jobban lásd! ---

    // A. Kontraszt növelése:
    // A roughness értékét négyzetre emeljük.
    // Eredmény: Ami eddig 0.5 (fél-érdes) volt, az 0.25 lesz (sokkal simább/fényesebb).
    // A nagyon érdes (1.0) marad 1.0. Ez kiemeli a fénylő részeket.
    roughness = pow(roughness, 2.0);

    // B. Fényforrás beállítása (legyen erősebb)
    vec3 lightPos = vec3(5.0, 5.0, 5.0);
    vec3 viewPos  = vec3(0.0, 2.0, -8.0);
    vec3 lightColor = vec3(1.5, 1.5, 1.5); // 1.0 helyett 1.5 (erősebb lámpa)

    // --- Számítások ---

    // Ambient
    vec3 ambient = 0.05 * lightColor; // Kicsit visszavettem, hogy a sötét sötétebb legyen

    // Diffuse
    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular (Tükröződés)
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);

    // C. Shininess (Fényesség) növelése:
    // A 32.0 helyett 256.0-t használunk.
    // Ez sokkal kisebb, tűélesebb fénypontot eredményez a sima részeken (mint a vizes szikla).
    // Az (1.0 - roughness) miatt az érdes részeken ez drasztikusan lecsökken.
    float shininess = (1.0 - roughness) * 256.0;

    float spec = pow(max(dot(norm, halfwayDir), 0.0), max(shininess, 0.001));

    // D. Specular Intenzitás szorzó:
    // Megszorozzuk 3.0-val a tükröződést, hogy "vakítson" a sima részeken.
    vec3 specular = lightColor * spec * (1.0 - roughness) * 3.0;

    // Végeredmény
    vec3 result = (ambient + diffuse + specular) * objectColor;

    outColor = vec4(result, 1.0);
}