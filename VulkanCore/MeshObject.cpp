/**
 * @file MeshObject.cpp
 * @brief 3D objektum kezelése: Tangens számítás és 11 float/vertex (Pos+Norm+UV+Tan) struktúra.
 */
#include "MeshObject.h"
#include <stdexcept>
#include <cstring>
#include "vertex_tools.h"
#include <cmath>
#include <iostream>

MeshObject::MeshObject() = default;
MeshObject::~MeshObject() = default;

void MeshObject::create(VulkanContext* ctx, const std::vector<float>& vertices) {
    // Kontextus mentése a későbbi buffer műveletekhez
    this->context = ctx;

    // --- 1. Lépés: Tangensek kiszámítása és adatstruktúra konverzió ---
    // Az eredeti bemenet 8 float/vertex: Position(3), Normal(3), TexCoord(2)
    // A cél 11 float/vertex: Position(3), Normal(3), TexCoord(2), Tangent(3)
    std::vector<float> newVertices;

    if (!vertices.empty()) {
        // Memória előfoglalása a teljesítmény érdekében: (eredeti elemszám / 8) * 11
        newVertices.reserve((vertices.size() / 8) * 11);
    }

    // Vertexek száma az eredeti tömb alapján
    int vertexCountInput = static_cast<int>(vertices.size() / 8);

    // Háromszögenként iterálunk (3 vertex alkot egy primitívet) a tangens számításához
    for (int i = 0; i < vertexCountInput; i += 3) {
        // Vertex 1 adatok (Pozíció és UV koordináta a tangenshez elengedhetetlen)
        glm::vec3 v1(vertices[(i+0)*8 + 0], vertices[(i+0)*8 + 1], vertices[(i+0)*8 + 2]);
        glm::vec2 uv1(vertices[(i+0)*8 + 6], vertices[(i+0)*8 + 7]);

        // Vertex 2 adatok
        glm::vec3 v2(vertices[(i+1)*8 + 0], vertices[(i+1)*8 + 1], vertices[(i+1)*8 + 2]);
        glm::vec2 uv2(vertices[(i+1)*8 + 6], vertices[(i+1)*8 + 7]);

        // Vertex 3 adatok
        glm::vec3 v3(vertices[(i+2)*8 + 0], vertices[(i+2)*8 + 1], vertices[(i+2)*8 + 2]);
        glm::vec2 uv3(vertices[(i+2)*8 + 6], vertices[(i+2)*8 + 7]);

        // Tangens vektor kiszámítása a háromszög síkja és az UV tér eltolódása alapján
        glm::vec3 tangent = calculateTangent(v1, v2, v3, uv1, uv2, uv3);

        // Az új, bővített vertex puffer feltöltése
        for (int k = 0; k < 3; k++) {
            // Első 8 float (Pos, Norm, UV) másolása változtatás nélkül
            for (int j = 0; j < 8; j++) {
                newVertices.push_back(vertices[(i+k)*8 + j]);
            }
            // A frissen számított tangens (X, Y, Z) hozzáadása minden vertexhez a háromszögben
            newVertices.push_back(tangent.x);
            newVertices.push_back(tangent.y);
            newVertices.push_back(tangent.z);
        }
    }

    // A GPU felé küldendő vertexek száma (már a 11-es stride-dal számolva)
    this->vertexCount = static_cast<uint32_t>(newVertices.size() / 11);
    VkDeviceSize bufferSize = newVertices.size() * sizeof(float);

    // --- 2. Lépés: Vulkan erőforrások kezelése ---
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    // Ideiglenes (Staging) buffer létrehozása: CPU által írható, látható memória
    context->createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory
    );

    // Adatok feltöltése a staging bufferbe
    void* data;
    vkMapMemory(context->getDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, newVertices.data(), (size_t)bufferSize);
    vkUnmapMemory(context->getDevice(), stagingBufferMemory);

    // Végleges Vertex Buffer létrehozása: Csak a GPU számára elérhető (gyors) memória
    context->createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        vertexBuffer,
        vertexBufferMemory
    );

    // Adatátvitel: Staging Buffer -> Végleges Vertex Buffer
    context->copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    // Staging buffer felszabadítása (már nincs rá szükség a másolás után)
    vkDestroyBuffer(context->getDevice(), stagingBuffer, nullptr);
    vkFreeMemory(context->getDevice(), stagingBufferMemory, nullptr);
}

void MeshObject::cleanup(VkDevice device) {
    // GPU erőforrások biztonságos törlése
    if (vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, vertexBuffer, nullptr);
    }
    if (vertexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, vertexBufferMemory, nullptr);
    }
}

void MeshObject::setTexture(VkDescriptorSet descSet) {
    this->textureDescriptorSet = descSet;
}

// Push Constant struktúra: Illeszkednie kell a shaderben definiált layout-hoz (128 byte)
struct ObjectPushConstants {
    glm::mat4 model; // Model-világ mátrix (64 byte)
    glm::mat4 mvp;   // Model-View-Projection mátrix (64 byte)
};

void MeshObject::draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, glm::mat4 viewProjection, float animationTime) const {
    if (vertexCount == 0) return;

    // Vertex puffer bekötése a pipeline-ba
    VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    // --- 1. Transzformációs mátrixok összeállítása ---
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);

    // Dinamikus forgatás az idő függvényében
    if (rotationSpeed != 0.0f) {
        model = glm::rotate(model, glm::radians(rotationSpeed * animationTime), rotationAxis);
    }

    // Teljes MVP mátrix kiszámítása
    glm::mat4 mvp = viewProjection * model;

    // --- 2. Push Constants küldése a GPU-nak ---
    ObjectPushConstants pushs{};
    pushs.model = model;
    pushs.mvp = mvp;

    vkCmdPushConstants(
        commandBuffer,
        pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(ObjectPushConstants),
        &pushs
    );

    // --- 3. Textúra (Descriptor Set) bekötése ---
    if (textureDescriptorSet != VK_NULL_HANDLE) {
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            0, // Descriptor set index (Set 0)
            1,
            &textureDescriptorSet,
            0, nullptr
        );
    }

    // --- 4. Rajzolási parancs kiadása ---
    vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
}