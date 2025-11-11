#pragma once

#include "VulkanContext.h"
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class MeshObject {
public:
    MeshObject();
    ~MeshObject();

    void create(VulkanContext* context, const std::vector<float>& vertices);
    void cleanup(VkDevice device);

    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, float animationTime, const glm::mat4& viewProjection, bool isWireframe) const;

    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);
    float rotationSpeed = 0.0f;
private:
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    uint32_t vertexCount = 0;
    VulkanContext* context = nullptr;
};