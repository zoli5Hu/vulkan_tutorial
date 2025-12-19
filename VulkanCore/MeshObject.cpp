/**
 * @file MeshObject.cpp
 * @brief Megvalósítja a MeshObject osztály metódusait.
 */
#include "MeshObject.h"
#include <stdexcept>
#include <cstring>
#include <cmath> // A sin() függvényhez

MeshObject::MeshObject() = default;

MeshObject::~MeshObject() = default;

void MeshObject::create(VulkanContext* ctx, const std::vector<float>& vertices) {
    this->context = ctx;

    // --- JAVÍTÁS: 8 float van egy vertexben (Pos3 + Norm3 + UV2) ---
    // Ha 5-tel osztasz, "szemetet" olvas a GPU a végén -> széteső modell.
    this->vertexCount = static_cast<uint32_t>(vertices.size() / 8);

    VkDeviceSize bufferSize = vertices.size() * sizeof(float);

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    // Staging Buffer létrehozása
    context->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    // Adatok másolása
    void* data;
    vkMapMemory(context->getDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(context->getDevice(), stagingBufferMemory);

    // Vertex Buffer létrehozása
    context->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

    // Másolás Staging -> Vertex
    context->copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    vkDestroyBuffer(context->getDevice(), stagingBuffer, nullptr);
    vkFreeMemory(context->getDevice(), stagingBufferMemory, nullptr);
}

void MeshObject::cleanup(VkDevice device) {
    if (vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, vertexBufferMemory, nullptr);
    }
}

/**
 * @brief Frissített draw metódus textúra támogatással.
 */
void MeshObject::draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, float animationTime, const glm::mat4& viewProjection) const {
    if (vertexBuffer == VK_NULL_HANDLE || vertexCount == 0) return;

    // 1. Model Mátrix
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);

    if (rotationSpeed > 0.0f) {
        model = glm::rotate(model, animationTime * glm::radians(rotationSpeed), rotationAxis);
        // Megtartottam a pulzálást, amit az eredeti kódod tartalmazott:
        float scale = sin(animationTime) * 0.5f + 1.5f; // Kicsit szelídítettem a skálázáson, hogy ne legyen túl nagy
        model = glm::scale(model, glm::vec3(scale));
    }

    // 2. MVP Mátrix
    glm::mat4 mvp = viewProjection * model;

    // 3. Push Constants
    vkCmdPushConstants(
        commandBuffer,
        pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(mvp),
        &mvp
    );

    // --- ÚJ RÉSZ: Textúra Kötése ---
    // Ha van beállítva textúra descriptor set, akkor bekötjük a Binding 0-ra.
    if (textureDescriptorSet != VK_NULL_HANDLE) {
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            0, // First Set (0-s indexű set)
            1, // Descriptor Count
            &textureDescriptorSet,
            0,
            nullptr
        );
    }
    // -------------------------------

    // 4. Vertex Buffer
    VkBuffer buffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

    // 5. Rajzolás
    vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
}