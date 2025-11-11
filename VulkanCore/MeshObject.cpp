/**
 * @file MeshObject.cpp
 * @brief Megvalósítja a MeshObject osztály metódusait.
 *
 * Ez az osztály felelős a 3D geometria adatok Vulkan memóriába történő másolásáért
 * (vertex bufferek kezelése), valamint az objektum világtérbeli transzformációinak
 * (pozíció, forgatás) kezeléséért a rajzolási parancsok rögzítésekor.
 */
#include "MeshObject.h"
#include <stdexcept>
#include <cstring> // Memóriakezelési függvényekhez (memcpy)

/**
 * @brief Alapértelmezett konstruktor.
 */
MeshObject::MeshObject() = default;

/**
 * @brief Alapértelmezett destruktor.
 */
MeshObject::~MeshObject() = default;

/**
 * @brief Létrehozza a MeshObject Vulkan erőforrásait (vertex puffer).
 *
 * A feltöltés staging pufferen keresztül történik, ami biztosítja a memória
 * optimális elhelyezését a GPU-n (DEVICE_LOCAL_BIT).
 *
 * @param ctx Mutató a Vulkan környezet objektumra.
 * @param vertices A feltöltendő vertex adatok vektora (Pozíció, UV, stb.).
 */
void MeshObject::create(VulkanContext* ctx, const std::vector<float>& vertices) {
    this->context = ctx;
    // Kiszámítja a kirajzolandó csúcsok számát.
    // Mivel minden vertex 5 float (X,Y,Z,U,V), a teljes méretet osztjuk 5-tel.
    this->vertexCount = static_cast<uint32_t>(vertices.size() / 5);

    VkDeviceSize bufferSize = vertices.size() * sizeof(float);

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    // 1. Létrehozza a Staging Puffert (CPU-ról látható és másolható)
    context->createBuffer(
        bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // Átviteli forrásként való használat
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // CPU-ról elérhető (látható) és konzisztens
        stagingBuffer, stagingBufferMemory
    );

    // 2. Adatok másolása a Staging Pufferbe
    void* data;
    // Leképezi (map-eli) a memória egy részét a CPU számára elérhető címre.
    vkMapMemory(context->getDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
    // Átmásolja a vertex adatokat a leképezett memóriába.
    memcpy(data, vertices.data(), (size_t)bufferSize);
    // Feloldja (unmap-eli) a memória leképezését.
    vkUnmapMemory(context->getDevice(), stagingBufferMemory);

    // 3. Létrehozza a cél (Vertex) Puffert (GPU-ra optimalizált)
    context->createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, // Átviteli cél és vertex puffer
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, // GPU-specifikus (gyors) memória
        vertexBuffer, vertexBufferMemory
    );

    // 4. Staging Puffer tartalmának átmásolása a Cél Pufferbe
    context->copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    // 5. Staging Puffer felszabadítása (már nincs rá szükség)
    vkDestroyBuffer(context->getDevice(), stagingBuffer, nullptr);
    vkFreeMemory(context->getDevice(), stagingBufferMemory, nullptr);
}

/**
 * @brief Felszabadítja az objektum Vulkan erőforrásait.
 *
 * @param device A Vulkan logikai eszköz, amellyel az erőforrásokat létrehoztuk.
 */
void MeshObject::cleanup(VkDevice device) {
    // Csak akkor szabadít fel, ha a puffer ténylegesen létrejött.
    if (vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, vertexBufferMemory, nullptr);
    }
}

/**
 * @brief Rögzíti a rajzolási parancsokat a parancspufferbe.
 *
 * Kiszámolja a Model-View-Projection (MVP) mátrixot, Push Konstansként feltölti,
 * majd köti a vertex puffert és elindítja a rajzolási hívást (vkCmdDraw).
 *
 * @param commandBuffer A rögzítés alatt álló parancspuffer.
 * @param pipelineLayout A pipeline elrendezése, amely definiálja a Push Konstansokat.
 * @param animationTime A futásidő (másodpercben) az animációhoz.
 * @param viewProjection A Kamera (View) és a Perspektívikus (Projection) mátrix összevonva.
 * @param isWireframe Jelzi, hogy az objektumot wireframe módban rajzoljuk-e (nem használt ezen a szinten).
 */
void MeshObject::draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, float animationTime, const glm::mat4& viewProjection, bool isWireframe) const {
    if (vertexBuffer == VK_NULL_HANDLE || vertexCount == 0) return;

    // 1. Model Mátrix számítása
    glm::mat4 model = glm::mat4(1.0f);
    // Transzláció (elhelyezés a világban)
    model = glm::translate(model, position);

    // Forgatás hozzáadása, ha a forgási sebesség pozitív
    if (rotationSpeed > 0.0f) {
        // Forgatás: idő * sebesség (fok/másodperc) radiánra konvertálva, a tengely körül
        model = glm::rotate(model, animationTime * glm::radians(rotationSpeed), rotationAxis);
    }

    // 2. MVP Mátrix számítása
    // MVP = Projection * View * Model (oszlop-fő mátrix szorzás sorrendje)
    glm::mat4 mvp = viewProjection * model;

    // 3. Push Konstansok Feltöltése
    // A mátrix átadása közvetlenül a Vertex Shadernek a Push Konstans mechanizmuson keresztül.
    vkCmdPushConstants(
        commandBuffer,
        pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT, // A Vertex Shader számára küldjük
        0, // Eltolás (offset) a Push Konstans blokkban
        sizeof(mvp),
        &mvp
    );

    // 4. Vertex Puffer Kötése
    VkBuffer buffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    // Kösse a vertex puffert a 0. kötési ponthoz.
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

    // 5. Rajzolási Hívás (Draw Call)
    // Rajzolja ki az objektumot. A vertexCount-t használja a rajzolási primitívek számának meghatározására.
    vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
}