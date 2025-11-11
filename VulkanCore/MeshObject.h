// VulkanCore/MeshObject.h
#pragma once

#include "VulkanContext.h"
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // A transzformációkhoz

class MeshObject {
public:
    MeshObject();
    ~MeshObject();

    // Létrehozza a vertex buffert és memóriát allokál
    void create(VulkanContext* context, const std::vector<float>& vertices);
    void cleanup(VkDevice device);

    // Kiszámítja az MVP mátrixot, megköti a buffert és elküldi a rajzolási parancsot
    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, float animationTime, const glm::mat4& viewProjection, bool isWireframe) const;

    // Transzformáció beállítások
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);
    float rotationSpeed = 0.0f; // 0.0f = statikus
private:
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    uint32_t vertexCount = 0;
    VulkanContext* context = nullptr;
};