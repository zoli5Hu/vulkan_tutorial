// VulkanCore/MeshObject.cpp
#include "MeshObject.h"
#include <stdexcept>
#include <cstring>

MeshObject::MeshObject() = default;
MeshObject::~MeshObject() = default;

// Buffer létrehozása és adatok feltöltése (Staging Bufferrel)
void MeshObject::create(VulkanContext* ctx, const std::vector<float>& vertices) {
    this->context = ctx;
    // Feltételezzük, hogy 5 float/vertex van (X,Y,Z,U,V)
    this->vertexCount = static_cast<uint32_t>(vertices.size() / 5);

    VkDeviceSize bufferSize = vertices.size() * sizeof(float);

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    context->createBuffer(
        bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingBufferMemory
    );

    void* data;
    vkMapMemory(context->getDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(context->getDevice(), stagingBufferMemory);

    context->createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        vertexBuffer, vertexBufferMemory
    );
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

void MeshObject::draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, float animationTime, const glm::mat4& viewProjection, bool isWireframe) const {
    if (vertexBuffer == VK_NULL_HANDLE || vertexCount == 0) return;

    // --- Model Mátrix Kiszámítása (Transzformáció) ---
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);

    // Animálás
    if (rotationSpeed > 0.0f) {
        model = glm::rotate(model, animationTime * glm::radians(rotationSpeed), rotationAxis);
    }

    // Végső MVP (Model * View * Projection)
    glm::mat4 mvp = viewProjection * model;

    // Push Constant frissítése
    vkCmdPushConstants(
        commandBuffer,
        pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(mvp),
        &mvp
    );

    // Buffer Kötése
    VkBuffer buffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

    // Rajzolás
    vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
}