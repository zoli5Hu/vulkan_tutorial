/**
 * @file MeshObject.cpp
 * @brief Frissítve: 128 byte-os Push Constant küldése (Model + MVP).
 */
#include "MeshObject.h"
#include <stdexcept>
#include <cstring>
#include <cmath>

MeshObject::MeshObject() = default;
MeshObject::~MeshObject() = default;

void MeshObject::create(VulkanContext* ctx, const std::vector<float>& vertices) {
    this->context = ctx;

    // FONTOS: 8 float van egy vertexben (Pos3 + Norm3 + UV2)
    this->vertexCount = static_cast<uint32_t>(vertices.size() / 8);

    VkDeviceSize bufferSize = vertices.size() * sizeof(float);

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    // Staging Buffer
    context->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    // Adatok másolása
    void* data;
    vkMapMemory(context->getDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(context->getDevice(), stagingBufferMemory);

    // Vertex Buffer
    context->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

    // Másolás Staging -> Vertex
    context->copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    vkDestroyBuffer(context->getDevice(), stagingBuffer, nullptr);
    vkFreeMemory(context->getDevice(), stagingBufferMemory, nullptr);
}

void MeshObject::cleanup(VkDevice device) {
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

// Segédstruktúra: Ez pontosan 128 byte (2 db 4x4-es mátrix)
// Ennek kell egyeznie a shader "push_constant" blokkjával.
struct ObjectPushConstants {
    glm::mat4 model; // 64 byte
    glm::mat4 mvp;   // 64 byte
};

void MeshObject::draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, glm::mat4 viewProjection, float animationTime) {
    if (vertexCount == 0) return;

    // Vertex Buffer bekötése
    VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    // 1. Model Mátrix kiszámítása (Forgatás, pozíció)
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);

    if (rotationSpeed != 0.0f) {
        model = glm::rotate(model, glm::radians(rotationSpeed * animationTime), rotationAxis);
    }

    // 2. MVP Mátrix (Kamera * Model)
    glm::mat4 mvp = viewProjection * model;

    // 3. Adatok küldése a Shadernek (Model + MVP)
    ObjectPushConstants pushs{};
    pushs.model = model;
    pushs.mvp = mvp;

    vkCmdPushConstants(
        commandBuffer,
        pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(ObjectPushConstants), // Fontos: 128 byte-ot küldünk!
        &pushs
    );

    // 4. Textúra bekötése (Set 0)
    if (textureDescriptorSet != VK_NULL_HANDLE) {
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            0, // Set 0
            1,
            &textureDescriptorSet,
            0, nullptr
        );
    }

    // 5. Rajzolás
    vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
}