#version 450

// --- BEMENETEK ---
layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec4 fragPosLightSpace;

// --- KIMENET ---
layout(location = 0) out vec4 outColor;

// --- TEXTÚRÁK ---
layout(set = 0, binding = 0) uniform sampler2D diffuseSampler;
layout(set = 0, binding = 1) uniform sampler2D roughnessSampler;
layout(set = 1, binding = 0) uniform sampler2D shadowMap;

// --- ÁRNYÉK SZÁMÍTÁS ---
float calculateShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }

    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    // Kicsi bias a shadow acne ellen
    float bias = 0.001;

    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

    return shadow;
}

vec3 calcLight(vec3 lightPos, vec3 lightColor, vec3 normal, vec3 fragPos, vec3 viewDir, float roughness, bool useShadow, float shadowValue) {
    vec3 lightDir = normalize(lightPos - fragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    float shininess = (1.0 - roughness) * 64.0;
    float spec = pow(max(dot(normal, halfwayDir), 0.0), max(shininess, 0.001));

    // JAVÍTÁS: Roughness kiemelése!
    // A szorzót 2.0-ról 4.0-ra emeltem, így a fényes részek sokkal jobban csillognak.
    vec3 specular = lightColor * spec * (1.0 - roughness) * 4.0;

    float shadowFactor = useShadow ? (1.0 - shadowValue) : 1.0;

    return shadowFactor * (diffuse + specular);
}

void main() {
    vec3 objectColor = texture(diffuseSampler, fragTexCoord).rgb;
    float roughness = texture(roughnessSampler, fragTexCoord).g;

    // Kontraszt növelése a roughness mapen
    roughness = pow(roughness, 2.0);

    vec3 norm = normalize(fragNormal);
    vec3 viewPos = vec3(0.0, 2.0, -8.0);
    vec3 viewDir = normalize(viewPos - fragPos);

    // --- 1. FÉNY (Nap - Sárgás, Árnyékkal) ---
    vec3 lightPos1 = vec3(5.0, 5.0, 5.0);
    vec3 lightColor1 = vec3(1.5, 1.2, 0.8); // Meleg sárgás fény

    vec3 lightDir1 = normalize(lightPos1 - fragPos);
    float shadow = calculateShadow(fragPosLightSpace, norm, lightDir1);

    vec3 lighting1 = calcLight(lightPos1, lightColor1, norm, fragPos, viewDir, roughness, true, shadow);

    // --- 2. FÉNY (Kék - Erősebb, Nincs árnyék) ---
    vec3 lightPos2 = vec3(-5.0, 3.0, -5.0);

    // JAVÍTÁS: Kék fény felerősítése!
    // A szorzót 0.2-ről 1.5-re emeltem, hogy látványos legyen az ellenfény.
    vec3 lightColor2 = vec3(0.2, 0.4, 1.0) * 1.5;

    vec3 lighting2 = calcLight(lightPos2, lightColor2, norm, fragPos, viewDir, roughness, false, 0.0);

    vec3 ambient = 0.05 * objectColor;

    vec3 result = ambient + (lighting1 + lighting2) * objectColor;

    outColor = vec4(result, 1.0);
}